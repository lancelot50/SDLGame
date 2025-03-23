#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

#pragma comment(lib, "SDL3.lib")
#pragma comment(lib, "SDL3_ttf.lib")


struct DebugManager
{
    bool bShowObjectRect = false;
};
DebugManager DM;


int checkCollision(const SDL_FRect& rect1, const SDL_FRect& rect2)
{
    if (rect1.x + rect1.w >= rect2.x &&
        rect2.x + rect2.w >= rect1.x &&
        rect1.y + rect1.h >= rect2.y &&
        rect2.y + rect2.h >= rect1.y) {
        return 1;
    }
    return 0;
}

class Viewport
{
public :
    const int WIDTH = 1280;
    const int HEIGHT = 960;
};

struct Location
{
    float x = 0;
    float y = 0;
    void operator+=(const Location& In)
    {
        x += In.x;
        y += In.y;
    }
};
struct Rect
{
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};



class Texture
{
public:
    SDL_Texture* Tex = nullptr;
    float W = 0;
    float H = 0;
    Texture(SDL_Renderer* renderer, std::string Name, float Width, float Height)
    {
        SDL_Surface* bmp = SDL_LoadBMP(Name.c_str());
        Tex = SDL_CreateTextureFromSurface(renderer, bmp);
        SDL_DestroySurface(bmp);

        W = Width;
        H = Height;
    }
    ~Texture()
    {
        SDL_DestroyTexture(Tex);
    }
};

class Object
{
protected :
    SDL_FRect SrcRect = { 0,0,0,0 };
public:
    int dx = 0;
    int dy = 0;
    bool show = true;
    Location Loc;

    Texture* pTex=nullptr;

    virtual ~Object() { }

    int MapIndex=0;

    virtual void Update() {}
    void Init(Texture& Tex, int InitialMapIndex)
    {
        pTex = &Tex;
        MapIndex = InitialMapIndex;
    }

    SDL_FRect GetSrcRect() const { return SrcRect; }

    void MoveDelta(const Location& Delta)
    {
        Loc += Delta;
    }

    void SetLocation(const Location& NewLoc)
    {
        Loc = NewLoc;
    }
};


class Spearman : public Object
{
public :
    Spearman()
    {
        SrcRect= { 32,256, 32,32 };
    }
};

class Alien : public Object
{
public:
    void Update() override
    {
        Loc.x += dx;
    }
};

class Missile : public Object
{
public:
    void Update() override
    {
    }
};

struct ClickableArea
{
    SDL_FRect TexDestRect = { 0,0,0,0 };

    bool IsIn(float X, float Y)
    {
        if ((TexDestRect.x <= X && X <= TexDestRect.x + TexDestRect.w) && (TexDestRect.y <= Y && Y <= TexDestRect.y + TexDestRect.h))
            return true;
        else
            return false;
    }
};


struct Tile : public ClickableArea
{
    static const int Width = 16;
//    static const int Width = 32;
    int BitmapIdx=0;
    int MapIdx = 0;
    int Property=0;
    SDL_FRect TexSrcRect = { 0,0,0,0 };

    virtual bool CanPlaceHere() const { return true; }
};

struct Castle : public Tile
{
    bool CanPlaceHere() const override { return false; }
};

class Window;

class RenderInterface
{
protected:
    Viewport* _VP;
public:
    virtual RenderInterface* CreateRenderer(Viewport* VP) = 0;
    virtual void RenderText(const std::string& message, float x, float y) = 0;
    virtual void RenderObject(Object* obj, Tile* pTile) = 0;
    virtual void RenderTile(Tile* pTile, int X, int Y, int MapW, int MapH) = 0;
    virtual void RenderUI(Window* pWnd) = 0;
    virtual void RenderTexture(Texture* pTex, SDL_FRect* pDestRect) = 0;
    virtual void RenderBox(SDL_FRect* pFRect, Uint8 R, Uint8 G, Uint8 B, Uint8 A ) = 0;

    virtual void Destroy() = 0;

    virtual void PreRender() = 0;
    virtual void PostRender() = 0;

    virtual void* GetRenderer() = 0;

    Viewport* GetViewport() const { return _VP; }
};

class InputHandler
{
public:
    virtual bool HandleInput(const SDL_Event& event) = 0;
};

class SubSystem
{
public:
    virtual void Update() = 0;
    virtual void Render(RenderInterface* RI) = 0;
};


class Window : public SubSystem, public ClickableArea
{
    Texture* pTex = nullptr;

public :
//    SDL_FRect Size;
    std::string Title;
    Location TitleLoc;

    Window(const std::string& a_Title, const SDL_FRect& a_Size)
    {
        Title = a_Title;
        TexDestRect = a_Size;
        TitleLoc = { a_Size.x + a_Size.w/2, a_Size.y + 20 };
    }

    void Update() override {}
    void Render(RenderInterface* RI) override
    {
        RI->RenderUI(this);
        if (pTex)
            RI->RenderTexture(pTex, &TexDestRect);
        else
            RI->RenderBox(&TexDestRect, 255, 255, 255, 255);
        if(Title.length()>0)
            RI->RenderText(Title, TitleLoc.x, TitleLoc.y);
    }
};


class ResourceManager
{
    std::vector<Texture*> Data;
public:
    enum
    {
        ResID_SpaceShip = 0,
        ResID_Alien = 1,
        ResID_Missile = 2,
        ResID_Tile=3,
        ResID_Army=4,
    };
    void LoadResources(RenderInterface* RI)
    {
        SDL_Renderer* renderer = static_cast<SDL_Renderer*>(RI->GetRenderer());
        Texture* SpaceShip = new Texture(renderer, "spaceship.bmp", 100, 100);
        Data.push_back(SpaceShip);
        Texture* Alien = new Texture(renderer, "alien.bmp", 60, 60);
        Data.push_back(Alien);
        Texture* Missile = new Texture(renderer, "missile.bmp", 20, 50);
        Data.push_back(Missile);

        Texture* Tile = new Texture(renderer, "buch-outdoor.bmp", 384, 192);
//      Texture* Tile = new Texture(renderer, "buch-outdoor2x.bmp", 384*2, 192*2);
        Data.push_back(Tile);

        Texture* Army = new Texture(renderer, "Army.bmp", 448, 448);
        Data.push_back(Army);
    }
    ~ResourceManager()
    {
        for (auto& i : Data)
            i->~Texture();
    }

    Texture& GetTex(int ResID) const
    {
        return *Data[ResID];
    }
};
ResourceManager RM;




class Level : public SubSystem, public InputHandler
{
    Object* spaceship;
    std::vector<Object*> objects;

    static const int MapW = 20;
    static const int MapH = 20;
    int pMap[MapW*MapH] = 
    {
        0,  1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,
        20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,
        40 , 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
        144,149,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
        144,149,150,150,150,150,150,150,150,150,150,150,150,150,205,150,150,150,150,150,
        144,149,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
        144,149,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
        144,149,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
        144,149,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
        144,149,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,

        150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,144,
        150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,168,
        150,150,205,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,217,
        150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
        150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
        150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,205,150,150,150,150,
        150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
        150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
        150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
        150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
    };

    std::vector<Tile*> vTileMap;

public:
    int Width = 0;
    int Height = 0;

    void Init(const Viewport& VP)
    {
        Width = VP.WIDTH;
        Height = VP.HEIGHT;
      
        initMap();
    }

    void initMap()
    {
        Texture& mapTex = RM.GetTex(ResourceManager::ResID_Tile);

        for (int j = 0; j < MapH; ++j)
        {
            for (int i = 0; i < MapW; ++i)
            {
                int bitmapIdx = pMap[j * MapW + i];
                Tile* pTile = nullptr;
                if (bitmapIdx == 205)
                    pTile = new Castle();
                else
                    pTile=new Tile();
                pTile->BitmapIdx = bitmapIdx;
                pTile->MapIdx = j * MapW + i;

                int mapTexW = static_cast<int>(mapTex.W);
                int mapTileTexW = mapTexW / pTile->Width;
                float srcX = static_cast<float>((pTile->BitmapIdx * pTile->Width) % mapTexW);
                float srcY = static_cast<float>(pTile->BitmapIdx / mapTileTexW) * pTile->Width;
                pTile->TexSrcRect = { srcX, srcY, static_cast<float>(pTile->Width), static_cast<float>(pTile->Width) };

                float tileSize = pTile->Width * 3;
                float x = i * tileSize;
                float y = j * tileSize;
                pTile->TexDestRect = { x, y, tileSize, tileSize };

                vTileMap.push_back(pTile);
            }
        }
    }

    void CreateSpaceShip(Texture& Tex)
    {
        spaceship = new Object();
//        spaceship->Init(Tex, StartPos(), Rect{0,0,Width, Height});
        objects.push_back(spaceship);
    }
    void CreateAliens(Texture& Tex)
    {
        Alien* alien = new Alien();
    //  alien->Init(Tex, Location{ 50 + i * 70, 50 }, Rect{ 0,0,Width, Height });
        objects.push_back(alien);

    }

    void createSpearman(int MapIndex)
    {
        Spearman* spearman = new Spearman();
        spearman->Init(RM.GetTex(ResourceManager::ResID_Army), MapIndex);
        objects.push_back(spearman);
    }



    void Destroy()
    {
        for (Object* obj : objects)
            delete obj;
        objects.clear();

        for (auto& i : vTileMap)
            delete i;
        vTileMap.clear();
    }

    size_t GetObjNum() const {  return objects.size(); }

    bool PlayerMoveLeft()
    {
        return true;
    }
    bool PlayerMoveRight()
    {
        return true;
    }

    bool CreateMissile(Texture& Tex)
    {
        return true;
    }

    void updateCollision()
    {
        for (Object* obj : objects)
        {
            Missile* missile = dynamic_cast<Missile*>(obj);
            if (missile && missile->show)
            {
                for (Object* target : objects)
                {
                    Alien* alien = dynamic_cast<Alien*>(target);
                    if (alien && alien->show)
                    {
                        SDL_FRect missileRect = { missile->Loc.x, missile->Loc.y, missile->pTex->W, missile->pTex->H };
                        SDL_FRect alienRect = { alien->Loc.x, alien->Loc.y, alien->pTex->W, alien->pTex->H };
                        if (checkCollision(missileRect, alienRect))
                        {
                            missile->show = false;
                            alien->show = false;
                        }
                    }
                }
            }
        }
    }

    bool HandleInput(const SDL_Event& event)
    {
        bool isHandled = false;
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6)
                isHandled=PlayerMoveRight();
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4)
                isHandled = PlayerMoveLeft();
            if (event.key.key == SDLK_SPACE)
                isHandled = CreateMissile(RM.GetTex(ResourceManager::ResID_Missile));
            if (event.key.key == SDLK_F9)
            {
                DM.bShowObjectRect = !DM.bShowObjectRect;
                isHandled = true;
            }
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if(event.button.button == SDL_BUTTON_LEFT)
            {
                float x = event.button.x;
                float y = event.button.y;

                std::cout << "x:" << x << ", y:" << y << std::endl;
                for (auto& i : vTileMap)
                {
                    if (i->IsIn(x, y))
                    {
                        std::cout << "click map Index : " << i->MapIdx << std::endl;

                        bool exist = false;
                        for (auto& obj : objects)
                        {
                            if (obj->MapIndex == i->MapIdx)
                            {
                                std::cout << "Exist spearman!" << std::endl;
                                exist = true;
                                break;
                            }
                        }
                        if(!exist && i->CanPlaceHere())
                            createSpearman(i->MapIdx);
                    }
                }

            }
        }

        return isHandled;
    }

    void Update() override
    {
        for (Object* obj : objects)
            obj->Update();

        updateCollision();

        // show 플래그가 false인 객체들 삭제
        objects.erase(std::remove_if(objects.begin(), objects.end(), [](Object* obj) {
            if (!obj->show)
            {
                delete obj;
                return true;
            }
            return false;
        }), objects.end());
    }

    void Render(RenderInterface* RI) override
    {
        for (int j = 0; j < MapH; ++j)
        {
            for (int i = 0; i < MapW; ++i)
            {
                //Tile tile;
                //tile.BitmapIdx = pMap[j * MapW + i];
                //tile.MapIdx = j * MapW + i;
                RI->RenderTile(vTileMap[j* MapW + i],i,j, MapW, MapH);//  ;
            }
        }

        for (Object* obj : objects)
            RI->RenderObject(obj, vTileMap[obj->MapIndex]);
    }
};


class SDLRenderInterface : public RenderInterface
{
    SDL_Window* window;
    SDL_Renderer* renderer;

    TTF_Font* font;
    SDL_Color textColor = { 255, 255, 255, 255 }; // 흰색

public :
    SDLRenderInterface() {}

    RenderInterface* CreateRenderer(Viewport* VP) override
    {
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow("Spaceship Game", VP->WIDTH, VP->HEIGHT, 0);
        renderer = SDL_CreateRenderer(window, nullptr);
        _VP = VP;

        TTF_Init();
        font = TTF_OpenFont("arial.ttf", 12); // 폰트 파일은 실제 위치에 맞춰 지정해야 함

        return this;
    }
    void Destroy() override
    {
        TTF_CloseFont(font);
        TTF_Quit();

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
    }

    void RenderText(const std::string & message, float x, float y) override     // x,y 가 중앙
    {
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, message.c_str(), message.length(), textColor);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_FRect textRect = { static_cast<float>(x  - textSurface->w /2) , static_cast<float>(y- textSurface->h/2), static_cast<float>(textSurface->w), static_cast<float>(textSurface->h)};
        SDL_RenderTexture(renderer, textTexture, NULL, &textRect);

        SDL_DestroySurface(textSurface);
        SDL_DestroyTexture(textTexture);

        if (DM.bShowObjectRect)
        {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderRect(renderer, &textRect);
        }
    }

    void RenderObject(Object* Obj, Tile*pTile) override
    {
        SDL_FRect srcRect= Obj->GetSrcRect();
        SDL_RenderTexture(renderer, Obj->pTex->Tex, &srcRect, &pTile->TexDestRect);
        if (DM.bShowObjectRect)
        {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderRect(renderer, &pTile->TexDestRect);
        }
    }

    void RenderTile(Tile* pTile, int X, int Y, int MapW, int MapH) override
    {
        SDL_RenderTexture(renderer, RM.GetTex(ResourceManager::ResID_Tile).Tex , &pTile->TexSrcRect, &pTile->TexDestRect);
        if (DM.bShowObjectRect)
        {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderRect(renderer, &pTile->TexDestRect);
            std::string str=std::to_string(X)+","+std::to_string(Y)+" "+ std::to_string(pTile->BitmapIdx);
            RenderText(str, pTile->TexDestRect.x+20 , pTile->TexDestRect.y + 10 );
        }
    }

    void RenderUI(Window* pWnd) override
    {
    }

    void RenderTexture(Texture* pTex, SDL_FRect* pDestRect) override
    {
        SDL_FRect srcRect = { 0,0, static_cast<float>(pTex->W), static_cast<float>(pTex->H)};
        SDL_RenderTexture(renderer, pTex->Tex, &srcRect, pDestRect);
    }
    void RenderBox(SDL_FRect* pFRect, Uint8 R, Uint8 G, Uint8 B, Uint8 A) override
    {
        SDL_SetRenderDrawColor(renderer, R, G, B, A);
        SDL_RenderRect(renderer, pFRect);
    }




    void PreRender() override
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
    }

    void PostRender() override
    {
        SDL_RenderPresent(renderer);
    }

    void* GetRenderer() { return renderer;  }
};


class StateManager;
class GameState : public SubSystem, public InputHandler
{
    StateManager* pSM=nullptr;
public :
    GameState(StateManager* pStateMgr) { pSM = pStateMgr; }
    virtual void Init(const Viewport& VP) {}
    virtual void Destroy() {}
    virtual size_t GetObjNum() const { return 0; }
    void GotoMenuState();
    void GotoPlayingState();
};

class GameStatePlaying : public GameState
{
    Level Stage;
public :
    GameStatePlaying(StateManager* pSM) : GameState(pSM) {}
    void Init(const Viewport& VP)
    {
        Stage.Init(VP);
//        Stage.CreateSpaceShip(RM.GetTex(ResourceManager::ResID_SpaceShip));
//        Stage.CreateAliens(RM.GetTex(ResourceManager::ResID_Alien));
    }

    void Destroy()
    {
        Stage.Destroy();
    }

    size_t GetObjNum() const { return Stage.GetObjNum(); }

    void Update()
    {
        Stage.Update();
    }
    void Render(RenderInterface* RI)
    {
        Stage.Render(RI);
    }
    bool HandleInput(const SDL_Event& event)
    {
        bool isHandled=Stage.HandleInput(event);
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_F10)
            {
                GotoMenuState();
            }
        }
        return isHandled;
    }
};

class GameStateMenu : public GameState
{
    std::vector<Window*> vpWindowArray;
public :
    GameStateMenu(StateManager* pSM) : GameState(pSM)
    {
    }
    void Init(const Viewport& VP) override
    {
        const float menuWndW = 500;
        const float menuWndH = 500;
        Location start = { VP.WIDTH / 2 - menuWndW / 2, 100 };

        Window* pMenuWnd = new Window("Menu", { start.x, start.y, menuWndW, menuWndH });
        size_t len = pMenuWnd->Title.length();
        vpWindowArray.push_back(pMenuWnd);

        const float btnWndW = 200;
        const float btnWndH = 50;
        Location btnLoc = { pMenuWnd->TexDestRect.x + pMenuWnd->TexDestRect.w / 2 - btnWndW / 2, pMenuWnd->TexDestRect.y + 300 };
        Window* pReturnToGameWnd = new Window("Press ESC goes back to Play", { btnLoc.x, btnLoc.y, btnWndW, btnWndH });
        vpWindowArray.push_back(pReturnToGameWnd);
    }

    void Destroy() override
    {
        for (auto& pWnd : vpWindowArray)
            delete pWnd;
    }
    void Update() {}
    void Render(RenderInterface* RI)
    {
        for (auto& wnd : vpWindowArray)
            wnd->Render(RI);
    }
    bool HandleInput(const SDL_Event& event)
    {
        bool isHandled = false;
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_ESCAPE)
            {
                GotoPlayingState();
                isHandled = true;
            }
            if (event.key.key == SDLK_F9)
            {
                DM.bShowObjectRect = !DM.bShowObjectRect;
                isHandled = true;
            }
        }

        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                float x = event.button.x;
                float y = event.button.y;

                for (auto it=vpWindowArray.rbegin(); it!=vpWindowArray.rend(); ++it )
                {
                    if ((*it)->IsIn(x, y))
                    {
                        if ((*it)->Title == "Press ESC goes back to Play")
                        {
                            GotoPlayingState();
                            break;
                        }
                    }
                }
            }
        }



        return isHandled;
    }
};

class StateManager : public SubSystem, public InputHandler
{
    GameState* State;

    GameStatePlaying* pGameStatePlaying;
    GameStateMenu* pGameStateMenu;
public :
    StateManager()
    {
        pGameStatePlaying = new GameStatePlaying(this);
        pGameStateMenu = new GameStateMenu(this);
    }

    void Init(const Viewport& VP)
    {
        pGameStateMenu->Init(VP);
        pGameStatePlaying->Init(VP);
        State = pGameStatePlaying;
    }

    void Destroy()
    {
        State->Destroy();
    }

    size_t GetObjNum() const
    {
        return State->GetObjNum();
    }

    void GotoMenuState()
    {
        State = pGameStateMenu;
    }
    void GotoPlayingState()
    {
        State = pGameStatePlaying;
    }

    void Update()
    {
        State->Update();
    }

    void Render(RenderInterface* RI)
    {
        State->Render(RI);
    }

    bool HandleInput(const SDL_Event& event)
    {
        bool isHandled=State->HandleInput(event);
        return isHandled;
    }
};

void GameState::GotoMenuState()
{
    pSM->GotoMenuState();
}
void GameState::GotoPlayingState()
{
    pSM->GotoPlayingState();
}



class Game
{
    RenderInterface* RI;
    Viewport VP;
    
    StateManager StateMgr;

public:

private:
    class FPS : public SubSystem
    {
        Uint64 lastFrameTime = 0; // 마지막 프레임의 시간
        __int64 fps = 0; // 현재 FPS

    public:
        size_t ObjectCount = 0;

        FPS() { }
        ~FPS() { }
        void Update() override
        {
            Uint64 currentFrameTime = SDL_GetTicks();
            if (currentFrameTime - lastFrameTime > 0)
                fps = 1000 / (currentFrameTime - lastFrameTime); // 1000ms를 프레임 시간으로 나눔
            lastFrameTime = currentFrameTime;
        }
        void Render(RenderInterface* RI) override
        {
            Viewport* vp = RI->GetViewport();
            RI->RenderText("Object count: " + std::to_string(ObjectCount), static_cast<float>(vp->WIDTH-100), 10.f);
            RI->RenderText("FPS: " + std::to_string(fps), static_cast<float>(vp->WIDTH - 60), 40);
        }
    };

    FPS Fps;

    void initGameData()
    {
        RM.LoadResources(RI);
        StateMgr.Init(VP);
    }

    void init()
    {
        RI = new SDLRenderInterface();
        RI->CreateRenderer(&VP);
        initGameData();
    }

    int Update()
    {
        SDL_Event event;
        int quit = 0;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
                quit = 1;
            else
            {
                StateMgr.HandleInput(event);
            }
        }

        StateMgr.Update();

        Fps.ObjectCount = StateMgr.GetObjNum();
        Fps.Update(); 

        return quit;
    }

    void Render()
    {
        RI->PreRender();

        StateMgr.Render(RI);

        Fps.Render(RI);

        RI->PostRender();
    }

    void loop()
    {
        int quit = 0;
        while (!quit)
        {
            quit = Update();
            Render();
            SDL_Delay(10); // 일정한 프레임을 유지하기 위해
        }
    }

    void terminate()
    {
        StateMgr.Destroy();

        RI->Destroy();
        SDL_Quit();
    }

public:
    void Start()
    {
        init();
        loop();
        terminate();
    }
};

int main(int argc, char** argv)
{
    Game game;
    game.Start();
    return 0;
}

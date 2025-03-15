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


int checkCollision(const SDL_Rect& rect1, const SDL_Rect& rect2)
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
    int x = 0;
    int y = 0;
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
    int W = 0;
    int H = 0;
    Texture(SDL_Renderer* renderer, std::string Name, int Width, int Height)
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
public:
    int dx = 0;
    int dy = 0;
    bool show = true;
    Location Loc;

    Texture* pTex=nullptr;
    int TexWidth=0;
    int TexHeight=0;

    Rect rect;

    int ResourceID = 0;

    virtual ~Object() { }

    virtual void Update() {}
    void Init(Texture& Tex, const Location& InitLoc, const Rect& StageRect)
    {
        pTex = &Tex;

        Loc = InitLoc;
        rect = StageRect;
    }

    void MoveDelta(const Location& Delta)
    {
        Loc += Delta;
    }

    void SetLocation(const Location& NewLoc)
    {
        Loc = NewLoc;
    }
};

class Alien : public Object
{
public:
    void Update() override
    {
        Loc.x += dx;
        if (Loc.x-pTex->W/2 <= 0 || Loc.x+pTex->W/2 >= rect.w)
        {
            dx *= -1;
            Loc.y += 10;
        }
    }
};

class Missile : public Object
{
public:
    void Update() override
    {
        Loc.y += dy;
        if (Loc.y < 0)
            show = false;
    }
};

class RenderInterface
{
public:
    virtual RenderInterface* CreateRenderer(Viewport* VP) = 0;
    virtual void RenderText(const std::string& message, float x, float y) = 0;
    virtual void RenderObject(Object* obj) = 0;
    virtual void Destroy() = 0;

    virtual void PreRender() = 0;
    virtual void PostRender() = 0;

    virtual void* GetRenderer() = 0;
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



class ResourceManager
{
    std::vector<Texture*> Data;
public:
    enum
    {
        ResID_SpaceShip = 0,
        ResID_Alien = 1,
        ResID_Missile = 2,
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
    Location StartLoc;

    Object* spaceship;
    std::vector<Object*> objects;

public:
    int Width = 0;
    int Height = 0;

    void Init(const Viewport& VP)
    {
        Width = VP.WIDTH;
        Height = VP.HEIGHT;

        StartLoc=Location{ Width / 2, Height - 100 };
    }

    void CreateSpaceShip(Texture& Tex)
    {
        spaceship = new Object();
        spaceship->Init(Tex, StartPos(), Rect{0,0,Width, Height});
        objects.push_back(spaceship);
    }
    void CreateAliens(Texture& Tex)
    {
        for (int i = 0; i < 10; i++)
        {
            Alien* alien = new Alien();
            alien->Init(Tex, Location{ 50 + i * 70, 50 }, Rect{ 0,0,Width, Height });
            alien->dx = 1;
            objects.push_back(alien);
        }
    }

    void Destroy()
    {
        for (Object* obj : objects)
            delete obj;
        objects.clear();
    }

    const Location& StartPos() const { return StartLoc; }
    size_t GetObjNum() const {  return objects.size(); }

    bool PlayerMoveLeft()
    {
        spaceship->MoveDelta({ -30, 0 });
        return true;
    }
    bool PlayerMoveRight()
    {
        spaceship->MoveDelta({ 30, 0 });
        return true;
    }

    bool CreateMissile(Texture& Tex)
    {
        Missile* missile = new Missile();
        missile->Init(Tex, spaceship->Loc, Rect{0,0,Width, Height});
        missile->dy = -10;
        objects.push_back(missile);
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
                        SDL_Rect missileRect = { missile->Loc.x, missile->Loc.y, missile->pTex->W, missile->pTex->H };
                        SDL_Rect alienRect = { alien->Loc.x, alien->Loc.y, alien->pTex->W, alien->pTex->H };
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
        for (Object* obj : objects)
        {
            if (obj->show)
                RI->RenderObject(obj);
        }
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
    RenderInterface* CreateRenderer(Viewport* VP)
    {
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow("Spaceship Game", VP->WIDTH, VP->HEIGHT, 0);
        renderer = SDL_CreateRenderer(window, nullptr);

        TTF_Init();
        font = TTF_OpenFont("arial.ttf", 24); // 폰트 파일은 실제 위치에 맞춰 지정해야 함

        return this;
    }
    void Destroy()
    {
        TTF_CloseFont(font);
        TTF_Quit();

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
    }

    void RenderText(const std::string & message, float x, float y)
    {
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, message.c_str(), message.length(), textColor);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_FRect textRect = { x, y, static_cast<float>(textSurface->w), static_cast<float>(textSurface->h) };
        SDL_RenderTexture(renderer, textTexture, NULL, &textRect);

        SDL_DestroySurface(textSurface);
        SDL_DestroyTexture(textTexture);
    }

    void RenderObject(Object* Obj)
    {
        SDL_FRect texRect = { static_cast<float>(Obj->Loc.x - Obj->pTex->W / 2), static_cast<float>(Obj->Loc.y - Obj->pTex->H/2), static_cast<float>(Obj->pTex->W), static_cast<float>(Obj->pTex->H) };
        SDL_RenderTexture(renderer, Obj->pTex->Tex, nullptr, &texRect);
        if (DM.bShowObjectRect)
        {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderRect(renderer, &texRect);
        }
    }

    void PreRender()
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
    }

    void PostRender()
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
        Stage.CreateSpaceShip(RM.GetTex(ResourceManager::ResID_SpaceShip));
        Stage.CreateAliens(RM.GetTex(ResourceManager::ResID_Alien));
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
public :
    GameStateMenu(StateManager* pSM) : GameState(pSM) {}
    void Update() {}
    void Render(RenderInterface* RI)
    {
        const float menu_title_x = 500;
        const float menu_title_y = 200;
        RI->RenderText("Menu", menu_title_x, menu_title_y);
        RI->RenderText("Press ESC goes back to Play", menu_title_x-100, menu_title_y+50);
    }
    bool HandleInput(const SDL_Event& event)
    {
        bool isHandled = false;
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_ESCAPE)
            {
                GotoPlayingState();
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

        State = pGameStatePlaying;
    }

    void Init(const Viewport& VP)
    {
        State->Init(VP);
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
            RI->RenderText("Object count: " + std::to_string(ObjectCount), 10, 10);
            RI->RenderText("FPS: " + std::to_string(fps), 10, 40);
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

#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

// 충돌 처리 함수
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

class Level
{
    Location StartLoc;

public:
    int Width = 0;
    int Height = 0;

    void Init(const Viewport& VP)
    {
        Width = VP.WIDTH;
        Height = VP.HEIGHT;

        StartLoc=Location{ Width / 2, Height - 100 };
    }
    const Location& StartPos() const { return StartLoc; }
};

class Object
{
public:
    int dx = 0;
    int dy = 0;
    bool show = true;
    Location Loc;

    int ResourceID = 0;

    virtual ~Object() { }

    virtual void Update(const Level& CurLevel) {}
    void Init(int ResID, const Location& InitLoc)
    {
        ResourceID = ResID;
        Loc = InitLoc;
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
    void Update(const Level& CurLevel) override
    {
        Loc.x += dx;
        if (Loc.x <= 0 || Loc.x >= CurLevel.Width)
        {
            dx *= -1;
            Loc.y += 10;
        }
    }
};

class Missile : public Object
{
public:
    void Update(const Level& CurLevel) override
    {
        Loc.y += dy;
        if (Loc.y < 0)
            show = false;
    }
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
        SDL_FreeSurface(bmp);

        W = Width;
        H = Height;
    }
    ~Texture()
    {
        SDL_DestroyTexture(Tex);
    }
};

class RenderInterface
{
public :
    virtual RenderInterface* CreateRenderer(Viewport* VP) = 0;
    virtual void RenderText(const std::string& message, int x, int y) = 0;
    virtual void RenderObject(Object* obj, const Texture& Tex) = 0;
    virtual void Destroy()=0;

    virtual void PreRender() = 0;
    virtual void PostRender() = 0;

    virtual void* GetRenderer() = 0;
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
        window = SDL_CreateWindow("Spaceship Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, VP->WIDTH, VP->HEIGHT, 0);
        renderer = SDL_CreateRenderer(window, -1, 0);

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

    void RenderText(const std::string & message, int x, int y)
    {
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, message.c_str(), textColor);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_Rect textRect = { x, y, textSurface->w, textSurface->h };
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);
    }

    void RenderObject(Object* Obj, const Texture& Tex)
    {
        SDL_Rect texRect = { Obj->Loc.x - Tex.W / 2, Obj->Loc.y - Tex.H / 2, Tex.W, Tex.H };
        SDL_RenderCopy(renderer, Tex.Tex, NULL, &texRect);
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
        Texture* SpaceShip=new Texture(renderer, "spaceship.bmp", 100, 100);
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

    const Texture& GetTex(int ResID) const
    {
        return *Data[ResID];
    }
};


class SubSystem
{
    virtual void Update() = 0;
    virtual void Render(RenderInterface* RI) = 0;
};

class Game
{
    RenderInterface* RI;
    Viewport VP;
    Level Stage;
    ResourceManager RM;
public:

private:

    class FPS : public SubSystem
    {
        Uint32 lastFrameTime = 0; // 마지막 프레임의 시간
        int fps = 0; // 현재 FPS

    public:
        size_t ObjectCount = 0;

        FPS() { }
        ~FPS() { }
        void Update() override
        {
            Uint32 currentFrameTime = SDL_GetTicks();
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

    Object* spaceship;
    std::vector<Object*> objects;

    void initGameData()
    {
        Stage.Init(VP);
        RM.LoadResources(RI);

        spaceship = new Object();
        spaceship->Init(ResourceManager::ResID_SpaceShip, Stage.StartPos());
        objects.push_back(spaceship);

        for (int i = 0; i < 10; i++)
        {
            Alien* alien = new Alien();
            alien->Init(ResourceManager::ResID_Alien, Location{ 50 + i * 70, 50 });
            alien->dx = 1;
            objects.push_back(alien);
        }
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
            if (event.type == SDL_QUIT)
            {
                quit = 1;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_KP_6)
                    spaceship->MoveDelta({ 30, 0 });
                if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_KP_4)
                    spaceship->MoveDelta({ -30, 0 });
                if (event.key.keysym.sym == SDLK_SPACE)
                {
                    Missile* missile = new Missile();
                    missile->Init(ResourceManager::ResID_Missile, spaceship->Loc);
                    missile->dy = -10;
                    objects.push_back(missile);
                }
            }
        }

        for (Object* obj : objects)
        {
            obj->Update(Stage); // 모든 객체 업데이트
        }

        updateCollision(); // 충돌 처리

        // show 플래그가 false인 객체들 삭제
        objects.erase(std::remove_if(objects.begin(), objects.end(), [](Object* obj) {
            if (!obj->show)
            {
                delete obj;
                return true;
            }
            return false;
        }), objects.end());

        Fps.ObjectCount = objects.size();
        Fps.Update(); // FPS 계산

        return quit;
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
                        SDL_Rect missileRect = { missile->Loc.x, missile->Loc.y, RM.GetTex(missile->ResourceID).W, RM.GetTex(missile->ResourceID).H };
                        SDL_Rect alienRect = { alien->Loc.x, alien->Loc.y, RM.GetTex(alien->ResourceID).W, RM.GetTex(alien->ResourceID).H };
                        if( checkCollision(missileRect, alienRect))
                        {
                            missile->show = false;
                            alien->show = false;
                        }
                    }
                }
            }
        }
    }

    void Render()
    {
        RI->PreRender();

        for (Object* obj : objects)
        {
            if (obj->show)
            {
                const Texture& tex = RM.GetTex(obj->ResourceID);
                RI->RenderObject(obj, tex);
            }
        }

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
        for (Object* obj : objects)
        {
            delete obj;
        }
        objects.clear();

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

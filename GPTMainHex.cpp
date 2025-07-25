#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <cmath>
#include <fstream> // For file operations
#include <sstream> // For string stream operations

#pragma comment(lib, "SDL3.lib")
#pragma comment(lib, "SDL3_ttf.lib")

// --- Forward Declarations & Global Constants ---
class RenderInterface;
class GameState;
class Window;
class CastleInfoWnd;
class Castle;

const float HEX_SIDE_LENGTH = 24.0f;
const float HEX_FLAT_TOP_WIDTH = HEX_SIDE_LENGTH * 2.0f;
const float HEX_FLAT_TOP_HEIGHT = HEX_SIDE_LENGTH * sqrtf(3.0f);
const float HORIZONTAL_SPACING = HEX_FLAT_TOP_WIDTH * 0.75f;
const float VERTICAL_SPACING = HEX_FLAT_TOP_HEIGHT;
const float ODD_ROW_X_OFFSET = HEX_FLAT_TOP_WIDTH / 2.0f;
// --- End Forward Declarations ---

struct DebugManager
{
    bool bShowObjectRect = false;
};
DebugManager DM;

class Viewport
{
public:
    const int WIDTH = 1920;
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
    float  W = 0;
    float H = 0;
    Texture(SDL_Renderer* renderer, std::string Name, float Width, float Height)
    {
        SDL_Surface* bmp = SDL_LoadBMP(Name.c_str());
        if (!bmp) {
            std::cerr << "Failed to load BMP: " << Name << " - " << SDL_GetError() << std::endl;
            // Handle error, maybe load a placeholder or exit
        }
        Tex = SDL_CreateTextureFromSurface(renderer, bmp);
        if (!Tex) {
            std::cerr << "Failed to create texture from surface: " << Name << " - " << SDL_GetError() << std::endl;
        }
        SDL_DestroySurface(bmp);

        W = Width;
        H = Height;
    }
    ~Texture()
    {
        if (Tex) {
            SDL_DestroyTexture(Tex);
        }
    }
};

enum Faction
{
    Faction_None = 0,
    Faction_Wee = 1,
    Faction_Chok = 2,
    Faction_Oh = 3,
};

class Object
{
protected:

    SDL_FRect SrcRect = { 0,0,0,0 };
    Faction Fac = Faction_None;
public:
    std::string Name;
    int dx = 0;
    int dy = 0;
    bool show = true;
    Location Loc; // Not directly used for rendering objects on map tiles in this setup

    Texture* pTex = nullptr;

    virtual ~Object() {}

    int MapIndex = 0;

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
};

class Swordman : public Object
{
public:
    Swordman(Faction a_Fac)
    {
        Fac = a_Fac;
        SrcRect = { 0,192 + 32 * static_cast<float>(Fac), 32 ,32 };
    }
};

class Spearman : public Object
{
public:
    Spearman(Faction a_Fac)
    {
        Fac = a_Fac;
        SrcRect = { 32,192 + 32 * static_cast<float>(Fac), 32,32 };
    }
};
class Polearm : public Object
{
public:
    Polearm(Faction a_Fac)
    {
        Fac = a_Fac;
        SrcRect = { 64,192 + 32 * static_cast<float>(Fac), 32,32 };
    }
};

class Castle : public Object
{
public:
    int Gold = 0;
    int Food = 0;
private:
    int Order = 90;

    int Duration = 2700;

    int Soldier = 14000;
    int SoldierMorale = 90;

    int Spears = 0;
    int Polearms = 0;
    int Bows = 0;
    int Horses = 0;

    int GoldPerMonth = 500;
    int FoodPerSeason = 8000;

    int NumOfPerson = 0;

public:
    Castle(Faction a_Fac)
    {
        Fac = a_Fac;
        SrcRect = { 224,192 + 32 * static_cast<float>(Fac), 32,32 };
    }
    void InitData(std::string a_Name, int a_Gold, int a_Food)
    {
        Name = a_Name;
        Gold = a_Gold;
        Food = a_Food;
    }
};

class Alien : public Object
{
public:
    void Update() override
    {
    }
};

struct ClickableArea
{
    SDL_FRect  TexDestRect = { 0,0,0,0 };

    bool IsIn(float X, float Y)
    {
        if ((TexDestRect.x <= X && X <= TexDestRect.x + TexDestRect.w) &&
            (TexDestRect.y <= Y && Y <= TexDestRect.y + TexDestRect.h))
            return true;
        else
            return false;
    }
};

struct Tile : public ClickableArea
{
    static const int SourceBitmapTileSize = 16;
    int BitmapIdx = 0;
    int MapIdx = 0;
    int Property = 0;
    SDL_FRect TexSrcRect = { 0,0,0,0 };

    virtual bool CanPlaceHere() const { return true; }

    bool IsInHex(float px, float py, float hex_side_length)
    {
        float hex_bb_x = TexDestRect.x;
        float hex_bb_y = TexDestRect.y;
        float hex_bb_w = TexDestRect.w;
        float hex_bb_h = TexDestRect.h;

        float hex_center_x = hex_bb_x + hex_bb_w / 2.0f;
        float hex_center_y = hex_bb_y + hex_bb_h / 2.0f;

        float rel_x = px - hex_center_x;
        float rel_y = py - hex_center_y;

        if (abs(rel_x) > hex_bb_w / 2.0f || abs(rel_y) > hex_bb_h / 2.0f) {
            return false;
        }

        float half_height = hex_side_length * sqrtf(3.0f) / 2.0f;
        float outer_width_half = hex_side_length;

        return abs(rel_y) <= half_height && abs(rel_y) <= sqrtf(3.0f) * (outer_width_half - abs(rel_x));
    }
};

struct CastleTile : public Tile
{
    bool CanPlaceHere() const override { return false; }
};

enum class HAlign { Left, Center, Right };

class RenderInterface
{
protected:
    Viewport* _VP;
public:
    virtual RenderInterface* CreateRenderer(Viewport* VP) = 0;
    virtual void RenderText(const std::string& message, float x, float y, float availableWidth, HAlign align = HAlign::Left) = 0;
    virtual void RenderObject(Object* obj, Tile* pTile, bool bSelected) = 0;
    virtual void RenderTile(Tile* pTile, int X, int Y, int MapW, int MapH) = 0;
    virtual void RenderTexture(Texture* pTex, SDL_FRect* pDestRect) = 0;
    virtual void RenderBox(SDL_FRect* pFRect, Uint8 R, Uint8 G, Uint8 B, Uint8 A) = 0;

    virtual void Destroy() = 0;

    virtual void PreRender() = 0;
    virtual void PostRender() = 0;

    virtual void* GetRenderer() = 0;

    Viewport* GetViewport() const { return _VP; }
};

// --- Windowing System ---
class Window : public ClickableArea {
public:
    std::string Title;
    SDL_FRect Rect;
    RenderInterface* RI;
    bool bShow = true;
    Texture* pTex = nullptr;

    Window(const std::string& a_Title, const SDL_FRect& a_Rect, RenderInterface* a_RI)
        : Title(a_Title), Rect(a_Rect), RI(a_RI) {
        TexDestRect = Rect;
    }

    virtual ~Window() {}

    void SetTexture(Texture* a_pTex) {
        pTex = a_pTex;
    }

    void SetPosition(float x, float y) {
        Rect.x = x;
        Rect.y = y;
        TexDestRect.x = x;
        TexDestRect.y = y;
    }

    virtual void Render(RenderInterface* a_RI) {
        if (!bShow) return;

        if (pTex) {
            a_RI->RenderTexture(pTex, &Rect);
        }
        else {
            // Draw a default window box
            a_RI->RenderBox(&Rect, 100, 100, 100, 200);
        }
        if (!Title.empty()) {
            float centerX = Rect.x + Rect.w / 2.0f;
            a_RI->RenderText(Title, Rect.x, Rect.y + 5, Rect.w, HAlign::Center);
        }
    }

    virtual void Execute(GameState* pState) {
        // Default implementation does nothing
    }
};

class CastleInfoWnd : public Window {
public:
    Castle* pCastle = nullptr;

    CastleInfoWnd(const std::string& a_Title, const SDL_FRect& a_Rect, RenderInterface* a_RI)
        : Window(a_Title, a_Rect, a_RI) {}

    void Init(Castle* a_pCastle) {
        pCastle = a_pCastle;
    }

    // Overload for initial setup
    void Init(const std::string& castleName, int gold, int food) {
        // This is a bit of a hack since we don't have a real castle object yet.
    }

    void Render(RenderInterface* a_RI) override {
        if (!bShow) return;

        Window::Render(a_RI); // Render window background

        if (pCastle) {
            float currentY = Rect.y + 10; // Start with top padding
            const float lineSpacing = 20; // Adjust as needed
            const float centerX = Rect.x + Rect.w / 2.0f;
            const float leftX = Rect.x + 10; // Left padding

            std::string name = pCastle->Name;
            std::string gold = u8"골드: " + std::to_string(pCastle->Gold);
            std::string food = u8"식량: " + std::to_string(pCastle->Food);

            a_RI->RenderText(name, leftX, currentY, 0.0f, HAlign::Left);
            currentY += lineSpacing;
            a_RI->RenderText(gold, leftX, currentY, 0.0f, HAlign::Left);
            currentY += lineSpacing;
            a_RI->RenderText(food, leftX, currentY, 0.0f, HAlign::Left);
        }
    }
};
// --- End Windowing System ---

class ResourceManager
{
    std::vector<Texture*> Data;
public:
    enum
    {
        ResID_SpaceShip = 0,
        ResID_Alien = 1,
        ResID_Tile = 2,
        ResID_Army = 3,
        ResID_GameMenu = 4,
        ResID_CastleMenu = 5,
    };
    void LoadResources(RenderInterface* RI)
    {
        SDL_Renderer* renderer = static_cast<SDL_Renderer*>(RI->GetRenderer());
        Data.push_back(new Texture(renderer, "spaceship.bmp", 100, 100));
        Data.push_back(new Texture(renderer, "alien.bmp", 60, 60));
        Data.push_back(new Texture(renderer, "buch-outdoor.bmp", 384, 192));
        Data.push_back(new Texture(renderer, "Army.bmp", 448, 448));
        Data.push_back(new Texture(renderer, "GameMenu.bmp", 150, 208));
        Data.push_back(new Texture(renderer, "CastleMenu.bmp", 88, 214));
    }
    ~ResourceManager()
    {
        for (auto& i : Data)
            delete i;
        Data.clear();
    }

    Texture& GetTex(int ResID) const
    {
        return *Data[ResID];
    }
};

ResourceManager RM; // Define RM here, after ResourceManager class

class SDLRenderInterface : public RenderInterface
{
    SDL_Window* window;
    SDL_Renderer* renderer;

    TTF_Font* font;
    SDL_Color textColor = { 255, 255, 255, 255 }; // White color for text

public:
    SDLRenderInterface() : window(nullptr), renderer(nullptr), font(nullptr) {}

    RenderInterface* CreateRenderer(Viewport* VP) override
    {
        SDL_Init(SDL_INIT_VIDEO);

        window = SDL_CreateWindow("Hexagon Map Game", VP->WIDTH, VP->HEIGHT, 0);
        if (!window) {
            std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return nullptr;
        }

        renderer = SDL_CreateRenderer(window, nullptr);
        if (!renderer) {
            std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window);
            SDL_Quit();
            return nullptr;
        }
        _VP = VP;

        if (TTF_Init() == false) {
            std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return nullptr;
        }

        font = TTF_OpenFont("NotoSansKR-Medium.ttf", 12);
        if (!font) {
            std::cerr << "Failed to load font: NotoSansKR-Medium.ttf - " << SDL_GetError() << std::endl;
        }

        return this;
    }

    void Destroy() override
    {
        if (font) {
            TTF_CloseFont(font);
            font = nullptr;
        }
        TTF_Quit();

        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        SDL_Quit();
    }

    void RenderText(const std::string& message, float x, float y, float availableWidth, HAlign align = HAlign::Left) override
    {
        SDL_FRect textRect;
        SDL_Texture* textTexture = CreateTextTexture(message, &textRect, x, y);
        if (!textTexture) {
            return;
        }

        float renderX = x;
        if (align == HAlign::Center) {
            renderX = x + (availableWidth - textRect.w) / 2.0f;
        } else if (align == HAlign::Right) {
            renderX = x + availableWidth - textRect.w;
        }
        textRect.x = renderX;

        SDL_RenderTexture(renderer, textTexture, nullptr, &textRect); // Changed NULL to nullptr
        SDL_DestroyTexture(textTexture);
        if (DM.bShowObjectRect)
        {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderRect(renderer, &textRect);
        }
    }

    void RenderObject(Object* Obj, Tile* pTile, bool bSelectedIndex) override
    {
        SDL_FRect srcRect = Obj->GetSrcRect();
        SDL_RenderTexture(renderer, Obj->pTex->Tex, &srcRect, &pTile->TexDestRect);

        if (bSelectedIndex)
        {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderRect(renderer, &pTile->TexDestRect);
        }
        if (DM.bShowObjectRect)
        {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderRect(renderer, &pTile->TexDestRect);
        }
    }

    void RenderTile(Tile* pTile, int X, int Y, int MapW, int MapH) override
    {
        SDL_RenderTexture(renderer, RM.GetTex(ResourceManager::ResID_Tile).Tex, &pTile->TexSrcRect, &pTile->TexDestRect);

        if (DM.bShowObjectRect) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 255, 200);

            float hex_center_x = pTile->TexDestRect.x + pTile->TexDestRect.w / 2.0f;
            float hex_center_y = pTile->TexDestRect.y + pTile->TexDestRect.h / 2.0f;

            float s = HEX_SIDE_LENGTH;
            float h_half = HEX_SIDE_LENGTH * sqrtf(3.0f) / 2.0f;

            SDL_FPoint vertices[6];
            vertices[0] = { hex_center_x + s,          hex_center_y };
            vertices[1] = { hex_center_x + s * 0.5f,   hex_center_y + h_half };
            vertices[2] = { hex_center_x - s * 0.5f,   hex_center_y + h_half };
            vertices[3] = { hex_center_x - s,          hex_center_y };
            vertices[4] = { hex_center_x - s * 0.5f,   hex_center_y - h_half };
            vertices[5] = { hex_center_x + s * 0.5f,   hex_center_y - h_half };

            for (int k = 0; k < 6; ++k) {
                SDL_RenderLine(renderer, vertices[k].x, vertices[k].y,
                    vertices[(k + 1) % 6].x, vertices[(k + 1) % 6].y);
            }
        }
        if (DM.bShowObjectRect)
        {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderRect(renderer, &pTile->TexDestRect);
            std::string str = std::to_string(X) + "," + std::to_string(Y) + " " + std::to_string(pTile->BitmapIdx);
            RenderText(str, pTile->TexDestRect.x + pTile->TexDestRect.w / 2.0f, pTile->TexDestRect.y + pTile->TexDestRect.h / 2.0f, 0.0f, HAlign::Center);
        }
    }

    void RenderTexture(Texture* pTex, SDL_FRect* pDestRect) override
    {
        SDL_FRect srcRect = { 0,0, static_cast<float>(pTex->W), static_cast<float>(pTex->H) };
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

    void* GetRenderer() { return renderer; }

    SDL_Texture* CreateTextTexture(const std::string& message, SDL_FRect* outRect, float x, float y)
    {
        if (!font) {
            std::cerr << "Cannot create text texture: Font not loaded." << std::endl;
            return nullptr;
        }
        
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, message.c_str(), strlen(message.c_str()), textColor);
        if (!textSurface) {
            std::cerr << "Failed to create text surface: " << SDL_GetError() << std::endl;
            return nullptr;
        }
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (!textTexture) {
            std::cerr << "Failed to create text texture: " << SDL_GetError() << std::endl;
            SDL_DestroySurface(textSurface);
            return nullptr;
        }

        *outRect = { x, y, static_cast<float>(textSurface->w), static_cast<float>(textSurface->h) };

        SDL_DestroySurface(textSurface);
        return textTexture;
    }
};

class SubSystem
{
public:
    virtual void Update() = 0;
    virtual void Render(RenderInterface* RI) = 0; // Forward declare RenderInterface
};

class InputHandler
{
public:
    virtual bool HandleInput(const SDL_Event& event) = 0;
};

class Level : public SubSystem, public InputHandler
{
    Object* spaceship;
    std::vector<Object*> objects;

    static const int MapW = 20;
    static const int MapH = 20;
    std::vector<int> pMap; // Changed to dynamic array

    std::vector<Tile*> vTileMap;

public:
    int Width = 0;
    int Height = 0;

    int SelectedIndex = -1;

    void Init(const Viewport& VP)
    {
        Width = VP.WIDTH;
        Height = VP.HEIGHT;

        initMap();
    }

    void initMap()
    {
        pMap.clear(); // Clear existing map data
        std::ifstream mapFile("map.txt");
        if (!mapFile.is_open()) {
            std::cerr << "Failed to open map.txt" << std::endl;
            // Handle error, maybe load a default map or exit
            return;
        }

        std::string line;
        while (std::getline(mapFile, line)) {
            std::stringstream ss(line);
            std::string segment;
            while (std::getline(ss, segment, ',')) {
                pMap.push_back(std::stoi(segment));
            }
        }
        mapFile.close();

        Texture& mapTex = RM.GetTex(ResourceManager::ResID_Tile);

        for (int j = 0; j < MapH; ++j)
        {
            for (int i = 0; i < MapW; ++i)
            {
                int mapIdx = j * MapW + i;
                int bitmapIdx = pMap[mapIdx];
                Tile* pTile = nullptr;

                if (bitmapIdx == 202)
                {
                    pTile = new CastleTile();
                    static int createdFaction = Faction_Wee;
                    createCastle(mapIdx, static_cast<Faction>(createdFaction++));
                    std::cout << "Created castle at map index: " << mapIdx << std::endl;
                }
                else
                    pTile = new Tile();
                pTile->BitmapIdx = bitmapIdx;
                pTile->MapIdx = mapIdx;

                int mapTexW = static_cast<int>(mapTex.W);
                int mapTileTexW = mapTexW / Tile::SourceBitmapTileSize;
                float srcX = static_cast<float>((pTile->BitmapIdx % mapTileTexW) * Tile::SourceBitmapTileSize);
                float srcY = static_cast<float>((pTile->BitmapIdx / mapTileTexW) * Tile::SourceBitmapTileSize);
                pTile->TexSrcRect = { srcX, srcY, static_cast<float>(Tile::SourceBitmapTileSize), static_cast<float>(Tile::SourceBitmapTileSize) };

                float destX = i * HORIZONTAL_SPACING;
                float destY = j * VERTICAL_SPACING;

                if (j % 2 != 0) {
                    destX += ODD_ROW_X_OFFSET;
                }

                pTile->TexDestRect = { destX, destY, HEX_FLAT_TOP_WIDTH, HEX_FLAT_TOP_HEIGHT };

                vTileMap.push_back(pTile);
            }
        }
    }

    void CreateSpaceShip(Texture& Tex)
    {
        spaceship = new Object();
        objects.push_back(spaceship);
    }
    void CreateAliens(Texture& Tex)
    {
        Alien* alien = new Alien();
        objects.push_back(alien);
    }

    void createCastle(int MapIndex, Faction a_Fac)
    {
        Castle* castle = new Castle(a_Fac);
        castle->Init(RM.GetTex(ResourceManager::ResID_Army), MapIndex);

        switch (MapIndex)
        {
        case 94:
            castle->InitData(u8"허창", 2200, 22000);
            break;
        case 241:
            castle->InitData(u8"성도", 4400, 47400);
            break;
        case 315:
            castle->InitData(u8"낙양", 5000, 454000);
            break;
        default:
            castle->InitData(u8"디폴트", 1000, 10000);
            break;
        }
        objects.push_back(castle);
    }

    void createSpearman(int MapIndex, Faction Fac)
    {
        Spearman* spearman = new Spearman(Fac);
        spearman->Init(RM.GetTex(ResourceManager::ResID_Army), MapIndex);
        objects.push_back(spearman);
    }

    bool SetCastleInfoWnd(class Window* pWnd) // Forward declare Window
    {
        CastleInfoWnd* pCastleInfoWnd = static_cast<CastleInfoWnd*>(pWnd);
        for (Object* obj : objects)
        {
            if (obj->MapIndex == SelectedIndex && dynamic_cast<Castle*>(obj))
            {
                Castle* pCastle = static_cast<Castle*>(obj);
                static_cast<CastleInfoWnd*>(pWnd)->Init(pCastle);
                return true;
            }
        }
        return false;
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

    size_t GetObjNum() const { return objects.size(); }

    bool PlayerMoveLeft()
    {
        return true;
    }
    bool PlayerMoveRight()
    {
        return true;
    }

    bool HandleInput(const SDL_Event& event)
    {
        bool isHandled = false;
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6)
                isHandled = PlayerMoveRight();
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4)
                isHandled = PlayerMoveLeft();
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
                float x = static_cast<float>(event.button.x);
                float y = static_cast<float>(event.button.y);

                std::cout << "Mouse Click at screen coordinates: x=" << x << ", y=" << y << std::endl;

                for (auto& pTile : vTileMap)
                {
                    if (pTile->IsInHex(x, y, HEX_SIDE_LENGTH))
                    {
                        std::cout << "Click detected in hexagonal map tile with index: " << pTile->MapIdx << std::endl;
                        SelectedIndex = pTile->MapIdx;
                        isHandled = true;
                        break;
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
                RI->RenderTile(vTileMap[j * MapW + i], i, j, MapW, MapH);
            }
        }

        for (Object* obj : objects)
            RI->RenderObject(obj, vTileMap[obj->MapIndex], SelectedIndex == obj->MapIndex);
    }
};

class StateManager; // Forward declaration for GameState

class GameState : public SubSystem, public InputHandler
{
protected:
    StateManager* pSM = nullptr;
public:
    GameState(StateManager* pStateMgr) { pSM = pStateMgr; }
    virtual void Init(const Viewport& VP, RenderInterface* RI) = 0;
    virtual void Destroy() = 0;
    virtual size_t GetObjNum() const { return 0; }
    void GotoMenuState();
    void GotoPlayingState();
};

class GameStatePlaying : public GameState
{
    Level Stage;

    int lastSelectedIndex = -1;

    std::vector<Window*> vpWindowArray;
    CastleInfoWnd* pCastleInfoWnd = nullptr;
    Window* pCastleMenuWnd = nullptr;
public:
    GameStatePlaying(StateManager* pSM) : GameState(pSM) {}
    void Init(const Viewport& VP, RenderInterface* RI) override
    {
        Stage.Init(VP);

        pCastleInfoWnd = new CastleInfoWnd(u8"성 정보", { static_cast<float>(VP.WIDTH - 300), 100.f, 230.f, 300.f }, RI);
        pCastleInfoWnd->Init(u8"허창", 1000, 2000);
        vpWindowArray.push_back(pCastleInfoWnd);

        pCastleMenuWnd = new Window("", { static_cast<float>(VP.WIDTH - 300), 400.f, 88.f, 214.f }, RI);
        pCastleMenuWnd->SetTexture(&RM.GetTex(ResourceManager::ResID_CastleMenu));
        pCastleMenuWnd->bShow = false;
        vpWindowArray.push_back(pCastleMenuWnd);
    }

    void Destroy() override
    {
        Stage.Destroy();
        for (auto& wnd : vpWindowArray)
            delete wnd;
        vpWindowArray.clear();
        pCastleInfoWnd = nullptr;
        pCastleMenuWnd = nullptr;
    }

    size_t GetObjNum() const override { return Stage.GetObjNum(); }

    void Update() override
    {
        Stage.Update();
    }
    void Render(RenderInterface* RI) override
    {
        Stage.Render(RI);
        for (auto& wnd : vpWindowArray)
            wnd->Render(RI);
    }
    bool HandleInput(const SDL_Event& event) override
    {
        bool isHandled = false;

        isHandled = Stage.HandleInput(event);

        if (Stage.SelectedIndex != -1) {
            bool bCastleFound = Stage.SetCastleInfoWnd(pCastleInfoWnd);
            if (bCastleFound) {
                pCastleInfoWnd->bShow = true;

                if (lastSelectedIndex != Stage.SelectedIndex) {
                    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
                        float x = static_cast<float>(event.button.x);
                        float y = static_cast<float>(event.button.y);
                        pCastleMenuWnd->SetPosition(x, y);
                        pCastleMenuWnd->bShow = true;
                    }
                }
            } else {
                pCastleInfoWnd->bShow = false;
                pCastleMenuWnd->bShow = false;
            }
        } else {
            pCastleInfoWnd->bShow = false;
            pCastleMenuWnd->bShow = false;
        }

        lastSelectedIndex = Stage.SelectedIndex;

        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_F10)
            {
                GotoMenuState();
                isHandled = true;
            }
        }
        return isHandled;
    }

private:
    Viewport* GetViewport() {
        static Viewport vp_instance;
        return &vp_instance;
    }
};

class ReturnToGameWnd : public Window
{
public:
    ReturnToGameWnd(const std::string& a_Title, const SDL_FRect& a_Size, RenderInterface* a_RI) : Window(a_Title, a_Size, a_RI) {}
    void Execute(GameState* pState) override
    {
        pState->GotoPlayingState();
    }
};

class GameStateMenu : public GameState
{
    std::vector<Window*> vpWindowArray;
public:
    GameStateMenu(StateManager* pSM) : GameState(pSM) {}
    void Init(const Viewport& VP, RenderInterface* RI) override
    {
        const float menuWndW = 150;
        const float menuWndH = 208;
        Location start = { VP.WIDTH / 2.0f - menuWndW / 2.0f, 100.0f };

        Window* pMenuWnd = new Window("", { start.x, start.y, menuWndW, menuWndH }, RI);
        pMenuWnd->SetTexture(&RM.GetTex(ResourceManager::ResID_GameMenu));
        vpWindowArray.push_back(pMenuWnd);

        const float btnWndW = 140;
        const float btnWndH = 26;
        Location btnLoc = { pMenuWnd->TexDestRect.x + 6, pMenuWnd->TexDestRect.y + 144 };
        Window* pReturnToGameWnd = new ReturnToGameWnd("", { btnLoc.x, btnLoc.y, btnWndW, btnWndH }, RI);
        vpWindowArray.push_back(pReturnToGameWnd);
    }

    void Destroy() override
    {
        for (auto& pWnd : vpWindowArray)
            delete pWnd;
        vpWindowArray.clear();
    }
    void Update() override {}
    void Render(RenderInterface* RI) override
    {
        for (auto& wnd : vpWindowArray)
            wnd->Render(RI);
    }
    bool HandleInput(const SDL_Event& event) override
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
                float x = static_cast<float>(event.button.x);
                float y = static_cast<float>(event.button.y);
                for (auto it = vpWindowArray.rbegin(); it != vpWindowArray.rend(); ++it)
                {
                    if ((*it)->IsIn(x, y))
                    {
                        (*it)->Execute(this);
                        isHandled = true;
                        break;
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
public:
    StateManager() : State(nullptr), pGameStatePlaying(nullptr), pGameStateMenu(nullptr)
    {
        pGameStatePlaying = new GameStatePlaying(this);
        pGameStateMenu = new GameStateMenu(this);
    }

    void Init(const Viewport& VP, RenderInterface* RI)
    {
        pGameStateMenu->Init(VP, RI);
        pGameStatePlaying->Init(VP, RI);
        State = pGameStatePlaying;
    }

    void Destroy()

    {
        if (pGameStatePlaying) { delete pGameStatePlaying; pGameStatePlaying = nullptr; }
        if (pGameStateMenu) { delete pGameStateMenu; pGameStateMenu = nullptr; }
        State = nullptr;
    }

    size_t GetObjNum() const
    {
        return State ? State->GetObjNum() : 0;
    }

    void GotoMenuState()
    {
        State = pGameStateMenu;
    }
    void GotoPlayingState()
    {
        State = pGameStatePlaying;
    }

    void Update() override
    {
        if (State) State->Update();
    }

    void Render(RenderInterface* RI) override
    {
        if (State) State->Render(RI);
    }

    bool HandleInput(const SDL_Event& event) override
    {
        bool isHandled = false;
        if (State) isHandled = State->HandleInput(event);
        return isHandled;
    }
};

// Define the state transition methods from GameState
void GameState::GotoMenuState()
{
    if (pSM) pSM->GotoMenuState();
}
void GameState::GotoPlayingState()
{
    if (pSM) pSM->GotoPlayingState();
}

class Game
{
    RenderInterface* RI;
    Viewport VP;

    StateManager StateMgr;

public:
    Game() : Fps(nullptr) {}

private:
    class FPS : public SubSystem
    {
        Uint64 lastFrameTime = 0;
        __int64 fps = 0;

        SDL_Texture* ObjectCountTex = nullptr;
        SDL_Texture* FPSTex = nullptr;

        SDL_FRect ObjectCountRect;
        SDL_FRect FPSRect;

        RenderInterface* RI = nullptr;

    public:
        size_t ObjectCount = 0;

        FPS(RenderInterface* a_RI) : RI(a_RI) {}
        void Update() override
        {
            Uint64 currentFrameTime = SDL_GetTicks();
            if (lastFrameTime == 0) {
                lastFrameTime = currentFrameTime;
            }
            Uint64 deltaTime = currentFrameTime - lastFrameTime;
            if (deltaTime > 0)
                fps = 1000 / deltaTime;
            lastFrameTime = currentFrameTime;

            static size_t prevObjectCount = 0;
            static __int64 prevFps = 0;

            if (ObjectCount != prevObjectCount || fps != prevFps)
            {
                SDLRenderInterface* ri = static_cast<SDLRenderInterface*>(RI);
                Viewport* vp = ri->GetViewport();

                if (ObjectCountTex) SDL_DestroyTexture(ObjectCountTex);
                ObjectCountTex = ri->CreateTextTexture("Object count: " + std::to_string(ObjectCount), &ObjectCountRect, static_cast<float>(vp->WIDTH - 100), 10.f);

                if (FPSTex) SDL_DestroyTexture(FPSTex);
                FPSTex = ri->CreateTextTexture("FPS: " + std::to_string(fps), &FPSRect, static_cast<float>(vp->WIDTH - 60), 40.f);

                prevObjectCount = ObjectCount;
                prevFps = fps;
            }
        }
        void Render(RenderInterface* RI) override
        {
            SDL_Renderer* renderer = static_cast<SDL_Renderer*>(RI->GetRenderer());
            if (ObjectCountTex) SDL_RenderTexture(renderer, ObjectCountTex, nullptr, &ObjectCountRect); // Changed NULL to nullptr
            if (FPSTex) SDL_RenderTexture(renderer, FPSTex, nullptr, &FPSRect); // Changed NULL to nullptr
        }
    
        ~FPS()
        {
            if (ObjectCountTex) SDL_DestroyTexture(ObjectCountTex);
            if (FPSTex) SDL_DestroyTexture(FPSTex);
        }
    };

    FPS* Fps;

    void initGameData()
    {
        RM.LoadResources(RI);
        StateMgr.Init(VP, RI);
    }

    void init()
    {
        RI = new SDLRenderInterface();
        if (!RI->CreateRenderer(&VP)) {
            std::cerr << "Failed to create renderer. Exiting." << std::endl;
            exit(1);
        }
        initGameData();
        Fps = new FPS(RI);
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

        Fps->ObjectCount = StateMgr.GetObjNum();
        Fps->Update();

        return quit;
    }

    void Render()
    {
        RI->PreRender();

        StateMgr.Render(RI);

        // Render buch-outdoor.bmp at bottom right with scaling
        Texture& tileTex = RM.GetTex(ResourceManager::ResID_Tile);
        float scale = HEX_FLAT_TOP_WIDTH / Tile::SourceBitmapTileSize;
        float scaledW = tileTex.W * scale;
        float scaledH = tileTex.H * scale;
        SDL_FRect destRect = { static_cast<float>(VP.WIDTH - scaledW), static_cast<float>(VP.HEIGHT - scaledH), scaledW, scaledH };
        RI->RenderTexture(&tileTex, &destRect);

        if (DM.bShowObjectRect) {
            int tileCols = static_cast<int>(tileTex.W / Tile::SourceBitmapTileSize);
            int tileRows = static_cast<int>(tileTex.H / Tile::SourceBitmapTileSize);
            float scaledTileSize = Tile::SourceBitmapTileSize * scale;

            for (int j = 0; j < tileRows; ++j) {
                for (int i = 0; i < tileCols; ++i) {
                    SDL_FRect tileRect = {
                        destRect.x + i * scaledTileSize,
                        destRect.y + j * scaledTileSize,
                        scaledTileSize,
                        scaledTileSize
                    };
                    RI->RenderBox(&tileRect, 255, 255, 0, 255); // Yellow box

                    std::string indexStr = std::to_string(j * tileCols + i);
                    RI->RenderText(indexStr, tileRect.x + tileRect.w / 2.0f, tileRect.y + tileRect.h / 2.0f, 0.0f, HAlign::Center);
                }
            }
        }

        Fps->Render(RI);

        RI->PostRender();
    }

    void loop()
    {
        int quit = 0;
        while (!quit)
        {
            quit = Update();
            Render();
            SDL_Delay(10);
        }
    }

    void terminate()
    {
        StateMgr.Destroy();

        if (RI) {
            RI->Destroy();
            delete RI;
            RI = nullptr;
        }
        if (Fps) {
            delete Fps;
            Fps = nullptr;
        }
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
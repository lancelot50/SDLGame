//#include <SDL.h> 
//#include<string>
//
//
//int checkCollision(SDL_Rect rect1, SDL_Rect rect2)
//{
//    // 두 개의 바운딩 박스가 서로 겹치는지 확인
//    if (rect1.x + rect1.w >= rect2.x &&
//        rect2.x + rect2.w >= rect1.x &&
//        rect1.y + rect1.h >= rect2.y &&
//        rect2.y + rect2.h >= rect1.y) {
//        // 충돌 발생
//        return 1;
//    }
//    // 충돌하지 않음
//    return 0;
//}
//
//class Game
//{
//    SDL_Window* window;
//    SDL_Renderer* renderer;
//public :    
//    static const int WINDOW_WIDTH = 1024;
//    static const int WINDOW_HEIGHT = 768;
//private :
//    struct Location
//    {
//        int x=0;
//        int y=0;
//        void operator+=(const Location& In)
//        {
//            x += In.x;
//            y += In.y;
//        }
//    };
//    struct Object
//    {
//        int dx = 0;
//        int dy = 0;
//        int health=0;
//        bool show=true;
//        Location Loc;
//
//        SDL_Rect rect;
//        SDL_Texture* texture = nullptr;
//
//        static const int SPACESHIP_WIDTH = 100;
//        static const int SPACESHIP_HEIGHT = 100;
//
//        static const int MISSILE_WIDTH = 60;
//        static const int MISSILE_HEIGHT = 60;
//
//        Object() {}
//        void Init(SDL_Renderer* renderer, std::string textureName, int TexW, int TexH, const Location& InitLoc )
//        {
//            SDL_Surface* bmp = SDL_LoadBMP(textureName.c_str());
//            texture = SDL_CreateTextureFromSurface(renderer, bmp);
//            SDL_FreeSurface(bmp);
//
//            rect = { InitLoc.x - TexW / 2, InitLoc.y - TexH / 2, TexW,TexH };
//            Loc = InitLoc;
//        }
//        virtual ~Object()
//        {
//            SDL_DestroyTexture(texture);
//        }
//        void updateTextureRect()
//        {
//            rect.x = Loc.x - rect.w/2;
//            rect.y = Loc.y - rect.h/2;
//        }
//        void MoveDelta(const Location& Delta)
//        {
//            Loc += Delta;
//            updateTextureRect();
//        }
//        void SetLocation(const Location& Delta)
//        {
//            Loc = Delta;
//            updateTextureRect();
//        }
//    };
//
//    Object spaceship;
//    Object aliens[10];
//
//    struct Missile : public Object
//    {
//        Missile()
//        {
//            show = false;
////            rect = { Game::WINDOW_WIDTH / 2-MISSILE_WIDTH/2, Game::WINDOW_HEIGHT - MISSILE_HEIGHT, MISSILE_WIDTH, MISSILE_HEIGHT };
//            dx = 1;
//            dy = -10;
//        }
//    };
//    Missile missile;
//
//    void initGameData()
//    {
//        spaceship.Init(renderer, "spaceship.bmp", Object::SPACESHIP_WIDTH, Object::SPACESHIP_HEIGHT, Location{ WINDOW_WIDTH / 2, WINDOW_HEIGHT-100 });
//        for (int i = 0; i < 10; i++)// 외계인 우주선 배열 초기화
//            aliens[i].Init(renderer, "alien.bmp", Object::SPACESHIP_WIDTH, Object::SPACESHIP_HEIGHT, Location{ WINDOW_WIDTH / 2, WINDOW_HEIGHT - 100 });
//        missile.Init(renderer, "missile.bmp", Object::MISSILE_WIDTH, Object::MISSILE_HEIGHT, Location{ WINDOW_WIDTH / 2, WINDOW_HEIGHT - 100 });
//
//
//        // 외계인 우주선 위치 초기화
//        int alienX = 50;
//        int alienY = 50;
//        for (int i = 0; i < 10; i++)
//        {
//            aliens[i].dx = aliens[i].dy = 1;
//            aliens[i].rect.x = alienX;
//            aliens[i].rect.y = alienY;
//            alienX += Object::SPACESHIP_WIDTH;
//            if (alienX >= WINDOW_WIDTH - Object::SPACESHIP_WIDTH)
//            {
//                alienX = 50;
//                alienY += Object::SPACESHIP_HEIGHT;
//            }
//        }
//    }
//
//    void init()
//    {
//        SDL_Init(SDL_INIT_VIDEO);
//
//        window = SDL_CreateWindow("galaga", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
//        renderer = SDL_CreateRenderer(window, -1, 0);
//
//        initGameData();
//    }
//
//    int Update()
//    {
//        SDL_Event event;
//        int quit = 0;
//        while (SDL_PollEvent(&event))            // 이벤트 처리
//        {
//            if (event.type == SDL_QUIT)
//            {
//                quit = 1;
//            }
//            else if (event.type == SDL_KEYDOWN)
//            {
//                if (event.key.keysym.sym == SDLK_RIGHT)
//                    spaceship.MoveDelta({ 30,0 });
//                if (event.key.keysym.sym == SDLK_LEFT)
//                    spaceship.MoveDelta({ -30,0 });
//                if (event.key.keysym.sym == SDLK_SPACE)
//                {
//                    missile.show = true;
//                    missile.SetLocation(spaceship.Loc);
//                }
//            }
//        }
//        for (int i = 0; i < 10; i++)
//        {
//            if (!aliens[i].show)
//                continue;
//            aliens[i].rect.x += aliens[i].dx;
//            if (aliens[i].rect.x <= 0 || aliens[i].rect.x > WINDOW_WIDTH) {
//                aliens[i].dx *= -1;
//                aliens[i].rect.y += 10;
//            }
//            if (!missile.show)
//                continue;
//            int collision = checkCollision(aliens[i].rect, missile.rect);
//            if (collision)
//            {
//                missile.show = false;
//                aliens[i].show = false;
//                missile.rect.y = spaceship.rect.y;
//            }
//        }
//        if (missile.show)
//        {
//            missile.rect.y += missile.dy;
//            if (missile.rect.y < 0)
//                missile.show = false;
//        }
//
//        return quit;
//    }
//    void Render()
//    {
//        // 화면 지우기
//        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
//        SDL_RenderClear(renderer);
//
//        SDL_RenderCopy(renderer, spaceship.texture, NULL, &(spaceship.rect));
//
//        // 외계인 우주선 그리기
//        for (int i = 0; i < 10; i++)
//            if (aliens[i].show)
//                SDL_RenderCopy(renderer, aliens[i].texture, NULL, &(aliens[i].rect));
//
//        if (missile.show)
//            SDL_RenderCopy(renderer, missile.texture, NULL, &(missile.rect));
//
//        SDL_Delay(10);
//        SDL_RenderPresent(renderer);
//    }
//
//    void loop()
//    {
//        int quit = 0;
//        while (!quit)
//        {
//            quit=Update();
//            Render();
//        }
//    }
//    void terminate()
//    {
//        SDL_DestroyRenderer(renderer); // 렌더러 파괴
//        SDL_DestroyWindow(window); // 윈도우 파괴
//        SDL_Quit(); // SDL 종료
//    }
//
//public:
//    void Start()
//    {
//        init();
//        loop();
//        terminate();
//    }
//};
//
//int main(int argc, char** argv)
//{
//    Game game;
//    game.Start();
//    return 0;
//}

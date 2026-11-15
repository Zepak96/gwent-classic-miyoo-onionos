#include <SDL.h>
#include <string>
int main(int, char**) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) return 1;
  SDL_Window* w=SDL_CreateWindow("Gwent Classic",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,640,480,SDL_WINDOW_SHOWN);
  if(!w){SDL_Quit();return 2;}
  SDL_Renderer* r=SDL_CreateRenderer(w,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
  bool running=true;
  while(running){ SDL_Event e; while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)running=false; if(e.type==SDL_KEYDOWN&&e.key.keysym.sym==SDLK_ESCAPE)running=false;}
    SDL_SetRenderDrawColor(r,29,24,22,255); SDL_RenderClear(r); SDL_SetRenderDrawColor(r,118,81,48,255);
    SDL_Rect table{24,24,592,432}; SDL_RenderDrawRect(r,&table); SDL_RenderPresent(r); }
  SDL_DestroyRenderer(r); SDL_DestroyWindow(w); SDL_Quit(); return 0;
}

#include "gui_window.hpp"

#ifdef USE_GUI

#include <SDL2/SDL.h>
#include <stdexcept>
#include <string>

void runImagePreviewWindow(int width, int height, const uint8_t* rgbData) {
    if (width <= 0 || height <= 0 || !rgbData) return;
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());

    SDL_Window* win = SDL_CreateWindow("Melt image preview",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       width, height,
                                       SDL_WINDOW_SHOWN);
    if (!win) {
        SDL_Quit();
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }

    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        SDL_DestroyWindow(win);
        SDL_Quit();
        throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
    }

    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!tex) {
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        throw std::runtime_error(std::string("SDL_CreateTexture failed: ") + SDL_GetError());
    }

    if (SDL_UpdateTexture(tex, nullptr, rgbData, width * 3) != 0) {
        SDL_DestroyTexture(tex);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        throw std::runtime_error(std::string("SDL_UpdateTexture failed: ") + SDL_GetError());
    }

    bool quit = false;
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = true;
            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) quit = true;
            if (e.type == SDL_KEYDOWN && (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_q)) quit = true;
        }
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, nullptr, nullptr);
        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
}

#else

void runImagePreviewWindow(int width, int height, const uint8_t* rgbData) {
    (void)width;
    (void)height;
    (void)rgbData;
}

#endif

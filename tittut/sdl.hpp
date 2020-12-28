#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <string>

class SDLWindow {
private:
    std::string name_;
  SDL_Window *win_ = nullptr;
  SDL_Renderer *ren_ = nullptr;
  SDL_Texture *texture_ = nullptr;
  SDL_Rect rect_ = {};

public:
  SDLWindow(const std::string &name, int width, int height) : name_(name) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
      std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
      return;
    }

    win_ = SDL_CreateWindow(name_.c_str(), 100, 100, width, height,
                            SDL_WINDOW_SHOWN);
    if (win_ == nullptr) {
      std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
      SDL_Quit();
      return;
    }

    ren_ = SDL_CreateRenderer(
        win_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren_ == nullptr) {
      SDL_DestroyWindow(win_);
      std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
      SDL_Quit();
      return;
    }

    texture_ = SDL_CreateTexture(ren_, SDL_PIXELFORMAT_YUY2,
                                 SDL_TEXTUREACCESS_STREAMING, width, height);
    if (texture_ == nullptr) {
      SDL_DestroyWindow(win_);
      std::cout << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
      SDL_Quit();
      return;
    }

    rect_.w = width;
    rect_.h = height;
  }

  ~SDLWindow() {
    SDL_DestroyWindow(win_);
    SDL_Quit();
  }

  void updateTexture(void *buffer) {
    SDL_UpdateTexture(texture_, &rect_, buffer, rect_.w * 2);
  }

  void render() {
    SDL_RenderClear(ren_);
    SDL_RenderCopy(ren_, texture_, NULL, &rect_);
    SDL_RenderPresent(ren_);
  }
};

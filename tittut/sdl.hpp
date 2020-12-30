#pragma once

#include "v4l.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <string>

void initSDL() {
    static bool initialized = false;
    if (!initialized) {
        if (SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error(std::string("Could not initialize SDL: ") + SDL_GetError());
        }
        std::atexit(SDL_Quit);
        initialized = true;
    }
}

// Window that renders a video stream.
// NOTE: This class should only be constructed in the main thread because of
//       how SDL works.
class SDLWindow {
private:
    std::string name_;
  SDL_Window *win_ = nullptr;
  SDL_Renderer *ren_ = nullptr;
  SDL_Texture *texture_ = nullptr;
  SDL_Rect rect_ = {};
  bool quit_;
  V4L v4l_;

public:
  SDLWindow(const std::string &name, int width, int height) :
      name_(name), quit_(false), v4l_(width, height, V4L2_PIX_FMT_YUYV) {
    initSDL();

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
    const char *error = SDL_GetError();
    if (*error) {
      SDL_Log("SDL error: %s", error);
      SDL_ClearError();
    }
  }

  void updateTexture(void *buffer) {
    if (SDL_UpdateTexture(texture_, &rect_, buffer, rect_.w * 2))
    {
        throw std::runtime_error("Failed updating texture");
    }
  }

  void render() {
    SDL_RenderClear(ren_);
    SDL_RenderCopy(ren_, texture_, NULL, &rect_);
    SDL_RenderPresent(ren_);
  }

  void pollEvents() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
          if (event.key.keysym.sym == SDLK_ESCAPE) {
            std::cout << "Pressed quit...\n";
            quit_ = true;
          }
          break;
        case SDL_QUIT:
          std::cout << "Quitting...\n";
          quit_ = true;
          break;
        }
      }
  }

  void run() {
    void *buffer = v4l_.getBuffer();
    while (!quit_) {
        pollEvents();
        v4l_.update();
        updateTexture(buffer);
        render();
    }
  }
};

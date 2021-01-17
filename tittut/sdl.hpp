#pragma once

#include "v4l.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <memory>
#include <functional>
#include <iostream>
#include <string>

void sdlError(std::string msg) {
    std::string sdlErrorMsg(SDL_GetError());
    SDL_ClearError();
    throw std::runtime_error(msg + ": " + sdlErrorMsg);
}

void initSDL() {
    static bool initialized = false;
    if (!initialized) {
        if (SDL_Init(SDL_INIT_VIDEO)) {
            sdlError("SDL_Init");
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
    std::unique_ptr<SDL_Window, std::function<void(SDL_Window*)>>
        win_{nullptr, [](SDL_Window* w) { SDL_DestroyWindow(w); }};
    SDL_Renderer *ren_ = nullptr;
    SDL_Texture *texture_ = nullptr;
    SDL_Rect rect_ = {};
    bool quit_ = false;
    std::unique_ptr<VideoStream> videoStream_;

public:
    SDLWindow(const std::string &name, int width, int height, std::unique_ptr<VideoStream> &stream) :
              name_(name), videoStream_(std::move(stream)) {
        initSDL();

        win_.reset(SDL_CreateWindow(name_.c_str(), 100, 100, width, height,
                                SDL_WINDOW_SHOWN));
        if (win_.get() == nullptr) {
            sdlError("SDL_CreateWindow");
        }

        ren_ = SDL_CreateRenderer(
            win_.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (ren_ == nullptr) {
            sdlError("SDL_CreateRenderer");
        }

        texture_ = SDL_CreateTexture(ren_, SDL_PIXELFORMAT_YUY2,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     width,
                                     height);
        if (texture_ == nullptr) {
            sdlError("SDL_CreateTexture");
        }

        rect_.w = width;
        rect_.h = height;
  }

  void updateTexture(void *buffer) {
    if (SDL_UpdateTexture(texture_, &rect_, buffer, rect_.w * 2))
    {
        sdlError("SDL_UpdateTexture");
    }
  }

  void render() {
    if (SDL_RenderClear(ren_))
        sdlError("SDL_RenderClear");
    if (SDL_RenderCopy(ren_, texture_, NULL, &rect_))
        sdlError("SDL_RenderCopy");
    SDL_RenderPresent(ren_);
  }

  void pollEvents() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
          if (event.key.keysym.sym == SDLK_ESCAPE) {
            quit_ = true;
          }
          break;
        case SDL_QUIT:
          quit_ = true;
          break;
        }
      }
  }

  void run() {
    void *buffer = videoStream_->getBuffer();
    while (!quit_) {
        pollEvents();
        videoStream_->update();
        buffer = videoStream_->getBuffer();
        updateTexture(buffer);
        render();
    }
  }
};

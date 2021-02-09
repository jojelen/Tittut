#pragma once

#include "utils.hpp"
#include "video-stream.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <functional>
#include <iostream>
#include <memory>
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
    std::unique_ptr<SDL_Window, std::function<void(SDL_Window *)>> win_{
        nullptr, [](SDL_Window *w) { SDL_DestroyWindow(w); }};
    SDL_Renderer *ren_ = nullptr;
    SDL_Texture *texture_ = nullptr;
    SDL_Rect rect_ = {};
    bool quit_ = false;
    std::unique_ptr<VideoStream> videoStream_;
    int rowPitch_ = 0;
    bool flip_;

    void flipBuffer(uint8_t *srcBuffer, uint8_t *dstBuffer) {
        TIMER("Flipping image");
        for (int y = 0; y < rect_.h; ++y) {
            for (int x = 0; x < rect_.w; x += 2) {
                uint8_t *srcY1 = srcBuffer + x * 2 + rowPitch_ * y;
                uint8_t *srcY2 = srcBuffer + (x + 1) * 2 + rowPitch_ * y;
                uint8_t *dstY1 = dstBuffer + (rect_.w - 1 - x) * 2 +
                                 rowPitch_ * (rect_.h - 1 - y);
                uint8_t *dstY2 = dstBuffer + (rect_.w - 1 - (x + 1)) * 2 +
                                 rowPitch_ * (rect_.h - 1 - y);

                *dstY1 = *srcY1;
                *dstY2 = *srcY2;
                *(dstY1 + 1) = *(srcY2 + 1);
                *(dstY2 + 1) = *(srcY1 + 1);
            }
        }
    }

  public:
    SDLWindow(const std::string &name, int width, int height,
              std::unique_ptr<VideoStream> &stream, bool flip = false)
        : name_(name), videoStream_(std::move(stream)), flip_(flip) {
        initSDL();

        win_.reset(SDL_CreateWindow(name_.c_str(), 100, 100, width, height,
                                    SDL_WINDOW_SHOWN));
        if (win_.get() == nullptr) {
            sdlError("SDL_CreateWindow");
        }

        ren_ = SDL_CreateRenderer(win_.get(), -1,
                                  SDL_RENDERER_ACCELERATED |
                                      SDL_RENDERER_PRESENTVSYNC);
        if (ren_ == nullptr) {
            sdlError("SDL_CreateRenderer");
        }

        texture_ =
            SDL_CreateTexture(ren_, SDL_PIXELFORMAT_YUY2,
                              SDL_TEXTUREACCESS_STREAMING, width, height);
        if (texture_ == nullptr) {
            sdlError("SDL_CreateTexture");
        }

        rect_.w = width;
        rect_.h = height;
        rowPitch_ = rect_.w * 2;
    }

    void updateTexture(void *buffer) {
        TIMER("Updating texture");
        if (SDL_UpdateTexture(texture_, &rect_, buffer, rowPitch_)) {
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
        std::vector<uint8_t> flippedBuffer;
        while (!quit_) {
            TIMER("One frame");
            pollEvents();
            videoStream_->update();
            uint8_t *buffer = static_cast<uint8_t *>(videoStream_->getBuffer());
            if (flip_) {
                flippedBuffer.resize(videoStream_->getBufferSize());
                flipBuffer(buffer, flippedBuffer.data());
                buffer = flippedBuffer.data();
            }
            updateTexture(static_cast<void *>(buffer));
            render();
        }
    }
};

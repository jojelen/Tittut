// Simple webcam application that uses Video4Linux for retrieving video stream
// and SDL2 for viewing it in a window.
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <chrono>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <linux/videodev2.h>
#include <stdexcept>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

using namespace std;

// Singleton class that handles Video4Linux.
class V4L {
private:
  int fd_;
  v4l2_buffer bufferInfo_;
  void *buffer_;

  void call_ioctl(string_view msg, unsigned long int req, void *arg) {
    int ret = ioctl(fd_, req, arg);
    if (ret < 0) {
      throw runtime_error(string(msg) + " failed (" + to_string(ret) +
                          "): " + strerror(errno));
    }
  }

  V4L() : fd_(-1), buffer_(nullptr) {
    bufferInfo_ = {};
    fd_ = open("/dev/video0", O_RDWR);
    if (fd_ < 0) {
      throw runtime_error(string("Couldn't open /dev/video0: ") +
                          strerror(errno));
    }

    getCapabilities();
  }

public:
  V4L(V4L const &) = delete;
  V4L &operator=(V4L const &) = delete;
  ~V4L() {
    call_ioctl("Deactivate streaming", VIDIOC_STREAMOFF, &bufferInfo_.type);
    close(fd_);
  }

  static V4L &instance() {
    static V4L v4l;

    return v4l;
  }

  void getCapabilities() {
    v4l2_capability cap = {};
    call_ioctl("Query capabilities", VIDIOC_QUERYCAP, &cap);

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
      throw runtime_error(
          "The device does not handle single-planar video capture.");
    }
  }

  void requestBuffers() {
    v4l2_requestbuffers bufReq = {};
    bufReq.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufReq.memory = V4L2_MEMORY_MMAP;
    bufReq.count = 1;

    call_ioctl("Request buffers", VIDIOC_REQBUFS, &bufReq);
  }

  void setFormat(int width, int height, int pixelFormat) {
    v4l2_format format = {};
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = pixelFormat;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;

    call_ioctl("Set format", VIDIOC_S_FMT, &format);

    requestBuffers();

    bufferInfo_ = {};
    bufferInfo_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufferInfo_.memory = V4L2_MEMORY_MMAP;
    bufferInfo_.index = 0;

    call_ioctl("Query buffers", VIDIOC_QUERYBUF, &bufferInfo_);
  }

  void *getBuffer() {
    buffer_ = mmap(NULL, bufferInfo_.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                   fd_, bufferInfo_.m.offset);
    if (buffer_ == MAP_FAILED) {
      throw runtime_error(string("mmap of buffer failed: ") + strerror(errno));
    }

    call_ioctl("Activate streaming", VIDIOC_STREAMON, &bufferInfo_.type);

    return buffer_;
  }

  void update() {
    call_ioctl("Put buffer in queue", VIDIOC_QBUF, &bufferInfo_);
    call_ioctl("Wait for buffer in queue", VIDIOC_DQBUF, &bufferInfo_);
  }
};

class SDLWindow {
private:
  string name_;
  SDL_Window *win_ = nullptr;
  SDL_Renderer *ren_ = nullptr;
  SDL_Texture *texture_ = nullptr;
  SDL_Rect rect_ = {};

public:
  SDLWindow(const string &name, int width, int height) : name_(name) {
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

int main() {
  V4L &v4l = V4L::instance();

  int width = 320;
  int height = 180;
  v4l.setFormat(width, height, V4L2_PIX_FMT_YUYV);

  void *buffer = v4l.getBuffer();

  SDLWindow win("SDL window", width, height);

  for (int i = 0; i < 200; ++i) {
    v4l.update();
    win.updateTexture(buffer);
    win.render();
  }
}

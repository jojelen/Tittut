#pragma once

#include <linux/videodev2.h>
#include <stdexcept>
#include <cstdio>
#include <sys/stat.h>
#include <iostream>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

// Singleton class that handles Video4Linux.
class V4L {
private:
  int fd_;
  v4l2_buffer bufferInfo_;
  void *buffer_;

  void call_ioctl(std::string_view msg, unsigned long int req, void *arg) {
    int ret = ioctl(fd_, req, arg);
    if (ret < 0) {
      throw std::runtime_error(std::string(msg) + " failed (" + std::to_string(ret) +
                          "): " + strerror(errno));
    }
  }

  V4L() : fd_(-1), buffer_(nullptr) {
    bufferInfo_ = {};
    fd_ = open("/dev/video0", O_RDWR);
    if (fd_ < 0) {
      throw std::runtime_error(std::string("Couldn't open /dev/video0: ") +
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
      throw std::runtime_error(
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
      throw std::runtime_error(std::string("mmap of buffer failed: ") + strerror(errno));
    }

    call_ioctl("Activate streaming", VIDIOC_STREAMON, &bufferInfo_.type);

    return buffer_;
  }

  void update() {
    call_ioctl("Put buffer in queue", VIDIOC_QBUF, &bufferInfo_);
    call_ioctl("Wait for buffer in queue", VIDIOC_DQBUF, &bufferInfo_);
  }
};

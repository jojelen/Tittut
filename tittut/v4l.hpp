#pragma once

#include "utils.hpp"
#include "videostream.hpp"

#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/videodev2.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

class V4L : public VideoStream {
  private:
    int fd_;
    v4l2_buffer bufferInfo_;

    void call_ioctl(std::string_view msg, unsigned long int req,
                    const void *arg) const {
        int ret = ioctl(fd_, req, arg);
        if (ret < 0) {
            throw std::runtime_error(std::string(msg) + " failed (" +
                                     std::to_string(ret) +
                                     "): " + strerror(errno));
        }
    }

    void getCapabilities() const {
        v4l2_capability cap = {};
        call_ioctl("Query capabilities", VIDIOC_QUERYCAP, &cap);

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
            throw std::runtime_error(
                "The device does not handle single-planar video capture.");
        }
    }

    void requestBuffers() const {
        v4l2_requestbuffers bufReq = {};
        bufReq.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        bufReq.memory = V4L2_MEMORY_MMAP;
        bufReq.count = 1;

        call_ioctl("Request buffers", VIDIOC_REQBUFS, &bufReq);
    }

    void setFormat(int width, int height, int pixelFormat) const {
        if (width <= 0 || height <= 0)
            throw std::invalid_argument(
                std::string("Invalid format dimensions: ") +
                "Use \"v4l2-ctl --list-formats-ext\" to see available formats");
        v4l2_format format = {};
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        format.fmt.pix.pixelformat = pixelFormat;
        format.fmt.pix.width = width;
        format.fmt.pix.height = height;

        call_ioctl("Set format", VIDIOC_S_FMT, &format);

        if (format.fmt.pix.width != static_cast<unsigned int>(width) ||
            format.fmt.pix.height != static_cast<unsigned int>(height))
            throw std::invalid_argument(
                std::string("Invalid format dimensions: ") +
                "Use \"v4l2-ctl --list-formats-ext\" to see available formats");
    }

    void queryBuffer() {
        requestBuffers();

        bufferInfo_ = {};
        bufferInfo_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        bufferInfo_.memory = V4L2_MEMORY_MMAP;
        bufferInfo_.index = 0;

        call_ioctl("Query buffers", VIDIOC_QUERYBUF, &bufferInfo_);
    }

    void mapBuffer() {
        buffer_ = mmap(NULL, bufferInfo_.length, PROT_READ | PROT_WRITE,
                       MAP_SHARED, fd_, bufferInfo_.m.offset);
        if (buffer_ == MAP_FAILED) {
            throw std::runtime_error(std::string("mmap of buffer failed: ") +
                                     strerror(errno));
        }
    }

  public:
    V4L(int width, int height, int pixelFormat) : fd_(-1) {
        fd_ = open("/dev/video0", O_RDWR);
        if (fd_ < 0) {
            throw std::runtime_error(
                std::string("Couldn't open /dev/video0: ") + strerror(errno));
        }

        try {
            getCapabilities();
            setFormat(width, height, pixelFormat);
            queryBuffer();
            mapBuffer();

            call_ioctl("Activate streaming", VIDIOC_STREAMON,
                       &bufferInfo_.type);
        } catch (std::exception const &e) {
            close(fd_);
            throw e;
        }
    }

    V4L(V4L const &) = delete;
    V4L &operator=(V4L const &) = delete;
    ~V4L() {
        if (munmap(buffer_, bufferInfo_.length)) {
            std::cerr << "ERROR: munmap failed!\n";
        }
        buffer_ = nullptr;
        call_ioctl("Deactivate streaming", VIDIOC_STREAMOFF, &bufferInfo_.type);
        close(fd_);
    }

    void update() override {
        TIMER("Buffer ioctls in update");
        call_ioctl("Put buffer in queue", VIDIOC_QBUF, &bufferInfo_);
        call_ioctl("Wait for buffer in queue", VIDIOC_DQBUF, &bufferInfo_);
    }

    inline void *getBuffer() override { return buffer_; }

    inline size_t getBufferSize() const { return bufferInfo_.length; }
};

#pragma once

#include "utils.hpp"
#include "video-stream.hpp"

#include <chrono>
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
#include <thread>
#include <unistd.h>
#include <vector>

struct Frame {
    void *data;
    v4l2_buffer buffer;
};

class V4LStream : public VideoStream {
  private:
    static constexpr int STREAM_TYPE_ = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int fd_ = -1;
    std::vector<Frame> buffers_;
    size_t currFrame_ = 0;
    static constexpr size_t NUM_BUFFERS = 2;

    void call_ioctl(std::string_view msg, unsigned long int req,
                    const void *arg) const {
        using namespace std::chrono_literals;

        int ret = -1;
        while (true) {
            ret = ioctl(fd_, req, arg);
            if (ret < 0 && errno == EAGAIN) {
                std::this_thread::sleep_for(2ms);
            } else {
                break;
            }
        }

        if (ret < 0) {
            throw std::runtime_error(std::string(msg) + " failed (" +
                                     std::to_string(errno) +
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

    void requestBuffers(int count) const {
        v4l2_requestbuffers bufReq = {};
        bufReq.type = STREAM_TYPE_;
        bufReq.memory = V4L2_MEMORY_MMAP;
        bufReq.count = count;

        call_ioctl("Request buffers", VIDIOC_REQBUFS, &bufReq);

        // This does not seem to be supported in my version...
        // uint32_t caps = static_cast<uint32_t>(bufReq.capabilities);
        // bool capMmap = V4L2_BUF_CAP_SUPPORTS_MMAP & caps;
        // bool capDma = V4L2_BUF_CAP_SUPPORTS_DMABUF & caps;
        // bool capUser = V4L2_BUF_CAP_SUPPORTS_USERPTR & caps;
        // bool capReq = V4L2_BUF_CAP_SUPPORTS_REQUESTS & caps;
        // bool capOrph =V4L2_BUF_CAP_SUPPORTS_ORPHANED_BUFS  & caps;

        // std::cout << "Buffer capabilities:\n";
        // std::cout << "V4L2_BUF_CAP_SUPPORTS_MMAP: " << capMmap;
        // std::cout << "V4L2_BUF_CAP_SUPPORTS_DMABUF: " << capDma;
        // std::cout << "V4L2_BUF_CAP_SUPPORTS_USERPTR: " << capUser;
        // std::cout << "V4L2_BUF_CAP_SUPPORTS_REQUESTS: " << capReq;
        // std::cout << "V4L2_BUF_CAP_SUPPORTS_ORPHANED: " << capOrph;
    }

    void setFormat(int width, int height, int pixelFormat) const {
        if (width <= 0 || height <= 0)
            throw std::invalid_argument(
                std::string("Invalid format dimensions: ") +
                "Use \"v4l2-ctl --list-formats-ext\" to see available formats");
        v4l2_format format = {};
        format.type = STREAM_TYPE_;
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

    void printParams() const {
        v4l2_streamparm params = {};
        params.type = STREAM_TYPE_;

        call_ioctl("Get params", VIDIOC_G_PARM, &params);

        std::cout << "Parameters:\n";
        std::cout << "capabilities: " << params.parm.capture.capability << "\n";
        std::cout << "capturemode: " << params.parm.capture.capturemode << "\n";
        std::cout << "timerperframe: "
                  << params.parm.capture.timeperframe.numerator << " / "
                  << params.parm.capture.timeperframe.denominator << "\n";
    }

    void queryBuffer() {
        for (size_t i = 0; i < buffers_.size(); ++i) {
            buffers_[i].buffer = {};
            buffers_[i].buffer.type = STREAM_TYPE_;
            buffers_[i].buffer.memory = V4L2_MEMORY_MMAP;
            buffers_[i].buffer.index = i;

            call_ioctl("Query buffers", VIDIOC_QUERYBUF, &buffers_[i].buffer);
        }
    }

    void mapBuffer() {
        for (auto &b : buffers_) {
            b.data = mmap(NULL, b.buffer.length, PROT_READ | PROT_WRITE,
                          MAP_SHARED, fd_, b.buffer.m.offset);
            if (b.data == MAP_FAILED) {
                throw std::runtime_error(
                    std::string("mmap of buffer failed: ") + strerror(errno));
            }
        }
    }

  public:
    V4LStream(int width, int height, int pixelFormat) : fd_(-1) {
        fd_ = open("/dev/video0", O_RDWR | O_NONBLOCK);
        if (fd_ < 0) {
            throw std::runtime_error(
                std::string("Couldn't open /dev/video0: ") + strerror(errno));
        }

        buffers_.resize(NUM_BUFFERS);

        try {
            getCapabilities();
            setFormat(width, height, pixelFormat);
            requestBuffers(static_cast<int>(buffers_.size()));
            queryBuffer();
            mapBuffer();
            for (auto &b : buffers_) {
                call_ioctl("Put buffer in queue", VIDIOC_QBUF, &b.buffer);
            }
            call_ioctl("Activate streaming", VIDIOC_STREAMON, &STREAM_TYPE_);
            printParams();

        } catch (std::exception const &e) {
            close(fd_);
            throw std::runtime_error(std::string("ERROR: Could not set up video streaming: ") +
                      std::string(e.what()));
        }
    }

    V4LStream(V4LStream const &) = delete;
    V4LStream &operator=(V4LStream const &) = delete;
    ~V4LStream() {
        for (auto &b : buffers_) {
            if (b.buffer.flags & V4L2_BUF_FLAG_QUEUED) {
                call_ioctl("Wait for buffer in queue", VIDIOC_DQBUF,
                       &b.buffer);

            }
            if (munmap(b.data, b.buffer.length)) {
                std::cerr << "ERROR: munmap failed!\n";
            }

            b.data = nullptr;
        }

        call_ioctl("Deactivate streaming", VIDIOC_STREAMOFF, &STREAM_TYPE_);
        requestBuffers(0);

        if (close(fd_)) {
            std::cerr << "[ERROR]: Failed to close video fd\n";
        }
    }

    void update() override {
        if (!(buffers_[currFrame_].buffer.flags & V4L2_BUF_FLAG_QUEUED)) {
            TIMER("Put buffer in queue");
            call_ioctl("Put buffer in queue", VIDIOC_QBUF,
                       &buffers_[currFrame_].buffer);
        }

        currFrame_ = (currFrame_ + 1 == buffers_.size()) ? 0 : currFrame_ + 1;
        {
            TIMER("Waiting for frame");
            call_ioctl("Wait for buffer in queue", VIDIOC_DQBUF,
                       &buffers_[currFrame_].buffer);
        }
        buffer_ = buffers_[currFrame_].data;
    }

    inline void *getBuffer() override { return buffer_; }

    inline size_t getBufferSize() const {
        return buffers_[currFrame_].buffer.length;
    }
};

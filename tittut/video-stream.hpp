// Abstract class for a videostream.
#pragma once

#include <tuple>

class VideoStream {
  protected:
    void *buffer_;
    int width_;
    int height_;
    int format_;

  public:
    VideoStream(int width, int height, int format)
        : buffer_(nullptr), width_(width), height_(height), format_(format) {}
    virtual ~VideoStream() {}

    virtual void *getBuffer() = 0;
    virtual size_t getBufferSize() const = 0;
    virtual void update() = 0;

    std::tuple<int, int, int> getMetaData() const {
        return {width_, height_, format_};
    }
};

// Abstract class for a videostream.
#pragma once

class VideoStream {
    protected:
        void *buffer_;
    public:
        VideoStream() : buffer_(nullptr) {}
        virtual ~VideoStream() {}

        virtual void* getBuffer() = 0;
        virtual void update() = 0;
};

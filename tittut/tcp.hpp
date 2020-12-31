#pragma once

#include "videostream.hpp"
#include "sdl.hpp"

#include <arpa/inet.h>
#include <assert.h>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#define HEADER_SIZE 16

// TODO: Endieness has to be the same on each machine.

enum class PkgType { STREAM_CONFIG, FRAME, TEXT };

struct StreamConfig {
    uint64_t width;
    uint64_t height;
};

// The package that is sent over tcp is
// uint64_t data size, i.e. data.size()
// uint64_t type
// uint8_t data
// uint8_t ...
// uint8_t data
struct Package {
    PkgType type;
    std::vector<uint8_t> data;
};

bool getPackage(int socket, Package &pkg) {
    std::vector<uint8_t> header(HEADER_SIZE);
    int bytes = recv(socket, static_cast<void *>(header.data()), HEADER_SIZE,
                     MSG_WAITALL);
    if (bytes == 0) {
        std::cout << "Connection closed\n";
        return false;
    } else if (bytes < 0) {
        throw std::runtime_error(std::string("Failed reading reading out package header from socket: ") + strerror(errno));
    }

    uint64_t dataSize = 0;
    std::memcpy(&dataSize, header.data(), sizeof(uint64_t));
    uint64_t pkgType = 0;
    std::memcpy(&pkgType, header.data() + sizeof(uint64_t), sizeof(uint64_t));

    if (pkg.data.size() != dataSize)
        pkg.data.resize(dataSize);

    bytes = recv(socket, static_cast<void *>(pkg.data.data()), dataSize,
                 MSG_WAITALL);
    if (bytes < 0) {
        throw std::runtime_error(std::string("Failed reading reading out package data from socket: ") + strerror(errno));
    }
    pkg.type = static_cast<PkgType>(pkgType);

    return true;
}

void addNumToVec(size_t num, std::vector<uint8_t> &vec) {
    if (num >= UINT64_MAX)
        throw std::invalid_argument("Too large num");

    uint64_t castedNum = static_cast<uint64_t>(num);
    vec.resize(vec.size() + sizeof(uint64_t));
    std::memcpy(vec.data() + vec.size() - sizeof(uint64_t), &castedNum,
                sizeof(uint64_t));
}

size_t getNumFromVec(size_t idx, const std::vector<uint8_t> &vec) {
    assert(vec.size() >= idx*sizeof(uint64_t));

    uint64_t num = 0;
    std::memcpy(&num, vec.data() + idx * sizeof(uint64_t), sizeof(uint64_t));

    return static_cast<uint64_t>(num);
}

void sendPackage(int socket, PkgType type, const std::vector<uint8_t> &data) {
    std::vector<uint8_t> header;
    addNumToVec(data.size(), header);
    addNumToVec(static_cast<size_t>(type), header);

    assert(header.size() == HEADER_SIZE);

    int bytes =
        send(socket, static_cast<const void *>(header.data()), HEADER_SIZE, 0);
    if (bytes < 0) {
        throw std::runtime_error("Could not send header of package");
    }

    bytes =
        send(socket, static_cast<const void *>(data.data()), data.size(), 0);
    if (bytes < 0) {
        throw std::runtime_error("Could not send data of package");
    }
    std::cout << "Sent " << bytes << " bytes data with data size "
              << data.size() << std::endl;
}

void sendBuffer(int socket, const void* buffer, size_t bufferSize) {
    std::vector<uint8_t> header;
    addNumToVec(bufferSize, header);
    addNumToVec(static_cast<size_t>(PkgType::FRAME), header);

    assert(header.size() == HEADER_SIZE);

    int bytes =
        send(socket, static_cast<const void *>(header.data()), HEADER_SIZE, 0);
    if (bytes < 0) {
        throw std::runtime_error("Could not send header of package");
    }

    bytes =
        send(socket, buffer, bufferSize, 0);
    if (bytes < 0) {
        throw std::runtime_error("Could not send data of package");
    }
    std::cout << "Sent " << bytes << " bytes data with data size "
              << bufferSize << std::endl;
}

void sendMsg(int socket, std::string_view msg) {
    std::vector<uint8_t> data(msg.length());
    std::copy(msg.begin(), msg.end(), data.begin());
    std::cout << "Sending msg(" << msg << "), data.size=" << data.size()
              << std::endl;
    sendPackage(socket, PkgType::TEXT, data);
}

void readPkg(const Package &pkg) {
    if (pkg.type == PkgType::TEXT) {
        std::string msg(pkg.data.begin(), pkg.data.end());
        std::cout << "Recieved msg: " << msg << std::endl;
    } else {
        std::cerr << "Package is not of text type (type="
                  << static_cast<int>(pkg.type) << ")\n";
    }
}

void sendStreamConfig(int socket, const StreamConfig &cfg) {
    std::vector<uint8_t> data;

    addNumToVec(static_cast<size_t>(cfg.width), data);
    addNumToVec(static_cast<size_t>(cfg.height), data);

    sendPackage(socket, PkgType::STREAM_CONFIG, data);
}

class TcpVideoStream : public VideoStream {
    int socket_ = -1;
    Package pkg_ = {};

    void setupStream(int width, int height) const {
        std::cout << "Setting up stream\n";
        StreamConfig cfg = {.width = static_cast<uint64_t>(width),
                            .height = static_cast<uint64_t>(height)};

        sendStreamConfig(socket_, cfg);
    }

  public:
    TcpVideoStream(const std::string &ip, int port, int width, int height) {
        socket_ = socket(PF_INET, SOCK_STREAM, 0);
        if (socket_ < 0) {
            throw std::runtime_error("Could not create socket");
        }

        struct sockaddr_in serverAddr = {};
        socklen_t addrLen = sizeof(struct sockaddr_in);

        serverAddr.sin_family = PF_INET;
        serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
        serverAddr.sin_port = htons(port);

        std::cout << "Connecting to tcp server...\n";
        if (connect(socket_, (sockaddr *)&serverAddr, addrLen) < 0) {
            throw std::runtime_error("Could not connect to server");
        }

        setupStream(width, height);

        pkg_.data.resize(width*height*2);
    }
    ~TcpVideoStream() { close(socket_); }

    void *getBuffer() override {
        void* buffer = static_cast<void*> (pkg_.data.data());
        return buffer; }
    void update() override {
            if (!getPackage(socket_, pkg_))
                std::cerr << "Some trouble occured\n";
    }
};

void connectToServer() {
    try {
        std::unique_ptr<VideoStream> stream= std::make_unique<TcpVideoStream>("127.0.0.1", 4097, 320, 180);
        SDLWindow win("SDL window", 320, 180, stream);
        win.run();
    } catch (std::exception const &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
    }
}

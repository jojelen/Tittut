#pragma once

#include "sdl.hpp"
#include "videostream.hpp"

#include <arpa/inet.h>
#include <assert.h>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <tuple>
#include <unistd.h>
#include <vector>

#define HEADER_SIZE 16

// TODO: Endieness has to be the same on each machine.

enum class PKG_TYPE {
    INVALID = -1,
    CLOSED = 0,
    STREAM_CONFIG = 1,
    FRAME = 2,
    TEXT = 3,
    NUM_TYPES = 4
};

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
    PKG_TYPE type;
    std::vector<uint8_t> data;
};

// Read out the next package's type and data size.
std::tuple<PKG_TYPE, uint64_t> readPackageHeader(int socket) {
    std::vector<uint8_t> header(HEADER_SIZE);
    int bytes = recv(socket, static_cast<void *>(header.data()), HEADER_SIZE,
                     MSG_WAITALL);
    if (bytes == 0) {
        std::cout << "Connection closed\n";
        return std::tuple{PKG_TYPE::CLOSED, 0};
    } else if (bytes < 0) {
        throw std::runtime_error(
            std::string(
                "Failed reading reading out package header from socket: ") +
            strerror(errno));
    }

    uint64_t dataSize = 0;
    std::memcpy(&dataSize, header.data(), sizeof(uint64_t));
    uint64_t pkgType = 0;
    std::memcpy(&pkgType, header.data() + sizeof(uint64_t), sizeof(uint64_t));

    const static uint64_t MAX_TYPES =
        static_cast<uint64_t>(PKG_TYPE::NUM_TYPES);
    PKG_TYPE type = (pkgType >= MAX_TYPES) ? PKG_TYPE::INVALID
                                           : static_cast<PKG_TYPE>(pkgType);

    std::cout << "Recieved type " << static_cast<size_t>(pkgType)
              << " message of " << dataSize << " bytes\n";

    return std::tuple{type, dataSize};
}

void readPackageData(int socket, Package &pkg, uint64_t size) {
    if (size > pkg.data.size())
        pkg.data.resize(size);

    int bytes =
        recv(socket, static_cast<void *>(pkg.data.data()), size, MSG_WAITALL);
    if (bytes < 0) {
        throw std::runtime_error(
            std::string(
                "Failed reading reading out package data from socket: ") +
            strerror(errno));
    }

    // DEBUG
    std::cout << "Read " << bytes << " of " << size << " bytes of type "
              << static_cast<int>(pkg.type) << " message\n";
}

void getPackage(int socket, Package &pkg) {
    uint64_t dataSize = 0;
    std::tie(pkg.type, dataSize) = readPackageHeader(socket);

    if (pkg.type > PKG_TYPE::CLOSED)
        readPackageData(socket, pkg, dataSize);
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
    assert(vec.size() >= idx * sizeof(uint64_t));

    uint64_t num = 0;
    std::memcpy(&num, vec.data() + idx * sizeof(uint64_t), sizeof(uint64_t));

    return static_cast<uint64_t>(num);
}

void sendPackage(int socket, PKG_TYPE type, const std::vector<uint8_t> &data) {
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

void sendBuffer(int socket, const void *buffer, size_t bufferSize) {
    std::vector<uint8_t> header;
    addNumToVec(bufferSize, header);
    addNumToVec(static_cast<size_t>(PKG_TYPE::FRAME), header);

    assert(header.size() == HEADER_SIZE);

    int bytes = send(socket, static_cast<const void *>(header.data()),
                     HEADER_SIZE, MSG_NOSIGNAL);
    if (bytes < 0) {
        throw std::runtime_error("Could not send header of package");
    }

    bytes = send(socket, buffer, bufferSize, MSG_NOSIGNAL);
    if (bytes < 0) {
        throw std::runtime_error("Could not send data of package");
    }
    std::cout << "Sent " << bytes << " bytes data with data size " << bufferSize
              << std::endl;
}

void sendMsg(int socket, std::string_view msg) {
    std::vector<uint8_t> data(msg.length());
    std::copy(msg.begin(), msg.end(), data.begin());
    std::cout << "Sending msg \"" << msg << "\", data.size=" << data.size()
              << std::endl;
    sendPackage(socket, PKG_TYPE::TEXT, data);
}

void readPackage(const Package &pkg) {
    if (pkg.type == PKG_TYPE::TEXT) {
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

    sendPackage(socket, PKG_TYPE::STREAM_CONFIG, data);
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

        pkg_.data.resize(width * height * 2);
    }

    ~TcpVideoStream() { close(socket_); }

    inline void *getBuffer() override {
        return static_cast<void *>(pkg_.data.data());
    }

    void update() override {
        getPackage(socket_, pkg_);
        if (pkg_.type == PKG_TYPE::TEXT)
            readPackage(pkg_);
    }
};

void connectToServer(const std::string &ip, int port, int width, int height) {
    try {
        std::unique_ptr<VideoStream> stream =
            std::make_unique<TcpVideoStream>(ip, port, width, height);
        std::string windowName =
            "Video stream from " + ip + ":" + std::to_string(port);
        SDLWindow win(windowName, width, height, stream);
        win.run();
    } catch (std::exception const &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
    }
}

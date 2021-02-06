#pragma once

#include <arpa/inet.h>
#include <assert.h>
#include <chrono>
#include <iostream>
#include <string_view>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h>

#ifndef NDEBUG
#define LOG(x) log(x)
#else
#define LOG(x)
#endif

#ifdef NDEBUG
#define TIMER(x)
#else
#define TIMER(x) Timer timer(x)
#endif

void log(const std::string &msg) { std::cout << msg << std::endl; }

// Prints the time since construction at time of destruction.
class Timer {
  public:
    Timer() : mMessage("Timer:") { mStart = std::chrono::steady_clock::now(); }

    Timer(std::string_view msg) : mMessage(msg) {
        mStart = std::chrono::steady_clock::now();
    }

    ~Timer() { printDuration(); }

    void printDuration() const {
        static constexpr long int MILLION = 1e6;
        static constexpr long int BILLION = 1e9;

        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double> duration = end - mStart;
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration)
                      .count();
        if (ns < MILLION) {
            std::cout << mMessage << ": " << ns << " ns\n";
        } else if (ns >= MILLION && ns <= BILLION) {
            auto ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(duration)
                    .count();
            std::cout << mMessage << ": " << ms << " ms\n";
        } else {
            size_t sec = static_cast<size_t>(
                std::chrono::duration_cast<std::chrono::seconds>(duration)
                    .count());

            size_t min = sec / 60;
            size_t hours = min / 60;
            min %= 60;
            sec = sec - 60 * 60 * hours - 60 * min;
            std::cout << mMessage << ": " << hours << " h " << min << " m "
                      << sec << " s\n";
        }
    }

  private:
    std::chrono::steady_clock::time_point mStart;
    std::string_view mMessage;
};

int createListenSocket(int port) {
    int localSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (localSocket == -1) {
        throw std::runtime_error("Could not create socket" +
                                 std::string(strerror(errno)));
    }

    struct sockaddr_in localAddress = {};
    localAddress.sin_family = AF_INET;
    localAddress.sin_addr.s_addr = INADDR_ANY;
    localAddress.sin_port = htons(port);

    if (bind(localSocket, (struct sockaddr *)&localAddress,
             sizeof(localAddress)) < 0) {
        throw std::runtime_error("Could not bind socket" +
                                 std::string(strerror(errno)));
    }

    return localSocket;
}

int connectTo(const std::string &ip, int port) {
    int connSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (connSocket < 0) {
        throw std::runtime_error("Could not create socket" +
                                 std::string(strerror(errno)));
    }

    struct sockaddr_in serverAddr = {};
    socklen_t addrLen = sizeof(struct sockaddr_in);

    serverAddr.sin_family = PF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    serverAddr.sin_port = htons(port);

    std::cout << "Connecting to " << ip << ":" << std::to_string(port)
              << "...\n";
    if (connect(connSocket, (sockaddr *)&serverAddr, addrLen) < 0) {
        throw std::runtime_error("Could not connect to " + ip + ":" +
                                 std::to_string(port) +
                                 std::string(strerror(errno)));
    }

    return connSocket;
}

void addNumToVec(uint64_t num, std::vector<uint8_t> &vec) {
    if (sizeof(size_t) > sizeof(uint64_t) && num >= UINT64_MAX)
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

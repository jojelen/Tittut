#pragma once

#include <chrono>
#include <iostream>
#include <string_view>

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
            std::cout << mMessage << ": " << hours << " h " << min << " m " << sec
                 << " s\n";
        }
    }

  private:
    std::chrono::steady_clock::time_point mStart;
    std::string_view mMessage;
};

#ifdef NDEBUG
#define TIMER(x)
#else
#define TIMER(x) Timer timer(x)
#endif

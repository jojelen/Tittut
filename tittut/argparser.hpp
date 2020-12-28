#pragma once

// Argparse
#include <algorithm>
#include <optional>
#include <iostream>
#include <variant>
#include <vector>
#include <string_view>

class Arg {
public:
  std::string name_;
  std::optional<std::string> optionalName_;
  std::variant<std::string, bool, int> value_;

  Arg(std::string_view name) : name_(name.data()) {}

  template <typename T> Arg &defaultValue(T val) {
    value_ = val;
    return *this;
  }

  Arg &optional(std::string_view option) {
    optionalName_ = option.data();
    return *this;
  }
};

void assignArg(Arg *arg, const char *argv) {
    if (std::holds_alternative<int>(arg->value_)) {
        std::string strArg(argv);
        arg->value_ = std::stoi(strArg);
    }
    else if (std::holds_alternative<std::string>(arg->value_)) {
        arg->value_ = std::string(argv);
    }
    else {
        throw std::runtime_error(std::string("Error while assigning ") + argv +
                " to "+ arg->name_);
    }
}

class ArgParser {
private:
  std::string title_;
  std::string description_;
  std::vector<Arg> args_;

public:
  ArgParser(std::string_view title) : title_(title.data()) {}

  void description(std::string_view desc) { description_ = desc.data(); }

  Arg &addArg(std::string_view name) {
    args_.emplace_back(name);
    return args_.back();
  }

  void parse(int argc, const char *argv[]) {
    std::vector<Arg *> positionalArgs;
    std::vector<Arg *> optionalArgs;

    for (auto &arg : args_) {
      if (arg.optionalName_.has_value())
        optionalArgs.push_back(&arg);
      else
        positionalArgs.push_back(&arg);
    }

    size_t numPositional = 0;
    for (int i = 1; i < argc; ++i) {
      std::string currArg(argv[i]);
      if (currArg == "--help" || currArg == "-h")
        printHelp(argv[0]);
      else if (currArg[0] == '-') {
        bool foundArg = false;
        for (auto &arg : optionalArgs) {
          if (arg->optionalName_.value() == currArg) {
            foundArg = true;
            if (std::holds_alternative<bool>(arg->value_)) {
              bool newValue = !std::get<bool>(arg->value_);
              arg->value_ = newValue;
              break;
            } else if (i >= argc - 1)
              throw std::invalid_argument(std::string("Invalid option: ") + argv[i] +
                                          ", is missing a value");
            assignArg(arg, argv[i+1]);
            i++;
            break;
          }
        }
        if (!foundArg)
          throw std::invalid_argument(std::string("Invalid option: ") + argv[i]);
      } else {
        assignArg(positionalArgs[numPositional++], argv[i]);
      }
    }
  }

  void printHelp(const char *argvZero) {
    std::cout << title_ << "\n\n" << description_ << "\n";

    std::vector<Arg *> positionalArgs;
    std::vector<Arg *> optionalArgs;

    for (auto &arg : args_) {
      if (arg.optionalName_.has_value())
        optionalArgs.push_back(&arg);
      else
        positionalArgs.push_back(&arg);
    }

    // Sort optional arguments alphabetically,
    std::sort(
        optionalArgs.begin(), optionalArgs.end(), [](Arg *arg1, Arg *arg2) {
          return arg1->optionalName_.value() < arg2->optionalName_.value();
        });

    std::cout << "\nUsage: " << argvZero;
    if (!optionalArgs.empty())
      std::cout << " [OPTIONS]";
    for (auto &arg : positionalArgs)
      std::cout << " " << arg->name_;
    std::cout << std::endl;

    if (!optionalArgs.empty()) {
      std::cout << "\nOptional arguments:\n";
      for (auto &arg : optionalArgs) {
        std::cout << "\t" << arg->optionalName_.value() << "\t" << arg->name_
                  << std::endl;
      }
    }

    if (!positionalArgs.empty()) {
      std::cout << "\nPositional arguments:\n";
      for (auto &arg : positionalArgs) {
        std::cout << "\t" << arg->name_ << std::endl;
      }
    }
  }

  template<typename T>
      T get(std::string_view name) {
          for (auto & arg: args_){
              if (arg.name_ == name && std::holds_alternative<T>(arg.value_)) {
                  return std::get<T>(arg.value_);
              }
          }

          throw std::invalid_argument(std::string("Failed retrieving ") + std::string(name));
      }
};

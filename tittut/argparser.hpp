// Small argparser written in C++17.
//
// Example of usage:
//
//  ArgParser parser("Tittut");
//  parser.description("Streaming application using Video4Linux and SDL2.");
//  parser.addArg("width").optional("-x").defaultValue(320);
//  parser.addArg("height").optional("-y").defaultValue(180);
//  parser.addArg("debug").optional("-d").defaultValue(false);
//
//  parser.parse(argc, argv);
//
//  int width = parser.get<int>("width");
//  int height = parser.get<int>("height");
#pragma once

#include <algorithm>
#include <optional>
#include <iostream>
#include <variant>
#include <vector>
#include <string_view>

class Arg {
public:
  std::string name_;
  std::optional<std::string> desc_;
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

  Arg &description(std::string_view desc) {
    desc_ = desc.data();
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
            " to " + arg->name_);
  }
}

class ArgParser {
private:
  std::string title_;
  std::string description_;
  std::vector<Arg> args_;
  std::vector<Arg *> positionalArgs_;
  std::vector<Arg *> optionalArgs_;

  void printHelp(const char *argvZero) {
    std::cout << title_ << "\n\n" << description_ << "\n";


    // Sort optional arguments alphabetically,
    std::sort(
        optionalArgs_.begin(), optionalArgs_.end(), [](Arg *arg1, Arg *arg2) {
          return arg1->optionalName_.value() < arg2->optionalName_.value();
        });

    std::cout << "\nUsage: " << argvZero;
    if (!optionalArgs_.empty())
      std::cout << " [OPTIONS]";
    for (auto &arg : positionalArgs_)
      std::cout << " " << arg->name_;
    std::cout << std::endl;

    if (!optionalArgs_.empty()) {
      std::cout << "\nOptional arguments:\n";
      for (auto &arg : optionalArgs_) {
        std::cout << "\t" << arg->optionalName_.value() << "\t" << arg->name_;
        if (arg->desc_.has_value())
            std::cout << "\t" << arg->desc_.value();
        std::cout << std::endl;
      }
    }

    if (!positionalArgs_.empty()) {
      std::cout << "\nPositional arguments:\n";
      for (auto &arg : positionalArgs_) {
        std::cout << "\t" << arg->name_;
        if (arg->desc_.has_value())
            std::cout << "\t" << arg->desc_.value();
        std::cout << std::endl;
      }
    }
  }

  void splitArgs() {
    optionalArgs_.clear();
    positionalArgs_.clear();
    for (auto &arg : args_) {
      if (arg.optionalName_.has_value())
        optionalArgs_.push_back(&arg);
      else
        positionalArgs_.push_back(&arg);
    }
  }

public:
  ArgParser(std::string_view title) : title_(title.data()) {
      addArg("Help")
          .optional("-h")
          .defaultValue(false)
          .description("Print help.");
  }

  void description(std::string_view desc) { description_ = desc.data(); }

  Arg &addArg(std::string_view name) {
    args_.emplace_back(name);
    return args_.back();
  }

  void parse(int argc, const char *argv[]) {
    splitArgs();

    size_t numPositional = 0;
    for (int i = 1; i < argc; ++i) {
      std::string currArg(argv[i]);
      if (currArg == "--help" || currArg == "-h"){
        printHelp(argv[0]);
        exit(0);
      }
      else if (currArg[0] == '-') {
        bool foundArg = false;
        for (auto &arg : optionalArgs_) {
          if (arg->optionalName_.value() == currArg) {
            foundArg = true;
            if (std::holds_alternative<bool>(arg->value_)) {
              bool newValue = !std::get<bool>(arg->value_);
              arg->value_ = newValue;
              break;
            } else if (i >= argc - 1)
              throw std::invalid_argument(std::string("Invalid option: ") +
                      argv[i] + ", is missing a value");
            assignArg(arg, argv[i+1]);
            i++;
            break;
          }
        }
        if (!foundArg)
          throw std::invalid_argument(std::string("Invalid option: ") +
                                      argv[i]);
      } else {
        assignArg(positionalArgs_[numPositional++], argv[i]);
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

      throw std::invalid_argument(std::string("Failed retrieving ") +
                                  std::string(name));
  }
};

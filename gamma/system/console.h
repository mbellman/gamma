#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <tuple>

#include "system/type_aliases.h"

namespace Gamma {
  class Console {
  public:
    template<typename ...Args>
    static void log(Args&& ...args) {
      out(args...);
      std::cout << "\n";
    }

    static std::string* getMessages() {
      return Console::messages;
    }

  private:
    static std::stringstream output;
    // @todo store timestamps of each message so they
    // can eventually be cleared automatically
    static std::string messages[5];
    static uint32 index;

    template<typename Arg, typename ...Args>
    static void out(Arg& arg, Args&& ...args) {
      output << arg;

      if constexpr (sizeof...(args) > 0) {
        output << " ";
        out(args...);
      } else {
        std::cout << output.str();

        // @todo instead of shifting each old message back,
        // cycle through message slots when adding new messages
        // and retrieve/display them by iterating from index ->
        // index + 5 with overflow back to 0
        for (uint32 i = 4; i > 0; i--) {
          if (messages[i - 1].size() > 0) {
            messages[i] = messages[i - 1];
          }
        }

        messages[0] = output.str();

        // Reset output stream
        output.str(std::string());
        output.clear();
      }
    }
  };
}
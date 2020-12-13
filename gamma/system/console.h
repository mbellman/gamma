#pragma once

#include <iostream>
#include <tuple>

namespace Gamma {
  template<typename ...Args>
  void log(Args&& ...args) {
    out(args...);
    std::cout << "\n";
  }

  template<typename Arg, typename ...Args>
  void out(Arg& arg, Args&& ...args) {
    std::cout << arg;

    if constexpr (sizeof...(args) > 0) {
      std::cout << " ";
      out(args...);
    }
  }
}
#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace Gamma {
  class Signaler {
  public:
    ~Signaler() {
      for (auto& [key, listeners] : listenerMap) {
        listeners.clear();
      }

      listenerMap.clear();
    }

    template<typename T>
    void on(std::string event, const std::function<void(T)>& listener) {
      listenerMap[event].push_back([=](void* data) {
        listener((T)data);
      });
    }

  protected:
    template<typename T>
    void signal(std::string event, T data) {
      auto& listeners = listenerMap[event];

      for (auto& listener : listeners) {
        listener((void*)data);
      }
    }

  private:
    std::map<std::string, std::vector<std::function<void(void*)>>> listenerMap;
  };
}
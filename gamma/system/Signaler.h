#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace Gamma {
  struct BaseListenerRecord {
    virtual ~BaseListenerRecord() {};
  };

  template<typename Data>
  struct ListenerRecord : BaseListenerRecord {
    using Listener = std::function<void(Data)>;

    const Listener listener;

    ListenerRecord(const Listener& listener) : listener(listener) {};
  };

  class BaseSignaler {
  public:
    virtual ~BaseSignaler() {
      for (auto& [event, eventRecords] : records) {
        for (auto* record : eventRecords) {
          delete record;
        }
      }
    }

  protected:
    template<typename Data>
    void addListener(std::string event, const std::function<void(Data)>& listener) {
      records[event].push_back(new ListenerRecord<Data>(listener));
    }

    template<typename Data>
    void dispatch(std::string event, Data data) {
      auto& listenerRecords = records[event];

      for (auto* listenerRecord : listenerRecords) {
        auto* record = (ListenerRecord<Data>*)listenerRecord;

        record->listener(data);
      }
    }

  private:
    std::map<std::string, std::vector<BaseListenerRecord*>> records;
  };

  template<typename A>
  class Signaler : BaseSignaler {
  public:
    void on(std::string event, const std::function<void(A)>& listener) { addListener(event, listener); }

  protected:
    void signal(std::string event, A data) { dispatch(event, data); }
  };

  // @TODO Investigate ways of achieving this without a second class definition + overloads
  template<typename A, typename B>
  class Signaler2 : BaseSignaler {
  public:
    void on(std::string event, const std::function<void(A)>& listener) { addListener(event, listener); }
    void on(std::string event, const std::function<void(B)>& listener) { addListener(event, listener); }

  protected:
    void signal(std::string event, A data) { dispatch(event, data); }
    void signal(std::string event, B data) { dispatch(event, data); }
  };
}
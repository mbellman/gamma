#include "system/Commander.h"
#include "system/console.h"

namespace Gamma {
  Commander::Commander() {
    input.on<Key>("keyup", [&](Key key) {
      if (key == Key::C || key == Key::TAB) {
        if (input.isKeyHeld(Key::CONTROL)) {
          resetCurrentCommand();
        } else {
          isEnteringCommand = true;
        }
      } else if (key == Key::ESCAPE && isEnteringCommand) {
        resetCurrentCommand();
      } else if (key == Key::ENTER && isEnteringCommand) {
        processCurrentCommand();
      }
    });

    input.on<Key>("keydown", [&](Key key) {
      if (key == Key::BACKSPACE && isEnteringCommand && currentCommand.length() > 0) {
        currentCommand.pop_back();
      }
    });

    input.on<char>("input", [&](char character) {
      if (isEnteringCommand) {
        currentCommand += character;
      }
    });
  }

  const std::string& Commander::getCommand() const {
    return currentCommand;
  }

  bool Commander::isOpen() const {
    return isEnteringCommand;
  }

  void Commander::processCurrentCommand() {
    Console::log("Command:", currentCommand);

    resetCurrentCommand();
  }

  void Commander::resetCurrentCommand() {
    currentCommand = "";
    isEnteringCommand = false;
  }
}
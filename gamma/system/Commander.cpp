#include "system/Commander.h"
#include "system/console.h"

namespace Gamma {
  Commander::Commander() {
    input.on<Key>("keydown", [&](Key key) {
      if (key == Key::C && input.isKeyHeld(Key::CONTROL) && isEnteringCommand) {
        resetCurrentCommand();
      } else if (key == Key::TAB) {
        if (isEnteringCommand) {
          resetCurrentCommand();
        } else {
          isEnteringCommand = true;
        }
      } else if (key == Key::BACKSPACE && isEnteringCommand && currentCommand.length() > 0) {
        currentCommand.pop_back();
      } else if (key == Key::ESCAPE && isEnteringCommand) {
        resetCurrentCommand();
      }
    });

    input.on<Key>("keyup", [&](Key key) {
      if (key == Key::ENTER && isEnteringCommand) {
        processCurrentCommand();
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
#include "system/Commander.h"
#include "system/console.h"
#include "system/flags.h"

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

  bool Commander::currentCommandIncludes(std::string match) {
    return currentCommand.find(match) != std::string::npos;
  }

  const std::string& Commander::getCommand() const {
    return currentCommand;
  }

  bool Commander::isOpen() const {
    return isEnteringCommand;
  }

  void Commander::processCurrentCommand() {
    if (currentCommandIncludes("enable")) {
      if (currentCommandIncludes("reflect")) {
        Gm_EnableFlags(GammaFlags::RENDER_REFLECTIONS);

        Console::log("[Gamma] Reflections enabled");
      }

      if (currentCommandIncludes("refract")) {
        Gm_EnableFlags(GammaFlags::RENDER_REFRACTIONS);

        Console::log("[Gamma] Refractions enabled");
      }
    } else if (currentCommandIncludes("disable")) {
      if (currentCommandIncludes("reflect")) {
        Gm_DisableFlags(GammaFlags::RENDER_REFLECTIONS);

        Console::log("[Gamma] Reflections disabled");
      }

      if (currentCommandIncludes("refract")) {
        Gm_DisableFlags(GammaFlags::RENDER_REFRACTIONS);

        Console::log("[Gamma] Refractions disabled");
      }
    }

    resetCurrentCommand();
  }

  void Commander::resetCurrentCommand() {
    currentCommand = "";
    isEnteringCommand = false;
  }
}
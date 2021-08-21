#include "system/Commander.h"
#include "system/console.h"
#include "system/flags.h"

namespace Gamma {
  struct Command {
    const char* keyword;
    const char* displayName;
    GammaFlags flag;
  };

  static Command commands[] = {
    { "reflect", "Reflections", GammaFlags::RENDER_REFLECTIONS },
    { "refract", "Refractions", GammaFlags::RENDER_REFRACTIONS },
    { "shadow", "Shadows", GammaFlags::RENDER_SHADOWS },
    { "debug buffers", "Debug buffers", GammaFlags::SHOW_DEBUG_BUFFERS }
  };

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
      for (uint32 i = 0; i < 4; i++) {
        auto& command = commands[i];

        if (currentCommandIncludes(command.keyword)) {
          Gm_EnableFlags(command.flag);

          Console::log("[Gamma]", command.displayName, "enabled");
        }
      }
    } else if (currentCommandIncludes("disable")) {
      for (uint32 i = 0; i < 4; i++) {
        auto& command = commands[i];

        if (currentCommandIncludes(command.keyword)) {
          Gm_DisableFlags(command.flag);

          Console::log("[Gamma]", command.displayName, "disabled");
        }
      }
    }

    resetCurrentCommand();
  }

  void Commander::resetCurrentCommand() {
    currentCommand = "";
    isEnteringCommand = false;
  }
}
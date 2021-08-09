#include "system/console.h"

namespace Gamma {
  std::stringstream Console::output;
  std::string Console::messages[5];
  uint32 Console::index = 0;
}
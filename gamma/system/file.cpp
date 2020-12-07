#include <fstream>

#include "system/file.h"

std::string Gamma::gm_LoadFile(const char* path) {
  std::string source;
  std::ifstream file(path);

  if (file.fail()) {
    printf("[FileLoader] Error opening file: %s\n", path);
  } else {
    std::string line;

    while (std::getline(file, line)) {
      source.append(line + "\n");
    }
  }

  file.close();

  return source;
}
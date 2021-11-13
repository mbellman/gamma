#include <vector>

#include "system/file.h"
#include "system/yaml_parser.h"

namespace Gamma {
  /**
   * Gm_SplitString
   * --------------
   *
   * @todo description
   * @todo create string_helpers.h/cpp
   */
  static std::vector<std::string> Gm_SplitString(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> values;
    uint32 offset = 0;
    uint32 found = 0;

    while ((found = str.find(delimiter, offset)) != std::string::npos) {
      values.push_back(str.substr(offset, found - offset));

      offset = found + delimiter.size();
    }

    return values;
  }

  /**
   * Gm_TrimString
   * -------------
   *
   * @todo description
   * @todo create string_helpers.h/cpp
   */
  static std::string Gm_TrimString(const std::string& str) {
    std::string trimmed;

    for (uint32 i = 0; i < str.size(); i++) {
      if (str[i] != ' ' && str[i] != ' ') {
        trimmed += str[i];
      }
    }

    return trimmed;
  }

  /**
   * Gm_ParseYamlFile
   * ----------------
   *
   * @todo make fully functional
   */
  YamlStructure& Gm_ParseYamlFile(const char* path) {
    std::string fileContents = Gm_LoadFileContents(path);
    auto lines = Gm_SplitString(fileContents, "\n");
    uint32 depth = 0;
    auto* structure = new YamlStructure();

    for (auto& line : lines) {
      // @optimize we can walk through each character in the file contents
      // and skip over whitespace + chunk each line into a vector in a combined
      // routine, rather than splitting first and trimming here
      auto trimmed = Gm_TrimString(line);

      if (trimmed == "{") {
        depth++;
      } else if (trimmed == "}") {
        depth--;
      } else if (trimmed.find(":") != std::string::npos) {
        // @todo if the property name is followed by {,
        // we need to create a new YamlStructure() and
        // add the inner fields to that, recursively
        auto propertyName = trimmed.substr(0, trimmed.find(":"));
        YamlProperty property;

        structure->properties[propertyName] = property;
      }
    }

    return *structure;
  }

  /**
   * Gm_FreeYamlStructure
   * --------------------
   */
  void Gm_FreeYamlStructure(YamlStructure* structure) {
    for (auto& [ key, value ] : structure->properties) {
      if (value.structure != nullptr) {
        Gm_FreeYamlStructure(value.structure);
      }
    }

    structure->properties.clear();

    delete structure;
  }
}
#include <vector>

#include "system/assert.h"
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
   * @todo parse plain data values
   */
  YamlObject& Gm_ParseYamlFile(const char* path) {
    std::string fileContents = Gm_LoadFileContents(path);
    auto lines = Gm_SplitString(fileContents, "\n");
    auto* root = new YamlObject();
    std::vector<YamlObject*> objectStack;

    objectStack.push_back(root);

    for (auto& line : lines) {
      // @optimize we can walk through each character in the file contents
      // and skip over whitespace + chunk each line into a vector in a combined
      // routine, rather than splitting first and trimming here
      auto trimmed = Gm_TrimString(line);

      if (trimmed[0] == '}') {
        // End of object
        objectStack.pop_back();
      } else if (trimmed.find(":") != std::string::npos) {
        // Property declaration
        YamlProperty property;
        auto propertyName = trimmed.substr(0, trimmed.find(":"));
        auto* currentObject = objectStack.back();

        // Add the property to the current object
        currentObject->properties[propertyName] = property;

        if (trimmed.back() == '{') {
          // If the property defines a nested object,
          // create it and push it onto the stack
          auto* nestedObject = new YamlObject();

          objectStack.push_back(nestedObject);

          // Assign the property to the nested object
          currentObject->properties[propertyName].object = nestedObject;
        }
      }
    }

    // Remove the root object from the stack
    objectStack.pop_back();

    assert(objectStack.size() == 0, "Malformed YAML file");

    return *root;
  }

  /**
   * Gm_FreeYamlObject
   * --------------------
   */
  void Gm_FreeYamlObject(YamlObject* object) {
    for (auto& [ key, value ] : object->properties) {
      if (value.object != nullptr) {
        Gm_FreeYamlObject(value.object);
      }
    }

    object->properties.clear();

    delete object;
  }
}
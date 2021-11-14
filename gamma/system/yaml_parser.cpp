#include <cctype>
#include <vector>

#include "system/assert.h"
#include "system/file.h"
#include "system/string_helpers.h"
#include "system/yaml_parser.h"

namespace Gamma {
  /**
   * Gm_ParsePrimitiveValue
   * ----------------------
   *
   * @todo description
   */
  static void* Gm_ParsePrimitiveValue(const std::string& str) {
    // Check for boolean literals
    if (str == "true") {
      return new bool(true);
    } else if (str == "false") {
      return new bool(false);
    }

    // Check for numbers
    bool isNumber = std::isdigit(str[0]);

    if (isNumber) {
      return new int(std::stoi(str));
    }

    // Fall back to string
    return new std::string(str);
  }

  /**
   * Gm_ParseYamlFile
   * ----------------
   */
  YamlObject& Gm_ParseYamlFile(const char* path) {
    std::vector<YamlObject*> objectStack;
    std::string fileContents = Gm_LoadFileContents(path);
    auto lines = Gm_SplitString(fileContents, "\n");
    auto* root = new YamlObject();

    objectStack.push_back(root);

    for (auto& line : lines) {
      // @optimize we can walk through each character in the file contents
      // and skip over whitespace + chunk each line into a vector in a combined
      // routine, rather than splitting first and trimming here
      auto trimmedLine = Gm_TrimString(line);

      if (trimmedLine[0] == '}') {
        // End of object
        objectStack.pop_back();
      } else if (trimmedLine.find(":") != std::string::npos) {
        // Property declaration
        YamlProperty property;
        auto propertyName = trimmedLine.substr(0, trimmedLine.find(":"));
        auto& currentObject = *objectStack.back();

        if (trimmedLine.back() == '{') {
          // Nested object property
          auto* nestedObject = new YamlObject();

          objectStack.push_back(nestedObject);

          property.object = nestedObject;
        } else if (trimmedLine.back() == '[') {
          // Array property
          // @todo
        } else {
          // Primitive property (string, number, or boolean)
          uint32 vStart = trimmedLine.find(":") + 1;
          uint32 vLength = trimmedLine.find(",") - vStart;
          auto value = Gm_TrimString(trimmedLine.substr(vStart, vLength));

          property.primitive = Gm_ParsePrimitiveValue(value);
        }

        currentObject[propertyName] = property;
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
    for (auto& [ key, value ] : *object) {
      if (value.object != nullptr) {
        // Recursively delete nested objects
        Gm_FreeYamlObject(value.object);
      } else if (value.primitive != nullptr) {
        // Delete primitives
        delete value.primitive;
      }
    }

    object->clear();

    delete object;
  }
}
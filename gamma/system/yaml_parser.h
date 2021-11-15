#pragma once

#include <map>
#include <string>
#include <vector>

#include "system/string_helpers.h"
#include "system/type_aliases.h"

namespace Gamma {
  struct YamlProperty;

  /**
   * YamlObject
   * ----------
   *
   * @todo description
   */
  typedef std::map<std::string, YamlProperty> YamlObject;

  /**
   * YamlArray
   * ---------
   *
   * @todo description
   */
  template<typename T>
  struct YamlArray : std::vector<T> {
    virtual ~YamlArray() {
      for (auto* value : *this) {
        delete value;
      }
    }
  };

  /**
   * YamlProperty
   * ------------
   *
   * @todo description
   */
  struct YamlProperty {
    /**
     * For leaf properties (strings, numbers,
     * booleans, or arrays), the regular value.
     * Remains nullptr otherwise.
     */
    void* value = nullptr;
    /**
     * For nested object properties, a pointer
     * to that object. Remains nullptr otherwise.
     */
    YamlObject* object = nullptr;
  };

  /**
   * Gm_ParseYamlFile
   * ----------------
   *
   * @todo description
   */
  YamlObject& Gm_ParseYamlFile(const char* path);

  /**
   * Gm_ReadYamlProperty
   * -------------------
   *
   * @todo description
   */
  template<typename T>
  T Gm_ReadYamlProperty(const YamlObject& object, const std::string& propertyChain) {
    auto properties = Gm_SplitString(propertyChain, ".");
    auto* currentObject = &object;

    for (uint32 i = 0; i < properties.size() - 1; i++) {
      currentObject = currentObject->at(properties[i]).object;
    }

    return *(T*)currentObject->at(properties.back()).value;
  }

  /**
   * Gm_FreeYamlObject
   * -----------------
   *
   * @todo description
   */
  void Gm_FreeYamlObject(YamlObject* object);
}
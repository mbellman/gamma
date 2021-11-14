#pragma once

#include <string>
#include <map>

#include "system/string_helpers.h"
#include "system/type_aliases.h"

namespace Gamma {
  struct YamlProperty;

  typedef std::map<std::string, YamlProperty> YamlObject;

  struct YamlProperty {
    /**
     * For plain data values, the value of the property.
     * Remains nullptr otherwise.
     */
    void* primitive = nullptr;
    /**
     * For YamlObject properties, the nested object.
     * Remains nullptr otherwise.
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

    return *(T*)currentObject->at(properties.back()).primitive;
  }

  /**
   * Gm_FreeYamlObject
   * -----------------
   *
   * @todo description
   */
  void Gm_FreeYamlObject(YamlObject* object);
}
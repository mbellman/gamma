#pragma once

#include <string>
#include <map>

#include "system/type_aliases.h"

namespace Gamma {
  struct YamlObject;

  struct YamlProperty {
    /**
     * For plain data values, the value of the property.
     * Remains nullptr otherwise.
     */
    void* plainValue = nullptr;
    /**
     * For YamlObject properties, the nested object.
     * Remains nullptr otherwise.
     */
    YamlObject* object = nullptr;
  };

  struct YamlObject {
    std::map<std::string, YamlProperty> properties;
  };

  YamlObject& Gm_ParseYamlFile(const char* path);
  void Gm_FreeYamlObject(YamlObject* object);
}
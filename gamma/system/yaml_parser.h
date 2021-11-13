#pragma once

#include <string>
#include <map>

#include "system/type_aliases.h"

namespace Gamma {
  struct YamlStructure;

  struct YamlProperty {
    /**
     * For plain data values, the value of the property.
     * Remains nullptr otherwise.
     */
    void* rawValue = nullptr;
    /**
     * For YamlStructure properties, the nested structure.
     * Remains nullptr otherwise.
     */
    YamlStructure* structure = nullptr;
  };

  struct YamlStructure {
    std::map<std::string, YamlProperty> properties;
  };

  YamlStructure& Gm_ParseYamlFile(const char* path);
  void Gm_FreeYamlStructure(YamlStructure* structure);
}
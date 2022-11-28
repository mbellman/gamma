#!/bin/bash

format_project_name() {
  declare name="$1"

  # To lowercase
  name=$(echo $name | tr '[:upper:]' '[:lower:]')

  # Replace whitespace with '-'
  name=$(echo $name | tr -s ' ' | tr ' ' '-')

  echo $name
}

echo "What do you want to name this project?"

read project_name

declare formatted_project_name=$(format_project_name "$project_name")

echo "Okay, it'll be named $formatted_project_name!"

sed -i "s/\"Gamma\"/\"$project_name\"/g" gamma.sln
sed -i "s/\"Gamma.vcxproj\"/\"$formatted_project_name.vcxproj\"/g" gamma.sln

mv 'gamma.sln' "$formatted_project_name.sln"

mv 'Gamma.vcxproj' "$formatted_project_name.vcxproj"
mv 'Gamma.vcxproj.user' "$formatted_project_name.vcxproj.user"
mv 'Gamma.vcxproj.filters' "$formatted_project_name.vcxproj.filters"
#!/bin/bash

format_project_name() {
  declare name="$1"

  # To lowercase
  name=$(echo $name | tr '[:upper:]' '[:lower:]')

  # Replace whitespace with '-'
  name=$(echo $name | tr -s ' ' | tr ' ' '-')

  echo $name
}

# Naming
echo "What do you want to name this project?"

read project_name

declare formatted_project_name=$(format_project_name "$project_name")

echo "Okay, it'll be named $formatted_project_name!"

sed -i "s/\"Gamma\"/\"$project_name\"/g" gamma.sln
sed -i "s/\"Gamma.vcxproj\"/\"$formatted_project_name.vcxproj\"/g" gamma.sln

mv 'gamma.sln' "$formatted_project_name.sln"
mv 'game.vcxproj' "$formatted_project_name.vcxproj"
mv 'game.vcxproj.filters' "$formatted_project_name.vcxproj.filters"

# Create Release/Debug directories with DLLs
mkdir Release
mkdir Debug

cp dlls/* Release
cp dlls/* Debug

# Remove zip/setup script
rm gamma.zip
rm setup.sh
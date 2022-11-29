#!/bin/bash

declare -a files=(
  game/*
  gamma/*
  fonts/*
  dlls/*
  .vscode/*
  external/*
  ".gitignore"
  "game.vcxproj"
  "game.vcxproj.filters"
  "gamma.sln"
  "setup.sh"
)

tar -a -c -f gamma.zip "${files[@]}"
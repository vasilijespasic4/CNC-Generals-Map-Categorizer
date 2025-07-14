cmake_minimum_required(VERSION 3.10)

project(GeneralsMapCategorizer C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(categorizer src/RefPack.c src/File.c src/main.c)
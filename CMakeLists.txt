cmake_minimum_required(VERSION 3.24)
project(music-cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

#FTXUI for ui part of the program
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
  GIT_TAG v6.1.9
)

#nlohmann-json (for favs and saved directories)
FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)

FetchContent_MakeAvailable(ftxui json)

add_executable(music-cpp
  src/main.cpp
  src/library.cpp
  src/config.cpp
  src/player.cpp
  src/ui.cpp
)

target_include_directories(music-cpp PRIVATE vendor)

target_link_libraries(music-cpp PRIVATE
  ftxui::screen
  ftxui::dom
  ftxui::component
  nlohmann_json::nlohmann_json
)

if(UNIX AND NOT APPLE)
  target_link_libraries(music-cpp PRIVATE pthread m dl)
endif()

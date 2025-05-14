cmake_minimum_required(VERSION 3.14)
project(keyboard-sounds VERSION 1.0.0 LANGUAGES CXX)

# — enforce C++17 —
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

# — point at your local SFML 3 unpack (./libs/SFML) —
set(SFML_ROOT "${CMAKE_SOURCE_DIR}/libs/SFML")
list(APPEND CMAKE_PREFIX_PATH "${SFML_ROOT}")

# — find SFML 3 Audio & System —
find_package(SFML 3 REQUIRED
  COMPONENTS Audio System
  HINTS "${SFML_ROOT}"
  PATH_SUFFIXES lib/cmake/SFML
)

# — gather all .cpp under src/ —
file(GLOB_RECURSE SOURCES
  "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# — create exe from those sources —
add_executable(${PROJECT_NAME}
  WIN32
  ${SOURCES}
)

# — include your headers folder —
target_include_directories(${PROJECT_NAME} PRIVATE
  "${CMAKE_SOURCE_DIR}/include"
)

# — compile defs for Unicode —
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)

# — link SFML::Audio, SFML::System & Win32 libs —
target_link_libraries(${PROJECT_NAME} PRIVATE
  SFML::Audio
  SFML::System
  winmm
  user32
  gdi32
  comctl32
  uxtheme
)

# — enable Windows .rc resource compilation —
set(CMAKE_RC_COMPILER_INIT windres)
enable_language(RC)

# — copy sounds/ into the build folder after each build —
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/sounds"
    "${CMAKE_BINARY_DIR}/sounds"
)

# — copy required SFML DLLs to build directory —
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "Copying SFML DLLs to build directory..."
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${SFML_ROOT}/bin/sfml-audio-3.dll"
    "${SFML_ROOT}/bin/sfml-system-3.dll"
    "${CMAKE_BINARY_DIR}/"
)

# Check for debug build and copy debug DLLs if needed
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Debug build - copying debug DLLs..."
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${SFML_ROOT}/bin/sfml-audio-d-3.dll"
      "${SFML_ROOT}/bin/sfml-system-d-3.dll"
      "${CMAKE_BINARY_DIR}/"
  )
endif()

# — optional install rule —
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# — diagnostic messages —
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "SFML v${SFML_VERSION} detected under ${SFML_ROOT}")

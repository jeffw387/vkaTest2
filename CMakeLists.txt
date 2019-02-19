cmake_minimum_required(VERSION 3.10)
project(vkaTest2)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
   file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.13/conan.cmake"
                 "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_cmake_run(
  BASIC_SETUP 
  BUILD missing
  PROFILE $ENV{CONANPROFILE}
  CONANFILE conanfile.py
  SETTINGS cppstd=17
  BUILD_TYPE Debug)

add_executable(main ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp)
target_link_libraries(main PUBLIC ${CONAN_LIBS})

add_subdirectory(src)
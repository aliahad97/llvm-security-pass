cmake_minimum_required(VERSION 3.20)
project(HelloFunctionPass LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "HelloFunctionPass: using LLVMConfig.cmake in ${LLVM_DIR}")
message(STATUS "HelloFunctionPass: LLVM version ${LLVM_PACKAGE_VERSION}")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

add_library(HelloFunctionPass MODULE HelloFunctionPass.cpp)

if(APPLE)
  target_link_options(HelloFunctionPass PRIVATE "-Wl,-undefined,dynamic_lookup")
endif()

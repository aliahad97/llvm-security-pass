cmake_minimum_required(VERSION 3.20)
project(HelloModulePass LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "HelloModulePass: using LLVMConfig.cmake in ${LLVM_DIR}")
message(STATUS "HelloModulePass: LLVM version ${LLVM_PACKAGE_VERSION}")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

add_library(HelloModulePass MODULE HelloModulePass.cpp)

# macOS: the plugin references LLVM symbols that the opt binary provides at
# load time. Allow the linker to leave them unresolved in the .dylib.
if(APPLE)
  target_link_options(HelloModulePass PRIVATE "-Wl,-undefined,dynamic_lookup")
endif()

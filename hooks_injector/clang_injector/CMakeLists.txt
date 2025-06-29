cmake_minimum_required(VERSION 3.16)
project(XTraceClangInjector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Clang and LLVM
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Found Clang in: ${CLANG_CMAKE_DIR}")

# Set up LLVM/Clang includes and link directories
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Find required Clang libraries
# Note: We use LLVM shared library to avoid command line option conflicts
set(CLANG_LIBS
    clangTooling
    clangFrontend
    clangDriver
    clangSerialization
    clangCodeGen
    clangParse
    clangSema
    clangStaticAnalyzerFrontend
    clangStaticAnalyzerCheckers
    clangStaticAnalyzerCore
    clangAnalysis
    clangARCMigrate
    clangRewrite
    clangRewriteFrontend
    clangEdit
    clangAST
    clangLex
    clangBasic
    LLVM
)

# Create the executable
add_executable(xtrace-clang-injector
    src/main.cpp
    src/XTraceInjector.cpp
    src/MethodVisitor.cpp
    src/CodeRewriter.cpp
)

# Link libraries
target_link_libraries(xtrace-clang-injector ${CLANG_LIBS})

# Set up include directories for our project
target_include_directories(xtrace-clang-injector PRIVATE include)

# Compiler flags
target_compile_options(xtrace-clang-injector PRIVATE -fno-rtti)

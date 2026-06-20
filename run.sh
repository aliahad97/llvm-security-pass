#!/usr/bin/env bash
# Build the hello-world module + function passes and run both against
# examples/example.c. Reruns are incremental thanks to CMake.
#
# Override Homebrew LLVM location with: LLVM_PREFIX=/path/to/llvm ./run.sh

set -euo pipefail

LLVM_PREFIX="${LLVM_PREFIX:-/opt/homebrew/opt/llvm}"
if [[ ! -x "$LLVM_PREFIX/bin/opt" ]]; then
  echo "error: LLVM not found at $LLVM_PREFIX" >&2
  echo "       install with: brew install llvm" >&2
  echo "       or set LLVM_PREFIX=/path/to/your/llvm" >&2
  exit 1
fi
export PATH="$LLVM_PREFIX/bin:$PATH"
export LLVM_DIR="$LLVM_PREFIX/lib/cmake/llvm"

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD="$REPO/build"
mkdir -p "$BUILD"

echo ">>> Building module pass"
cmake -S "$REPO/hello-world-pass/module-pass" \
      -B "$BUILD/module-pass" \
      -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD/module-pass" -j

echo ">>> Building function pass"
cmake -S "$REPO/hello-world-pass/function-pass" \
      -B "$BUILD/function-pass" \
      -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD/function-pass" -j

echo ">>> Lowering examples/example.c to LLVM IR"
clang -O0 -S -emit-llvm "$REPO/examples/example.c" -o "$BUILD/example.ll"

# CMake's MODULE library suffix is platform-dependent (.so / .dylib / .dll),
# so glob rather than hardcode.
MODULE_PLUGIN=$(ls "$BUILD/module-pass"/libHelloModulePass.* | head -n1)
FUNC_PLUGIN=$(ls "$BUILD/function-pass"/libHelloFunctionPass.* | head -n1)

echo
echo "=== Module pass ==="
opt -load-pass-plugin="$MODULE_PLUGIN" \
    -passes="hello-module" \
    -disable-output "$BUILD/example.ll"

echo
echo "=== Function pass ==="
# function(...) adaptor: lifts the function pass into a module pipeline,
# so opt runs it once per defined function in the module.
opt -load-pass-plugin="$FUNC_PLUGIN" \
    -passes="function(hello-function)" \
    -disable-output "$BUILD/example.ll"

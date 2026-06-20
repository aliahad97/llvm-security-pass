# LLVM pass cheatsheet

Quick reference for working on this repo. Assumes Homebrew LLVM 22 on `$PATH`:

```bash
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
```

## C → LLVM IR

| Goal | Command |
|---|---|
| Textual IR (`.ll`) | `clang -S -emit-llvm foo.c -o foo.ll` |
| Bitcode (`.bc`) | `clang -c -emit-llvm foo.c -o foo.bc` |
| `.bc` ↔ `.ll` | `llvm-dis foo.bc -o foo.ll`  /  `llvm-as foo.ll -o foo.bc` |
| Keep variable names | add `-fno-discard-value-names` |
| Include debug info (`!dbg`) | add `-g` |
| `-O0` without the `optnone` blocker | add `-Xclang -disable-O0-optnone` |

Useful combo for pass development:

```bash
clang -O0 -Xclang -disable-O0-optnone -g -fno-discard-value-names \
      -S -emit-llvm foo.c -o foo.ll
```

## Build the pass plugin

```bash
# First time / clean config
cmake -S hello-world-pass/module-pass -B build/module-pass -DCMAKE_BUILD_TYPE=Debug
# Incremental rebuild
cmake --build build/module-pass -j
# Verbose (see compile command)
cmake --build build/module-pass --verbose
# Nuke a single pass
rm -rf build/module-pass
```

`Debug` build is what you want while developing — it enables `LLVM_DEBUG()` macros and gives usable lldb output.

## Run a pass with `opt`

```bash
PLUGIN=build/module-pass/libHelloModulePass.so

# Module pass
opt -load-pass-plugin=$PLUGIN -passes="hello-module" -disable-output foo.ll

# Function pass (must wrap with function(...))
opt -load-pass-plugin=$PLUGIN -passes="function(hello-function)" -disable-output foo.ll

# Mix with built-in passes
opt -load-pass-plugin=$PLUGIN -passes="mem2reg,hello-module,dce" -S foo.ll -o out.ll

# Write transformed IR instead of throwing it away
opt -load-pass-plugin=$PLUGIN -passes="hello-module" -S foo.ll -o out.ll
```

Drop `-disable-output` if you want the transformed IR; add `-S` for textual, omit for bitcode.

## Compile a program *through* the pass

Like a real toolchain — clang runs the plugin as part of its own pipeline:

```bash
clang -fpass-plugin=$PLUGIN -O1 foo.c -o foo
```

Useful for end-to-end testing of a hardening pass against a real binary.

## Inspect & verify IR

```bash
# Sanity-check IR is well-formed
opt -passes=verify -disable-output foo.ll

# Strip debug noise for readability
opt -passes=strip-debug -S foo.ll -o foo.stripped.ll

# Per-function CFG as Graphviz dot files (one .NAME.dot per function)
opt -passes=dot-cfg -disable-output foo.ll
dot -Tpng .add.dot -o add.png

# List every pass name opt knows about (confirm your plugin registered)
opt -load-pass-plugin=$PLUGIN --print-passes | grep hello
```

## Debug the pipeline

```bash
# Trace which passes run, which get skipped, and why
opt -load-pass-plugin=$PLUGIN -passes="function(hello-function)" \
    --debug-pass-manager -disable-output foo.ll

# Dump IR before/after every pass (or just yours)
opt ... -print-after-all -disable-output foo.ll
opt ... -print-after=hello-module -disable-output foo.ll
opt ... -print-changed -disable-output foo.ll   # only when IR changed

# Show the pipeline string opt would actually run
opt ... -print-pipeline-passes -disable-output foo.ll

# Time each pass / dump STATISTIC() counters
opt ... -time-passes -stats -disable-output foo.ll

# Turn on LLVM_DEBUG(dbgs() << ...) for a debug type (Debug build only)
opt ... -debug-only=my-tag -disable-output foo.ll
```

## Print from inside a pass

```cpp
errs() << "msg\n";              // unbuffered stderr — use this
F.print(errs());                // dump a Function
M.print(errs(), nullptr);       // dump a Module
I.print(errs()); errs() << "\n"; // dump an Instruction
BB.printAsOperand(errs(), false); // BB label
F.dump();  M.dump();            // Debug builds only, convenience
```

## Step through the pass in lldb

```bash
lldb -- $(brew --prefix llvm)/bin/opt \
     -load-pass-plugin=$PLUGIN -passes="function(hello-function)" \
     -disable-output foo.ll
(lldb) b HelloFunctionPass::run
(lldb) run
```

Requires a `Debug` cmake build of the plugin.

## IR → object / executable

```bash
llc foo.ll -o foo.s                   # assembly
llc -filetype=obj foo.ll -o foo.o     # object
clang foo.ll -o foo                   # straight through to executable
```

## Gotchas reminder (see CLAUDE.md for context)

- Function passes silently skipped at `-O0` → either `-Xclang -disable-O0-optnone` on clang, or `static bool isRequired() { return true; }` on the pass.
- Function passes need `function(...)` wrapping in `-passes=`.
- Plugin headers: `llvm/Plugins/PassPlugin.h` (not `llvm/Passes/...` — moved in LLVM 22).
- macOS plugin suffix is `.so`, not `.dylib`, with CMake `MODULE` libraries.

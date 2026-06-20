# llvm-security-pass

A personal sandbox for writing **out-of-tree LLVM passes focused on code
hardening**. Targets Homebrew LLVM 22 on macOS arm64, uses the new pass
manager throughout.

```bash
brew install llvm cmake
./run.sh
```

The script builds the two starter passes and runs them on `examples/example.c`.
See [`CLAUDE.md`](CLAUDE.md) for the build pattern, plugin skeleton, and the
gotchas worth knowing before writing the next pass.

## Layout

- `hello-world-pass/module-pass/` — counts instructions, module scope
- `hello-world-pass/function-pass/` — counts instructions, function scope
- `examples/` — C inputs to lower with `clang -S -emit-llvm`

Each pass is its own CMake project. Add a new one by copying a directory and
renaming.

## Exercise ideas — security passes to build next

A rough difficulty ramp. Each one is small enough to fit in a single
directory under `hello-world-pass/` (rename it once a non-trivial one lands).

### Warm-up — observation only

1. **Dangerous-function detector.** Walk every `CallInst`, warn on
   `strcpy`/`strcat`/`gets`/`sprintf`/`memcpy` with non-constant length.
   No IR mutation; just `errs()` diagnostics with source locations from
   `!dbg` metadata. Module pass.
2. **Indirect-call inventory.** List every `call`/`invoke` whose callee is
   not a `Function*` constant. Feeds later CFI work. Module pass.
3. **Risky-pointer-arithmetic scanner.** Flag every `getelementptr` on a
   stack array where the index is not a `ConstantInt`. Function pass.

### Stack & memory hardening

4. **Stack canary insertion (toy SSP).** For every function with a
   stack-allocated array ≥ N bytes, insert a canary `alloca` after the prologue,
   load a per-module random cookie, and verify at every `ret`. Calls
   `__stack_chk_fail` on mismatch. Function pass; demonstrates `IRBuilder`,
   `alloca` ordering, multi-exit handling.
5. **Stack-array bounds annotator.** Replace fixed-size stack `alloca`s with
   a runtime-checked wrapper: emit a length sidecar and bounds-check every
   GEP that derives from the alloca. Good intro to alias-style reasoning
   without doing real alias analysis.
6. **Heap-allocation tagger.** Wrap `malloc`/`calloc`/`realloc` to call a
   runtime that records size and pointer, and instrument `free` to verify.
   Cheap UAF/double-free sanitizer skeleton. Module pass + small runtime.

### Control-flow integrity

7. **Forward-edge CFI (type-id check).** Assign each function-type a tag,
   stash it in a global table, and instrument every indirect call to compare
   the callee's tag against the call site's expected tag before dispatching.
   Aborts on mismatch. Module pass.
8. **Backward-edge CFI (shadow stack).** At each function entry, push the
   return address onto a per-thread shadow stack; at every `ret`, pop and
   compare. Demonstrates inserting code at function boundaries and TLS
   access from IR.
9. **Return-address verifier.** Lighter than (8): hash the return address
   into a tag at entry, store in a callee-saved slot, recompute and compare
   at exit. Educational comparison piece against the shadow stack.

### Obfuscation (defensive flavour)

10. **Control-flow flattening.** Convert each function's CFG into a single
    dispatcher `switch` over a state variable. Classic OLLVM transform —
    teaches block splitting, PHI rewriting, and `DominatorTree` usage.
11. **Opaque predicate injection.** Insert `if (always_true_via_math)`
    branches before key blocks, with both arms producing the same effect.
    Stresses pass-ordering interactions with `simplifycfg`/`instcombine`.
12. **Constant-string encryption.** Find every `GlobalVariable` holding a
    C string, XOR-encrypt its initializer, emit a one-time decrypt thunk
    invoked from `llvm.global_ctors`. Module pass.

### Anti-tampering / integrity

13. **Self-checksum pass.** At link time, embed a hash of each function's
    `.text` bytes; at runtime, recompute and compare on entry. Requires
    coordinating with a linker script or post-link patch — fun rabbit hole.
14. **Anti-debug instrumentation.** Inject `ptrace(PTRACE_TRACEME)` or
    `isatty(STDIN)`-style probes at chosen entry points and divert on
    detection. macOS variant uses `sysctl(KERN_PROC)`.

### Side-channel mitigations

15. **Constant-time-cmp enforcer.** For functions annotated
    `__attribute__((annotate("ct")))`, verify the body contains no
    secret-dependent branches or variable-latency instructions (`div`,
    `udiv`, table-driven `switch`). Reports violations; doesn't transform.
16. **Speculation barrier insertion.** Insert `llvm.x86.sse2.lfence` (or
    arm64 `dsb`/`isb` intrinsics) after every bounds check on
    secret-tainted indices. Toy Spectre-v1 mitigation.

## Suggested progression

Pick **(1)** first to confirm the toolchain handles `CallInst` walking and
debug-info reads. Then **(4)** for the first real IR-mutating pass — it
exercises `IRBuilder`, multi-exit insertion, and runtime-library linkage,
which most subsequent ideas reuse. After that, follow whichever branch
interests you: hardening (5–9), obfuscation (10–12), or integrity (13–14).

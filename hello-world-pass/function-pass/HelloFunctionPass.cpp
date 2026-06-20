//===- HelloFunctionPass.cpp - Count instructions, function scope ----------===//
//
// A hello-world LLVM new-PM function pass. Sums the instructions in the given
// function and prints the count. Pure observation: no IR is mutated.
//
// Build produces libHelloFunctionPass.dylib; load with:
//   opt -load-pass-plugin=libHelloFunctionPass.dylib -passes=hello-function ...
//
// Note: a function pass is invoked once per function in the module by the
// pass manager, so running -passes=hello-function over a module gives one
// line per defined function — no explicit iteration needed.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

struct HelloFunctionPass : PassInfoMixin<HelloFunctionPass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    unsigned count = 0;
    for (BasicBlock &BB : F)
      count += BB.size();
    errs() << "[hello-function] " << F.getName() << ": " << count
           << " instructions\n";
    return PreservedAnalyses::all();
  }

  // -O0 clang tags every function with `optnone`, which makes the FunctionPM
  // skip non-required passes. Mark this pass as required so it always runs.
  static bool isRequired() { return true; }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "HelloFunctionPass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "hello-function") {
                    FPM.addPass(HelloFunctionPass());
                    return true;
                  }
                  return false;
                });
          }};
}

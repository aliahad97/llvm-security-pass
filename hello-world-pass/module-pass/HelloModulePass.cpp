//===- HelloModulePass.cpp - Count instructions, module scope --------------===//
//
// A hello-world LLVM new-PM module pass. Walks every function in the module,
// sums the instructions in each, and prints per-function and module totals.
// Pure observation: no IR is mutated, so all analyses are preserved.
//
// Build produces libHelloModulePass.dylib; load with:
//   opt -load-pass-plugin=libHelloModulePass.dylib -passes=hello-module ...
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

struct HelloModulePass : PassInfoMixin<HelloModulePass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    unsigned moduleTotal = 0;
    for (Function &F : M) {
      // Skip pure declarations (no body to count).
      if (F.isDeclaration())
        continue;
      unsigned fnCount = 0;
      for (BasicBlock &BB : F)
        fnCount += BB.size();
      errs() << "[hello-module] " << F.getName() << ": " << fnCount
             << " instructions\n";
      moduleTotal += fnCount;
    }
    errs() << "[hello-module] module \"" << M.getName() << "\" total: "
           << moduleTotal << " instructions\n";
    return PreservedAnalyses::all();
  }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "HelloModulePass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "hello-module") {
                    MPM.addPass(HelloModulePass());
                    return true;
                  }
                  return false;
                });
          }};
}

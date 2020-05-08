// merge packing_* into this.
#include "packing.h"

using namespace std;
using namespace llvm;


PreservedAnalyses PackMemIntoReg::run(Module &M, ModuleAnalysisManager &FAM) {
  if (verifyModule(M, &errs(), nullptr))
    exit(1);
  
  return PreservedAnalyses::all();
}
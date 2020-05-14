// merge packing_* into this.
#include "packing.h"

using namespace std;
using namespace llvm;


PreservedAnalyses PackMemIntoReg::run(Module &M, ModuleAnalysisManager &FAM) {
  if (verifyModule(M, &errs(), nullptr))
    exit(1);
  LLVMContext* context = &M.getContext();
  vector<Packing*>* packingLst = Packing::getPacking(M,FAM,*context);
  for(auto &G : M.global_objects()){
    if(auto *F = dyn_cast<Function>(&G)){
      for(auto &BB : F){
        for(auto &I : BB){
          Packing::getOptimizedInsts(&I, *context, packingLst)==0)
        }
      }
    }
  }
  
  return PreservedAnalyses::all();
}
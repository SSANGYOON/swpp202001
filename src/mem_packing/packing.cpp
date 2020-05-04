// merge packing_* into this.
PreservedAnalyses PackMemIntoReg::run(Module &M, ModuleAnalysisManager &FAM) {
  if (verifyModule(M, &errs(), nullptr))
    exit(1);
  
  return PreservedAnalyses::all();
}
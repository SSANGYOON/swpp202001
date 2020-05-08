#ifndef TEAM13_PACKING_H
#define  TEAM13_PACKING_H

#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include <string>
#include <vector>

#include "llvm/IR/Value.h"

using namespace std;
using namespace llvm;

enum PackedPosition = {HIGHER,LOWER};

class Packing{
  llvm::Value *packingReg; //packing register after replacement for assignment
  llvm::Value *memValue;  //packed variable's Value instance.
  enum PackedPosition packedPos; //position in packing register
public:
  llvm::Value* getPackingReg(){return this->packingReg;}
  llvm::Value* getMemValue(){return this->memValue;}
  enum PackedPosition getPackedPosition(){return this->packedPos;}

  llvm::Value* setPackingReg(llvm::Value* v){
    this->packingReg = v;
    return this->packingReg;
  }
  llvm::Value* setMemValue(llvm::Value* v){
    this->memValue = v;
    return this->memValue;
  }
  enum PackedPosition setPackedPosition(enum PackedPosition pos){
    this->packedPos = pos;
    return this->packedPos;
  }

  static Packing* find(llvm::Value* val,vector<Packing*> &PackingLst);
  static vector<llvm::Instruction*>* getOptimizedInsts(llvm::LoadInst *loadInst);

};

class PackMemIntoReg : public llvm::PassInfoMixin<PackMemIntoReg> {
  std::string outputFile;
  bool printDepromotedModule;

public:
  PackMemIntoReg(std::string outputFile, bool printDepromotedModule) :
      outputFile(outputFile), printDepromotedModule(printDepromotedModule) {}
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &MAM);
};

#endif
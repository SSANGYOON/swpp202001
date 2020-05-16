#ifndef TEAM13_PACKING_H
#define  TEAM13_PACKING_H

#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include <utility>
#include <string>
#include <vector>

#include "llvm/IR/Value.h"

using namespace std;
using namespace llvm;

typedef int PackedPosition;
#define HIGHER 1
#define LOWER 0

class Replacement{
public:
  vector<Instruction*>* insts;
  LoadInst* loadInst;
  Replacement(vector<Instruction*>* insts, LoadInst* loadInst){
   this->insts = insts;
   this->loadInst = loadInst;
  }
};

class Packing{
  llvm::Value *packingReg; //packing register after replacement for assignment
  llvm::Value *memValue;  //packed variable's Value instance.
  PackedPosition packedPos; //position in packing register
public:
  llvm::Value* getPackingReg(){return this->packingReg;}
  llvm::Value* getMemValue(){return this->memValue;}
  PackedPosition getPackedPosition(){return this->packedPos;}

  llvm::Value* setPackingReg(llvm::Value* v){
    this->packingReg = v;
    return this->packingReg;
  }
  llvm::Value* setMemValue(llvm::Value* v){
    this->memValue = v;
    return this->memValue;
  }
  PackedPosition setPackedPosition(PackedPosition pos){
    this->packedPos = pos;
    return this->packedPos;
  }
  
  static Packing* find(llvm::Value* val,vector<Packing*> &PackingLst);
  static int getOptimizedInsts(llvm::LoadInst* loadInst, llvm::LLVMContext &context, vector<Packing*> &PackingLst,vector<Replacement*> *repLst);
  static vector<Value*>* find_ptr32(Function &F);
  static vector<Packing*>* getPacking(Function &F, FunctionAnalysisManager &FAM, llvm::LLVMContext &context,vector<Instruction*>* removeLst);
};

class PackMemIntoReg : public llvm::PassInfoMixin<PackMemIntoReg> {
  std::string outputFile;
  bool printDepromotedModule;

public:
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);
};
#endif

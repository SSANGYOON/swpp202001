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

typedef int PackedPosition;
#define HIGHER 1
#define LOWER 0

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
  int Packing::getOptimizedInsts(llvm::LoadInst* loadInst, llvm::LLVMContext &context, vector<Packing*> &PackingLst)
  static vector<Value*>* find_ptr32(Module &M);
  static vector<Packing*>* getPacking(Module &M, ModuleAnalysisManager &FAM)
};

class PackMemIntoReg : public llvm::PassInfoMixin<PackMemIntoReg> {
  std::string outputFile;
  bool printDepromotedModule;

public:
  PackMemIntoReg(std::string outputFile, bool printDepromotedModule) :
      outputFile(outputFile), printDepromotedModule(printDepromotedModule) {}
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &MAM){
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
};

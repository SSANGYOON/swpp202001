#include <vector>
#include <algorithm>
#include "packing.h"

using namespace std;
using namespace llvm;

/**
 * 
 * @brief find packing instance from packing list
 * @details find packing instance that has same memValue with val in PackingLst
 * @author 권혁진
 * @date 2020-05-05
 * 
 * @param llvm::Value* val original pointer 
 * that is not packed in original code.
 * @param std::vector<Packing*> &PackingLst list of Packing instance.
 * 
 * @return NULL cannot find val in PackingLst
 * @return Packing* searching succeed
 * 
*/
Packing* find(llvm::Value* val, vector<Packing*> &PackingLst){
  vector<Packing*>::iterator it = std::find_if(
      PackingLst.begin(),PackingLst.end(),
      [&] (Packing* packing) { return packing->getMemValue()==val; }
      );
  if(it==PackingLst.end())
    return NULL;
  return *it;
}

/**
 * 
 * @brief build and return optimized instructions.
 * @details build and return optimized instructions 
 * with which will replace load instruction.
 * @author 권혁진
 * @date 2020-05-05
 * 
 * @param llvm::LoadInst* loadInst load instruction to replace with optimized instructions.
 * @param llvm::LLVMContext context context value.
 * 
 * @return vector<llvm::Instruction*>*
 * @return NULL It is not packed value on memory.
 * 
*/
vector<llvm::Instruction*>* Packing::getOptimizedInsts(llvm::LoadInst* loadInst, llvm::LLVMContext context){
  Value* pointerValue = loadInst->getPointerOperand();
  
  // find packing instance from packing list
  Packing* packPtr = Packing::find(pointerValue);

  // if it is not packed value
  if(!packPtr){
    return NULL;
  }

  auto loadName = loadInst->getName();
  auto baseTwine = new Twine(loadName);
  uint64_t shrNum;
  uint64_t mask=4294967296; // 4294967296 = 2^32

  switch(packPtr->getPackedPosition()){
    case HIGHER:
      shrNum = 32;
      break;
    case LOWER:
      shrNum = 0;
      break;
  }

  auto lshrOp = llvm::BinaryOperator::CreateLShr(
    packPtr->getPackingReg(),
    ConstantInt::get(llvm::IntegerType::getInt64Ty(context),shrNum),
    baseTwine->concat("_tmp1"))
  );
  auto uremOp = llvm::BinaryOperator::CreateURem(
    dyn_cast<Value>(lshrOp),
    ConstantInt::get(llvm::IntegerType::getInt64Ty(context),mask),
    baseTwine->concat("_tmp2")
  );
  auto truncOp = llvm::TruncInst(
    dyn_cast<Value>(uremOp),
    llvm::IntegerType::getInt32Ty(context),
    *baseTwine
  );

  vector<Value*>* optimizedInst = new vector();
  optimizedInst->push_back(lshrOp);
  optimizedInst->push_back(uremOp);
  optimizedInst->push_back(truncOp);
  

  return optimizedInst;
}
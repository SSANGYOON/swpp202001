#include <vector>
#include <algorithm>
#include "packing.h"

using namespace std;
using namespace llvm;

/**
 * 
 * @brief find packable i32* registers candidate
 * @details find all i32* registers which is not used for ret, getelementptr or multiply stored
 * @author 이상윤
 * @date 2020-05-13
 * 
 * @param llvm::Function &F our optimizing function
 * 
 * @return return all packable or seemingly packable i32* value
 * 
*/
vector<Value*>* Packing::find_ptr32(Function &F){
  vector<Value*>* candidate = new vector<Value*>();
  //loop for block
  for(auto &BB  : F){
    //loop for instruction
    for(auto &I : BB){
      Value* p;
      if(auto a = dyn_cast<AllocaInst>(&I)){
        p = dyn_cast<Value>(a);
      }
      else if(auto gep = dyn_cast<GetElementPtrInst>(&I)){
        p = dyn_cast<Value>(gep);
      }
      else{
        continue;
      }
      //check if p is i32*
      llvm::Type* t = p->getType();
      llvm::Type* inner = t->getPointerElementType();
      if (inner->isIntegerTy()) {
        llvm::IntegerType* it = (llvm::IntegerType*) inner;
        if (it->getBitWidth() == 32) {
          int store_num=0;
          bool not_ret=true;
          bool not_gep=true;
          for(auto itr=p->use_begin();itr!=p->use_end();itr++){
            Use &U = *itr;
            User *Usr = U.getUser();
            Instruction *UsrI = dyn_cast<Instruction>(Usr);
            if(UsrI){
              //check if p is multiply stored
              if(dyn_cast<StoreInst>(UsrI))
                store_num+=1;
              //check if p is used for getelementptr
              else if(dyn_cast<GetElementPtrInst>(UsrI)){
                not_gep=false;
                break;
              }
              //check if p is used for ret
              else if(dyn_cast<ReturnInst>(UsrI)){
                not_ret=false;
                break;
              }
            }
          }
          //if p is seemingly packable according to previous step then push it to vector to return
          if(store_num==1&&not_ret&&not_gep){
            candidate->push_back(p);
          }
        }
      }
    }
  }
  return candidate;
}
/**
 * 
 * @brief insert optimized store instruction and return vector containing all Packing instances for each pointer
 * @details not deleted alloca and store instruction for optimizing load instruction 
 * @author 이상윤
 * @date 2020-05-13
 * 
 * @param llvm::Function &F our optimizing function
 * @param llvm::FunctionAnalysisManager &FAM
 * @param llvm::LLVMContext context our Function context
 * @param vector<Value*>* candidate returned vector from vector<Value*>* find_ptr32(Function &M)
 * 
 * @return vector containing all Packing instances for each pointer
 * 
*/

vector<Packing*>* Packing::getPacking(Function &F, FunctionAnalysisManager &FAM,llvm::LLVMContext &context,vector<Instruction*>* removeLst){
  vector<Value*>* candidate=Packing::find_ptr32(F);
  vector<Packing*>* packs = new vector<Packing*>();
  DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);
  //compare every i32* pointers in candidate 
  //if two pointers are packable insert optimized instruction insert optimized instruction and assign Packing* instance
  //if ptr is already packed, don't pack twice
  for(auto itr1 = candidate->begin(); itr1<candidate->end();itr1++){
    bool itr1_packed=false;
    //check if ptr1 is already packed 
    if(find_if(packs->begin(),packs->end(),
    [&] (Packing* packing) { return packing->getMemValue()==(*itr1);}
    )!=packs->end()
    )
    {
      continue;
    }
    for(auto itr2 = itr1 + 1; itr2 < candidate->end(); itr2++){
      //check if ptr1 is already packed 
      if(find_if(packs->begin(),packs->end(),
      [&] (Packing* packing) { return packing->getMemValue()==(*itr2);}
      )!=packs->end()
      )
      {
        continue;
      }
      auto ptr1=*itr1;
      auto ptr2=*itr2;
      
      auto ptrI1=dyn_cast<Instruction>(ptr1);
      auto ptrI2=dyn_cast<Instruction>(ptr2);
      //determine what is allocated first. set ptr1 as previous one
      if(DT.dominates(ptrI2,ptrI2)){
        auto temp=ptr1;
        ptr1=ptr2;
        ptr2=temp; 
        auto tempI=ptrI1;
        ptrI1=ptrI2;
        ptrI2=tempI;
      }
      else if(DT.dominates(ptrI1,ptrI2)){
      }
      //not pack these two registers(not on same block)
      else{
        continue;
      }
      //don't pack ptr1 is stored before ptr2 is allocated
      bool not_store_before_2=true;
      for(auto itr=ptr1->use_begin();itr!=ptr1->use_end();itr++){
        Use &U = *itr;
        User *Usr = U.getUser();
        Instruction *UsrI = dyn_cast<Instruction>(Usr);
        if(dyn_cast<StoreInst>(UsrI)){
          if(DT.dominates(UsrI,ptrI2)){
            not_store_before_2=false;
            break;
          }
        }
      }
      if(not_store_before_2){
        StoreInst* s1;
        StoreInst* s2;
        for(auto itr=ptr1->use_begin();itr!=ptr1->use_end();itr++){
          Use &U = *itr;
          User *Usr = U.getUser();
          Instruction *UsrI = dyn_cast<Instruction>(Usr);
          if(dyn_cast<StoreInst>(UsrI)){
            s1=dyn_cast<StoreInst>(UsrI);
            break;
          }
        }
        for(auto itr=ptr2->use_begin();itr!=ptr2->use_end();itr++){
          Use &U = *itr;
          User *Usr = U.getUser();
          Instruction *UsrI = dyn_cast<Instruction>(Usr);
          if(dyn_cast<StoreInst>(UsrI)){
            s2=dyn_cast<StoreInst>(UsrI);
          
            break;
          }
        }
        auto baseTwine1 = new Twine(ptr1->getName());
        auto baseTwine2 = new Twine(ptr2->getName());
        Value* v1 = s1->getValueOperand();
        Value* v2 = s2->getValueOperand();
        Value* PackingReg;
        if(dyn_cast<ConstantInt>(v1)&&dyn_cast<ConstantInt>(v2)){
          auto *C1 = dyn_cast<ConstantInt>(v1);
          auto *C2 = dyn_cast<ConstantInt>(v2);
          auto zextOp1 = ConstantInt::get(llvm::IntegerType::getInt64Ty(context),C1->getZExtValue());
          auto zextOp2 = ConstantInt::get(llvm::IntegerType::getInt64Ty(context),C2->getZExtValue());
          auto shl = llvm::BinaryOperator::CreateShl(zextOp1, ConstantInt::get(llvm::IntegerType::getInt64Ty(context),32),
          baseTwine2->concat("_shl")
          );
          auto addOp = llvm::BinaryOperator::CreateAdd(dyn_cast<Value>(zextOp2),dyn_cast<Value>(shl),baseTwine1->concat("_").concat(ptr2->getName()).concat("_fit"));
          PackingReg=dyn_cast<Value>(addOp);
          shl->insertAfter(s2);
          addOp->insertAfter(shl);
        }
        else if(dyn_cast<ConstantInt>(v2)&&!dyn_cast<ConstantInt>(v1)){
          auto zextOp1 = llvm::CastInst::CreateZExtOrBitCast(s1->getValueOperand(), llvm::IntegerType::getInt64Ty(context),
          baseTwine1->concat("_zext")
          );
          auto *C2 = dyn_cast<ConstantInt>(v2);
          auto zextOp2 = ConstantInt::get(llvm::IntegerType::getInt64Ty(context),C2->getZExtValue());
          auto shl = llvm::BinaryOperator::CreateShl(dyn_cast<Value>(zextOp1), ConstantInt::get(llvm::IntegerType::getInt64Ty(context),32),
          baseTwine2->concat("_shl")
          );
          auto addOp = llvm::BinaryOperator::CreateAdd(dyn_cast<Value>(zextOp2),dyn_cast<Value>(shl),baseTwine1->concat("_").concat(ptr2->getName()).concat("_fit"));
          PackingReg=dyn_cast<Value>(addOp);
          zextOp1->insertAfter(s2);
          shl->insertAfter(zextOp1);
          addOp->insertAfter(shl);
        }
        else if(dyn_cast<ConstantInt>(v1)&&!dyn_cast<ConstantInt>(v2)){
          auto *C1 = dyn_cast<ConstantInt>(v1);
          auto zextOp1 = ConstantInt::get(llvm::IntegerType::getInt64Ty(context),C1->getZExtValue());
          auto zextOp2 = llvm::CastInst::CreateZExtOrBitCast(s2->getValueOperand(), llvm::IntegerType::getInt64Ty(context),
          baseTwine2->concat("_zext")
          );
          auto shl = llvm::BinaryOperator::CreateShl(zextOp1, dyn_cast<Value>(ConstantInt::get(llvm::IntegerType::getInt64Ty(context),32)),
          baseTwine2->concat("_shl")
          );
          auto addOp = llvm::BinaryOperator::CreateAdd(dyn_cast<Value>(zextOp2),dyn_cast<Value>(shl),baseTwine1->concat("_").concat(ptr2->getName()).concat("_fit"));
          PackingReg=dyn_cast<Value>(addOp);
          zextOp2->insertAfter(s2);
          shl->insertAfter(zextOp2);
          addOp->insertAfter(shl);
        }
        else{
          
          auto zextOp1 = llvm::CastInst::CreateZExtOrBitCast(s1->getValueOperand(), llvm::IntegerType::getInt64Ty(context),
          baseTwine2->concat("_zext")
          );
          auto zextOp2 = llvm::CastInst::CreateZExtOrBitCast(s2->getValueOperand(), llvm::IntegerType::getInt64Ty(context),
          baseTwine2->concat("_zext")
          );
          
          auto shl = llvm::BinaryOperator::CreateShl(dyn_cast<Value>(zextOp1), dyn_cast<Value>(ConstantInt::get(llvm::IntegerType::getInt64Ty(context),32)),
          baseTwine2->concat("_shl")
          );
          
          auto addOp = llvm::BinaryOperator::CreateAdd(dyn_cast<Value>(zextOp2),dyn_cast<Value>(shl),baseTwine1->concat("_").concat(ptr2->getName()).concat("_fit"));
          PackingReg=dyn_cast<Value>(addOp);
          zextOp1->insertAfter(s2);
          zextOp2->insertAfter(zextOp1);
          shl->insertAfter(zextOp2);
          addOp->insertAfter(shl);
          
        }
        Packing *firstPack = new Packing();
        firstPack->setMemValue(s1->getPointerOperand());
        removeLst->push_back(dyn_cast<Instruction>(s1));
        removeLst->push_back(dyn_cast<Instruction>(s1->getPointerOperand()));

        firstPack->setPackedPosition(HIGHER);
        firstPack->setPackingReg(PackingReg);
        Packing *secondPack = new Packing();
        secondPack->setMemValue(s2->getPointerOperand());
        removeLst->push_back(dyn_cast<Instruction>(s2));
        removeLst->push_back(dyn_cast<Instruction>(s2->getPointerOperand()));
        secondPack->setPackedPosition(LOWER);
        secondPack->setPackingReg(PackingReg);
       
        packs->push_back(firstPack);
        packs->push_back(secondPack);
        
        break;
      }
      else{
        continue;
      }
    }
  }
  return packs;
}


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
Packing* a_find(llvm::Value* val, vector<Packing*> &PackingLst){
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
 * @return 0, success
 * @return -1 NULL It is not packed value on memory.
 * 
*/
int Packing::getOptimizedInsts(llvm::LoadInst* loadInst, llvm::LLVMContext& context, vector<Packing*> &PackingLst,vector<Replacement*> *repLst){

    if(loadInst==nullptr)
        return -1;
  Value* pointerValue = loadInst->getPointerOperand();
  
  // find packing instance from packing list
  Packing* packPtr = a_find(pointerValue,PackingLst);

  // if it is not packed value
  if(!packPtr){
    return -1;
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
    baseTwine->concat("_tmp1")
  );
  auto uremOp = llvm::BinaryOperator::CreateURem(
    dyn_cast<Value>(lshrOp),
    ConstantInt::get(llvm::IntegerType::getInt64Ty(context),mask),
    baseTwine->concat("_tmp2")
  );
  auto truncOp = new llvm::TruncInst(
    dyn_cast<Value>(uremOp),
    llvm::IntegerType::getInt32Ty(context),
    *baseTwine
  );

  vector<Instruction*> *insts = new vector<Instruction*>();
  insts->push_back(dyn_cast<Instruction>(lshrOp));
  insts->push_back(dyn_cast<Instruction>(uremOp));
  insts->push_back(dyn_cast<Instruction>(truncOp));
  //lshrOp->insertBefore(loadInst);
  //uremOp->insertBefore(loadInst);
  //truncOp->insertBefore(loadInst);
  Replacement* rep = new Replacement(insts,loadInst);
  repLst->push_back(rep);
  //loadInst->replaceAllUsesWith(truncOp);
  //loadInst->eraseFromParent();

  return 0;
}

PreservedAnalyses PackMemIntoReg::run(Function &F, FunctionAnalysisManager &FAM) {
    LLVMContext* context = &F.getContext();
    vector<Instruction*>* removeLst = new vector<Instruction*>();
    vector<Packing*>* packingLst = Packing::getPacking(F,FAM,*context,removeLst);
    vector<Replacement*> *repLst = new vector<Replacement*>();
    for(auto &BB : F){
      for(auto &I : BB){
        Packing::getOptimizedInsts(dyn_cast<LoadInst>(&I), *context, *packingLst,repLst);
      }
    }    
    for(auto &rep : *repLst){
      Instruction* lshrOp = (*(rep->insts))[0];
      Instruction* uremOp = (*(rep->insts))[1];
      Instruction* truncOp = (*(rep->insts))[2];
      Instruction* loadInst = rep->loadInst;

      lshrOp->insertBefore(loadInst);
      uremOp->insertBefore(loadInst);
      truncOp->insertBefore(loadInst);
      loadInst->replaceAllUsesWith(truncOp);
      loadInst->eraseFromParent();
    }
    for(auto *remove : *removeLst){
      remove->eraseFromParent();
    }
    return PreservedAnalyses::all();
}

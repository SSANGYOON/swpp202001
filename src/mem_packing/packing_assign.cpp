#include <vector>
#include <algorithm>
#include "packing.h"
#include
using namespace std;
using namespace llvm;
/**
 * 
 * @brief find packable i32* registers candidate
 * @details find all i32* registers which is not used for ret, getelementptr or multiply stored
 * @author 이상윤
 * @date 2020-05-13
 * 
 * @param llvm::Module &M our optimizing module
 * 
 * @return return all packable or seemingly packable i32* value
 * 
*/
vector<Value*>* Packing::find_ptr32(Module &M){
  vector<Value*>* candidate = new vector<Value*>();
  // loop for function
  for(auto F : M){
    //loop for block
    for(auto BB  : F){
      //loop for instruction
      for(auto I : B){
        Value p;
        if(auto a = dyn_cast<AllocaInst>(I)){
          p = dyn_cast<Value>(a);
        }
        else if(auto gep = dyn_cast<GetElementPtrInst>(I)){
          p = dyn_cast<Value>(a);
        }
        //check if p is i32*
        llvm::Type* t = p.getType();
        llvm::Type* inner = t->getPointerElementType();
        if (inner->isIntegerTy()) {
          llvm::IntegerType* it = (llvm::IntegerType*) inner;
          if (it->getBitWidth() == 32) {
            int store_num=0;
            bool not_ret=true;
            bool not_gep=true;
            for(auto itr=p.use_begin();itr<p.use_end();itr++){
              Use &U = *itr;
              User *Usr = U.getUser();
              Instruction *UsrI = dyn_cast<Instruction>(Usr);
              if(UsrI){
                //check if p is multiply stored
                if(dyn_cast<StoreInst>(UsrI))
                  store+=1;
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
              candidate->push_back(&p);
            }
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
 * @param llvm::Module &M our optimizing module
 * @param llvm::ModuleAnalysisManager &FAM
 * @param llvm::LLVMContext context our Module context
 * @param vector<Value*>* candidate returned vector from vector<Value*>* find_ptr32(Module &M)
 * 
 * @return vector containing all Packing instances for each pointer
 * 
*/
vector<Packing*>* Packing::getPacking(Module &M, ModuleAnalysisManager &FAM,llvm::LLVMContext &context){
  vector<Value*>* candidate=Packing::find_ptr32(M);
  vector<Packing*>* packs = new vector<Packing*>();
  DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(M);
  //compare every i32* pointers in candidate 
  //if two pointers are packable insert optimized instruction insert optimized instruction and assign Packing* instance
  //if ptr is already packed, don't pack twice
  for(auto itr1 = candidate->begin(); itr1!=candidate->end():itr1++){
    bool itr1_packed=false;
    //check if ptr1 is already packed 
    if(find_if(packs->begin(),packs->end(),
    [&] (Packing* packing) { return packing->getMemValue()==&(*itr1);}
    )
    )
    {
      continue;
    }
    for(auto itr2 = itr1 + 1; itr2 != candidate->end(); itr2++){
      //check if ptr1 is already packed 
      if(find_if(packs->begin(),packs->end(),
      [&] (Packing* packing) { return packing->getMemValue()==&(*itr2);}
      )
      )
      {
        continue;
      }
      auto ptr1=*itr1;
      auto ptr2=*itr2;
      auto ptrI1=dyn_cast<Instruction>(ptr1);
      auto ptrI2=dyn_cast<Instruction>(ptr2);
      //determine what is allocated first. set ptr1 as previous one
      if(ptrI1->getParent()->getParent()==ptrI2->getParent()->getParent()){
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
      }
      //don't pack ptr1 is stored before ptr2 is allocated
      bool not_store_before_2=true;
      for(auto itr=ptr1->use_begin();itr<ptr1->use_end();itr++){
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
        for(auto itr=ptr1->use_begin();itr<ptr1->use_end();itr++){
          Use &U = *itr;
          User *Usr = U.getUser();
          Instruction *UsrI = dyn_cast<Instruction>(Usr);
          if(dyn_cast<StoreInst>(UsrI)){
            s1=dyn_cast<StoreInst>(UsrI);
            break;
          }
        }
        for(auto itr=ptr2->use_begin();itr<ptr2->use_end();itr++){
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
      
        auto zextOp1 = llvm::ZExtInst(s1->getValueOperand(), llvm::IntegerType::getInt64Ty(context),
        baseTwine1->concat("_zext")
        );
        auto zextOp2 = llvm::ZExtInst(s2->getValueOperand(), llvm::IntegerType::getInt64Ty(context),
        baseTwine2->concat("_zext")
        );
        auto shl = llvm::BinaryOperator::CreateShl(dyn_cast<Value>(&zextOp1), ConstantInt::get(llvm::IntegerType::getInt64Ty(context),32),
        baseTwine2->concat("_shl")
        );
        auto addOp = llvm::BinaryOperator::CreateAdd(dyn_cast<Value>(&zextOp2),dyn_cast<Value>(shl),baseTwine1->concat("_").concat(ptr2->getName()).concat("_fit"));
        auto PackingReg=dyn_cast<Value>(addOp);
        Packing firstPack = new Packing();
        firstPack.setMemValue(s1->getValueOperand());
        firstPack.setPackedPosition(HIGHER);
        firstPack.setPackingReg(PackingReg);
        Packing secondPack = new Packing();
        secondPack.setMemValue(s2->getValueOperand());
        secondPack.setPackedPosition(LOWER);
        secondPack.setPackingReg(PackingReg);
        s2->getParent()->getInstList().push_back(zextOp1);
        zextOp1->getParent()->getInstList().push_back(zextOp2);
        zextOp2->getParent()->getInstList().push_back(shl);
        shl->getParent()->getInstList().push_back(addOp);
        packs->push_back(&firstPack);
        packs->push_back(&secondPack);
        break;
      }
      else{
        continue;
      }
    }
  }
}

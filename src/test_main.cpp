#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/SourceMgr.h"
#include "gtest/gtest.h"
#include "SimpleBackend.h"

#include "packing.h"

using namespace llvm;
using namespace std;

class MyAliasAnalysisTest : public testing::Test {
public:
  // Parses LLVM IR Assembly & returns a created module.
  // Note that the result is contained in unique_ptr; It is one of C++'s smart
  // pointer classes. if the unique_ptr goes out of a scope without being moved
  // to somewhere else, its content is deleted.
  // See also: https://thispointer.com/c11-unique_ptr-tutorial-and-examples/
  unique_ptr<Module> parseModule(StringRef Assembly) {
    SMDiagnostic Error;
    unique_ptr<Module> M = parseAssemblyString(Assembly, Error, Context);

    string errMsg;
    raw_string_ostream os(errMsg);
    Error.print("", os);
    EXPECT_TRUE(M) << os.str();

    return M;
  }

protected:
  // LLVMContext object maintains all other objects that are created during
  // parsing or optimizing programs. For example, when you created a new i16
  // type instance, it is registered at this Context instance.
  LLVMContext Context;
  // The LLVM IR module that is parsed.
  unique_ptr<Module> M;
  // @test() function inside M.
  Function *testF;
  // A mapping from register names to instructions inside @test().
  map<string, Instruction *> Instrs;

  // Parse the IR assembly by calling parseModule function.
  void parseAssembly(StringRef Assembly) {
    M = parseModule(Assembly);
    ASSERT_TRUE(M);

    Function *F = M->getFunction("main");
    ASSERT_TRUE(F) << "Test must have a function @test";
    if (!F)
      return;

    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
      if (I->hasName())
        Instrs[I->getName().str()] = &*I;
    }
    testF = F;
  }

  // Create an empty function & store it into testF.
  void createEmptyTestFunction() {
    M = unique_ptr<Module>(new Module("MyModule", Context));
    FunctionType *FTy =
      FunctionType::get(Type::getVoidTy(Context), {}, false);
    testF = Function::Create(FTy, Function::ExternalLinkage, "test", *M);
  }


  // Run basic alias analysis on two values A and B, and return its result.
  AliasResult getAliasResult(const Value *A, const Value *B,
                             BasicAA::Result &Res) {
    AAQueryInfo EmptyInfo;
    return Res.alias(MemoryLocation(A, 4), // 4-bytes access..!
                     MemoryLocation(B, 4), // 4-bytes access..!
                     EmptyInfo);
  }

  // Run basic alias analysis on two registers %a and %b, and return its result.
  AliasResult getAliasResult(const string &a, const string &b,
                             BasicAA::Result &Res) {
    AAQueryInfo EmptyInfo;
    EXPECT_TRUE(Instrs.count(a) != 0) << "There is no register " << a;
    EXPECT_TRUE(Instrs.count(b) != 0) << "There is no register " << b;
    return Res.alias(MemoryLocation(Instrs[a], 4), // 4-bytes access..!
                     MemoryLocation(Instrs[b], 4), // 4-bytes access..!
                     EmptyInfo);
  }
};

TEST(TestDemo, CheckMain) {
  // Show that the assembler correctly emits 'start main 0' as well as 'end main'
  LLVMContext Context;
  unique_ptr<Module> M(new Module("MyTestModule", Context));
  auto *I64Ty = Type::getInt64Ty(Context);
  auto *TestFTy = FunctionType::get(I64Ty, {}, false);
  Function *TestF = Function::Create(TestFTy, Function::ExternalLinkage,
                                     "main", *M);

  BasicBlock *Entry = BasicBlock::Create(Context, "entry", TestF);
  IRBuilder<> EntryBuilder(Entry);
  EntryBuilder.CreateRet(ConstantInt::get(I64Ty, 0));

  string str;
  raw_string_ostream os(str);
  AssemblyEmitter(&os).run(M.get());

  str = os.str();
  // These strings should exist in the assembly!
  EXPECT_NE(str.find("start main 0:"), string::npos);
  EXPECT_NE(str.find("end main"), string::npos);
}



TEST_F(MyAliasAnalysisTest, TestUsingParseAssembly) {
  // Let's parse this IR assembly, and create mappings from register names
  // to instruction objects!
  auto M = parseModule(R"myasm(
    define void @main() {
        %p = alloca i32
        %q = alloca i32
    
        store i32 15, i32* %p
        store i32 16, i32* %q
    
        %_P = load i32, i32* %p
        %_Q = load i32, i32* %q
    
        %r = add i32 %_P, %_Q
        ret void
    }
  )myasm");

  // Mappings created!
  outs() << "----- PackingRegTest.TestUsingParseAssembly -----" << "\n";

  // Let's build a passbuilder & get alias analysis result..

  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;
  PassBuilder PB;
  // Register all the basic analyses with the managers.
  

  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);


  FunctionPassManager FPM;
  // If you want to add a function-level pass, add FPM.addPass(MyPass()) here.
  FPM.addPass(PackMemIntoReg());
  ModulePassManager MPM;
  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
  // If you want to add your module-level pass, add MPM.addPass(MyPass2()) here.

  // Run!
  MPM.run(*M, MAM);

  string str;
  raw_string_ostream os(str);

  M->print(os,nullptr);

  str = os.str();
  outs() << str <<"\n";
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

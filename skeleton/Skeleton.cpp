#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
using namespace llvm;

namespace {
  struct SkeletonPass : public FunctionPass {
    static char ID;
    SkeletonPass() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &F) {
      // Não deve injetar no main
      if(F.getName() == "main")
        return false;

      errs() << "Injecting function " << F.getName() << "!\n";
      for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
        // Imprime cada instrução (para debug)
        errs() << *I << "\n";
        if(I->getOpcode() == Instruction::Ret) {
            ReturnInst* ret = dyn_cast<ReturnInst>(&*I);
            IRBuilder<> builder(ret);

            // Inverte o último bit do valor de retorno da função.
            Value* retVal = ret->getReturnValue();
            Value* error = builder.CreateXor(retVal, 0x00000001);
            
            // Agora o return vai retornar nosso valor errôneo e não mais o valor original
            ret->setOperand(0, error);
            errs() << "Found return point! and flipped it!" << "\n";

            // Modificamos a função
            return true;
        }
      }
      return false;
    }
  };
}

char SkeletonPass::ID = 0;

// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerSkeletonPass(const PassManagerBuilder &,
                         legacy::PassManagerBase &PM) {
  PM.add(new SkeletonPass());
}
static RegisterStandardPasses
  RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                 registerSkeletonPass);

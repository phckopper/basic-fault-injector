#include <cstdint>

#include "llvm/Pass.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
using namespace llvm;

namespace {
  struct SkeletonPass : public FunctionPass {
    static char ID;
    SkeletonPass() : FunctionPass(ID) {}
    
    void injectInstruction(Instruction *thisInst, Value *addrToInject) {
            errs() << "Injecting!!!" << "\n";
            errs() << *thisInst << "\n";
            
            std::uintptr_t address = reinterpret_cast<std::uintptr_t>(thisInst);
            Instruction *nextInst = thisInst->getNextNode();
            auto constAddress = ConstantInt::get(Type::getInt64Ty(thisInst->getContext()), address, false);

            IRBuilder<> builder(nextInst);

            Value* cmp = builder.CreateICmpEQ(constAddress, addrToInject);
            Instruction *ThenTerm , *ElseTerm;
            SplitBlockAndInsertIfThenElse(cmp, nextInst, &ThenTerm, &ElseTerm, nullptr);
            builder.SetInsertPoint(ThenTerm);
            Value* error = builder.CreateXor(thisInst, 0x00000001);

            builder.SetInsertPoint(nextInst);

            PHINode* phi = builder.CreatePHI(Type::getInt32Ty(thisInst->getContext()), 2, "injection");
            phi->addIncoming(error, ThenTerm->getParent());
            
            thisInst->replaceUsesOutsideBlock(phi, ThenTerm->getParent());
            phi->addIncoming(thisInst, ElseTerm->getParent());

    }

    virtual bool runOnFunction(Function &F) {
      if(F.getName() == "main")
          return false;

      LLVMContext& Ctx = F.getContext();
        FunctionCallee shouldInject = F.getParent()->getOrInsertFunction(
        "shouldInject", Type::getInt64Ty(Ctx)
      );
        

      errs() << "Injecting function " << F.getName() << "!\n";
      std::vector<Instruction *> toInject;

      IRBuilder<> builder(F.getEntryBlock().getFirstNonPHIOrDbgOrLifetime());
      Value *addrToInject = builder.CreateCall(shouldInject);

      for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
        if(I->getType() == Type::getInt32Ty(Ctx)) {
            toInject.push_back(&*I);
        }
      }

      for(Instruction *I: toInject) {
        // Imprime cada instrução (para debug)
        std::uintptr_t address = reinterpret_cast<std::uintptr_t>(I);
        errs() << "Address " << address << ": ";
        errs() << I << "\n";
        injectInstruction(I, addrToInject);
        // Modificamos a função
      }
        
      return true;
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

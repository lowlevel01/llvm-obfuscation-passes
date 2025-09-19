#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;

namespace {

struct MBA_Add: public PassInfoMixin<MBA_Add> {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
        SmallVector<BinaryOperator*, 16> Adds;
        SmallVector<BinaryOperator*, 16> Subs;
        for (auto &F : M) {
          if (F.isDeclaration()) continue;
          for (auto &B : F){
            for (auto &I : B){
              if(auto *op = dyn_cast<BinaryOperator>(&I) ){
                  if(op->getOpcode() == Instruction::Add && op->getType()->isIntegerTy()){
                    errs() << "Found an add with integer type\n";
                    Adds.push_back(op); 
                  }
                  
                  if(op->getOpcode() == Instruction::Sub && op->getType()->isIntegerTy()){
                    errs() << "Found a Sub with integer type" << "\n";
                    Subs.push_back(op);
                  }
                }
              }
            }
          }
       

        for (BinaryOperator *SubInst: Subs){
          errs() << "Replacing Sub" << *SubInst << "\n";
          
          IRBuilder<> Builder(SubInst);
          
          Value *lhs = SubInst->getOperand(0);
          Value *rhs = SubInst->getOperand(1);

          Value *neg_rhs = Builder.CreateNeg(rhs);
          Value *new_add = Builder.CreateAdd(lhs, neg_rhs);
          
          SubInst->replaceAllUsesWith(new_add);
          SubInst->eraseFromParent();


          if(auto *new_add_inst = dyn_cast<BinaryOperator>(new_add)){
            Adds.push_back(new_add_inst);
          }
        }
        for (BinaryOperator *AddInst : Adds) {
            errs() << "From Small vector:" << *AddInst << "\n";
            
            IRBuilder<> Builder(AddInst);
            Value *lhs = AddInst->getOperand(0);
            Value *rhs = AddInst->getOperand(1);
            Type *T = AddInst->getType();

            Value *xor_int = Builder.CreateXor(lhs, rhs);
            Value *and_int = Builder.CreateAnd(lhs, rhs);
            Value *shl_int = Builder.CreateShl(and_int, ConstantInt::get(T,1));
            Value *new_add = Builder.CreateAdd(xor_int, shl_int);
            
            AddInst->replaceAllUsesWith(new_add);
            AddInst->eraseFromParent();
          
        }      

        return PreservedAnalyses::none();
    };
};

}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "MBA Pass : Add",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    MPM.addPass(MBA_Add());
                });
        }
    };
}

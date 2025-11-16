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

#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <vector>
#include <map>

#define FUNC_MIN_BLOCK 3

using namespace llvm;

namespace {


struct ControlFlowFlattening : public PassInfoMixin<ControlFlowFlattening> {

  bool flatten(Function &F){
    if(F.size() < FUNC_MIN_BLOCK){
      errs() << "SKIPPED: Function " << F.getName() << " has less than" << FUNC_MIN_BLOCK <<" blocks.\n";
      return false;
    }

    // skipping function definitions with no body
    if(F.isDeclaration())
      return false;

    errs() << "Flattening the function: " << F.getName() << "\n";


    std::vector<BasicBlock*> orginalBlocks;

    // fetch the blocks and skip the first one
    BasicBlock &entryBB = F.getEntryBlock();
    for(BasicBlock &BB : F){
      if(&BB != &entryBB){
        orginalBlocks.push_back(&BB);
      }
    }

    if(orginalBlocks.empty())
      return false;

    // dispatcher block
    BasicBlock *dispatcherBB = BasicBlock::Create(F.getContext(), "dispatcher", &F);

    //creating a default basic block as a fallback, reaching it means the CFF logic is broken
    BasicBlock *defaultBB = BasicBlock::Create(F.getContext(), "dafault", &F);
    IRBuilder<> DefaultBuilder(defaultBB);
    DefaultBuilder.CreateUnreachable();

    // allocating the switch state counter on the she stack in the entryblock 
    IRBuilder<> EntryBuilder(&entryBB, entryBB.getFirstInsertionPt()); // the stack allocation should be in the beginning of the basic block
    AllocaInst *switchVar = EntryBuilder.CreateAlloca(
      Type::getInt32Ty(F.getContext()), nullptr, "switchVar" // nullptr means a single element not an array
    );

    // initialzing the switch counter to 0 pointing to the first block
    EntryBuilder.CreateStore(ConstantInt::get(Type::getInt32Ty(F.getContext()),0), switchVar);

    // entryblock should fall to the dispatch block
    Instruction *entryTerm = entryBB.getTerminator();
    if(entryTerm){
      IRBuilder<> Builder(entryTerm);
      Builder.CreateBr(dispatcherBB);
      entryTerm->eraseFromParent();

    }

    // create the switch statmenet in the dispatcher block
    IRBuilder<> DispatcherBuilder(dispatcherBB);
    LoadInst* switchVal = DispatcherBuilder.CreateLoad(
                          Type::getInt32Ty(F.getContext()),switchVar, "switchVal" );
    SwitchInst* switchInst = DispatcherBuilder.CreateSwitch(switchVal, defaultBB, orginalBlocks.size());

    // map every block to a switch counter value
    std::map<BasicBlock*, uint32_t> blockToCase;
    for(size_t i = 0; i < orginalBlocks.size(); i++){
      blockToCase[orginalBlocks[i]] = i;
      switchInst->addCase(ConstantInt::get(Type::getInt32Ty(F.getContext()), i), orginalBlocks[i]);
    }

    // modify the terminators of the original blocks
    for(BasicBlock* BB : orginalBlocks){
      Instruction* terminator = BB->getTerminator();
      if(!terminator)
        continue;

      IRBuilder<> Builder(terminator);

      if(auto *br = dyn_cast<BranchInst>(terminator)){

        if(br->isUnconditional()){
          // unconditional branch

          BasicBlock* successor = br->getSuccessor(0);

          if(successor == &entryBB) continue;

          auto it = blockToCase.find(successor);
          if( it != blockToCase.end()){
            Builder.CreateStore(ConstantInt::get(Type::getInt32Ty(F.getContext()), it->second), switchVar);
            Builder.CreateBr(dispatcherBB);
            terminator->eraseFromParent();
          }
        }else{
          //contidional branch
          Value* condition = br->getCondition();

          BasicBlock* trueSucc = br->getSuccessor(0);
          BasicBlock* falseSucc = br->getSuccessor(1);

          auto trueIt = blockToCase.find(trueSucc);
          auto falseIt = blockToCase.find(falseSucc);

          if(trueIt != blockToCase.end() && falseIt != blockToCase.end()){
            Value* caseVal = Builder.CreateSelect(
              condition,
              ConstantInt::get(Type::getInt32Ty(F.getContext()), trueIt->second),
              ConstantInt::get(Type::getInt32Ty(F.getContext()), falseIt->second)
            );

            Builder.CreateStore(caseVal, switchVar);
            Builder.CreateBr(dispatcherBB);
            terminator->eraseFromParent();

          }
        }

      }else if(auto *ret = dyn_cast<ReturnInst>(terminator)){
        // don't mess with return instructions
        continue;
      }
    }

    
  errs() << "Successfully flattened function: " << F.getName() << "\n";
  return true;

  }  


  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
        bool modified = false;
        SmallVector<Function*, 8> functionsToFlatten;

        for (Function &F : M) {
            if (!F.isDeclaration() && F.size() >= 3) {
                functionsToFlatten.push_back(&F);
            }
        }

        for (Function *F : functionsToFlatten) {
            if (flatten(*F)) {
                modified = true;
            }
        }

        return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }

};

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "Control Flow Flattening Pass",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    MPM.addPass(ControlFlowFlattening());
                });
        }
    };
}}

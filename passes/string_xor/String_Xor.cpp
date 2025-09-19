#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallVector.h"
#include <random>
#include <algorithm>

using namespace llvm;

namespace {
struct StringXORObfuscation : public PassInfoMixin<StringXORObfuscation> {
private:
  //random number generator as xor key
  std::mt19937 rng;

  uint8_t generateXORKey(){
    std::uniform_int_distribution<uint8_t> dist(1,255);
    return dist(rng);
  }

  std::string xorString(const std::string& str, uint8_t key){
    std::string result = str;
    for (size_t i = 0; i < result.size(); i++) {
      result[i] ^= key;
    }

    return result;
  }
  
  Function* declareExternalDecryptFunction(Module& M){
    const std::string funcName = "decrypt_string_xor";

    // check if function already exists
    if(Function* existingFunc = M.getFunction(funcName)){
      return existingFunc;
    }

    // create the function's signature (char* str, int len, char key ) -> char*
    LLVMContext& context = M.getContext();
    Type* charPtrTy = Type::getInt8PtrTy(context);
    Type* intTy = Type::getInt32Ty(context);
    Type* charTy = Type::getInt8Ty(context);

    FunctionType* funcType = FunctionType::get(
      charPtrTy, // th return type
      {charPtrTy, intTy, charTy}, // the parameter type
      false // The function is not variadic
    );

    // The decryption function will be linked later
    Function* decryptFunc = Function::Create(
      funcType,
      Function::ExternalLinkage,
      funcName,
      M
    );

    // We can set the paramerters names
    auto argIt = decryptFunc->arg_begin();
    argIt->setName("str");
    (++argIt)->setName("len");
    (++argIt)->setName("key");

    return decryptFunc;
  }

public:
  StringXORObfuscation() : rng(std::random_device{}()){}

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM){
    LLVMContext &context = M.getContext();

    bool modified = false;

    // Find all string Constants
    SmallVector<GlobalVariable*, 16> stringGlobals;

    for(GlobalVariable& GV : M.globals()){
      if(GV.hasInitializer() && GV.isConstant()){
        if(ConstantDataArray* CDA = dyn_cast<ConstantDataArray>(GV.getInitializer())){
          if(CDA->isString()){
            errs() << "Found String: " << CDA->getAsString() << "\n";
            stringGlobals.push_back(&GV);
            errs() << "bla\n";
          }
        }
      }
    }
    if(stringGlobals.empty()){
      errs() << "No strings found to obfuscate!\n";
      return PreservedAnalyses::all();
    }

    //errs() << "Found " << stringInstructions.size() << " instructions using strings\n";

    //Declare the decryption external function
    Function* decryptFunc = declareExternalDecryptFunction(M);

    // Now we process each constant string
    for (GlobalVariable *stringGV : stringGlobals){
      ConstantDataArray* originalCDA = cast<ConstantDataArray>(stringGV->getInitializer());
      std::string originalStr = originalCDA->getAsString().str();

      // DEBUG: Check if string has users
      errs() << "Processing string: " << originalStr << "\n";
      errs() << "Number of users: " << stringGV->getNumUses() << "\n";
    
      if (stringGV->use_empty()) {
        errs() << "String '" << originalStr << "' has no users, skipping...\n";
        continue;
      }

      uint8_t xorKey = generateXORKey();

      std::string encryptedStr = xorString(originalStr, xorKey);

      errs() << "Original: " << originalStr << "\n";
      errs() << "Key: " << (int)xorKey << "\n";
      errs() << "Encrypted: " << encryptedStr << "\n";

      // here we create a encrypted global variable 
      // false mean dont't add a null terminator add the end of the string
      Constant* encryptedCDA = ConstantDataArray::getString(context, encryptedStr, true);
      GlobalVariable* encryptedGV = new GlobalVariable(
        M,
        encryptedCDA->getType(),
        false, // means string isn't constant 
        GlobalValue::PrivateLinkage,
        encryptedCDA,
        stringGV->getName().str()+"_encrypted"
      );

      //create xor key global variable
      GlobalVariable* keyGV = new GlobalVariable(
        M,
        Type::getInt8Ty(context),
        true, // means constant
        GlobalValue::PrivateLinkage,
        ConstantInt::get(Type::getInt8Ty(context), xorKey),
        stringGV->getName().str() + "_key"
      );

      // here we will replace all uses of the original string
      SmallVector<User*, 16> users(stringGV->users());

      for(User* user : users){
        if(Instruction* inst = dyn_cast<Instruction>(user)){
          IRBuilder<> builder(inst);

          // create a call To the external decryption function
          Value* encryptedPtr = builder.CreateBitCast(
            encryptedGV,
            Type::getInt8PtrTy(context),
            "encrypted_ptr"
          );

          Value* strLen = ConstantInt::get(
            Type::getInt32Ty(context),
            originalStr.size()
          );

          Value* key = builder.CreateLoad(
            Type::getInt8Ty(context),
            keyGV,
            "key"
          );

          CallInst* decryptCall = builder.CreateCall(
            decryptFunc,
            {encryptedPtr, strLen, key},
            "decrypted_str"
          );

          /// here we replace the uses
          Value* castedResult = builder.CreateBitCast(
            decryptCall,
            stringGV->getType(),
            "casted_result"
          );

          inst->replaceUsesOfWith(stringGV, castedResult);

          modified = true;
          continue;
        }

        if (ConstantExpr* CE = dyn_cast<ConstantExpr>(user)) {
          SmallVector<User*, 4> ceUsers(CE->users()); // copy
          for (User* ceUser : ceUsers) {
            if (Instruction* iu = dyn_cast<Instruction>(ceUser)) {
              Instruction* newInst = CE->getAsInstruction(iu); // inserts before iu
              iu->replaceUsesOfWith(CE, newInst);
            }
          }
        }
      }
      //remove the original strings 
      if(stringGV->use_empty()){
        stringGV->eraseFromParent();
      }
    }

    return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};
}


extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "String XOR Obfuscation",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    MPM.addPass(StringXORObfuscation());
                });
        }
    };
}

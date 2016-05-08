#include <memory>
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/NestedNameSpecifier.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/TemplateBase.h"
#include "clang/AST/Type.h"
#include "clang/AST/TypeLoc.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/Sema.h"
using clang::RecursiveASTVisitor;
using clang::CompilerInstance;
namespace meditation {

class BaseAstVisitor : public RecursiveASTVisitor<BaseAstVisitor>{
public:
    explicit BaseAstVisitor(CompilerInstance* c): m_compiler(c){}
    virtual ~BaseAstVisitor(){}
private:
    CompilerInstance *m_compiler;
};

class CFuncVisitor : public RecursiveASTVisitor<CFuncVisitor>{
public:
    explicit CFuncVisitor(std::vector<clang::FunctionDecl*>& fl):m_func_list(fl){}
    virtual ~CFuncVisitor(){}
public:
    bool VisitFunctionDecl(clang::FunctionDecl *fdecl) {
        if(fdecl->isThisDeclarationADefinition()) {
            if(fdecl->getBody()){
                TraverseStmt(fdecl->getBody());
            }
        }
        return true;
    }
    bool VisitStmt(clang::Stmt *stmt){
        if(llvm::isa<clang::CallExpr>(*stmt)){
            auto def = llvm::cast<clang::CallExpr>(*stmt).getDirectCallee();
            m_func_list.push_back(def);
        }
        return true;
    }
private:
    std::vector<clang::FunctionDecl*>& m_func_list;
};

namespace {
bool isInputFile(const std::vector<clang::FrontendInputFile> input_files, llvm::StringRef file_name){
    for(const auto& i:input_files){
        if(i.getFile() == file_name){
            return true;
        }
    }
    return false;
}
}

class MeditationConsumer : public clang::ASTConsumer{
public:
    MeditationConsumer(clang::CompilerInstance *c) :m_compiler(c) {}
    virtual bool HandleTopLevelDecl(clang::DeclGroupRef D){
        for(auto& d:D){
            CFuncVisitor v(m_func_list);
            v.TraverseDecl(d);
        }
        return true;
    }
    virtual void HandleTranslationUnit(clang::ASTContext &ctx) {
        for(auto i:m_func_list){
            llvm::StringRef defFileName = ctx.getSourceManager().getFilename(i->getLocation());
            //if(!isInputFile(m_compiler->getFrontendOpts().Inputs, defFileName)){
                i->dump();
           // }
        }
    }
private:
    clang::CompilerInstance *m_compiler;
    std::vector<clang::FunctionDecl*> m_func_list;

};

class MeditationAction: public clang::ASTFrontendAction
{
public:
    explicit MeditationAction(clang::CompilerInstance *c) : m_compiler{c}{}
protected:

    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &compiler, llvm::StringRef)
    {
        return std::unique_ptr<MeditationConsumer>(new MeditationConsumer(m_compiler));
    }
private:
    clang::CompilerInstance *m_compiler;
};
}


#include "Meditation_driver.h"
using meditation::createCompilerInstance;
using meditation::MeditationAction;
int main(int argc, const char **argv)
{
    std::unique_ptr<clang::CompilerInstance> compiler(createCompilerInstance(argc, argv));
    if(compiler){
        std::unique_ptr<clang::ASTFrontendAction> action(new MeditationAction(compiler.get()));
        compiler->ExecuteAction(*action);
    }
    return 0;
}
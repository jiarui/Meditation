#include <memory>
#include <tuple>
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

typedef std::tuple<clang::FunctionDecl*, std::vector<clang::FunctionDecl*>> FunctionDependency;

class CFuncCallVisitor : public RecursiveASTVisitor<CFuncCallVisitor>{
public:
    explicit CFuncCallVisitor(std::vector<clang::FunctionDecl*>& func_list) : _FuncList(func_list) {}
    virtual ~CFuncCallVisitor() {}
    bool VisitCallExpr(clang::CallExpr *callexpr){
        auto def = llvm::cast<clang::CallExpr>(*callexpr).getDirectCallee();
        if(def) { //Not call back
            _FuncList.push_back(def);
        }
        return true;
    }
private:
    std::vector<clang::FunctionDecl*>& _FuncList;
};

class CFuncDeclVisitor : public RecursiveASTVisitor<CFuncDeclVisitor>{
public:
    explicit CFuncDeclVisitor(std::vector<FunctionDependency>& fl):_FuncDep(fl){}
    virtual ~CFuncDeclVisitor(){}
public:
    bool VisitFunctionDecl(clang::FunctionDecl *fdecl) {
        if(fdecl->isThisDeclarationADefinition() && fdecl->isGlobal() && !fdecl->isInlined()) {
            FunctionDependency fdep = std::make_tuple(fdecl, std::vector<clang::FunctionDecl*>{});
            std::vector<clang::FunctionDecl*> funclist;
            if(auto body = fdecl->getBody()){
                CFuncCallVisitor call_v(funclist);
                call_v.TraverseStmt(body);
            }
            _FuncDep.push_back(std::make_tuple(fdecl, funclist));
        }
        return true;
    }
private:
    std::vector<FunctionDependency>& _FuncDep;
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
            CFuncDeclVisitor v(_FuncDep);
            v.TraverseDecl(d);
        }
        return true;
    }
    virtual void HandleTranslationUnit(clang::ASTContext &ctx) {
        for(auto const& i:_FuncDep){
            //llvm::outs()<<"FunctionDependency:  ";
            llvm::outs()<<'{';
            auto function = std::get<0>(i);
            llvm::outs()<<R"("function")"<<':';
            printJSON(function, ctx.getSourceManager());
            llvm::outs()<<',';
            llvm::outs()<<R"("dependency")"<<':';
            llvm::outs()<<'[';
            auto& function_deps = std::get<1>(i);
            for(auto j = 0; j<function_deps.size(); ++j){
                printJSON(function_deps[j], ctx.getSourceManager());
                if(j!=function_deps.size()-1){
                    llvm::outs()<<',';
                }
            }
            llvm::outs()<<']';
            llvm::outs()<<'}';
            llvm::outs()<<'\n';
        }
    }
private:
    void print(clang::FunctionDecl*, const clang::SourceManager&);
    void printJSON(clang::FunctionDecl*, const clang::SourceManager&);
    clang::CompilerInstance *m_compiler;
    std::vector<FunctionDependency> _FuncDep;

};

void MeditationConsumer::printJSON(clang::FunctionDecl *function, const clang::SourceManager& sourceManager){
    llvm::outs()<<'{'<<R"("name")"<<':'<<'\"'<<function->getName()<<'\"'<<',';
    llvm::outs()<<R"("file")"<<':'<<'\"'<<sourceManager.getFilename(function->getLocation())<<'\"'<<',';
    //llvm::outs()<<R"("decl")"<<":"<<'\"';
    //llvm::outs()<<'\"'<<'}';

}

void MeditationConsumer::print(clang::FunctionDecl *function, const clang::SourceManager& sourceManager){
    llvm::outs()<<'{';
    llvm::outs()<<sourceManager.getFilename(function->getLocStart())<<':';
    llvm::outs()<<function->getName();
    llvm::outs()<<'}';
}


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
#include "llvm/Support/ManagedStatic.h"

using meditation::createCompilerInstance;
using meditation::MeditationAction;
int main(int argc, const char **argv)
{
    std::unique_ptr<clang::CompilerInstance> compiler(createCompilerInstance(argc, argv));
    if(compiler){
        std::unique_ptr<clang::ASTFrontendAction> action(new MeditationAction(compiler.get()));
        compiler->ExecuteAction(*action);
    }
    llvm::llvm_shutdown();
    return 0;
}
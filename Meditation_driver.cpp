//
// Created by coaxmetal on 16-5-4.
//
#include <ctype.h>
#include <stdint.h>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Tool.h"
#include "llvm/Support/Process.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"

#include "Meditation_driver.h"

using clang::CompilerInstance;
using clang::CompilerInvocation;
using clang::IntrusiveRefCntPtr;
using clang::DiagnosticOptions;
using clang::TextDiagnosticPrinter;
using clang::DiagnosticIDs;
using clang::DiagnosticsEngine;
using clang::driver::Driver;
using clang::driver::Compilation;
using clang::driver::JobList;
using clang::driver::Command;
using std::unique_ptr;

namespace {
std::string getExecutablePath(const char* argv0){
    void *main_addr = (void*)(intptr_t)getExecutablePath;
    return llvm::sys::fs::getMainExecutable(argv0, main_addr);
}
}

namespace meditation {
CompilerInstance *createCompilerInstance(int argc, const char **argv)
{
    std::string path = getExecutablePath(argv[0]);
    IntrusiveRefCntPtr<DiagnosticOptions> diag_options = new DiagnosticOptions;
    TextDiagnosticPrinter *diag_client = new TextDiagnosticPrinter(llvm::errs(), &*diag_options);
    IntrusiveRefCntPtr<DiagnosticIDs> diag_id = new DiagnosticIDs;
    DiagnosticsEngine diagnostics(diag_id, &*diag_options, diag_client);

    Driver driver{path, llvm::sys::getDefaultTargetTriple(), diagnostics};
    driver.setTitle("Meditation");

    llvm::SmallVector<const char*, 256> args;
    llvm::SpecificBumpPtrAllocator<char> argAllocator;
    std::error_code EC = llvm::sys::Process::GetArgumentVector(args, llvm::makeArrayRef(argv, argc), argAllocator);
    args.push_back("-fsyntax-only");
    if(EC){
        llvm::errs()<<"Cannot get arguments: " << EC.message() << '\n';
    }

    unique_ptr<Compilation> compilation(driver.BuildCompilation(args));
    if(compilation->getJobs().size()!=1 || !llvm::isa<Command>(*(compilation->getJobs().begin()))) {
        llvm::SmallString<256> msg;
        llvm::raw_svector_ostream out(msg);
        compilation->getJobs().Print(out, "; ", true);
        diagnostics.Report(clang::diag::err_fe_expected_compiler_job) << out.str();
        return nullptr;
    }
    const Command& command = llvm::cast<Command>(*(compilation->getJobs().begin()));
    if (StringRef(command.getCreator().getName()) != "clang") {
        diagnostics.Report(clang::diag::err_fe_expected_clang_command);
        return nullptr;
    }
    const clang::driver::ArgStringList &cc_arguments = command.getArguments();
    unique_ptr<CompilerInvocation> invocation(new CompilerInvocation);
    CompilerInvocation::CreateFromArgs(*invocation, cc_arguments.data(), cc_arguments.data()+cc_arguments.size(), diagnostics);
    invocation->getFrontendOpts().DisableFree = false;

    CompilerInstance *compiler = new CompilerInstance;
    compiler->setInvocation(invocation.release());

    compiler->createDiagnostics();
    if(!compiler->hasDiagnostics()) return nullptr;
    if(compiler->getHeaderSearchOpts().UseBuiltinIncludes && compiler->getHeaderSearchOpts().ResourceDir.empty()){
        compiler->getHeaderSearchOpts().ResourceDir = CompilerInvocation::GetResourcesPath(argv[0], (void*)(intptr_t)getExecutablePath);
    }

    return compiler;

}
}

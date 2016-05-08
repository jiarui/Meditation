cmake_minimum_required(VERSION 3.2)
project(Meditation)
set(LLVM_MODULES all)
execute_process(COMMAND llvm-config --cppflags --libs $(LLVM_MODULES) OUTPUT_VARIABLE LLVM_FLAGS)
set(LLVM_PATH /usr/lib/llvm-3.7)

include_directories(${LLVM_PATH}/include/)
link_directories(${LLVM_PATH}/lib/)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES Meditation.cpp Meditation_driver.h Meditation_driver.cpp)
add_executable(Meditation ${SOURCE_FILES})

target_link_libraries(Meditation
        clangFrontend
        clangSerialization
        clangDriver
        clangParse
        clangSema
        clangAnalysis
        clangAST
        clangBasic
        clangEdit
        clangLex)

target_link_libraries(Meditation
        LLVMX86AsmParser # MC, MCParser, Support, X86CodeGen, X86Desc, X86Info
        LLVMX86CodeGen # Analysis, AsmPrinter, CodeGen, Core, MC, Support, Target,
        # X86AsmPrinter, X86Desc, X86Info, X86Utils
        LLVMX86Desc # MC, MCDisassembler, Object, Support, X86AsmPrinter, X86Info
        LLVMX86AsmPrinter # MC, Support, X86Utils
        LLVMX86Info # Support
        LLVMX86Utils # Core, Support
        LLVMCodeGen # Analysis, Core, MC, Scalar, Support, Target, TransformUtils
        LLVMipo
        LLVMScalarOpts
        LLVMInstCombine
        LLVMTransformUtils
        LLVMTarget # Analysis, MC, Core, Support
        LLVMAnalysis # Core, Support
        LLVMOption # Support
        LLVMMCDisassembler # MC, Support
        LLVMMCParser # MC, Support
        LLVMMC # Object, Support
        LLVMProfileData # Core, Support, Object
        LLVMObject # BitReader, Core, Support
        LLVMBitReader # Core, Support
        LLVMCore # Support
        LLVMSupport
        pthread
        z
        tinfo
        ${CMAKE_DL_LIBS}
        )
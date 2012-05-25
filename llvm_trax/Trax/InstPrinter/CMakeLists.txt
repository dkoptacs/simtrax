include_directories( ${CMAKE_CURRENT_BINARY_DIR}/.. 
                     ${CMAKE_CURRENT_SOURCE_DIR}/.. )

add_llvm_library(LLVMTraxAsmPrinter
    TraxInstPrinter.cpp
  )

add_dependencies(LLVMTraxAsmPrinter TraxCodeGenTable_gen)

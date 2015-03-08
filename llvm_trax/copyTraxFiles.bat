:: paths to copy from/to
set source=Trax
set llvmVer=llvm-3.5.0.src
set destInclude=%llvmVer%\\include\\llvm\\IR
set destTarget=%llvmVer%\\lib\\Target\\Mips

:: copy IntrinsicsMips.td
copy /Y %source%\\IntrinsicsMips.td   %destInclude%\\IntrinsicsMips.td

:: copy the rest
copy /Y %source%\\MipsInstrFormats.td %destTarget%\\MipsInstrFormats.td
copy /Y %source%\\MipsSchedule.td     %destTarget%\\MipsSchedule.td
copy /Y %source%\\MipsInstrInfo.td    %destTarget%\\MipsInstrInfo.td

pause

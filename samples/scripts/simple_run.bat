:: This is an example script telling how to run simtrax on Windows

@echo off

set SIMTRAXROOT=%~dp0\..\..
::SIMTRAXROOT=\absolute\path\to\simtrax

set ASSEMBLY=%SIMTRAXROOT%\samples\bin\gradient\gradient_rt-llvm.s
:: simulation report file
set OUTPUT_FILE=temp_results.txt
:: image output name
set OUTPUT_PREFIX=out
:: hardware configuration file
set CONFIG=%SIMTRAXROOT%\samples\configs\default.config

:: WIDTH=32
:: HEIGHT=32
set WIDTH=64
set HEIGHT=64
:: WIDTH=128
:: HEIGHT=128
:: WIDTH=512
:: HEIGHT=512
set SIMULATION_THREADS=2
set NUM_PROCS=32
set THREADS_PER_PROC=1
set SIMD_WIDTH=1
set NUM_TMS=4
set NUM_ICACHES=4
set NUM_ICACHE_BANKS=16
set NUM_L2S=1
set NUM_REGS=36
:: 32 compiler used + 4 reserved
::OTHER is used just to add extra arguments
set OTHER=--disable-usimm

set SIMTRAX=%SIMTRAXROOT%\sim\bin\simtrax.exe

:: SCENE=--no-scene
::cornell
set SCENEDIR=%SIMTRAXROOT%\samples\scenes\cornell
set SCENE=--view-file %SCENEDIR%\cornell.view --model %SCENEDIR%\CornellBox.obj --light-file %SCENEDIR%\cornell.light

set ICACHE_CONF=--num-icaches %NUM_ICACHES% --num-icache-banks %NUM_ICACHE_BANKS% --num-l2s %NUM_L2S%
set CORE_CONF=--num-regs %NUM_REGS% --num-thread-procs %NUM_PROCS% --threads-per-proc %THREADS_PER_PROC% --num-cores %NUM_TMS% --simd-width %SIMD_WIDTH%
set IMAGE_CONF=--width %WIDTH% --height %HEIGHT% --output-prefix %OUTPUT_PREFIX%
set MISC_CONF=--simulation-threads %SIMULATION_THREADS% --config-file %CONFIG%

set RUN=%SIMTRAX% %OTHER% %CORE_CONF% %IMAGE_CONF% %MISC_CONF% %ICACHE_CONF% %SCENE% --load-assembly %ASSEMBLY%
echo %RUN%
call %RUN% > %OUTPUT_FILE% 2>&1


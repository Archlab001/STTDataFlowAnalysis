#!/bin/bash
#mkdir -p analyzed
mkdir -p logs
mkdir -p reports
mkdir -p netlist
mkdir -p tmax
mkdir -p svf
mkdir -p WORK

export ultra_switch=true
export high_switch=true
export area_switch=false
export power_switch=true
export fix_hold_switch=false
export exit_switch=true
export remove_tie_dont_use_switch=true
# export filelist=../aes_ASIC/rtl


dc_shell-xg-t -f ./scripts/read_rtl.tcl             |tee -i logs/read_rtl.log 


dc_shell-xg-t -f ./scripts/constraint_compile.tcl  |tee -i logs/constraint_compile.log

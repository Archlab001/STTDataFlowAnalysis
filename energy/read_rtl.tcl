#***************************************************
date
#****************************************************

#set filelist 	[getenv filelist]
#set topDir 		"$filelist"
#set search_path      	". $$filelist"
set exit_switch 	[getenv exit_switch]

# set TOP_MODULE  MulticastNetwork_Input
# set TOP_MODULE		SystolicNetwork_Input
set TOP_MODULE		RegFile
set_svf ${svfDir}/${TOP_MODULE}.svf

define_design_lib WORK -path WORK

analyze -format verilog -lib WORK  [sh ls $topDir/*.v]
elaborate $TOP_MODULE

current_design $TOP_MODULE	
link
if { [link] == 0 } {
	echo "Linking Error when deal with $TOP_MODULE"
	exit;
}
uniquify
if { [check_design] == 0 } {
	echo "Check Design Error when deal with $TOP_MODULE"
	exit;
}

write -format ddc -hierarchy -output ${netlistDir}/${TOP_MODULE}_unmapped.ddc

date

#****************************************************
#  Finish and Quit
#****************************************************
if {$exit_switch == "true"} {
exit
}

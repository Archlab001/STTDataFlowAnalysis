#****************************************************
#Note: 	clocks and reset signals were assumed as ideal
#****************************************************

#****************************************************
#MAX freq : 125M

set SYS_CLK_PERIOD 5.0
#****************************************************

#****************************************************
date
#****************************************************

set fix_hold_switch 	[getenv fix_hold_switch]
set exit_switch 	[getenv exit_switch]
set area_switch  	[getenv area_switch]
set power_switch  	[getenv power_switch]
set ultra_switch 	[getenv ultra_switch]
set high_switch  	[getenv high_switch]
set remove_tie_dont_use_switch [getenv remove_tie_dont_use_switch]


# Define some variables for design -- {aes_ASIC}
#****************************************************
# set TOP_MODULE		MulticastNetwork_Input
# set TOP_MODULE		SystolicNetwork_Input
# set TOP_MODULE		SystolicNetwork_Output
set TOP_MODULE		RegFile
set Rst_list		[list reset]
set Clk_list		[list clock]

set_svf 	${svfDir}/${TOP_MODULE}.svf

#Read saved unmapped ddc file
read_ddc  ${netlistDir}/${TOP_MODULE}_unmapped.ddc
# read_saif -input ./test.saif -inst top/systolic
#****************************************************
# Define The Design Enviroment
#****************************************************


set_operating_conditions -analysis_type bc_wc -min ff1p16v25c  -max tt1p05v25c

set_wire_load_mode  "segmented"
set_wire_load_model -name 16000  -library saed32hvt_tt1p05v25c


# remove dont_use attribute
#****************************************************
#if { $remove_tie_dont_use_switch == "true" } {
	#set_attribute  [get_lib_cells saed32rvt_dlvl_ff0p85v125c_i0p85v/TIE*] dont_touch false
#	set_attribute  [get_lib_cells smic18_tt/TIE*] dont_touch false
	#set_attribute  [get_lib_cells saed32rvt_dlvl_ss0p75v125c_i0p75v/TIE*] dont_touch false

	#set_attribute  [get_lib_cells saed32rvt_dlvl_ff0p85v125c_i0p85v/TIE*] dont_use false
#	set_attribute  [get_lib_cells smic18_tt/TIE*] dont_use false
	#set_attribute  [get_lib_cells saed32rvt_dlvl_ss0p75v125c_i0p75v/TIE*] dont_use false
#}

#****************************************************
# List of dont use cells. Avoiding scan and jk flip-flops, latches
#****************************************************
if 1 {

 set_dont_use saed32hvt_tt1p05v25c/SDFF*
 set_dont_use saed32rvt_tt1p05v25c/SDFF*



}


#****************************************************
# clock defination and reset
#****************************************************
#MAX freq : 125M
#physical clock and reset
current_design $TOP_MODULE

#should use pins? Can PAD_* be used?

create_clock -name wb_clk -period $SYS_CLK_PERIOD -waveform [list 0 [expr $SYS_CLK_PERIOD /2]]  [get_ports clock]

set_dont_touch_network  [all_clocks]
#wb_clk
set_ideal_network [get_ports "clock"]

set_dont_touch_network  [get_ports "$Rst_list"]
set_ideal_network [get_ports "reset"]

#****************************************************
# clock constraints
#****************************************************
# set_clock_latency	0.8	[all_clocks]
set_clock_uncertainty	0.3	[all_clocks]
set_clock_transition    0.3     [all_clocks]

report_clocks -nosplit >  ${reportsDir}/${TOP_MODULE}.clocks.txt

#****************************************************
# drive and load, max_fanout,max_capacitance
#****************************************************
set MAX_LOAD	[ load_of saed32hvt_tt1p05v25c/DFFX1_HVT/A1 ]

set_drive 0	[get_ports "$Rst_list"]
set_drive 0 	[get_ports "$Clk_list"]
# set_drive 0.05  [remove_from_collection [all_inputs] [get_ports [list clock reset]]] 

set_driving_cell -lib_cell I1025_EW [remove_from_collection [all_inputs] \
         [get_ports [list clock reset]]]
set_max_capacitance [expr $MAX_LOAD*25] [get_designs *]

set_load [expr $MAX_LOAD] [all_outputs]

set_max_fanout 32 [all_inputs]

set_max_transition 1.5 $TOP_MODULE

#****************************************************
# input delay and output delay
# input delay and output delay were set to 50% clock period
#****************************************************

#should use -max and -min

#define ports (exclude *clk ports)
#jtag ports; use set_case_analysis?
#input delay : max : period - setup
#input delay : min : hold
#output delay : max : setup
#output delay : min : -hold

set wb_in_ports [remove_from_collection [all_inputs]  [get_ports [list clock reset]]]
set wb_out_ports [remove_from_collection [all_outputs]  [get_ports [list clock reset]]]

set_input_delay -max 1 -clock wb_clk $wb_in_ports
set_input_delay -min 0.1 -clock wb_clk $wb_in_ports

set_output_delay -max 1 -clock wb_clk $wb_out_ports
set_output_delay -min -1 -clock wb_clk $wb_out_ports

#exit


#****************************************************
# false path
#****************************************************
#set_case_analysis is enough?
set_false_path -from [get_ports "$Rst_list"]


#****************************************************
# case_analysis
#****************************************************
set_case_analysis 0 [get_pins "wb_rst_i_0/DOUT"]

#****************************************************
# area and power
#if { $area_switch == "true"}{
	set_max_area     0   
#}
#if { $power_switch == "true" } {
	set_max_total_power 0 uw
#}

#****************************************************
# don't touch
#****************************************************


#****************************************************
#  Map and Optimize the design
#****************************************************
check_design

#compile
#avoid "assign"
set verilogout_no_tri true
set verilogout_equation false

#dxzhang:should be used only once
set_fix_multiple_port_nets -buffer_constants -all

#compile  -map_effort high -boundary_optimization
#compile  -map_effort medium -boundary_optimization

if {$ultra_switch == "true"} {
	# set_ultra_optimization true -force
	}
if {$high_switch == "true"} {
	# set compile_ultre_ungroup_dw true
 	# compile -map_effort high -boundary_optimization -ungroup
	compile_ultra
} else {
	compile -map_effort medium -boundary_optimization
	}

#****************************************************
#  fix_hold_time
#****************************************************

if {$fix_hold_switch == "true"} {
        set_fix_hold [get_clocks *]
        compile -incremental -only_hold_time
}	


check_design  >  ${reportsDir}/${TOP_MODULE}.check_design.txt
check_timing  >  ${reportsDir}/${TOP_MODULE}.check_timing.txt

#****************************************************
#  Output Reports
#****************************************************

report_design -nosplit >  ${reportsDir}/${TOP_MODULE}.design.txt
report_port -nosplit >  ${reportsDir}/${TOP_MODULE}.port.txt
report_net -nosplit >  ${reportsDir}/${TOP_MODULE}.net.txt
report_timing_requirements -nosplit >  ${reportsDir}/${TOP_MODULE}.timing_requirements.txt
report_constraint -nosplit -all_violators >  ${reportsDir}/${TOP_MODULE}.constraint.txt
report_timing -nosplit >  ${reportsDir}/${TOP_MODULE}.timing.txt
# report_area -nosplit >  ${reportsDir}/${TOP_MODULE}.area.txt
report_area -physical >  ${reportsDir}/${TOP_MODULE}.area.txt
# report_power -cell > ${reportsDir}/${TOP_MODULE}.power.txt
report_power -nosplit > ${reportsDir}/${TOP_MODULE}.power.txt

#****************************************************
#  Change Naming Rule
#****************************************************
remove_unconnected_ports -blast_buses [find -hierarchy cell {"*"}]
set bus_inference_style {%s[%d]}
set bus_naming_style {%s[%d]}
set hdlout_internal_busses true
change_names -hierarchy -rule verilog
define_name_rules name_rule -allowed {a-z A-Z 0-9 _} -max_length 255 -type cell
define_name_rules name_rule -allowed {a-z A-Z 0-9 _[]} -max_length 255 -type net
define_name_rules name_rule -map {{"\\*cell\\*" "cell"}}
define_name_rules name_rule -case_insensitive -remove_internal_net_bus -equal_ports_nets
change_names -hierarchy -rules name_rule

#****************************************************
#  Output Results
#****************************************************

write -format verilog   -hierarchy      -output  ${netlistDir}/${TOP_MODULE}.vg
write -format ddc -hierarchy -output ${netlistDir}/${TOP_MODULE}.ddc
write_sdf  ${netlistDir}/${TOP_MODULE}_post_dc.sdf
write_sdc  -nosplit ${netlistDir}/${TOP_MODULE}.sdc

date

#****************************************************
#  Finish and Quit
#****************************************************
if {$exit_switch == "true"} {
exit
}

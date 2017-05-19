open_project synth
set target [read [open "target.tcl" r]]

set INCLUDES "-I ../../../include/ -I../"
add_files $target -cflags "-std=c++0x -DBIT_ACCURATE $INCLUDES"
add_files -tb $target -cflags "-std=c++0x -DBIT_ACCURATE $INCLUDES"

set designs [split [read [open "designs.tcl" r]]]

open_solution base
csim_design -clean -compiler clang -clang_sanitizer
foreach design $designs {
    puts "-------------------- NOTE: Compiling $design --------------------"
    open_solution $design
    #config_compile -unsafe_math_optimizations
    set_part  {xc7vx690tffg1761-2}
    set_top $design
    create_clock -period 2

    csynth_design
}

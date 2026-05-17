[[ -d ghdl ]] || git clone https://github.com/ghdl/ghdl.git ghdl
../build/Release/src/vhdl_fe $(find ghdl/testsuite/synth -name '*.vhd' -or -name '*.vhdl' | grep -v 'synth/issue' )
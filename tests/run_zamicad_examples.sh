[[ -d zamiacad ]] || git clone git://git.code.sf.net/p/zamiacad/code zamiacad
../build/Release/src/vhdl_fe $(find zamiacad/examples -name '*.vhd' -or -name '*.vhdl' | grep -v _tb.vhd )
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../../../../target/lib/ 
export PATH=$PATH:../../../../target/bin/
export YUMA_HOME=../../../../../
export YUMA_MODPATH=$YUMA_MODPATH:../../yang/:../../../../modules/

BASENAME=simple_yang_test

yangdump format=h indent=4 module=../../yang/${BASENAME}.yang output=${BASENAME}.h
yangdump format=c indent=4 module=../../yang/${BASENAME}.yang output=${BASENAME}.c
yangdump format=cpp_test indent=4 module=../../yang/${BASENAME}.yang output=${BASENAME}.cpp


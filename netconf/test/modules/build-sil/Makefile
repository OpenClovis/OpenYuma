#ifndef JFLAG
#JFLAG = -j3
#endif
	
DIRS = device_test simple_list_test simple_yang_test

all: treeall

clean: treeclean

treeall:
	for dir in $(DIRS); do\
	  cd $$dir && $(MAKE) $(JFLAG) all && cd ..;\
        done

treeclean:
	for dir in $(DIRS); do\
	  cd $$dir && $(MAKE) clean  && cd ..;\
        done

ARCH=`uname -s` 
TOP=`pwd` 

all: 
	make ARCH=`uname -s` TOP=`pwd` -f Makefile.dir $@

.DEFAULT:
	make ARCH=`uname -s` TOP=`pwd` -f Makefile.dir $@


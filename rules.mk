# Obsolete version of Painter renderer
#LIB_OBJ += $O/gen_painter.o $O/pnt_ren_mthd.o $O/painter_clip.o \
#	$O/painter_util.c $O/paintr_trans.o
#LIBS += -ldrawcgm

# Finally, add the math library
LIBS += -lm

all: ${BUILD_LIBS} ${BUILD_EXES}
	@echo "Finished making ALL in " $(PWD)

submakes:
	@for i in dummy $(SUBMAKES) ; do \
	if test $$i != dummy ; then (cd $$i ; make $f ); fi ; done

subdepends:
	@for i in dummy $(SUBMAKES) ; do \
	if test $$i != dummy ; then (cd $$i ; make depend ); fi ; done

depend: subdepends
	@echo Generating dependencies in $(PWD)
	@echo '# Automatically generated on ' `date` > depend.mk.${ARCH}
	makedepend -p${TOP}/obj/${ARCH}/ -fdepend.mk.${ARCH} -DMAKING_DEPEND \
	  -- $(CFLAGS) -I${TOP} -- $(DEPENDSOURCE)

objdir:
	@echo "Creating object directory " $O
	@if test ! -d ${TOP}/obj ; then mkdir ${TOP}/obj ; fi
	@if test ! -d $O ; then mkdir $O ; fi

libdir:
	@echo "Creating library directory " $L
	@if test ! -d ${TOP}/lib ; then mkdir ${TOP}/lib ; fi
	@if test ! -d $L ; then mkdir $L ; fi

bindir:
	@echo "Creating bin directory " $B
	@if test ! -d ${TOP}/bin ; then mkdir ${TOP}/bin ; fi
	@if test ! -d $B ; then mkdir $B ; fi

clean:
	-for i in dummy $(SUBMAKES) ; do \
	if test $$i != dummy ; then (cd $$i ; make clean ); fi ; done
	-rm -f $O/* core

clobber: clean 
	-rm -f $(BUILD_EXES) ${BUILD_LIBS} depend.mk.${ARCH}

install:
	-for i in dummy $(INSTALLABLES) ; do \
	if test $$i != dummy ; then \
		(rm $(INSTALL_PATH)/$$i ; mv $$i $(INSTALL_PATH) ); \
	else true ; fi ; done
	-for i in dummy $(LIB_INSTALLABLES) ; do \
	if test $$i != dummy ; then \
		(rm $(LIB_INSTALL_PATH)/$$i ; mv $$i $(LIB_INSTALL_PATH) ); \
	else true ; fi ; done
	-for i in dummy $(SUBMAKES) ; do \
	if test $$i != dummy ; then (cd $$i ; make install ); \
	else true ; fi ; done

tarfile: $(CSOURCE) $(CXXSOURCE) $(FSOURCE) $(HFILES) $(DOCFILES) \
		$(MISCFILES) 
	if test ${PWD} == ${TOP} ; then rm -f /usr/tmp/drawp3d.tar ; fi
	tar -cvf /usr/tmp/drawp3d.tar $^
	-for i in dummy $(SUBMAKES) ; do \
	if test $$i != dummy ; then (tar -rvf /usr/tmp/drawp3d.tar $$i ); fi ; done

rcsclean:
	rcsclean $(CSOURCE) $(HFILES) $(FSOURCE) $(OTHER_SOURCE) $(DOCS)
	-for i in dummy $(SUBMAKES) ; do \
	if test $$i != dummy ; then (cd $$i ; make rcsclean); \
	else true ; fi ; done

rcscheckout: $(CSOURCE) $(HFILES) $(FSOURCE) $(OTHER_SOURCE) $(DOCS)
	-for i in dummy $(SUBMAKES) ; do \
	if test $$i != dummy ; then (cd $$i ; make rcscheckout); \
	else true ; fi ; done

$O/%.o : %.c
	@echo "Compiling " $<
	@if test ! -d ${TOP}/obj ; then mkdir ${TOP}/obj ; fi
	@if test ! -d $O ; then mkdir $O ; fi
	@${CC} -c -o $@ ${CFLAGS} $<

$O/%.o : %.cxx
	@echo "Compiling " $<
	@if test ! -d ${TOP}/obj ; then mkdir ${TOP}/obj ; fi
	@if test ! -d $O ; then mkdir $O ; fi
	@${CXX} -c -o $@ ${CFLAGS} $<

$O/%.o : %.f
	@echo "Compiling " $<
	@if test ! -d ${TOP}/obj ; then mkdir ${TOP}/obj ; fi
	@if test ! -d $O ; then mkdir $O ; fi
	@${FC} -c -o $@ ${FFLAGS} $<

.DEFAULT: submakes
	@echo "Making $@ in " $(PWD)


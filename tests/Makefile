.PHONY = all clean test runtest

PROGRAM = ac_test
all: runtest

CXXFLAGS = -O3 -g -march=native -Wall -DDEBUG
MYCXXFLAGS = -MMD -I.. $(CXXFLAGS)
%.o : %.cxx
	$(CXX) $< -std=c++11 -c $(MYCXXFLAGS)

-include dep.cxx
SRC = test_main.cxx ac_test_simple.cxx ac_test_aggr.cxx test_bigfile.cxx
SRC_BENCH = bench_main.cxx

OBJ = ${SRC:.cxx=.o}
OBJ_BENCH = ${SRC_BENCH:.cxx=.o}

-include dep.txt

$(PROGRAM) : testinput/text.tar testinput/image.bin
$(PROGRAM) : $(OBJ) ../libac.so
	$(CXX) $(OBJ) -L.. -lac -o $@
	-cat *.d > dep.txt

bench : $(OBJ_BENCH) ../libac.so
	$(CXX) $(OBJ_BENCH) -L.. -lac -o $@

runtest:$(PROGRAM)
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):.. ./$(PROGRAM) testinput/*

testinput/text.tar:
	echo "download testing files (gcc tarball)..."
	if [ ! -d testinput ] ; then mkdir testinput; fi
	cd testinput && \
	curl ftp://ftp.gnu.org/gnu/gcc/gcc-1.42.tar.gz -o text.tar.gz 2>/dev/null \
    && gzip -d text.tar.gz

testinput/image.bin:
	echo "download testing files.."
	if [ ! -d testinput ] ; then mkdir testinput; fi
	curl http://www.3dvisionlive.com/sites/default/files/Curiosity_render_hiresb.jpg -o $@ 2>/dev/null

clean:
	-rm -f *.o *.d dep.txt $(PROGRAM)

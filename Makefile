.PHONY: all clean run
all: test

CXXFLAGS := -std=c++11 -Wunused-parameter -fno-exceptions -fno-rtti -fno-stack-protector -fomit-frame-pointer
LDFLAGS := 
ifdef DEBUG
	CXXFLAGS := $(CXXFLAGS) -g
else
	CXXFLAGS := $(CXXFLAGS) -Os
endif


test: test.o | test.s
	clang++ $(CXXFLAGS) $(LDFLAGS) $^ -o $@
	strip -u -r $@

%.o: %.cpp
	clang++ $(CXXFLAGS) -c $< -o $@

%.s: %.cpp
	clang++ -masm=intel -S $^ $(CXXFLAGS) -o $@

test.o: ringbuffer.hpp

clean:
	-rm -rf *.o *.s test

RUNCMD=./$^
ifdef DEBUG
	RUNCMD=lldb -o run ./$^
endif

run: test
	$(RUNCMD)
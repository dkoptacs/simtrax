# If you don't like the defaults set evironment variables to something
# else.
EXE=simtrax


# for OSX
CXX = clang++
CXXFLAGS = -g -Wall -Wno-format-security -O3 -I/opt/local/include/
LDFLAGS = -L/opt/local/lib/ -lboost_regex-mt -lboost_system-mt -lboost_thread-mt

# for CADE
#CXX = g++
#CXXFLAGS = -g -Wall -O3 -I/usr/local/stow/boost/boost_1_55_0/include/ -L/usr/local/stow/boost/boost_1_55_0/lib/ -lboost_regex -lboost_system -lboost-thread

# for ubuntu/other linux
# you may need to add paths to your boost include (-I) and/or library (-L) directories, as above
#CXX = g++
#CXXFLAGS = -g -Wall -Wno-unused-result -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-format-security -O3
#LDFLAGS = -lboost_regex -lboost_system -lboost_thread

#LDFLAGS ?=

mkdirs=objs

all: mkdirs ${EXE}
default: mkdirs simwhrt

mkdirs:
	@mkdir -p objs;

objs/%.o: %.cc
	@echo "Building $<"
	@$(CXX) -o $@ -c $< -MD $(CXXFLAGS)

SOURCES := $(wildcard *.cc)
OBJS := $(addprefix objs/, $(notdir $(SOURCES:.cc=.o)))

# Include dependency files
-include $(patsubst %.o,%.d,$(OBJS))

${EXE}: $(OBJS)
	@echo "Building ${EXE}"
	@$(CXX) -o $@ $(OBJS) $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -rf objs/ ${EXE}

# If you don't like the defaults set evironment variables to something
# else.
EXE=simtrax


# for OSX
CXX = clang++
CXXFLAGS = -g -Wall -O3 -pthread -I/opt/local/include/ -L/opt/local/lib/ -lboost_regex-mt

#for CADE
#CXX = g++
#CXXFLAGS = -g -Wall -O3 -pthread -I/usr/local/stow/boost/boost_1_55_0/include/ -L/usr/local/stow/boost/boost_1_55_0/lib/ -lboost_regex

#LDFLAGS ?=

mkdirs=objs

all: mkdirs ${EXE}
default: mkdirs simwhrt

mkdirs:
	@mkdir -p objs;

objs/%.o: %.cc
	@echo "Building $<"
	@$(CXX) $(CXXFLAGS) -o $@ -c $< -MD

SOURCES := $(wildcard *.cc)
OBJS := $(addprefix objs/, $(notdir $(SOURCES:.cc=.o)))

# Include dependency files
-include $(patsubst %.o,%.d,$(OBJS))

${EXE}: $(OBJS)
	@echo "Building ${EXE}"
	@$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS)

clean:
	rm -rf objs/ ${EXE}

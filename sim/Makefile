# If you don't like the defaults set evironment variables to something
# else.
EXE=simtrax

CXX = g++-4.9
CXXFLAGS = -g -Wall -O3 -pthread --std=c++11

LDFLAGS ?=

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

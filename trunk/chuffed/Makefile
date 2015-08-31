CC = g++
#CC = ~/gcc-4.3.0/bin/g++
CPPFLAGS = -g -O3 -I . -I../
CPPFLAGS += -Wall -Wno-non-virtual-dtor -Wno-unused-function -Wno-deprecated
#CPPFLAGS += -DNDEBUG
#CPPFLAGS += -pg

# Work out the architecture.
ifeq ($(shell uname -m),x86_64)
CPPFLAGS += -m64
else
ifeq ($(shell uname -s),Darwin)
CPPFLAGS += -m64
else
CPPFLAGS += -m32
endif
endif

ifeq ($(PARALLEL),true)
#CC = /usr/lib64/lam/bin/mpic++
#CC = /usr/lib64/openmpi/1.4-gcc/bin/mpic++
CC = /usr/bin/mpic++
CPPFLAGS += -DPARALLEL
endif

SRCS = $(wildcard core/*.c) $(wildcard vars/*.c) $(wildcard branching/*.c) $(wildcard primitives/*.c) $(wildcard globals/*.c) $(wildcard parallel/*.c) $(wildcard mip/*.c) $(wildcard ldsb/*.c) $(wildcard mdd/*.c) $(wildcard mymip/*.c)

OBJS = $(addsuffix .o, $(basename $(SRCS)))
DEPS = $(addsuffix .d, $(basename $(SRCS)))

FLATZINCSRC = $(wildcard flatzinc/*.c)
FLATZINCOBJ = $(addsuffix .o, $(basename $(FLATZINCSRC)))
FLATZINCDEP = $(addsuffix .d, $(basename $(FLATZINCSRC)))

EXAMPLESRC =  $(wildcard examples/*.c)
EXAMPLEOBJ = $(addsuffix .o, $(basename $(EXAMPLESRC)))
EXAMPLEDEP = $(addsuffix .d, $(basename $(EXAMPLESRC)))
EXAMPLES = $(basename $(EXAMPLESRC))

all : fzn_chuffed
#$(EXAMPLES)

fzn_chuffed : $(FLATZINCOBJ) $(OBJS)
	$(CC) $(CPPFLAGS) -o $@ $(FLATZINCOBJ) $(OBJS)

$(EXAMPLES) : % : %.o $(OBJS)
	$(CC) $(CPPFLAGS) -o $@ $@.o $(OBJS)

clean :
	rm -f fzn_chuffed $(EXAMPLES) $(OBJS) $(FLATZINCOBJ) $(EXAMPLEOBJ) $(DEPS) $(FLATZINCDEP) $(EXAMPLEDEP)

-include $(DEPS)
-include $(FLATZINCDEP)
-include $(EXAMPLEDEP)

%.d: %.c
	@$(CC) -MM -MT  $(subst .d,.o,$@) -MT $@ $(CPPFLAGS) $< > $@

%.tab.h: %.tab.c
%.tab.c: %.yxx
	bison -t -o $@ -d $<

%.yy.c: %.lxx
	flex -o $@ $<

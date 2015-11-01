################ common flags and rules ########################################

.DELETE_ON_ERROR:

SHELL := /bin/bash

CXX = ${CROSS_COMPILE}g++
CC = ${CROSS_COMPILE}gcc
LD = ${CXX}
LDFLAGS =
LDLIBS =
flags = -march=armv7-a -mfpu=neon -mfloat-abi=hard -mthumb
CFLAGS = ${flags}
CXXFLAGS = ${flags}
CPPFLAGS = -I . -I include
flags += -funsigned-char
flags += -fno-strict-aliasing -fwrapv
CXXFLAGS += -std=gnu++1y
CXXFLAGS += -fno-operator-names
flags += -Og -g
flags += -Wall -Wextra
#flags += -Werror
flags += -Wno-unused-parameter -Wno-error=unused-function
CXXFLAGS += -ffunction-sections -fdata-sections

flags += -fmax-errors=3

export GCC_COLORS = 1

clean ::
	${RM} *.o


################ package magic #################################################

define declare_pkg =
${pkg}_CFLAGS != pkg-config --cflags ${pkg}
${pkg}_LDLIBS != pkg-config --libs ${pkg}
endef
$(foreach pkg,${declared_pkgs}, $(eval ${declare_pkg}))

CXXFLAGS += $(foreach pkg,${pkgs},${${pkg}_CFLAGS})
LDLIBS += $(foreach pkg,${pkgs},${${pkg}_LDLIBS})


################ automatic header dependencies #################################

# a place to put them out of sight
depdir := .dep
$(shell mkdir -p ${depdir})

# generate them
CPPFLAGS += -MMD -MQ $@ -MP -MF >( cat >${depdir}/$@.d )

# use them
-include ${depdir}/*.d

# clean them up
clean ::
	${RM} -r ${depdir}

# fix built-in rules that bork because they think all deps are sources
%: %.o
	${LINK.o} ${^:%.h=} ${LDLIBS} ${OUTPUT_OPTION}

%: %.c
	${LINK.c} ${^:%.h=} ${LDLIBS} ${OUTPUT_OPTION}

%: %.cc
	${LINK.cc} ${^:%.h=} ${LDLIBS} ${OUTPUT_OPTION}


################ to check what the compiler is making of your code #############

ifdef use_clang

%.asm: %.c
	$(COMPILE.c) -S -Xclang -masm-verbose $(OUTPUT_OPTION) $<
%.asm: %.cc
	$(COMPILE.cc) -S -Xclang -masm-verbose $(OUTPUT_OPTION) $<

else

%.asm: %.c
	$(COMPILE.c) -S -g0 -fverbose-asm $(OUTPUT_OPTION) $<
%.asm: %.cc
	$(COMPILE.cc) -S -g0 -fverbose-asm $(OUTPUT_OPTION) $<

endif

clean ::
	${RM} *.asm

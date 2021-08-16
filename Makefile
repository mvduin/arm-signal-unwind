all :: test unwind_sigreturn.o

clean ::
	${RM} test

check :: all
	./test

test: test.cc unwind_sigreturn.o
test: LDFLAGS += -rdynamic

unwind_sigreturn.o: unwind_sigreturn.c sa_restorer_v2.S rt_sa_restorer_v2.S
	${LINK.c} -r -nostdlib ${^:%.h=} ${OUTPUT_OPTION}

include common.mk

flags += -fexceptions -fnon-call-exceptions -Wa,-mthumb

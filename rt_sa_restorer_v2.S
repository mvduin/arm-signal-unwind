#include <asm/unistd.h>

.syntax unified

.global __default_rt_sa_restorer_v2
.type __default_rt_sa_restorer_v2, "function"
	.fnstart
	.personality __eh_personality_sigframe 
	nop 
__default_rt_sa_restorer_v2:
	mov	r7, __NR_rt_sigreturn
	svc	0
	.handlerdata
	.word	128	// offset to ucontext
	.fnend 

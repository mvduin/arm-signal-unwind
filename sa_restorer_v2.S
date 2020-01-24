#include <asm/unistd.h>

.syntax unified

.global __default_sa_restorer
.type __default_sa_restorer, "function"
.global __default_sa_restorer_v2
.type __default_sa_restorer_v2, "function"
	.fnstart 
	.personality __eh_personality_sigframe 
	nop 
__default_sa_restorer:
__default_sa_restorer_v2:
	mov	r7, __NR_sigreturn
	svc	0
	.handlerdata
	.word	0	// offset to ucontext
	.fnend 

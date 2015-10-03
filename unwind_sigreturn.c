#include "arm-unwind-pr.h"
#include <ucontext.h>
#include <signal.h>

typedef __u16  u16;
typedef __u32  u32;

#define T_BIT ( 1 << 5 )

static void *
next_instruction( struct sigcontext const *mc )
{
	// "Wait!", you say, "what about Jazelle?", ... *rolls eyes*
	//
	if( mc->arm_cpsr & T_BIT ) {
		u16 *pc = (u16 *) mc->arm_pc;
		if( *pc++ >= 0xe800 ) pc++;
		return (void *)( (u32) pc | 1 );
	}
	return (u32 *) mc->arm_pc + 1;
}

static pr_response_t
sigframe_unwind_virtual( struct Unwind_Context *ctx, struct ucontext *uc )
{
	// simulate a sigreturn, but skip over current instruction
	// (see also restore_sigframe() in arch/arm/kernel/signal.c)


	// The idea behind uc_regspace is that you could iterate over the
	// coprocessor register blocks and skip the ones you don't recognize,
	// but if CONFIG_IWMMXT && ! test_thread_flag(TIF_USING_IWMMXT) then
	// the kernel will leave the iwmmxt regspace uninitialized, including
	// its size field, so yeah...
	//
	// You know what, not gonna bother with them for virtual unwind at all.
	// I can probably get away with that?


	// restore all core registers except PC
	set_sp( ctx, &uc->uc_mcontext.arm_r0 );
	pop_regs( ctx, 0x7fff );

	// set pc to next instruction
	set_pc( ctx, next_instruction( &uc->uc_mcontext ) );

	return PRC_SCAN_CONTINUE;
}

static struct ucontext *
sigframe_ucontext( struct Unwind_Context *ctx, struct Unwind_Exception *exc )
{
	// consult handler data for offset from SP to ucontext
	u32 *data = exc->ehtp;
	// theory:
	//	data[0]  is relative pointer to __eh_personality_sigframe
	//	data[1]  is our handlerdata
	// practice:
	//	data[0]  is relative pointer to __eh_personality_sigframe
	//	data[1]  is crap inserted by GNU assembler
	//	data[2]  is our handlerdata
	++data;
	if( *data == 0xb0b0b0 )
		++data;  // thank you GAS, but I'm not .personalityindex 0
	return get_sp( ctx ) + *data;
}

pr_response_t
__eh_personality_sigframe(
		unwind_state_t state,
		struct Unwind_Exception *exc,
		struct Unwind_Context *ctx )
{
	struct ucontext *uc = sigframe_ucontext( ctx, exc );

	switch( state.action ) {
	case US_SCAN:
		return sigframe_unwind_virtual( ctx, uc );
	case US_UNWIND:
		sigframe_unwind_virtual( ctx, uc );
		sigprocmask( SIG_SETMASK, &uc->uc_sigmask, NULL );
		return PRC_UNWIND_CONTINUE;
	default:
		return PRC_FAILURE;
	}
}

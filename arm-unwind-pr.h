#pragma once
#include <linux/types.h>
#include <stdbool.h>

// ARM EABI Exception Handling
//
// NOTE: Except for function names there's no worry here about polluting the
// global namespace, hence absolutely no excuse to use the hideous
// underscore-prefixed identifiers that the official definitions use.
//
// I've taken significant liberties in renaming things to increase readability.
//
// This header is biased towards the perspective of a personality routine.

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
	US_SCAN		=  0,  // virtual unwinding (stack-scanning)
	// valid responses:
	//	PRC_SCAN_CONTINUE
	//	PRC_SCAN_CATCH		should be avoided if state.no_catch
	//	PRC_FAILURE
	//
	// If scanning with state.no_catch (used e.g. to generate backtrace),
	// unwinder will ignore PRC_SCAN_CATCH and continue scanning anyway.

	US_UNWIND	=  1,  // actual unwinding
	US_UNWIND_RESUME=  2,  // resume unwinding after catch/cleanup
	// valid responses:
	//	PRC_UNWIND_CONTINUE
	//	PRC_UNWIND_CATCH	unless state.no_catch (forced unwind)
	//	PRC_UNWIND_CLEANUP

	// Note: other responses are treated as PRC_FAILURE, and during unwind
	// usually results in immediate abort().

} unwind_action_t;

typedef struct {
	unwind_action_t action : 2;
	bool		: 1;
	bool no_catch	: 1;
	__u32		: 0;
} unwind_state_t;


typedef enum {
	// catch/cleanup is treated the same by the unwinder, but the p.r. had
	// better know the difference to be able to honor US_NO_CATCH.
	//
	PRC_UNWIND_CONTINUE	= 8,
	PRC_UNWIND_CATCH	= 7,
	PRC_UNWIND_CLEANUP	= PRC_UNWIND_CATCH,

	PRC_SCAN_CONTINUE	= PRC_UNWIND_CONTINUE,
	PRC_SCAN_CATCH		= 6,

	PRC_FAILURE		= 9,  // unspecified fatal error
} pr_response_t;


// Reasons for the destructor to be called.  Note that if you recognize your
// own exceptions you may use other reasons, or not use it at all.  But these
// two codes are used generically by the unwinder in response to failure or a
// call to _Unwind_DeleteException( exc ).  The latter is what you should do if
// you catch an foreign exception.
//
typedef enum {
	DEL_EXCEPTION_FAILURE	= 9,
	DEL_EXCEPTION_CAUGHT	= 1,
} del_reason_t;


struct __attribute__((aligned(8))) Unwind_Exception {
	// officially called _Unwind_Control_Block

	// Identifies originating language and implementation.
	char originator[8];

	// See explanation above.
	void (*destructor)( del_reason_t, struct Unwind_Exception *exc );

	// Initialize first field to zero before throwing exception.
	__u32 unwinder_private[5];
	
	// Filled in by personality routine when returning PRC_SCAN_CATCH, to
	// be able to easily recognize the same frame during unwind.  The only
	// mandatory field is the stack frame pointer, remainder may be used
	// freely as needed by the language.
	__u32 catch_sp;
	__u32 catch_cache[5];
	
	// Filled in by personality routine when returning PRC_UNWIND_CATCH or
	// PRC_UNWIND_CLEANUP to pass info it may need to resume unwinding.
	__u32 cleanup_cache[4];

	// Data passed from unwinder to personality routine:
	__u32 fnstart;		// function start address
	__u32 *ehtp;		// pointer to EHT entry
	__u32 compact_eht : 1;	// single-word EHT entry embedded in index table
};

// dispose of exception after having been caught
void _Unwind_DeleteException( struct Unwind_Exception *exc );

// resume unwinding after cleanup;  reenters p.r. with US_UNWIND_RESUME
__attribute__(( noreturn ))
void _Unwind_Resume( struct Unwind_Exception *exc );


// Access virtual register set (low-level functions)
//
// Don't use the primitives, use the wrapper functions below.  The exception
// handling spec may give you the impression there are more ways you can call
// these primitives -- there aren't, libgcc doesn't implement them.

struct Unwind_Context;

void _Unwind_VRS_Set( struct Unwind_Context *, int, int, int, const void * );
void _Unwind_VRS_Get( struct Unwind_Context *, int, int, int, void * );
void _Unwind_VRS_Pop( struct Unwind_Context *, int, __u32, int );


// Access virtual core registers (r0 .. r15)
// Note that bit 0 of PC is used as thumb-bit, just like LR

#define R_PC  15
#define R_LR  14
#define R_SP  13

static inline __u32 get_reg( struct Unwind_Context *ctx, int reg )
{
	__u32 val;
	_Unwind_VRS_Get( ctx, 0, reg, 0, &val );
	return val;
}

static inline void set_reg( struct Unwind_Context *ctx, int reg, __u32 val )
{
	_Unwind_VRS_Set( ctx, 0, reg, 0, &val );
}

static inline void *get_sp( struct Unwind_Context *ctx )
{
	return (void *) get_reg( ctx, R_SP );
}

static inline void set_sp( struct Unwind_Context *ctx, void *sp )
{
	set_reg( ctx, R_SP, (__u32) sp );
}

static inline void set_lr( struct Unwind_Context *ctx, void *lr )
{
	set_reg( ctx, R_LR, (__u32) lr );
}

static inline void set_pc( struct Unwind_Context *ctx, void *pc )
{
	set_reg( ctx, R_PC, (__u32) pc );
}


// Pop virtual registers from the stack (using virtual SP)

// pop any subset of the 32 core registers (popping SP is allowed)
static inline void pop_regs( struct Unwind_Context *ctx, __u32 regmask )
{
	_Unwind_VRS_Pop( ctx, 0, regmask, 0 );
}

// pop contiguous subset of the 16 or 32 Neon/VFP dword registers
static inline void pop_vfp_dregs( struct Unwind_Context *ctx,
		__u32 start, __u32 count )
{
	_Unwind_VRS_Pop( ctx, 1, start << 16 | count, 5 );
}

// pop contiguous subset of the 16 WMMX data dword registers
static inline void pop_wmmxd_dregs( struct Unwind_Context *ctx,
		__u32 start, __u32 count )
{
	_Unwind_VRS_Pop( ctx, 3, start << 16 | count, 5 );
}

// pop any subset of the 4 WMMX control word registers
static inline void pop_wmmxc_regs( struct Unwind_Context *ctx, __u32 regmask )
{
	_Unwind_VRS_Pop( ctx, 4, regmask, 0 );
}


#ifdef __cplusplus
}   /* extern "C" */
#endif

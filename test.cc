#include <asm/unistd.h>
#include <signal.h>
#include <stdio.h>
#include <execinfo.h>
#include <exception>
#include <stdlib.h>

template< typename T, size_t n >
static constexpr size_t countof( T (&)[n] ) {  return n;  }

void backtrace()
{
	void *ptrs[100];
	backtrace_symbols_fd( ptrs, backtrace( ptrs, countof(ptrs) ), 2 );
}

[[noreturn]]
void abort_backtrace()
{
	backtrace();
	abort();
}


struct IntendedUncaughtException {};
struct SegFault {};

void backtrace_handler( int )
{
	backtrace();
	throw SegFault {};
}


void usr1_handler( int )
{
}


void segv_handler( int )
{
	// clobber caller-save VFP register to verify it is restored
	register float f asm("s0") = 42;
	asm volatile( "" :: "t"(f) );

	throw SegFault {};
}

// compile with hardfp and optimization to ensure f ends up in s0 and stays
// here until the fprintf
[[gnu::noinline]]
void test_segv( float f = 0 )
{
	int volatile *p = NULL;
	asm volatile( "" : "+r"(p) );

	asm volatile( "" : "+t"(f) );
	try {
		(void) *p;
	} catch( SegFault ) {
		f += 1;
	}

	fprintf( stderr, "%s\n", f == 1 ? "ok" : "FAIL" );
}

[[gnu::noinline]]
int foo( unsigned n );

[[gnu::noinline]]
int bar( unsigned n ) {
	if( n > 0 )
		return foo( n-1 );
	test_segv();
	return 1;
}

[[gnu::noinline]]
int foo( unsigned n ) {
	bar( n );
	return 0;
}

int main()
{
	std::set_terminate( abort_backtrace );

	struct sigaction act {};

	fprintf( stderr, "via return (no siginfo)... " );
	act.sa_handler = usr1_handler;
	act.sa_flags = SA_RESETHAND;
	sigaction( SIGUSR1, &act, NULL );
	raise( SIGUSR1 );
	printf( "ok\n" );

	fprintf( stderr, "via return (with siginfo)... " );
	act.sa_handler = usr1_handler;
	act.sa_flags = SA_SIGINFO | SA_RESETHAND;
	sigaction( SIGUSR1, &act, NULL );
	raise( SIGUSR1 );
	printf( "ok\n" );

	fprintf( stderr, "via exception (no siginfo)... " );
	act.sa_handler = segv_handler;
	act.sa_flags = SA_RESETHAND;
	sigaction( SIGSEGV, &act, NULL );
	test_segv();

	fprintf( stderr, "via exception (with siginfo)... " );
	act.sa_handler = segv_handler;
	act.sa_flags = SA_SIGINFO | SA_RESETHAND;
	sigaction( SIGSEGV, &act, NULL );
	test_segv();

	fprintf( stderr, "\n" );

	fprintf( stderr, "backtrace in segv...\n" );
	act.sa_handler = backtrace_handler;
	sigaction( SIGSEGV, &act, NULL );
	foo( 3 );

	fprintf( stderr, "\n" );
	fprintf( stderr, "uncaught exception:\n" );
	throw IntendedUncaughtException {};

	return 0;
}

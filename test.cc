#include <asm/unistd.h>
#include <signal.h>
#include <stdio.h>

static void usr1_handler( int )
{
}


struct SegFault {};

static void segv_handler( int )
{
	register float f asm("s0") = 42;
	asm volatile( "" :: "t"(f) );

	throw SegFault {};
}

int main()
{
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

	int volatile *p = NULL;
	asm volatile( "" : "+r"(p) );

	fprintf( stderr, "via exception (no siginfo)... " );
	act.sa_handler = segv_handler;
	act.sa_flags = SA_RESETHAND;
	sigaction( SIGSEGV, &act, NULL );
	register float f asm("s0") = 0;
	asm volatile( "" : "+t"(f) );
	try {
		(void) *p;
	} catch( SegFault ) {
	}
	fprintf( stderr, "ok (%f)\n", f );

	fprintf( stderr, "via exception (with siginfo)... " );
	act.sa_handler = segv_handler;
	act.sa_flags = SA_SIGINFO | SA_RESETHAND;
	sigaction( SIGSEGV, &act, NULL );
	try {
		(void) *p;
	} catch( SegFault ) {
	}
	fprintf( stderr, "ok\n" );

	return 0;
}

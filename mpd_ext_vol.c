//#define DEBUG 1
#define RED "\x1b[31;01m"
#define DARKRED "\x1b[31;06m"
#define RESET "\x1b[0m"
#define GREEN "\x1b[32;06m"
#define YELLOW "\x1b[33;06m"
#define PORT 0xe010
#define DOWN 0x02
#define UP 0x01
#define STOP 0x00
#define SPINTIME 10
#define POLLTIME 100000
#define SOCKETWAITINGTIME 500000
#define CONNECTATTEMPTS 5

#include <mpd/client.h>
#include <mpd/status.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#ifndef DEBUG
#include <sys/io.h>
#endif

struct mpd_connection *conn;


void* spin_worker( void* argument) {
// direction might be 0x01 -- up, 0x02 -- down, 0x00 -- stop spin

	int direction;
	static bool is_spinning = false, keep_spinning = false, manual_stop = false;
	static int old_direction;
	direction = *(int*)argument;
	if ( is_spinning ) {
#ifdef DEBUG
		printf( YELLOW"Spinning was ON"RESET"\n" );
#endif
		if( old_direction != direction ) {
			if( !manual_stop ) {
				manual_stop = true;
				old_direction = direction;
#ifndef DEBUG
				outb( STOP, PORT );
#else
				printf( RED"Clock"RESET"\n" );
#endif
			} else goto jump;
		} else {
			keep_spinning = true;
			if( manual_stop ) {
				goto avoid_double;
jump:				keep_spinning = true;
avoid_double:			manual_stop = false;
				old_direction = direction;
#ifndef DEBUG
				outb( direction, PORT );
#else
				printf( RED"Click %d"RESET"\n", direction );
#endif
			}
		}
	} else {
#ifdef DEBUG
		printf( YELLOW"Spinning was OFF"RESET"\n" );
#endif
		is_spinning = true;
		old_direction = direction;
#ifndef DEBUG
		outb( direction, PORT );
#else
		printf( RED"Click %d"RESET"\n", direction );
#endif
		do {
			keep_spinning = false;
			sleep( SPINTIME );
		} while( keep_spinning );
		if( !manual_stop ) {
#ifndef DEBUG
			outb( STOP, PORT );
#else
			printf( RED"Clock"RESET"\n" );
#endif
		}
		is_spinning = false;
		manual_stop = false;
	}
	pthread_exit( NULL );
}

void call_spin( int direction ) {
	pthread_t thread;
	pthread_create( &thread, NULL, spin_worker, &direction );
}

void handle_signal( int signum ) {
#ifdef DEBUG
	printf( DARKRED"I've been took by signal"RED" %03i\n"RESET, signum );
#endif
	mpd_connection_free( conn );
#ifndef DEBUG
	outb( STOP, PORT );
#else
	printf( RED"Clock"RESET"\n" );
	printf( YELLOW"Confirm smooth exit"RESET"\n" );
#endif
}

static int handle_error( struct mpd_connection *c ) {
	assert( mpd_connection_get_error( c ) != MPD_ERROR_SUCCESS );
	fprintf( stderr, "%s\n", mpd_connection_get_error_message( c ) );
	mpd_connection_free( c );
#ifndef DEBUG
	outb( STOP, PORT );
#else
	printf( RED"Clock"RESET"\n" );
	printf( YELLOW"Confirm smooth exit"RESET"\n" );
#endif
	return EXIT_FAILURE;
}

int main( int argc, char **argv ) {
	static int old_volume = 0, current_volume = 0, att_counter = 0, mpd_port = 0;
	bool connect_success = false;
	char *mpd_host = NULL;
#ifndef DEBUG
	if( ioperm( PORT, 1, 1 ) ) {
		perror( "ioperm(): LPT port access" );
		return EXIT_FAILURE;
	}
	outb( STOP, PORT );
#else
	printf( RED"Clock"RESET"\n" );
#endif
	if( argc > 1 ) mpd_host = argv[1];
	if( argc > 2 ) mpd_port = atoi( argv[2] );
	do {
		att_counter++;
		usleep( SOCKETWAITINGTIME );
		conn = mpd_connection_new( mpd_host, mpd_port, 1000 );
#ifdef DEBUG
		printf( RED"Connect_attempt %d"RESET"\n", att_counter );
#endif
		connect_success = mpd_connection_get_error( conn ) == MPD_ERROR_SUCCESS;
		if( !connect_success && ( att_counter == CONNECTATTEMPTS ) ) return handle_error( conn );
	} while( !connect_success );

	if( signal( SIGTERM, handle_signal ) == SIG_IGN ) signal( SIGTERM, SIG_IGN );
	if( signal( SIGKILL, handle_signal ) == SIG_IGN ) signal( SIGKILL, SIG_IGN );
	if( signal(  SIGINT, handle_signal ) == SIG_IGN ) signal( SIGINT, SIG_IGN );

	struct mpd_status * status;
	do {
		mpd_send_status( conn );
		status = mpd_recv_status( conn );
		if( status == NULL ) return handle_error( conn );
		current_volume = mpd_status_get_volume( status );
		if( old_volume > 0 && ( old_volume != current_volume ) ) {
#ifdef DEBUG
			printf( GREEN"Volume changed to:"RESET" %03i%%\n", current_volume );
#endif
			if( old_volume < current_volume ) {
				call_spin( UP );
			} else {
				call_spin( DOWN );
			}
		}
		old_volume = current_volume;
		mpd_status_free( status );
	} while( !usleep( POLLTIME ) );
}
/**
 * @file tnk_sniff.c
 * @author Benjamin Ming Yang @ National Taiwan University (b98204032@gmail.com)
 * @brief tnk_sniff is a quick utility to sniff & display all the trace out of a tank player tank.
 * @date 2025-05-07
 *
 * @copyright Copyright (c) 2025
 *
 */

/* */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
/* */
#include <scan.h>
#include <progbar.h>

/* */
#define PROG_NAME       "tnk_sniff"
#define VERSION         "1.0.0 - 2025-05-07"
#define AUTHOR          "Benjamin Ming Yang"
/* */
#define MAX_SCNL_CODE_LEN  8
#define DEF_WILDCARD_STR   "wild"
#define TIMESTAMP_FORMAT   "%04d/%02d/%02d_%02d:%02d:%05.2f"
/* */
static bool  accept_tb_cond( const TRACE2_HEADER *, const void * );
static char *timestamp_gen( char *, const double );
static void  print_trace_data( const TRACE2_HEADER * );
static int   proc_argv( int, char *[] );
static void  usage( void );

/* */
static bool  DataFlag    = false;
static char *InputTank   = NULL;
static char *OutputTank  = NULL;
static char *ExtractSta  = NULL;
static char *ExtractComp = NULL;
static char *ExtractNet  = NULL;
static char *ExtractLoc  = NULL;

/**
 * @brief
 *
 * @param argc
 * @param argv
 * @return int
 */
int main( int argc, char *argv[] )
{
	int         ifd;           /* file of waveform data to read from   */
	struct stat fs;
	uint8_t    *tankstart;
	uint8_t    *tankend;
	TB_INFO    *tb_infos = NULL;
	int         num_tb;

	TRACE2_HEADER *trh2 = NULL;
	char           stime[32] = { 0 };
	char           etime[32] = { 0 };

	struct timespec tt1, tt2;  /* Nanosecond Timer */

/* */
	if ( proc_argv( argc, argv ) ) {
		usage();
		return -1;
	}
/* Nanosecond Timer */
	timespec_get(&tt1, TIME_UTC);
/* Open a waveform files */
	if ( (ifd = open(InputTank, O_RDONLY, 0)) < 0 ) {
		fprintf(stderr, "%s Can not open tankfile <%s>!\n", progbar_now(), InputTank);
		return -1;
	}
/* */
	fstat(ifd, &fs);
	fprintf(stderr, "%s Open the tankfile <%s>, size is %ld bytes.\n", progbar_now(), InputTank, (size_t)fs.st_size);
	fprintf(stderr, "%s Mapping the tankfile <%s> into memory...\n", progbar_now(), InputTank);
	tankstart = mmap(NULL, (size_t)fs.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, ifd, 0);
	tankend   = tankstart + (size_t)fs.st_size;
/* Now lets get down to business and cut the data out of the tank */
	if ( scan_tb( &tb_infos, &num_tb, tankstart, tankend, accept_tb_cond, NULL ) <= 0 ) {
		fprintf(stderr, "%s Can not mark the tracebuf from tankfile <%s>.\n", progbar_now(), InputTank);
		return -1;
	}
/* */
	progbar_init( num_tb + 1 );
	fprintf(stderr, "%s Estimation complete, total %d traces.\n", progbar_now(), num_tb);
/* Write chronological multiplexed output file */
	for ( register int i = 0; i < num_tb; i++ ) {
		trh2 = (TRACE2_HEADER *)(tankstart + tb_infos[i].offset);
	/* Then, simulate sniffwave output */
		timestamp_gen( stime, trh2->starttime );
		timestamp_gen( etime, trh2->endtime );
		fprintf(
			stdout, "%s.%s.%s.%s (%X %X) ",
			trh2->sta, trh2->chan, trh2->net, trh2->loc, trh2->version[0], trh2->version[1]
		);
	/* */
		fprintf(
			stdout,
	/* More decimal places for slower sample rates */
			trh2->samprate < 1.0 ? "%d %c%c %4d %6.4f %s (%.4f) %s (%.4f) %ld bytes\n" : "%d %c%c %4d %.1f %s (%.4f) %s (%.4f) %ld bytes ",
			trh2->pinno, tb_infos[i].orig_byte_order, trh2->datatype[1], trh2->nsamp, trh2->samprate,
			stime, trh2->starttime, etime, trh2->endtime, tb_infos[i].size
		);
		fprintf(stdout, "\n");
	/* */
		if ( DataFlag )
			print_trace_data( trh2 );
	/* */
		progbar_inc();
	}

/* */
	munmap(tankstart, (size_t)fs.st_size);
	close(ifd);
/* */
	if ( tb_infos )
		free(tb_infos);
	progbar_inc();
/* Nanosecond Timer */
	timespec_get(&tt2, TIME_UTC);
	fprintf(
		stderr, "%s Sniffing complete! Total processing time: %.3f sec.\n", progbar_now(),
		(float)(tt2.tv_sec - tt1.tv_sec) + (float)(tt2.tv_nsec - tt1.tv_nsec)* 1e-9
	);

	return 0;
}

/**
 * @brief
 *
 * @param trh2
 * @param arg
 * @return true
 * @return false
 */
static bool accept_tb_cond( const TRACE2_HEADER *trh2, const void *arg )
{
/* If the packet's endtime is BEFORE StartEpoch or starttime is AFTER EndEpoch, we will drop it */
	if (
		(!ExtractSta || !strcmp(ExtractSta, trh2->sta)) &&
		(!ExtractComp || !strcmp(ExtractComp, trh2->chan)) &&
		(!ExtractNet || !strcmp(ExtractNet, trh2->net)) &&
		(!ExtractLoc || !strcmp(ExtractLoc, trh2->loc))
	) {
		return true;
	}

	return false;
}

/**
 * @brief
 *
 * @param buffer
 * @param timestamp
 * @return char*
 */
char *timestamp_gen( char *buffer, const double timestamp )
{
	struct tm    sptime;
	const time_t _timestamp = (time_t)timestamp;
	double sec_f = timestamp - (double)_timestamp;

/* Checking for buffer size */
	if ( buffer == NULL )
		return buffer;
/* */
	gmtime_r(&_timestamp, &sptime);
	sec_f += (double)sptime.tm_sec;
	sprintf(
		buffer, TIMESTAMP_FORMAT,
		sptime.tm_year + 1900, sptime.tm_mon + 1, sptime.tm_mday,
		sptime.tm_hour, sptime.tm_min, sec_f
	);

	return buffer;
}

#define PRINT_INT_TRACE_DATA(__INPUT, __SEQ) ({ \
			fprintf(stdout, "%6d ", (__INPUT)); \
			if ( (__SEQ) % 10 == 9 ) \
				fprintf(stdout, "\n"); \
		})

#define PRINT_FLOAT_TRACE_DATA(__INPUT, __SEQ) ({ \
			fprintf(stdout, "%6.4lf ", (__INPUT)); \
			if ( (__SEQ) % 10 == 9 ) \
				fprintf(stdout, "\n"); \
		})

#define STATICS_TRACE_DATA(__INPUT, __MAX, __MIN, __AVG) ({ \
			if ( (__INPUT) > (__MAX) ) \
				(__MAX) = (__INPUT); \
			if ( (__INPUT) < (__MIN) ) \
				(__MIN) = (__INPUT); \
			(__AVG) += (__INPUT); \
		})

/**
 * @brief
 *
 * @param trh2
 */
static void print_trace_data( const TRACE2_HEADER *trh2 )
{
/* */
	int16_t *sdata = (int16_t *)(trh2 + 1);
	int32_t *ldata = (int32_t *)(trh2 + 1);
	float   *fdata = (float *)(trh2 + 1);
	double  *ddata = (double *)(trh2 + 1);
/* */
	int    max  = 0;
	int    min  = 0;
	double dmax = 0.0;
	double dmin = 0.0;
	double avg  = 0.0;
/* */
	bool   floating = false;

	if ( !strcmp(trh2->datatype, "s2") || !strcmp(trh2->datatype, "i2") ) {
		max = min = *sdata;
		for ( register int i = 0; i < trh2->nsamp; i++, sdata++ ) {
			PRINT_INT_TRACE_DATA( *sdata, i );
			STATICS_TRACE_DATA( *sdata, max, min, avg );
		}
	}
	else if ( !strcmp(trh2->datatype, "s4") || !strcmp(trh2->datatype, "i4") ) {
		max = min = *ldata;
		for ( register int i = 0; i < trh2->nsamp; i++, ldata++ ) {
			PRINT_INT_TRACE_DATA( *ldata, i );
			STATICS_TRACE_DATA( *ldata, max, min, avg );
		}
	}
	else if ( !strcmp(trh2->datatype, "t4") || !strcmp(trh2->datatype, "f4") ) {
		dmax = dmin = *fdata;
		for ( register int i = 0; i < trh2->nsamp; i++, fdata++ ) {
			PRINT_FLOAT_TRACE_DATA( *fdata, i );
			STATICS_TRACE_DATA( *fdata, dmax, dmin, avg );
		}
		floating = true;
	}
	else if ( !strcmp(trh2->datatype, "t8") || !strcmp(trh2->datatype, "f8") ) {
		dmax = dmin = *ddata;
		for ( register int i = 0; i < trh2->nsamp; i++, ddata++ ) {
			PRINT_FLOAT_TRACE_DATA( *ddata, i );
			STATICS_TRACE_DATA( *ddata, dmax, dmin, avg );
		}
		floating = true;
	}
	else {
		fprintf(stdout, "Unknown datatype %s\n", trh2->datatype);
	}
/* */
	avg = avg / trh2->nsamp;
	if ( floating ) {
		fprintf(stdout, "Raw Data statistics max=%lf min=%lf avg=%lf\n", dmax, dmin, avg);
		fprintf(
			stdout, "DC corrected statistics max=%lf min=%lf spread=%lf\n",
			(dmax - avg), (dmin - avg), fabs(dmax - dmin)
		);
	}
	else {
		fprintf(stdout, "Raw Data statistics max=%d min=%d avg=%lf\n", max, min, avg);
		fprintf(
			stdout, "DC corrected statistics max=%lf min=%lf spread=%d\n",
			(double)(max - avg), (double)(min - avg), abs(max - min)
		);
	}
/* */
	fprintf(stdout, "\n");
	fflush(stdout);

	return;
}

/**
 * @brief
 *
 * @param argc
 * @param argv
 * @return int
 */
static int proc_argv( int argc, char *argv[] )
{
/* Parse command line args */
	for ( register int i = 1; i < argc; i++ ) {
	/* check switches */
		if ( !strcmp(argv[i], "-v") ) {
			fprintf(stdout, "%s\n", PROG_NAME);
			fprintf(stdout, "Version: %s\n", VERSION);
			fprintf(stdout, "Author:  %s\n", AUTHOR);
			fprintf(stdout, "Compiled at %s %s\n", __DATE__, __TIME__);
			exit(0);
		}
		else if ( !strcmp(argv[i], "-h") ) {
			usage();
			exit(0);
		}
		else if ( !strcmp(argv[i], "-s") ) {
			if ( strlen(argv[++i]) > MAX_SCNL_CODE_LEN ) {
				fprintf(stderr, "Error: SCNL code length must be less than %d\n", MAX_SCNL_CODE_LEN);
				return -1;
			}
			if ( strcmp(argv[i], DEF_WILDCARD_STR) )
				ExtractSta = argv[i];
		}
		else if ( !strcmp(argv[i], "-c") ) {
			if ( strlen(argv[++i]) > MAX_SCNL_CODE_LEN ) {
				fprintf(stderr, "Error: SCNL code length must be less than %d\n", MAX_SCNL_CODE_LEN);
				return -1;
			}
			if ( strcmp(argv[i], DEF_WILDCARD_STR) )
				ExtractComp = argv[i];
		}
		else if ( !strcmp(argv[i], "-n") ) {
			if ( strlen(argv[++i]) > MAX_SCNL_CODE_LEN ) {
				fprintf(stderr, "Error: SCNL code length must be less than %d\n", MAX_SCNL_CODE_LEN);
				return -1;
			}
			if ( strcmp(argv[i], DEF_WILDCARD_STR) )
				ExtractNet = argv[i];
		}
		else if ( !strcmp(argv[i], "-l") ) {
			if ( strlen(argv[++i]) > MAX_SCNL_CODE_LEN ) {
				fprintf(stderr, "Error: SCNL code length must be less than %d\n", MAX_SCNL_CODE_LEN);
				return -1;
			}
			if ( strcmp(argv[i], DEF_WILDCARD_STR) )
				ExtractLoc = argv[i];
		}
		else if ( !strcmp(argv[i], "-y") ) {
			DataFlag = true;
		}
		else if ( i == argc - 1 ) {
			InputTank = argv[i];
			OutputTank = NULL;
		}
		else if ( i == argc - 2 ) {
			InputTank = argv[i++];
			OutputTank = argv[i];
		/* Just in case */
			break;
		}
		else {
			fprintf(stderr, "Unknown option: %s\n\n", argv[i]);
			return -1;
		}
	} /* end of command line args for loop */

/* check command line args */
	if ( !InputTank ) {
		fprintf(stderr, "Error, an input tank name must be provided\n");
		return -2;
	}

	return 0;
}

/**
 * @brief
 *
 */
static void usage( void )
{
	fprintf(stdout, "\n%s\n", PROG_NAME);
	fprintf(stdout, "Version: %s\n", VERSION);
	fprintf(stdout, "Author:  %s\n", AUTHOR);
	fprintf(stdout, "Compiled at %s %s\n", __DATE__, __TIME__);
	fprintf(stdout, "***************************\n");
	fprintf(stdout, "Usage: %s [options] <input tankfile>\n\n", PROG_NAME);
	fprintf(stdout,
		"*** Options ***\n"
		" All default values for -s, -c, -n and -l are wildcard (wild)\n"
		" -s station_code  Specify the extract station code, max length is 8\n"
		" -c channel_code  Specify the extract channel code, max length is 8\n"
		" -n network_code  Specify the extract network code, max length is 8\n"
		" -l location_code Specify the extract location code, max length is 8\n"
		" -y               Print out the full data contained in the packet\n"
		" -h               Show this usage message\n"
		" -v               Report program version\n"
		"\n"
		"This program will sniff & display the trace data from the input TANK file by order.\n"
		"\n"
	);
}

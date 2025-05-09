/**
 * @file tnk_extract.c
 * @author Benjamin Ming Yang @ National Taiwan University (b98204032@gmail.com)
 * @brief tnk_extract is a quick utility to extract a specified SCNL out of a tank player tank.
 *        The data from the tank can then be used in tankplayer.
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
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
/* */
#include <scan.h>
#include <progbar.h>

/* */
#define PROG_NAME       "tnk_extract"
#define VERSION         "1.0.0 - 2025-05-07"
#define AUTHOR          "Benjamin Ming Yang"
/* */
#define MAX_SCNL_CODE_LEN  8
#define DEF_WILDCARD_STR   "wild"

/* */
static bool accept_tb_cond( const TRACE2_HEADER *, const void * );
static int  proc_argv( int, char *[] );
static void usage( void );

/* */
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
	FILE       *ofp = stdout;  /* file of waveform data to write out   */
	struct stat fs;
	uint8_t    *tankstart;
	uint8_t    *tankend;
	uint8_t    *tankbyte;
	TB_INFO    *tb_infos = NULL;
	int         num_tb;

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
	progbar_init( num_tb + 2 );
	fprintf(stderr, "%s Estimation complete, total %d traces.\n", progbar_now(), num_tb);
/* If user chose to output the result to local file, then open the file descript to write */
	if ( OutputTank && (ofp = fopen(OutputTank, "wb")) == (FILE *)NULL ) {
		fprintf(stderr, "%s ERROR!! Can't open tankfile <%s> for output! Exiting!\n", progbar_now(), OutputTank);
		return -1;
	}
	progbar_inc();
/* Write chronological multiplexed output file */
	for ( register int i = 0; i < num_tb; i++ ) {
		tankbyte = tankstart + tb_infos[i].offset;
		if ( fwrite(tankbyte, tb_infos[i].size, 1, ofp) != 1 ) {
			fprintf(stderr, "%s Error writing %ld bytes to output.\n", progbar_now(), tb_infos[i].size);
		/* Remove the error file */
			if ( OutputTank )
				remove(OutputTank);
			break;
		}
		progbar_inc();
	}

/* */
	munmap(tankstart, (size_t)fs.st_size);
	close(ifd);
/* */
	if ( ofp != stdout )
		fclose(ofp);
	if ( tb_infos )
		free(tb_infos);
	progbar_inc();
/* Nanosecond Timer */
	timespec_get(&tt2, TIME_UTC);
	fprintf(
		stderr, "%s Extracting complete! Total processing time: %.3f sec.\n", progbar_now(),
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
	if ( !ExtractSta && !ExtractComp && !ExtractNet && !ExtractLoc ) {
		fprintf(stderr, "Error, at least one of SCNL code should be specified\n");
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
	fprintf(stdout, "Usage: %s [options] <input tankfile> <output tankfile>\n\n", PROG_NAME);
	fprintf(stdout, "       or %s [options] <input tankfile> > <output tankfile>\n\n", PROG_NAME);
	fprintf(stdout,
		"*** Options ***\n"
		" All default values for -s, -c, -n and -l are wildcard (wild)\n"
		" -s station_code  Specify the extract station code, max length is 8\n"
		" -c channel_code  Specify the extract channel code, max length is 8\n"
		" -n network_code  Specify the extract network code, max length is 8\n"
		" -l location_code Specify the extract location code, max length is 8\n"
		" -h               Show this usage message\n"
		" -v               Report program version\n"
		"\n"
		"This program will extract the specified SCNL data from the input TANK file.\n"
		"\n"
	);
}

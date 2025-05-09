/**
 * @file scan.c
 * @author Benjamin Ming Yang @ Department of Geoscience, National Taiwan University (b98204032@gmail.com)
 * @brief Scaning function for the trace buf data.
 * @date 2025-05-08
 *
 * @copyright Copyright (c) 2025
 *
 */

/**
 * @name
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @name
 *
 */
#include <trace_buf.h>
#include <swap.h>
#include <scan.h>

/**
 * @brief
 *
 */
#define MAX_NUM_TBUF 524288

/**
 * @brief
 *
 * @param tb_infos
 * @param num_tb_info
 * @param tankstart
 * @param tankend
 * @param accept_cond
 * @param arg
 * @return int
 */
int scan_tb(
	TB_INFO **tb_infos, int *num_tb_info, void * const tankstart, void * const tankend,
	ACCEPT_TB_COND accept_cond, const void *arg
) {
	int            _num_tb_info = 0;                    /* Total # msgs read of one trace                     */
	int            max_tb_info  = MAX_NUM_TBUF;         /* Max # msgs read of one trace                       */
	char           o_byte_order = ' ';                  /* The original byte order of the trace               */
	size_t         skipbyte     = 0;                    /* total # bytes skipped from last successed fetching */
	TRACE2_HEADER *trh2         = NULL;                 /* tracebuf message read from file                    */
	uint8_t       *tankbyte     = (uint8_t *)tankstart;
	TB_INFO       *_tb_infos    = (TB_INFO *)calloc(max_tb_info, sizeof(TB_INFO));

/* */
	if ( !_tb_infos || !tankstart || !tankend ) {
		if ( _tb_infos )
			free(_tb_infos);
	}
/* Read thru mapping memory reading headers; gather info about all tracebuf messages */
	do {
		trh2 = (TRACE2_HEADER *)tankbyte;
	/* Swap the byte order into local order, and check the validity of this tracebuf */
		if ( swap_wavemsg2_makelocal( trh2, &o_byte_order ) < 0 ) {
			if ( ++tankbyte < (uint8_t *)tankend ) {
				skipbyte++;
				continue;
			}
			else {
				break;
			}
		}
		else if ( skipbyte ) {
			fprintf(
				stderr, "%s: Shift total %ld bytes, found the next correct tracebuf for <%s.%s.%s.%s> %13.2f+%4.2f!\n",
				__func__, skipbyte, trh2->sta, trh2->chan, trh2->net, trh2->loc, trh2->starttime, trh2->endtime-trh2->starttime
			);
			skipbyte = 0;
		}

	/* Fill in the pertinent info */
		_tb_infos[_num_tb_info].offset          = tankbyte - (uint8_t *)tankstart;
		_tb_infos[_num_tb_info].size            = (atoi(&trh2->datatype[1]) * trh2->nsamp) + sizeof(TRACE2_HEADER);
		_tb_infos[_num_tb_info].time            = trh2->endtime;
		_tb_infos[_num_tb_info].orig_byte_order = o_byte_order;
	/* Keep track the total bytes we have read, and move the pointer to the next header */
		tankbyte += _tb_infos[_num_tb_info].size;
	/* Skip those do not fit the condition */
		if ( accept_cond && !accept_cond( trh2, arg ) )
			continue;
	/* Skip over data samples */
		if ( _tb_infos[_num_tb_info].size > MAX_TRACEBUF_SIZ ) {
			fprintf(
				stderr, "%s: *** tracebuf[%ld bytes] too large, maximum is %d bytes ***\n",
				__func__, _tb_infos[_num_tb_info].size, MAX_TRACEBUF_SIZ
			);
			continue;
		}
	/* Now, really store this packet */
		_num_tb_info++;
	/* Allocate more space if necessary */
		if ( _num_tb_info >= max_tb_info ) {
			max_tb_info <<= 1;
			if ( (_tb_infos = realloc(_tb_infos, max_tb_info * sizeof(TB_INFO))) == NULL ) {
				fprintf(
					stderr, "%s: *** Could not realloc list to %ld bytes ***\n",
					__func__, max_tb_info * sizeof(TB_INFO)
				);
				return -2;
			}
		}
	} while ( tankbyte < (uint8_t *)tankend );

/* */
	if ( _num_tb_info > 0 ) {
		*num_tb_info = _num_tb_info;
		*tb_infos    = _tb_infos;
	}
	else if ( _tb_infos ) {
		free(_tb_infos);
	}

	return _num_tb_info;
}

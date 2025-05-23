/**
 * @file swap.c
 * @author Benjamin Ming Yang @ Department of Geoscience, National Taiwan University (b98204032@gmail.com)
 * @brief Handy routines for swapping things. Fixes wave message into local byte order,
 *        based on your local byte order
 * @date 2025-05-07
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
#include <string.h>
#include <stdint.h>
/**
 * @name
 *
 */
#include <trace_buf.h>
#include <swap.h>

/**
 * @name
 *
 */
static int mklocal_wavemsg_ver( TRACE2X_HEADER *, char, char * );
static int probe_host_byteorder( void );

/**
 * @name
 *
 */
static int HostByteOrder = BYTE_ORDER_UNDEFINE;

/**
 * @brief Byte swap 2-byte unsigned integer
 *
 * @param data
 */
void swap_uint16( void *data )
{
	uint8_t temp;
	union {
		uint8_t c[2];
	} dat;

	memcpy(&dat, data, 2);
	temp     = dat.c[0];
	dat.c[0] = dat.c[1];
	dat.c[1] = temp;
	memcpy(data, &dat, 2);

	return;
}

/**
 * @brief Byte swap 4-byte unsigned integer
 *
 * @param data
 */
void swap_uint32( void *data )
{
	uint8_t temp;
	union {
		uint8_t c[4];
	} dat;

	memcpy(&dat, data, 4);
	temp     = dat.c[0];
	dat.c[0] = dat.c[3];
	dat.c[3] = temp;
	temp     = dat.c[1];
	dat.c[1] = dat.c[2];
	dat.c[2] = temp;
	memcpy(data, &dat, 4);

	return;
}

/**
 * @brief Byte swap 8-byte unsigned integer
 *
 * @param data
 */
void swap_uint64( void *data )
{
	uint8_t temp;
	union {
		uint8_t c[8];
	} dat;

	memcpy(&dat, data, 8);
	temp     = dat.c[0];
	dat.c[0] = dat.c[7];
	dat.c[7] = temp;
	temp     = dat.c[1];
	dat.c[1] = dat.c[6];
	dat.c[6] = temp;
	temp     = dat.c[2];
	dat.c[2] = dat.c[5];
	dat.c[5] = temp;
	temp     = dat.c[3];
	dat.c[3] = dat.c[4];
	dat.c[4] = temp;
	memcpy(data, &dat, 8);

	return;
}

/**
 * @brief Byte-swap a universal TYPE_TRACEBUF message in place. Changes the 'datatype' field in the message header
 *
 * @param wvmsg
 * @return int
 * @retval -1 if unknown data type.
 * @retval -2 if checksumish calculation of header fails.
 * @retval 0 Elsewise (SUCCESS).
 */
int swap_wavemsg_makelocal( TRACE_HEADER *wvmsg, char *orig_byte_order )
{
	return mklocal_wavemsg_ver( (TRACE2X_HEADER *)wvmsg, '1', orig_byte_order );
}

/**
 * @brief Byte-swap a universal TYPE_TRACEBUF2 message in place. Changes the 'datatype' field in the message header
 *
 * @param wvmsg
 * @return int
 * @retval -1 if unknown data type.
 * @retval -2 if checksumish calculation of header fails.
 * @retval 0 Elsewise (SUCCESS).
 */
int swap_wavemsg2_makelocal( TRACE2_HEADER *wvmsg, char *orig_byte_order )
{
	return mklocal_wavemsg_ver( (TRACE2X_HEADER *)wvmsg, wvmsg->version[0], orig_byte_order );
}

/**
 * @brief Byte-swap a universal TYPE_TRACEBUF2X message in place. Changes the 'datatype' field in the message header
 *
 * @param wvmsg
 * @return int
 * @retval -1 if unknown data type.
 * @retval -2 if checksumish calculation of header fails.
 * @retval 0 Elsewise (SUCCESS).
 */
int swap_wavemsg2x_makelocal( TRACE2X_HEADER *wvmsg, char *orig_byte_order )
{
	return mklocal_wavemsg_ver( wvmsg, wvmsg->version[0], orig_byte_order );
}

/**
 * @brief
 *
 * @remark CSS datatype codes
 *         t4  SUN IEEE single precision real
 *         t8  SUN IEEE double precision real
 *         s4  SUN IEEE integer
 *         s2  SUN IEEE short integer
 *         f4  VAX/Intel IEEE single precision real
 *         f8  VAX/Intel IEEE double precision real
 *         i4  VAX/Intel IEEE integer
 *         i2  VAX/Intel IEEE short integer
 *         g2  NORESS gain-ranged
 *
 * @remark 2002/03/18 bt DK: Perform a CheckSumish kind of calculation on the header
 *         ensure that the tracebuf ends within 5 samples of the given endtime.
 *
 * @param wvmsg
 * @param version
 * @param orig_byte_order Optional parameter
 * @return int
 */
static int mklocal_wavemsg_ver( TRACE2X_HEADER *wvmsg, char version, char *orig_byte_order )
{
	static const int tracedata_max_size = MAX_TRACEBUF_SIZ - sizeof(TRACE2X_HEADER);
/* */
	int      data_size  = 0;   /* flag telling us how many bytes in the data */
	char     byte_order = ' ';
	char     loc_ibyte_order;
	char     loc_fbyte_order;
	char     ops_ibyte_order;
	char     ops_fbyte_order;
/* */
	int32_t *int_ptr;
	int16_t *short_ptr;
	float   *float_ptr;
	double  *double_ptr;

	int    nsamp;
	double samprate;
	double starttime;
	double endtime;

	double _endtime;
	double fudge;

/* */
	switch ( HostByteOrder ) {
	case BYTE_ORDER_UNDEFINE:
		HostByteOrder = probe_host_byteorder();
	default:
	/* */
		if ( HostByteOrder == BYTE_ORDER_BIG_ENDIAN ) {
			loc_ibyte_order = 's';
			loc_fbyte_order = 't';
			ops_ibyte_order = 'i';
			ops_fbyte_order = 'f';
		}
		else if ( HostByteOrder == BYTE_ORDER_LITTLE_ENDIAN ) {
			loc_ibyte_order = 'i';
			loc_fbyte_order = 'f';
			ops_ibyte_order = 's';
			ops_fbyte_order = 't';
		}
		else {
			return -1;
		}
		break;
	}

/* See what sort of data it carries */
	if ( wvmsg->datatype[0] == 's' && (wvmsg->datatype[1] == '2' || wvmsg->datatype[1] == '4') )
		byte_order = 's';
	else if ( wvmsg->datatype[0] == 'i' && (wvmsg->datatype[1] == '2' || wvmsg->datatype[1] == '4') )
		byte_order = 'i';
	else if ( wvmsg->datatype[0] == 't' && (wvmsg->datatype[1] == '4' || wvmsg->datatype[1] == '8') )
		byte_order = 't';
	else if ( wvmsg->datatype[0] == 'f' && (wvmsg->datatype[1] == '4' || wvmsg->datatype[1] == '8') )
		byte_order = 'f';
	else
	/* We don't know this message type*/
		return -1;
/* */
	data_size = wvmsg->datatype[1] - '0';

/* SWAP the header (if neccessary) */
	if ( byte_order != loc_ibyte_order && byte_order != loc_fbyte_order ) {
	/* swap the header */
		swap_int( &(wvmsg->pinno) );
		swap_int( &(wvmsg->nsamp) );
		swap_double( &(wvmsg->starttime) );
		swap_double( &(wvmsg->endtime)   );
		swap_double( &(wvmsg->samprate)  );
		if ( version == TRACE2_VERSION0 ) {
			switch ( wvmsg->version[1] ) {
			case TRACE2_VERSION11:
				swap_float( &(wvmsg->x.v21.conversion_factor) );
				break;
			}
		}
	}
/* */
	if ( wvmsg->nsamp > (tracedata_max_size / data_size) ) {
		fprintf(
			stderr,"%s: packet from %s.%s.%s.%s has bad number of samples=%d datatype=%s\n",
			__func__, wvmsg->sta, wvmsg->chan, wvmsg->net, wvmsg->loc, wvmsg->nsamp, wvmsg->datatype
		);
		return -1;
	}

/* Moved nsamp memcpy to here to avoid byte-alignment with next statement */
	memcpy(&nsamp,     &wvmsg->nsamp,     sizeof(int)   );
	memcpy(&samprate,  &wvmsg->samprate,  sizeof(double));
	memcpy(&starttime, &wvmsg->starttime, sizeof(double));
	memcpy(&endtime,   &wvmsg->endtime,   sizeof(double));
/* */
	_endtime = starttime + ((nsamp - 1) / samprate);
	fudge    = 5.0 / samprate;

/*
 * This is supposed to be a simple sanity check to ensure that the
 * endtime is within 5 samples of where it should be. We're not
 * trying to be judgemental here, we're just trying to ensure that
 * we protect ourselves from complete garbage, so that we don't segfault
 * when allocating samples based on a bad nsamp
 */
	if ( endtime < (_endtime - fudge) || endtime > (_endtime + fudge) ) {
		fprintf(
			stderr,"%s: packet from %s.%s.%s.%s has inconsistent header values!\n",
			__func__, wvmsg->sta, wvmsg->chan, wvmsg->net, wvmsg->loc
		);
		fprintf(stderr,"%s: header.starttime  : %.4lf\n", __func__, starttime);
		fprintf(stderr,"%s: header.samplerate : %.1lf\n", __func__, samprate );
		fprintf(stderr,"%s: header.nsample    : %d\n", __func__,    nsamp    );
		fprintf(stderr,"%s: header.endtime    : %.4lf\n", __func__, endtime  );
		fprintf(stderr,"%s: computed.endtime  : %.4lf\n", __func__, _endtime );
		fprintf(stderr,"%s: header.endtime is not within 5 sample intervals of computed.endtime!\n", __func__);
		return -2;
	}

/* SWAP the data (if neccessary) */
	if ( byte_order == ops_ibyte_order ) {
	/* Swap the data */
		int_ptr   = (int32_t *)(wvmsg + 1);
		short_ptr = (int16_t *)(wvmsg + 1);
		for ( register int i = 0; i < nsamp; i++, int_ptr++, short_ptr++ ) {
			if ( data_size == 2 )
				swap_int16( short_ptr );
			else if ( data_size == 4 )
				swap_int32( int_ptr );
		}
	/* Re-write the data type field in the message */
		wvmsg->datatype[0] = loc_ibyte_order;
		if ( data_size == 2 )
			wvmsg->datatype[1] = '2';
		else if ( data_size == 4 )
			wvmsg->datatype[1] = '4';
	}
	else if ( byte_order == ops_fbyte_order ) {
	/* Swap the data */
		float_ptr  = (float *)(wvmsg + 1);
		double_ptr = (double *)(wvmsg + 1);
		for ( register int i = 0; i < nsamp; i++, float_ptr++, double_ptr++ ) {
			if ( data_size == 4 )
				swap_float( float_ptr );
			else if ( data_size == 8 )
				swap_double( double_ptr );
		}
	/* Re-write the data type field in the message */
		wvmsg->datatype[0] = loc_fbyte_order;
		if ( data_size == 4 )
			wvmsg->datatype[1] = '4';
		else if ( data_size == 8 )
			wvmsg->datatype[1] = '8';
	}
/* Assign the original byte order to the optional parameter, if the caller want to know this information */
	if ( orig_byte_order )
		*orig_byte_order = byte_order;

	return 0;
}

/**
 * @brief
 *
 * @return int
 */
static int probe_host_byteorder( void )
{
	const uint16_t probe = 256;

	return *(const uint8_t *)&probe ? BYTE_ORDER_BIG_ENDIAN : BYTE_ORDER_LITTLE_ENDIAN;
}

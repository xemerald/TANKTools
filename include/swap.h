/**
 * @file swap.h
 * @author Benjamin Ming Yang @ Department of Geoscience, National Taiwan University (b98204032@gmail.com)
 * @brief Include file for swap.c: handy routines for swapping things.
 *        Fixes wave message into local byte order, based on your local byte order
 * @date 2025-05-07
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
/**
 * @name
 *
 */
#include <trace_buf.h>

/**
 * @name
 *
 */
#define BYTE_ORDER_UNDEFINE      -1
#define BYTE_ORDER_LITTLE_ENDIAN  0
#define BYTE_ORDER_BIG_ENDIAN     1

/**
 * @name
 *
 */
void swap_uint16( void * );
#define swap_int16( data ) swap_uint16( data )
#define swap_short( data ) swap_uint16( data )
void swap_uint32( void * );
#define swap_int32( data ) swap_uint32( data )
#define swap_int( data ) swap_uint32( data )
#define swap_float( data ) swap_uint32( data )
void swap_uint64( void * );
#define swap_int64( data ) swap_uint64( data )
#define swap_double( data ) swap_uint64( data )

/**
 * @name
 *
 */
int swap_wavemsg_makelocal( TRACE_HEADER *, char * );
int swap_wavemsg2_makelocal( TRACE2_HEADER *, char * );
int swap_wavemsg2x_makelocal( TRACE2X_HEADER *, char * );

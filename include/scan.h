/**
 * @file scan.h
 * @author Benjamin Ming Yang @ Department of Geoscience, National Taiwan University (b98204032@gmail.com)
 * @brief
 * @date 2024-02-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once
/**
 * @name
 *
 */
#include <stdbool.h>
/**
 * @name
 *
 */
#include <trace_buf.h>

/**
 * @brief
 *
 */
typedef struct {
	size_t offset;          /* Offset in bytes from beginning of input file  */
	size_t size;            /* Length in bytes of this TRACEBUF2 message     */
	double time;            /* A time from the header of this TRACEBUF2 msg  */
	char   orig_byte_order; /* The original byte order of this TRACEBUF2 msg */
	char   padding[7];      /* The padding for 8-bytes alignments            */
} TB_INFO;

/**
 * @brief
 *
 */
typedef bool (*ACCEPT_TB_COND)( const TRACE2_HEADER *, const void * );

/**
 * @name
 *
 */
int scan_tb( TB_INFO **, int *, void * const, void * const, ACCEPT_TB_COND, const void * );

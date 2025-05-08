/**
 * @file scan.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2024-02-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

/* */
#include <stdbool.h>
/* */
#include <trace_buf.h>

/**
 * @brief
 *
 */
typedef struct {
	size_t offset;    /* Offset in bytes from beginning of input file */
	size_t size;      /* Length in bytes of this TRACEBUF2 message    */
	double time;      /* A time from the header of this TRACEBUF2 msg */
} TB_INFO;

int scan_tb( TB_INFO **, int *, void * const, void * const, bool (*)( const TRACE2_HEADER *, const void * ), const void * );

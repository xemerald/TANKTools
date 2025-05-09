/**
 * @file progbar.h
 * @author Benjamin Ming Yang @ Department of Geoscience, National Taiwan University (b98204032@gmail.com)
 * @brief
 * @date 2025-05-08
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

/**
 * @name Progression bar function
 *
 */
char *progbar_now( void );
int   progbar_init( const int );
int   progbar_inc( void );

//------------------------------------------------------------------------------
//  Higher Order Filter Project
//
//  higher_order_filter.h: helper functions for higher order filter project
//  Copyright (c) 2016 Elliot Patros. All rights reserved.
//------------------------------------------------------------------------------

#ifndef _higher_order_filter_h
#define _higher_order_filter_h

// obligatory Pure Data header (plus any others we need) -----------------------
#include <math.h>   // for tan and some constants
#include <float.h>  // for FLT_EPSILON
#include <string.h> // for memset
#include <stdlib.h> // for *alloc family

// defines ---------------------------------------------------------------------
#define UNUSED_PARAM(expr) do {(void)(expr); } while (0)

// constants -------------------------------------------------------------------
static const t_float default_Q = M_SQRT1_2;
static const t_float default_freq = 1000.f;
static const t_float default_dB = 0.f;
static const t_float min_freq = FLT_EPSILON;
static const t_float max_freq_ratio = 0.5f - FLT_EPSILON;
static const t_float min_Q = FLT_EPSILON;
static const t_float max_Q = 1000.f - FLT_EPSILON;
static const t_float max_order = 65536.f;

// comparisons -----------------------------------------------------------------
static inline
t_float clip_float(const t_float val, const t_float min, const t_float max)
{
    return (val < min) ? min : (fminf(val, max));
}

static inline
t_float clip_freq_ratio(const t_float freq, const t_float sr)
{
    return clip_float(freq / sr, min_freq, sr * max_freq_ratio);
}

static inline
t_float clip_Q(const t_float Q)
{
    return clip_float(Q, min_Q, max_Q);
}

static inline
t_int clip_order(const t_int order)
{
    return lrintf(clip_float(order, 1.f, max_order));
}

// conversions -----------------------------------------------------------------
static inline
t_float dB_to_gain(const t_float dB)
{
    return clip_float(powf(10.f, dB * 0.05f), 0.f, FLT_MAX);
}

static inline
t_float gain_to_dB(const t_float gain)
{
    return clip_float(20.f * log10f(gain), FLT_MIN, FLT_MAX);
}

#endif // _higher_order_filter_h defined

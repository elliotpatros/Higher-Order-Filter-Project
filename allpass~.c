//------------------------------------------------------------------------------
//  Higher Order Filter Project
//
//  allpass~.c: second-order allpass filter
//  Copyright (c) 2016 Elliot Patros. All rights reserved.
//------------------------------------------------------------------------------

// Pd header and constants -----------------------------------------------------
#include "m_pd.h"
#include "higher_order_filter.h"

// pointer to this object's class ----------------------------------------------
static t_class* allpass_class;

// this object's struct --------------------------------------------------------
typedef struct allpass
{
    // instance of this object. must always be first
    t_object object;
    
    // state of each inlet value
    t_float sample;    // first inlet: audio, so not used for control rate
    t_float Q;         // second inlet: filter Q, or width
    t_float freq;      // third inlet: filter cutoff frequency (Hz.)
    
    // state of pd audio
    t_float sr;        // sample rate for filter math (Hz.)
    
    // filter coefficients and delay table
    t_float b_coef[3]; // 'B' coefficients (B0, B1, B2)
    t_float a_coef[2]; // 'A' coefficients (A1, A2)
    t_float f_feed[2]; // feedforward delay
    t_float b_feed[2]; // feedback delay
    t_int   wptr;      // write pointer (for delay tables)
    
} t_allpass;

// _perform --------------------------------------------------------------------
/*
 * called at the start of every block while 'dsp' is on.
 * borrowing from miller's explanation, it's called with a single pointer 'ptr',
 * where ptr[0] is our function's location in the dsp call list. we return a new
 * pointer, which will point to the next dsp function. meanwhile, arguments that
 * are useful for processing audio samples are packed after ptr[0], as specified
 * in the _dsp function.
 */
static t_int* allpass_perform(t_int* ptr)
{
    t_float*    input    = (t_float*)  ptr[1];
    t_float*    output   = (t_float*)  ptr[2];
    const t_int nSamples = (t_int)     ptr[3];
    t_allpass*  x        = (t_allpass*)ptr[4];
    
    for (t_int n = 0; n < nSamples; ++n)
    {
        // get wrapped read pointers and copy input sample
        const t_int rptr1    = x->wptr;
        const t_int rptr0    = 1 - rptr1;
        const t_float sample = input[n];
        
        // make output sample and write to feedback delay table
        x->b_feed[x->wptr] =
        output[n] =
        sample           * x->b_coef[0] +
        x->f_feed[rptr0] * x->b_coef[1] +
        x->f_feed[rptr1] * x->b_coef[2] -
        x->b_feed[rptr0] * x->a_coef[0] -
        x->b_feed[rptr1] * x->a_coef[1];
        
        // write input sample to feedforward delay table
        x->f_feed[x->wptr] = sample;
        
        // toggle write pointer
        x->wptr = rptr0;
    }
    
    return &ptr[5];
}

// update coefficients ---------------------------------------------------------
/*
 * called after filter parameters are changed.
 * here we calculate the B and A coefficients for this filter. the filter
 * equations are the canonical second-order filters from DAFX vol.2 (p.50).
 * the term 'Q' is a scalar for filter resonance.
 * the term 'K' is a function of cutoff frequency and sampling rate.
 * all other terms are derived from Q and K to minimize redundant computation.
 */
static void allpass_update_BA(t_allpass* x)
{
    const t_float Q            = clip_Q(x->Q);
    const t_float K            = tanf(M_PI * clip_freq_ratio(x->freq, x->sr));
    const t_float KKQ          = K * K * Q;
    const t_float rDenominator = 1.f / (KKQ + K + Q);
    
    x->b_coef[0] =
    x->a_coef[1] = ((KKQ - K) + Q) * rDenominator;
    x->b_coef[1] =
    x->a_coef[0] = 2.f * (KKQ - Q) * rDenominator;
    x->b_coef[2] = 1.f;
}

// update allpass Q ------------------------------------------------------------
/*
 * called when we get the message "Q".
 * updates Q (arbitrary scalar).
 */
static void allpass_Q(t_allpass* x, t_floatarg new_Q)
{
    x->Q = new_Q;
    allpass_update_BA(x);
}

// update allpass frequency ----------------------------------------------------
/*
 * called when we get the message "freq".
 * updates freq (Hz.).
 */
static void allpass_freq(t_allpass* x, t_floatarg new_freq)
{
    x->freq = new_freq;
    allpass_update_BA(x);
}

// _new ------------------------------------------------------------------------
/*
 * called when this object is instantiated.
 * initialize object members and allocate memory.
 */
static void* allpass_new(t_symbol* selector, int argc, t_atom* argv)
{
    UNUSED_PARAM(selector);
    
    // make a pointer to this object
    t_allpass* x = (t_allpass*)pd_new(allpass_class);
    
    // make a new inlet for cutoff frequency and filter Q
    inlet_new(&x->object, &x->object.ob_pd, gensym("float"), gensym("Q"));
    inlet_new(&x->object, &x->object.ob_pd, gensym("float"), gensym("freq"));
    
    // make a signal outlet
    outlet_new(&x->object, gensym("signal"));
    
    // get creation arguments from user if they exist
    x->sample = 0.f;
    x->sr     = 44100.f; // just a guess (it gets updated when dsp is turned on)
    x->Q      = (argc > 0) ? atom_getfloat(&argv[0]) : default_Q;
    x->freq   = (argc > 1) ? atom_getfloat(&argv[1]) : default_freq;
    
    // update BA coefficients
    allpass_update_BA(x);
    
    return (void*)x;
}

// _dsp ------------------------------------------------------------------------
/*
 * called when dsp is turned on.
 * tell pd what arguments our _perform function needs, as well as where to find
 * them. if necessary, any initialization that needs info about pd's dsp state
 * should happen here to.
 */
static void allpass_dsp (t_allpass* x, t_signal** sig)
{
    // init the allpass filter
    x->sr = sig[0]->s_sr;    // set the allpass filter sampling rate
    allpass_update_BA(x);    // update BA coefficients
    
    // add this object's dsp function to pd's dsp function list
    dsp_add(allpass_perform, // this class' perform method
            4,               // number of perform method parameters
            sig[0]->s_vec,   // inlet sample vector
            sig[1]->s_vec,   // outlet sample vector
            sig[0]->s_n,     // block size (nSamples)
            x);              // pointer to this object
}

// _setup ----------------------------------------------------------------------
/*
 * called the first time someone loads this object in the current pd session.
 * tell pd about this object's "class", including our name, and which methods
 * and arguments we can handle.
 */
void allpass_tilde_setup(void)
{
    // tell pd how to build our class
    allpass_class = class_new(gensym("allpass~"),    // name
                           (t_newmethod)allpass_new, // _new
                           0,                        // _free
                           sizeof(t_allpass),        // size
                           CLASS_DEFAULT,            // flags
                           A_GIMME,                  // arg types list...
                           0);                       // ...0-terminated
    
    // tell pd that our left inlet expects audio
    CLASS_MAINSIGNALIN(allpass_class, t_allpass, sample);
    
    // tell pd which methods can be called by users (including dsp)
    class_addmethod(allpass_class, (t_method)allpass_dsp, gensym("dsp"), 0);
    class_addmethod(allpass_class, (t_method)allpass_Q, gensym("Q"), A_FLOAT, 0);
    class_addmethod(allpass_class, (t_method)allpass_freq, gensym("freq"), A_FLOAT, 0);
}

//------------------------------------------------------------------------------
//  Higher Order Filter Project
//
//  fir~.c: nth order finite impulse response filter
//  Copyright (c) 2016 Elliot Patros. All rights reserved.
//------------------------------------------------------------------------------

// Pd header and constants -----------------------------------------------------
#include "m_pd.h"
#include "higher_order_filter.h"

// pointer to this object's class ----------------------------------------------
static t_class* fir_class;

// this object's struct --------------------------------------------------------
typedef struct fir
{
    // instance of this object. must always be first
    t_object object;
    
    // state of each inlet value
    t_float  sample; // first inlet: audio, so not used for control rate
    
    // filter coefficients and delay table
    t_float* table;  // feed forward delay table
    t_word*  coefs;  // 'B' coefficients from table
    int      order;  // size of coefficient table
    t_int    wptr;   // write pointer (for delay tables)
    
} t_fir;

// _perform --------------------------------------------------------------------
/*
 * called at the start of every block while 'dsp' is on.
 * borrowing from miller's explanation, we get a pointer (ptr), where ptr[0] is
 * our function's location in the dsp call list. we return a new pointer, which
 * points to the next dsp function. meanwhile, arguments that are useful for
 * processing audio samples are packed after ptr[0], in the order specified in
 * this object's _dsp function.
 */
static t_int* fir_perform(t_int* ptr)
{
    // get this object's dsp-related state
    t_float*    input    = (t_float*)ptr[1];
    t_float*    output   = (t_float*)ptr[2];
    const t_int nSamples = (t_int)   ptr[3];
    t_fir*      x        = (t_fir*)  ptr[4];
    
    // zero-out output if there's no coefficient array
    if (x->coefs == 0)
    {
        memset(output, 0, sizeof(t_float) * nSamples);
        return &ptr[5];
    }
    
    // calculate fir: y(n) = sum(x(n - k) * h(k))
    for (t_int n = 0; n < nSamples; ++n, ++x->wptr)
    {
        x->wptr = (x->wptr < x->order) ? x->wptr : 0;
        
        const t_float x_n = input[n];
        t_float y_n = x->coefs[0].w_float * x_n;
        
        for (t_int k = 1; k < x->order; ++k)
        {
            const t_int m = x->wptr - k;
            y_n += x->coefs[k].w_float * x->table[(m < 0) ? x->order + m : m];
        }
        
        output[n] = y_n;
        x->table[x->wptr] = x_n;
    }
    
    return &ptr[5];
}

// _free -----------------------------------------------------------------------
/*
 * called when this object is deleted.
 * free any memory we've allocated.
 */
static void fir_free(t_fir* x)
{
    x->coefs = 0;
    x->order = 0;
    
    if (x->table != 0)
    {
        free(x->table);
        x->table = 0;
    }
}

// _set ------------------------------------------------------------------------
/*
 * called when we get the message "set".
 * if the table name is valid (exists, has floats, etc), we'll point to its
 * contents and use them for FIR coefficients in the _perform function.
 */
static void fir_set(t_fir* x, t_symbol* array_name)
{
    t_garray* array;
    
    if (array_name == 0)
    {   // array name is empty
        return;
    }
    else if ((array = (t_garray*)pd_findbyclass(array_name, garray_class)) == 0)
    {   // array name doesn't exist
        pd_error(x, "%s: no such array", array_name->s_name);
        fir_free(x);
        return;
    }
    else if (garray_getfloatwords(array, &x->order, &x->coefs) == 0)
    {   // array isn't for floats only
        pd_error(x, "%s: bad array template for fir~", array_name->s_name);
        fir_free(x);
        return;
    }
    else if (x->table != 0)
    {   // replace an old table
        x->table = (t_float*)realloc(x->table, sizeof(t_float) * x->order);
    }
    else
    {   // reference a new table for the first time
        x->table = (t_float*)calloc(x->order, sizeof(t_float));
    }
    
    if (x->table == 0)
    {   // delay line failed to allocate memory
        pd_error(x, "not enough memory for fir~");
        fir_free(x);
    }
}

// _new ------------------------------------------------------------------------
/*
 * called when a this object is instantiated.
 * initialize object members and allocate memory.
 */
static void* fir_new(t_symbol* s, int argc, t_atom* argv)
{
    UNUSED_PARAM(s);
    
    // setup this object with it's class
    t_fir* x = (t_fir*)pd_new(fir_class);
    
    // setup audio outlet
    outlet_new(&x->object, gensym("signal"));
    
    // setup internal state
    x->sample = 0;
    x->table  = 0;
    x->coefs  = 0;
    x->order  = 0;
    x->wptr   = 0;
    
    // parse any creation arguments
    t_symbol* array_name = (argc > 0) ? atom_getsymbol(&argv[0]) : 0;
    fir_set(x, array_name);
    
    if (x->coefs == 0 || x->table == 0)
    {
        fir_free(x);
    }
    
    return (void*)x;
}

// _dsp ------------------------------------------------------------------------
/*
 * called when dsp is turned on.
 * tell pd what arguments our _perform function needs, as well as where to find 
 * them. if necessary, any initialization that needs info about pd's dsp state
 * should happen here to.
 */
static void fir_dsp (t_fir* x, t_signal** sig)
{
    dsp_add(fir_perform,   // this class' perform method
            4,             // number of perform method parameters
            sig[0]->s_vec, // inlet sample vector
            sig[1]->s_vec, // outlet sample vector
            sig[0]->s_n,   // block size (nSamples)
            x);            // pointer to this object
}

// _setup ----------------------------------------------------------------------
/*
 * called the first time someone loads this object in the current pd session. 
 * tell pd about this object's "class", including our name, and which methods
 * and arguments we can handle.
 */
void fir_tilde_setup(void)
{
    // tell pd how to build our class
    fir_class = class_new(gensym("fir~"),       // name
                          (t_newmethod)fir_new, // _new
                          (t_method)fir_free,   // _free
                          sizeof(t_fir),        // size
                          CLASS_DEFAULT,        // flags
                          A_GIMME,              // arg types list...
                          0);                   // ...0-terminated
    
    // tell pd that our left inlet expects audio
    CLASS_MAINSIGNALIN(fir_class, t_fir, sample);
    
    // tell pd which methods can be called by users (including dsp)
    class_addmethod(fir_class, (t_method)fir_dsp, gensym("dsp"), 0);
    class_addmethod(fir_class, (t_method)fir_set, gensym("set"), A_SYMBOL, 0);
}

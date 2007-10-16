/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2007 Carnegie Mellon University.  All rights 
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
/**
 * \file gmm_precomp.c
 * GMM precomputation tool.
 */
#include <gau_cb.h>
#include <gau_mix.h>

#include <stdio.h>
#include <string.h>

#include "gau_file.h"

static const arg_t defn[] = {
  { "-help",
    ARG_BOOLEAN,
    "no",
    "Shows the usage of the tool"},
  
  { "-example",
    ARG_BOOLEAN,
    "no",
    "Shows example of how to use the tool"},

  { "-var",
    REQARG_STRING,
    NULL,
    "Variance input file." },
  
  { "-outvar",
    REQARG_STRING,
    NULL,
    "Variance output file" },
  
  { "-outnorm",
    REQARG_STRING,
    NULL,
    "Normalization constant output file" },

  { "-logbase",
    ARG_FLOAT32,
    "1.0001",
    "Log base to use for output (must be 1.0001 for PocketSphinx)" },

  { "-output_endian",
    ARG_STRING,
#ifdef WORDS_BIGENDIAN
    "big",
#else
    "little",
#endif
    "Endianness of output files (one of 'big', 'little')" },

  { "-mmap",
    ARG_BOOLEAN,
    "no",
    "Use memory-mapped I/O for reading files (unnecessary here...)"},

  { "-varfloor",
    ARG_FLOAT32,
    "0.0001",
    "Mixture gaussian variance floor (applied to data from -var file)" },

  { NULL, 0, NULL, NULL }
};

int
main(int argc, char *argv[])
{
    cmd_ln_t *config;
    const char *invar, *outvar, *outnorm;
    gau_cb_t *cb;
    gau_file_t out_file;
    int byteswap;
    var_t ****invvars;
    norm_t ***norms;

    config = cmd_ln_parse_r(NULL, defn, argc, argv, TRUE);
    if (config == NULL) {
        return 1;
    }
    invar = cmd_ln_str_r(config, "-var");
    outvar = cmd_ln_str_r(config, "-outvar");
    outnorm = cmd_ln_str_r(config, "-outnorm");

#ifdef WORDS_BIGENDIAN
    byteswap = !strcmp(cmd_ln_str_r(config, "-output_endian"), "big");
#else
    byteswap = !strcmp(cmd_ln_str_r(config, "-output_endian"), "little");
#endif

    cb = gau_cb_read(config, NULL, invar, NULL);

    invvars = gau_cb_get_invvars(cb);
    norms = gau_cb_get_norms(cb);

    /* Create a new gau_file_t to write out precompiled data. */
    gau_cb_get_shape(cb, &out_file.n_mgau,
                     &out_file.n_feat, &out_file.n_density,
                     (const int **)&out_file.veclen);
    /* FIXME: Also we want to precompile for fixed-point here... */
    out_file.format = GAU_FLOAT32;
    out_file.width = 4;
    out_file.flags = GAU_FILE_PRECOMP;
    out_file.scale = 1.0;
    out_file.bias = 0.0;
    out_file.logbase = cmd_ln_float32_r(config, "-logbase");
    out_file.data = invvars[0][0][0];
    /* Write out variances. */
    gau_file_write(&out_file, outvar, byteswap);

    /* Now modify it to be 3-dimensional for the normalizers. */
    out_file.veclen = NULL;
    out_file.data = norms[0][0];
    /* Write out normalizers. */
    gau_file_write(&out_file, outnorm, byteswap);

    gau_cb_free(cb);
    
    return 0;
}

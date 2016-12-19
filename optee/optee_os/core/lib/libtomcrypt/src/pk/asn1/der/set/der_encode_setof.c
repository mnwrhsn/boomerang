/*
 * Copyright (c) 2001-2007, Tom St Denis
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */
#include "tomcrypt.h"

/**
  @file der_encode_setof.c
  ASN.1 DER, Encode SET OF, Tom St Denis
*/

#ifdef LTC_DER

struct edge {
   unsigned char *start;
   unsigned long  size;
};

static int qsort_helper(const void *a, const void *b)
{
   struct edge   *A = (struct edge *)a, *B = (struct edge *)b;
   int            r;
   unsigned long  x;

   /* compare min length */
   r = XMEMCMP(A->start, B->start, MIN(A->size, B->size));

   if (r == 0 && A->size != B->size) {
      if (A->size > B->size) {
         for (x = B->size; x < A->size; x++) {
            if (A->start[x]) {
               return 1;
            }
         }
      } else {
         for (x = A->size; x < B->size; x++) {
            if (B->start[x]) {
               return -1;
            }
         }
      }
   }

   return r;
}

/**
   Encode a SETOF stucture
   @param list      The list of items to encode
   @param inlen     The number of items in the list
   @param out       [out] The destination
   @param outlen    [in/out] The size of the output
   @return CRYPT_OK on success
*/
int der_encode_setof(ltc_asn1_list *list, unsigned long inlen,
                     unsigned char *out,  unsigned long *outlen)
{
   unsigned long  x, y, z;
   ptrdiff_t hdrlen;
   int            err;
   struct edge   *edges;
   unsigned char *ptr, *buf;

   /* check that they're all the same type */
   for (x = 1; x < inlen; x++) {
      if (list[x].type != list[x-1].type) {
         return CRYPT_INVALID_ARG;
      }
   }

   /* alloc buffer to store copy of output */
   buf = XCALLOC(1, *outlen);
   if (buf == NULL) {
      return CRYPT_MEM;
   }

   /* encode list */
   if ((err = der_encode_sequence_ex(list, inlen, buf, outlen, LTC_ASN1_SETOF)) != CRYPT_OK) {
       XFREE(buf);
       return err;
   }

   /* allocate edges */
   edges = XCALLOC(inlen, sizeof(*edges));
   if (edges == NULL) {
      XFREE(buf);
      return CRYPT_MEM;
   }

   /* skip header */
      ptr = buf + 1;

      /* now skip length data */
      x = *ptr++;
      if (x >= 0x80) {
         ptr += (x & 0x7F);
      }

      /* get the size of the static header */
      hdrlen = ptr - buf;


   /* scan for edges */
   x = 0;
   while (ptr < (buf + *outlen)) {
      /* store start */
      edges[x].start = ptr;

      /* skip type */
      z = 1;

      /* parse length */
      y = ptr[z++];
      if (y < 128) {
         edges[x].size = y;
      } else {
         y &= 0x7F;
         edges[x].size = 0;
         while (y--) {
            edges[x].size = (edges[x].size << 8) | ((unsigned long)ptr[z++]);
         }
      }

      /* skip content */
      edges[x].size += z;
      ptr           += edges[x].size;
      ++x;
   }

   /* sort based on contents (using edges) */
   XQSORT(edges, inlen, sizeof(*edges), &qsort_helper);

   /* copy static header */
   XMEMCPY(out, buf, hdrlen);

   /* copy+sort using edges+indecies to output from buffer */
   for (y = hdrlen, x = 0; x < inlen; x++) {
      XMEMCPY(out+y, edges[x].start, edges[x].size);
      y += edges[x].size;
   }

#ifdef LTC_CLEAN_STACK
   zeromem(buf, *outlen);
#endif

   /* free buffers */
   XFREE(edges);
   XFREE(buf);

   return CRYPT_OK;
}

#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */
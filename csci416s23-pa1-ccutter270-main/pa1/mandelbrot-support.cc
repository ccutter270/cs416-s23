/*
  Copyright (c) 2010-2011, Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.


   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
   TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <cstdio>
#include "CycleTimer.h"

void WritePPM(int *buf, int width, int height, const char *fn) {
  FILE *fp = fopen(fn, "wb");
  fprintf(fp, "P6\n");
  fprintf(fp, "%d %d\n", width, height);
  fprintf(fp, "255\n");
  for (int i = 0; i < width * height; ++i) {
    // Map the iteration count to colors by just alternating between two greys.
    char c = (buf[i] & 0x1) ? (char)240 : 20;
    for (int j = 0; j < 3; ++j) {
      fputc(c, fp);
    }
  }
  fclose(fp);
  fprintf(stderr, "Wrote image file %s\n", fn);
}

namespace {
inline int mandel(float c_re, float c_im, int count) {
  float z_re = c_re, z_im = c_im;
  int i;
  for (i = 0; i < count; ++i) {
    if (z_re * z_re + z_im * z_im > 4.f) {
      break;
    }

    float new_re = z_re * z_re - z_im * z_im;
    float new_im = 2.f * z_re * z_im;
    z_re = c_re + new_re;
    z_im = c_im + new_im;
  }

  return i;
}
}  // namespace

void MandelbrotSerial(float x0, float y0, float x1, float y1, int width, int height,
                      int maxIterations, int output[], int start_row, int end_row, int start_col,
                      int end_col) {
  float dx = (x1 - x0) / width;
  float dy = (y1 - y0) / height;

  for (int j = start_row; j < end_row; j++) {
    for (int i = start_col; i < end_col; i++) {
      float x = x0 + i * dx;
      float y = y0 + j * dy;

      int index = (j * width + i);
      output[index] = mandel(x, y, maxIterations);
    }
  }
}
// ------------------------------------------------------------------------------
// qpc_ap.c
// A-priori decoding for the SuperFox polar code: message symbols that are KNOWN
// are decoded as what a polar code calls frozen symbols.
//
// CE3TSK 2026-09-20, for JTDX_CONTEST.                  Contact: jtdx_contest@ce3tsk.com
// ------------------------------------------------------------------------------
//
//    This source is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//    This file is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
// The decoder itself is Nico Palermo IV3NWV's _qpc_decode (np_qpc.c, unchanged). A polar code
// already has positions whose value the decoder is TOLD instead of deciding it - the frozen
// ones, which the code's definition lists with the value 0. qpc_decode_ap hands _qpc_decode a
// copy of that list in which nap more positions are marked frozen, with the values the caller
// states: message symbol kap[i] (the encoder's order: x[k] stands at position xpos[k]) has the
// value vap[i]. Everything else is qpc_decode's, line for line.
//
// What it is worth depends on WHICH symbols are told (lib/superfox/qpc_decode2.f90): the decoder
// is successive cancellation, it decides position 127 first and position 0 last, and its first
// decisions are its least reliable ones.

#include <string.h>
#include "np_qpc.h"

void qpc_decode_ap(unsigned char* xdec, unsigned char* ydec, float* py, int nap, const int* kap, const unsigned char* vap)
{
    int k;
    unsigned char x[QPC_N], f[QPC_N], fsize[QPC_N];

    // the punctured symbol is known to be the first frozen value (qpc_decode)
    if (qpccode.np < qpccode.n) {
        memset(py, 0, QPC_Q * sizeof(float));
        py[qpccode.f[0]] = 1.0f;
    }

    memcpy(f, qpccode.f, QPC_N);
    memcpy(fsize, qpccode.fsize, QPC_N);
    for (k = 0; k < nap; k++) {
        if (kap[k] < 0 || kap[k] >= QPC_K) continue;
        f[qpccode.xpos[kap[k]]] = vap[k];
        fsize[qpccode.xpos[kap[k]]] = 0;
    }

    _qpc_decode(x, ydec, py, f, fsize, QPC_N);

    // demap the information symbols; a frozen leaf reports what it WOULD have decided, not the
    // value it was told, so the told symbols are written back
    for (k = 0; k < QPC_K; k++)
        xdec[k] = x[qpccode.xpos[k]];
    for (k = 0; k < nap; k++)
        if (kap[k] >= 0 && kap[k] < QPC_K) xdec[kap[k]] = vap[k];
}

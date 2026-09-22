// ------------------------------------------------------------------------------
// qpc_scl.c
// CRC-aided successive-cancellation LIST decoding of the SuperFox q-ary polar code
// (127,50), Q = 128, with CANDIDATE SETS for the Hound slots of a SuperFox message.
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
// PATENT NOTICE. The source code is available under the GNU GPL v3 and is intended for
// educational, simulation and research purposes. CRC-aided successive-cancellation list
// (CA-SCL) decoding of polar codes is used in 5G; deploying it in commercial telecom networks
// may require Standard Essential Patent (SEP) licensing from the appropriate 3GPP patent pools.
// This notice is information only and adds no restriction to the GPL.
//
// WHAT IT IS. WSJT-X's decoder (np_qpc.c, IV3NWV) is successive cancellation: it walks the
// polar transform's 128 positions from 127 down to 0 and at every information position takes
// the most probable of the 128 symbol values - ONE path, and a wrong early decision is final.
// This decoder keeps the L most probable paths. A path's metric is the sum of the log
// probabilities of its decisions - at a FROZEN position the log probability of the value the
// code (or an a-priori hypothesis) says is there, which is what tells a good path from a bad one
// between the information positions. At an information position every path is extended by all
// 128 values, and the L best of the P x 128 extensions live on. At the end the paths are tried
// best first against the message's CRC-21, and the first that passes is the decode.
//
// HOW. The same recursion as _qpc_decode, written as a function of a SET of paths: a node gets
// one array of symbol pdfs per path; it convolves lower and upper half per path and hands the
// results to the upper child, which returns a new set of paths - each with the path it came from
// (its origin), its hard decisions and its metric; for each of those the node folds the decisions
// into the ORIGIN's pdfs (convhard, multiply) and hands that to the lower child; what comes back
// is stitched together. Nothing is copied when a path forks: a fork is two paths with one origin.
// With L = 1 this is _qpc_decode, decision for decision (test/experiments/sfox_scl: the whole
// regression suite passes with this decoder at L = 1 in place of the donor's).
//
// The arithmetic - convolution by Walsh-Hadamard transform, the normalised product, the hard
// convolution - is the code's own and could not be anything else; it is written out again here
// because np_qpc.c keeps its versions file-static. qpc_fwht.c and the code definition are used.
//
// A-PRIORI symbols (nap, kap, vap) are frozen positions for the call, as in qpc_ap.c: message
// symbol kap[i] (encoder order) has the value vap[i].
//
// Every buffer is per THREAD; the candidate sets are one per process and must be laid from a
// serial point (the SuperFox slot is decoded single-threaded today).

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "qpc_fwht.h"
#include "np_qpc.h"

uint32_t nhash2(const void* key, uint64_t length, uint32_t initval);

#define SCL_LMAX 256                // the largest list; memory is taken for the largest L a thread has asked for: 2 MB at L = 16,
                                    // 34 MB at 256 - and kept: a thread's buffers are never given back (one thread calls today)
#define SCL_TINY 1.0e-30f

// CANDIDATE SETS (step S4). A Hound slot is 28 bits = four message symbols. When the receiver knows
// WHO may stand in a slot - the Hounds it has heard, or "empty" (the filler token) - a path may only
// take, at each of the slot's four positions, a value that some candidate still alive on that path
// has there. Each path carries, per slot, the set of candidates its decisions so far agree with
// (a bit mask, up to 64 candidates a slot). The list then searches over ASSIGNMENTS OF CALLS, not
// over raw symbols. The metric is unchanged - the log of the FULL leaf probability - so constrained
// and unconstrained decisions stay comparable. Measured as the receiver runs it (L = 16, two looks;
// SUPERFOX_DECODER_IDEAS.md 4.11): a Fox with six Hounds, all of them in a pool of 16 - 50 % from
// -16.7 to -18.3 dB, on a fading channel from -15.3 to -17.3 dB.
#define SCL_NSLOT 9
#define SCL_CMAX 64
typedef struct { uint64_t m[SCL_NSLOT]; } scl_mask;
static int cset_init = 0, cset_n[SCL_NSLOT];   // cset_init: 0 not built, 1 being built, 2 built - read and set ATOMICALLY (cset_setup)
static _Thread_local int cset_on = 0;            // the sets act only while the caller says so (qpc_scl_cand)
static unsigned char cset_sym[SCL_NSLOT][SCL_CMAX][4];
static signed char pos_slot[QPC_N], pos_j[QPC_N];

typedef struct {            // what one level of the recursion needs, for up to L paths
    scl_mask* mh;           // the upper child's masks   [L]
    float* pyh;             // inputs of the upper child   [L][h*Q]
    float* pyl;             // inputs of the lower child   [L][h*Q]
    unsigned char* xh; unsigned char* yh;   // the upper child's decisions   [L][h]
    unsigned char* xl; unsigned char* yl;   // the lower child's             [L][h]
    int o1[SCL_LMAX], o2[SCL_LMAX];
    float m1[SCL_LMAX], m2[SCL_LMAX];
    const float* in[SCL_LMAX];
} scl_level;

// RE-ENTRANT: every buffer is per thread (the donor's np_qpc.c keeps its work arrays file-static and can
// never be called from two threads; this one can - README.md, step S2b)
static _Thread_local scl_level lev[QPC_LOG2N];
static _Thread_local int scl_alloc = 0;          // the list size this thread's buffers hold
typedef struct { float m; int p; int s; uint64_t mask; } scl_cand;
static _Thread_local scl_cand* cand = NULL;      // [L * Q]
static _Thread_local unsigned char* xo = NULL;   // the root's decisions [L * N]
static _Thread_local unsigned char* yo = NULL;
static _Thread_local scl_mask* rootmask = NULL;  // [L]

static void* scl_take(void* old, size_t bytes)
{
    void* p;
    free(old);
    p = malloc(bytes);
    if (!p) abort();                              // no memory for a few MB: nothing sensible can follow
    return p;
}

static void scl_setup(int L)
{
    int d, h;
    if (L <= scl_alloc) return;
    for (d = 0; d < QPC_LOG2N; d++) {
        h = (QPC_N >> d) / 2;
        lev[d].pyh = (float*)scl_take(lev[d].pyh, sizeof(float) * L * h * QPC_Q);
        lev[d].pyl = (float*)scl_take(lev[d].pyl, sizeof(float) * L * h * QPC_Q);
        lev[d].xh = (unsigned char*)scl_take(lev[d].xh, (size_t)L * h); lev[d].yh = (unsigned char*)scl_take(lev[d].yh, (size_t)L * h);
        lev[d].xl = (unsigned char*)scl_take(lev[d].xl, (size_t)L * h); lev[d].yl = (unsigned char*)scl_take(lev[d].yl, (size_t)L * h);
        lev[d].mh = (scl_mask*)scl_take(lev[d].mh, sizeof(scl_mask) * L);
    }
    cand = (scl_cand*)scl_take(cand, sizeof(scl_cand) * L * QPC_Q);
    xo = (unsigned char*)scl_take(xo, (size_t)L * QPC_N); yo = (unsigned char*)scl_take(yo, (size_t)L * QPC_N);
    rootmask = (scl_mask*)scl_take(rootmask, sizeof(scl_mask) * L);
    scl_alloc = L;
}

static void row_conv(float* dst, const float* a, const float* b)
{
    float fa[QPC_Q], fb[QPC_Q], ta[QPC_Q], tb[QPC_Q]; int k;
    memcpy(ta, a, sizeof ta); memcpy(tb, b, sizeof tb);
    qpc_fwht(fa, ta); qpc_fwht(fb, tb);
    for (k = 0; k < QPC_Q; k++) fa[k] *= fb[k];
    qpc_fwht(dst, fa);
    for (k = 0; k < QPC_Q; k++) dst[k] *= 1.0f / QPC_Q;
}

static void row_mulhard(float* dst, const float* lower, const float* upper, unsigned char hd)
{
    // lower[k] * upper[k ^ hd], normalised; uniform if nothing is left (np_qpc.c: pdf_mul after pdf_convhard)
    int k; float v, norm = 0.0f;
    for (k = 0; k < QPC_Q; k++) { v = lower[k] * upper[k ^ hd]; dst[k] = v; norm += v; }
    if (norm <= 0.0f) for (k = 0; k < QPC_Q; k++) dst[k] = 1.0f / QPC_Q;
    else { norm = 1.0f / norm; for (k = 0; k < QPC_Q; k++) dst[k] *= norm; }
}

static int cand_cmp(const void* a, const void* b)
{
    float d = ((const scl_cand*)b)->m - ((const scl_cand*)a)->m;
    if (d > 0) return 1;
    if (d < 0) return -1;
    // equal metrics: the lower path, then the lower symbol - a fixed order, whatever qsort does
    if (((const scl_cand*)a)->p != ((const scl_cand*)b)->p) return ((const scl_cand*)a)->p - ((const scl_cand*)b)->p;
    return ((const scl_cand*)a)->s - ((const scl_cand*)b)->s;
}

// P paths in (pdfs py[p], metrics min[p]); up to L paths out: decisions xout/yout [.][n], origin, metric
static int scl_node(int n, int d, int pos0, int P, const float* const* py, const float* min, const scl_mask* maskin, const unsigned char* f, const unsigned char* fsize,
                    int L, unsigned char* xout, unsigned char* yout, int* orig, float* mout, scl_mask* maskout)
{
    int p, q, r, k, s, nc, Q1, Q2, h, size, slot, c; float sum, pr; uint64_t alive, vm[QPC_Q];

    if (n == 1) {
        if (fsize[0] == 0) {                       // frozen: every path goes on, and pays for disagreeing
            for (p = 0; p < P; p++) {
                sum = 0.0f; for (s = 0; s < QPC_Q; s++) sum += py[p][s];
                pr = (sum > 0.0f) ? py[p][f[0]] / sum : 1.0f / QPC_Q;
                mout[p] = min[p] + logf(pr > SCL_TINY ? pr : SCL_TINY);
                xout[p] = f[0]; yout[p] = f[0]; orig[p] = p; maskout[p] = maskin[p];
            }
            return P;
        }
        nc = 0;                                    // information: the L best of the P x Q extensions
        slot = pos_slot[pos0]; if (slot >= 0 && (cset_n[slot] == 0 || !cset_on)) slot = -1;
        for (p = 0; p < P; p++) {
            sum = 0.0f; for (s = 0; s < QPC_Q; s++) sum += py[p][s];
            if (slot >= 0) {                        // only what a candidate still alive on this path has here
                memset(vm, 0, sizeof vm); alive = maskin[p].m[slot];
                for (c = 0; c < cset_n[slot]; c++) if (alive >> c & 1) vm[cset_sym[slot][c][(int)pos_j[pos0]]] |= (uint64_t)1 << c;
            }
            for (s = 0; s < QPC_Q; s++) {
                if (slot >= 0 && vm[s] == 0) continue;
                pr = (sum > 0.0f) ? py[p][s] / sum : 1.0f / QPC_Q;
                cand[nc].m = min[p] + logf(pr > SCL_TINY ? pr : SCL_TINY); cand[nc].p = p; cand[nc].s = s; cand[nc].mask = (slot >= 0) ? vm[s] : 0; nc++;
            }
        }
        if (L == 1 && slot < 0) {                              // plain successive cancellation: the first maximum, as pdf_max takes it
            int best = 0; for (k = 1; k < nc; k++) if (py[0][k] > py[0][best]) best = k;
            xout[0] = (unsigned char)best; yout[0] = (unsigned char)best; orig[0] = 0; mout[0] = cand[best].m; maskout[0] = maskin[0]; return 1;
        }
        qsort(cand, nc, sizeof(scl_cand), cand_cmp);
        if (nc > L) nc = L;
        for (k = 0; k < nc; k++) { xout[k] = (unsigned char)cand[k].s; yout[k] = xout[k]; orig[k] = cand[k].p; mout[k] = cand[k].m;
                                   maskout[k] = maskin[cand[k].p]; if (slot >= 0) maskout[k].m[slot] = cand[k].mask; }
        return nc;
    }

    h = n / 2; size = h * QPC_Q;
    scl_level* v = &lev[d];
    for (p = 0; p < P; p++) {                      // the upper child sees lower (*) upper, per path
        for (k = 0; k < h; k++) row_conv(v->pyh + (size_t)p * size + k * QPC_Q, py[p] + k * QPC_Q, py[p] + size + k * QPC_Q);
        v->in[p] = v->pyh + (size_t)p * size;
    }
    Q1 = scl_node(h, d + 1, pos0 + h, P, v->in, min, maskin, f + h, fsize + h, L, v->xh, v->yh, v->o1, v->m1, v->mh);
    for (q = 0; q < Q1; q++) {                     // the lower child sees lower x upper shifted by the upper child's decisions
        const float* src = py[v->o1[q]];
        for (k = 0; k < h; k++) row_mulhard(v->pyl + (size_t)q * size + k * QPC_Q, src + k * QPC_Q, src + size + k * QPC_Q, v->yh[q * h + k]);
    }
    for (q = 0; q < Q1; q++) v->in[q] = v->pyl + (size_t)q * size;
    Q2 = scl_node(h, d + 1, pos0, Q1, v->in, v->m1, v->mh, f, fsize, L, v->xl, v->yl, v->o2, v->m2, maskout);
    for (r = 0; r < Q2; r++) {
        q = v->o2[r]; orig[r] = v->o1[q]; mout[r] = v->m2[r];
        for (k = 0; k < h; k++) {
            xout[r * n + k] = v->xl[r * h + k];             xout[r * n + h + k] = v->xh[q * h + k];
            yout[r * n + k] = v->yl[r * h + k];             yout[r * n + h + k] = v->yh[q * h + k] ^ v->yl[r * h + k];
        }
    }
    return Q2;
}

// The maps from code positions to Hound slots are built ONCE per process. 2026-09-21, the search in threads
// (qpc_decode2.f90 sfox_search_mt): the first call of this decoder may now come from several threads at once, and a
// plain flag set "last" is no guarantee under the C memory model - one thread could build while another reads half
// a map. So the flag is taken with an atomic compare-and-swap (exactly one thread builds), the others wait for it
// with acquire loads, and the builder publishes with a release store. The caller also builds them from a serial
// point before any batch (qpc_scl_init), so the wait is never reached in practice. The candidate SETS themselves
// are laid only from serial code (the pool pass) and are read-only while threads decode.
static void cset_setup(void)
{
    int i, j, m, expect = 0;
    if (__atomic_load_n(&cset_init, __ATOMIC_ACQUIRE) == 2) return;
    if (!__atomic_compare_exchange_n(&cset_init, &expect, 1, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
        while (__atomic_load_n(&cset_init, __ATOMIC_ACQUIRE) != 2) { }   // another thread is building: its maps, when done
        return;
    }
    for (i = 0; i < QPC_N; i++) { pos_slot[i] = -1; pos_j[i] = 0; }
    for (i = 0; i < SCL_NSLOT; i++) { cset_n[i] = 0;
        for (j = 0; j < 4; j++) { m = 4 + 4 * i + j; pos_slot[qpccode.xpos[QPC_K - 1 - m]] = (signed char)i; pos_j[qpccode.xpos[QPC_K - 1 - m]] = (signed char)j; } }
    __atomic_store_n(&cset_init, 2, __ATOMIC_RELEASE);
}

// build the shared maps now, from a serial point (qpc_decode2.f90 before the threaded search's batches)
void qpc_scl_init(void) { cset_setup(); }

// the candidate set of Hound slot `slot` (0..8): n calls as pack28 writes them, at most 64; n = 0: the
// slot is not constrained. slot < 0: no slot is.
void qpc_scl_setcand(int slot, int n, const int* n28)
{
    int i, j;
    cset_setup();                  // a no-op once built (atomic)
    if (slot < 0) { for (i = 0; i < SCL_NSLOT; i++) cset_n[i] = 0; return; }
    if (slot >= SCL_NSLOT) return;
    if (n > SCL_CMAX) n = SCL_CMAX;
    if (n < 0) n = 0;
    for (i = 0; i < n; i++) for (j = 0; j < 4; j++) cset_sym[slot][i][j] = (unsigned char)(((unsigned)n28[i] >> (21 - 7 * j)) & 127);
    cset_n[slot] = n;
}

// the candidate sets on (1) or off (0) for the calls that follow, on this thread
void qpc_scl_cand(int on) { cset_on = on; }

// xdec[50], ydec[128], py[128*128] as qpc_decode takes them; L the list size (1 .. 256); nap/kap/vap
// a-priori symbols (qpc_ap.c); ncrc how many paths, best first, may be tried against the CRC - each
// is a 2^-21 chance of a false line; without candidate sets the right path is the best one in 19
// decodes of 20, with them in every decode on record. Returns
// the rank (1 = the most probable path) of the path whose CRC-21 passed - that path is what xdec and
// ydec hold - or 0 when none passed, and then they hold the most probable path, as successive
// cancellation would have returned it. *ntried: how many paths WERE tried against the CRC.
int qpc_decode_scl(unsigned char* xdec, unsigned char* ydec, float* py, int L, int nap, const int* kap, const unsigned char* vap, int ncrc, int* ntried)
{
    int orig[SCL_LMAX]; float mout[SCL_LMAX];
    unsigned char f[QPC_N], fsize[QPC_N], msg[QPC_K]; const float* in0[1]; float m0[1] = { 0.0f };
    int k, r, Q, rank = 0; uint32_t crc, sent;

    if (L < 1) L = 1;
    if (L > SCL_LMAX) L = SCL_LMAX;
    scl_setup(L);
    if (qpccode.np < qpccode.n) { memset(py, 0, QPC_Q * sizeof(float)); py[qpccode.f[0]] = 1.0f; }   // the punctured symbol (qpc_decode)
    memcpy(f, qpccode.f, QPC_N); memcpy(fsize, qpccode.fsize, QPC_N);
    for (k = 0; k < QPC_K; k++) f[qpccode.xpos[k]] = 0;          // the encoder writes its message there (qpc_encode); a decoder must not see it
    for (k = 0; k < nap; k++) if (kap[k] >= 0 && kap[k] < QPC_K) { f[qpccode.xpos[kap[k]]] = vap[k]; fsize[qpccode.xpos[kap[k]]] = 0; }

    cset_setup();                  // a no-op once built (atomic)
    { scl_mask m00; int i;
      for (i = 0; i < SCL_NSLOT; i++) m00.m[i] = (cset_n[i] >= 64) ? ~(uint64_t)0 : (((uint64_t)1 << cset_n[i]) - 1);
      in0[0] = py;
      Q = scl_node(QPC_N, 0, 0, 1, in0, m0, &m00, f, fsize, L, xo, yo, orig, mout, rootmask); }
    // the paths come out of the last leaf best first, except after a FROZEN last leaf, which keeps the order of its inputs
    if (ncrc < 1) ncrc = 1;
    if (ncrc > Q) ncrc = Q;
    for (r = 0; r < ncrc && rank == 0; r++) {
        int best = -1;                                           // selection by metric: Q is small
        for (k = 0; k < Q; k++) if (orig[k] >= 0 && (best < 0 || mout[k] > mout[best])) best = k;
        for (k = 0; k < QPC_K; k++) msg[k] = xo[best * QPC_N + qpccode.xpos[QPC_K - 1 - k]];   // message order: symbol m is x[49-m]
        crc = nhash2(msg, 47, 571) & 0x1FFFFF;
        sent = ((uint32_t)msg[47] << 14) | ((uint32_t)msg[48] << 7) | (uint32_t)msg[49];
        if (crc == sent || r == 0) {                             // the best path is the answer unless a later one passes
            for (k = 0; k < QPC_K; k++) xdec[k] = xo[best * QPC_N + qpccode.xpos[k]];
            memcpy(ydec, yo + best * QPC_N, QPC_N);
            if (crc == sent) rank = r + 1;
        }
        orig[best] = -1;                                         // taken
    }
    if (ntried) *ntried = r;
    return rank;
}

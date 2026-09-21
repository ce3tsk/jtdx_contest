#ifndef COMMONS_H
#define COMMONS_H

#define NSMAX 6827
#define NTMAX 120
#define RX_SAMPLE_RATE 12000

#ifdef __cplusplus
#include <cstdbool>
//extern "C" {
#else
#include <stdbool.h>
#endif

  /*
   * This structure is shared with Fortran code, it MUST be kept in
   * sync with lib/jt9com.f90
   * int nutc;                   //UTC as integer, HHMM
   * int ntrperiod;              //TR period (seconds)
   * int nfqso;                  //User-selected QSO freq (kHz)
   * int npts8;                  //npts for c0() array
   * int nfa;                    //Low decode limit (Hz)
   * int nfSplit;                //JT65 | JT9 split frequency
   * int nfb;                    //High decode limit (Hz)
   * int ntol;                   //+/- decoding range around fQSO (Hz)
   * bool ndiskdat;              //true ==> data read from *.wav file
   * bool newdat;                //true ==> new data, must do long FFT
   */
typedef struct dec_data {
  float ss[184*NSMAX];
  float savg[NSMAX];
  short int d2[NTMAX*RX_SAMPLE_RATE];
  float dd2[NTMAX*RX_SAMPLE_RATE];
  struct
  {
//    char datetime[20]; 
    char mycall[12];
    char mybcall[12];
    char hiscall[12];
    char hisbcall[12];
//    char mygrid[6];
    char hisgrid[6];
    int listutc[10];
    int napwid;
    int nQSOProgress;
    int nftx;
    int nutc;
    int ntrperiod;
    int nfqso;
    int npts8;
    int nfa;
    int nfSplit;
    int nfb;
    int ntol;
    int kin;
    int nzhsym;
    int ndepth;
    int ncandthin;
    int ndtcenter;
    int nft8cycles;
    int nft8swlcycles;
    int ntxmode;
    int nmode;
    int nlist;
    int nranera;
    int ntrials10;
    int ntrialsrxf10;
    int naggressive;
    int nharmonicsdepth;
    int ntopfreq65;
    int nprepass;
    int nsdecatt;
    int nlasttx;
    int ndelay;
    int nmt;
    int nft8rxfsens;
    int nft4depth;
    int nsecbandchanged;
    int nft8ensemble;   // CE3TSK: ensemble members 0-5 (perturbed re-decodes, lib/ft8ensemble.f90)
    int nft8bgeffort;   // CE3TSK: background effort: -1 as the budget allows, 0 off, n units (pipeline ensemble)
    int nbgmargin;      // CE3TSK: the background phase stops this many tenths of a second before the next decode
    int nbgbudget;      // CE3TSK P7: the background's window from the decode's start, tenths of a second (0: the period)
    int nrxbudget;      // CE3TSK P8: the RX phase's budget for 'budget auto' members, tenths of a second (<=0: 27)
    int nft8bgcycles;   // CE3TSK: the TX background recipe (PIPELINED_DECODE_PLAN.md): cycles, SWL cycles,
    int nft8bgswlcycles; //   ensemble members (-1 as the budget allows), RX frequency sensitivity
    int nft8bgensemble;
    int nft8bgrxfsens;
    bool ndiskdat;
    bool newdat;
    bool nagain;
    bool nagainfil; 
    bool nswl;
    bool nfilter;
    bool nstophint;
    bool nagcc;
    bool nhint;
    bool fmaskact;
    bool showharmonics;
    bool lft8lowth;
    bool lft8subpass;
    bool ltxing;
    bool lhidetest;
    bool lhidetelemetry;
    bool lhideft8dupes;
    bool lhound;
    bool lhidehash;
    bool lcommonft8b;
    bool lmycallstd;
    bool lhiscallstd;
    bool lapmyc;
    bool lmodechanged;
    bool lbandchanged;
    bool lenabledxcsearch;
    bool lwidedxcsearch;
    bool lmultinst;
    bool lskiptx1;
    bool lforcesync;
    bool learlystart;
    bool lft8deeposd;   // CE3TSK: OSD order 2 for every FT8 candidate (weak-signal mode)
    bool lft8twopass;   // CE3TSK: second slicing pass, thread slices offset by half a slice
    bool lft8altpass;   // CE3TSK: alternate-approach pass (7 cycles + OSD order 2) on the subtracted band
    bool lbgswl;        // CE3TSK: the TX background recipe: SWL mode, OSD order 2, second slicing pass,
    bool lbgdeeposd;    //   alternate-approach pass, low thresholds, subpass
    bool lbgtwopass;
    bool lbgaltpass;
    bool lbglowth;
    bool lbgsubpass;
    int nft8bgclassic;  // CE3TSK P9: the classic background unit - the plain non-SWL decode with this many cycles, 0 off
    bool lft4altpass;   // CE3TSK: FT4 alternate pass on the residual (the other DT search windows)
    bool lft4twopass;   // CE3TSK: FT4 second slicing pass, as lft8twopass is for FT8
    int nft4ensemble;   // CE3TSK: FT4 ensemble members 0-6, as nft8ensemble counts FT8's (decodepreset.h)
    bool lft4deeposd;   // CE3TSK: OSD order 2 for every FT4 candidate, as lft8deeposd is for FT8 (item 58)
    int nft4bgensemble; // CE3TSK: the FT4 TX background's target member count, 0 off (item 59)
    int nft4bgdepth;    // CE3TSK item 69: the FT4 TX background's effort 1-3, 0 = the RX phase's
    bool lft4bgdeeposd; // CE3TSK item 69: deep OSD in the FT4 TX background
    bool lft4bgaltpass; // CE3TSK item 69: alternate pass in the FT4 TX background
    bool lft4bgtwopass; // CE3TSK item 69: second slicing pass in the FT4 TX background
    bool lft4bgresidual;// CE3TSK item 72: the residual unit in the FT4 TX background (FT8's: all known decodes subtracted, threshold 0.8, once more)
    int nft4sens;       // CE3TSK item 72: FT4 decoder sensitivity, 0 JTDX's thresholds, 1 sync minimum 1.0 + sync quality 16
    int nft4bgsens;     // CE3TSK item 73: the same for the FT4 TX background phase (the residual scales from it)
    int nft4rxfsens;    // CE3TSK item 75: FT4 QSO RX frequency sensitivity 0-3 (the virtual candidate at the QSO frequency)
    int nft4bgrxfsens;  // CE3TSK item 75: the same for the FT4 TX background phase
    /* CE3TSK: the decode request counter. jtdxjt9 used to be triggered purely by the .lock
       file going from present to absent, and jt9a's wait after a decode treats "absent" as
       "keep sleeping" - so a removal that landed before the decoder reached that wait was lost
       and both sides waited for each other for ever (the GUI for <DecodeFinished>, the decoder
       for the lock to reappear). decode() increments this before publishing the block; the
       decoder serves any value it has not served yet. A value cannot be missed the way an edge
       can. .lock still does what it is good at: stopping the TX background. */
    int ndecreq;
    int nft4bgeffort;   // CE3TSK item 78: the FT4 TX background switch, FT8's nft8bgeffort mirrored - 0 off, 1 on. The
                        //   phase runs when this is set, whatever the member target: the members above the RX count,
                        //   or with none left the phase's own extras alone (deep OSD, alternate pass, residual)
    int nsftol;         // CE3TSK: SuperFox receive (SuperHound, SUPERFOX_PLAN.md) - 0 off; N > 0 on, with N Hz the
                        //   search range either side of nfqso for the SuperFox sync tone. When on, and lhound, the
                        //   Fox's (even) slot is decoded by the SuperFox receiver instead of the FT8 decoder
    } params;
} dec_data_t;

// for unknown reason values of the variables at beginning of dec_data list are being
// not updated while Decode button is pushed manually, for decoding again keep variables at end
// of the list

#ifdef __cplusplus
extern "C" {
#endif

extern struct {
  float wave[606720];
} foxcom_;

#define NUM_JT65_SYMBOLS 126               //63 data + 63 sync
#define NUM_JT9_SYMBOLS 85                 //69 data + 16 sync
#define NUM_T10_SYMBOLS 85                 //69 data + 16 sync
#define NUM_WSPR_SYMBOLS 162               //(50+31)*2, embedded sync
#define NUM_FT8_SYMBOLS 79
#define NUM_FT4_SYMBOLS 105

#define NUM_CW_SYMBOLS 250
#define TX_SAMPLE_RATE 48000

extern int volatile itone[NUM_WSPR_SYMBOLS];   //Audio tones for all Tx symbols
extern int volatile icw[NUM_CW_SYMBOLS];	    //Dits for CW ID


#ifdef __cplusplus
}
#endif

#endif // COMMONS_H

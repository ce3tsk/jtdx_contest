#ifndef WAV16TO32_H
#define WAV16TO32_H

#include <QtGlobal>

/* CE3TSK: the two sample-depth conversions behind File -> Open and File -> Convert bit depth.

   JTDX carries 16 bit samples (short d2[] in commons.h, integer(c_short) id2 in jt9com.f90) -
   upstream's layout, restored 2026-09-03 after a year on the jtdx_32a 32 bit chain
   (AUDIO_DEPTH_PLAN.md: 73 on-air periods decode identically at either depth). Recordings the
   fork saved in that year are 32 bit, so wav32to16 is what File -> Open uses on them, and
   wav16to32 serves the converter.

   wav32to16 keeps the top 16 bits: an arithmetic shift, i.e. floor division by 65536, the same
   rule as test/wav/make16.sh, so a 32 bit recording decodes exactly as its 16 bit twin. Right
   shifting a negative value is implementation-defined before C++20 and arithmetic on every
   compiler this builds with; the cast back to qint16 takes the low half of the result. It is
   never used in place - read_wav_file() goes through a scratch buffer.

   wav16to32 is designed for in place use: the 16 bit data parked in the tail of the
   destination buffer (src as a byte address at least 2*frames above dst) and expanded
   forwards. Under that precondition each write lands entirely below every source element not
   yet read; the one element whose write overlaps its own source bytes is safe because the value
   is read before the store. Expanding BACKWARDS from a tail parking is NOT safe - writing dst[i]
   clobbers src[i-1] before it is read - which is the intuitive way round and wrong. The shift
   maps 16 bit full scale onto 32 bit full scale; it is done in unsigned arithmetic because left
   shifting a negative value is undefined before C++20. */
inline void wav16to32 (qint16 const * src, qint32 * dst, int frames)
{
  for (int i = 0; i < frames; ++i)
    {
      qint32 const v = src[i];              // read before write: the last element overlaps
      dst[i] = qint32 (quint32 (v) << 16);
    }
}

inline void wav32to16 (qint32 const * src, qint16 * dst, int frames)
{
  for (int i = 0; i < frames; ++i) dst[i] = qint16 (src[i] >> 16);
}

#endif

#ifndef BLOCKGATHER_H
#define BLOCKGATHER_H

// CE3TSK 2026-09-26: the one piece of block gathering Detector::writeData and
// TCITransceiver::writeAudioData share - what to do when the block shrinks under a part-filled
// buffer.
//
// Both gather the input into blocks of m_samplesPerFFT * 4 frames and hand each full block to
// the down-sampler. Every mode uses 3456 samples a block and FT2 half that, so a switch INTO FT2
// can find more frames gathered than the new block holds. The loops computed the room left as the
// unsigned "block - m_bufferPos", which then wrapped: the block end was never met again and the
// input was stored past the end of the buffer until the period wrapped - production crashed on it
// (2026-09-26). Dropping the part block stopped the overrun but not the damage: 15 s is a multiple
// of 3.75 s, so a switch early in an FT8 period is NOT followed by a period reset, and every later
// sample of that FT2 period landed up to a block early in d2. Handing on the whole new-size blocks
// already gathered, in order, and keeping the rest at the front loses no frame and moves none.
//
// Header only, so the harness drives it directly; test/harness/test_detector.cpp also drives both
// loops that call it - Detector's, and TCITransceiver's through its test friend (no server needed).

#include <algorithm>
#include <cstddef>

namespace JTDX
{
  // `pos` frames are held in `buffer`; hands every whole `block` of them to `hand_on`, in order, and
  // moves the remainder to the front. Returns the new fill, which is below `block` (a zero block
  // is left alone rather than looped on).
  template <class T, class HandOn>
  std::size_t rebase_blocks (T * buffer, std::size_t pos, std::size_t block, HandOn hand_on)
  {
    if (!block) return pos;
    std::size_t done {0};
    for (; pos - done >= block; done += block) hand_on (buffer + done);
    // a forward copy to lower addresses is safe with overlap - but only when it moves: with nothing
    // handed on, the destination would lie inside the source, which std::copy does not allow
    if (done) std::copy (buffer + done, buffer + pos, buffer);
    return pos - done;
  }
}

#endif

#ifndef MQTTCODEC_H
#define MQTTCODEC_H

// CE3TSK 2026-11-16 (TODO.md 3.-260, RECEPTION_MAP_PLAN.md stage 2): MQTT 3.1.1, the part a listener needs - the feed
// of mqtt.pskreporter.info is read with it (spotfeed.cpp). Free of Qt, so that it is tested byte for byte.
//
//   out: CONNECT (a clean session, no will, no name or password), SUBSCRIBE and UNSUBSCRIBE (quality of service 0),
//        PINGREQ, DISCONNECT;
//   in:  CONNACK, SUBACK, UNSUBACK, PINGRESP and PUBLISH at quality 0 - a subscription at quality 0 is sent nothing
//        else, so anything else is not this protocol.
//
// The Reader takes the bytes as they arrive, in any pieces, and hands out whole packets. A stream that is not what it
// should be - a length over four bytes, a packet over 8192 bytes (a spot is 250), a kind no listener is sent, a
// packet whose parts do not add up to its length - is BROKEN: the Reader says why and reads no more of it, and the
// feed drops the connection. Nothing is guessed and nothing is skipped.
//
// test_spotfeed has the rules.

#include <cstddef>
#include <string>
#include <vector>

namespace mqtt_codec
{
  std::size_t const maxPacket {8192};   // the longest packet taken, without its first byte and its length

  enum Type {Connack = 2, Publish = 3, Suback = 9, Unsuback = 11, Pingresp = 13};

  namespace detail
  {
    inline void put16 (std::string& out, std::size_t n)
    {
      out.push_back (static_cast<char> ((n >> 8) & 0xFF));
      out.push_back (static_cast<char> (n & 0xFF));
    }

    inline void putText (std::string& out, std::string const& text)
    {
      put16 (out, text.size ());
      out += text;
    }

    // a packet: its first byte, its length in MQTT's seven bits a byte, its body
    inline std::string packet (unsigned char first, std::string const& body)
    {
      std::string p;
      p.push_back (static_cast<char> (first));
      std::size_t n {body.size ()};
      do
        {
          unsigned char b {static_cast<unsigned char> (n % 128)};
          n /= 128;
          if (n) b |= 0x80;
          p.push_back (static_cast<char> (b));
        }
      while (n);
      return p + body;
    }
  }

  // CONNECT: protocol "MQTT" level 4 (3.1.1), a clean session and nothing else, the keep-alive in seconds
  inline std::string connect (std::string const& clientId, unsigned keepAliveSeconds)
  {
    std::string body;
    detail::putText (body, "MQTT");
    body.push_back (4);
    body.push_back (0x02);
    detail::put16 (body, keepAliveSeconds);
    detail::putText (body, clientId);
    return detail::packet (0x10, body);
  }

  // SUBSCRIBE to these topic filters, each at quality 0
  inline std::string subscribe (unsigned packetId, std::vector<std::string> const& topics)
  {
    std::string body;
    detail::put16 (body, packetId);
    for (auto const& t : topics)
      {
        detail::putText (body, t);
        body.push_back (0);
      }
    return detail::packet (0x82, body);
  }

  inline std::string unsubscribe (unsigned packetId, std::vector<std::string> const& topics)
  {
    std::string body;
    detail::put16 (body, packetId);
    for (auto const& t : topics) detail::putText (body, t);
    return detail::packet (0xA2, body);
  }

  inline std::string pingreq () {return std::string ("\xC0\x00", 2);}
  inline std::string disconnect () {return std::string ("\xE0\x00", 2);}

  struct Packet
  {
    int type {0};
    bool sessionPresent {false};   // CONNACK
    int returnCode {0};            // CONNACK: 0 is accepted
    unsigned packetId {0};         // SUBACK, UNSUBACK
    std::vector<int> granted;      // SUBACK: 0 granted at quality 0, 0x80 refused
    std::string topic;             // PUBLISH
    std::string payload;
    bool retained {false};         // PUBLISH: the broker's kept message of the topic
  };

  class Reader
  {
  public:
    void feed (char const * data, std::size_t size)
    {
      if (m_error.empty ()) m_buffer.append (data, size);
    }

    // the stream's next whole packet; false when it has not all arrived - or the stream is broken (broken () then)
    bool next (Packet& p)
    {
      if (!m_error.empty ()) return false;
      std::size_t const have {m_buffer.size () - m_at};
      if (have < 2) return false;
      unsigned char const first {static_cast<unsigned char> (m_buffer[m_at])};
      std::size_t length {0}, factor {1}, head {1};
      for (;;)
        {
          if (head > 4) return fail ("a packet's length runs over four bytes");
          if (head >= have) return false;
          unsigned char const b {static_cast<unsigned char> (m_buffer[m_at + head])};
          length += (b & 0x7F) * factor;
          factor *= 128;
          ++head;
          if (!(b & 0x80)) break;
        }
      if (length > maxPacket) return fail ("a packet longer than 8192 bytes");
      if (have < head + length) return false;
      std::string const body {m_buffer, m_at + head, length};
      m_at += head + length;
      if (m_at > 4096)   // what was read is let go
        {
          m_buffer.erase (0, m_at);
          m_at = 0;
        }
      p = Packet {};
      p.type = first >> 4;
      int const flags {first & 0x0F};
      auto u8 = [&body] (std::size_t i) {return static_cast<unsigned> (static_cast<unsigned char> (body[i]));};
      switch (p.type)
        {
        case Connack:
          if (flags || 2 != length || (u8 (0) & 0xFE)) return fail ("a CONNACK that is none");
          p.sessionPresent = u8 (0) & 1;
          p.returnCode = static_cast<int> (u8 (1));
          return true;
        case Suback:
          if (flags || length < 3) return fail ("a SUBACK that is none");
          p.packetId = u8 (0) << 8 | u8 (1);
          for (std::size_t i = 2; i < length; ++i)
            {
              if (u8 (i) > 2 && 0x80 != u8 (i)) return fail ("a SUBACK that is none");
              p.granted.push_back (static_cast<int> (u8 (i)));
            }
          return true;
        case Unsuback:
          if (flags || 2 != length) return fail ("an UNSUBACK that is none");
          p.packetId = u8 (0) << 8 | u8 (1);
          return true;
        case Pingresp:
          if (flags || length) return fail ("a PINGRESP that is none");
          return true;
        case Publish:
          {
            if (flags & 0x0E) return fail ("a message at a quality of service that was not subscribed to");
            p.retained = flags & 1;
            if (length < 2) return fail ("a PUBLISH that is none");
            std::size_t const topic {u8 (0) << 8 | u8 (1)};
            if (!topic || 2 + topic > length) return fail ("a PUBLISH that is none");
            p.topic.assign (body, 2, topic);
            p.payload.assign (body, 2 + topic, std::string::npos);
            return true;
          }
        default:
          return fail ("a packet no listener is sent");
        }
    }

    bool broken () const {return !m_error.empty ();}
    std::string const& error () const {return m_error;}

    void reset ()   // for a new connection
    {
      m_buffer.clear ();
      m_at = 0;
      m_error.clear ();
    }

  private:
    bool fail (char const * why)
    {
      m_error = why;
      m_buffer.clear ();
      m_at = 0;
      return false;
    }

    std::string m_buffer;
    std::size_t m_at {0};
    std::string m_error;
  };
}

#endif

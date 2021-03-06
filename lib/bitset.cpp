/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2009.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */


#include "bitset.h"



namespace MR {



  const uint8_t BitSet::masks[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};



  BitSet::BitSet (const size_t b, const bool allocator) :
      bits  (b),
      bytes ((bits + 7) / 8),
      data  (new uint8_t[bytes])
  {
    memset (data, (allocator ? 0xFF : 0x00), bytes);
  }


  BitSet::BitSet (const BitSet& that) :
      bits  (that.bits),
      bytes (that.bytes),
      data  (new uint8_t[bytes])
  {
    memcpy (data, that.data, bytes);
  }


  BitSet::~BitSet() {
    delete[] data; data = NULL;
  }





  void BitSet::resize (const size_t new_size, const bool allocator)
  {

    size_t new_bits;
    new_bits = new_size;
    const size_t new_bytes = (new_bits + 7) / 8;
    uint8_t* new_data = new uint8_t[new_bytes];
    if (new_bytes > bytes) {
      memcpy (new_data, data, bytes);
      memset (new_data + bytes, (allocator ? 0xFF : 0x00), new_bytes - bytes);
      const size_t excess_bits = bits - (8 * (bytes - 1));
      const uint8_t mask = 0xFF << excess_bits;
      data[bytes - 1] = allocator ? (data[bytes - 1] | mask) : (data[bytes - 1] & ~mask);
    } else {
      memcpy (new_data, data, new_bytes);
    }
    delete[] data;
    bits = new_bits;
    bytes = new_bytes;
    data = new_data;
    new_data = NULL;

  }


  void BitSet::clear (const bool allocator)
  {
    memset(data, (allocator ? 0xFF : 0x00), bytes);
  }


  bool BitSet::full() const
  {

    const size_t bytes_to_test = (bits % 8) ? bytes - 1 : bytes;
    for (size_t i = 0; i != bytes_to_test; ++i) {
      if (data[i] != 0xFF)
        return false;
    }
    if (!(bits % 8))
      return true;

    const size_t excess_bits = bits - (8 * (bytes - 1));
    const uint8_t mask = 0xFF << excess_bits;
    if ((data[bytes - 1] | mask) != 0xFF)
      return false;
    return true;

  }


  bool BitSet::empty() const
  {

    const size_t bytes_to_test = (bits % 8) ? bytes - 1 : bytes;
    for (size_t i = 0; i != bytes_to_test; ++i) {
      if (data[i])
        return false;
    }
    if (!(bits % 8))
      return true;

    const size_t excess_bits = bits - (8 * (bytes - 1));
    const uint8_t mask = ~(0xFF << excess_bits);
    if (data[bytes - 1] & mask)
      return false;
    return true;

  }


  size_t BitSet::count () const
  {
    size_t count = 0;
    for (size_t i = 0; i != bits; ++i) {
      if (test (i))
        ++count;
    }
    return count;
  }







  BitSet& BitSet::operator= (const BitSet& that)
  {
    delete[] data;
    bits = that.bits;
    bytes = that.bytes;
    data = new uint8_t[bytes];
    memcpy (data, that.data, bytes);
    return *this;
  }


  bool BitSet::operator== (const BitSet& that) const
  {
    if (bits != that.bits)
      return false;
    if (bits % bytes) {
      if (memcmp(data, that.data, bytes - 1))
        return false;

      const size_t excess_bits = bits - (8 * (bytes - 1));
      const uint8_t mask = ~(0xFF << excess_bits);
      if ((data[bytes - 1] & mask) != (that.data[bytes - 1] & mask))
        return false;

      return true;
    } else {
      return (!memcmp (data, that.data, bytes));
    }
  }


  bool BitSet::operator!= (const BitSet& that) const
  {
    return (!(*this == that));
  }


  BitSet& BitSet::operator|= (const BitSet& that)
  {
    if (bits != that.bits)
      throw Exception ("\nFIXME: Illegal BitSet '|' operator call - size mismatch\n");
    for (size_t i = 0; i != bytes; ++i)
      data[i] |= that.data[i];
    return *this;
  }


  BitSet& BitSet::operator&= (const BitSet& that)
    {
    if (bits != that.bits)
      throw Exception ("\nFIXME: Illegal BitSet '&' operator call - size mismatch\n");
    for (size_t i = 0; i != bytes; ++i)
      data[i] &= that.data[i];
    return *this;
    }


  BitSet& BitSet::operator^= (const BitSet& that)
  {
    if (bits != that.bits)
      throw Exception ("\nFIXME: Illegal BitSet '^' operator call - size mismatch\n");
    for (size_t i = 0; i != bytes; ++i)
      data[i] ^= that.data[i];
    return *this;
  }


  BitSet BitSet::operator| (const BitSet& that) const
  {
    BitSet result (*this);
    result |= that;
    return result;
  }


  BitSet BitSet::operator& (const BitSet& that) const
  {
    BitSet result (*this);
    result &= that;
    return result;
  }


  BitSet BitSet::operator^ (const BitSet& that) const
  {
    BitSet result (*this);
    result ^= that;
    return result;
  }


  BitSet BitSet::operator~() const
  {
    BitSet result (*this);
    for (size_t i = 0; i != bytes; ++i)
      result.data[i] = ~data[i];
    return result;
  }






}

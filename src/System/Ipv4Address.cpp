// Copyright (c) 2021-2022, Dynex Developers
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Parts of this project are originally copyright by:
// Copyright (c) 2012-2016, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero project
// Copyright (c) 2014-2018, The Forknote developers
// Copyright (c) 2018, The TurtleCoin developers
// Copyright (c) 2016-2018, The Karbowanec developers
// Copyright (c) 2017-2022, The CROAT.community developers


#include "Ipv4Address.h"
#include <stdexcept>
#include "android.h"

namespace System {

namespace {

uint8_t readUint8(const std::string& source, size_t& offset) {
  if (offset == source.size() || source[offset] < '0' || source[offset] > '9') {
    throw std::runtime_error("Unable to read value from string");
  }

  uint8_t value = source[offset] - '0';
  if (offset + 1 == source.size() || source[offset + 1] < '0' || source[offset + 1] > '9') {
    offset = offset + 1;
    return value;
  }

  if (value == 0) {
    throw std::runtime_error("Unable to read value from string");
  }

  value = value * 10 + (source[offset + 1] - '0');
  if (offset + 2 == source.size() || source[offset + 2] < '0' || source[offset + 2] > '9') {
    offset = offset + 2;
    return value;
  }

  if ((value == 25 && source[offset + 2] > '5') || value > 25) {
    throw std::runtime_error("Unable to read value from string");
  }

  value = value * 10 + (source[offset + 2] - '0');
  offset = offset + 3;
  return value;
}

}

Ipv4Address::Ipv4Address(uint32_t value) : value(value) {
}

Ipv4Address::Ipv4Address(const std::string& dottedDecimal) {
  size_t offset = 0;
  value = readUint8(dottedDecimal, offset);
  if (offset == dottedDecimal.size() || dottedDecimal[offset] != '.') {
    throw std::runtime_error("Invalid Ipv4 address string");
  }

  ++offset;
  value = value << 8 | readUint8(dottedDecimal, offset);
  if (offset == dottedDecimal.size() || dottedDecimal[offset] != '.') {
    throw std::runtime_error("Invalid Ipv4 address string");
  }

  ++offset;
  value = value << 8 | readUint8(dottedDecimal, offset);
  if (offset == dottedDecimal.size() || dottedDecimal[offset] != '.') {
    throw std::runtime_error("Invalid Ipv4 address string");
  }

  ++offset;
  value = value << 8 | readUint8(dottedDecimal, offset);
  if (offset < dottedDecimal.size()) {
    throw std::runtime_error("Invalid Ipv4 address string");
  }
}

bool Ipv4Address::operator!=(const Ipv4Address& other) const {
  return value != other.value;
}

bool Ipv4Address::operator==(const Ipv4Address& other) const {
  return value == other.value;
}

uint32_t Ipv4Address::getValue() const {
  return value;
}

std::string Ipv4Address::toDottedDecimal() const {
  std::string result;
  result += std::to_string(value >> 24);
  result += '.';
  result += std::to_string(value >> 16 & 255);
  result += '.';
  result += std::to_string(value >> 8 & 255);
  result += '.';
  result += std::to_string(value & 255);
  return result;
}

bool Ipv4Address::isLoopback() const {
  // 127.0.0.0/8
  return (value & 0xff000000) == (127 << 24);
}

bool Ipv4Address::isPrivate() const {
  return
    // 10.0.0.0/8
    (int)(value & 0xff000000) == (int)(10 << 24) ||
    // 172.16.0.0/12
    (int)(value & 0xfff00000) == (int)((172 << 24) | (16 << 16)) ||
    // 192.168.0.0/16
    (int)(value & 0xffff0000) == (int)((192 << 24) | (168 << 16));
}

}

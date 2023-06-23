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


#include "HttpParser.h"

#include <algorithm>

#include "HttpParserErrorCodes.h"

namespace {

void throwIfNotGood(std::istream& stream) {
  if (!stream.good()) {
    if (stream.eof()) {
      throw std::system_error(make_error_code(CryptoNote::error::HttpParserErrorCodes::END_OF_STREAM));
    } else {
      throw std::system_error(make_error_code(CryptoNote::error::HttpParserErrorCodes::STREAM_NOT_GOOD));
    }
  }
}

}

namespace CryptoNote {

HttpResponse::HTTP_STATUS HttpParser::parseResponseStatusFromString(const std::string& status) {
  if (status == "200 OK" || status == "200 Ok") return CryptoNote::HttpResponse::STATUS_200;
  else if (status.substr(0, 4) == "401 ") return CryptoNote::HttpResponse::STATUS_401;
  else if (status == "404 Not Found") return CryptoNote::HttpResponse::STATUS_404;
  else if (status == "500 Internal Server Error") return CryptoNote::HttpResponse::STATUS_500;
  else throw std::system_error(make_error_code(CryptoNote::error::HttpParserErrorCodes::UNEXPECTED_SYMBOL),
      "Unknown HTTP status code is given");

  return CryptoNote::HttpResponse::STATUS_200; //unaccessible
}


void HttpParser::receiveRequest(std::istream& stream, HttpRequest& request) {
  readWord(stream, request.method);
  readWord(stream, request.url);

  std::string httpVersion;
  readWord(stream, httpVersion);

  readHeaders(stream, request.headers);

  std::string body;
  size_t bodyLen = getBodyLen(request.headers);
  if (bodyLen) {
    readBody(stream, request.body, bodyLen);
  }
}


void HttpParser::receiveResponse(std::istream& stream, HttpResponse& response) {
  std::string httpVersion;
  readWord(stream, httpVersion);
  
  std::string status;
  char c;
  
  stream.get(c);
  while (stream.good() && c != '\r') { //Till the end
    status += c;
    stream.get(c);
  }

  throwIfNotGood(stream);

  if (c == '\r') {
    stream.get(c);
    if (c != '\n') {
      throw std::runtime_error("Parser error: '\\n' symbol is expected");
    }
  }

  response.setStatus(parseResponseStatusFromString(status));
  
  std::string name;
  std::string value;

  while (readHeader(stream, name, value)) {
    response.addHeader(name, value);
    name.clear();
    value.clear();
  }

  response.addHeader(name, value);
  auto headers = response.getHeaders();
  size_t length = 0;
  auto it = headers.find("content-length");
  if (it != headers.end()) {
    length = std::stoul(it->second);
  }
  
  std::string body;
  if (length) {
    readBody(stream, body, length);
  }

  response.setBody(body);
}


void HttpParser::readWord(std::istream& stream, std::string& word) {
  char c;

  stream.get(c);
  while (stream.good() && c != ' ' && c != '\r') {
    word += c;
    stream.get(c);
  }

  throwIfNotGood(stream);

  if (c == '\r') {
    stream.get(c);
    if (c != '\n') {
      throw std::system_error(make_error_code(CryptoNote::error::HttpParserErrorCodes::UNEXPECTED_SYMBOL));
    }
  }
}

void HttpParser::readHeaders(std::istream& stream, HttpRequest::Headers& headers) {
  std::string name;
  std::string value;

  while (readHeader(stream, name, value)) {
    headers[name] = value; //use insert
    name.clear();
    value.clear();
  }

  headers[name] = value; //use insert
}

bool HttpParser::readHeader(std::istream& stream, std::string& name, std::string& value) {
  char c;
  bool isName = true;

  stream.get(c);
  while (stream.good() && c != '\r') {
    if (c == ':') {
      if (stream.peek() == ' ') {
        stream.get(c);
      }

      if (name.empty()) {
        throw std::system_error(make_error_code(CryptoNote::error::HttpParserErrorCodes::EMPTY_HEADER));
      }

      if (isName) {
        isName = false;
        stream.get(c);
        continue;
      }
    }

    if (isName) {
      name += c;
      stream.get(c);
    } else {
      value += c;
      stream.get(c);
    }
  }

  throwIfNotGood(stream);

  stream.get(c);
  if (c != '\n') {
    throw std::system_error(make_error_code(CryptoNote::error::HttpParserErrorCodes::UNEXPECTED_SYMBOL));
  }

  std::transform(name.begin(), name.end(), name.begin(), ::tolower);

  c = stream.peek();
  if (c == '\r') {
    stream.get(c).get(c);
    if (c != '\n') {
      throw std::system_error(make_error_code(CryptoNote::error::HttpParserErrorCodes::UNEXPECTED_SYMBOL));
    }

    return false; //no more headers
  }

  return true;
}

size_t HttpParser::getBodyLen(const HttpRequest::Headers& headers) {
  auto it = headers.find("content-length");
  if (it != headers.end()) {
    size_t bytes = std::stoul(it->second);
    return bytes;
  }

  return 0;
}

void HttpParser::readBody(std::istream& stream, std::string& body, const size_t bodyLen) {
  size_t read = 0;

  while (stream.good() && read < bodyLen) {
    body += stream.get();
    ++read;
  }

  throwIfNotGood(stream);
}

}

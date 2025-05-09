/******************************************************************************
 *
 * Project:  OpenCPN
 *
 ***************************************************************************
 *   Copyright (C) 2019 Alec Leamas                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 ***************************************************************************
 */
#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <sys/stat.h>

#ifdef __MSVC__
#include <io.h>
#include <direct.h>
#include <stdlib.h>
#else
#include <unistd.h>
#endif

#include "model/ocpn_utils.h"

namespace ocpn {

bool endswith(const std::string& str, const std::string& suffix) {
  return str.size() >= suffix.size() &&
         0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

bool startswith(const std::string& str, const std::string& prefix) {
  return prefix.size() <= str.size() &&
         strncmp(str.c_str(), prefix.c_str(), prefix.size()) == 0;
}

std::vector<std::string> split(const char* token_string,
                               const std::string& delimiter) {
  std::vector<std::string> tokens;
  std::string s = std::string(token_string);
  size_t pos = 0;
  std::string token;
  while ((pos = s.find(delimiter)) != std::string::npos) {
    token = s.substr(0, pos);
    tokens.push_back(token);
    s.erase(0, pos + delimiter.length());
  }
  tokens.push_back(s);
  return tokens;
}

std::vector<std::string> split(const std::string& string,
                               const std::string& delimiter) {
  return split(string.c_str(), delimiter);
}

bool exists(const std::string& name) {
#ifdef __MSVC__
  return (_access(name.c_str(), 0) != -1);
#else
  return (access(name.c_str(), F_OK) != -1);
#endif
}

/** @deprecated Using std::filesystem instead. */
void mkdir(const std::string path) {
#if defined(_WIN32) && !defined(__MINGW32__)
  _mkdir(path.c_str());
#elif defined(__MINGW32__)
  ::mkdir(path.c_str());
#else
  ::mkdir(path.c_str(), 0755);
#endif
}

std::string ltrim(const std::string& arg) {
  using namespace std;

  string s(arg);
  s.erase(s.begin(),
          find_if(s.begin(), s.end(), [](int ch) { return !isspace(ch); }));
  return s;
}

std::string rtrim(const std::string& arg) {
  using namespace std;

  string s(arg);
  s.erase(
      find_if(s.rbegin(), s.rend(), [](int ch) { return !isspace(ch); }).base(),
      s.end());
  return s;
}

std::string trim(std::string s) {
  s = ltrim(s);
  s = rtrim(s);
  return s;
}

std::string join(std::vector<std::string> v, char c) {
  std::string s;
  for (auto p = v.begin(); p != v.end(); p++) {
    s += *p;
    if (p != v.end() - 1) {
      s += c;
    }
  }
  return s;
}

std::string tolower(const std::string& input) {
  std::string s(input);
  std::transform(s.begin(), s.end(), s.begin(), ::tolower);
  return s;
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos) return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

void copy_file(const std::string& src_path, const std::string& dest_path) {
  std::ifstream source(src_path, std::ios::binary);
  std::ofstream dest(dest_path, std::ios::binary);

  dest << source.rdbuf();

  source.close();
  dest.close();
}

bool N0183CheckSumOk(const std::string& sentence) {
  size_t check_start = sentence.find('*');
  if (check_start == std::string::npos || check_start > sentence.size() - 3)
    return false;  // * not found, or it didn't have 2 characters following it.

  std::string check_str = sentence.substr(check_start + 1, 2);
  unsigned long checksum = strtol(check_str.c_str(), 0, 16);
  if (checksum == 0L && check_str != "00") return false;

  unsigned char calculated_checksum = 0;
  for (std::string::const_iterator i = sentence.begin() + 1;
       i != sentence.end() && *i != '*'; ++i)
    calculated_checksum ^= static_cast<unsigned char>(*i);

  return calculated_checksum == checksum;
}

std::string printable(const std::string& str) {
  std::stringstream ss;
  for (auto it = str.begin(); it != str.end(); it++) {
    if (std::isprint(*it) && *it != '\r' && *it != '\n') {
      ss << *it;
    } else {
      std::stringstream ss2;
      ss2 << std::setw(2) << std::setfill('0') << std::uppercase << std::hex
          << static_cast<int>(*it);
      ss << "<" << ss2.str() << ">";
    }
  }
  return ss.str();
}

}  // namespace ocpn

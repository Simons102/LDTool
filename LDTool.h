/*
    LDTool - LEGO Dimensions tag crypto stuff
    Copyright (C) 2022  Simon Sievert

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LD_TOOL_H
#define LD_TOOL_H

#include <Arduino.h>

#define LD_UID_BYTES 7

class LDTool {
  public:
    // 7 byte uid, 4 byte buffer to write password into
    void genPwd(byte* uid, byte* pwd);
    void encrypt(byte* uid, uint32_t characterId, uint32_t* buf);
    uint32_t decryptCharacterPages(byte* uid, uint32_t* buf);

  private:
    void uint32ToBytes(uint32_t value, byte* bytes);
    uint32_t rot32r(uint32_t value, byte amount);
    uint32_t scramble(byte* uid, byte count);
    void genTeaKey(byte* uid, uint32_t* key);
    void teaEncrypt(uint32_t v[2], const uint32_t k[4]);
    void teaDecrypt(uint32_t v[2], const uint32_t k[4]);
    uint32_t swapEndian(uint32_t value);
};

#endif // LD_TOOL_H

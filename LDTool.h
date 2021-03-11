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

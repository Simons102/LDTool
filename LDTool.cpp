#include "LDTool.h"

// generate password based on uid
void LDTool::genPwd(byte* uid, byte* pwd) {
    // 32 bytes buffer
    byte base[] = "UUUUUUU(c) Copyright LEGO 2014AA";
    // copy uid into beginning of buffer
    memcpy(base, uid, 7);
    base[30] = 0xaa;
    base[31] = 0xaa;

    // will hold final password value
    uint32_t v2 = 0;
    // calculate password
    for (byte i = 0; i < 8; i++) {
        uint32_t v4 = rot32r(v2, 25);
        uint32_t v5 = rot32r(v2, 10);
        uint32_t b = ((uint32_t)base[i * 4 + 3] << 24) |
                     ((uint32_t)base[i * 4 + 2] << 16) |
                     ((uint32_t)base[i * 4 + 1] << 8) | (uint32_t)base[i * 4];
        v2 = b + v4 + v5 - v2;
    }

    // convert password to bytes
    uint32ToBytes(v2, pwd);
}

uint32_t LDTool::rot32r(uint32_t value, byte amount) {
    amount %= 32;
    return (value << (32 - amount)) | (value >> amount);
}

void LDTool::uint32ToBytes(uint32_t value, byte* bytes) {
    memset(bytes, 0, 4);
    bytes[3] = value >> 24;
    bytes[2] = value >> 16;
    bytes[1] = value >> 8;
    bytes[0] = value;
}

uint32_t LDTool::scramble(byte* uid, byte count) {
    if (!(count >= 0 && count <= 8)) {
        // invalid count range
        return 0;
    }

    byte base[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xb7,
                   0xd5, 0xd7, 0xe6, 0xe7, 0xba, 0x3c, 0xa8, 0xd8,
                   0x75, 0x47, 0x68, 0xcf, 0x23, 0xe9, 0xfe, 0xaa};

    memcpy(base, uid, LD_UID_BYTES);
    base[(count * 4) - 1] = 0xaa;

    uint32_t v2 = 0;
    for (byte i = 0; i < count; i++) {
        uint32_t v4 = rot32r(v2, 25);
        uint32_t v5 = rot32r(v2, 10);
        uint32_t b = ((uint32_t)base[i * 4 + 3] << 24) |
                     ((uint32_t)base[i * 4 + 2] << 16) |
                     ((uint32_t)base[i * 4 + 1] << 8) | (uint32_t)base[i * 4];
        v2 = b + v4 + v5 - v2;
    }

    // not sure if this has to have it's endianness swapped
    return v2;
}

void LDTool::genTeaKey(byte* uid, uint32_t* key) {
    key[0] = scramble(uid, 3);
    key[1] = scramble(uid, 4);
    key[2] = scramble(uid, 5);
    key[3] = scramble(uid, 6);

    for (byte i = 0; i < 4; i++) {
        key[i] = swapEndian(key[i]);
    }
}

// Tiny Encryption Algorithm (TEA)
// copied from https://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm

void LDTool::teaEncrypt(uint32_t v[2], const uint32_t k[4]) {
    uint32_t v0 = v[0], v1 = v[1], sum = 0, i; /* set up */
    uint32_t delta = 0x9E3779B9;               /* a key schedule constant */
    uint32_t k0 = k[0], k1 = k[1], k2 = k[2], k3 = k[3]; /* cache key */
    for (i = 0; i < 32; i++) {                           /* basic cycle start */
        sum += delta;
        v0 += ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
        v1 += ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
    } /* end cycle */
    v[0] = v0;
    v[1] = v1;
}

void LDTool::teaDecrypt(uint32_t v[2], const uint32_t k[4]) {
    uint32_t v0 = v[0], v1 = v[1], sum = 0xC6EF3720,
             i;                  /* set up; sum is 32*delta */
    uint32_t delta = 0x9E3779B9; /* a key schedule constant */
    uint32_t k0 = k[0], k1 = k[1], k2 = k[2], k3 = k[3]; /* cache key */
    for (i = 0; i < 32; i++) {                           /* basic cycle start */
        v1 -= ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
        v0 -= ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
        sum -= delta;
    } /* end cycle */
    v[0] = v0;
    v[1] = v1;
}

// buf should be written on pages 36 (0x24) and 37 (0x25)
void LDTool::encrypt(byte* uid, uint32_t characterId, uint32_t* buf) {
    uint32_t key[4];
    genTeaKey(uid, key);

    buf[0] = buf[1] = characterId;

    for (byte i = 0; i < 4; i++) {
        key[i] = swapEndian(key[i]);
    }

    teaEncrypt(buf, key);

    for (byte i = 0; i < 2; i++) {
        buf[i] = swapEndian(buf[i]);
    }
}

uint32_t LDTool::decryptCharacterPages(byte* uid, uint32_t* buf) {
    uint32_t key[4];
    genTeaKey(uid, key);

    for (byte i = 0; i < 4; i++) {
        key[i] = swapEndian(key[i]);
    }

    for (byte i = 0; i < 2; i++) {
        buf[i] = swapEndian(buf[i]);
    }

    teaDecrypt(buf, key);

    if (buf[0] == buf[1]) {
        return buf[0];
    }

    return 0;
}

uint32_t LDTool::swapEndian(uint32_t value) {
    byte bytes[4];
    memset(bytes, 0, 4);
    bytes[3] = value >> 24;
    bytes[2] = value >> 16;
    bytes[1] = value >> 8;
    bytes[0] = value;

    return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) |
           ((uint32_t)bytes[2] << 8) | (uint32_t)bytes[3];
}

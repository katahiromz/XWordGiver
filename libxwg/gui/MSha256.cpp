// MSha256.cpp -- SHA-256 hash generator
////////////////////////////////////////////////////////////////////////////

#include "MSha256.h"

static const uint32_t s_sha256_h[8] =
{
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f,
    0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

static const uint32_t s_sha256_k[64] =
{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b,
    0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
    0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
    0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
    0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
    0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
    0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
    0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
    0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

////////////////////////////////////////////////////////////////////////////
// inline functions

static inline uint32_t mzc_hi_long(uint64_t q)
{
    return static_cast<uint32_t>(q >> 32);
}

static inline uint32_t mzc_lo_long(uint64_t q)
{
    return static_cast<uint32_t>(q);
}

static inline uint32_t mzc_shr(uint32_t x, size_t n)
{
    assert(n < 32);
    return (x >> n);
}

static inline uint32_t mzc_rotr(uint32_t x, size_t n)
{
    assert(n < 32);
    return static_cast<uint32_t>((x >> n) | (x << (32 - n)));
}

static inline uint32_t mzc_ch(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (~x & z);
}

static inline uint32_t mzc_maj(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y)^(x & z)^(y & z);
}

static inline uint32_t mzc_bsig0(uint32_t x)
{
    return mzc_rotr(x, 2) ^ mzc_rotr(x, 13) ^ mzc_rotr(x, 22);
}

static inline uint32_t mzc_bsig1(uint32_t x)
{
    return mzc_rotr(x, 6) ^ mzc_rotr(x, 11) ^ mzc_rotr(x, 25);
}

static inline uint32_t mzc_ssig0(uint32_t x)
{
    return mzc_rotr(x, 7) ^ mzc_rotr(x, 18) ^ mzc_shr(x, 3);
}

static inline uint32_t mzc_ssig1(uint32_t x)
{
    return mzc_rotr(x, 17) ^ mzc_rotr(x, 19) ^ mzc_shr(x, 10);
}

////////////////////////////////////////////////////////////////////////////
// MSha256

MSha256::MSha256()
{
    init();
}

MSha256::MSha256(const char *salt)
{
    assert(salt);
    init();
    add_data(salt, std::strlen(salt));
}

void MSha256::init()
{
    assert(sizeof(m_h) == sizeof(s_sha256_h));
    memcpy(m_h, s_sha256_h, sizeof(s_sha256_h));

    m_iw = 0;
    m_len = 0;
    memset(m_w, 0, 256);
}

void MSha256::_update_table()
{
    uint32_t x[8];

    memcpy(x, m_h, 32);
    uint32_t t1, t2;
    for (int t = 0; t < 64; t += 8)
    {
#define MZC_ROUND(n, a, b, c, d, e, f, g, h) do { \
    t1 = x[h] + mzc_bsig1(x[e]) + mzc_ch(x[e], x[f], x[g]) + s_sha256_k[t + n] + m_w[t + n]; \
    t2 = mzc_bsig0(x[a]) + mzc_maj(x[a], x[b], x[c]); \
    x[d] += t1; \
    x[h] = t1 + t2; \
} while (0)
        MZC_ROUND(0, 0, 1, 2, 3, 4, 5, 6, 7);
        MZC_ROUND(1, 7, 0, 1, 2, 3, 4, 5, 6);
        MZC_ROUND(2, 6, 7, 0, 1, 2, 3, 4, 5);
        MZC_ROUND(3, 5, 6, 7, 0, 1, 2, 3, 4);
        MZC_ROUND(4, 4, 5, 6, 7, 0, 1, 2, 3);
        MZC_ROUND(5, 3, 4, 5, 6, 7, 0, 1, 2);
        MZC_ROUND(6, 2, 3, 4, 5, 6, 7, 0, 1);
        MZC_ROUND(7, 1, 2, 3, 4, 5, 6, 7, 0);
#undef MZC_ROUND
    }

    for (int i = 0; i < 8; ++i)
        m_h[i] += x[i];
}

void MSha256::add_data(const void* ptr, size_t len)
{
    assert(ptr || len == 0);
	uint32_t dwLen = static_cast<uint32_t>(len);

    const uint8_t *pb = reinterpret_cast<const uint8_t *>(ptr);

push_top:
    uint32_t index = 0;
    uint32_t t_4;

    for (t_4 = m_iw; (m_iw % 4) != 0 && index < dwLen; ++t_4)
    {
        m_w[t_4 / 4] |= pb[index] << (24 - (t_4 % 4) * 8);
        ++index;
    }
    m_iw = t_4;

    uint32_t t;
    for (t = m_iw / 4; t < 16 && index + 3 < dwLen; ++t)
    {
        m_w[t] |= (pb[index] << 24) |
                  (pb[index + 1] << 16) |
                  (pb[index + 2] << 8) |
                  (pb[index + 3]);
        index += 4;
    }

    for (t_4 = t * 4; t_4 < 64 && index < dwLen; ++t_4)
    {
        m_w[t_4 / 4] |= pb[index++] << (24 - (t_4 % 4) * 8);
    }
    m_iw = t_4;

    if (t_4 == 64)
    {
        for (int t = 16; t < 64; ++t)
        {
            m_w[t] = mzc_ssig1(m_w[t - 2]) + m_w[t - 7] +
                     mzc_ssig0(m_w[t - 15]) + m_w[t - 16];
        }
        _update_table();

        m_iw = 0;
        memset(m_w, 0, 256);
        m_len += index * 8;
        if (index < dwLen)
        {
            pb += index;
            dwLen -= index;
            goto push_top;
        }
    }
    else
    {
        m_len += dwLen * 8;
    }
}

void MSha256::generate_binary(void *p32bytes)
{
    assert(p32bytes);
    uint8_t *pb = reinterpret_cast<uint8_t *>(p32bytes);

    m_w[m_iw / 4] |= 0x80000000 >> ((m_iw % 4) * 8);
    m_iw++;
    while (m_iw % 4)
    {
        ++m_iw;
    }

    if (m_iw > 56)
    {
        for (int t = 16; t < 64; ++t)
        {
            m_w[t] = mzc_ssig1(m_w[t - 2]) + m_w[t - 7] +
                     mzc_ssig0(m_w[t - 15]) + m_w[t - 16];
        }
        _update_table();
        m_iw = 0;
    }

    for (int t = m_iw / 4; t < 14; ++t)
    {
        m_w[t] = 0;
    }

    m_w[14] = mzc_hi_long(m_len);
    m_w[15] = mzc_lo_long(m_len);

    for (int t = 16; t < 64; ++t)
    {
        m_w[t] = mzc_ssig1(m_w[t - 2])  + m_w[t - 7] +
                 mzc_ssig0(m_w[t - 15]) + m_w[t - 16];
    }
    _update_table();

    for (int t = 0; t < 32; t++)
    {
        pb[t] = static_cast<uint8_t>(m_h[t / 4] >> ((3 - t % 4) * 8));
    }
}

/*static*/ void
MSha256::binary_from_passwd_and_salt(void *p32bytes, const void *pass, size_t pass_len, const char *salt)
{
    assert(pass || pass_len == 0);
    MSha256 sha256(salt);
    sha256.add_data(pass, pass_len);
    sha256.generate_binary(p32bytes);
}

/*static*/ void
MSha256::binary_from_passwd_and_salt(void *p32bytes, const char *pass, const char *salt)
{
    binary_from_passwd_and_salt(p32bytes, pass, std::strlen(pass), salt);
}

////////////////////////////////////////////////////////////////////////////
// test and sample

#ifdef UNITTEST
    #include <iostream>
    #include <fstream>
    #include <string>

    void print_binary(const void *p, size_t size)
    {
        static const char *s_hex = "0123456789abcdef";
        const unsigned char *pb = reinterpret_cast<const unsigned char *>(p);
        std::cout << "{";
        while (size--)
        {
            unsigned char b = *pb;
            std::cout << "0x" << s_hex[b >> 4] << s_hex[b & 0xF] << ", ";
            ++pb;
        }
        std::cout << "}" << std::endl;
    }

    int main(void)
    {
        // interactive mode
        std::string salt, line;
        std::cout << "Enter 'exit' to exit." << std::endl;

        std::cout << "Enter salt string: ";
        std::getline(std::cin, salt);
        if (salt == "exit" || salt == "quit")
            return 0;

        std::string result;
        for (;;)
        {
            std::cout << "Enter password: ";
            std::getline(std::cin, line);
            if (line == "exit")
                break;

            MSha256::hex_from_passwd_and_salt(result, line.data(), salt.data());
            std::cout << result << std::endl;

            char binary[32];
            MSha256::binary_from_passwd_and_salt(binary, line.data(), salt.data());
            print_binary(binary, 32);
        }

        return 0;
    }
#endif  // def UNITTEST

////////////////////////////////////////////////////////////////////////////

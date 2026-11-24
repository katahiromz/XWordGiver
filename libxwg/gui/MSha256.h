// MSha256.h -- SHA-256 hash generator
////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>  // for uint8_t, uint32_t, uint64_t
#include <cstring>  // for std::strlen
#include <cassert>  // for assert macro
#include <iterator> // for std::distance

////////////////////////////////////////////////////////////////////////////
// mzc_hex_string_from_bytes --- Create a hex string from binary

template <typename T_STRING, typename T_ITER>
void mzc_hex_string_from_bytes(T_STRING& str, T_ITER begin_, T_ITER end_)
{
    typedef typename T_STRING::value_type T_CHAR;
    static const char s_hex[] = "0123456789abcdef";
    for (str.clear(); begin_ != end_; ++begin_)
    {
        const unsigned char by = *begin_;
        str += static_cast<T_CHAR>(s_hex[by >> 4]);
        str += static_cast<T_CHAR>(s_hex[by & 0x0F]);
    }
}

template <typename T_STRING, typename T_CONTAINER>
inline void mzc_hex_string_from_bytes(T_STRING& str, const T_CONTAINER& bytes)
{
    mzc_hex_string_from_bytes(str, bytes.begin(), bytes.end());
}

////////////////////////////////////////////////////////////////////////////
// MSha256 class --- SHA-256 hash generator class

class MSha256
{
public:
    MSha256();
    MSha256(const char *salt);

    void init();
    void add_data(const void* ptr, size_t len);

    template <typename T_ITER>
    void add_data(T_ITER begin_, T_ITER end_)
    {
        std::size_t len = std::distance(begin_, end_) * sizeof(*begin_);
        add_data(&*begin_, len);
    }

    void generate_binary(void* p32bytes);

    template <typename T_STRING>
    void generate_hash_string(T_STRING& str)
    {
        char hash[32];
        generate_binary(hash);
        mzc_hex_string_from_bytes(str, hash, hash + 32);
    }

    template <typename T_STRING>
    static void
    hex_from_passwd_and_salt(T_STRING& str, const void *pass, size_t pass_len, const char *salt = "")
    {
        assert(pass || pass_len == 0);
        MSha256 sha256(salt);
        sha256.add_data(pass, pass_len);
        sha256.generate_hash_string(str);
    }

    template <typename T_STRING, typename T_ITER>
    static void
    hex_from_passwd_and_salt(T_STRING& str, T_ITER begin_, T_ITER end_, const char *salt = "")
    {
        assert(salt);
        MSha256 sha256(salt);
        sha256.add_data(begin_, end_);
        sha256.generate_hash_string(str);
    }

    template <typename T_STRING>
    static void
    hex_from_passwd_and_salt(T_STRING& str, const char *pass, const char *salt = "")
    {
        assert(pass);
        MSha256 sha256(salt);
        sha256.add_data(salt, std::strlen(salt));
        sha256.add_data(pass, std::strlen(pass));
        sha256.generate_hash_string(str);
    }

    // NOTE: p32bytes must be a pointer to 32 bytes
    static void binary_from_passwd_and_salt(void *p32bytes, const void *pass, size_t pass_len, const char *salt = "");
    static void binary_from_passwd_and_salt(void *p32bytes, const char *pass, const char *salt = "");

protected:
    uint32_t m_h[8], m_w[64], m_iw;
    uint64_t m_len;

    void _update_table();
};

////////////////////////////////////////////////////////////////////////////

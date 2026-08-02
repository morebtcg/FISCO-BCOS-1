#pragma once
#include <bcos-utilities/Common.h>
#include <bcos-utilities/Exceptions.h>
#include <algorithm>
#include <random>

// bcos-security is used to storage security

namespace bcos
{
namespace security
{
DERIVE_BCOS_EXCEPTION(KeyCenterAlreadyInit);
DERIVE_BCOS_EXCEPTION(KeyCenterDataKeyError);
DERIVE_BCOS_EXCEPTION(KeyCenterConnectionError);
DERIVE_BCOS_EXCEPTION(KeyCenterCall);
DERIVE_BCOS_EXCEPTION(KeyCenterInitError);
DERIVE_BCOS_EXCEPTION(KeyCenterCloseError);
DERIVE_BCOS_EXCEPTION(EncryptedFileError);
DERIVE_BCOS_EXCEPTION(EncryptedLevelDBEncryptFailed);
DERIVE_BCOS_EXCEPTION(EncryptedLevelDBDecryptFailed);
DERIVE_BCOS_EXCEPTION(EncryptFailed);
DERIVE_BCOS_EXCEPTION(DecryptFailed);
DERIVE_BCOS_EXCEPTION(KmsTypeError);
DERIVE_BCOS_EXCEPTION(NotImplementedError);

// MSVC 的 <random> 要求 independent_bits_engine 的 UIntType 是 unsigned short/int/
// long/long long 之一（不允许 char/uint8_t，见 C2338）。用 unsigned int 生成
// CHAR_BIT(8) 位随机值，赋给 unsigned char 时取低 8 位即可。
using random_bytes_engine =
    std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned int>;

}  // namespace security

}  // namespace bcos
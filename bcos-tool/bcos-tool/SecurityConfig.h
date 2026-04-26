#pragma once

#include "bcos-framework/security/CloudKmsType.h"
#include "bcos-framework/security/KeyEncryptionType.h"
#include "bcos-framework/security/StorageEncryptionType.h"
#include <boost/property_tree/ptree_fwd.hpp>
#include <string>

namespace bcos::tool
{
class SecurityConfig
{
public:
    const std::string& privateKeyPath() const { return m_privateKeyPath; }
    const std::string& hsmLibPath() const { return m_hsmLibPath; }
    const int& keyIndex() const { return m_keyIndex; }
    const int& encKeyIndex() const { return m_encKeyIndex; }
    const std::string& password() const { return m_password; }
    bool storageSecurityEnable() const { return m_storageSecurityEnable; }
    const std::string& storageSecurityUrl() const { return m_storageSecurityUrl; }
    const std::string& storageSecurityCipherDataKey() const
    {
        return m_storageSecurityCipherDataKey;
    }
    security::KeyEncryptionType keyEncryptionType() const { return m_keyEncryptionType; }
    security::StorageEncryptionType storageEncryptionType() const { return m_storageEncryptionType; }
    security::CloudKmsType cloudKmsType() const { return m_cloudKmsType; }
    const std::string& bcosKmsKeySecurityCipherDataKey() const
    {
        return m_bcosKmsKeySecurityCipherDataKey;
    }
    const std::string& keyEncryptionUrl() const { return m_keyEncryptionUrl; }

    void setPrivateKeyPath(const std::string& privateKeyPath) { m_privateKeyPath = privateKeyPath; }
    void setHsmLibPath(const std::string& hsmLibPath) { m_hsmLibPath = hsmLibPath; }
    void setKeyIndex(int keyIndex) { m_keyIndex = keyIndex; }
    void setEncKeyIndex(int encKeyIndex) { m_encKeyIndex = encKeyIndex; }
    void setPassword(const std::string& password) { m_password = password; }
    void setStorageSecurityEnable(bool storageSecurityEnable)
    {
        m_storageSecurityEnable = storageSecurityEnable;
    }
    void setStorageSecurityUrl(const std::string& storageSecurityUrl)
    {
        m_storageSecurityUrl = storageSecurityUrl;
    }
    void setStorageSecurityCipherDataKey(const std::string& storageSecurityCipherDataKey)
    {
        m_storageSecurityCipherDataKey = storageSecurityCipherDataKey;
    }
    void setKeyEncryptionType(security::KeyEncryptionType keyEncryptionType)
    {
        m_keyEncryptionType = keyEncryptionType;
    }
    void setStorageEncryptionType(security::StorageEncryptionType storageEncryptionType)
    {
        m_storageEncryptionType = storageEncryptionType;
    }
    void setCloudKmsType(security::CloudKmsType cloudKmsType) { m_cloudKmsType = cloudKmsType; }
    void setBcosKmsKeySecurityCipherDataKey(const std::string& cipherDataKey)
    {
        m_bcosKmsKeySecurityCipherDataKey = cipherDataKey;
    }
    void setKeyEncryptionUrl(const std::string& keyEncryptionUrl)
    {
        m_keyEncryptionUrl = keyEncryptionUrl;
    }

    void loadSecurityConfig(boost::property_tree::ptree const& config);
    void loadStorageSecurityConfig(boost::property_tree::ptree const& config);

private:
    std::string m_privateKeyPath;
    std::string m_hsmLibPath;
    int m_keyIndex = 0;
    int m_encKeyIndex = 0;
    std::string m_password;
    bool m_storageSecurityEnable = false;
    std::string m_storageSecurityUrl;
    std::string m_storageSecurityCipherDataKey;
    security::KeyEncryptionType m_keyEncryptionType = security::KeyEncryptionType::LEGACY;
    security::StorageEncryptionType m_storageEncryptionType =
        security::StorageEncryptionType::LEGACY;
    security::CloudKmsType m_cloudKmsType = security::CloudKmsType::AWS;
    std::string m_bcosKmsKeySecurityCipherDataKey;
    std::string m_keyEncryptionUrl;
};
}  // namespace bcos::tool
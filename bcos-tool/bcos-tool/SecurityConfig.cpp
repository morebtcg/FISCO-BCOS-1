/**
 *  Copyright (C) 2021 FISCO BCOS.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @brief security configuration loading implementation
 * @file SecurityConfig.cpp
 */

#include "SecurityConfig.h"
#include "Exceptions.h"
#include "bcos-utilities/BoostLog.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/throw_exception.hpp>

#define SecurityConfig_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("NodeConfig")

using namespace bcos;
using namespace bcos::tool;

namespace
{
struct SecuritySelection
{
    security::KeyEncryptionType keyEncryptionType = security::KeyEncryptionType::LEGACY;
    std::string keyEncryptionUrl;
    std::string bcosKmsKeySecurityCipherDataKey;
    bool storageSecurityEnable = false;
};

struct StorageSecuritySelection
{
    security::StorageEncryptionType storageEncryptionType =
        security::StorageEncryptionType::LEGACY;
    std::string storageSecurityUrl;
    std::string storageSecurityCipherDataKey;
};

security::KeyEncryptionType parseKeyEncryptionType(
    std::string const& keyEncryptionTypeStr, std::string const& privateKeyPath)
{
    auto keyEncryptionTypeOption = magic_enum::enum_cast<security::KeyEncryptionType>(
        keyEncryptionTypeStr, magic_enum::case_insensitive);
    if (keyEncryptionTypeOption.has_value())
    {
        return keyEncryptionTypeOption.value();
    }

    SecurityConfig_LOG(ERROR) << LOG_DESC("loadSecurityConfig")
                              << LOG_KV("privateKeyPath", privateKeyPath)
                              << LOG_KV("keyEncryptionType", keyEncryptionTypeStr);
    BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment("Please set kms_type to LEGACY!"));
}

security::StorageEncryptionType parseStorageEncryptionType(
    std::string const& storageEncryptionTypeStr)
{
    auto storageEncryptionTypeOption = magic_enum::enum_cast<security::StorageEncryptionType>(
        storageEncryptionTypeStr, magic_enum::case_insensitive);
    if (storageEncryptionTypeOption.has_value())
    {
        return storageEncryptionTypeOption.value();
    }

    SecurityConfig_LOG(ERROR) << LOG_DESC("loadStorageSecurityConfig")
                              << LOG_KV("storageEncryptionType", storageEncryptionTypeStr);
    BOOST_THROW_EXCEPTION(
        InvalidConfig() << errinfo_comment("Please set kms_type to LEGACY or BCOSKMS!"));
}

security::CloudKmsType parseCloudKmsType(
    std::string const& privateKeyPath, security::KeyEncryptionType keyEncryptionType,
    std::string const& cloudKmsTypeStr)
{
    auto cloudKmsTypeOption =
        magic_enum::enum_cast<security::CloudKmsType>(cloudKmsTypeStr, magic_enum::case_insensitive);
    if (cloudKmsTypeOption.has_value())
    {
        return cloudKmsTypeOption.value();
    }

    SecurityConfig_LOG(ERROR) << LOG_DESC("loadSecurityConfig")
                              << LOG_KV("privateKeyPath", privateKeyPath)
                              << LOG_KV(
                                     "keyEncryptionType", std::string(magic_enum::enum_name(keyEncryptionType)));
    BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment("Please set cloud_kms_type with AWS!"));
}

SecuritySelection loadSecuritySelection(
    boost::property_tree::ptree const& config, std::string const& privateKeyPath,
    SecurityConfig& securityConfig)
{
    std::string keyEncryptionTypeStr = config.get<std::string>("security.kms_type", "LEGACY");
    SecuritySelection selection;
    selection.keyEncryptionType = parseKeyEncryptionType(keyEncryptionTypeStr, privateKeyPath);
    selection.keyEncryptionUrl = config.get<std::string>("security.kms_connection_str", "");
    securityConfig.setKeyEncryptionType(selection.keyEncryptionType);
    securityConfig.setKeyEncryptionUrl(selection.keyEncryptionUrl);
    return selection;
}

void loadLegacyBcosKmsSecuritySelection(boost::property_tree::ptree const& config,
    std::string const& privateKeyPath, SecurityConfig& securityConfig, SecuritySelection& selection)
{
    selection.keyEncryptionType = security::KeyEncryptionType::BCOSKMS;
    auto keyCenterUrl = config.get<std::string>("storage_security.key_center_url", "");
    selection.bcosKmsKeySecurityCipherDataKey =
        config.get<std::string>("storage_security.cipher_data_key", "");
    if (keyCenterUrl.empty() || selection.bcosKmsKeySecurityCipherDataKey.empty())
    {
        SecurityConfig_LOG(ERROR) << LOG_DESC("loadSecurityConfig default with bcos kms failed!")
                                  << LOG_KV("key_center_url", keyCenterUrl)
                                  << LOG_KV("cipher_data_key",
                                         selection.bcosKmsKeySecurityCipherDataKey);
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "Please provide key_center_url and cipher_data_key!"));
    }

    selection.keyEncryptionUrl = keyCenterUrl;
    securityConfig.setBcosKmsKeySecurityCipherDataKey(selection.bcosKmsKeySecurityCipherDataKey);
    securityConfig.setKeyEncryptionUrl(selection.keyEncryptionUrl);
    SecurityConfig_LOG(INFO) << LOG_DESC("loadSecurityConfig LEGACY")
                             << LOG_KV("privateKeyPath", privateKeyPath)
                             << LOG_KV("keyEncryptionType",
                                    std::string(magic_enum::enum_name(selection.keyEncryptionType)))
                             << LOG_KV("m_KeyEncryptionUrl", selection.keyEncryptionUrl);
}

void loadLegacyHsmSecuritySelection(
    std::string const& privateKeyPath, SecurityConfig& securityConfig, SecuritySelection& selection)
{
    SecurityConfig_LOG(INFO) << LOG_DESC("loadSecurityConfig LEGACY")
                             << LOG_KV("privateKeyPath", privateKeyPath)
                             << LOG_KV("keyEncryptionType",
                                    std::string(magic_enum::enum_name(selection.keyEncryptionType)));
    selection.keyEncryptionType = security::KeyEncryptionType::HSM;
    securityConfig.setKeyEncryptionType(selection.keyEncryptionType);
}

void normalizeLegacySecuritySelection(boost::property_tree::ptree const& config,
    std::string const& privateKeyPath, bool enableHsm, SecurityConfig& securityConfig,
    SecuritySelection& selection)
{
    if (selection.keyEncryptionType != security::KeyEncryptionType::LEGACY)
    {
        return;
    }

    if (selection.storageSecurityEnable)
    {
        loadLegacyBcosKmsSecuritySelection(config, privateKeyPath, securityConfig, selection);
    }

    if (!enableHsm)
    {
        return;
    }

    loadLegacyHsmSecuritySelection(privateKeyPath, securityConfig, selection);
}

void loadHsmSecurityConfig(boost::property_tree::ptree const& config, SecurityConfig& securityConfig)
{
    auto hsmLibPath =
        config.get<std::string>("security.hsm_lib_path", "/usr/local/lib/libgmt0018.so");
    auto keyIndex = config.get<int>("security.key_index");
    auto encKeyIndex = config.get<int>("security.enc_key_index", keyIndex);
    auto password = config.get<std::string>("security.password", "");
    securityConfig.setHsmLibPath(hsmLibPath);
    securityConfig.setKeyIndex(keyIndex);
    securityConfig.setEncKeyIndex(encKeyIndex);
    securityConfig.setPassword(password);
    SecurityConfig_LOG(INFO) << LOG_DESC("loadSecurityConfig HSM") << LOG_KV("lib_path", hsmLibPath)
                             << LOG_KV("key_index", keyIndex)
                             << LOG_KV("enc_key_index", encKeyIndex)
                             << LOG_KV("password", password);
}

void loadCloudKmsSecurityConfig(boost::property_tree::ptree const& config,
    std::string const& privateKeyPath, security::KeyEncryptionType keyEncryptionType,
    SecurityConfig& securityConfig)
{
    auto cloudKmsTypeStr = config.get<std::string>("security.cloud_kms_type", "");
    auto cloudKmsType = parseCloudKmsType(privateKeyPath, keyEncryptionType, cloudKmsTypeStr);
    securityConfig.setCloudKmsType(cloudKmsType);
    SecurityConfig_LOG(INFO) << LOG_DESC("loadSecurityConfig")
                             << LOG_KV("privateKeyPath", privateKeyPath)
                             << LOG_KV("keyEncryptionType",
                                    std::string(magic_enum::enum_name(keyEncryptionType)))
                             << LOG_KV("cloudKmsType", std::string(magic_enum::enum_name(cloudKmsType)));
}

void ensureBcosKmsCipherDataKey(boost::property_tree::ptree const& config,
    std::string const& privateKeyPath, security::KeyEncryptionType keyEncryptionType,
    SecuritySelection& selection, SecurityConfig& securityConfig)
{
    if (selection.bcosKmsKeySecurityCipherDataKey.empty())
    {
        selection.bcosKmsKeySecurityCipherDataKey =
            config.get<std::string>("security.cipher_data_key", "");
    }
    securityConfig.setBcosKmsKeySecurityCipherDataKey(selection.bcosKmsKeySecurityCipherDataKey);

    if (!selection.bcosKmsKeySecurityCipherDataKey.empty())
    {
        return;
    }

    SecurityConfig_LOG(ERROR) << LOG_DESC("loadSecurityConfig")
                              << LOG_KV("privateKeyPath", privateKeyPath)
                              << LOG_KV(
                                     "keyEncryptionType", std::string(magic_enum::enum_name(keyEncryptionType)));
    BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment("Please provide cipher_data_key!"));
}

StorageSecuritySelection loadStorageSecuritySelection(
    boost::property_tree::ptree const& config, SecurityConfig& securityConfig)
{
    std::string storageEncryptionTypeStr =
        config.get<std::string>("storage_security.kms_type", "LEGACY");
    StorageSecuritySelection selection;
    selection.storageEncryptionType = parseStorageEncryptionType(storageEncryptionTypeStr);
    selection.storageSecurityUrl = config.get<std::string>("storage_security.kms_connection_str", "");
    securityConfig.setStorageEncryptionType(selection.storageEncryptionType);
    securityConfig.setStorageSecurityUrl(selection.storageSecurityUrl);
    return selection;
}

void normalizeLegacyStorageSecuritySelection(boost::property_tree::ptree const& config,
    StorageSecuritySelection& selection, SecurityConfig& securityConfig)
{
    if (selection.storageEncryptionType != security::StorageEncryptionType::LEGACY)
    {
        return;
    }

    selection.storageEncryptionType = security::StorageEncryptionType::BCOSKMS;
    auto keyCenterUrl = config.get<std::string>("storage_security.key_center_url", "");
    if (keyCenterUrl.empty())
    {
        SecurityConfig_LOG(ERROR) << LOG_DESC("loadStorageSecurityConfig default with bcos kms failed!")
                                  << LOG_KV("key_center_url", keyCenterUrl);
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "Please provide key_manager_ip and key_manager_port!"));
    }
    selection.storageSecurityUrl = keyCenterUrl;
    securityConfig.setStorageSecurityUrl(selection.storageSecurityUrl);
    SecurityConfig_LOG(INFO) << LOG_DESC("loadStorageSecurityConfig BCOSKMS")
                             << LOG_KV("storageEncryptionType",
                                    ("security::StorageEncryptionType::LEGACY"))
                             << LOG_KV("m_storageSecurityUrl", selection.storageSecurityUrl);
}

void ensureStorageSecurityCipherDataKey(boost::property_tree::ptree const& config,
    StorageSecuritySelection& selection, SecurityConfig& securityConfig)
{
    selection.storageSecurityCipherDataKey =
        config.get<std::string>("storage_security.cipher_data_key", "");
    securityConfig.setStorageSecurityCipherDataKey(selection.storageSecurityCipherDataKey);
    if (selection.storageSecurityCipherDataKey.empty())
    {
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment("Please provide cipher_data_key!"));
    }
}
}  // namespace

void SecurityConfig::loadSecurityConfig(boost::property_tree::ptree const& config)
{
    auto privateKeyPath = config.get<std::string>("security.private_key_path", "node.pem");
    setPrivateKeyPath(privateKeyPath);
    auto selection = loadSecuritySelection(config, privateKeyPath, *this);

    bool enableHsm = config.get<bool>("security.enable_hsm", false);
    selection.storageSecurityEnable = config.get<bool>("storage_security.enable", false);
    setStorageSecurityEnable(selection.storageSecurityEnable);
    normalizeLegacySecuritySelection(config, privateKeyPath, enableHsm, *this, selection);

    if (selection.keyEncryptionType == security::KeyEncryptionType::HSM)
    {
        loadHsmSecurityConfig(config, *this);
    }
    else if (selection.keyEncryptionType == security::KeyEncryptionType::CLOUDKMS)
    {
        loadCloudKmsSecurityConfig(config, privateKeyPath, selection.keyEncryptionType, *this);
    }
    else if (selection.keyEncryptionType == security::KeyEncryptionType::BCOSKMS)
    {
        ensureBcosKmsCipherDataKey(
            config, privateKeyPath, selection.keyEncryptionType, selection, *this);
    }
    else if (selection.keyEncryptionType == security::KeyEncryptionType::LEGACY)
    {
        SecurityConfig_LOG(INFO) << LOG_DESC("loadSecurityConfig")
                                 << LOG_KV("privateKeyPath", privateKeyPath)
                                 << LOG_KV("keyEncryptionType",
                                        std::string(magic_enum::enum_name(selection.keyEncryptionType)));
    }
    else
    {
        SecurityConfig_LOG(ERROR) << LOG_DESC("loadSecurityConfig")
                                  << LOG_KV("privateKeyPath", privateKeyPath)
                                  << LOG_KV("keyEncryptionType",
                                         std::string(magic_enum::enum_name(selection.keyEncryptionType)));
        BOOST_THROW_EXCEPTION(InvalidConfig() << errinfo_comment(
                                  "Please set kms_type to DEFAULT or HSM or CLOUDKMS or BCOSKMS!"));
    }

    setKeyEncryptionType(selection.keyEncryptionType);
    setKeyEncryptionUrl(selection.keyEncryptionUrl);

    SecurityConfig_LOG(INFO) << LOG_DESC("loadSecurityConfig")
                             << LOG_KV("privateKeyPath", privateKeyPath)
                             << LOG_KV("keyEncryptionType",
                                    std::string(magic_enum::enum_name(selection.keyEncryptionType)));
}

void SecurityConfig::loadStorageSecurityConfig(boost::property_tree::ptree const& config)
{
    auto storageSecurityEnable = config.get<bool>("storage_security.enable", false);
    setStorageSecurityEnable(storageSecurityEnable);
    if (!storageSecurityEnable)
    {
        return;
    }

    auto selection = loadStorageSecuritySelection(config, *this);
    normalizeLegacyStorageSecuritySelection(config, selection, *this);
    ensureStorageSecurityCipherDataKey(config, selection, *this);
    setStorageEncryptionType(selection.storageEncryptionType);
    SecurityConfig_LOG(INFO) << LOG_DESC("loadStorageSecurityConfig")
                             << LOG_KV("m_storageSecurityUrl", selection.storageSecurityUrl);
}
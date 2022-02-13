// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>

#include "core/hle/service/nfp/amiibo_types.h"

struct mbedtls_md_context_t;

namespace Service::NFP::AmiiboCrypto {
constexpr std::size_t HMAC_POS_DATA = 0x8;
constexpr std::size_t HMAC_POS_TAG = 0x1B4;

using HmacKey = std::array<u8, 0x10>;
using DrgbOutput = std::array<u8, 0x20>;

struct HashSeed {
    union {
        std::array<u8, 0x40> raw;
        struct {
            u16 magic;
            std::array<u8, 0xE> padding;
            std::array<u8, 0x8> uuid1;
            std::array<u8, 0x8> uuid2;
            std::array<u8, 0x20> keygen_salt;
        } data;
    };
};
static_assert(sizeof(HashSeed) == 0x40, "HashSeed is an invalid size");

struct InternalKey {
    HmacKey hmac_key;
    std::array<char, 0xE> type_string;
    u8 reserved;
    u8 magic_length;
    std::array<u8, 0x10> magic_bytes;
    std::array<u8, 0x20> xor_pad;
};
static_assert(sizeof(InternalKey) == 0x50, "InternalKey is an invalid size");
static_assert(std::is_trivially_copyable_v<InternalKey>, "InternalKey must be trivially copyable.");

struct CryptoCtx {
    std::array<char, 480> buffer;
    bool used;
    std::size_t buffer_size;
    s16 counter;
};

struct DerivedKeys {
    std::array<u8, 0x10> aes_key;
    std::array<u8, 0x10> aes_iv;
    std::array<u8, 0x10> hmac_key;
};
static_assert(sizeof(DerivedKeys) == 0x30, "DerivedKeys is an invalid size");

/// Converts mii data from nintendo 3ds format to nintendo switch format
Service::Mii::MiiInfo AmiiboRegisterInfoToMii(const AmiiboRegisterInfo& register_info);

/// Validates that the amiibo file is not corrupted
bool IsAmiiboValid(const EncryptedNTAG215File& ntag_file);

/// Converts from encrypted file format to encoded file format
NTAG215File NfcDataToEncodedData(const EncryptedNTAG215File& nfc_data);

/// Converts from encoded file format to encrypted file format
EncryptedNTAG215File EncodedDataToNfcData(const NTAG215File& encoded_data);

/// Returns password needed to allow write access to protected memory
u32 GetTagPassword(const TagUuid& uuid);

// Generates Seed needed for key derivation
HashSeed GetSeed(const NTAG215File& data);

// Middle step on the generation of derived keys
void PreGenerateKey(const InternalKey& key, const HashSeed& seed, u8* output,
                    std::size_t& outputLen);

// Initializes mbedtls context
void CryptoInit(CryptoCtx& ctx, mbedtls_md_context_t& hmac_ctx, const HmacKey& hmac_key,
                const u8* seed, std::size_t seed_size);

// Feeds data to mbedtls context to generate the derived key
void CryptoStep(CryptoCtx& ctx, mbedtls_md_context_t& hmac_ctx, DrgbOutput& output);

// Generates the derived key from amiibo data
DerivedKeys GenerateKey(const InternalKey& key, const NTAG215File& data);

// Encodes or decodes amiibo data
void Cipher(const DerivedKeys& keys, const NTAG215File& in_data, NTAG215File& out_data);

/// Loads both amiibo keys from key_retail.bin
bool LoadKeys(InternalKey& locked_secret, InternalKey& unfixed_info);

/// Decodes encripted amiibo data returns true if output is valid
bool DecodeAmiibo(const EncryptedNTAG215File& encrypted_tag_data, NTAG215File& tag_data);

/// Encodes plain amiibo data returns true if output is valid
bool EncodeAmiibo(const NTAG215File& tag_data, EncryptedNTAG215File& encrypted_tag_data);

} // namespace Service::NFP::AmiiboCrypto

/************************************************************
 *
 *                 OPEN TRANSACTIONS
 *
 *       Financial Cryptography and Digital Cash
 *       Library, Protocol, API, Server, CLI, GUI
 *
 *       -- Anonymous Numbered Accounts.
 *       -- Untraceable Digital Cash.
 *       -- Triple-Signed Receipts.
 *       -- Cheques, Vouchers, Transfers, Inboxes.
 *       -- Basket Currencies, Markets, Payment Plans.
 *       -- Signed, XML, Ricardian-style Contracts.
 *       -- Scripted smart contracts.
 *
 *  EMAIL:
 *  fellowtraveler@opentransactions.org
 *
 *  WEBSITE:
 *  http://www.opentransactions.org/
 *
 *  -----------------------------------------------------
 *
 *   LICENSE:
 *   This Source Code Form is subject to the terms of the
 *   Mozilla Public License, v. 2.0. If a copy of the MPL
 *   was not distributed with this file, You can obtain one
 *   at http://mozilla.org/MPL/2.0/.
 *
 *   DISCLAIMER:
 *   This program is distributed in the hope that it will
 *   be useful, but WITHOUT ANY WARRANTY; without even the
 *   implied warranty of MERCHANTABILITY or FITNESS FOR A
 *   PARTICULAR PURPOSE.  See the Mozilla Public License
 *   for more details.
 *
 ************************************************************/
#if OT_CRYPTO_USING_TREZOR

#include "opentxs/core/crypto/TrezorCrypto.hpp"

#include "opentxs/core/app/App.hpp"
#include "opentxs/core/crypto/CryptoEngine.hpp"
#include "opentxs/core/crypto/CryptoHash.hpp"
#include "opentxs/core/crypto/CryptoSymmetric.hpp"
#include "opentxs/core/crypto/Ecdsa.hpp"
#include "opentxs/core/crypto/OTAsymmetricKey.hpp"
#include "opentxs/core/crypto/OTPassword.hpp"
#include "opentxs/core/crypto/OTPasswordData.hpp"
#include "opentxs/core/util/Assert.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/OTData.hpp"
#include "opentxs/core/Proto.hpp"
#include "opentxs/core/String.hpp"

extern "C" {
#if OT_CRYPTO_WITH_BIP39
#include <trezor-crypto/bip39.h>
#if OT_CRYPTO_WITH_BIP32
#include <trezor-crypto/bignum.h>
#include <trezor-crypto/curves.h>
#endif
#endif
}

#include <stdint.h>
#include <array>

namespace opentxs
{
#if OT_CRYPTO_WITH_BIP39
std::string TrezorCrypto::toWords(const OTPassword& seed) const
{
    std::string wordlist(
        ::mnemonic_from_data(
            static_cast<const uint8_t*>(seed.getMemory()),
            seed.getMemorySize()));
    return wordlist;
}

void TrezorCrypto::WordsToSeed(
    const std::string words,
    OTPassword& seed,
    const std::string passphrase) const
{
    OT_ASSERT_MSG(!words.empty(), "Mnemonic was blank.");
    OT_ASSERT_MSG(!passphrase.empty(), "Passphrase was blank.");

    seed.SetSize(512/8);

    ::mnemonic_to_seed(
        words.c_str(),
        passphrase.c_str(),
        static_cast<uint8_t*>(seed.getMemoryWritable()),
        nullptr);
}
#endif // OT_CRYPTO_WITH_BIP39

#if OT_CRYPTO_WITH_BIP32
TrezorCrypto::TrezorCrypto()
  : secp256k1_(get_curve_by_name(CurveName(EcdsaCurve::SECP256K1).c_str()))
{
    OT_ASSERT(nullptr != secp256k1_);
}

std::string TrezorCrypto::SeedToFingerprint(
    const EcdsaCurve& curve,
    const OTPassword& seed) const
{
    auto node = InstantiateHDNode(curve, seed);

    if (node) {
        OTData pubkey(
            static_cast<void*>(node->public_key),
            sizeof(node->public_key));
        Identifier identifier;
        identifier.CalculateDigest(pubkey);
        String fingerprint(identifier);

        return fingerprint.Get();
    }

    return "";
}

serializedAsymmetricKey TrezorCrypto::SeedToPrivateKey(
    const EcdsaCurve& curve,
    const OTPassword& seed) const
{
    serializedAsymmetricKey derivedKey;
    auto node = InstantiateHDNode(curve, seed);

    OT_ASSERT_MSG(node, "Derivation of root node failed.");

    if (node) {
        derivedKey = HDNodeToSerialized(
            CryptoAsymmetric::CurveToKeyType(curve),
            *node,
            TrezorCrypto::DERIVE_PRIVATE);

        if (derivedKey) {
            OTPassword root;
            App::Me().Crypto().Hash().Digest(
                proto::HASHTYPE_BTC160,
                seed,
                root);
            derivedKey->mutable_path()->set_root(
                root.getMemory(),
                root.getMemorySize());
        }
    }

    return derivedKey;
}

serializedAsymmetricKey TrezorCrypto::GetChild(
    const proto::AsymmetricKey& parent,
    const uint32_t index) const
{
    auto node = SerializedToHDNode(parent);

    if (proto::KEYMODE_PRIVATE == parent.mode()) {
        hdnode_private_ckd(node.get(), index);
    } else {
        hdnode_public_ckd(node.get(), index);
    }
    serializedAsymmetricKey key = HDNodeToSerialized(
        parent.type(),
        *node,
        TrezorCrypto::DERIVE_PRIVATE);

    return key;
}

serializedAsymmetricKey TrezorCrypto::HDNodeToSerialized(
    const proto::AsymmetricKeyType& type,
    const HDNode& node,
    const DerivationMode privateVersion) const
{
    serializedAsymmetricKey key = std::make_shared<proto::AsymmetricKey>();

    key->set_version(1);
    key->set_type(type);

    if (privateVersion) {
        key->set_mode(proto::KEYMODE_PRIVATE);
        key->set_chaincode(node.chain_code, sizeof(node.chain_code));

        OTPassword plaintextKey;
        plaintextKey.setMemory(node.private_key, sizeof(node.private_key));
        OTData encryptedKey;
        BinarySecret masterPassword(
            App::Me().Crypto().AES().InstantiateBinarySecretSP());
        masterPassword = CryptoSymmetric::GetMasterKey("");

        bool encrypted = Ecdsa::EncryptPrivateKey(
            plaintextKey,
            *masterPassword,
            encryptedKey);

        if (encrypted) {
            key->set_key(encryptedKey.GetPointer(), encryptedKey.GetSize());
        }
    } else {
        key->set_mode(proto::KEYMODE_PUBLIC);
        key->set_key(node.public_key, sizeof(node.public_key));
    }

    return key;
}

std::unique_ptr<HDNode> TrezorCrypto::InstantiateHDNode(const EcdsaCurve& curve)
{
    auto entropy = App::Me().Crypto().AES().InstantiateBinarySecretSP();

    OT_ASSERT_MSG(entropy, "Failed to obtain entropy.");

    entropy->randomizeMemory(256/8);

    auto output = InstantiateHDNode(curve, *entropy);

    OT_ASSERT(output);

    output->depth = 0;
    output->fingerprint = 0;
    output->child_num = 0;
    OTPassword::zeroMemory(output->chain_code, sizeof(output->chain_code));
    OTPassword::zeroMemory(output->private_key, sizeof(output->private_key));
    OTPassword::zeroMemory(output->public_key, sizeof(output->public_key));

    return output;
}

std::unique_ptr<HDNode> TrezorCrypto::InstantiateHDNode(
    const EcdsaCurve& curve,
    const OTPassword& seed)
{
    std::unique_ptr<HDNode> output;
    output.reset(new HDNode);

    OT_ASSERT_MSG(output, "Instantiation of HD node failed.");

    auto curveName = CurveName(curve);

    if (1 > curveName.size()) { return output; }

    int result = ::hdnode_from_seed(
        static_cast<const uint8_t*>(seed.getMemory()),
        seed.getMemorySize(),
        CurveName(curve).c_str(),
        output.get());

    OT_ASSERT_MSG((1 == result), "Setup of HD node failed.");

    ::hdnode_fill_public_key(output.get());

    return output;
}

std::unique_ptr<HDNode> TrezorCrypto::SerializedToHDNode(
    const proto::AsymmetricKey& serialized) const
{
    auto node = InstantiateHDNode(
        CryptoAsymmetric::KeyTypeToCurve(serialized.type()));

    OTPassword::safe_memcpy(
        &(node->chain_code[0]),
        sizeof(node->chain_code),
        serialized.chaincode().c_str(),
        serialized.chaincode().size(),
        false);

    if (proto::KEYMODE_PRIVATE == serialized.mode()) {

        OTData encryptedKey(
            serialized.key().c_str(),
            serialized.key().size());
        BinarySecret plaintextKey(
            App::Me().Crypto().AES().InstantiateBinarySecretSP());
        BinarySecret masterPassword(
            App::Me().Crypto().AES().InstantiateBinarySecretSP());
        masterPassword = CryptoSymmetric::GetMasterKey("");

        Ecdsa::DecryptPrivateKey(
            encryptedKey,
            *masterPassword,
            *plaintextKey);

        OTPassword::safe_memcpy(
            &(node->private_key[0]),
            sizeof(node->private_key),
            plaintextKey->getMemory(),
            plaintextKey->getMemorySize(),
            false);
    } else {
        OTPassword::safe_memcpy(
            &(node->public_key[0]),
            sizeof(node->public_key),
            serialized.key().c_str(),
            serialized.key().size(),
            false);
    }

    return node;
}

std::string TrezorCrypto::CurveName(const EcdsaCurve& curve)
{
    switch (curve) {
        case (EcdsaCurve::SECP256K1) : {
            return ::SECP256K1_NAME;
        }
        case (EcdsaCurve::ED25519) : {
            return ::ED25519_NAME;
        }
        default : {}
    }

    return "";
}

bool TrezorCrypto::RandomKeypair(
    OTPassword& privateKey,
    OTData& publicKey) const
{
    bool valid = false;

    do {
        privateKey.randomizeMemory(256/8);

        if (ValidPrivateKey(privateKey)) {
            valid = true;
        }
    } while (false == valid);

    return ScalarBaseMultiply(privateKey, publicKey);
}

bool TrezorCrypto::ValidPrivateKey(const OTPassword& key) const
{
    std::unique_ptr<bignum256> input(new bignum256);
    std::unique_ptr<bignum256> max(new bignum256);

    OT_ASSERT(input);
    OT_ASSERT(max);

    bn_read_be(key.getMemory_uint8(), input.get());
    bn_normalize(input.get());

    bn_read_be(KeyMax, max.get());
    bn_normalize(max.get());

    const bool zero = bn_is_zero(input.get());

    const bool size = bn_is_less(input.get(), max.get());

    return (!zero && size);
}

bool TrezorCrypto::ECDH(
    const OTData& publicKey,
    const OTPassword& privateKey,
    OTPassword& secret) const
{
    OT_ASSERT(secp256k1_);

    curve_point point;

    const bool havePublic = ecdsa_read_pubkey(
        secp256k1_->params,
        static_cast<const uint8_t*>(publicKey.GetPointer()),
        &point);

    if (!havePublic) {
        otErr << __FUNCTION__ << ": Invalid public key." << std::endl;

        return false;
    }

    bignum256 scalar;
    bn_read_be(privateKey.getMemory_uint8(), &scalar);

    curve_point sharedSecret;
    point_multiply(
        secp256k1_->params,
        &scalar,
        &point,
        &sharedSecret);

    std::array<std::uint8_t, 32> output{};
    secret.setMemory(output.data(), sizeof(output));

    OT_ASSERT(32 == secret.getMemorySize());

    bn_write_be(
        &sharedSecret.x,
        static_cast<std::uint8_t*>(secret.getMemoryWritable()));

    return true;
}

bool TrezorCrypto::ScalarBaseMultiply(
    const OTPassword& privateKey,
    OTData& publicKey) const
{
    std::array<std::uint8_t, 33> blank{};
    publicKey.Assign(blank.data(), blank.size());

    OT_ASSERT(secp256k1_);

    ecdsa_get_public_key33(
        secp256k1_->params,
        privateKey.getMemory_uint8(),
        static_cast<std::uint8_t*>(const_cast<void*>(publicKey.GetPointer())));

    curve_point notUsed;

    return (1 == ecdsa_read_pubkey(
        secp256k1_->params,
        static_cast<const std::uint8_t*>(publicKey.GetPointer()),
        &notUsed));
}
#endif // OT_CRYPTO_WITH_BIP32
} // namespace opentxs
#endif // OT_CRYPTO_USING_TREZOR

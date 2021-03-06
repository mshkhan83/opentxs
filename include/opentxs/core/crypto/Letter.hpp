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

#ifndef OPENTXS_CORE_CRYPTO_LETTER_HPP
#define OPENTXS_CORE_CRYPTO_LETTER_HPP

#include "opentxs/core/crypto/CryptoSymmetric.hpp"
#include "opentxs/core/crypto/OTASCIIArmor.hpp"
#include "opentxs/core/crypto/OTEnvelope.hpp"
#include "opentxs/core/Contract.hpp"
#include "opentxs/core/Proto.hpp"
#include "opentxs/core/String.hpp"

#include <list>
#include <map>
#include <string>
#include <tuple>

namespace opentxs

{

class Nym;
class OTData;
class OTPasswordData;

typedef std::list<symmetricEnvelope> listOfSessionKeys;
typedef std::map<proto::AsymmetricKeyType, std::string> listOfEphemeralKeys;

/** A letter is a contract that contains the contents of an OTEnvelope along
 *  with some necessary metadata.
 */
class Letter : public Contract
{
private:
    typedef Contract ot_super;
    static const CryptoSymmetric::Mode defaultPlaintextMode_ =
        CryptoSymmetric::AES_256_GCM;
    static const CryptoSymmetric::Mode defaultSessionKeyMode_ =
        CryptoSymmetric::AES_256_GCM;
    static const proto::HashType defaultHMAC_ = proto::HASHTYPE_SHA256;
    listOfEphemeralKeys ephemeralKeys_;
    String iv_;
    String tag_;
    String plaintextMode_;
    OTASCIIArmor ciphertext_;
    listOfSessionKeys sessionKeys_;
    Letter() = delete;

protected:
    virtual int32_t ProcessXMLNode(irr::io::IrrXMLReader*& xml);

public:
    Letter(
        const listOfEphemeralKeys& ephemeralKeys,
        const String& iv,
        const String& tag,
        const String& mode,
        const OTASCIIArmor& ciphertext,
        const listOfSessionKeys& sessionKeys);
    explicit Letter(const String& input);
    virtual ~Letter();
    void Release_Letter();
    virtual void Release();
    virtual void UpdateContents();

    static bool Seal(
        const mapOfAsymmetricKeys& RecipPubKeys,
        const String& theInput,
        OTData& dataOutput);
    static bool Open(
        const OTData& dataInput,
        const Nym& theRecipient,
        String& theOutput,
        const OTPasswordData* pPWData = nullptr);

    std::string& EphemeralKey(const proto::AsymmetricKeyType& type);
    const String& IV() const;
    const String& AEADTag() const;
    CryptoSymmetric::Mode Mode() const;
    const listOfSessionKeys& SessionKeys() const;
    const OTASCIIArmor& Ciphertext() const;
};

}  // namespace opentxs

#endif  // OPENTXS_CORE_CRYPTO_LETTER_HPP

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
 *  fellowtraveler\opentransactions.org
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

#ifndef OPENTXS_CORE_APP_WALLET_HPP
#define OPENTXS_CORE_APP_WALLET_HPP

#include "opentxs/core/Nym.hpp"
#include "opentxs/core/Types.hpp"
#include "opentxs/core/contract/ServerContract.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/storage/Storage.hpp"

#include <chrono>
#include <list>
#include <map>
#include <mutex>
#include <string>
#include <tuple>

namespace opentxs
{

class App;

typedef std::shared_ptr<const class Nym> ConstNym;
typedef std::shared_ptr<const class ServerContract> ConstServerContract;
typedef std::shared_ptr<const class UnitDefinition> ConstUnitDefinition;

/** \brief This class manages instantiated contracts and provides easy access
 *  to them.
 *
 * \ingroup native
 *
 *  It includes functionality which was previously found in OTWallet, and adds
 *  new capabilities such as the ability to (optionally) automatically perform
 *  remote lookups for contracts which are not already present in the local
 *  database.
 */
class Wallet
{
private:
    typedef std::pair<std::mutex, std::shared_ptr<class Nym>> NymLock;
    typedef std::map<std::string, NymLock> NymMap;
    typedef std::map<std::string, std::shared_ptr<class ServerContract>> ServerMap;
    typedef std::map<std::string, std::shared_ptr<class UnitDefinition>> UnitMap;

    friend App;

    NymMap nym_map_;
    ServerMap server_map_;
    UnitMap unit_map_;
    std::mutex nym_map_lock_;
    std::mutex server_map_lock_;
    std::mutex unit_map_lock_;

    Wallet() = default;
    Wallet(const Wallet&) = delete;
    Wallet operator=(const Wallet&) = delete;

    /**   Save an instantiated unit definition to storage and add to internal
     *    map.
     *
     *    The smart pointer will not be initialized if the provided serialized
     *    contract is invalid.
     *
     *    \param[in] contract the instantiated UnitDefinition object
     */
    ConstUnitDefinition UnitDefinition(
        std::unique_ptr<class UnitDefinition>& contract);

    /**   Save an instantiated server contract to storage and add to internal
     *    map.
     *
     *    The smart pointer will not be initialized if the provided serialized
     *    contract is invalid.
     *
     *    \param[in] contract the instantiated ServerContract object
     */
    ConstServerContract Server(
        std::unique_ptr<ServerContract>& contract);

public:
    /**   Obtain a smart pointer to an instantiated nym.
     *
     *    The smart pointer will not be initialized if the object does not
     *    exist or is invalid.
     *
     *    If the caller is willing to accept a network lookup delay, it can
     *    specify a timeout to be used in the event that the contract can not
     *    be located in local storage and must be queried from a remote
     *    location.
     *
     *    If no timeout is specified, the remote query will still happen in the
     *    background, but this method will return immediately with a null
     *    result.
     *
     *    \param[in] id the identifier of the nym to be returned
     *    \param[in] timeout The caller can set a non-zero value here if it's
     *                     willing to wait for a network lookup. The default value
     *                     of 0 will return immediately.
     */
    ConstNym Nym(
        const Identifier& id,
        const std::chrono::milliseconds& timeout = std::chrono::milliseconds(0));

    /**   Instantiate a nym from serialized form
     *
     *    The smart pointer will not be initialized if the provided serialized
     *    contract is invalid.
     *
     *    \param[in] nym the serialized version of the contract
     */
    ConstNym Nym(const proto::CredentialIndex& nym);

    /**   Load a peer reply object
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] request the identifier of the peer reply object
     *    \param[in] box the box from which to retrive the peer object
     *    \returns A smart pointer to the object. The smart pointer will not be
     *             instantiated if the object does not exist or is invalid.
     */
    std::shared_ptr<proto::PeerReply> PeerReply(
        const Identifier& nym,
        const Identifier& reply,
        const StorageBox& box) const;

    /**   Clean up the recipient's copy of a peer reply
     *
     *    The peer reply is moved from the nym's SentPeerReply
     *    box to the FinishedPeerReply box.
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] reply the identifier of the peer reply object
     *    \returns true if the request is successfully stored
     */
    bool PeerReplyComplete(
        const Identifier& nym,
        const Identifier& reply);

    /**   Store the recipient's copy of a peer reply
     *
     *    The peer reply is stored in the SendPeerReply box for the
     *    specified nym.
     *
     *    The corresponding request is moved from the nym's IncomingPeerRequest
     *    box to the ProcessedPeerRequest box.
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] request the identifier of the corresponding request
     *    \param[in] reply the serialized peer reply object
     *    \returns true if the request is successfully stored
     */
    bool PeerReplyCreate(
        const Identifier& nym,
        const Identifier& request,
        const proto::PeerReply& reply);

    /**   Rollback a PeerReplyCreate call
     *
     *    The original request is returned to IncomingPeerRequest box
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] request the identifier of the corresponding request
     *    \param[in] reply the identifier of the peer reply object
     *    \returns true if the rollback is successful
     */
    bool PeerReplyCreateRollback(
        const Identifier& nym,
        const Identifier& request,
        const Identifier& reply);

    /**   Obtain a list of incoming peer replies
     *
     *    \param[in] nym the identifier of the nym whose box is returned
     */
    ObjectList PeerReplyIncoming(const Identifier& nym) const;

    /**   Store the senders's copy of a peer reply
     *
     *    The peer reply is stored in the IncomingPeerReply box for the
     *    specified nym.
     *
     *    The corresponding request is moved from the nym's SentPeerRequest
     *    box to the FinishedPeerRequest box.
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] request the identifier of the corresponding request
     *    \param[in] reply the serialized peer reply object
     *    \returns true if the request is successfully stored
     */
    bool PeerReplyReceive(
        const Identifier& nym,
        const Identifier& request,
        const proto::PeerReply& reply);

    /**   Load a peer reply object
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] request the identifier of the peer reply object
     *    \param[in] box the box from which to retrive the peer object
     *    \returns A smart pointer to the object. The smart pointer will not be
     *             instantiated if the object does not exist or is invalid.
     */
    std::shared_ptr<proto::PeerRequest> PeerRequest(
        const Identifier& nym,
        const Identifier& request,
        const StorageBox& box) const;

    /**   Clean up the sender's copy of a peer reply
     *
     *    The peer reply is moved from the nym's IncomingPeerReply
     *    box to the ProcessedPeerReply box.
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] reply the identifier of the peer reply object
     *    \returns true if the request is successfully moved
     */
    bool PeerRequestComplete(
        const Identifier& nym,
        const Identifier& reply);

    /**   Store the initiator's copy of a peer request
     *
     *    The peer request is stored in the SentPeerRequest box for the
     *    specified nym.
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] request the serialized peer request object
     *    \returns true if the request is successfully stored
     */
    bool PeerRequestCreate(
        const Identifier& nym,
        const proto::PeerRequest& request);

    /**   Rollback a PeerRequestCreate call
     *
     *    The request is deleted from to SentPeerRequest box
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] request the identifier of the peer request
     *    \returns true if the rollback is successful
     */
    bool PeerRequestCreateRollback(
        const Identifier& nym,
        const Identifier& request);

    /**   Obtain a list of incoming peer requests
     *
     *    \param[in] nym the identifier of the nym whose box is returned
     */
    ObjectList PeerRequestIncoming(const Identifier& nym) const;

    /**   Store the recipient's copy of a peer request
     *
     *    The peer request is stored in the IncomingPeerRequest box for the
     *    specified nym.
     *
     *    \param[in] nym the identifier of the nym who owns the object
     *    \param[in] request the serialized peer request object
     *    \returns true if the request is successfully stored
     */
    bool PeerRequestReceive(
        const Identifier& nym,
        const proto::PeerRequest& request);

    /**   Unload and delete a server contract
     *
     *    This method destroys the contract object, removes it from the
     *    in-memory map, and deletes it from local storage.
     *    \param[in]  id the indentifier of the contract to be removed
     *    \returns true if successful, false if the contract did not exist
     *
     */
    bool RemoveServer(const Identifier& id);

    /**   Unload and delete a unit definition contract
     *
     *    This method destroys the contract object, removes it from the
     *    in-memory map, and deletes it from local storage.
     *    \param[in]  id the indentifier of the contract to be removed
     *    \returns true if successful, false if the contract did not exist
     *
     */
    bool RemoveUnitDefinition(const Identifier& id);

    /**   Obtain a smart pointer to an instantiated server contract.
     *
     *    The smart pointer will not be initialized if the object does not
     *    exist or is invalid.
     *
     *    If the caller is willing to accept a network lookup delay, it can
     *    specify a timeout to be used in the event that the contract can not
     *    be located in local storage and must be queried from a remote
     *    location.
     *
     *    If no timeout is specified, the remote query will still happen in the
     *    background, but this method will return immediately with a null
     *    result.
     *
     *    \param[in] id the identifier of the contract to be returned
     *    \param[in] timeout The caller can set a non-zero value here if it's
     *                     willing to wait for a network lookup. The default value
     *                     of 0 will return immediately.
     */
    ConstServerContract Server(
        const Identifier& id,
        const std::chrono::milliseconds& timeout = std::chrono::milliseconds(0));

    /**   Instantiate a server contract from serialized form
     *
     *    The smart pointer will not be initialized if the provided serialized
     *    contract is invalid.
     *
     *    \param[in] contract the serialized version of the contract
     */
    ConstServerContract Server(const proto::ServerContract& contract);

    /**   Create a new server contract
     *
     *    The smart pointer will not be initialized if the provided parameters
     *    can not form a valid contract
     *
     *    \param[in] nymid the identifier of nym which will create the contract
     *    \param[in] name the official name of the server
     *    \param[in] terms human-readable server description & terms of use
     *    \param[in] url externally-reachable IP address or hostname
     *    \param[in] port externally-reachable listen port
     */
    ConstServerContract Server(
        const std::string& nymid,
        const std::string& name,
        const std::string& terms,
        const std::list<ServerContract::Endpoint>& endpoints);

    /**   Returns a list of all available server contracts and their aliases
     */
    ObjectList ServerList();

    /**   Updates the alias for the specified nym.
     *
     *    An alias is a local label which is not part of the nym credentials
     *    itself.
     *
     *    \param[in] id the identifier of the nym whose alias is to be set
     *    \param[in] alias the alias to set or update for the specified nym
     *    \returns true if successful, false if the nym can not be located
     */
    bool SetNymAlias(const Identifier& id, const std::string alias);

    /**   Updates the alias for the specified server contract.
     *
     *    An alias is a local label which is not part of the server contract
     *    itself.
     *
     *    \param[in] id the identifier of the contract whose alias is to be set
     *    \param[in] alias the alias to set or update for the specified contract
     *    \returns true if successful, false if the contract can not be located
     */
    bool SetServerAlias(const Identifier& id, const std::string alias);

    /**   Updates the alias for the specified unit definition contract.
     *
     *    An alias is a local label which is not part of the unit definition
     *    contract itself.
     *
     *    \param[in] id the identifier of the contract whose alias is to be set
     *    \param[in] alias the alias to set or update for the specified contract
     *    \returns true if successful, false if the contract can not be located
     */
    bool SetUnitDefinitionAlias(const Identifier& id, const std::string alias);

    /**   Obtain a list of all available unit definition contracts and their
     *    aliases
     */
    ObjectList UnitDefinitionList();

    /**   Obtain a smart pointer to an instantiated unit definition contract.
     *
     *    The smart pointer will not be initialized if the object does not
     *    exist or is invalid.
     *
     *    If the caller is willing to accept a network lookup delay, it can
     *    specify a timeout to be used in the event that the contract can not
     *    be located in local storage and must be queried from a remote
     *    location.
     *
     *    If no timeout is specified, the remote query will still happen in the
     *    background, but this method will return immediately with a null
     *    result.
     *
     *    \param[in] id the identifier of the contract to be returned
     *    \param[in] timeout The caller can set a non-zero value here if it's
     *                     willing to wait for a network lookup. The default
     *                     value of 0 will return immediately.
     */
    ConstUnitDefinition UnitDefinition(
        const Identifier& id,
        const std::chrono::milliseconds& timeout = std::chrono::milliseconds(0));

    /**   Instantiate a unit definition contract from serialized form
     *
     *    The smart pointer will not be initialized if the provided serialized
     *    contract is invalid.
     *
     *    \param[in] contract the serialized version of the contract
     */
    ConstUnitDefinition UnitDefinition(const proto::UnitDefinition& contract);

    /**   Create a new currency contract
     *
     *    The smart pointer will not be initialized if the provided parameters
     *    can not form a valid contract
     *
     *    \param[in] nymid the identifier of nym which will create the contract
     *    \param[in] shortname a short human-readable identifier for the contract
     *    \param[in] name the official name of the unit of account
     *    \param[in] symbol symbol for the unit of account
     *    \param[in] terms human-readable terms and conditions
     *    \param[in] tla three-letter acronym abbreviation of the unit of account
     *    \param[in] power the number of decimal places to shift to display
     *                     fractional units
     *    \param[in] fraction the name of the fractional unit
     */
    ConstUnitDefinition UnitDefinition(
        const std::string& nymid,
        const std::string& shortname,
        const std::string& name,
        const std::string& symbol,
        const std::string& terms,
        const std::string& tla,
        const uint32_t& power,
        const std::string& fraction);

    /**   Create a new security contract
     *
     *    The smart pointer will not be initialized if the provided parameters
     *    can not form a valid contract
     *
     *    \param[in] nymid the identifier of nym which will create the contract
     *    \param[in] shortname a short human-readable identifier for the contract
     *    \param[in] name the official name of the unit of account
     *    \param[in] symbol symbol for the unit of account
     *    \param[in] terms human-readable terms and conditions
     */
    ConstUnitDefinition UnitDefinition(
        const std::string& nymid,
        const std::string& shortname,
        const std::string& name,
        const std::string& symbol,
        const std::string& terms);

    ~Wallet() = default;
};
}  // namespace opentxs
#endif // OPENTXS_CORE_APP_WALLET_HPP

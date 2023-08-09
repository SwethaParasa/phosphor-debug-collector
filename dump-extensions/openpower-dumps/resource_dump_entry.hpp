#pragma once

#include "com/ibm/Dump/Entry/Resource/server.hpp"
#include "dump_entry.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

#include <chrono>

namespace openpower
{
namespace dump
{
namespace resource
{
template <typename T>
using ServerObject = typename sdbusplus::server::object_t<T>;

using EntryIfaces = sdbusplus::server::object_t<
    sdbusplus::com::ibm::Dump::Entry::server::Resource>;

using originatorTypes = sdbusplus::xyz::openbmc_project::Common::server::
    OriginatedBy::OriginatorTypes;

class Manager;

/** @class Entry
 *  @brief Resource Dump Entry implementation.
 *  @details An extension to Dump::Entry class and
 *  A concrete implementation for the
 *  com::ibm::Dump::Entry::Resource DBus API
 */
class Entry : virtual public phosphor::dump::Entry, virtual public EntryIfaces
{
  public:
    Entry() = delete;
    Entry(const Entry&) = delete;
    Entry& operator=(const Entry&) = delete;
    Entry(Entry&&) = delete;
    Entry& operator=(Entry&&) = delete;
    ~Entry() = default;

    /** @brief Constructor for the resource dump Entry Object
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Object path to attach to
     *  @param[in] dumpId - Dump id.
     *  @param[in] timeStamp - Dump creation timestamp
     *             since the epoch.
     *  @param[in] dumpSize - Dump size in bytes.
     *  @param[in] sourceId - DumpId provided by the source.
     *  @param[in] vspStr- Input to host to generate the resource dump.
     *  @param[in] pwd - Password needed by host to validate the request.
     *  @param[in] status - status  of the dump.
     *  @param[in] originatorId - Id of the originator of the dump
     *  @param[in] originatorType - Originator type
     *  @param[in] baseEntryPath - Base entry path
     *  @param[in] parent - The dump entry's parent.
     *  @param[in] emitSignal - Default true, this to emit the signal for dump
     *             object
     */
    Entry(sdbusplus::bus_t& bus, const std::string& objPath, uint32_t dumpId,
          uint64_t timeStamp, uint64_t dumpSize, const uint32_t sourceId,
          std::string vspStr, std::string pwd,
          phosphor::dump::OperationStatus status, std::string originatorId,
          originatorTypes originatorType, const std::string& baseEntryPath,
          phosphor::dump::Manager& parent, bool emitSignal = true) :
        phosphor::dump::Entry(bus, objPath.c_str(), dumpId, timeStamp, dumpSize,
                              status, originatorId, originatorType, parent),
        EntryIfaces(bus, objPath.c_str(), EntryIfaces::action::defer_emit),
        baseEntryPath(baseEntryPath)
    {
        sourceDumpId(sourceId);
        vspString(vspStr);
        password(pwd);
        // Emit deferred signal.
        if (emitSignal)
            this->openpower::dump::resource::EntryIfaces::emit_object_added();
    };

    /** @brief Method to initiate the offload of dump
     *  @param[in] uri - URI to offload dump.
     */
    void initiateOffload(std::string uri) override;

    /** @brief Method to update an existing dump entry
     *  @param[in] timeStamp - Dump creation timestamp
     *  @param[in] dumpSize - Dump size in bytes.
     *  @param[in] sourceId - The id of dump in the origin.
     */
    void update(uint64_t timeStamp, uint64_t dumpSize, uint32_t sourceId);

    /**
     * @brief Delete resource dump in host memory and the entry dbus object
     */
    void delete_() override;

    /** @brief Function to get the dump ID
     *
     *  @return Dump ID
     */
    uint32_t getID() const
    {
        return id;
    }

    /** @brief Function to set the dump ID
     *
     *  @return DumpId
     */
    void setID(uint32_t dumpId)
    {
        id = dumpId;
    }

  private:
    /** @brief Based entry path of dumps*/
    std::string baseEntryPath;
};

} // namespace resource
} // namespace dump
} // namespace openpower

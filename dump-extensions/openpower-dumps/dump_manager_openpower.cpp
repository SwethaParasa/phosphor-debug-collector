#include "config.h"

#include "dump_manager_openpower.hpp"

#include "dump_entry_factory.hpp"
#include "dump_utils.hpp"
#include "op_dump_consts.hpp"
#include "op_dump_util.hpp"
#include "system_dump_entry.hpp"

#include <com/ibm/Dump/Create/common.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <regex>

namespace openpower::dump
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

void Manager::notifyDump(uint32_t sourceDumpId, uint64_t size,
                         NotifyDumpTypes type, [[maybe_unused]] uint32_t token)
{
    DumpEntryFactory dumpFact(bus, baseEntryPath, *this);
    try
    {
        auto optEntry = dumpFact.notifyDump(convertNotifyToCreateType(type),
                                            sourceDumpId, size, lastEntryId + 1,
                                            entries);
        if (optEntry)
        {
            auto& entry = *optEntry;
            entries.insert(
                std::make_pair(entry->getDumpId(), std::move(entry)));
            lastEntryId++;
        }
    }
    catch (const std::invalid_argument& e)
    {
        lg2::error("Invalid DumpID : {ID}, Dump Type: {TYPE}", "ID",
                   sourceDumpId, "TYPE", type);
        lastEntryId++;
    }
}

sdbusplus::message::object_path
    Manager::createDump(phosphor::dump::DumpCreateParams params)
{
    try
    {
        DumpEntryFactory dumpFact(bus, baseEntryPath, *this);

        auto dumpEntry = dumpFact.createEntry(lastEntryId + 1, params);
        if (!dumpEntry)
        {
            lg2::error("Dump entry creation failed");
            return {};
        }

        uint32_t id = dumpEntry->getDumpId();
        entries.insert(std::make_pair(id, std::move(dumpEntry)));
        std::string idStr = std::format("{:08X}", id);
        lastEntryId++;
        return baseEntryPath + "/" + idStr;
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to create dump: {ERROR}", "ERROR", e);
        throw;
    }
    return {};
}

void Manager::updateEntry(const std::filesystem::path& fullPath)
{
    lg2::info("A new dump file found {PATH}", "PATH", fullPath.string());
    std::string filename = fullPath.filename().string();

    // Parse Filename SYSDUMP.<SerialNumber>.<DumpId>.<DateTime>Date
    std::regex pattern("(SYSDUMP).([a-zA-Z0-9]+).([0-9a-fA-F]{8}).([0-9]+)");
    std::smatch match;

    if (!std::regex_match(filename, match, pattern))
    {
        lg2::error("Filename does not match expected format, {FILENAME}",
                   "FILENAME", filename);
        return;
    }

    std::string dumpIdStr = match[3];
    std::string timestampStr = match[4];

    uint32_t dumpId = std::stoi(dumpIdStr, 0, 16);

    uint64_t timestamp = phosphor::dump::timeToEpoch(timestampStr);

    uint64_t fileSize = std::filesystem::file_size(fullPath);

    auto it = entries.find(dumpId);
    if (it == entries.end())
    {
        lg2::error("Entry with Dump ID {DUMP_ID} not found", "DUMP_ID",
                   std::format("{:08X}", dumpId));
        return;
    }
    auto opEntry = dynamic_cast<openpower::dump::Entry*>(it->second.get());

    opEntry->update(timestamp, fileSize, fullPath);
}

void Manager::restore()
{
    std::filesystem::path dir(dumpDir);
    if (!std::filesystem::exists(dir) || std::filesystem::is_empty(dir))
    {
        return;
    }

    // Initialize DumpEntryFactory
    DumpEntryFactory dumpFact(bus, baseEntryPath, *this);

    // Dump file path: <DUMP_PATH>/<id>/<filename>
    for (const auto& p : std::filesystem::directory_iterator(dir))
    {
        auto idStr = p.path().filename().string();

        // Consider only directories with dump id as name.
        // Note: As per design one file per directory.
        if ((std::filesystem::is_directory(p.path())) &&
            std::all_of(idStr.begin(), idStr.end(), ::isxdigit))
        {
            // Convert hex string to number
            uint32_t id = static_cast<uint32_t>(std::stoul(idStr, nullptr, 16));

            // Remove upper 8 bytes to get the actual entry ID
            uint32_t entryId = id & 0x00FFFFFF;

            lastEntryId = std::max(lastEntryId, entryId);
            auto objPath = std::filesystem::path(baseEntryPath) / idStr;

            // Create a dump entry
            std::unique_ptr<phosphor::dump::Entry> entry;
            try
            {
                entry = dumpFact.createEntryWithDefaults(id, objPath);
            }
            catch (const std::invalid_argument& e)
            {
                lg2::error(
                    "Invalid Dump Path, Dump Storage Path : {PATH} , Dump ID : {ID}",
                    "PATH", objPath, "ID", id);
                continue;
            }
            // Locate the serialized file
            std::filesystem::path serializedFilePath = p.path() / ".preserve" /
                                                       "serialized_entry.bin";
            if (std::filesystem::exists(serializedFilePath))
            {
                // Call deserialize to update the entry from the serialized
                // file
                entry->deserialize(serializedFilePath);
            }

            // Insert the entry into the entries map
            entries.insert(std::make_pair(id, std::move(entry)));

            // Check for dump file and call update if it exists
            for (const auto& fileIt :
                 std::filesystem::directory_iterator(p.path()))
            {
                if (fileIt.path().filename() != ".preserve")
                {
                    updateEntry(fileIt.path());
                }
            }
        }
    }
}

} // namespace openpower::dump

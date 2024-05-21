#include "bmc_dump_entry.hpp"

#include "dump_manager.hpp"
#include "dump_offload.hpp"

#include <phosphor-logging/lg2.hpp>

namespace phosphor
{
namespace dump
{
namespace bmc
{

void Entry::delete_()
{
    if (isOffloadInProgress())
    {
        log<level::ERR>(
            fmt::format("Dump offload is in progress, cannot delete id({})", id)
                .c_str());
        elog<NotAllowed>(
            Reason("Dump offload is in progress, please try later"));
    }
    
    // Delete Dump file from Permanent location
    try
    {
        std::filesystem::remove_all(file.parent_path());
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        // Log Error message and continue
        lg2::error("Failed to delete dump file, errormsg: {ERROR}", "ERROR", e);
    }

    // Remove Dump entry D-bus object
    phosphor::dump::Entry::delete_();
}

void Entry::initiateOffload(std::string uri)
{
    phosphor::dump::offload::requestOffload(file, id, uri);
    offloaded(true);
}

} // namespace bmc
} // namespace dump
} // namespace phosphor

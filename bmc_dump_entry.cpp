#include "bmc_dump_entry.hpp"

#include "dump_manager.hpp"
#include "dump_offload.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>

namespace phosphor
{
namespace dump
{
namespace bmc
{
using namespace phosphor::logging;
using NotAllowed = sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;

void Entry::delete_()
{
    if (isOffloadInProgress())
    {
        lg2::error(
            "Another offload is in progress URI {URI} id {ID} cannot continue",
            "URI", offloadUri(), "ID", id);
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

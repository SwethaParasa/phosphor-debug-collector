#include "bmc_dump_entry.hpp"
#include "dump_manager.hpp"
#include "dump_offload.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "dump_utils.hpp"

#include <fmt/core.h>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace dump
{
namespace bmc_stored
{
using namespace phosphor::logging;

void Entry::delete_()
{
    // Delete Dump file from Permanent location
    try
    {
        std::filesystem::remove_all(
            std::filesystem::path(path()).parent_path());
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        // Log Error message and continue
        log<level::ERR>(
            fmt::format("Failed to delete dump file({}), errormsg({})", path(),
                        e.what())
                .c_str());
    }

    // Remove Dump entry D-bus object
    phosphor::dump::Entry::delete_();
}

void Entry::initiateOffload(std::string uri)
{
    phosphor::dump::offload::requestOffload(path(), id, uri);
    offloaded(true);
}

} // namespace bmc_stored
} // namespace dump
} // namespace phosphor

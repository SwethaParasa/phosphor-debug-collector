#include "op_dump_util.hpp"

#include "dump_utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"

#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>

#include <filesystem>

#include <fmt/core.h>
#include <unistd.h>


namespace openpower
{
namespace dump
{
namespace util
{

constexpr auto MP_REBOOT_FILE = "/run/openbmc/mpreboot@0";

using namespace phosphor::logging;
using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

void isOPDumpsEnabled()
{
    auto enabled = true;
    constexpr auto enable = "xyz.openbmc_project.Object.Enable";
    constexpr auto policy = "/xyz/openbmc_project/dump/system_dump_policy";
    constexpr auto property = "org.freedesktop.DBus.Properties";

    using disabled =
        sdbusplus::xyz::openbmc_project::Dump::Create::Error::Disabled;

    try
    {
        auto bus = sdbusplus::bus::new_default();

        auto service = phosphor::dump::getService(bus, policy, enable);

        auto method =
            bus.new_method_call(service.c_str(), policy, property, "Get");
        method.append(enable, "Enabled");
        auto reply = bus.call(method);
        std::variant<bool> v;
        reply.read(v);
        enabled = std::get<bool>(v);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>(
            fmt::format("Error({}) in getting dump policy, default is enabled",
                        e.what())
                .c_str());
        report<InternalFailure>();
    }

    if (!enabled)
    {
        log<level::ERR>("OpePOWER dumps are disabled, skipping");
        elog<disabled>();
    }
    log<level::INFO>("OpenPOWER dumps are enabled");
}

BIOSAttrValueType readBIOSAttribute(const std::string& attrName,
                                    sdbusplus::bus_t& bus)
{
    std::tuple<std::string, BIOSAttrValueType, BIOSAttrValueType> attrVal;
    auto method = bus.new_method_call(
        "xyz.openbmc_project.BIOSConfigManager",
        "/xyz/openbmc_project/bios_config/manager",
        "xyz.openbmc_project.BIOSConfig.Manager", "GetAttribute");
    method.append(attrName);
    try
    {
        auto result = bus.call(method);
        result.read(std::get<0>(attrVal), std::get<1>(attrVal),
                    std::get<2>(attrVal));
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        lg2::error("Failed to read BIOS Attribute: {ATTRIBUTE_NAME}",
                   "ATTRIBUTE_NAME", attrName);
        throw;
    }
    return std::get<1>(attrVal);
}

bool isSystemDumpInProgress(sdbusplus::bus_t& bus)
{
    try
    {
        auto dumpInProgress = std::get<std::string>(
            readBIOSAttribute("pvm_sys_dump_active", bus));
        if (dumpInProgress == "Enabled")
        {
            lg2::info("A system dump is already in progress");
            return true;
        }
    }
    catch (const std::bad_variant_access& ex)
    {
        lg2::error("Failed to read pvm_sys_dump_active property value due "
                   "to bad variant access error:{ERROR}",
                   "ERROR", ex);
        return false;
    }
    catch (const std::exception& ex)
    {
        lg2::error("Failed to read pvm_sys_dump_active error:{ERROR}", "ERROR",
                   ex);
        return false;
    }

    lg2::info("Another system dump is not in progress");
    return false;
}

bool isInMpReboot()
{
    return std::filesystem::exists(MP_REBOOT_FILE);
}

bool isSystemDumpInProgress()
{
    using BiosBaseTableItem = std::pair<
        std::string,
        std::tuple<std::string, bool, std::string, std::string, std::string,
                   std::variant<int64_t, std::string>,
                   std::variant<int64_t, std::string>,
                   std::vector<std::tuple<
                       std::string, std::variant<int64_t, std::string>>>>>;
    using BiosBaseTable = std::vector<BiosBaseTableItem>;

    try
    {
        std::string dumpInProgress{};
        auto bus = sdbusplus::bus::new_default();

        auto retVal =
            phosphor::dump::readDBusProperty<std::variant<BiosBaseTable>>(
                bus, "xyz.openbmc_project.BIOSConfigManager",
                "/xyz/openbmc_project/bios_config/manager",
                "xyz.openbmc_project.BIOSConfig.Manager", "BaseBIOSTable");
        const auto baseBiosTable = std::get_if<BiosBaseTable>(&retVal);
        if (baseBiosTable == nullptr)
        {
            log<level::ERR>(
                "Util failed to read BIOSconfig property BaseBIOSTable");
            return false;
        }
        for (const auto& item : *baseBiosTable)
        {
            const auto attributeName = std::get<0>(item);
            auto attrValue = std::get<5>(std::get<1>(item));
            auto val = std::get_if<std::string>(&attrValue);
            if (val != nullptr && attributeName == "pvm_sys_dump_active")
            {
                dumpInProgress = *val;
                break;
            }
        }
        if (dumpInProgress.empty())
        {
            log<level::ERR>(
                "Util failed to read pvm_hmc_managed property value");
            return false;
        }
        if (dumpInProgress == "Enabled")
        {
            log<level::INFO>("A system dump is already in progress");
            return true;
        }
    }
    catch (const std::exception& ex)
    {
        log<level::ERR>(
            fmt::format("Failed to read pvm_sys_dump_active ({})", ex.what())
                .c_str());
        return false;
    }

    log<level::INFO>("Another system dump is not in progress");
    return false;
}

} // namespace util
} // namespace dump
} // namespace openpower

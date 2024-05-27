#include "dump_utils.hpp"

#include "dump_types.hpp"

#include <phosphor-logging/lg2.hpp>

namespace phosphor
{
namespace dump
{

std::string getService(sdbusplus::bus_t& bus, const std::string& path,
                       const std::string& interface)
{
    constexpr auto objectMapperName = "xyz.openbmc_project.ObjectMapper";
    constexpr auto objectMapperPath = "/xyz/openbmc_project/object_mapper";

    auto method = bus.new_method_call(objectMapperName, objectMapperPath,
                                      objectMapperName, "GetObject");

    method.append(path);
    method.append(std::vector<std::string>({interface}));

    std::vector<std::pair<std::string, std::vector<std::string>>> response;

    try
    {
        auto reply = bus.call(method);
        reply.read(response);
        if (response.empty())
        {
            lg2::error(
                "Error in mapper response for getting service name, PATH: "
                "{PATH}, INTERFACE: {INTERFACE}",
                "PATH", path, "INTERFACE", interface);
            return std::string{};
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Error in mapper method call, errormsg: {ERROR}, "
                   "PATH: {PATH}, INTERFACE: {INTERFACE}",
                   "ERROR", e, "PATH", path, "INTERFACE", interface);
        throw;
    }
    return response[0].first;
}

void createPEL(const std::string& dumpFilePath, const std::string& dumpFileType,
               const int dumpId, const std::string& pelSev,
               const std::string& errIntf)
{
    try
    {
        constexpr auto loggerObjectPath = "/xyz/openbmc_project/logging";
        constexpr auto loggerCreateInterface =
            "xyz.openbmc_project.Logging.Create";
        constexpr auto loggerService = "xyz.openbmc_project.Logging";
        constexpr auto dumpFileString = "File Name";
        constexpr auto dumpFileTypeString = "Dump Type";
        constexpr auto dumpIdString = "Dump ID";

        sd_bus* pBus = nullptr;
        sd_bus_default(&pBus);

        // Implies this is a call from Manager. Hence we need to make an async
        // call to avoid deadlock with Phosphor-logging.
        auto retVal = sd_bus_call_method_async(
            pBus, nullptr, loggerService, loggerObjectPath,
            loggerCreateInterface, "Create", nullptr, nullptr, "ssa{ss}",
            errIntf.c_str(), pelSev.c_str(), 3, dumpIdString,
            std::to_string(dumpId).c_str(), dumpFileString,
            dumpFilePath.c_str(), dumpFileTypeString, dumpFileType.c_str());

        if (retVal < 0)
        {
            log<level::ERR>("Error calling sd_bus_call_method_async",
                            entry("retVal=%d", retVal),
                            entry("MSG=%s", strerror(-retVal)));
        }
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(
            "Error in calling creating PEL. Standard exception caught",
            entry("ERROR=%s", e.what()));
    }
}

} // namespace dump
} // namespace phosphor

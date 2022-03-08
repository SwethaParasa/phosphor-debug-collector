#pragma once

#include "dump_utils.hpp"

namespace openpower
{
namespace dump
{
namespace util
{

/** @brief Check whether OpenPOWER dumps are enabled
 *
 * A xyz.openbmc_project.Dump.Create.Error.Disabled will be thrown
 * if the dumps are disabled.
 * If the settings service is not running then considering as
 * the dumps are enabled.
 */
void isOPDumpsEnabled();

/** @brief Check whether memory preserving reboot is in progress
 *  @return true - memory preserving reboot in progress
 *          false - no memory preserving reboot is going on
 */
bool isInMpReboot();

using BIOSAttrValueType = std::variant<int64_t, std::string>;

/** @brief Read a BIOS attribute value
 *
 *  @param[in] attrName - Name of the BIOS attribute
 *  @param[in] bus - D-Bus handle
 *
 *  @return The value of the BIOS attribute as a variant of possible types
 *
 *  @throws sdbusplus::exception::SdBusError if failed to read the attribute
 */
BIOSAttrValueType readBIOSAttribute(const std::string& attrName,
                                    sdbusplus::bus_t& bus);

/** @brief Check whether a system is in progress or available to offload.
 *
 *  @param[in] bus - D-Bus handle
 *
 *  @return true - A dump is in progress or available to offload
 *          false - No dump in progress
 */
bool isSystemDumpInProgress(sdbusplus::bus_t& bus);
} // namespace util
} // namespace dump
} // namespace openpower

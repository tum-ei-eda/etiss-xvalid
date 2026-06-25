// SPDX-License-Identifier: Apache-2.0
//
// This file is part of etiss-xvalid. It is licensed under the Apache License, Version 2.0; you may not use this file
// except in compliance with the License. You should have received a copy of the license along with this project. If not,
// see the LICENSE file.
#define ETISS_LIBNAME XValid

#include "etiss/xvalid/DataReadTracer.h"
#include "etiss/xvalid/DataWriteTracer.h"
#include "etiss/xvalid/ISAExtensionValidator.h"
#include "etiss/xvalid/InstructionTracer.h"

#include "etiss/ETISS.h"
#include "etiss/LibraryInterface.h"
#include "etiss/helper/PluginLibrary.h"

namespace
{
uint64_t readUintOption(const std::map<std::string, std::string> &options, const std::string &key, uint64_t fallback)
{
    auto iter = options.find(key);
    if (iter == options.end())
    {
        return fallback;
    }

    return std::stoull(iter->second, nullptr, 0);
}

std::string readStringOption(const std::map<std::string, std::string> &options, const std::string &key,
                             const std::string &fallback)
{
    auto iter = options.find(key);
    if (iter == options.end())
    {
        return fallback;
    }

    return iter->second;
}

std::string readStringConfig(const std::map<std::string, std::string> &options, const std::string &key,
                             const std::string &fallback)
{
    return etiss::cfg().get<std::string>(key, readStringOption(options, key, fallback));
}
}

extern "C"
{
    ETISS_LIBRARYIF_VERSION_FUNC_IMPL

    ETISS_PLUGIN_EXPORT unsigned XValid_countPlugin()
    {
        return 4;
    }

    ETISS_PLUGIN_EXPORT const char *XValid_namePlugin(unsigned index)
    {
        switch (index)
        {
        case 0:
            return "ISAExtensionValidator";
        case 1:
            return "GTS";
        case 2:
            return "DataWriteTracer";
        case 3:
            return "DataReadTracer";
        default:
            return nullptr;
        }
    }

    ETISS_PLUGIN_EXPORT etiss::Plugin *XValid_createPlugin(unsigned index, std::map<std::string, std::string> options)
    {
        switch (index)
        {
        case 0:
            return new etiss::plugin::ISAExtensionValidator();
        case 1:
        {
            const std::string pcRange =
                readStringConfig(options, "plugin.instruction_tracer.pc_range",
                                 readStringConfig(options, "plugin.gts.pc_range", ""));
            const std::string pcRangePath =
                readStringConfig(options, "plugin.instruction_tracer.pc_range_path",
                                 readStringConfig(options, "plugin.gts.pc_range_path", "pcs.tmp"));
            return new InstructionTracer(pcRange, pcRangePath);
        }
        case 2:
        {
            const uint64_t addr =
                readUintOption(options, "plugin.data_write_tracer.logaddr",
                               readUintOption(options, "plugin.data_write_tracer.addr", 0));
            const uint64_t mask =
                readUintOption(options, "plugin.data_write_tracer.logmask",
                               readUintOption(options, "plugin.data_write_tracer.mask", 0));
            return new etiss::plugin::DataWriteTracer(addr, mask);
        }
        case 3:
        {
            const uint64_t addr =
                readUintOption(options, "plugin.data_read_tracer.logaddr",
                               readUintOption(options, "plugin.data_read_tracer.addr", 0));
            const uint64_t mask =
                readUintOption(options, "plugin.data_read_tracer.logmask",
                               readUintOption(options, "plugin.data_read_tracer.mask", 0));
            return new etiss::plugin::DataReadTracer(addr, mask);
        }
        default:
            return nullptr;
        }
    }

    ETISS_PLUGIN_EXPORT void XValid_deletePlugin(etiss::Plugin *plugin)
    {
        delete plugin;
    }
}

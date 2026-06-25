// SPDX-License-Identifier: Apache-2.0
//
// This file is part of etiss-xvalid. It is licensed under the Apache License, Version 2.0; you may not use this file
// except in compliance with the License. You should have received a copy of the license along with this project. If not,
// see the LICENSE file.
#define ETISS_LIBNAME XValid

#include "etiss/xvalid/DataReadTracer.h"
#include "etiss/xvalid/DataWriteTracer.h"
#include "etiss/xvalid/ISAExtensionValidator.h"

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

uint64_t readUintConfig(const std::map<std::string, std::string> &options, const std::string &key, uint64_t fallback)
{
    return etiss::cfg().get<uint64_t>(key, readUintOption(options, key, fallback));
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
        return 3;
    }

    ETISS_PLUGIN_EXPORT const char *XValid_namePlugin(unsigned index)
    {
        switch (index)
        {
        case 0:
            return "ISAExtensionValidator";
        case 1:
            return "DataWriteTracer";
        case 2:
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
        {
            const std::string instructions = readStringConfig(options, "plugin.xvalid.itrace_instructions", "*");
            const std::string pcRange = readStringConfig(options, "plugin.xvalid.itrace_pc_ranges", "");
            const std::string pcRangePath =
                readStringConfig(options, "plugin.xvalid.itrace_pc_ranges_fpath", "pcs.tmp");
            return new etiss::plugin::ISAExtensionValidator(instructions, pcRange, pcRangePath);
        }
        case 1:
        {
            const uint64_t addr = readUintConfig(options, "plugin.xvalid.dwrite_trace.logaddr", 0);
            const uint64_t mask = readUintConfig(options, "plugin.xvalid.dwrite_trace.logmask", 0);
            return new etiss::plugin::DataWriteTracer(addr, mask);
        }
        case 2:
        {
            const uint64_t addr = readUintConfig(options, "plugin.xvalid.dread_trace.logaddr", 0);
            const uint64_t mask = readUintConfig(options, "plugin.xvalid.dread_trace.logmask", 0);
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

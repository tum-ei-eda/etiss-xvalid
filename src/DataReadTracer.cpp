// SPDX-License-Identifier: Apache-2.0
//
// This file is part of etiss-xvalid. It is licensed under the Apache License, Version 2.0; you may not use this file
// except in compliance with the License. You should have received a copy of the license along with this project. If not,
// see the LICENSE file.
#include "etiss/xvalid/DataReadTracer.h"

#include "etiss/ETISS.h"
#include "etiss/xvalid/TraceFileWriter.h"

#include <cstring>

namespace etiss
{
namespace plugin
{

static etiss_int32 dataReadTracerLog(void *handle, ETISS_CPU *cpu, etiss_uint64 addr, etiss_uint8 *buf,
                                     etiss_uint32 len)
{
    auto *customHandle = static_cast<DataReadTracer::CustomHandle *>(handle);
    const etiss_int32 result = customHandle->origSys->dread(customHandle->origSys->handle, cpu, addr, buf, len);

    if (result == 0 && (addr & customHandle->mask) == customHandle->addr && TraceFileWriter::instance().isTracing())
    {
        DReadEntry entry{};
        entry.type = 3;
        entry.pc = cpu->instructionPointer;
        entry.addr = addr;
        entry.length = len > sizeof(entry.data) ? sizeof(entry.data) : len;
        std::memcpy(entry.data, buf, entry.length);

        TraceFileWriter::instance().writeDRead(entry);
    }

    return result;
}

DataReadTracer::DataReadTracer(uint64_t addrValue, uint64_t addrMask)
{
    customHandle_.addr = addrValue & addrMask;
    customHandle_.mask = addrMask;
    if (customHandle_.addr == 0 && customHandle_.mask == 0)
    {
        etiss::log(etiss::WARNING,
                   "DataReadTracer instantiated with mask and address set to 0. It will trace all reads.");
    }
}

ETISS_System DataReadTracer::getWrapInfo(ETISS_System *origSystem)
{
    customHandle_.origSys = origSystem;

    ETISS_System wrapInfo = {};
    wrapInfo.handle = &customHandle_;
    wrapInfo.dread = &dataReadTracerLog;
    return wrapInfo;
}

} // namespace plugin
} // namespace etiss

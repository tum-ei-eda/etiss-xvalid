#include "etiss/xvalid/DataWriteTracer.h"

#include "etiss/ETISS.h"
#include "etiss/xvalid/TraceFileWriter.h"

#include <cstring>

namespace etiss
{
namespace plugin
{

static etiss_int32 dataWriteTracerLog(void *handle, ETISS_CPU *cpu, etiss_uint64 addr, etiss_uint8 *buf,
                                      etiss_uint32 len)
{
    auto *customHandle = static_cast<DataWriteTracer::CustomHandle *>(handle);

    if ((addr & customHandle->mask) == customHandle->addr && TraceFileWriter::instance().isTracing())
    {
        DWriteEntry entry{};
        entry.type = 2;
        entry.pc = cpu->instructionPointer;
        entry.addr = addr;
        entry.length = len > sizeof(entry.data) ? sizeof(entry.data) : len;
        std::memcpy(entry.data, buf, entry.length);

        TraceFileWriter::instance().writeDWrite(entry);
    }

    return customHandle->origSys->dwrite(customHandle->origSys->handle, cpu, addr, buf, len);
}

DataWriteTracer::DataWriteTracer(uint64_t addrValue, uint64_t addrMask)
{
    customHandle_.addr = addrValue & addrMask;
    customHandle_.mask = addrMask;
    if (customHandle_.addr == 0 && customHandle_.mask == 0)
    {
        etiss::log(etiss::WARNING,
                   "DataWriteTracer instantiated with mask and address set to 0. It will trace all writes.");
    }
}

ETISS_System DataWriteTracer::getWrapInfo(ETISS_System *origSystem)
{
    customHandle_.origSys = origSystem;

    ETISS_System wrapInfo = {};
    wrapInfo.handle = &customHandle_;
    wrapInfo.dwrite = &dataWriteTracerLog;
    return wrapInfo;
}

} // namespace plugin
} // namespace etiss

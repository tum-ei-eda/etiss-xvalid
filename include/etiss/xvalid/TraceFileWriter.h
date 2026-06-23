// SPDX-License-Identifier: Apache-2.0
//
// This file is part of etiss-xvalid. It is licensed under the Apache License, Version 2.0; you may not use this file
// except in compliance with the License. You should have received a copy of the license along with this project. If not,
// see the LICENSE file.
//
// Original author: Heidi Holappa <73523507+heidi-holappa@users.noreply.github.com>
#ifndef ETISS_XVALID_TRACEFILEWRITER_H_
#define ETISS_XVALID_TRACEFILEWRITER_H_

#include <cstdint>
#include <fstream>
#include <mutex>
#include <string>

#pragma pack(push, 1)
struct StateSnapshotEntry
{
    uint32_t type;
    uint32_t pc;
    uint32_t sp;
    uint32_t x[32];
    uint64_t f[32];
    char instruction[16];
};
#pragma pack(pop)

static_assert(sizeof(StateSnapshotEntry) == 412, "StateSnapshotEntry size must be 412 bytes");

#pragma pack(push, 1)
struct DWriteEntry
{
    uint32_t type;
    uint32_t pc;
    uint64_t addr;
    uint32_t length;
    uint8_t data[64];
};
#pragma pack(pop)

static_assert(sizeof(DWriteEntry) == 84, "DWriteEntry size must be 84 bytes");

class TraceFileWriter
{
  public:
    static TraceFileWriter &instance(const std::string &outputPath = "trace.bin");

    void writeStateSnapshot(const StateSnapshotEntry &entry);
    void writeDWrite(const DWriteEntry &entry);

    void activateTrace();
    void deactivateTrace();
    bool isTracing() const;

  private:
    TraceFileWriter() = default;
    ~TraceFileWriter();

    TraceFileWriter(const TraceFileWriter &) = delete;
    TraceFileWriter &operator=(const TraceFileWriter &) = delete;

    void openFile(const std::string &outputPath);

    std::ofstream outfile_;
    std::mutex mutex_;
    bool trace_active_ = false;
};

#endif

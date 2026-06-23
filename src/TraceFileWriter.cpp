// SPDX-License-Identifier: Apache-2.0
//
// This file is part of etiss-xvalid. It is licensed under the Apache License, Version 2.0; you may not use this file
// except in compliance with the License. You should have received a copy of the license along with this project. If not,
// see the LICENSE file.
//
// Original author: Heidi Holappa <73523507+heidi-holappa@users.noreply.github.com>
#include "etiss/xvalid/TraceFileWriter.h"

#include "etiss/ETISS.h"

#include <cstdio>

TraceFileWriter &TraceFileWriter::instance(const std::string &outputPath)
{
    static TraceFileWriter inst;
    static bool initialized = false;

    if (!initialized)
    {
        inst.openFile(outputPath);
        initialized = true;
    }

    return inst;
}

void TraceFileWriter::openFile(const std::string &outputPath)
{
    if (std::ifstream(outputPath))
    {
        if (std::remove(outputPath.c_str()) == 0)
        {
            etiss::log(etiss::INFO, "TraceFileWriter: old output file '" + outputPath + "' deleted at startup.");
        }
        else
        {
            etiss::log(etiss::WARNING, "TraceFileWriter: failed to delete old output file '" + outputPath + "'.");
        }
    }

    outfile_.open(outputPath, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!outfile_)
    {
        etiss::log(etiss::ERROR, "TraceFileWriter: cannot open output file '" + outputPath + "'.");
    }
}

TraceFileWriter::~TraceFileWriter()
{
    if (outfile_.is_open())
    {
        outfile_.close();
    }
}

void TraceFileWriter::writeStateSnapshot(const StateSnapshotEntry &entry)
{
    if (!trace_active_)
        return;

    std::lock_guard<std::mutex> lock(mutex_);
    outfile_.write(reinterpret_cast<const char *>(&entry), sizeof(entry));
}

void TraceFileWriter::writeDWrite(const DWriteEntry &entry)
{
    if (!trace_active_)
        return;

    std::lock_guard<std::mutex> lock(mutex_);
    outfile_.write(reinterpret_cast<const char *>(&entry), sizeof(entry));
}

void TraceFileWriter::activateTrace()
{
    etiss::log(etiss::INFO, "TraceFileWriter: trace activated.");
    trace_active_ = true;
}

void TraceFileWriter::deactivateTrace()
{
    etiss::log(etiss::INFO, "TraceFileWriter: trace deactivated.");
    trace_active_ = false;
}

bool TraceFileWriter::isTracing() const
{
    return trace_active_;
}

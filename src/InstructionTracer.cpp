// SPDX-License-Identifier: Apache-2.0
//
// This file is part of etiss-xvalid. It is licensed under the Apache License, Version 2.0; you may not use this file
// except in compliance with the License. You should have received a copy of the license along with this project. If not,
// see the LICENSE file.
//
// Original author: Heidi Holappa <73523507+heidi-holappa@users.noreply.github.com>
#include "etiss/xvalid/InstructionTracer.h"

#include "etiss/CPUArch.h"
#include "etiss/ETISS.h"
#include "etiss/Instruction.h"
#include "etiss/xvalid/CpuArchConfig.h"
#include "etiss/xvalid/TraceFileWriter.h"

#include <cstring>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_set>

namespace
{
const std::unordered_set<std::string> instructionsToSnapshot = { "cjr", "cswsp" };

etiss_uint32 lowPc = 0;
etiss_uint32 highPc = 0;
}

InstructionTracer::InstructionTracer()
{
    std::ifstream pcsFile("pcs.tmp");
    if (!pcsFile.is_open())
    {
        etiss::log(etiss::ERROR, "Failed to read PC range from pcs.tmp. Tracing will not be activated.");
        return;
    }

    std::string line;
    if (std::getline(pcsFile, line))
    {
        std::stringstream ss(line);
        std::string token;

        if (std::getline(ss, token, ';'))
        {
            lowPc = static_cast<etiss_uint32>(std::stoul(token, nullptr, 10));
        }

        if (std::getline(ss, token, ';'))
        {
            highPc = static_cast<etiss_uint32>(std::stoul(token, nullptr, 10));
        }

        etiss::log(etiss::INFO, "Loaded PC range: low=" + std::to_string(lowPc) + ", high=" + std::to_string(highPc));
    }
    else
    {
        etiss::log(etiss::WARNING, "pcs.tmp is empty or unreadable.");
    }
}

bool InstructionTracer::callback()
{
    std::lock_guard<std::mutex> guard(mutex_);
    return false;
}

bool InstructionTracer::callbackOnInstruction(etiss::instr::Instruction &instr) const
{
    return instr.name_ == "cjr";
}

void InstructionTracer::finalizeInstrSet(etiss::instr::ModedInstructionSet &mis) const
{
    mis.foreach (
        [](etiss::instr::VariableInstructionSet &vis)
        {
            vis.foreach (
                [](etiss::instr::InstructionSet &set)
                {
                    set.foreach (
                        [](etiss::instr::Instruction &instr)
                        {
                            if (instructionsToSnapshot.find(instr.name_) == instructionsToSnapshot.end())
                            {
                                return;
                            }

                            instr.addCallback(
                                [&instr](etiss::instr::BitArray &, etiss::CodeSet &cs,
                                         etiss::instr::InstructionContext &)
                                {
                                    std::stringstream ss;
                                    ss << "// InstructionTracer: collect state information\n";
                                    ss << "InstructionTracer_collect_state((" << XVALID_STRINGIFY(XVALID_CPU_TYPE)
                                       << "*) cpu, \"" << instr.name_ << "\");\n";
                                    cs.append(etiss::CodePart::PREINITIALDEBUGRETURNING).code() = ss.str();
                                    return true;
                                },
                                0);
                        });
                });
        });
}

void InstructionTracer::initCodeBlock(etiss::CodeBlock &block) const
{
    block.fileglobalCode().insert("extern void InstructionTracer_collect_state(" XVALID_STRINGIFY(
        XVALID_CPU_TYPE) "*, const char*);");
}

extern "C"
{
    void InstructionTracer_collect_state(XVALID_CPU_TYPE *cpu, const char *instruction)
    {
        const etiss_uint32 pc = static_cast<etiss_uint32>(reinterpret_cast<ETISS_CPU *>(cpu)->instructionPointer);
        auto &writer = TraceFileWriter::instance();

        if (!writer.isTracing() && lowPc <= pc && pc <= highPc)
        {
            writer.activateTrace();
        }

        if (writer.isTracing())
        {
            StateSnapshotEntry entry{};
            entry.type = 1;
            entry.pc = pc;
            entry.sp = cpu->SP;
            std::strncpy(entry.instruction, instruction, sizeof(entry.instruction) - 1);

            for (int i = 0; i < 32; ++i)
                entry.x[i] = *cpu->X[i];

            for (int i = 0; i < 32; ++i)
                entry.f[i] = *cpu->F[i];

            writer.writeStateSnapshot(entry);
        }

        if (std::string(instruction) == "cjr" && lowPc <= pc && pc <= highPc)
        {
            writer.deactivateTrace();
        }
    }
}

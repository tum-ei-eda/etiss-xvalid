// SPDX-License-Identifier: Apache-2.0
//
// This file is part of etiss-xvalid. It is licensed under the Apache License, Version 2.0; you may not use this file
// except in compliance with the License. You should have received a copy of the license along with this project. If not,
// see the LICENSE file.
//
// Original author: Heidi Holappa <73523507+heidi-holappa@users.noreply.github.com>
#include "etiss/xvalid/InstructionTracer.h"

#include "etiss/CPUArch.h"
#include "etiss/CPUCore.h"
#include "etiss/ETISS.h"
#include "etiss/Instruction.h"
#include "etiss/VirtualStruct.h"
#include "etiss/xvalid/TraceFileWriter.h"

#include <cstring>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>

namespace
{
etiss_uint64 readFieldOrZero(const std::shared_ptr<etiss::VirtualStruct> &state, const std::string &name)
{
    auto field = state ? state->findName(name) : nullptr;
    if (!field)
    {
        return 0;
    }
    return field->read();
}

std::string trim(std::string value)
{
    const auto first = value.find_first_not_of(" \t\n\r");
    if (first == std::string::npos)
    {
        return "";
    }
    const auto last = value.find_last_not_of(" \t\n\r");
    return value.substr(first, last - first + 1);
}

std::uint32_t parsePc(std::string value)
{
    return static_cast<std::uint32_t>(std::stoul(trim(value), nullptr, 0));
}
}

InstructionTracer::InstructionTracer(std::string pc_range_path)
    : InstructionTracer("", std::move(pc_range_path))
{
}

InstructionTracer::InstructionTracer(std::string pc_range, std::string pc_range_path)
{
    if (!pc_range.empty())
    {
        std::stringstream ranges(pc_range);
        std::string range;
        while (std::getline(ranges, range, ','))
        {
            std::stringstream bounds(range);
            std::string low;
            std::string high;
            if (!std::getline(bounds, low, ':') || !std::getline(bounds, high, ':'))
            {
                etiss::log(etiss::WARNING, "Ignoring invalid PC range entry: " + range);
                continue;
            }

            const auto low_pc = parsePc(low);
            const auto high_pc = parsePc(high);
            if (low_pc <= high_pc)
            {
                pc_ranges_.push_back({ low_pc, high_pc });
            }
            else
            {
                etiss::log(etiss::WARNING, "Ignoring PC range with high < low: " + range);
            }
        }

        if (!pc_ranges_.empty())
        {
            etiss::log(etiss::INFO, "Loaded " + std::to_string(pc_ranges_.size()) + " PC range(s) from option.");
            return;
        }

        etiss::log(etiss::WARNING, "plugin.instruction_tracer.pc_range did not contain a usable range.");
    }

    std::ifstream pcsFile(pc_range_path);
    if (!pcsFile.is_open())
    {
        etiss::log(etiss::ERROR, "Failed to read PC range from " + pc_range_path + ". Tracing will not be activated.");
        return;
    }

    std::string line;
    if (std::getline(pcsFile, line))
    {
        std::stringstream ss(line);
        std::string token;

        if (std::getline(ss, token, ';'))
        {
            pc_ranges_.push_back({ parsePc(token), 0 });
        }

        if (!pc_ranges_.empty() && std::getline(ss, token, ';'))
        {
            pc_ranges_.back().second = parsePc(token);
        }

        const auto low_pc = pc_ranges_.empty() ? 0 : pc_ranges_.back().first;
        const auto high_pc = pc_ranges_.empty() ? 0 : pc_ranges_.back().second;
        etiss::log(etiss::INFO,
                   "Loaded PC range from " + pc_range_path + ": low=" + std::to_string(low_pc) +
                       ", high=" + std::to_string(high_pc));
    }
    else
    {
        etiss::log(etiss::WARNING, pc_range_path + " is empty or unreadable.");
    }
}

bool InstructionTracer::callback()
{
    std::lock_guard<std::mutex> guard(mutex_);
    return false;
}

bool InstructionTracer::callbackOnInstruction(etiss::instr::Instruction &instr) const
{
    return true;
}

void InstructionTracer::finalizeInstrSet(etiss::instr::ModedInstructionSet &mis) const
{
    mis.foreach (
        [this](etiss::instr::VariableInstructionSet &vis)
        {
            vis.foreach (
                [this](etiss::instr::InstructionSet &set)
                {
                    set.foreach (
                        [this](etiss::instr::Instruction &instr)
                        {
                            instr.addCallback(
                                [this, &instr](etiss::instr::BitArray &, etiss::CodeSet &cs,
                                         etiss::instr::InstructionContext &)
                                {
                                    std::stringstream ss;
                                    ss << "// InstructionTracer: collect state information\n";
                                    ss << "InstructionTracer_collect_state(" << getPointerCode() << ", cpu, \""
                                       << instr.name_ << "\");\n";
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
    block.fileglobalCode().insert("extern void InstructionTracer_collect_state(void*, ETISS_CPU*, const char*);");
}

void *InstructionTracer::getPluginHandle()
{
    return this;
}

bool InstructionTracer::isPcInRange(std::uint32_t pc) const
{
    for (const auto &range : pc_ranges_)
    {
        if (range.first <= pc && pc <= range.second)
        {
            return true;
        }
    }
    return false;
}

void InstructionTracer::collectState(ETISS_CPU *cpu, const char *instruction)
{
    const auto state = plugin_core_ ? plugin_core_->getStruct() : std::shared_ptr<etiss::VirtualStruct>{};
    const etiss_uint32 pc = static_cast<etiss_uint32>(readFieldOrZero(state, "instructionPointer"));
    auto &writer = TraceFileWriter::instance();
    const bool pc_in_range = isPcInRange(pc);

    if (!writer.isTracing() && pc_in_range)
    {
        writer.activateTrace();
    }
    else if (writer.isTracing() && !pc_in_range)
    {
        writer.deactivateTrace();
    }

    if (writer.isTracing())
    {
        StateSnapshotEntry entry{};
        entry.type = 1;
        entry.pc = pc;
        entry.sp = static_cast<etiss_uint32>(readFieldOrZero(state, "X2"));
        std::strncpy(entry.instruction, instruction, sizeof(entry.instruction) - 1);

        // The legacy trace record has fixed RV32-sized arrays. Architectures with
        // fewer GPRs or without FPRs leave the absent slots as zero.
        for (int i = 0; i < 32; ++i)
            entry.x[i] = static_cast<etiss_uint32>(readFieldOrZero(state, "X" + std::to_string(i)));

        for (int i = 0; i < 32; ++i)
            entry.f[i] = readFieldOrZero(state, "F" + std::to_string(i));

        // TODO: extend the trace schema to capture all generated CoreDSL state once
        // m2isar emits complete VirtualStruct fields for PRIV, DPC, RES_ADDR, and
        // every concrete CSR/materialized state variable.
        writer.writeStateSnapshot(entry);
    }

    if (std::string(instruction) == "cjr" && pc_in_range)
    {
        writer.deactivateTrace();
    }
}

extern "C"
{
    void InstructionTracer_collect_state(void *plugin, ETISS_CPU *cpu, const char *instruction)
    {
        static_cast<InstructionTracer *>(plugin)->collectState(cpu, instruction);
    }
}

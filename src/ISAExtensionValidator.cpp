// SPDX-License-Identifier: Apache-2.0
//
// This file is part of etiss-xvalid. It is licensed under the Apache License, Version 2.0; you may not use this file
// except in compliance with the License. You should have received a copy of the license along with this project. If not,
// see the LICENSE file.
//
// Original author: Heidi Holappa <73523507+heidi-holappa@users.noreply.github.com>
#include "etiss/xvalid/ISAExtensionValidator.h"

#include "etiss/CPUArch.h"
#include "etiss/CPUCore.h"
#include "etiss/ETISS.h"
#include "etiss/Instruction.h"
#include "etiss/VirtualStruct.h"
#include "etiss/xvalid/TraceFileWriter.h"

#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace etiss::plugin;

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

void appendPcRange(std::vector<std::pair<std::uint32_t, std::uint32_t>> &pc_ranges, const std::string &range)
{
    std::stringstream bounds(range);
    std::string low;
    std::string high;
    if (!std::getline(bounds, low, ':') || !std::getline(bounds, high, ':'))
    {
        etiss::log(etiss::WARNING, "Ignoring invalid ISAExtensionValidator PC range entry: " + range);
        return;
    }

    const auto low_pc = parsePc(low);
    const auto high_pc = parsePc(high);
    if (low_pc <= high_pc)
    {
        pc_ranges.push_back({ low_pc, high_pc });
    }
    else
    {
        etiss::log(etiss::WARNING, "Ignoring ISAExtensionValidator PC range with high < low: " + range);
    }
}
}

ISAExtensionValidator::ISAExtensionValidator(std::string instruction_filter, std::string pc_range,
                                             std::string pc_range_path)
{
    std::stringstream instructions(instruction_filter.empty() ? "*" : instruction_filter);
    std::string instruction;
    while (std::getline(instructions, instruction, ','))
    {
        instruction = trim(instruction);
        if (instruction == "*")
        {
            trace_all_instructions_ = true;
        }
        else if (!instruction.empty())
        {
            instructions_with_callback_.insert(instruction);
        }
    }

    if (!pc_range.empty())
    {
        std::stringstream ranges(pc_range);
        std::string range;
        while (std::getline(ranges, range, ','))
        {
            appendPcRange(pc_ranges_, range);
        }
    }
    else if (!pc_range_path.empty())
    {
        std::ifstream pcsFile(pc_range_path);
        std::string line;
        if (std::getline(pcsFile, line))
        {
            std::stringstream fileRange(line);
            std::string low;
            std::string high;
            if (std::getline(fileRange, low, ';') && std::getline(fileRange, high, ';'))
            {
                appendPcRange(pc_ranges_, low + ":" + high);
            }
        }
    }

    etiss::log(etiss::INFO,
               "ISAExtensionValidator configured with " +
                   std::to_string(trace_all_instructions_ ? 0 : instructions_with_callback_.size()) +
                   (trace_all_instructions_ ? " named instruction filters (all enabled)" : " instruction filter(s)") +
                   " and " + std::to_string(pc_ranges_.size()) + " PC range(s).");
}

void ISAExtensionValidator::initInstrSet(etiss::instr::ModedInstructionSet &) const
{
}

void ISAExtensionValidator::finalizeInstrSet(etiss::instr::ModedInstructionSet &mis) const
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
                            if (!shouldInstrumentInstruction(instr.name_))
                            {
                                return;
                            }

                            const auto instruction_name = instr.name_;
                            instr.addCallback(
                                [this, instruction_name](etiss::instr::BitArray &, etiss::CodeSet &cs,
                                                         etiss::instr::InstructionContext &)
                                {
                                    std::stringstream ss;
                                    ss << "// ISAExtensionValidation: collect state information\n";
                                    ss << "ISAExtensionValidation_collect_state(" << getPointerCode() << ", cpu, \""
                                       << instruction_name << "\");\n";
                                    cs.append(etiss::CodePart::PREINITIALDEBUGRETURNING).code() = ss.str();
                                    return true;
                                },
                                0);
                        });
                });
        });
}

void ISAExtensionValidator::initCodeBlock(etiss::CodeBlock &block) const
{
    block.fileglobalCode().insert("extern void ISAExtensionValidation_collect_state(void*, ETISS_CPU*, const char*);");
}

void ISAExtensionValidator::finalizeCodeBlock(etiss::CodeBlock &) const
{
}

void *ISAExtensionValidator::getPluginHandle()
{
    return this;
}

std::string ISAExtensionValidator::_getPluginName() const
{
    return "ISAExtensionValidator";
}

bool ISAExtensionValidator::shouldInstrumentInstruction(const std::string &instruction) const
{
    return trace_all_instructions_ || instructions_with_callback_.find(instruction) != instructions_with_callback_.end();
}

bool ISAExtensionValidator::isPcInRange(std::uint32_t pc) const
{
    if (pc_ranges_.empty())
    {
        return true;
    }

    for (const auto &range : pc_ranges_)
    {
        if (range.first <= pc && pc <= range.second)
        {
            return true;
        }
    }
    return false;
}

extern "C"
{
    void ISAExtensionValidation_collect_state(void *plugin, ETISS_CPU *cpu, const char *instruction)
    {
        static_cast<ISAExtensionValidator *>(plugin)->collectState(cpu, instruction);
    }
}

void ISAExtensionValidator::collectState(ETISS_CPU *, const char *instruction)
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

        for (int i = 0; i < 32; ++i)
            entry.x[i] = static_cast<etiss_uint32>(readFieldOrZero(state, "X" + std::to_string(i)));

        for (int i = 0; i < 32; ++i)
            entry.f[i] = readFieldOrZero(state, "F" + std::to_string(i));

        writer.writeStateSnapshot(entry);
    }
}

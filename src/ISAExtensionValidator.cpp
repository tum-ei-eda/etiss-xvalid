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
#include "etiss/Instruction.h"
#include "etiss/VirtualStruct.h"

#include <fstream>
#include <iostream>
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

std::vector<std::string> indexedFields(const std::shared_ptr<etiss::VirtualStruct> &state, const std::string &prefix)
{
    std::vector<std::string> names;
    for (int i = 0;; ++i)
    {
        const std::string name = prefix + std::to_string(i);
        if (!state || !state->findName(name))
        {
            break;
        }
        names.push_back(name);
    }
    return names;
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
    std::stringstream instructions(instruction_filter.empty() ? "cjr" : instruction_filter);
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
    std::cout << "ISAExtensionValidator::initInstrSet" << std::endl;
}

void ISAExtensionValidator::finalizeInstrSet(etiss::instr::ModedInstructionSet &mis) const
{
    std::cout << "ISAExtensionValidator::finalizeInstrSet" << std::endl;
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

                            instr.addCallback(
                                [this](etiss::instr::BitArray &, etiss::CodeSet &cs,
                                   etiss::instr::InstructionContext &)
                                {
                                    std::stringstream ss;
                                    ss << "// ISAExtensionValidation: collect state information\n";
                                    ss << "ISAExtensionValidation_collect_state(" << getPointerCode() << ", cpu);\n";
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
    std::cout << "ISAExtensionValidator::initCodeBlock" << std::endl;
    block.fileglobalCode().insert("extern void ISAExtensionValidation_collect_state(void*, ETISS_CPU*);");
}

void ISAExtensionValidator::finalizeCodeBlock(etiss::CodeBlock &) const
{
    std::cout << "ISAExtensionValidator::finalizeCodeBlock" << std::endl;
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

bool ISAExtensionValidator::shouldCollectPc(std::uint32_t pc) const
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
    void ISAExtensionValidation_collect_state(void *plugin, ETISS_CPU *cpu)
    {
        static_cast<ISAExtensionValidator *>(plugin)->collectState(cpu);
    }
}

void ISAExtensionValidator::collectState(ETISS_CPU *)
{
    const auto state = plugin_core_ ? plugin_core_->getStruct() : std::shared_ptr<etiss::VirtualStruct>{};
    const etiss_uint32 pc = static_cast<etiss_uint32>(readFieldOrZero(state, "instructionPointer"));
    if (!shouldCollectPc(pc))
    {
        return;
    }

    const auto xNames = indexedFields(state, "X");
    const auto fNames = indexedFields(state, "F");

    printf("X[%s]: %u\n", "PC", pc);

    for (std::size_t i = 0; i < xNames.size(); ++i)
    {
        printf("%s: %u", xNames[i].c_str(), static_cast<etiss_uint32>(readFieldOrZero(state, xNames[i])));
        printf("%s", i + 1 == xNames.size() ? "\n" : ", ");
    }

    for (std::size_t i = 0; i < fNames.size(); ++i)
    {
        printf("%s: %lu", fNames[i].c_str(), readFieldOrZero(state, fNames[i]));
        printf("%s", i + 1 == fNames.size() ? "\n" : ", ");
    }
}

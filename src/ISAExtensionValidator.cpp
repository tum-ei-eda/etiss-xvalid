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

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace etiss::plugin;

namespace
{
const std::unordered_set<std::string> instructionsWithCallback = { "cjr" };

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
                            if (instructionsWithCallback.find(instr.name_) == instructionsWithCallback.end())
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

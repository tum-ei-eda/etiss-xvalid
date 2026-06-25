// SPDX-License-Identifier: Apache-2.0
//
// This file is part of etiss-xvalid. It is licensed under the Apache License, Version 2.0; you may not use this file
// except in compliance with the License. You should have received a copy of the license along with this project. If not,
// see the LICENSE file.
//
// Original author: Heidi Holappa <73523507+heidi-holappa@users.noreply.github.com>
#include "etiss/xvalid/ISAExtensionValidator.h"

#include "etiss/CPUArch.h"
#include "etiss/Instruction.h"
#include "etiss/xvalid/CpuArchConfig.h"

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>

using namespace etiss::plugin;

namespace
{
const std::unordered_set<std::string> instructionsWithCallback = { "cjr" };
}

void ISAExtensionValidator::initInstrSet(etiss::instr::ModedInstructionSet &) const
{
    std::cout << "ISAExtensionValidator::initInstrSet" << std::endl;
}

void ISAExtensionValidator::finalizeInstrSet(etiss::instr::ModedInstructionSet &mis) const
{
    std::cout << "ISAExtensionValidator::finalizeInstrSet" << std::endl;
    mis.foreach (
        [](etiss::instr::VariableInstructionSet &vis)
        {
            vis.foreach (
                [](etiss::instr::InstructionSet &set)
                {
                    set.foreach (
                        [](etiss::instr::Instruction &instr)
                        {
                            if (instructionsWithCallback.find(instr.name_) == instructionsWithCallback.end())
                            {
                                return;
                            }

                            instr.addCallback(
                                [](etiss::instr::BitArray &, etiss::CodeSet &cs,
                                   etiss::instr::InstructionContext &)
                                {
                                    std::stringstream ss;
                                    ss << "// ISAExtensionValidation: collect state information\n";
                                    ss << "ISAExtensionValidation_collect_state(("
                                       << XVALID_STRINGIFY(XVALID_CPU_TYPE) << "*) cpu);\n";
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
    block.fileglobalCode().insert("extern void ISAExtensionValidation_collect_state(" XVALID_STRINGIFY(
        XVALID_CPU_TYPE) "*);");
}

void ISAExtensionValidator::finalizeCodeBlock(etiss::CodeBlock &) const
{
    std::cout << "ISAExtensionValidator::finalizeCodeBlock" << std::endl;
}

void *ISAExtensionValidator::getPluginHandle()
{
    return nullptr;
}

std::string ISAExtensionValidator::_getPluginName() const
{
    return "ISAExtensionValidator";
}

extern "C"
{
    void ISAExtensionValidation_collect_state(XVALID_CPU_TYPE *cpu)
    {
        const etiss_uint32 pc = static_cast<etiss_uint32>(reinterpret_cast<ETISS_CPU *>(cpu)->instructionPointer);

        etiss_uint32 x[32];
        for (int i = 0; i < 32; ++i)
            x[i] = *cpu->X[i];

        etiss_uint64 f[32];
        for (int i = 0; i < 32; ++i)
            f[i] = *cpu->F[i];

        printf("X[%s]: %u\n", "PC", pc);

        for (int i = 0; i < 32; ++i)
        {
            printf("%c[%d]: %u", 'X', i, x[i]);
            printf("%s", i == 31 ? "\n" : ", ");
        }

        for (int i = 0; i < 32; ++i)
        {
            printf("%c[%d]: %lu", 'F', i, f[i]);
            printf("%s", i == 31 ? "\n" : ", ");
        }
    }
}

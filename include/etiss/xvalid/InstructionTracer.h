// SPDX-License-Identifier: Apache-2.0
//
// This file is part of etiss-xvalid. It is licensed under the Apache License, Version 2.0; you may not use this file
// except in compliance with the License. You should have received a copy of the license along with this project. If not,
// see the LICENSE file.
//
// Original author: Heidi Holappa <73523507+heidi-holappa@users.noreply.github.com>
#ifndef ETISS_XVALID_INSTRUCTIONTRACER_H_
#define ETISS_XVALID_INSTRUCTIONTRACER_H_

#include "etiss/IntegratedLibrary/InstructionSpecificAddressCallback.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

class InstructionTracer : public etiss::plugin::InstructionSpecificAddressCallback
{
  public:
    explicit InstructionTracer(std::string pc_range_path = "pcs.tmp");
    InstructionTracer(std::string pc_range, std::string pc_range_path);
    void collectState(ETISS_CPU *cpu, const char *instruction);

  private:
    std::vector<std::pair<std::uint32_t, std::uint32_t>> pc_ranges_;
    std::string output_path_;
    std::string snapshot_content_;
    int counter_ = 0;
    std::mutex mutex_;

    bool isPcInRange(std::uint32_t pc) const;

    bool callback() override;
    bool callbackOnInstruction(etiss::instr::Instruction &instr) const override;

    void initCodeBlock(etiss::CodeBlock &) const override;
    void finalizeInstrSet(etiss::instr::ModedInstructionSet &) const override;
    void *getPluginHandle() override;
};

#endif

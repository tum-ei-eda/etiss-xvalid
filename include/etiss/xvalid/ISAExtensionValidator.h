// SPDX-License-Identifier: Apache-2.0
//
// This file is part of etiss-xvalid. It is licensed under the Apache License, Version 2.0; you may not use this file
// except in compliance with the License. You should have received a copy of the license along with this project. If not,
// see the LICENSE file.
//
// Original author: Heidi Holappa <73523507+heidi-holappa@users.noreply.github.com>
#ifndef ETISS_XVALID_ISAEXTENSIONVALIDATOR_H_
#define ETISS_XVALID_ISAEXTENSIONVALIDATOR_H_

#include "etiss/Plugin.h"

#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace etiss
{
namespace plugin
{

class ISAExtensionValidator : public etiss::TranslationPlugin
{
  public:
    explicit ISAExtensionValidator(std::string instruction_filter = "cjr", std::string pc_range = "",
                                   std::string pc_range_path = "");
    void collectState(ETISS_CPU *cpu);
    void initInstrSet(etiss::instr::ModedInstructionSet &) const override;
    void finalizeInstrSet(etiss::instr::ModedInstructionSet &) const override;
    void initCodeBlock(etiss::CodeBlock &) const override;
    void finalizeCodeBlock(etiss::CodeBlock &) const override;
    void *getPluginHandle() override;
    std::string _getPluginName() const override;

  private:
    std::unordered_set<std::string> instructions_with_callback_;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> pc_ranges_;
    bool trace_all_instructions_ = false;

    bool shouldInstrumentInstruction(const std::string &instruction) const;
    bool shouldCollectPc(std::uint32_t pc) const;
};

} // namespace plugin
} // namespace etiss

#endif

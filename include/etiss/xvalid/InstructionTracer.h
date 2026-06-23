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

#include <mutex>
#include <string>

class InstructionTracer : public etiss::plugin::InstructionSpecificAddressCallback
{
  public:
    InstructionTracer();

  private:
    std::string output_path_;
    std::string snapshot_content_;
    int counter_ = 0;
    std::mutex mutex_;

    bool callback() override;
    bool callbackOnInstruction(etiss::instr::Instruction &instr) const override;

    void initCodeBlock(etiss::CodeBlock &) const override;
    void finalizeInstrSet(etiss::instr::ModedInstructionSet &) const override;
};

#endif

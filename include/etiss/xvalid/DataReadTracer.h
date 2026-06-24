// SPDX-License-Identifier: Apache-2.0
//
// This file is part of etiss-xvalid. It is licensed under the Apache License, Version 2.0; you may not use this file
// except in compliance with the License. You should have received a copy of the license along with this project. If not,
// see the LICENSE file.
#ifndef ETISS_XVALID_DATAREADTRACER_H_
#define ETISS_XVALID_DATAREADTRACER_H_

#include "etiss/IntegratedLibrary/SelectiveSysWrapper.h"
#include "etiss/Plugin.h"

namespace etiss
{
namespace plugin
{

class DataReadTracer : public etiss::plugin::SelectiveSysWrapper
{
  public:
    struct CustomHandle
    {
        uint64_t addr = 0;
        uint64_t mask = 0;
        ETISS_System *origSys = nullptr;
    };

    DataReadTracer(uint64_t addrValue, uint64_t addrMask);

    ETISS_System getWrapInfo(ETISS_System *origSystem) final;

  protected:
    std::string _getPluginName() const override { return "DataReadTracer"; }

  private:
    CustomHandle customHandle_;
};

} // namespace plugin
} // namespace etiss

#endif

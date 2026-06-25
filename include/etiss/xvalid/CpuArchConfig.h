// SPDX-License-Identifier: Apache-2.0
//
// This file is part of etiss-xvalid. It is licensed under the Apache License, Version 2.0; you may not use this file
// except in compliance with the License. You should have received a copy of the license along with this project. If not,
// see the LICENSE file.
#ifndef ETISS_XVALID_CPU_ARCH_CONFIG_H_
#define ETISS_XVALID_CPU_ARCH_CONFIG_H_

#ifndef XVALID_CPU_TYPE
#define XVALID_CPU_TYPE RV32IMACFD
#endif

#ifndef XVALID_CPU_HEADER
#define XVALID_CPU_HEADER "Arch/RV32IMACFD/RV32IMACFD.h"
#endif

#define XVALID_STRINGIFY_IMPL(value) #value
#define XVALID_STRINGIFY(value) XVALID_STRINGIFY_IMPL(value)

#include XVALID_CPU_HEADER

#endif

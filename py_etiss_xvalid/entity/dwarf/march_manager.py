# SPDX-License-Identifier: Apache-2.0
#
# This file is part of etiss-xvalid. It is licensed under the Apache License, Version 2.0; you may not use this file
# except in compliance with the License. You should have received a copy of the license along with this project. If not,
# see the LICENSE file.
#
# Original author: Heidi Holappa <73523507+heidi-holappa@users.noreply.github.com>
from py_etiss_xvalid.entity.march.march_base import MArchBase
from py_etiss_xvalid.entity.march.rv32im_zicsr_zifencei import RV32IM_zicsr_zifencei
from py_etiss_xvalid.entity.march.rv32imac_zicsr import RV32IMAC_zicsr
from py_etiss_xvalid.entity.march.rv32imafdc_zicsr_zifencei import RV32IMAFDC_zicsr_zifencei


class MArchManager:

    def __init__(self):
        self.supported_marchs = [
            RV32IMAC_zicsr(),
            RV32IMAFDC_zicsr_zifencei(),
            RV32IM_zicsr_zifencei()
        ]

    def march_is_supported(self, march: str) -> bool:
        is_supported = False
        for m in self.supported_marchs:
            if march == m.get_march_name():
                is_supported = True
                break
        return is_supported


    def get_march_with_name(self, march_name: str) -> MArchBase:
        march = None
        for m in self.supported_marchs:
            if march_name == m.get_march_name():
                march = m
                break
        return march

# SPDX-License-Identifier: Apache-2.0
#
# This file is part of etiss-xvalid. It is licensed under the Apache License, Version 2.0; you may not use this file
# except in compliance with the License. You should have received a copy of the license along with this project. If not,
# see the LICENSE file.
#
# Original author: Heidi Holappa <73523507+heidi-holappa@users.noreply.github.com>
from typing import List

from py_etiss_xvalid.entity.dwarf.formal_parameter import FormalParameter
from py_etiss_xvalid.entity.dwarf.frame_base_info import FrameBaseInfo
from py_etiss_xvalid.entity.dwarf.local_variable import LocalVariable
from py_etiss_xvalid.entity.dwarf.types import AbstractType

"""
    A class instance of representing data extracted from subprogram DIE
"""
class Subprogram:

    def __init__(self):
        self.name:  str = ""
        self.low_pc: int = 0
        self.high_pc: int = 0
        self.type_info: None | AbstractType = None
        self.frame_base_infos: List[FrameBaseInfo] = []
        self._formal_parameters = []
        self._local_variables = []
        self._indent: int = 2

    def add_frame_base_info(self, frame_base_info: FrameBaseInfo) -> None:
        if not self.frame_base_infos:
            self.frame_base_infos.append(frame_base_info)
        elif self.frame_base_infos[-1].get_reg() != frame_base_info.get_reg() or self.frame_base_infos[-1].get_offset() != frame_base_info.get_offset():
            self.frame_base_infos.append(frame_base_info)

    def add_formal_parameter(self, formal_param: FormalParameter) -> None:
        self._formal_parameters.append(formal_param)

    def get_formal_parameters(self) -> List[FormalParameter]:
        return self._formal_parameters

    def has_formal_parameters(self) -> bool:
        return bool(self._formal_parameters)

    def add_local_variable(self, var: LocalVariable) -> None:
        self._local_variables.append(var)

    def get_local_variables(self) -> List[LocalVariable]:
        return self._local_variables

    def has_local_variables(self) -> bool:
        return bool(self._local_variables)

    def __str__(self) -> str:
        output = ""
        output += f"{self._indent * ' '}> Subprogram:\n"
        output += f"{self._indent * ' '}  ┌ Name: {self.name}\n"
        if self.frame_base_infos:
            output += f"{self._indent * ' '}  ├ CFA register values and offsets:\n"
            for info in self.frame_base_infos:
                output += str(info)
        if self.type_info:
            base = self.type_info.get_base()
            range = self.type_info.get_range()
            output += f"{self._indent * ' '}  ├ Type composition: {str(self.type_info)}\n"
            output += f"{self._indent * ' '}  ├ Base type: {base.type}\n"
            output += f"{self._indent * ' '}  ├ Base type byte size: {base.byte_size}\n"
            if range != base.byte_size:
                output += f"{self._indent * ' '}  ├ Range: {range}\n"
        else:
            output += f"{self._indent * ' '}  ├ Type composition: void\n"
        output += f"{self._indent * ' '}  ├ Low-PC: {self.low_pc}\n"
        output += f"{self._indent * ' '}  └ High-PC: {self.high_pc}\n"
        return output




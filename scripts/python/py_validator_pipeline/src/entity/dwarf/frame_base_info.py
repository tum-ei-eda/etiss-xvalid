# SPDX-License-Identifier: Apache-2.0
#
# This file is part of etiss-xvalid. It is licensed under the Apache License, Version 2.0; you may not use this file
# except in compliance with the License. You should have received a copy of the license along with this project. If not,
# see the LICENSE file.
#
# Original author: Heidi Holappa <73523507+heidi-holappa@users.noreply.github.com>
class FrameBaseInfo:

    def __init__(self, pc, reg, offset) -> None:
        self._pc = pc
        self._reg = reg
        self._offset = offset


    def get_pc(self):
        return self._pc

    def get_reg(self):
        return self._reg

    def get_offset(self):
        return self._offset

    def __str__(self) -> str:
        result = f"    ├── pc: {self._pc}, reg value: {self._reg}, offset: {self._offset}\n"
        return result


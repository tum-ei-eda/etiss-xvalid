# SPDX-License-Identifier: Apache-2.0
#
# This file is part of etiss-xvalid. It is licensed under the Apache License, Version 2.0; you may not use this file
# except in compliance with the License. You should have received a copy of the license along with this project. If not,
# see the LICENSE file.
#
# Original author: Heidi Holappa <73523507+heidi-holappa@users.noreply.github.com>
import re
from py_etiss_xvalid.entity.dwarf.var_and_param_base import VarAndParamBase

class LocalVariable(VarAndParamBase):

    def __init__(self) -> None:
        super().__init__()
        self._indent = 6  # override

    def get_location_value(self):
        # For now we assume all formal params are in fbregs
        parsed_loc = re.search(r"\(DW_OP_fbreg:\s*([-+]?\d+)\)", self._location).group(1)
        return int(parsed_loc)


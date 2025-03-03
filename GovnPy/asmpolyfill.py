# GovnCore 16X assembler polyfill
# Copyright (C) 2025 t.me/pyproman
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import re
import argparse
parser = argparse.ArgumentParser(prog='asmpolyfill', description='GovnAsm Polyfill')
parser.add_argument('source', type=str, help='GovnAsm source code, will be patched in-place')
args = parser.parse_args()
regs = "ax bx cx dx si gi sp bp".split(" ") # Todo: other regs
with open(args.source) as f:
    data = f.read()

ndata = data

def patch(regex):
    def applier(fn):
        global ndata
        def wrapper(match):
            return f"; {match.group(0).strip()}\n  {fn(*match.groups())}"
        ndata = re.sub(r"\b" + regex + r"\b", wrapper, ndata)
    return applier

@patch(r"storw %(..)")
def storw(reg):
    if reg != "ax":
        raise ValueError(f"Storw only supports %ax (got {reg})")
    return "bytes $8B"

@patch(r"lodsw")
def lodsw():
    return "bytes $8A"

@patch(r"xchg %(..) %(..)")
def xchg(a, b):
    r1 = regs.index(a)
    r2 = regs.index(b)
    return f"bytes $88 ${r1*16+r2:02X}"


@patch(r"and %(..) %(..)")
def xchg(a, b):
    r1 = regs.index(a)
    r2 = regs.index(b)
    return f"bytes $10 $E1 ${r1*16+r2:02X}"

@patch(r"or %(..) %(..)")
def xchg(a, b):
    r1 = regs.index(a)
    r2 = regs.index(b)
    return f"bytes $10 $E2 ${r1*16+r2:02X}"

@patch(r"jmp %(..)")
def jmpr(a):
    return f"bytes $0F $31 ${regs.index(a):02X}"

ndata = ndata.replace("pop %bp\n  push %bp", "cop %bp")
with open(args.source, "w") as f:
    f.write(ndata)

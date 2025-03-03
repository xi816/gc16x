from functools import reduce
# GovnFont generator
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

from operator import or_
from sys import argv
from PIL import Image


print("Mekesoft Graphics Framework Font Converter")
print()

sprite_table = {}

for fname in argv[1:]:
    if not (fname.startswith("U+") and fname.endswith(".png") and fname[2:-4].isalnum()):
        print(f"Skipping {fname} - invalid name")
    base = int(fname[2:-4], 16)
    print(f"Loading {fname}")
    with Image.open(fname) as im:
        ch_size = im.size[1] // 9, im.size[0] // 7
        print(f"| {ch_size[0] * ch_size[1]} characters: \\u{base:04x}-\\u{base + ch_size[0] * ch_size[1]:04x}")
        bitmap = im.load()
        chars = 0
        for i in range(ch_size[0]):
            for j in range(ch_size[1]):
                if bitmap[j * 7 + 1, i * 9 + 1] == (255, 0, 0):
                    # invalid character
                    continue
                chars += 1
                sprite = bytes((int(''.join(
                        '1'
                        if bitmap[j * 7 + kk, i * 9 + k] in ((0, 0, 0), (0, 0, 0, 255), 0)
                        else '0'
                        for kk in range(1, 6)
                    ), 2) for k in range(1, 8)))

                used_bits = f'{reduce(or_, sprite):8b}'
                if '1' in used_bits:
                    start, end = used_bits.find('1'), used_bits.rfind('1')
                else:  # space
                    start, end = 0, 0
                sprite_table[base + i * ch_size[1] + j] = bytes((i << start for i in sprite)), end - start + 2
    print(f"| {chars} detected")

print(f"Total: {len(sprite_table)} characters.")
print(f"Generating GovnFont(TM) for GovnUtf8...")
GF1_SPRLEN = 8
GF1_START = 0x20
GF1_END = 0x100
file = bytearray((GF1_END - GF1_START) * (GF1_SPRLEN + 1))
for i in range(GF1_START, GF1_END):
    try:
        if i < 0x80:
            ui = i
        else:
            ui = i - 0x80 + 0x400
        char, char_len = sprite_table[ui]
        base = (GF1_SPRLEN + 1) * (i - GF1_START)
        print(len(file), i, base)
        file[base:base+GF1_SPRLEN-1] = bytes(int(f'{b:08b}'[::-1],2) for b in char)
        file[base+GF1_SPRLEN] = char_len
    except KeyError:
        print(f"{i:02x} is not defined in the font")

print("Flushing to disk...")
with open("target.gf1", "wb") as f:
    f.write(file)

print("ok!")

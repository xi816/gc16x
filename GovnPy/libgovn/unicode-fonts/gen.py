#! /usr/bin/python3

import PIL.Image
import sys
import os
import re

if len(sys.argv) != 3:
    print(f"usage: {sys.argv[0]} <FONT FOLDER> <OUTPUT IMAGE>")
    sys.exit(1)

files = sorted(sys.argv[1] + "/" + x for x in os.listdir(sys.argv[1]))
chars = PIL.Image.open("gen-chars.png")

WHITE = (255, 255, 255)
BLACK = (0, 0, 0)

def empty_separator(width):
    img = PIL.Image.new("RGB", (width, 9), color=WHITE)

    for j in range(3, 6):
        for i in range(1, width-1):
            if not ((i + j) & 1):
                img.putpixel((i, j), BLACK)

    return img

def char_index(c):
    o = ord(c)
    if ord('0') <= o <= ord('9'):
        return o - ord('0')
    if ord('A') <= o <= ord('F'):
        return o - ord('A') + 10
    if c == 'U':
        return 16
    if c == '+':
        return 17
    raise Exception(f"Unsupported char '{c}'")

def char_image(c):
    index = 4 * char_index(c)
    return chars.crop((index, 0, index + 3, 5))

def separator(start, width):
    img = PIL.Image.new("RGB", (width, 9), color=WHITE)

    for j in range(3, 6):
        for i in range(1, width//2 - 15):
            if not ((i + j) & 1):
                img.putpixel((i, j), BLACK)
        for i in range(width//2 + 15, width-1):
            if not ((i + j) & 1):
                img.putpixel((i, j), BLACK)

    x = width//2 - 11
    for c in "U+{:04X}".format(start):
        img.paste(char_image(c), (x, 2))
        x += 4

    return img

class Block:
    def __init__(self, path):
        basename = os.path.basename(path)
        match = re.search(r'U\+([0-9A-Za-z]{4})', basename)
        self.block_start = None

        self.img = PIL.Image.open(path)

        if match is not None:
            self.block_start = int(match[1], 16)
            print('{} U+{:04X}'.format(basename, self.block_start))
        else:
            print(f'{basename} not a block')

    def header(self, width):
        if self.block_start is not None:
            return separator(self.block_start, width)
        else:
            return empty_separator(width)

    def body(self):
        return self.img

    def height(self):
        return self.img.height + 9

    def width(self):
        return self.img.width

blocks = [ Block(file) for file in files ]
width = max(b.width() for b in blocks)
height = sum(b.height() for b in blocks)

left_height = 0
for b in blocks:
    left_height += b.height()
    if 2 * left_height >= height: break

result = PIL.Image.new("RGB", (2 * width + 8, left_height), color=WHITE)

x = 0
y = 0

for b in blocks:
    header = b.header(width)
    result.paste(header, (x, y))
    y += header.height

    body = b.body()
    result.paste(body, (x, y))
    y += body.height

    if y >= left_height:
        y = 0
        x += width + 8

result.save(sys.argv[2])

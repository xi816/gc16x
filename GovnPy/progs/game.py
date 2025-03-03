# GovnPy game example
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
import govn.game
import govn.gui
import govn.font

TAIL_SPR = spr(
    " #####  \n"
    "####### \n"
    "####### \n"
    "####### \n"
    "####### \n"
    "####### \n"
    " #####  \n"
    "        "
)
SNAKE_SPR = spr(
    " #####  \n"
    "####### \n"
    "####### \n"
    "# ### # \n"
    "####### \n"
    "####### \n"
    " #####  \n"
    "        "
)
APPLE_SPR = spr(
    " #####  \n"
    "## #### \n"
    "# ##### \n"
    "####### \n"
    "####### \n"
    "####### \n"
    " #####  \n"
    "        "
)

snake_tail = bytes(100)
snake_tail_len = bytes(1)
snake_dir = bytes(1)
# . 1 .
# 2 . 3
# . 4 .
snake_x = bytes(1)
snake_y = bytes(1)

apple_x = bytes(1)
apple_y = bytes(1)

def init():
    snake_dir[0] = 3
    gen_apple()

def gen_apple():
    apple_x[0] = rand() % 10
    apple_y[0] = rand() % 10

def tick():
    # Go forward
    old_x = snake_x[0]
    old_y = snake_y[0]
    # Go forward
    if snake_dir[0] == 1:
        snake_y[0] = snake_y[0] - 1
    elif snake_dir[0] == 2:
        snake_x[0] = snake_x[0] - 1
    elif snake_dir[0] == 3:
        snake_x[0] = snake_x[0] + 1
    else:
        snake_y[0] = snake_y[0] + 1

    # Wall crash
    if snake_x[0] == 0xFF or snake_x[0] == 10:
        puts("You can't escape...\n")
        return 1
    if snake_y[0] == 0xFF or snake_y[0] == 10:
        puts("You can't escape...\n")
        return 1

    # Self crash
    if snake_tail_len[0]:
        i = 1
        while i != snake_tail_len[0]:
            if (snake_tail[i * 2] == snake_x[0]
            and snake_tail[i * 2 + 1] == snake_y[0]):
                puts("You hit yourself...\n")
                return 1
            i = i + 1

    if snake_x[0] == apple_x[0] and snake_y[0] == apple_y[0]:
        snake_tail_len[0] = snake_tail_len[0] + 1
        if snake_tail_len[0] == 100:
            puts("You won! :D\n")
            return 2
        gen_apple()
    else:
        i = 0
        while i != snake_tail_len[0]:
            snake_tail[i * 2] = snake_tail[i * 2 + 2]
            snake_tail[i * 2 + 1] = snake_tail[i * 2 + 3]
            i = i + 1
    if snake_tail_len[0]:
        snake_tail[snake_tail_len[0] * 2 - 2] = old_x
        snake_tail[snake_tail_len[0] * 2 - 1] = old_y

def frame():
    cls(0)
    vputs(0, 0, 15, "GovnSnake\nContains Terminal-as-Timer technology")
    box(130, 55, 80, 80, 8)
    if cur_frame[0] % 5 == 4:
        i = tick()
        if i == 1:
            puts("Game Over :(\n")
            return 1
        elif i == 2:
            return 256
    i = 0
    while i != snake_tail_len[0]:
        spr(130 + snake_tail[i * 2] * 8, 55 + snake_tail[i * 2 + 1] * 8, 15, TAIL_SPR)
        i = i + 1
    spr(130 + snake_x[0] * 8, 55 + snake_y[0] * 8, 15, SNAKE_SPR)
    spr(130 + apple_x[0] * 8, 55 + apple_y[0] * 8, 10, APPLE_SPR)
    show()

def on_key(k):
    if k == 0x0a or k == ord('q'):
        return 52
    elif k == ord('w'):
        snake_dir[0] = 1
    elif k == ord('a'):
        snake_dir[0] = 2
    elif k == ord('s'):
        snake_dir[0] = 4
    elif k == ord('d'):
        snake_dir[0] = 3

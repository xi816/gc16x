# GovnPy shell example
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
import govn.std
buf = bytes(32)
def rfib(x):
    if x == 1 or x == 0:
        return 1
    return rfib(x-1) + rfib(x-2)
def sfib(x):
    a = 1
    b = 1
    while x:
        a = b + a

        b = b + a
        a = b - a
        b = b - a

        x = x - 1
    return a
def getint(bf):
    c = 0
    i = 0
    while bf[i]:
        c = c * 10 + (bf[i] - 0x30)
        i = i + 1
    return c
def fibcalc():
    puts("Number> ")
    gets(buf)
    puts("Result: ")
    puti(rfib(getint(buf)))
    puts("\n")
lost_cpu_mem = bytes(400)
def lost_editor():
    puts('Lost Core Input Mode\nType code, end with . on a seperate line\n')
    ptr = lost_cpu_mem
    flag = True
    while flag:
        auxptr = ptr
        ptr = gets(ptr)
        ptr[0] = 0x0a
        if ptr == auxptr + 1:
            if auxptr[0] == 0x2e:
                auxptr[0] = 0
                flag = False
        ptr = ptr + 1
def lost_emulator():
    puts("\x1b[9mLox\x1b[0m Lost Core Starting...\n") #]
    ptr = lost_cpu_mem
    aif = False
    while ptr[0]:
        if ptr[0] == 0xa:
            ptr = ptr + 1
        elif memcmp(6, ptr, "print "):
            auxptr = ptr + 6
            while auxptr[0] != 0x0a:
                auxptr = auxptr + 1
            auxptr[0] = 0
            puts(ptr + 6)
            puts("\n")
            if aif:
                puts("[SYS] Condition met for print: ")
                puts(ptr+6)
                puts("\n")
                aif = False
            ptr = auxptr + 1
        elif memcmp(6, ptr, "print\n"):
            puts("\n")
            ptr = ptr + 6
        elif memcmp(12, ptr, "input_string"):
            puts("Lost -> ")
            gets(buf)
            ptr = ptr + 13
        elif memcmp(3, ptr, "if "):
            auxptr = ptr + 3
            while auxptr[0] != 0x0a:
                auxptr = auxptr + 1
            auxptr[0] = 0
            if scmp(buf, ptr + 3) == 0:
                ptr = auxptr
                while ptr[0] != 0x0a:
                    ptr = ptr + 1
                ptr = ptr + 1
            else:
                ptr = auxptr + 1
                aif = True
        else:
            puts("\x1b[31m ERROR: UNKNOWN SHIT \x1b[0m") #]
            puts(ptr)
            return 0
# def pix(x, y, c):
#     return __asm__(x, y, c,
#                    "pop %ax\n"
#                    "pop %gi\n"
#                    "pop %si\n"
#                    "mul %gi 340\n"
#                    "add %si %gi\n"
#                    "int $0C\n"
#                    "push 0")
# def gui():
#     x = 0
#     while x != 256:
#         y = 0
#         while y != 128:
#             pix(x, y, (x^y)&15)
#             y = y + 1
#         x = x + 1
#     __asm__(0, "int $11")
test_buf = bytes(4)
def gui():
    test_buf[0] = 0x31
    test_buf[1] = 0x32
    test_buf[2] = 0x33
    test_buf[3] = 0
    puts(test_buf)
    test_buf[0] = 0x33
    test_buf[1] = 0x32
    test_buf[2] = 0x31
    test_buf[3] = 0
    puts(test_buf)
    puts("\n123321 <- Must output this\n")
def shell(nest):
    flag = 1
    puts("GovnShell.py [Version 25.2.8]\n(c) 2025 Govn Industries & t.me/pyproman\n\nYou are ")
    puti(nest)
    puts(" levels away from boot\n")
    while flag:
        puts("\x1b[33msh.py>\x1b[0m ") #]
        gets(buf)
        if buf[0] == 0:
            # Ok
            pass
        elif scmp(buf, "help"):
            puts("Welcome to a shell compiled with GovnPy 🤑!\n"
                 "help     Show help\n"
                 "dir      List tags\n"
                 "fib      Calculate fibbonaci\n"
                 "newsh    Start a new shell\n"
                 "shutdown int$0(0)\n"
                 "exit     Exit shell\n"
                 "lost     Enter shedevrolostcore emulator\n"
                 "graphics XOR Graphics demo\n")
        elif scmp(buf, "dir"):
            puts("Your disk is cooked bro\n")
        elif scmp(buf, "fib"):
            fibcalc()
        elif scmp(buf, "graphics"):
            gui()
        elif scmp(buf, "lost"):
            lost_editor()
            lost_emulator()
        elif scmp(buf, "newsh"):
            shell(nest + 1)
        elif scmp(buf, "shutdown"):
            exit(0)
        elif scmp(buf, "exit"):
            puts("Deleting C/System32...\nok\n")
            flag = 0
        else:
            puts("Your command is bad... just like that apple\n")
    puts("Goodbye!\n")
def main():
    shell(0)

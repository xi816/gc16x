#!/usr/bin/env bash
# GovnPy wrapper
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

set -euo pipefail
gpy-clean() {
    rm progs/compiled/*
}
crash() {
    stty icanon isig iexten echo
    printf "\x1b[31mGC16X exited with error:\x1b[0m %s\n" $1
}
gpy-run() {
    # Parse command line
    target="2"
    mode="cli"
    while [[ "$#" -gt 0 ]]; do
        case $1 in
            -v|--version)
                target="$2"
                shift
                ;;
            -g|--gui)
                mode="gui"
                ;;
            -b|--bench)
                mode="bench"
                ;;
            -h|--help)
                echo "test.sh run [-v|--version VERSION] [-g|--gui] module"
                echo Run module from progs/ with a specified compiler version
                echo "-g (--gui): Enable SDL"
                exit 1
                ;;
            *)
                module="$1"
                ;;
        esac
        shift
    done
    case $target in
        1)
            printf "\x1b[33mYou are using a deprecated compiler version (1)\x1b[0m\n"
            python3 compiler.py progs/$module.py > progs/compiled/$module.s
            ;;
        2)
            python3 compiler2.py progs/$module.py progs/compiled/$module.s
            python3 asmpolyfill.py progs/compiled/$module.s
            ;;
        *)
            echo Unknown compiler version "$target"
            exit 1
            ;;
    esac
    python3 ~/gc16x/asm/asm progs/compiled/$module.s progs/compiled/$module.bin
    python3 ~/gc16x/asm/asm -export progs/compiled/$module.s progs/compiled/$module.sym || true
    case $mode in
        cli)
            env SDL_VIDEODRIVER=no ~/gc16x/gc16 progs/compiled/$module.bin || crash $?
            ;;
        gui)
            ~/gc16x/gc16 progs/compiled/$module.bin || crash $?
            ;;
        bench)
            time printf "fib\n27\nexit\n" | env SDL_VIDEODRIVER=no ~/gc16x/gc16 progs/compiled/$module.bin
            ;;
    esac
}

case $1 in
    clean)
        shift
        gpy-clean $*
        ;;
    run)
        shift
        gpy-run $*
        ;;
    *)
        echo Unknown command $1
        exit 1
        ;;
esac

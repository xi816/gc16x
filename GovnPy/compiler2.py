#!/usr/bin/python3
# GovnPy compiler version 2
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

import argparse
import ast
CTX_FD = 0
CTX_GLOB = 1
CTX_LBL = 2
CTX_LOC = 3
CTX_RSF = 4
LBL_IF = 1
LBL_WHILE = 2
LBL_COND = 3

def I16(v):
    if -2**16 < v < 0:
        return 2**16 + v
    elif 0 <= v < 2**16:
        return v
    else:
        return ValueError(f"{v} cant fit in I16")
def getreg(stack, ptr = -1):
    regs = ["%ax", "%bx", "%cx", "%gi"]
    # %sx for local var access
    # %dx for temporary shit
    if len(stack) > 4:
        raise ValueError("u tebya stek perepolnen durak")
    if len(stack) < -ptr > 0 or len(stack) <= ptr >= 0:
        print(f"[DEBUG] {len(stack)=} {ptr=}")
        raise IndexError("stack index out of range")
    return regs[(len(stack) + ptr if ptr < 0 else ptr) % len(regs)]
def counter(name):
    local = [0]
    def count():
        local[0] += 1
        return local[0]
    return count
def counters(*names):
    return tuple(counter(n) for n in names)
def emit_expr(ctx, node):
    match node:
        case ast.Constant():
            match node.value:
                case str():
                    sid = f"str{len(ctx[CTX_GLOB])}"
                    ctx[CTX_GLOB][sid] = node.value + "\x00"
                    ctx[CTX_RSF].append(...)
                    ctx[CTX_FD].write(f"  mov {getreg(ctx[CTX_RSF])} {sid}\n")
                case int():
                    ctx[CTX_RSF].append(...)
                    ctx[CTX_FD].write(f"  mov {getreg(ctx[CTX_RSF])} {int(node.value)}\n")
        case ast.UnaryOp():
            match node.op:
                case ast.Not():
                    emit_expr(ctx, ast.Compare(left=node.operand, comparators=[ast.Constant(value=0)], ops=[ast.Eq()]))
                    # TODO: Optimize this
                case _:
                    raise ValueError(f"unary op {node.op} not supported")
        case ast.Name():
            if node.id in ctx[CTX_LOC]:
                ctx[CTX_RSF].append(...)
                reg = getreg(ctx[CTX_RSF])
                ctx[CTX_FD].write(f"  mov %si {ctx[CTX_LOC][node.id]}\n"
                                  f"  add %si %bp\n" +
                                 (f"  xchg %ax {reg}\n" if reg != "%ax" else "") +
                                  f"  lodsw\n" +
                                 (f"  xchg %ax {reg}\n" if reg != "%ax" else ""))
            else:
                ctx[CTX_RSF].append(...)
                ctx[CTX_FD].write(f"  mov {getreg(ctx[CTX_RSF])} {node.id}\n")
        case ast.Subscript():
            # Pseudo-global vars
            if (
                isinstance(node.value, ast.Name) and node.value.id not in ctx[CTX_LOC]
                and isinstance(node.slice, ast.Constant) and node.slice.value == 0):
                ctx[CTX_RSF].append(...)
                ctx[CTX_FD].write(f"  mov {getreg(ctx[CTX_RSF])} *{node.value.id}\n")
                return
            emit_expr(ctx, node.value)
            emit_expr(ctx, node.slice)
            ctx[CTX_FD].write(f"  mov %si {getreg(ctx[CTX_RSF])}\n"
                              f"  add %si {getreg(ctx[CTX_RSF], -2)}\n"
                              f"  mov {getreg(ctx[CTX_RSF], -2)} *%si\n")
            ctx[CTX_RSF].pop()
        case ast.BoolOp():
            bid = ctx[CTX_LBL][LBL_COND]()
            for i in node.values:
                emit_expr(ctx, i)
                ctx[CTX_FD].write(f"  cmp {getreg(ctx[CTX_RSF])} 0\n" +
                                  (f"  jme .last{bid}\n" if type(node.op) == ast.And else f"  jmne .last{bid}\n"))
                ctx[CTX_RSF].pop()
            ctx[CTX_RSF].append(...)
            ctx[CTX_FD].write(f".last{bid}:\n")
        case ast.Compare():
            bid = ctx[CTX_LBL][LBL_COND]()
            if len(node.ops) != 1:
                raise ValueError(f"Ty che, {ast.dump(node)} - slishkom slozhno!")
            emit_expr(ctx, node.left)
            emit_expr(ctx, node.comparators[0])
            opcod = {
                ast.Eq: "jmne",
                ast.NotEq: "jme",
            }[type(node.ops[0])]
            ctx[CTX_FD].write(f"  cmp {getreg(ctx[CTX_RSF], -2)} {getreg(ctx[CTX_RSF])}\n"
                              f"  mov {getreg(ctx[CTX_RSF], -2)} 0\n"
                              f"  {opcod} .notone{bid}\n"
                              f"  mov {getreg(ctx[CTX_RSF], -2)} 1\n"
                              f".notone{bid}:\n")
            ctx[CTX_RSF].pop()
        case ast.BinOp():
            if isinstance(node.right, ast.Constant):
                op = {
                    ast.Add: "add {} {}",
                    ast.Sub: "sub {} {}",
                    ast.Mult: "mul {} {}",
                }[type(node.op)]
                emit_expr(ctx, node.left)
                ctx[CTX_FD].write("  " + op.format(getreg(ctx[CTX_RSF]), node.right.value) + "\n")
                return
            op = {
                ast.Add: "add {} {}",
                ast.Sub: "sub {} {}",
                ast.Mult: "mul {} {}",
                #ast.Mod: "div {0} {1} mov {0} %dx",
                ast.Mod: "mov %si {0} div {0} {1} mul {0} {1} sub %si {0} mov {0} %si",
                ast.BitXor: "mov %dx {0} or %dx {1} and {0} {1} sub {0} %dx",
                ast.BitAnd: "and {} {}"
            }[type(node.op)]
            emit_expr(ctx, node.left)
            emit_expr(ctx, node.right)
            ctx[CTX_FD].write("  " + op.format(getreg(ctx[CTX_RSF], -2), getreg(ctx[CTX_RSF], -1)) + "\n")
            ctx[CTX_RSF].pop()
        case ast.Call():
            if not isinstance(node.func, ast.Name):
                raise ValueError("Arbirtary function calling not supported")
            if node.func.id == "breakpoint":
                if len(node.args) != 0:
                    raise ValueError("Bad breakpoint() call")
                ctx[CTX_FD].write(f"  trap\n")
                return
            if node.func.id == "__asm__":
                if len(node.args) != 0:
                    print("\x1b[33m Note that one-call __asm__ is going to be deprecated \x1b[0m")
                ctx[CTX_FD].write(f"  nop\n")
                return
            if node.func.id == "ord":
                emit_expr(ctx, ast.Constant(value=ord(node.args[0].value)))
                return
            ctx[CTX_FD].write(f"  push %bp\n")
            for i, _ in enumerate(ctx[CTX_RSF]):
                ctx[CTX_FD].write(f"  push {getreg(ctx[CTX_RSF], -i)}\n")
            for i in node.args:
                if isinstance(i, ast.Constant) and isinstance(i.value, int):
                    ctx[CTX_FD].write(f"  push {int(i.value)}\n")
                else:
                    stack = []
                    emit_expr(ctx[:-1] + (stack,), i)
                    ctx[CTX_FD].write(f"  push {getreg(stack)}\n")
            if node.func.id != "putc":
                ctx[CTX_FD].write(f"  call {node.func.id}\n")
            else:
                ctx[CTX_FD].write(f"  int 2\n  push 1\n")
            ctx[CTX_RSF].append(...)
            ctx[CTX_FD].write(f"  pop {getreg(ctx[CTX_RSF])}\n")
            for i, _ in enumerate(ctx[CTX_RSF][-2::-1]):
                ctx[CTX_FD].write(f"  pop {getreg(ctx[CTX_RSF], -i)}\n")
            ctx[CTX_FD].write(f"  pop %bp\n")
        case _:
            raise ValueError(f"Unsupported expression: {type(node)}")
def emit_block(ctx, node):
    match node:
        case ast.Assign():
            if len(node.targets) != 1:
                raise ValueError("Multiple assignments are unsupported")
            match node.targets[0]:
                case ast.Name(id=vname):
                    stack = []
                    emit_expr(ctx + (stack,), node.value)
                    ctx[CTX_FD].write(f"  mov %si {ctx[CTX_LOC][vname]}\n"
                                    f"  add %si %bp\n"
                                    f"  storw {getreg(stack)}\n")
                case ast.Subscript():
                    stack = []
                    emit_expr(ctx + (stack,), node.value)
                    emit_expr(ctx + (stack,), node.targets[0].value)
                    emit_expr(ctx + (stack,), node.targets[0].slice)
                    ctx[CTX_FD].write(f"  mov %si {getreg(stack)}\n"
                                    f"  add %si {getreg(stack, -2)}\n"
                                    f"  storb {getreg(stack, -3)}\n")
        case ast.If():
            iif = ctx[CTX_LBL][LBL_IF]()
            stack = []
            emit_expr(ctx + (stack,), node.test)
            ctx[CTX_FD].write(f"  cmp {getreg(stack)} 0\n"
                              f"  jme .ifnot{iif}\n")
            for i in node.body:
                emit_block(ctx, i)
            ctx[CTX_FD].write(f"  jmp .ifend{iif}\n"
                              f".ifnot{iif}:\n")
            for i in node.orelse:
                emit_block(ctx, i)
            ctx[CTX_FD].write(f".ifend{iif}:\n")
        case ast.While():
            lid = ctx[CTX_LBL][LBL_WHILE]()
            ctx[CTX_FD].write(f".loop{lid}:\n")
            stack = []
            emit_expr(ctx + (stack,), node.test)
            ctx[CTX_FD].write(f"  cmp {getreg(stack)} 0\n"
                              f"  jme .loopend{lid}\n")
            for i in node.body:
                emit_block(ctx, i)
            ctx[CTX_FD].write(f"  jmp .loop{lid}\n"
                              f".loopend{lid}:\n")
        case ast.Expr():
            stack = []
            emit_expr(ctx + (stack,), node.value)
        case ast.Return():
            stack = []
            emit_expr(ctx + (stack,), node.value)
            ctx[CTX_FD].write(f"  push {getreg(stack)}\n"
                              f"  jmp .ret\n")
        case ast.Pass():
            pass # Pass
        case _:
            raise ValueError(f"Unsupported expression: {type(node)}")
def alloc_block(ctx, node):
    match node:
        case ast.Assign():
            if len(node.targets) != 1:
                raise ValueError("Multiple assignments are unsupported")
            if isinstance(node.targets[0], ast.Name):
                vname = node.targets[0].id
                if vname not in ctx[CTX_LOC]:
                    ctx[CTX_LOC][vname] = len(ctx[CTX_LOC]) * 2
        case ast.If():
            for i in node.body:
                alloc_block(ctx, i)
            for i in node.orelse:
                alloc_block(ctx, i)
        case ast.While():
            for i in node.body:
                alloc_block(ctx, i)
        case ast.Expr():
            pass
        case ast.Return():
            pass
        case ast.Pass():
            pass  #Pass
        case _:
            raise ValueError(f"Unsupported expression: {type(node)}")
def emit_global(ctx, node):
    match node:
        case ast.Import():
            for i in node.names:
                if not i.name.startswith("govn."):
                    raise ValueError("TODO: Python imports")
                file = f"libgovn/{i.name[5:]}.s"
                with open(file) as m:
                    ctx[CTX_FD].write(f"; {file}\n")
                    ctx[CTX_FD].write(m.read())
                    ctx[CTX_FD].write("\n")
        case ast.FunctionDef():
            if (node.args.posonlyargs
                or node.args.kwonlyargs
                or node.args.kw_defaults
                or node.args.defaults
                or node.args.vararg
                or node.args.kwarg):
                raise ValueError(f"Unsupported function definition {ast.dump(node.args)}")
            rsf = {}
            for i in node.args.args:
                if i.arg in rsf:
                    raise ValueError(f"Duplicate arg {i.arg}")
                rsf[i.arg] = len(rsf) * 2
            rsf["_ret"] = len(rsf) * 2
            for i in node.body:
                alloc_block(ctx + (..., rsf), i)
            for k in rsf.keys():
                rsf[k] = len(rsf) * 2 - rsf[k] - 2
            ctx[CTX_FD].write(f"{node.name}:\n"
                              f"  mov %bp {I16(-2*(len(rsf) - len(node.args.args) - 1))}\n"
                              f"  add %sp %bp\n" # yep
                              f"  mov %bp %sp\n"
                              f"  inx %bp\n")
            nctx = ctx + ((node.name,) + counters("if", "loop", "cond"), rsf)
            for i in node.body:
                emit_block(nctx, i)
            ctx[CTX_FD].write(f"  push 0\n"
                              f".ret:\n"
                              f"  pop %dx\n" # ret val
                              f"  lds {len(rsf)*2}\n"
                              f"  add %sp %si\n"
                              f"  lds {rsf['_ret']}\n"
                              f"  add %si %bp\n"
                              f"  lodsw\n"
                              f"  push %dx\n"
                              f"  push %ax\n"
                              f"  ret\n")
            #raise ValueError(f"Got {rsf}")
        case ast.Assign():
            if len(node.targets) != 1:
                raise ValueError("Multiple assignments are unsupported")
            match node.value:
                case ast.Constant():
                    data = (
                        node.value.value if isinstance(node.value.value, bytes) else
                        node.value.value.to_bytes(1) if isinstance(node.value.value, int) else
                        (node.value.value.encode() + b"\x00")
                    )
                    ctx[CTX_FD].write(f"{node.targets[0].id}: {'bytes ' + ' '.join(f'${x:02X}' for x in data)}\n")
                case ast.Call(func=ast.Name(id="bytes"), args=[ast.Constant(value=int())]):
                    ctx[CTX_FD].write(f"{node.targets[0].id}: reserve {node.value.args[0].value} bytes\n")
                case ast.Call(func=ast.Name(id="spr"), args=[ast.Constant(value=str())]):
                    ctx[CTX_FD].write(f"{node.targets[0].id}: bytes ")
                    data = node.value.args[0].value
                    for i in data.split("\n"):
                        ctx[CTX_FD].write(
                            f'${int("".join("1" if j in "#" else "0" for j in i[::-1]), 2):02X}'
                        )
                    ctx[CTX_FD].write("\n")
                case ast.Call(func=ast.Name(id="spr16"), args=[ast.Constant(value=str())]):
                    ctx[CTX_FD].write(f"{node.targets[0].id}: bytes ")
                    data = node.value.args[0].value
                    for i in data.split("\n"):
                        ctx[CTX_FD].write(
                            f'${int("".join("1" if j in "#" else "0" for j in i[::-1]), 2):02X}'
                        )
                    ctx[CTX_FD].write("\n")
                case _:
                    raise ValueError(f"Unkown assignment type {type(node.value)}")
        case _:
            print(f"Skipping {type(node)}, FIXME")

parser = argparse.ArgumentParser(prog='govnpy', description='GovnPy compiler')
parser.add_argument('source', type=str, help='GovnPy source code')
parser.add_argument('output', type=str, help='GovnAsm file name')
args = parser.parse_args()
with open(args.source) as f, open(args.output, "w") as fd:
    node = ast.parse(f.read())
    match node:
        case ast.Module():
            glob = {}
            for i in node.body:
                emit_global((fd, glob), i)
            for lab, s in glob.items():
                if isinstance(s, bytes):
                    fd.write(f"{lab}: bytes {' '.join(f'${x:02X}' for x in s)}\n")
                else:
                    enc_s = "".join(
                        i if ord(i) >= 0x20 else (f"^{chr(ord(i) + 0x40)}" if i != "\n" else "$")
                        for i in s.replace("^", "^^").replace("$", "^$").replace('"', '\\"')
                    )
                    fd.write(f"{lab}: bytes \"{enc_s}\"\n")

# GovnPy compiler version 1
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

import sys
import ast
def s16(i) -> str:
    if i >= 0:
        return str(i)
    return str(65536+i)
cnt_val = 0
def cnt() -> int:
    global cnt_val
    cnt_val += 1
    return cnt_val

def trepeshatel(fn, static={}):
    if fn is None:
        return static
    def nfn(n, s):
        r = fn(n,s)
        ki = (fn.__name__, type(n))
        if not ki in static:
            static[ki] = [0, 0]
        static[ki][0] += 1
        if r:
            static[ki][1] += r.count('\n') + 1
        return r
    return nfn

gdata = []
@trepeshatel
def emit_fn(node, stack) -> str:
    match node:
        case ast.Expr():
            return f"{emit_fn(node.value, stack)}\n  pop %ax"
        case ast.Compare():
            if len(node.ops) != 1:
                raise ValueError("Multiple comparison are not supported yet")
            bid = cnt()
            match node.ops[0]:
                case ast.Eq():
                    return f"{emit_fn(node.left, stack)}\n{emit_fn(node.comparators[0], stack)}\n  pop %ax\n  pop %bx\n  cmp %ax %bx\n  lda 1\n  jme .br{bid}\n  lda 0\n.br{bid}:\n  push %ax"
                case ast.NotEq():
                    return f"{emit_fn(node.left, stack)}\n{emit_fn(node.comparators[0], stack)}\n  pop %ax\n  pop %bx\n  cmp %ax %bx\n  lda 1\n  jmne .br{bid}\n  lda 0\n.br{bid}:\n  push %ax"
                case _:
                    raise ValueError(f"Unknown comparison type {type(node.ops[0])}")
        case ast.BoolOp():
            bid = cnt()
            return "\n".join(f"{emit_fn(i, stack)}\n  pop %ax\n  cmp %ax 0\n  {'jme' if type(node.op) == ast.And else 'jmne'} .bfail{bid}" for i in node.values) + f"\n.bfail{bid}:\n  push %ax\n"
        case ast.BinOp():  # Despite the name it is a two-operand operator
            ops = {
                ast.Add: "add %ax %bx",
                ast.Sub: "sub %ax %bx",
                ast.Mult: "mul %ax %bx",
                ast.Mod: "div %ax %bx lda %dx",
                ast.BitXor: "xor %ax %bx +%cx ;|expand",
                ast.BitAnd: "and %ax %bx ;|bin"
            }
            return f"{emit_fn(node.left, stack)}\n{emit_fn(node.right, stack)}\n  pop %bx\n  pop %ax\n  {ops[type(node.op)]}\n  push %ax"
        case ast.Subscript():
            #print(ast.dump(node, indent=4))
            return f"{emit_fn(node.value, stack)}\n{emit_fn(node.slice, stack)}\n  pop %ax\n  pop %si\n  add %si %ax\n  lda *%si\n  push %ax"
            #raise ValueError("todo")
        case ast.Return():
            return  f"{emit_fn(node.value, stack)}\n  jmp .ret"
        case ast.Assign():
            if len(node.targets) != 1:
                raise ValueError("Multiple assignments are unsupported")
            if isinstance(node.targets[0], ast.Name):
                vname = node.targets[0].id
                if vname not in stack:
                    stack[vname] = len(stack) * 2
                return f"{emit_fn(node.value, stack)}\n  lds {s16(-stack[vname])}\n  add %si %bp\n  pop %ax\n  stosw ;|bin"
            elif isinstance(node.targets[0], ast.Subscript):
                return f"{emit_fn(node.value, stack)}\n{emit_fn(node.targets[0].value, stack)}\n{emit_fn(node.targets[0].slice, stack)}\n  pop %gi\n  pop %si\n  pop %ax\n  add %si %gi\n  storb %ax ;|bin"
            else:
                raise ValueError("Weird assignment")
        case ast.Name():
            if node.id not in stack:
                return f"  lda {node.id}\n  push %ax"
            else:
                return f"  lds {s16(-stack[node.id])}\n  add %si %bp\n  lodsw ;|bin\n  push %ax"
        case ast.Pass():
            return ""
        case ast.Call():
            if node.func.id == "breakpoint":
                if len(node.args) != 0:
                    raise ValueError("Bad breakpoint call")
                return f"  trap"
            elif node.func.id == "__asm__":
                if len(node.args) < 1 or not isinstance(node.args[-1].value, str):
                    raise ValueError("Bad __asm__ call")
                return f"{chr(10).join(emit_fn(i, stack) for i in node.args[:-1])}\n   {node.args[-1].value}"
            return f"  push %bp\n{chr(10).join(emit_fn(i, stack) for i in node.args)}\n  call {node.func.id}\n  pop %ax\n  pop %bp\n  push %ax"
        case ast.While():
            loopid = cnt()
            return f""".loop{loopid}:
{emit_fn(node.test, stack)}
  pop %ax
  cmp %ax 0
  jme .lend{loopid}
{chr(10).join(emit_fn(i, stack) for i in node.body)}
  jmp .loop{loopid}
.lend{loopid}:"""
        case ast.If():
            ifid = cnt()
            return f"""{emit_fn(node.test, stack)}
  pop %ax
  cmp %ax 0
  jme .if{ifid}
{chr(10).join(emit_fn(i, stack) for i in node.body)}
  jmp .iend{ifid}
.if{ifid}:
{chr(10).join(emit_fn(i, stack) for i in node.orelse)}
.iend{ifid}:"""
        case ast.Constant():
            if isinstance(node.value, str):
                sid = f"str_{cnt()}"
                gdata.append([sid, node.value])
                return f"  lda {sid}\n  push %ax"
            if not isinstance(node.value, int):
                raise ValueError(f"{type(node.value)} no no")
            if node.value not in range(65536):
                raise ValueError("Bigint no no")
            return f"  push {int(node.value)}"
        case ast.UnaryOp():
            if isinstance(node.operand, ast.Constant):
                if not isinstance(node.operand.value, int):
                    raise ValueError("String no no")
                if node.operand.value not in range(65535):
                    raise ValueError("Bigint no no")
                return f"  push {s16(-node.operand.value)}"
            return f"{emit_fn(node.operand, stack)}\n  pop %bx\n  lda 0\n  sub %ax %bx\n  push %ax"
        case _:
            raise ValueError(f"{type(node)} no support")
def emit(node) -> str:
    match node:
        case ast.Import():
            code = ""
            for i in node.names:
                if not i.name.startswith("govn."):
                    raise ValueError("TODO: Python imports")
                with open("libgovn/" + i.name[5:] + ".s") as m:
                    code += m.read() + "\n"
            return code
        case ast.Module():
            gdata[:] = []
            return (
                "\n".join(map(emit, node.body))
                + "\n" + "\n".join(f"{lab}: {'bytes ' + ' '.join(f'${x:02X}' for x in s.encode() + bytes((0,)))}" for lab, s in gdata)
            )
        case ast.FunctionDef():
            if node.args.posonlyargs or node.args.kwonlyargs or node.args.kw_defaults or node.args.defaults or node.args.vararg or node.args.kwarg:
                raise ValueError(f"Unsupported function definition {ast.dump(node.args)}")
            stack = {}
            for i in node.args.args:
                if i.arg in stack:
                    raise ValueError(f"Duplicate arg {i.arg}")
                stack[i.arg] = len(stack) * 2
            stack["_ret"] = len(stack) * 2
            #raise ValueError("CBO Error")
            body_code = chr(10).join(emit_fn(i, stack) for i in node.body)
            print(";", stack)
            return f"""{node.name}:
  lda {(1 + len(node.args.args)) * 2 - 1}
  ; use xchg %bp %ax
  sub %bp %bp
  add %bp %sp
  add %bp %ax
  lda {(len(stack) - len(node.args.args)) * 2 + 1}
  sub %sp %ax
{body_code}
  push 0
.ret:
  pop %bx ; Ret value
  lds {s16(-stack['_ret'])}
  add %si %bp
  lodsw ;|bin
  sub %sp %sp
  add %sp %bp
  ldd 1
  add %sp %dx
  ; todo: change to xchg %bp %sp
  push %bx
  push %ax
  ret
"""
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
                    return f"{node.targets[0].id}: {'bytes ' + ' '.join(f'${x:02X}' for x in data)}"
                case ast.Call(func=ast.Name(id="bytes"), args=[ast.Constant(value=int())]):
                    return f"{node.targets[0].id}: reserve {node.value.args[0].value} bytes"
                case _:
                    raise ValueError(f"Unkown assignment type {type(node.value)}")
        case _:
            raise ValueError(f"{type(node)} no support")
if len(sys.argv) < 2:
    print("Are you dumb?")
    print("compiler.py GOVNOCOD.py > GOVNOCOD.s")
    sys.exit(1)
with open(sys.argv[1]) as f:
    parsed = ast.parse(f.read())
#print(ast.dump(parsed, indent=4))
# Stdlib
def collapse(txt):
    c = txt.count("\n  push %ax\n  pop %ax\n")
    n = txt.count("\n  pop %bp\n  push %bp\n")
    print(f"; PUSH-POP %AX COUNT: {c}")
    print(f"; PUSH-POP %BP COUNT: {n}")
    return txt.replace("\n  push %ax\n  pop %ax\n", "\n").replace("\n  pop %bp\n  push %bp\n", "\n  cop %bp\n")
print(collapse(
    emit(parsed)
    .replace("lodsw ;|bin\n", "bytes $8A\n")
    .replace("xchg %bp %ax ;|bin\n", "bytes $88 $70\n")
    .replace("stosw ;|bin\n", "bytes $8B\n")
    .replace("xchg %bp %sp ;|bin\n", "bytes $88 $76\n")
    .replace("lfa ;|bin\n", "bytes $FA\n")
    .replace("and %ax %bx ;|bin\n", "bytes $10 $D8 $01\n")
    .replace("xor %ax %bx +%cx ;|expand\n", "bytes $79 $00   $10 $D8 $01   $10 $D9 $21   $10 $01 $02\n")
))
for (f, k), (c, l) in sorted(trepeshatel(None).items(), key=lambda x:x[1][1]/x[1][0], reverse=True):
    print(";", f"{f:8}({k.__name__:10}) AVG {l//c:4} CNT {c:4} LIN {l:4}")

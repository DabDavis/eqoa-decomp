#!/usr/bin/env python3
"""The source mutations `eqperm.py` searches with: small rewrites of ONE C++
function, made on tree-sitter's concrete syntax tree and applied as text
splices, so everything a mutation does not touch keeps its exact spelling.

Each mutation is the kind of edit the phase notes record as a lever:
statement order, a named temporary or reference, a declaration's position
and scope, an if/else's polarity, operand order, `+=` against `= a +`,
`&a[i]` against `a + i`.  Most of them preserve the program's meaning; the
ones that might not are still safe to try, because a candidate only counts
when its BYTES are the target's, and identical bytes cannot mean anything
different.  Whether a hit reads like the original's source is a separate
judgement, made by a person.

A temporary's type is written `decltype(E)`, which tree-sitter parses as a
type; the compile step rewrites it to gcc 2.95's `__typeof__(E)`.

    ./eqperm_mut.py FILE [N]     # print N random mutants of a function
"""

from __future__ import annotations

import random
import re
import sys

import tree_sitter as ts
import tree_sitter_cpp as tscpp

_PARSER = None


def parser() -> ts.Parser:
    # Built lazily: eqperm.py forks its workers, and each builds its own.
    global _PARSER
    if _PARSER is None:
        _PARSER = ts.Parser(ts.Language(tscpp.language()))
    return _PARSER


CONTROL = {"break_statement", "continue_statement", "return_statement",
           "goto_statement"}
LOOPS = {"for_statement", "while_statement", "do_statement",
         "for_range_loop"}
COMPOSITE = {"binary_expression", "conditional_expression",
             "assignment_expression", "comma_expression"}


class Fn:
    """One function's text and its tree."""

    def __init__(self, text: str):
        self.text = text
        self.b = text.encode()
        self.tree = parser().parse(self.b)
        self.root = self.tree.root_node
        self._nodes = None
        self._idents = {}
        self._seqs = None
        self._member = None

    def t(self, n) -> str:
        return self.b[n.start_byte:n.end_byte].decode()

    def nodes(self) -> list:
        if self._nodes is None:
            out, stack = [], [self.root]
            while stack:
                n = stack.pop()
                out.append(n)
                stack.extend(reversed(n.children))
            self._nodes = out
        return self._nodes

    def of(self, *types) -> list:
        return [n for n in self.nodes() if n.type in types]

    def splice(self, edits) -> str:
        """Apply non-overlapping (start_byte, end_byte, text) edits."""
        out, pos = bytearray(), 0
        for s, e, new in sorted(edits, key=lambda x: (x[0], x[1])):
            if s < pos:
                raise ValueError("overlapping edits")
            out += self.b[pos:s]
            out += new.encode()
            pos = e
        out += self.b[pos:]
        return out.decode()

    def idents(self, n) -> set[str]:
        """Every identifier spelled inside n."""
        got = self._idents.get(n.id)
        if got is None:
            got, stack = set(), [n]
            while stack:
                m = stack.pop()
                if m.type == "identifier":
                    got.add(self.t(m))
                stack.extend(m.children)
            self._idents[n.id] = got
        return got

    def seqs(self) -> list:
        """Every statement sequence: (container, [statements]) -- a block's
        statements, or the statements after one `case X:`."""
        if self._seqs is None:
            res = []
            for n in self.nodes():
                if n.type == "compound_statement":
                    kids = [c for c in n.named_children if c.type != "comment"]
                elif n.type == "case_statement":
                    val = n.child_by_field_name("value")
                    kids = [c for c in n.named_children if c.type != "comment"
                            and (val is None or c.id != val.id)]
                else:
                    continue
                res.append((n, kids))
            self._seqs = res
            self._member = {s.id: (c, kids, i) for c, kids in res
                            for i, s in enumerate(kids)}
        return self._seqs

    def member(self, n):
        self.seqs()
        return self._member.get(n.id)

    def indent_of(self, n) -> str:
        line_start = self.b.rfind(b"\n", 0, n.start_byte) + 1
        pre = self.b[line_start:n.start_byte].decode()
        return pre if pre.strip() == "" else ""

    def body(self):
        for n in self.nodes():
            if n.type == "function_definition":
                return n.child_by_field_name("body")
        return None


def op(fn: Fn, n) -> str:
    o = n.child_by_field_name("operator")
    return fn.t(o) if o is not None else ""


def wrap(fn: Fn, n) -> str:
    t = fn.t(n)
    return f"({t})" if n.type in COMPOSITE else t


def declarator_name(fn: Fn, d) -> str | None:
    """The identifier a declarator declares."""
    for _ in range(8):
        if d is None:
            return None
        if d.type == "identifier":
            return fn.t(d)
        nxt = d.child_by_field_name("declarator")
        if nxt is None:
            kids = [k for k in d.named_children if k.type != "comment"]
            nxt = kids[0] if kids else None
        d = nxt
    return None


def declared(fn: Fn, n) -> set[str]:
    if n.type != "declaration":
        return set()
    out = set()
    for d in n.children_by_field_name("declarator"):
        nm = declarator_name(fn, d)
        if nm:
            out.add(nm)
    return out


def order_ok(fn: Fn, stmts: list) -> bool:
    """No statement uses a name a LATER declaration in the sequence declares."""
    for p, s in enumerate(stmts):
        names = declared(fn, s)
        if names and any(names & fn.idents(q) for q in stmts[:p]):
            return False
    return True


def replace_seq(fn: Fn, old: list, new: list) -> str:
    """Rewrite a sequence's statements in a new order, keeping the separators."""
    seps = [fn.b[old[k].end_byte:old[k + 1].start_byte].decode()
            for k in range(len(old) - 1)]
    text = fn.t(new[0]) + "".join(sep + fn.t(s) for sep, s in zip(seps, new[1:]))
    return fn.splice([(old[0].start_byte, old[-1].end_byte, text)])


def fresh(fn: Fn, stem: str) -> str:
    used = set(re.findall(r"[A-Za-z_]\w*", fn.text))
    k = 0
    while f"{stem}{k}" in used:
        k += 1
    return f"{stem}{k}"


# --- the mutations -------------------------------------------------------------

def m_swap(fn: Fn, rng: random.Random):
    """Swap two adjacent statements."""
    cands = [(c, s) for c, s in fn.seqs() if len(s) >= 2]
    if not cands:
        return None
    _, s = rng.choice(cands)
    i = rng.randrange(len(s) - 1)
    a, b = s[i], s[i + 1]
    if a.type in CONTROL or b.type in CONTROL:
        return None
    if not order_ok(fn, s[:i] + [b, a] + s[i + 2:]):
        return None
    return fn.splice([(a.start_byte, a.end_byte, fn.t(b)),
                      (b.start_byte, b.end_byte, fn.t(a))])


def m_move(fn: Fn, rng: random.Random):
    """Move one statement elsewhere in its sequence."""
    cands = [(c, s) for c, s in fn.seqs() if len(s) >= 3]
    if not cands:
        return None
    _, s = rng.choice(cands)
    i, j = rng.randrange(len(s)), rng.randrange(len(s))
    if i == j or s[i].type in CONTROL:
        return None
    new = s[:i] + s[i + 1:]
    new.insert(j, s[i])
    if not order_ok(fn, new):
        return None
    return replace_seq(fn, s, new)


TEMP_OK = {"binary_expression", "call_expression", "field_expression",
           "subscript_expression", "pointer_expression", "cast_expression",
           "unary_expression", "conditional_expression", "number_literal",
           "identifier", "parenthesized_expression"}
LVALUE = {"identifier", "field_expression", "subscript_expression",
          "pointer_expression", "parenthesized_expression"}


def temp_site(fn: Fn, e):
    """(statement, needs_reference) when e can be evaluated into a temporary
    declared just before its statement, else None."""
    p = e.parent
    if p is None:
        return None
    if e.type == "identifier" and p.type not in (
            "binary_expression", "argument_list", "init_declarator",
            "assignment_expression", "subscript_argument_list",
            "subscript_expression", "field_expression", "return_statement",
            "condition_clause", "conditional_expression", "pointer_expression",
            "unary_expression", "parenthesized_expression", "cast_expression"):
        return None
    if p.type == "init_declarator" and p.child_by_field_name("declarator") == e:
        return None
    if p.type == "call_expression" and p.child_by_field_name("function") == e:
        return None
    if e.type == "pointer_expression" and op(fn, e) == "&" and \
            p.type == "pointer_expression":
        return None
    need_ref = False
    if p.type == "assignment_expression" and p.child_by_field_name("left") == e:
        need_ref = True
    if p.type == "update_expression":
        need_ref = True
    if p.type == "pointer_expression" and op(fn, p) == "&":
        need_ref = True
    if p.type == "field_expression" and p.child_by_field_name("argument") == e:
        q = p.parent
        if q is not None and q.type == "call_expression" and \
                q.child_by_field_name("function") == p:
            need_ref = True             # the object a method is called on
        if e.type not in LVALUE and need_ref:
            return None
    if need_ref and e.type not in LVALUE:
        return None
    # walk up to the statement, refusing anything a temporary would hoist out of
    child, n = e, p
    while n is not None:
        if fn.member(n) is not None:
            if n.type in LOOPS:
                return None
            if n.type == "if_statement" and \
                    n.child_by_field_name("condition") != child:
                return None
            if n.type == "switch_statement" and \
                    n.child_by_field_name("condition") != child:
                return None
            if n.type in ("compound_statement", "case_statement"):
                return None
            return n, need_ref
        if n.type in LOOPS or n.type in ("if_statement", "switch_statement",
                                         "lambda_expression"):
            if n.type == "if_statement" and \
                    n.child_by_field_name("condition") == child:
                pass
            else:
                return None
        if n.type == "binary_expression" and op(fn, n) in ("&&", "||") and \
                n.child_by_field_name("right") == child:
            return None
        if n.type == "conditional_expression" and \
                n.child_by_field_name("condition") != child:
            return None
        if n.type == "comma_expression":
            return None
        child, n = n, n.parent
    return None


def m_temp(fn: Fn, rng: random.Random):
    """Evaluate a subexpression into a named temporary (or reference)."""
    cands = [n for n in fn.nodes() if n.type in TEMP_OK]
    rng.shuffle(cands)
    for e in cands[:12]:
        # literals and lone identifiers are levers too, but rarer ones
        if e.type in ("number_literal", "identifier") and rng.random() < 0.7:
            continue
        site = temp_site(fn, e)
        if site is None:
            continue
        stmt, need_ref = site
        ref = need_ref or (e.type in LVALUE and rng.random() < 0.35)
        name = fresh(fn, "pt")
        et = fn.t(e)
        if e.type == "parenthesized_expression":
            et = et[1:-1].strip()
        if ref:
            decl = f"decltype({et}) &{name} = {et};"
        elif rng.random() < 0.15:
            decl = f"int {name} = {et};"
        else:
            decl = f"decltype({et}) {name} = {et};"
        edits = [(stmt.start_byte, stmt.start_byte,
                  decl + "\n" + fn.indent_of(stmt))]
        targets = [e]
        if rng.random() < 0.3:
            same = [n for n in fn.nodes() if n.type == e.type and fn.t(n) == fn.t(e)
                    and stmt.start_byte <= n.start_byte and n.end_byte <= stmt.end_byte
                    and n.id != e.id]
            for n in same:
                if temp_site(fn, n) is not None and \
                        all(n.end_byte <= x.start_byte or n.start_byte >= x.end_byte
                            for x in targets):
                    targets.append(n)
        for n in targets:
            edits.append((n.start_byte, n.end_byte, name))
        return fn.splice(edits)
    return None


def m_inline(fn: Fn, rng: random.Random):
    """Fold a once-used local back into its single use."""
    cands = []
    for c, s in fn.seqs():
        for i, d in enumerate(s):
            if d.type != "declaration":
                continue
            ds = d.children_by_field_name("declarator")
            if len(ds) != 1 or ds[0].type != "init_declarator":
                continue
            val = ds[0].child_by_field_name("value")
            inner = ds[0].child_by_field_name("declarator")
            if val is None or val.type in ("argument_list", "initializer_list") \
                    or inner is None or inner.type != "identifier":
                continue
            cands.append((s, i, d, fn.t(inner), val))
    rng.shuffle(cands)
    for s, i, d, name, val in cands[:6]:
        uses = []
        for st in s[i + 1:]:
            stack = [st]
            while stack:
                m = stack.pop()
                if m.type == "identifier" and fn.t(m) == name:
                    uses.append(m)
                stack.extend(m.children)
        if len(uses) != 1:
            continue
        u = uses[0]
        p = u.parent
        if p.type in ("update_expression",) or \
                (p.type == "assignment_expression" and p.child_by_field_name("left") == u) or \
                (p.type == "pointer_expression" and op(fn, p) == "&"):
            continue
        # nothing between the declaration and the use may assign what val reads
        vids = fn.idents(val)
        bad = False
        for st in s[i + 1:]:
            for m in [st] + [x for x in fn.nodes()
                             if st.start_byte <= x.start_byte < st.end_byte]:
                if m.start_byte >= u.start_byte:
                    break
                if m.type == "assignment_expression":
                    lt = m.child_by_field_name("left")
                    if lt is not None and fn.idents(lt) & vids:
                        bad = True
                elif m.type == "update_expression":
                    if fn.idents(m) & vids:
                        bad = True
            if bad or st.end_byte >= u.start_byte:
                break
        if bad:
            continue
        nxt = s[i + 1] if i + 1 < len(s) else None
        end = nxt.start_byte if nxt is not None else d.end_byte
        vt = fn.t(val)
        rep = vt if val.type in ("identifier", "number_literal", "call_expression",
                                 "field_expression", "subscript_expression",
                                 "parenthesized_expression") else f"({vt})"
        return fn.splice([(d.start_byte, end, ""), (u.start_byte, u.end_byte, rep)])
    return None


COMM = {"+", "*", "&", "|", "^", "==", "!="}
FLIP = {"<": ">", ">": "<", "<=": ">=", ">=": "<="}


def m_commute(fn: Fn, rng: random.Random):
    """Swap a commutative operator's operands, or flip a comparison."""
    ns = [n for n in fn.of("binary_expression") if op(fn, n) in COMM or op(fn, n) in FLIP]
    if not ns:
        return None
    n = rng.choice(ns)
    o = op(fn, n)
    l, r = n.child_by_field_name("left"), n.child_by_field_name("right")
    return fn.splice([(n.start_byte, n.end_byte,
                       f"{wrap(fn, r)} {FLIP.get(o, o)} {wrap(fn, l)}")])


INVERT = {"<": ">=", ">=": "<", ">": "<=", "<=": ">", "==": "!=", "!=": "=="}


def negate(fn: Fn, v) -> str:
    if v.type == "unary_expression" and op(fn, v) == "!":
        a = v.child_by_field_name("argument")
        t = fn.t(a)
        if a.type == "parenthesized_expression":
            t = t[1:-1].strip()
        return t
    if v.type == "binary_expression" and op(fn, v) in INVERT:
        l, r = v.child_by_field_name("left"), v.child_by_field_name("right")
        return f"{fn.t(l)} {INVERT[op(fn, v)]} {fn.t(r)}"
    return f"!({fn.t(v)})"


def cond_value(fn: Fn, n):
    c = n.child_by_field_name("condition")
    if c is None:
        return None
    v = c.child_by_field_name("value")
    if v is None:
        kids = [k for k in c.named_children if k.type != "comment"]
        v = kids[0] if len(kids) == 1 else None
    if v is None or v.type == "declaration":
        return None
    return v


def m_negate_if(fn: Fn, rng: random.Random):
    """`if (c) A else B` -> `if (!c) B else A`."""
    ns = [n for n in fn.of("if_statement") if n.child_by_field_name("alternative")]
    if not ns:
        return None
    n = rng.choice(ns)
    v = cond_value(fn, n)
    if v is None:
        return None
    cons = n.child_by_field_name("consequence")
    alt_clause = n.child_by_field_name("alternative")
    alts = [k for k in alt_clause.named_children if k.type != "comment"]
    if len(alts) != 1:
        return None
    alt = alts[0]
    ind = fn.indent_of(n)
    newcons = fn.t(alt)
    if alt.type == "if_statement":
        newcons = "{\n" + ind + "    " + newcons + "\n" + ind + "}"
    elif alt.type != "compound_statement":
        newcons = "\n" + ind + "    " + newcons
    else:
        newcons = "\n" + ind + newcons
    newalt = fn.t(cons)
    if cons.type != "compound_statement":
        newalt = "\n" + ind + "    " + newalt
    else:
        newalt = "\n" + ind + newalt
    if not newcons.startswith("\n"):
        newcons = "\n" + ind + newcons
    text = f"if ({negate(fn, v)}){newcons}\n{ind}else{newalt}"
    return fn.splice([(n.start_byte, n.end_byte, text)])


ASSIGN = {"+=": "+", "-=": "-", "*=": "*", "/=": "/", "%=": "%", "&=": "&",
          "|=": "|", "^=": "^", "<<=": "<<", ">>=": ">>"}
UNASSIGN = {v: k for k, v in ASSIGN.items()}


def m_compound(fn: Fn, rng: random.Random):
    """`a += b` <-> `a = a + b`, and `x += 1` / `x = x + 1` -> `x++`."""
    ns = fn.of("assignment_expression")
    rng.shuffle(ns)
    for n in ns[:6]:
        o = op(fn, n)
        L, R = n.child_by_field_name("left"), n.child_by_field_name("right")
        if L is None or R is None:
            continue
        lt = fn.t(L)
        stmt_level = n.parent.type in ("expression_statement", "for_statement")
        if o in ("+=", "-=") and fn.t(R) == "1" and stmt_level and rng.random() < 0.5:
            return fn.splice([(n.start_byte, n.end_byte, f"{lt}{o[0]}{o[0]}")])
        if o in ASSIGN:
            return fn.splice([(n.start_byte, n.end_byte,
                               f"{lt} = {lt} {ASSIGN[o]} {wrap(fn, R)}")])
        if o == "=" and R.type == "binary_expression" and op(fn, R) in UNASSIGN:
            rl, rr = R.child_by_field_name("left"), R.child_by_field_name("right")
            if fn.t(rl) == lt:
                if op(fn, R) in ("+", "-") and fn.t(rr) == "1" and stmt_level \
                        and rng.random() < 0.5:
                    return fn.splice([(n.start_byte, n.end_byte,
                                       f"{lt}{op(fn, R) * 2}")])
                return fn.splice([(n.start_byte, n.end_byte,
                                   f"{lt} {UNASSIGN[op(fn, R)]} {fn.t(rr)}")])
    return None


def m_incdec(fn: Fn, rng: random.Random):
    """`i++` as a statement -> `++i` / `i += 1` / `i = i + 1`."""
    ns = [n for n in fn.of("update_expression")
          if n.parent is not None and n.parent.type in
          ("expression_statement", "for_statement", "comma_expression")]
    if not ns:
        return None
    n = rng.choice(ns)
    a = n.child_by_field_name("argument")
    if a is None:
        return None
    at = fn.t(a)
    t = fn.t(n)
    o = "++" if "++" in t else "--"
    forms = [f"{at}{o}", f"{o}{at}", f"{at} {o[0]}= 1", f"{at} = {at} {o[0]} 1"]
    forms = [f for f in forms if f.replace(" ", "") != t.replace(" ", "")]
    return fn.splice([(n.start_byte, n.end_byte, rng.choice(forms))])


def _simple_decl(fn: Fn, d):
    """(head, declarator node, init_declarator) for `T x = e;` or `T x;`."""
    if d.type != "declaration" or fn.member(d) is None:
        return None
    ds = d.children_by_field_name("declarator")
    if len(ds) != 1:
        return None
    if any(c.type == "storage_class_specifier" for c in d.children):
        return None
    head = fn.b[d.start_byte:ds[0].start_byte].decode()
    return head, ds[0]


def m_split_decl(fn: Fn, rng: random.Random):
    """`T x = e;` -> `T x; x = e;`."""
    cands = []
    for d in fn.of("declaration"):
        sd = _simple_decl(fn, d)
        if sd is None or sd[1].type != "init_declarator":
            continue
        head, idecl = sd
        inner = idecl.child_by_field_name("declarator")
        val = idecl.child_by_field_name("value")
        if inner is None or val is None or val.type in ("argument_list", "initializer_list"):
            continue
        if inner.type not in ("identifier", "pointer_declarator"):
            continue
        if inner.type == "identifier" and "const" in head.split():
            continue
        if "decltype" in head and "&" in fn.t(inner):
            continue
        cands.append((d, head, inner, val))
    if not cands:
        return None
    d, head, inner, val = rng.choice(cands)
    name = declarator_name(fn, inner)
    text = f"{head}{fn.t(inner)};\n{fn.indent_of(d)}{name} = {fn.t(val)};"
    return fn.splice([(d.start_byte, d.end_byte, text)])


def m_join_decl(fn: Fn, rng: random.Random):
    """`T x; ... x = e;` (the next statement) -> `T x = e;`."""
    cands = []
    for c, s in fn.seqs():
        for i in range(len(s) - 1):
            sd = _simple_decl(fn, s[i])
            if sd is None or sd[1].type not in ("identifier", "pointer_declarator"):
                continue
            nxt = s[i + 1]
            if nxt.type != "expression_statement":
                continue
            a = nxt.named_children[0] if nxt.named_children else None
            if a is None or a.type != "assignment_expression" or op(fn, a) != "=":
                continue
            if fn.t(a.child_by_field_name("left")) != declarator_name(fn, sd[1]):
                continue
            cands.append((s[i], sd, nxt, a))
    if not cands:
        return None
    d, (head, dn), nxt, a = rng.choice(cands)
    text = f"{head}{fn.t(dn)} = {fn.t(a.child_by_field_name('right'))};"
    return fn.splice([(d.start_byte, nxt.end_byte, text)])


def m_add_block(fn: Fn, rng: random.Random):
    """Put a run of statements in a block of their own."""
    cands = [(c, s) for c, s in fn.seqs() if s]
    if not cands:
        return None
    _, s = rng.choice(cands)
    i = rng.randrange(len(s))
    k = 1 + rng.randrange(min(3, len(s) - i))
    run = s[i:i + k]
    if k == 1 and run[0].type == "compound_statement":
        return None
    names = set().union(*(declared(fn, x) for x in run))
    if names and any(names & fn.idents(x) for x in s[i + k:]):
        return None
    ind = fn.indent_of(run[0])
    inner = fn.b[run[0].start_byte:run[-1].end_byte].decode()
    return fn.splice([(run[0].start_byte, run[-1].end_byte,
                       "{\n" + ind + "    " + inner + "\n" + ind + "}")])


def m_remove_block(fn: Fn, rng: random.Random):
    """Dissolve a bare `{ ... }` into its enclosing sequence."""
    cands = []
    for n in fn.of("compound_statement"):
        m = fn.member(n)
        if m is None:
            continue
        _, s, _ = m
        inner = [k for k in n.named_children if k.type != "comment"]
        if not inner:
            continue
        names = set().union(*(declared(fn, x) for x in inner))
        others = [x for x in s if x.id != n.id]
        if names and any(names & (fn.idents(x) | declared(fn, x)) for x in others):
            continue
        cands.append((n, inner))
    if not cands:
        return None
    n, inner = rng.choice(cands)
    return fn.splice([(n.start_byte, n.end_byte,
                       fn.b[inner[0].start_byte:inner[-1].end_byte].decode())])


def m_pad(fn: Fn, rng: random.Random):
    """Declare an unused local: it costs no code but takes a pseudo number."""
    cands = [(c, s) for c, s in fn.seqs() if s]
    if not cands:
        return None
    _, s = rng.choice(cands)
    st = s[rng.randrange(len(s))]
    return fn.splice([(st.start_byte, st.start_byte,
                       f"int {fresh(fn, 'pad')};\n{fn.indent_of(st)}")])


def m_subscript(fn: Fn, rng: random.Random):
    """`&a[i]` <-> `a + i`-style pointer arithmetic, `a[i]` <-> `*(a + i)`."""
    cands = []
    for n in fn.of("pointer_expression"):
        a = n.child_by_field_name("argument")
        if a is None:
            continue
        if op(fn, n) == "&" and a.type == "subscript_expression":
            cands.append(("addr", n, a))
        elif op(fn, n) == "*" and a.type == "parenthesized_expression":
            inner = [k for k in a.named_children if k.type != "comment"]
            if len(inner) == 1 and inner[0].type == "binary_expression" and \
                    op(fn, inner[0]) == "+":
                cands.append(("deref", n, inner[0]))
    for n in fn.of("subscript_expression"):
        p = n.parent
        if p is not None and p.type == "pointer_expression" and op(fn, p) == "&":
            continue
        cands.append(("sub", n, n))
    if not cands:
        return None
    kind, n, a = rng.choice(cands)
    if kind == "addr" or kind == "sub":
        base = a.child_by_field_name("argument")
        idx = a.child_by_field_name("indices")
        ik = [k for k in idx.named_children if k.type != "comment"] if idx else []
        if base is None or len(ik) != 1:
            return None
        sum_ = f"{wrap(fn, base)} + {wrap(fn, ik[0])}"
        return fn.splice([(n.start_byte, n.end_byte,
                           f"({sum_})" if kind == "addr" else f"*({sum_})")])
    l, r = a.child_by_field_name("left"), a.child_by_field_name("right")
    return fn.splice([(n.start_byte, n.end_byte, f"{wrap(fn, l)}[{fn.t(r)}]")])


def m_cond(fn: Fn, rng: random.Random):
    """`x != 0` <-> `x`, `x == 0` <-> `!x`, in a truth context."""
    cands = []
    for n in fn.nodes():
        p = n.parent
        if p is None or not n.is_named:
            continue
        truth = (p.type == "condition_clause" or
                 (p.type == "binary_expression" and op(fn, p) in ("&&", "||")) or
                 (p.type == "unary_expression" and op(fn, p) == "!" and False))
        if not truth:
            continue
        if n.type == "binary_expression" and op(fn, n) in ("!=", "==") and \
                fn.t(n.child_by_field_name("right")) == "0":
            cands.append(n)
        elif n.type in ("identifier", "field_expression", "call_expression",
                        "subscript_expression", "unary_expression",
                        "parenthesized_expression"):
            cands.append(n)
    if not cands:
        return None
    n = rng.choice(cands)
    if n.type == "binary_expression":
        l = n.child_by_field_name("left")
        t = wrap(fn, l) if op(fn, n) == "!=" else f"!{wrap(fn, l)}"
        if op(fn, n) == "!=":
            t = fn.t(l)
    elif n.type == "unary_expression" and op(fn, n) == "!":
        a = n.child_by_field_name("argument")
        t = f"{fn.t(a)} == 0"
    elif n.type == "unary_expression":
        return None
    else:
        t = f"{fn.t(n)} != 0"
    return fn.splice([(n.start_byte, n.end_byte, t)])


def m_assoc(fn: Fn, rng: random.Random):
    """`a + b + c` <-> `a + (b + c)`."""
    cands = []
    for n in fn.of("binary_expression"):
        o = op(fn, n)
        if o not in ("+", "*", "&", "|", "^"):
            continue
        l, r = n.child_by_field_name("left"), n.child_by_field_name("right")
        if l.type == "binary_expression" and op(fn, l) == o:
            cands.append(("right", n, l, r))
        if r.type == "parenthesized_expression":
            inner = [k for k in r.named_children if k.type != "comment"]
            if len(inner) == 1 and inner[0].type == "binary_expression" and \
                    op(fn, inner[0]) == o:
                cands.append(("left", n, l, inner[0]))
    if not cands:
        return None
    kind, n, l, r = rng.choice(cands)
    o = op(fn, n)
    if kind == "right":
        a, b = l.child_by_field_name("left"), l.child_by_field_name("right")
        t = f"{fn.t(a)} {o} ({fn.t(b)} {o} {wrap(fn, r)})"
    else:
        b, c = r.child_by_field_name("left"), r.child_by_field_name("right")
        t = f"{fn.t(l)} {o} {fn.t(b)} {o} {wrap(fn, c)}"
    return fn.splice([(n.start_byte, n.end_byte, t)])


def _single_assign(fn: Fn, st):
    """The assignment when st is `x = e;` or `{ x = e; }`."""
    if st.type == "compound_statement":
        kids = [k for k in st.named_children if k.type != "comment"]
        if len(kids) != 1:
            return None
        st = kids[0]
    if st.type != "expression_statement" or not st.named_children:
        return None
    a = st.named_children[0]
    if a.type == "assignment_expression" and op(fn, a) == "=":
        return a
    return None


def m_ternary(fn: Fn, rng: random.Random):
    """`x = c ? a : b;` <-> `if (c) x = a; else x = b;`."""
    cands = []
    for st in fn.of("expression_statement"):
        if fn.member(st) is None or not st.named_children:
            continue
        a = st.named_children[0]
        if a.type == "assignment_expression" and op(fn, a) == "=" and \
                a.child_by_field_name("right").type == "conditional_expression":
            cands.append(("split", st, a))
    for n in fn.of("if_statement"):
        if fn.member(n) is None or not n.child_by_field_name("alternative"):
            continue
        v = cond_value(fn, n)
        alts = [k for k in n.child_by_field_name("alternative").named_children
                if k.type != "comment"]
        if v is None or len(alts) != 1:
            continue
        a1 = _single_assign(fn, n.child_by_field_name("consequence"))
        a2 = _single_assign(fn, alts[0])
        if a1 and a2 and fn.t(a1.child_by_field_name("left")) == \
                fn.t(a2.child_by_field_name("left")):
            cands.append(("join", n, (v, a1, a2)))
    if not cands:
        return None
    kind, st, x = rng.choice(cands)
    ind = fn.indent_of(st)
    if kind == "split":
        L = fn.t(x.child_by_field_name("left"))
        ce = x.child_by_field_name("right")
        c = ce.child_by_field_name("condition")
        a = ce.child_by_field_name("consequence")
        b = ce.child_by_field_name("alternative")
        if c is None or a is None or b is None:
            return None
        t = (f"if ({fn.t(c)})\n{ind}    {L} = {fn.t(a)};\n{ind}else\n"
             f"{ind}    {L} = {fn.t(b)};")
    else:
        v, a1, a2 = x
        L = fn.t(a1.child_by_field_name("left"))
        t = (f"{L} = {wrap(fn, v)} ? {wrap(fn, a1.child_by_field_name('right'))}"
             f" : {wrap(fn, a2.child_by_field_name('right'))};")
    return fn.splice([(st.start_byte, st.end_byte, t)])


def _arms(fn: Fn, n):
    """(consequence, alternative) statements of an if/else, else None."""
    alt_clause = n.child_by_field_name("alternative")
    if alt_clause is None:
        return None
    alts = [k for k in alt_clause.named_children if k.type != "comment"]
    if len(alts) != 1:
        return None
    return n.child_by_field_name("consequence"), alts[0]


def _stmts(fn: Fn, s) -> list:
    if s.type == "compound_statement":
        return [k for k in s.named_children if k.type != "comment"]
    return [s]


def _as_block(fn: Fn, stmts: list, ind: str) -> str:
    inner = ("\n" + ind + "    ").join(fn.t(x) for x in stmts)
    return "{\n" + ind + "    " + inner + "\n" + ind + "}" if stmts else "{\n" + ind + "}"


def m_sink_tail(fn: Fn, rng: random.Random):
    """`if (c) {A} else {B} T;` -> `if (c) {A T} else {B T}`: the copies are
    counted twice by the allocator, and jump2 cross-jumps them back into one
    (phase 61: "cross-jumped code is counted twice")."""
    cands = []
    for n in fn.of("if_statement"):
        m = fn.member(n)
        arms = _arms(fn, n)
        if m is None or arms is None:
            continue
        _, s, i = m
        if i + 1 >= len(s) or s[i + 1].type == "declaration":
            continue
        cands.append((n, arms, s[i + 1]))
    if not cands:
        return None
    n, (a, b), t = rng.choice(cands)
    ind = fn.indent_of(n)
    v = cond_value(fn, n)
    if v is None:
        return None
    text = (f"if ({fn.t(v)})\n{ind}{_as_block(fn, _stmts(fn, a) + [t], ind)}\n{ind}else\n"
            f"{ind}{_as_block(fn, _stmts(fn, b) + [t], ind)}")
    return fn.splice([(n.start_byte, t.end_byte, text)])


def m_hoist_tail(fn: Fn, rng: random.Random):
    """`if (c) {A T} else {B T}` -> `if (c) {A} else {B} T;`."""
    cands = []
    for n in fn.of("if_statement"):
        arms = _arms(fn, n)
        if fn.member(n) is None or arms is None:
            continue
        sa, sb = _stmts(fn, arms[0]), _stmts(fn, arms[1])
        if sa and sb and fn.t(sa[-1]) == fn.t(sb[-1]) and sa[-1].type != "declaration":
            cands.append((n, sa, sb))
    if not cands:
        return None
    n, sa, sb = rng.choice(cands)
    ind = fn.indent_of(n)
    v = cond_value(fn, n)
    if v is None:
        return None
    text = (f"if ({fn.t(v)})\n{ind}{_as_block(fn, sa[:-1], ind)}\n{ind}else\n"
            f"{ind}{_as_block(fn, sb[:-1], ind)}\n{ind}{fn.t(sa[-1])}")
    return fn.splice([(n.start_byte, n.end_byte, text)])


def m_head(fn: Fn, rng: random.Random):
    """Move the statement before an if/else into both arms' heads, or a
    head both arms share out in front of the if."""
    cands = []
    for n in fn.of("if_statement"):
        m = fn.member(n)
        arms = _arms(fn, n)
        if m is None or arms is None:
            continue
        _, s, i = m
        v = cond_value(fn, n)
        sa, sb = _stmts(fn, arms[0]), _stmts(fn, arms[1])
        if sa and sb and fn.t(sa[0]) == fn.t(sb[0]) and sa[0].type != "declaration":
            cands.append(("out", n, v, sa, sb, None))
        if i > 0 and s[i - 1].type not in {"declaration"} | CONTROL and v is not None:
            # only when the moved statement cannot change the condition
            if not (fn.idents(v) & _assigned(fn, s[i - 1])):
                cands.append(("in", n, v, sa, sb, s[i - 1]))
    if not cands:
        return None
    kind, n, v, sa, sb, p = rng.choice(cands)
    if v is None:
        return None
    ind = fn.indent_of(n)
    if kind == "out":
        text = (f"{fn.t(sa[0])}\n{ind}if ({fn.t(v)})\n{ind}{_as_block(fn, sa[1:], ind)}\n"
                f"{ind}else\n{ind}{_as_block(fn, sb[1:], ind)}")
        return fn.splice([(n.start_byte, n.end_byte, text)])
    text = (f"if ({fn.t(v)})\n{ind}{_as_block(fn, [p] + sa, ind)}\n{ind}else\n"
            f"{ind}{_as_block(fn, [p] + sb, ind)}")
    return fn.splice([(p.start_byte, n.end_byte, text)])


def _assigned(fn: Fn, st) -> set[str]:
    out, stack = set(), [st]
    while stack:
        m = stack.pop()
        if m.type == "assignment_expression":
            lt = m.child_by_field_name("left")
            if lt is not None:
                out |= fn.idents(lt)
        elif m.type == "update_expression":
            out |= fn.idents(m)
        elif m.type == "call_expression":
            out.add("<call>")
        stack.extend(m.children)
    return out


MUTATIONS = [
    (m_swap, 6), (m_move, 4), (m_temp, 8), (m_inline, 3), (m_commute, 4),
    (m_negate_if, 2), (m_compound, 2), (m_incdec, 2), (m_split_decl, 2),
    (m_join_decl, 2), (m_add_block, 2), (m_remove_block, 2), (m_pad, 1),
    (m_subscript, 2), (m_cond, 2), (m_assoc, 1), (m_ternary, 1),
    (m_sink_tail, 2), (m_hoist_tail, 2), (m_head, 2),
]
_TOTAL = sum(w for _, w in MUTATIONS)


def pick(rng: random.Random):
    x = rng.random() * _TOTAL
    for m, w in MUTATIONS:
        x -= w
        if x < 0:
            return m
    return MUTATIONS[-1][0]


def _nerr(node) -> int:
    """ERROR and MISSING nodes under `node`; 0 without a walk when the tree is clean."""
    if not node.has_error:
        return 0
    return (node.type == "ERROR" or node.is_missing) + sum(_nerr(c) for c in node.children)


def mutate(text: str, rng: random.Random, tries: int = 25):
    """(new text, mutation name), or (None, None) when nothing applied.  A
    result may not parse WORSE than the text did: tree-sitter cannot read
    `float in[4] __attribute__((aligned(16)));`, and requiring a clean tree
    rejected every mutation of a function that holds one."""
    fn = Fn(text)
    base = _nerr(fn.root)
    for _ in range(tries):
        m = pick(rng)
        try:
            new = m(fn, rng)
        except (ValueError, IndexError, AttributeError, TypeError):
            new = None
        if new and new != text and _nerr(Fn(new).root) <= base:
            return new, m.__name__[2:]
    return None, None


def strip_comments(text: str) -> str:
    fn = Fn(text)
    edits = [(n.start_byte, n.end_byte, "") for n in fn.of("comment")]
    out = fn.splice(edits) if edits else text
    lines = [l.rstrip() for l in out.splitlines()]
    res = []
    for l in lines:
        if l == "" and res and res[-1] == "":
            continue
        res.append(l)
    # a line that held only a comment is gone entirely
    return "\n".join(l for i, l in enumerate(res)
                     if l != "" or (0 < i < len(res) - 1)) + "\n"


if __name__ == "__main__":
    src = open(sys.argv[1]).read()
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 5
    rng = random.Random(1)
    import difflib
    for _ in range(n):
        new, name = mutate(src, rng)
        print(f"===== {name}")
        if new:
            print("".join(difflib.unified_diff(src.splitlines(True), new.splitlines(True), n=1)))

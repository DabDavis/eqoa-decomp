#!/usr/bin/env python3
"""A VU1 simulator, modelled on PCSX2's microVU so that it can be checked
against captured launches (vu1dump records) bit for bit.

The data path is sequential: microVU stalls on every VF/VI read-after-write,
so a VF result is always the value of the latest write. What is NOT
interlocked is modelled by cycle, the way microVU's analysis pass does
(pcsx2/x86/microVU_Analyze.inl, _Compile.inl, _Flags.inl):

  * Q: DIV/SQRT 7, RSQRT 13.  The result goes into the OTHER of two
    instances, and becomes current when its counter runs out.
  * P: EFU latencies (ERLENG 24, ELENG 18, ...), the same two-instance
    scheme; a new EFU op waits for p-1 and flips the old one in.
  * MAC/clip flags: an FMAC issued at cycle c makes its flags visible at
    c+4.  A reader at cycle c sees the latest instance visible by then.
  * A branch whose VI register was written by the instruction right before
    it reads an OLDER value (analyzeBranchVI).

Floats follow PCSX2's VU1 settings here (Roundmode 3 = chop, DAZ/FTZ,
normal overflow clamping): every op is IEEE single in round-toward-zero.
"""
import math
import struct
from fractions import Fraction

import vudis as V

FMAX = 0x7F7FFFFF
MEMQ = 1024                      # VU1 data memory, in quadwords

# 'ps2': the console, which has no NaN or infinity -- an exponent-255
#        operand is a huge ordinary number, read here as +-FLT_MAX, so
#        0 * it is 0.
# 'pcsx2': the oracle, which computes on x86 SSE: NaN propagates (SSE
#        returns the first NaN operand, quieted; inf-inf and 0*inf give
#        0xFFC00000).  Only use it to compare against PCSX2 captures.
MODE = 'ps2'
QNAN = 0xFFC00000


def _isnan(u):
    return (u >> 23) & 0xFF == 0xFF and u & 0x7FFFFF


def _nan2(a, b):
    """SSE's NaN rule for a binary op, or None when neither is a NaN."""
    if MODE != 'pcsx2':
        return None
    if _isnan(a):
        return a | 0x400000
    if _isnan(b):
        return b | 0x400000
    return None


# ------------------------------------------------------------- floats -------

def clamp1(u):
    """microVU's mVUclamp1 in 'normal' overflow mode: minps(x, fmax) then
    maxps(x, -fmax) -- a NaN comes out +fmax, an infinity +-fmax."""
    if MODE != 'pcsx2' or (u >> 23) & 0xFF != 0xFF:
        return u
    if u & 0x7FFFFF:
        return FMAX
    return (u & 0x80000000) | FMAX


def _clamp_operands(op, kind, is_acc, dest_mask):
    """Which operands microVU clamps (microVU_Upper.inl's clampType column),
    as (clamp Fs, clamp Ft).  Only the multi-field ('PS') forms of SUB and
    MUL clamp Ft; the I register is never clamped."""
    ps = dest_mask == 0xF
    bc = kind in ('bc', 'accbc')
    if op == 'sub' and not is_acc:
        return ps, ps
    if op == 'mul' and not is_acc:
        return True, ps
    if op == 'mul' and is_acc and bc:
        return True, False
    if op == 'madd' and bc:
        return True, False
    return False, False


def u2f(u):
    """A register's bits as a Python float, with DAZ and the exponent-255
    clamp (inf/NaN read as +-FLT_MAX)."""
    e = (u >> 23) & 0xFF
    if e == 0:
        return -0.0 if u >> 31 else 0.0
    if e == 0xFF:
        if MODE == 'pcsx2':
            if u & 0x7FFFFF:
                return float('nan')
            return float('-inf') if u >> 31 else float('inf')
        u = (u & 0x80000000) | FMAX
    return struct.unpack('<f', struct.pack('<I', u))[0]


def _chop_double(x):
    """Round a double toward zero to single, with FTZ and saturation."""
    if x == 0.0:
        return 0x80000000 if math.copysign(1.0, x) < 0 else 0
    if x != x:
        return QNAN
    if math.isinf(x):
        return (0xFF800000 if x < 0 else 0x7F800000) if MODE == 'pcsx2' else \
            (0xFF7FFFFF if x < 0 else FMAX)
    b = struct.unpack('<Q', struct.pack('<d', x))[0]
    s = (b >> 63) << 31
    e = ((b >> 52) & 0x7FF) - 1023 + 127
    if e >= 0xFF:
        return s | FMAX
    if e <= 0:
        return s                  # flush to zero
    return s | e << 23 | ((b >> 29) & 0x7FFFFF)


def _step_down(u):
    """The next single toward zero."""
    m = u & 0x7FFFFFFF
    if m == 0:
        return u
    m -= 1
    if (m >> 23) == 0:
        m = 0                     # crossed into the denormals: FTZ
    return (u & 0x80000000) | m


def f_mul(a, b):
    n = _nan2(a, b)
    if n is not None:
        return n
    return _chop_double(u2f(a) * u2f(b))


def f_add(a, b):
    n = _nan2(a, b)
    if n is not None:
        return n
    x, y = u2f(a), u2f(b)
    s = x + y
    if s != s or math.isinf(s):
        return _chop_double(s)
    if s == 0.0:
        if x == 0.0 and y == 0.0:
            # IEEE: (+0)+(-0) = +0 except both negative
            neg = math.copysign(1, x) < 0 and math.copysign(1, y) < 0
            return 0x80000000 if neg else 0
        return 0
    bv = s - x
    av = s - bv
    err = (x - av) + (y - bv)
    r = _chop_double(s)
    if err != 0.0 and u2f(r) == s and (err < 0) != (s < 0):
        r = _step_down(r)
    return r


def f_sub(a, b):
    n = _nan2(a, b)              # SUBPS returns a NaN operand unflipped
    if n is not None:
        return n
    return f_add(a, b ^ 0x80000000)


def f_div(a, b):
    n = _nan2(a, b)
    if n is not None:
        return n
    x, y = u2f(a), u2f(b)
    if MODE == 'pcsx2' and (math.isinf(x) or math.isinf(y)):
        return _chop_double(x / y) if not (math.isinf(x) and math.isinf(y)) else QNAN
    if y == 0.0:
        return ((a ^ b) & 0x80000000) | FMAX
    if x == 0.0:
        return (a ^ b) & 0x80000000
    d = x / y
    r = _chop_double(d)
    if u2f(r) == d and abs(Fraction(x) / Fraction(y)) < abs(Fraction(d)):
        r = _step_down(r)
    return r


def f_sqrt(a):
    if MODE == 'pcsx2' and _isnan(a):
        return a | 0x400000
    x = abs(u2f(a))
    if math.isinf(x):
        return _chop_double(x)
    if x == 0.0:
        return 0
    s = math.sqrt(x)
    r = _chop_double(s)
    if u2f(r) == s and Fraction(s) * Fraction(s) > Fraction(x):
        r = _step_down(r)
    return r


def f_max(a, b, mini=False):
    def key(u):
        t = u ^ (0x7FFFFFFF if u >> 31 else 0)
        return t - (1 << 32) if t >> 31 else t
    ka, kb = key(a), key(b)
    if mini:
        return a if ka < kb else b
    return a if ka > kb else b


def ftoi(a, shift):
    if MODE == 'pcsx2':
        # cvttps2dq gives 0x80000000 for anything unrepresentable; microVU
        # turns it into 0x7FFFFFFF when the (scaled) bits compare GREATER
        # than 2^31's as signed ints -- which a positive NaN does
        b = f_mul(a, 0x41800000 + ((shift - 4) << 23)) if shift else a
        sb = b - (1 << 32) if b >> 31 else b
        x = u2f(b)
        if sb > 0x4F000000 - 1 and not (x == x and x < 2147483648.0):
            return 0x7FFFFFFF
        if x != x or x >= 2147483648.0 or x < -2147483648.0:
            return 0x80000000
        return int(x) & 0xFFFFFFFF
    x = u2f(a)
    if shift:
        x = u2f(_chop_double(x * (1 << shift)))
    if x != x:
        return 0x80000000
    if x >= 2147483648.0:
        return 0x7FFFFFFF
    if x < -2147483648.0:
        return 0x80000000
    return int(x) & 0xFFFFFFFF


def itof(a, shift):
    v = a - (1 << 32) if a >> 31 else a
    r = _chop_double(float(v))     # a double holds any int32 exactly
    if shift:
        r = _chop_double(u2f(r) / (1 << shift))
    return r


def sx16(v):
    v &= 0xFFFF
    return v - 0x10000 if v & 0x8000 else v


# ------------------------------------------------------------- state --------

class VUState:
    def __init__(self):
        self.vf = [[0, 0, 0, 0] for _ in range(32)]
        self.vf[0] = [0, 0, 0, 0x3F800000]
        self.vi = [0] * 16
        self.acc = [0, 0, 0, 0]
        self.q = 0
        self.p = 0
        self.i = 0
        self.mac = 0
        self.clip = 0
        self.status = 0
        self.mem = [0] * (MEMQ * 4)   # u32 words
        self.top = 0
        self.itop = 0


EFU_LAT = {'eatanxy': 54, 'eatanxz': 54, 'eatan': 54, 'eexp': 44, 'erleng': 24,
           'eleng': 18, 'ersadd': 18, 'ersqrt': 18, 'esadd': 11, 'esin': 29,
           'ercpr': 12, 'esqrt': 12, 'esum': 12}
Q_LAT = {'div': 7, 'sqrt': 7, 'rsqrt': 13}
FLAG_FMAC = ('bc', 'q', 'i', 'full', 'accbc', 'accq', 'acci', 'accfull')


class Sim:
    """Runs one launch.  `trace` collects (pc, cycle) if a list is given."""

    def __init__(self, st, micro_words):
        self.s = st
        self.code = micro_words           # list of (low, up)
        self.cycle = 0
        # pending cycles: per VF component, Q, P
        self.vfp = [[0] * 4 for _ in range(32)]
        self.vip = [0] * 16               # VI write latency: IALU 1, ILW/ILWR 4
        self.qp = 0
        self.pp = 0
        self.q_next = st.q
        self.p_next = st.p
        self.mac_inst = []                # (visible_cycle, value)
        self.clip_inst = []
        self.vi_hist = []                 # this block: (vi before, reg written, stall, reads flags)
        self.vi_delays = []               # (pc, reg, depth) where the rule fired
        self.kicks = []
        self.writes = set()               # memory words written

    # --- the pipeline bookkeeping ---
    def _inc(self, x, new_p=False):
        if x <= 0 and not new_p:
            return
        if x > 0:
            for r in self.vfp:
                for k in range(4):
                    if r[k]:
                        r[k] = max(r[k] - x, 0)
            for k in range(16):
                if self.vip[k]:
                    self.vip[k] = max(self.vip[k] - x, 0)
        if self.qp:
            self.qp = max(self.qp - x, 0)
            if not self.qp:
                self.s.q = self.q_next
        if self.pp:
            self.pp = max(self.pp - x, 0)
            if not self.pp or new_p:
                self.s.p = self.p_next

    def _flag(self, inst, initial):
        best, val = None, initial
        for vis, v in inst:
            if vis <= self.cycle and (best is None or vis >= best):
                best, val = vis, v
        return val

    # --- operand helpers ---
    def comp(self, r, c):
        return self.s.vf[r][c]

    def run(self, pc, max_steps=200000):
        s = self.s
        steps = 0
        end_after = None
        branch_to = None
        in_delay_next = False
        while True:
            steps += 1
            if steps > max_steps:
                raise RuntimeError('runaway at pc %d' % pc)
            low, up = self.code[pc]
            nxt = pc + 1
            in_delay = in_delay_next
            in_delay_next = False
            if branch_to is not None:
                nxt, branch_to = branch_to, None
            new_branch = self.step(pc, low, up)
            if in_delay:
                self.vi_hist = []         # a microVU block starts after every delay slot
            if self._was_branch:
                in_delay_next = True
            if new_branch is not None:
                branch_to = new_branch
            if end_after is not None:
                break
            if up >> 30 & 1:          # E bit: one more pair, then stop
                end_after = pc
            pc = nxt & 2047
        return pc

    def step(self, pc, low, up):
        s = self.s
        self._inc(1)
        ibit = up >> 31 & 1
        uw = up & 0x7FFFFFFF
        # ------------- analysis: what is read, what stalls
        uent = self._upper_entry(uw)
        lent = None if ibit else (('nop', 'nop') if low == V.LOWER_NOP else V.lower_entry(low))
        if uent is None or (lent is None and not ibit):
            raise RuntimeError('bad op at %d' % pc)
        stall = 0
        for r, comps in self._upper_reads(uw, uent):
            for c in comps:
                stall = max(stall, self.vfp[r][c])
        lname, lkind = lent if lent else ('loi', 'loi')
        for r, comps in self._lower_reads(low, lname, lkind):
            for c in comps:
                stall = max(stall, self.vfp[r][c])
        if not ibit:
            for r in self._vi_reads(low, lname, lkind):
                stall = max(stall, self.vip[r])
        if lname in Q_LAT or lname == 'waitq':
            stall = max(stall, self.qp)
        new_p = lname in EFU_LAT
        if new_p:
            stall = max(stall, self.pp - 1 if self.pp else 0)
        if lname == 'waitp':
            stall = max(stall, self.pp)
        self._inc(stall, new_p=False)
        if new_p and self.pp:
            self.s.p = self.p_next     # the old result flips in as the new one issues
        self.cycle += stall
        # ------------- read everything, then write
        vi_before = list(s.vi)
        s.vi_at_issue = vi_before
        q_now, p_now = s.q, s.p
        if ibit:
            s.i = low                  # LOI: the upper op of this pair sees it
        uwrite = self._exec_upper(uw, uent, q_now)
        lres = None if ibit else self._exec_lower(pc, low, lname, lkind, p_now)
        branch = None
        lvfw = None
        vi_written = None
        reads_flags = lname in ('fmand', 'fmeq', 'fmor', 'fcget', 'fcand', 'fceq', 'fcor',
                                'fsand', 'fseq', 'fsor')
        if lres:
            lvfw, viw, branch, mem = lres
            if viw is not None:
                r, v = viw
                if r:
                    s.vi[r] = v & 0xFFFF
                    vi_written = r
                    self.vip[r] = 4 if lname in ('ilw', 'ilwr') else 1
            for (a, v) in mem:
                s.mem[a] = v
                self.writes.add(a)
        # lower VF write first, upper wins when both write one register
        if lvfw:
            r, comps, vals = lvfw
            if r and not (uwrite and uwrite[0] == r and uwrite[0] != 'acc'):
                for c in comps:
                    s.vf[r][c] = vals[c]
                    self.vfp[r][c] = 4
        if uwrite:
            r, comps, vals, flags = uwrite
            if r == 'acc':
                for c in comps:
                    s.acc[c] = vals[c]
            elif r:
                for c in comps:
                    s.vf[r][c] = vals[c]
                    self.vfp[r][c] = 4
            if flags is not None:
                kind, v = flags
                if kind == 'mac':
                    self.mac_inst.append((self.cycle + 4, v))
                else:
                    self.clip_inst.append((self.cycle + 4, v))
        if lname in Q_LAT:
            self.qp = Q_LAT[lname]
        if new_p:
            self.pp = EFU_LAT[lname]
        self._was_branch = branch is not None
        if branch is not None:
            branch = self._branch_vi_delay(pc, low, lname, lkind, branch, stall)
        self.vi_hist.append((vi_before, vi_written, stall, reads_flags))
        self.cycle += 1
        return branch

    # --- the branch VI-delay rule (microVU analyzeBranchVI) ---
    def _branch_vi_delay(self, pc, low, lname, lkind, branch, stall):
        """analyzeBranchVI: a conditional branch whose register the
        previous instruction wrote reads it as it was before the EARLIEST of
        the consecutive writers in the last four instructions of this block."""
        target, regs, recompute = branch
        if not regs or stall or recompute is None or lname in ('jr', 'jalr'):
            return target
        hist = self.vi_hist          # this block's instructions, newest last
        vals = list(self.s.vi_at_issue)
        delayed = False
        for reg in regs:
            j, i, cyc = None, 0, 0
            while i < 4 and cyc < 4 and i < len(hist):
                before, written, st, rf = hist[-1 - i]
                if written == reg:
                    if rf:
                        break
                    j = i
                elif i == 0:
                    break
                cyc += st + 1
                i += 1
            if j is not None:
                vals[reg] = hist[-1 - j][0][reg]
                delayed = True
                self.vi_delays.append((pc, reg, j + 1))
        return recompute(vals) if delayed else target

    # --- decode helpers ---
    def _upper_entry(self, w):
        op = w & 0x3F
        if op >= 0x3C:
            return V.UPPER_T3.get(((w >> 6) & 31, op & 3))
        return V.UPPER_MAIN.get(op)

    @staticmethod
    def _dest(w):
        d = (w >> 21) & 15
        return [k for k in range(4) if d & (8 >> k)]

    def _upper_reads(self, w, ent):
        name, kind = ent
        dest = self._dest(w)
        ft, fs = (w >> 16) & 31, (w >> 11) & 31
        bc = w & 3
        out = []
        if kind == 'nop':
            return out
        if kind in ('bc', 'accbc'):
            out += [(fs, dest), (ft, [bc])]
        elif kind in ('q', 'i', 'accq', 'acci'):
            out += [(fs, dest)]
        elif kind in ('full', 'accfull'):
            if name in ('opmula', 'opmsub'):
                out += [(fs, [0, 1, 2]), (ft, [0, 1, 2])]
            else:
                out += [(fs, dest), (ft, dest)]
        elif kind == 'cvt':
            out += [(fs, dest)]
        elif kind == 'clip':
            out += [(fs, [0, 1, 2]), (ft, [3])]
        return [(r, c) for r, c in out if r]

    @staticmethod
    def _vi_reads(w, name, kind):
        it, is_ = (w >> 16) & 15, (w >> 11) & 15
        reads = {
            'lq': (is_,), 'sq': (it,), 'ilw': (is_,), 'iu15': (is_,), 'iaddi': (is_,),
            'i3': (is_, it), 'lqi': (is_,), 'lqd': (is_,), 'sqi': (it,), 'sqd': (it,),
            'mfir': (is_,), 'fm': (is_,), 'ib2': (is_, it), 'ib1': (is_,), 'jr': (is_,),
            'jalr': (is_,), 'xgkick': (is_,),
        }.get(kind, ())
        if name == 'isw':
            reads = (is_, it)
        elif name in ('ilwr',):
            reads = (is_,)
        elif name == 'iswr':
            reads = (is_, it)
        return [r for r in reads if r]

    def _lower_reads(self, w, name, kind):
        t, sreg = (w >> 16) & 31, (w >> 11) & 31
        dest = self._dest(w)
        fsf, ftf = (w >> 21) & 3, (w >> 23) & 3
        out = []
        if kind in ('sq', 'sqi', 'sqd'):
            out.append((sreg, dest))
        elif kind == 'mv':
            out.append((sreg, dest if name == 'move' else [(c + 1) & 3 for c in dest]))
        elif kind == 'div':
            out += [(sreg, [fsf]), (t, [ftf])]
        elif kind == 'sqrt':
            out.append((t, [ftf]))
        elif kind in ('mtir', 'r1', 'epf'):
            out.append((sreg, [fsf]))
        elif kind == 'ep':
            out.append((sreg, dest))
        return [(r, c) for r, c in out if r]

    # --- upper execution: -> (reg|'acc', comps, vals[4], flags) or None ---
    def _exec_upper(self, w, ent, q):
        s = self.s
        name, kind = ent
        if kind == 'nop':
            return None
        dest = self._dest(w)
        ft, fs, fd = (w >> 16) & 31, (w >> 11) & 31, (w >> 6) & 31
        bc = w & 3
        a = s.vf[fs]
        if kind in ('bc', 'accbc'):
            b = [s.vf[ft][bc]] * 4
        elif kind in ('q', 'accq'):
            b = [q] * 4
        elif kind in ('i', 'acci'):
            b = [s.i] * 4
        elif kind in ('full', 'accfull'):
            b = s.vf[ft]
        out = [0, 0, 0, 0]
        if kind == 'cvt':
            for c in dest:
                if name == 'abs':
                    out[c] = a[c] & 0x7FFFFFFF
                elif name.startswith('ftoi'):
                    out[c] = ftoi(a[c], int(name[4:]))
                else:
                    out[c] = itof(a[c], int(name[4:]))
            return (ft, dest, out, None)
        if kind == 'clip':
            w_ = u2f(s.vf[ft][3] & 0x7FFFFFFF)
            bits = 0
            for k in range(3):
                x = u2f(a[k])
                if x > w_:
                    bits |= 1 << (2 * k)
                if x < -w_:
                    bits |= 1 << (2 * k + 1)
            v = ((s.clip << 6) | bits) & 0xFFFFFF
            s.clip = v
            return (0, [], out, ('clip', v))
        # addx -> add, minii -> mini, maddax -> madd, mulaq -> mul, adda -> add
        op = name[:-1] if kind in ('bc', 'accbc', 'q', 'accq', 'i', 'acci') else name
        is_acc = kind.startswith('acc')
        if is_acc and op != 'opmula':
            op = op[:-1]
        acc = s.acc
        if MODE == 'pcsx2':
            cfs, cft = _clamp_operands(op, kind, is_acc, (w >> 21) & 15)
            if kind in ('accbc',) and op == 'mul' and (w & 3) == 3 and (w >> 21) & 15 == 0xF:
                cft = True                  # MULAw clamps Ft as well in its PS form
            if cft and kind in ('i', 'acci'):
                cft = False                 # checkVFClamp: never the I register
            if cfs:
                a = [clamp1(x) for x in a]
            if cft:
                b = [clamp1(x) for x in b]
        if name in ('opmula', 'opmsub'):
            A, B = a, s.vf[ft]
            idx = [(1, 2), (2, 0), (0, 1)]
            for c in dest:
                if c > 2:
                    continue
                i, j = idx[c]
                # the same index pattern in both, so `opmula acc, a, b` then
                # `opmsub d, b, a` leaves d = a x b
                if name == 'opmula':
                    out[c] = f_mul(A[i], B[j])
                else:
                    out[c] = f_sub(acc[c], f_mul(A[i], B[j]))
        else:
            for c in dest:
                x, y = a[c], b[c]
                if op == 'add':
                    out[c] = f_add(x, y)
                elif op == 'sub':
                    out[c] = f_sub(x, y)
                elif op == 'mul':
                    out[c] = f_mul(x, y)
                elif op == 'madd':
                    out[c] = f_add(acc[c], f_mul(x, y))
                elif op == 'msub':
                    out[c] = f_sub(acc[c], f_mul(x, y))
                elif op == 'max':
                    out[c] = f_max(x, y)
                elif op == 'mini':
                    out[c] = f_max(x, y, mini=True)
                else:
                    raise RuntimeError('upper op %s' % name)
        flags = None
        if op not in ('max', 'mini'):
            m = 0
            for c in dest:
                bit = 3 - c
                if out[c] & 0x7FFFFFFF == 0:
                    m |= 1 << bit
                if out[c] >> 31:
                    m |= 1 << (bit + 4)
            s.mac = m
            flags = ('mac', m)
        return ('acc' if is_acc else fd, dest, out, flags)

    # --- lower execution: -> (vfwrite, viwrite, branch, memwrites) ---
    def _exec_lower(self, pc, w, name, kind, p):
        s = self.s
        t, sr, d = (w >> 16) & 31, (w >> 11) & 31, (w >> 6) & 31
        it, is_, id_ = t & 15, sr & 15, d & 15
        dest = self._dest(w)
        fsf, ftf = (w >> 21) & 3, (w >> 23) & 3
        imm11 = V._sext(w & 0x7FF, 11)
        vi = s.vi
        mem = []

        def qaddr(base):
            return (base & (MEMQ - 1)) * 4

        def load(addr):
            return [s.mem[addr + k] for k in range(4)]

        if name == 'nop':
            return None
        if kind == 'lq':
            a = qaddr(sx16(vi[is_]) + imm11)
            return ((t, dest, load(a)), None, None, mem)
        if kind == 'sq':
            a = qaddr(sx16(vi[it]) + imm11)
            return (None, None, None, [(a + c, s.vf[sr][c]) for c in dest])
        if kind == 'lqi':
            a = qaddr(vi[is_])
            return ((t, dest, load(a)), (is_, vi[is_] + 1), None, mem)
        if kind == 'lqd':
            n = (vi[is_] - 1) & 0xFFFF
            return ((t, dest, load(qaddr(n))), (is_, n), None, mem)
        if kind == 'sqi':
            a = qaddr(vi[it])
            return (None, (it, vi[it] + 1), None, [(a + c, s.vf[sr][c]) for c in dest])
        if kind == 'sqd':
            n = (vi[it] - 1) & 0xFFFF
            a = qaddr(n)
            return (None, (it, n), None, [(a + c, s.vf[sr][c]) for c in dest])
        if name == 'ilw':
            a = qaddr(sx16(vi[is_]) + imm11)
            c = dest[0] if dest else 0
            v = s.mem[a + c] & 0xFFFF
            return (None, (it, v), None, mem)
        if kind == 'ilwr' and name == 'ilwr':
            a = qaddr(vi[is_])
            c = dest[0] if dest else 0
            return (None, (it, s.mem[a + c] & 0xFFFF), None, mem)
        if name == 'isw':
            a = qaddr(sx16(vi[is_]) + imm11)
            return (None, None, None, [(a + c, vi[it] & 0xFFFF) for c in dest])
        if name == 'iswr':
            a = qaddr(vi[is_])
            return (None, None, None, [(a + c, vi[it] & 0xFFFF) for c in dest])
        if kind == 'iu15':
            imm = ((w >> 10) & 0x7800) | (w & 0x7FF)
            v = vi[is_] + imm if name == 'iaddiu' else vi[is_] - imm
            return (None, (it, v), None, mem)
        if kind == 'iaddi':
            return (None, (it, vi[is_] + V._sext(d, 5)), None, mem)
        if kind == 'i3':
            a, b = vi[is_], vi[it]
            v = {'iadd': a + b, 'isub': a - b, 'iand': a & b, 'ior': a | b}[name]
            return (None, (id_, v), None, mem)
        if kind == 'mv':
            src = s.vf[sr]
            if name == 'move':
                vals = list(src)
            else:
                vals = [src[1], src[2], src[3], src[0]]
            return ((t, dest, vals), None, None, mem)
        if kind == 'mfir':
            v = sx16(vi[is_]) & 0xFFFFFFFF
            return ((t, dest, [v] * 4), None, None, mem)
        if kind == 'mtir':
            return (None, (it, s.vf[sr][fsf] & 0xFFFF), None, mem)
        if kind == 'mfp':
            return ((t, dest, [p] * 4), None, None, mem)
        if name == 'xtop':
            return (None, (it, s.top), None, mem)
        if name == 'xitop':
            return (None, (it, s.itop), None, mem)
        if name == 'xgkick':
            self.kicks.append(vi[is_])
            return None
        if kind == 'div':
            a, b = s.vf[sr][fsf], s.vf[t][ftf]
            if name == 'div':
                self.q_next = clamp1(f_div(a, b))
            else:
                self.q_next = clamp1(f_div(a, f_sqrt(b)))
            if not self.qp:
                pass
            return None
        if kind == 'sqrt':
            self.q_next = f_sqrt(s.vf[t][ftf])
            return None
        if kind == 'wait':
            return None
        if kind == 'ep':
            v = s.vf[sr]
            # DPPS 0x71: (y*y + x*x) + z*z, each step chopped.  The order
            # only shows with two NaN lanes: PCSX2's host passes y's payload
            # (measured over 92 launches whose padding vertex was garbage)
            sq = [f_mul(v[k], v[k]) for k in range(3)]
            tot = f_add(f_add(sq[1], sq[0]), sq[2])
            if name == 'eleng':
                self.p_next = f_sqrt(tot)
            elif name == 'erleng':
                r = f_sqrt(tot)
                if MODE == 'pcsx2' and r & 0x7FFFFFFF == 0:
                    # SSE_DIVSS clamps only its operands: 1/0 is +inf on
                    # the host (VIRFan with no point light, 598 launches)
                    self.p_next = 0x7F800000 | (r & 0x80000000)
                else:
                    self.p_next = f_div(0x3F800000, r)
            else:
                raise RuntimeError('EFU %s' % name)
            return None
        if name in ('fmand', 'fmeq', 'fmor'):
            m = self._flag(self.mac_inst, self.s_mac0)
            if name == 'fmand':
                v = m & vi[is_]
            elif name == 'fmor':
                v = m | vi[is_]
            else:
                v = int(m == (vi[is_] & 0xFFFF))
            return (None, (it, v), None, mem)
        if name in ('fcget', 'fcand', 'fceq', 'fcor'):
            c = self._flag(self.clip_inst, self.s_clip0)
            imm = w & 0xFFFFFF
            if name == 'fcget':
                return (None, (it, c & 0xFFF), None, mem)
            if name == 'fcand':
                return (None, (1, int((c & imm) != 0)), None, mem)
            if name == 'fceq':
                return (None, (1, int(c == imm)), None, mem)
            return (None, (1, int((c | imm) & 0xFFFFFF == 0xFFFFFF)), None, mem)
        if kind in V.BRANCH_KINDS or name in ('jr', 'jalr'):
            tgt = V.branch_target(w, pc)
            link = None
            if name == 'b':
                return (None, None, (tgt, (), None), mem)
            if name == 'bal':
                return (None, (it, pc + 2), (tgt, (), None), mem)
            if name == 'jr':
                return (None, None, (vi[is_], (is_,), lambda old: old[is_]), mem)
            if name == 'jalr':
                return (None, (it, pc + 2), (vi[is_], (is_,), lambda old: old[is_]), mem)
            cmp = {
                'ibeq': lambda o: sx16(o[it]) == sx16(o[is_]),
                'ibne': lambda o: sx16(o[it]) != sx16(o[is_]),
                'ibltz': lambda o: sx16(o[is_]) < 0,
                'ibgtz': lambda o: sx16(o[is_]) > 0,
                'iblez': lambda o: sx16(o[is_]) <= 0,
                'ibgez': lambda o: sx16(o[is_]) >= 0,
            }[name]
            regs = tuple(r for r in ((it, is_) if kind == 'ib2' else (is_,)) if r)
            fall = pc + 2

            def recompute(old, cmp=cmp, tgt=tgt, fall=fall):
                return tgt if cmp(old) else None
            res = tgt if cmp(vi) else None
            return (None, None, (res, regs, recompute), mem)   # a branch even when not taken
        raise RuntimeError('lower op %s (%s)' % (name, kind))


# ----------------------------------------------------- vu1dump records ------

REC = 24 + 16384 + 16384 + 512 + 128 + 16
REG_STATUS, REG_MAC, REG_CLIP, REG_R, REG_I, REG_Q, REG_P, REG_TPC = 16, 17, 18, 20, 21, 22, 23, 26


class Record:
    def __init__(self, buf, off):
        hdr = struct.unpack_from('<6I', buf, off)
        assert hdr[0] == 0x57315556, 'not a VU1W record'
        self.addr, self.top, self.itop, self.tpc, self.cycle = hdr[1:]
        o = off + 24
        self.mem = list(struct.unpack_from('<4096I', buf, o)); o += 16384
        micro = struct.unpack_from('<4096I', buf, o); o += 16384
        self.micro = [(micro[2 * k], micro[2 * k + 1]) for k in range(2048)]
        self.vf = [list(struct.unpack_from('<4I', buf, o + 16 * r)) for r in range(32)]; o += 512
        self.vi = list(struct.unpack_from('<32I', buf, o)); o += 128
        self.acc = list(struct.unpack_from('<4I', buf, o))

    def state(self):
        st = VUState()
        st.vf = [list(r) for r in self.vf]
        st.vi = [v & 0xFFFF for v in self.vi[:16]]
        st.acc = list(self.acc)
        st.q, st.p, st.i = self.vi[REG_Q], self.vi[REG_P], self.vi[REG_I]
        st.mac, st.clip, st.status = self.vi[REG_MAC], self.vi[REG_CLIP], self.vi[REG_STATUS]
        st.mem = list(self.mem)
        st.top, st.itop = self.top, self.itop
        return st


def load_records(path):
    buf = open(path, 'rb').read()
    return [Record(buf, k * REC) for k in range(len(buf) // REC)]


def load_records_prefix(path, n):
    with open(path, 'rb') as f:
        buf = f.read(n * REC)
    return [Record(buf, k * REC) for k in range(len(buf) // REC)]


def run_record(rec):
    st = rec.state()
    sim = Sim(st, rec.micro)
    sim.s_mac0 = st.mac
    sim.s_clip0 = st.clip
    entry = rec.addr if rec.addr != 0xFFFFFFFF else rec.tpc // 8
    sim.run(entry)
    if MODE == 'pcsx2' and sim.pp:
        # microVU's end-of-program save puts the in-flight EFU result in
        # REG_P (measured: the stale-looking P after VIRStrip4 is the
        # LAST erleng, run on the padding past the strip's end)
        sim.s.p = sim.p_next
    return sim

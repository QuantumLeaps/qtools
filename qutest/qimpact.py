#!/usr/bin/env python3

# =============================================================================
# qimpact.py -- QUTest Instrumentation-Impact Check
#
#                    Q u a n t u m  L e a P s
#                    ------------------------
#                    Modern Embedded Software
#
# Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
#
# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
#
# This software is dual-licensed under the terms of the open source GNU
# General Public License version 3 (or any later version), or alternatively,
# under the terms of one of the closed source Quantum Leaps commercial
# licenses.
#
# The terms of the open source GNU General Public License version 3
# can be found at: <www.gnu.org/licenses/gpl-3.0>
#
# The terms of the closed source Quantum Leaps commercial licenses
# can be found at: <www.state-machine.com/licensing>
#
# Redistributions in source code must retain this top-level comment block.
# Plagiarizing this software to sidestep the license obligations is illegal.
#
# Contact information:
# <www.state-machine.com>
# <info@state-machine.com>
#=============================================================================

import argparse
import difflib
import glob
import hashlib
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import datetime

TOOL_NAME = 'qimpact'
TOOL_VERSION = '8.1.6'

# default allowlists for rule R5: the QP/Spy instrumentation calls the QS
# functions (SafeQP/C: QS_*; SafeQP/C++: namespace QP::QS and the QS-only
# type QP::QSpyId) and the critical-section and memory-isolation functions
# of the port (QUTest ports for the host: QF_critEntry/QF_critExit, C++:
# QP::QF::critEntry/critExit; ARM Cortex-M ports: QF_crit_entry_/
# QF_crit_exit_; QF_onMemSys/QF_onMemApp for both)
DEFAULT_ALLOW_CALLS = (r'QS_\w+|QF_critEntry|QF_critExit|'
                       r'QF_crit_entry_|QF_crit_exit_|'
                       r'QF_onMemSys|QF_onMemApp|'
                       r'QF_enterCriticalSection_|QF_leaveCriticalSection_|'
                       r'QP::QS::\w+|QP::QSpyId::\w+|'
                       r'QP::QF::(?:critEntry|critExit|onMemSys|onMemApp|'
                       r'enterCriticalSection_|leaveCriticalSection_)')
# Note: the C++ dump prints static data members WITHOUT their class or
# namespace (QP::QS::priv_ appears as 'priv_'), so such names cannot be
# attributed to QS and are deliberately not allowlisted (R5 -> REVIEW).
DEFAULT_ALLOW_ASSIGN = r'QS_\w+|QP::QS::\w+'
# test-only code may (re)assign the port's critical-section status variable
# only from the port's critical-section entry (QS_CRIT_ENTRY on the QUTest
# ports); QP never nests critical sections, so this cannot disturb a
# critical section of the production code
RE_ALLOW_CRIT = re.compile(
    r'^critStat_ = (?:QF_critEntry|QP::QF::critEntry) \(\);$')

C_KEYWORDS = {
    'if', 'else', 'while', 'for', 'do', 'switch', 'case', 'default',
    'return', 'goto', 'break', 'continue', 'sizeof', 'void', 'char',
    'short', 'int', 'long', 'unsigned', 'signed', 'float', 'double',
    '_Bool', 'struct', 'union', 'enum', 'const', 'volatile', 'static',
    'register', 'extern', 'inline', '__asm__', 'asm', '__volatile__',
}

RE_FUNC_HDR = re.compile(r'^;; Function (.+) \(\S*\)$')
RE_LABEL_TOK = re.compile(r'<D\.(\d+)>')          # compiler labels
RE_TEMP_TOK = re.compile(r'\bD\.(\d+)\b')         # compiler temporaries
RE_TYPE_TOK = re.compile(r'<T[0-9a-f]+>')         # anonymous type ids
RE_NOOP = re.compile(r'^\(void\)\s*(?:\([^()]*\)\s*)*(?:0|[A-Za-z_]\w*);$')
RE_CALL = re.compile(r'((?:[A-Za-z_]\w*::)*[A-Za-z_]\w*)\s*\((?!\s*\*)')   # not fn-ptr types
RE_DECL_INIT = re.compile(
    r'^(?:(?:const|volatile|static|register|unsigned|signed|struct|union|'
    r'enum)\s+)*([A-Za-z_]\w*)(?:\s*\*+\s*(?:(?:const|volatile)\s+)*|\s+)'
    r'([A-Za-z_]\w*)(?:\[[^\]]*\])*'
    r'\s*(?:=[^=].*)?;$')
NOT_TYPES = {'return', 'goto', 'case', 'else', 'do', 'break', 'continue'}
RE_STRING = re.compile(r'"(?:[^"\\]|\\.)*"|\'(?:[^\'\\]|\\.)*\'')


def assign_target(stmt):
    """Target of an assignment statement ('x = y;', 'a[i] += 1;'), or None.
    Only an assignment operator outside all parentheses, brackets, and braces
    counts (not '=' in a C++ initializer such as '{.m=1}' or in a call
    argument), and never ==, !=, <=, >=."""
    s = RE_STRING.sub('""', stmt)
    depth = 0
    for k, ch in enumerate(s):
        if ch in '([{':
            depth += 1
        elif ch in ')]}':
            depth -= 1
        elif ch == '=' and depth == 0:
            prev = s[k - 1] if k else ''
            nxt = s[k + 1] if k + 1 < len(s) else ''
            if nxt == '=' or prev in '=!':
                return None             # == or != (comparison)
            if prev in '<>' and not s[:k].endswith(('<<', '>>')):
                return None             # <= or >= (comparison)
            lhs = s[:k]
            lhs = re.sub(r'(?:[+\-*/%&|^]|<<|>>)$', '', lhs)   # compound op
            return lhs.strip() or None
    return None


def func_name(sig):
    """Qualified name of a function from its dump header: C dumps give the
    bare name ('QHsm_ctor'), C++ dumps the signature
    ('void QP::QHsm::init(const void*, uint_fast8_t)')."""
    m = re.search(r'((?:[A-Za-z_]\w*::)*~?[A-Za-z_]\w*)\(', sig)
    return m.group(1) if m else sig


def decl_name(line):
    """Name declared by a simple declaration line, or None."""
    m = RE_DECL_INIT.match(line)
    if not m or m.group(1) in NOT_TYPES or '(' in line.split('=')[0]:
        return None
    return m.group(2)
RE_INCDEC = re.compile(
    r'^(?:\+\+|--)\s*([A-Za-z_][\w.\->\[\]]*)|^([A-Za-z_][\w.\->\[\]]*)\s*(?:\+\+|--)')
RE_CTRL_HDR = re.compile(r'^(if|else|while|for|switch|do)\b')
RE_CTRL_XFER = re.compile(r'^(return|break|continue)\b')
# jump targets: compiler labels (<D.nnn>, written <Lnnn> after lab()) and
# labels of the source code (identifiers); 'default:' is not a label
RE_GOTO = re.compile(r'\bgoto\s+(<L\d+>|[A-Za-z_]\w*)')
RE_LABEL_DEF = re.compile(r'^(?!default\b)(<L\d+>|[A-Za-z_]\w*):(?!:)')
# The C front end lists a label of the source code among the declarations of
# its block as 'void NAME = <<< error >>>;' (the label declaration has no
# printable initializer). This is not a compilation error; such lines are
# removed before the dump is checked for errors.
RE_LABEL_DECL = re.compile(r'^[ \t]*void [A-Za-z_]\w* = <<< error >>>;[ \t]*$',
                           re.M)


# -----------------------------------------------------------------------------
class ToolError(Exception):
    pass


# -----------------------------------------------------------------------------
def sha256(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(65536), b''):
            h.update(chunk)
    return h.hexdigest()


def compiler_version(cc):
    try:
        out = subprocess.run([cc, '--version'], capture_output=True,
                             text=True, check=True).stdout
        return out.splitlines()[0].strip()
    except (OSError, subprocess.CalledProcessError) as ex:
        raise ToolError('cannot run compiler "%s": %s' % (cc, ex))


def compiler_target(cc):
    """Target triple of the compiler ('cc -dumpmachine', e.g.
    'i686-w64-mingw32' or 'x86_64-linux-gnu'); recorded in every report
    because the compiler's version line does not identify its target."""
    try:
        out = subprocess.run([cc, '-dumpmachine'], capture_output=True,
                             text=True, check=True).stdout
        return out.strip() or '<unknown>'
    except (OSError, subprocess.CalledProcessError) as ex:
        raise ToolError('cannot run compiler "%s": %s' % (cc, ex))


def compile_dump(cc, src, cflags, defs, workdir, stem):
    """Compile a self-test case into workdir (dump stem.tree, object
    stem.o) and return the text of its tree dump."""
    os.makedirs(workdir, exist_ok=True)
    dump = os.path.join(workdir, stem + '.tree')
    for f in (dump, os.path.join(workdir, stem + '.o')):
        if os.path.exists(f):
            os.remove(f)
    cmd = [cc] + cflags + ['-D' + d for d in defs]
    if '-c' not in cflags:
        cmd.insert(1, '-c')
    cmd += ['-fdump-tree-original=' + os.path.abspath(dump),
            os.path.abspath(src),
            '-o', os.path.abspath(os.path.join(workdir, stem + '.o'))]
    proc = subprocess.run(cmd, cwd=workdir, capture_output=True, text=True)
    if proc.returncode != 0 or not os.path.isfile(dump):
        raise ToolError('self-test compilation failed for %s (defs: %s):\n%s'
                        % (src, ' '.join(defs) or '<none>', proc.stderr))
    return read_text(dump)


def read_text(path):
    with open(path, encoding='utf-8', errors='replace') as f:
        return f.read()


# -----------------------------------------------------------------------------
def parse_dump(text):
    """Split a dump into {function: [raw statement lines]} (order kept)."""
    funcs = {}
    order = []
    cur = None
    for line in text.splitlines():
        m = RE_FUNC_HDR.match(line)
        if m:
            cur = m.group(1)
            if cur in funcs:
                raise ToolError('function "%s" appears twice in dump' % cur)
            funcs[cur] = []
            order.append(cur)
            continue
        if cur is None or line.startswith(';;'):
            continue
        s = line.strip()
        if s:
            funcs[cur].append(s)
    for fn in order:
        funcs[fn] = flatten_cxx(funcs[fn])
    return funcs, order


# -----------------------------------------------------------------------------
# C++ front end: its dump prints some statement trees that have no C syntax
# as '<<< Unknown tree: KIND ... >>>' (spanning several lines) and wraps
# full expressions in '<<cleanup_point ... >>'. These wrappers are removed
# so that C++ statements read like C statements:
#   <<cleanup_point X>>                 -> X
#   <<< Unknown tree: expr_stmt X >>>   -> X         (expression statement)
#   <<< Unknown tree: void_cst >>>      -> (void) 0  (no-op)
#   <<< Unknown tree: non_lvalue_expr X >>> -> X     (rvalue of X)
#   (void) (X = Y);                     -> X = Y;    (discarded assignment)
# Any other '<<< Unknown tree' kind is kept verbatim (compared as text).
# The C front end never prints these wrappers, so C dumps are unchanged.
CXX_UNWRAP = {'expr_stmt', 'non_lvalue_expr'}


def _scan_cxx(s):
    """Parse the wrappers of one statement into a tree; return (tree,
    depth of wrappers still open at the end)."""
    root = []
    stack = [('R', None, root)]
    i, n = 0, len(s)
    while i < n:
        top = stack[-1][0]
        if s.startswith('<<< Unknown tree: ', i):
            m = re.match(r'<<< Unknown tree: (\w+)', s[i:])
            node = ('U', m.group(1), [])
            stack[-1][2].append(node)
            stack.append(node)
            i += m.end()
        elif s.startswith('<<cleanup_point', i):
            node = ('C', None, [])
            stack[-1][2].append(node)
            stack.append(node)
            i += len('<<cleanup_point')
        elif top == 'U' and s.startswith('>>>', i):
            stack.pop()
            i += 3
        elif top == 'C' and s.startswith('>>', i) and \
                not (i > 0 and s[i - 1] == ' ') and \
                not (i > 0 and s[i - 1] == '-') and \
                not (top == 'C' and s.startswith('>>>', i) and
                     _angle_open(stack[-1][2])):
            stack.pop()
            i += 2
        else:
            stack[-1][2].append(s[i])
            i += 1
    return root, len(stack) - 1


def _angle_open(children):
    """True if the text collected in a wrapper has an unclosed '<' token
    (e.g. 'TARGET_EXPR <D.12, {}' before its closing '>')."""
    txt = ''.join(c for c in children if isinstance(c, str))
    depth = 0
    for k, ch in enumerate(txt):
        if ch == '<' and k + 1 < len(txt) and txt[k + 1] not in ' =<':
            depth += 1
        elif ch == '>' and depth and txt[k - 1] not in ' -':
            depth -= 1
    return depth > 0


def _render(nodes):
    out = []
    for nd in nodes:
        if isinstance(nd, str):
            out.append(nd)
        elif nd[0] == 'C':
            out.append(_render(nd[2]).strip())
        elif nd[1] == 'void_cst':
            out.append('(void) 0')
        elif nd[1] in CXX_UNWRAP:
            out.append(_render(nd[2]).strip())
        else:
            out.append('<<< Unknown tree: %s%s >>>' % (nd[1], _render(nd[2])))
    return ''.join(out)


def _strip_void_assign(s):
    """'(void) (X = Y);' -> 'X = Y;' (only if the parentheses enclose the
    whole expression and it is an assignment)."""
    if not (s.startswith('(void) (') and s.endswith(');')):
        return s
    inner = s[len('(void) ('):-2]
    depth = 0
    for ch in inner:
        depth += (ch == '(') - (ch == ')')
        if depth < 0:
            return s            # the '(' closes before the end
    if depth == 0 and assign_target(inner + ';'):
        return inner + ';'
    return s


def flatten_cxx(lines):
    """Join multi-line C++ statement trees and remove their wrappers."""
    if not any('<<cleanup_point' in s or '<<< Unknown tree' in s
               for s in lines):
        return lines                    # C dump: unchanged
    out = []
    buf = None
    for s in lines:
        buf = s if buf is None else buf + ' ' + s
        tree, open_ = _scan_cxx(buf)
        if open_:
            continue                    # statement continues on next line
        t = re.sub(r'\s+', ' ', _render(tree)).strip()
        t = re.sub(r';\s*;$', ';', t)   # '<<cleanup_point X;>>;'
        out.append(_strip_void_assign(t))
        buf = None
    if buf is not None:
        raise ToolError('unterminated C++ statement tree in compiler dump')
    return out


def drop_dead_blocks(lines):
    """Remove 'if (0) { }' blocks whose body is empty after no-op removal.

    Such blocks come from the trace macros of the production build (for
    example QS_BEGIN_ID expanding to 'if (0) {...}'); they can never execute.
    A dead block with any remaining statement is kept (and then reported).
    """
    out = []
    i = 0
    while i < len(lines):
        if lines[i] == 'if (0)' and i + 2 < len(lines) and \
                lines[i + 1] == '{' and lines[i + 2] == '}':
            i += 3
            continue
        out.append(lines[i])
        i += 1
    return out


def drop_bare_scopes(lines):
    """Remove the braces of bare scope blocks (not opened by a control
    header). GCC opens such a scope wherever a block declares variables, so
    a declaration that exists in one build only can add a scope around
    unrelated code. A bare scope affects only the visibility of names, never
    the control flow. Braces of if/else/loop/switch bodies are kept.
    """
    out = []
    stack = []
    prev = None
    for s in lines:
        if s == '{':
            ctrl = prev is not None and (
                RE_CTRL_HDR.match(prev) is not None and not prev.endswith(';'))
            stack.append(ctrl)
            if ctrl:
                out.append(s)
        elif s == '}':
            if not stack:
                raise ToolError('unbalanced braces in compiler dump')
            if stack.pop():
                out.append(s)
        else:
            out.append(s)
        prev = s
    if stack:
        raise ToolError('unbalanced braces in compiler dump')
    return out


def normalize(lines):
    """Return (normalized lines for alignment, raw lines with label ids).

    Labels and temporaries are replaced by a placeholder for alignment, while
    their per-variant ids are kept separately for the correspondence check R4.
    No-op statements and debug markers are removed.
    """
    norm = []
    keyed = []
    lines = [RE_TYPE_TOK.sub('<T>', s) for s in lines]
    # '# DEBUG ...' lines are debug-information markers (statement frontiers
    # emitted with -g and optimization), not code
    lines = [s for s in lines if not s.startswith('# DEBUG')]
    lines = [s for s in lines if not RE_NOOP.match(s)]
    lines = drop_dead_blocks(lines)
    lines = drop_bare_scopes(lines)
    hdrs = []               # headers of the currently open blocks
    prev_key = ''
    for s in lines:
        ids = RE_LABEL_TOK.findall(s) + ['t' + t for t in RE_TEMP_TOK.findall(s)]
        k = RE_LABEL_TOK.sub('<L>', s)
        k = RE_TEMP_TOK.sub('D.#', k)
        # The alignment key includes the nesting depth, and for braces also
        # the header of the block, so that a line can only correspond to a
        # line at the same nesting level and a brace only to a brace of the
        # same kind of block.
        if s == '{':
            hdrs.append(prev_key)
            key = '%d|{@%s' % (len(hdrs) - 1, prev_key)
        elif s == '}':
            if not hdrs:
                raise ToolError('unbalanced braces in compiler dump')
            key = '%d|}@%s' % (len(hdrs) - 1, hdrs.pop())
        else:
            key = '%d|%s' % (len(hdrs), k)
        norm.append(key)
        keyed.append((s, ids))
        prev_key = k
    return norm, keyed


# -----------------------------------------------------------------------------
def brace_delta(line):
    line = RE_STRING.sub('""', line)
    return line.count('{') - line.count('}')


def is_complete(region):
    """True if the lines form complete statements (balanced braces, no
    dangling control header)."""
    depth = 0
    for raw in region:
        depth += brace_delta(raw)
        if depth < 0:
            return False
    last = region[-1] if region else ''
    if RE_CTRL_HDR.match(last) and not last.endswith(';') \
            and not last.endswith('}'):
        return False
    return depth == 0


def check_function(name, t_lines, p_lines, allow_calls, allow_assign):
    """Compare one function; return (verdict, findings, number of test-only
    statements, listing of the test-only statements)."""
    t_norm, t_keyed = normalize(t_lines)
    p_norm, p_keyed = normalize(p_lines)
    sm = difflib.SequenceMatcher(None, t_norm, p_norm, autojunk=False)
    fails = []
    reviews = []
    test_only = []          # list of regions (lists of (raw line, ids))
    label_map = {}          # test label id -> production label id
    rev_map = {}

    raw_regions = []        # (i1, i2) index ranges of test-only lines
    t_only_idx = set()      # indices of all test-only lines
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == 'equal':
            for k in range(i2 - i1):
                ti = t_keyed[i1 + k][1]
                pi = p_keyed[j1 + k][1]
                if len(ti) != len(pi):
                    fails.append('R4 label/temporary mismatch: %s'
                                 % t_keyed[i1 + k][0])
                    continue
                for a, b in zip(ti, pi):
                    if label_map.setdefault(a, b) != b or \
                            rev_map.setdefault(b, a) != a:
                        fails.append('R4 inconsistent control flow at: %s'
                                     % p_keyed[j1 + k][0])
        elif tag in ('insert', 'replace'):
            for k in range(j1, j2):
                fails.append('R1 production-only statement: %s'
                             % p_keyed[k][0])
            if tag == 'replace':
                raw_regions.append((i1, i2, False))
        elif tag == 'delete':
            raw_regions.append((i1, i2, True))

    # A test-only region next to identical lines can be aligned in several
    # equivalent ways (e.g. '}' just before or just after it). Slide each
    # pure-deletion region over identical neighbors and keep the position
    # where it forms complete statements, if there is one.
    for i1, i2, slidable in raw_regions:
        best = (i1, i2)
        if slidable:
            cands = [(i1, i2)]
            a, b = i1, i2
            while a > 0 and t_norm[a - 1] == t_norm[b - 1]:
                a, b = a - 1, b - 1
                cands.append((a, b))
            a, b = i1, i2
            while b < len(t_norm) and t_norm[a] == t_norm[b]:
                a, b = a + 1, b + 1
                cands.append((a, b))
            for c in cands:
                if is_complete([t_keyed[k][0] for k in range(*c)]):
                    best = c
                    break
        test_only.append(t_keyed[best[0]:best[1]])
        t_only_idx.update(range(best[0], best[1]))

    def lab(raw):       # raw line with labels as <Lnnn>; compiler comments
        # such as '// predicted unlikely by goto predictor.;' are not code
        if raw.startswith('//'):
            return ''
        return RE_LABEL_TOK.sub(lambda mm: '<L%s>' % mm.group(1), raw)

    # all gotos of the test variant: target label -> number of references
    all_gotos = {}
    for raw, _ in t_keyed:
        for g in RE_GOTO.findall(lab(raw)):
            all_gotos[g] = all_gotos.get(g, 0) + 1

    # locals declared in test-only code (allowed assignment targets)
    t_only_locals = set()
    for region in test_only:
        for raw, _ in region:
            name = decl_name(raw)
            if name and not RE_CTRL_HDR.match(raw):
                t_only_locals.add(name)

    # R2: a test-only declaration must not shadow a name of the production
    # code (otherwise identical-looking production statements could refer to
    # a different object in the test build)
    # The scope of a test-only declaration extends to the end of its block;
    # it shadows a production name only if production (shared) code within
    # that scope uses the same name.
    shadowed = set()
    for i in sorted(t_only_idx):
        name = decl_name(t_keyed[i][0])
        if not name or RE_CTRL_HDR.match(t_keyed[i][0]):
            continue
        depth = 0
        for j in range(i + 1, len(t_keyed)):
            raw = t_keyed[j][0]
            if raw == '{':
                depth += 1
            elif raw == '}':
                if depth == 0:
                    break           # end of the declaring block
                depth -= 1
            elif j not in t_only_idx and name in re.findall(
                    r'[A-Za-z_]\w*', RE_STRING.sub('', raw)):
                shadowed.add(name)
                break
    for name in sorted(shadowed):
        fails.append('R2 test-only declaration shadows the name "%s" used by '
                     'production code in its scope' % name)

    re_calls = re.compile(r'^(?:%s)$' % allow_calls)
    re_assign = re.compile(r'^(?:%s)$' % allow_assign)
    n_test_only = 0
    for region in test_only:
        n_test_only += len(region)
        # R2: complete statements/blocks only
        if not is_complete([raw for raw, _ in region]):
            fails.append('R2 test-only code is not a complete statement '
                         '(wraps or guards production code), starting at: %s'
                         % region[0][0])
        # R3: jumps must stay inside the same test-only block
        region_labels = set()
        region_gotos = {}
        for raw, _ in region:
            m = RE_LABEL_DEF.match(lab(raw))
            if m:
                region_labels.add(m.group(1))
            for g in RE_GOTO.findall(lab(raw)):
                region_gotos[g] = region_gotos.get(g, 0) + 1
        for l in region_labels:
            if all_gotos.get(l, 0) != region_gotos.get(l, 0):
                fails.append('R3 test-only label reached from outside its '
                             'test-only block: %s' % l.replace('<L', '<D.'))
        for raw, _ in region:
            # R3: control transfers
            if RE_CTRL_XFER.match(raw):
                fails.append('R3 test-only control transfer: %s' % raw)
            for g in RE_GOTO.findall(lab(raw)):
                if g not in region_labels:
                    fails.append('R3 test-only goto leaving its test-only '
                                 'block: %s' % raw)
            # R5: instrumentation only
            body = raw
            if RE_CTRL_HDR.match(body):
                body = body[body.find('('):] if '(' in body else ''
            for fn in RE_CALL.findall(body):
                if fn in C_KEYWORDS or fn in t_only_locals:
                    continue
                if not re_calls.match(fn):
                    reviews.append('R5 test-only call to non-allowlisted '
                                   'function "%s": %s' % (fn, raw))
            target = None
            if decl_name(raw):
                pass            # declaration of a test-only local
            else:
                m = RE_INCDEC.match(raw)
                if m:
                    target = m.group(1) or m.group(2)
                elif not RE_CTRL_HDR.match(raw):
                    target = assign_target(raw)
            if target and RE_ALLOW_CRIT.match(raw):
                target = None
            if target:
                base = re.match(r'[(*\s]*((?:[A-Za-z_]\w*::)*[A-Za-z_]\w*)',
                                target)
                base = base.group(1) if base else target
                if base not in t_only_locals and not re_assign.match(base):
                    reviews.append('R5 test-only assignment to "%s": %s'
                                   % (target, raw))

    verdict = 'FAIL' if fails else ('REVIEW' if reviews else 'PASS')
    listing = [raw for region in test_only for raw, _ in region]
    return verdict, fails + reviews, n_test_only, listing


def check_dumps(t_text, p_text, allow_calls, allow_assign):
    """Compare a test and a production dump; return (verdict, results)."""
    t_text = RE_LABEL_DECL.sub('', t_text)
    p_text = RE_LABEL_DECL.sub('', p_text)
    for text, what in ((t_text, 'test'), (p_text, 'production')):
        if '<<< error >>>' in text:
            raise ToolError('the %s build of the module did not compile '
                            '(the compiler dump contains errors); check the '
                            'compiler output above' % what)
    t_funcs, t_order = parse_dump(t_text)
    p_funcs, p_order = parse_dump(p_text)
    if not t_order and not p_order:
        raise ToolError('no functions found in either dump')
    if not p_order:
        # the whole module is instrumentation (e.g. QUTest support code)
        return 'N/A', [(fn, 'N/A', ['function exists only in the test build'
                                    ' (module has no production code)'],
                        len(t_funcs[fn]), t_funcs[fn]) for fn in t_order]
    results = []
    re_calls = re.compile(r'^(?:%s)$' % allow_calls)
    for fn in p_order:
        if fn not in t_funcs:
            results.append((fn, 'FAIL',
                            ['R1 function exists only in production build'],
                            0, []))
            continue
        v, f, n, lst = check_function(fn, t_funcs[fn], p_funcs[fn],
                                      allow_calls, allow_assign)
        results.append((fn, v, f, n, lst))
    for fn in t_order:
        if fn not in p_funcs:
            v = 'PASS' if re_calls.match(func_name(fn)) else 'REVIEW'
            results.append((fn, v, [] if v == 'PASS' else
                            ['R5 function exists only in test build'],
                            len(t_funcs[fn]), t_funcs[fn]))
    verdicts = [r[1] for r in results]
    mod = 'FAIL' if 'FAIL' in verdicts else \
          ('REVIEW' if 'REVIEW' in verdicts else 'PASS')
    return mod, results


# -----------------------------------------------------------------------------
def is_cxx(cc, cflags):
    """True if the compiler command compiles C++ (g++, c++, clang++, or a
    C++ language standard/-x c++ option)."""
    name = os.path.basename(cc).lower()
    return '++' in name or any(f.startswith(('-std=c++', '-std=gnu++'))
                               for f in cflags) or \
        ' '.join(cflags).find('-x c++') >= 0


def run_selftest(args, workroot, out):
    """Run the seeded self-test cases; return True when all verdicts match."""
    sdir = args.selftest_dir
    # the C cases (*.c) are valid C and C++ and run with any compiler (a C++
    # compiler such as g++ compiles them as C++); the C++ cases (*.cpp) run
    # only with a C++ compiler
    cases = sorted(glob.glob(os.path.join(sdir, '*.c')))
    if is_cxx(args.cc, args.cflags):
        cases += sorted(glob.glob(os.path.join(sdir, '*.cpp')))
    if not cases:
        raise ToolError('no self-test cases found in %s' % sdir)
    ok = True
    lines = []
    for case in cases:
        m = re.search(r'EXPECT:\s*(PASS|FAIL|REVIEW)', read_text(case))
        if not m:
            raise ToolError('self-test case %s has no EXPECT: line' % case)
        expect = m.group(1)
        stem = os.path.splitext(os.path.basename(case))[0]
        t_text = compile_dump(args.cc, case, args.cflags, ['Q_SPY'],
                              workroot, stem + '_test')
        p_text = compile_dump(args.cc, case, args.cflags, [],
                              workroot, stem + '_prod')
        mod, _ = check_dumps(t_text, p_text,
                             DEFAULT_ALLOW_CALLS, DEFAULT_ALLOW_ASSIGN)
        good = (mod == expect)
        ok = ok and good
        lines.append('  %-34s expected %-6s got %-6s %s'
                     % (os.path.basename(case), expect, mod,
                        'ok' if good else '*** MISMATCH ***'))
    out.append('Self-test : %d/%d cases %s'
               % (sum(1 for l in lines if l.endswith('ok')), len(cases),
                  'PASSED' if ok else 'FAILED'))
    if args.verbose or not ok:
        out.extend(lines)
    return ok


def selftest_main(argv, here):
    """Tool qualification: 'qimpact.py --selftest COMPILER [WORKDIR]'."""
    ap = argparse.ArgumentParser(
        prog=TOOL_NAME + ' --selftest',
        description='Run the seeded self-test cases with the given compiler '
                    'and base flags (tool qualification).')
    ap.add_argument('compiler', metavar='COMPILER',
                    help='compiler and base flags as one quoted argument, '
                         'exactly as used for the unit tests, '
                         'e.g. "gcc -c -g -O"')
    ap.add_argument('workdir', metavar='WORKDIR', nargs='?', default=None,
                    help='directory for the compiler outputs of the cases '
                         '(<case>_test.tree, <case>_prod.tree, *.o), kept '
                         'for inspection; default: a temporary directory, '
                         'removed afterwards')
    args = ap.parse_args(argv)
    args.verbose = True     # qualification evidence lists every case
    cmd = shlex.split(args.compiler, posix=(os.name != 'nt'))
    if not cmd:
        ap.error('COMPILER must name the compiler')
    args.cc, args.cflags = cmd[0], cmd[1:]
    out = ['%s %s -- self-test (tool qualification)'
           % (TOOL_NAME, TOOL_VERSION),
           'Date      : %s' % datetime.datetime.now().isoformat(
               timespec='seconds')]
    args.selftest_dir = os.path.join(here, 'qimpact_selftest')
    workroot = args.workdir or tempfile.mkdtemp(prefix='qimpact_')
    try:
        out.append('Tool      : %s (sha256 %s)'
                   % (os.path.abspath(__file__), sha256(__file__)))
        out.append('Compiler  : %s (%s)'
                   % (args.cc, compiler_version(args.cc)))
        out.append('Target    : %s' % compiler_target(args.cc))
        out.append('Flags     : %s' % ' '.join(args.cflags))
        out.append('Language  : %s' % ('C++ (C and C++ cases)'
                                       if is_cxx(args.cc, args.cflags)
                                       else 'C (C cases)'))
        out.append('Cases     : %s' % os.path.abspath(args.selftest_dir))
        out.append('Outputs   : %s' % (os.path.abspath(workroot)
                                       if args.workdir else
                                       '<temporary, removed>'))
        ok = run_selftest(args, workroot, out)
        status = 0 if ok else 3
        if not ok:
            out.append('The tool must not be used with this compiler and '
                       'these flags.')
    except ToolError as ex:
        out.append('TOOL ERROR: %s' % ex)
        status = 3
    finally:
        if not args.workdir:
            shutil.rmtree(workroot, ignore_errors=True)
    sys.stdout.write('\n'.join(out) + '\n')
    return status


def main(argv=None):
    here = os.path.dirname(os.path.abspath(__file__))
    if argv is None:
        argv = sys.argv[1:]
    if '--selftest' in argv:
        rest = list(argv)
        rest.remove('--selftest')
        return selftest_main(rest, here)
    ap = argparse.ArgumentParser(
        prog=TOOL_NAME,
        description='QUTest instrumentation-impact check: compare the '
                    'compiler dumps (-fdump-tree-original) of the module '
                    'under test built with and without the instrumentation. '
                    'The arguments are positional and few on purpose, so that '
                    'they pass through batch-file Python launchers, which '
                    'forward only a limited number of arguments. '
                    'Tool qualification (self-test, separate): '
                    '%s --selftest COMPILER [WORKDIR]' % TOOL_NAME)
    ap.add_argument('source', metavar='SOURCE',
                    help='source file of the module under test')
    ap.add_argument('dumps', metavar='DUMPS',
                    help='common path prefix of the two compiler dumps: '
                         'DUMPS_test.tree (test build) and DUMPS_prod.tree '
                         '(production build)')
    ap.add_argument('report', metavar='REPORT',
                    help="report file, appended to; only a one-line summary "
                         "goes to the standard output ('-' prints the full "
                         "report to the standard output instead)")
    ap.add_argument('compiler', metavar='COMPILER',
                    help='compiler and base flags of the dumps (without '
                         'includes and defines) as one quoted argument, '
                         'e.g. "gcc -c -g -O"; recorded in the report')
    ap.add_argument('defs', metavar='DEFS', nargs='?', default='',
                    help='instrumentation defines removed in the production '
                         'build, as one quoted argument (for the report only)')
    ap.add_argument('-v', '--verbose', action='store_true',
                    help='list the test-only statements of every function')
    args = ap.parse_args(argv)
    cmd = shlex.split(args.compiler, posix=(os.name != 'nt'))
    if not cmd:
        ap.error('COMPILER must name the compiler')
    args.cc, args.cflags = cmd[0], cmd[1:]
    args.test_dump = args.dumps + '_test.tree'
    args.prod_dump = args.dumps + '_prod.tree'
    args.test_defs = args.defs
    args.allow_calls = DEFAULT_ALLOW_CALLS
    args.allow_assign = DEFAULT_ALLOW_ASSIGN

    out = []
    out.append('')
    out.append('=' * 78)
    out.append('%s %s -- instrumentation-impact check'
               % (TOOL_NAME, TOOL_VERSION))
    out.append('Date      : %s' % datetime.datetime.now().isoformat(
        timespec='seconds'))
    status = 3
    try:
        out.append('Module    : %s' % (args.source or '<not found>'))
        if not args.source or not os.path.isfile(args.source):
            raise ToolError('source of the module under test not found '
                            '(check PROJECT and VPATH in the Makefile)')
        out.append('sha256    : %s' % sha256(args.source))
        out.append('Tool      : sha256 %s' % sha256(__file__))
        out.append('Compiler  : %s (%s)' % (args.cc, compiler_version(args.cc)))
        out.append('Target    : %s' % compiler_target(args.cc))
        out.append('Flags     : %s' % ' '.join(args.cflags))
        out.append('Removed   : %s' % (args.test_defs or '<not given>'))
        for d, what in ((args.test_dump, 'test'),
                        (args.prod_dump, 'production')):
            if not os.path.isfile(d):
                raise ToolError('dump file %s not found (did the '
                                'compilation fail?)' % d)
            # GCC writes the dump even when the compilation fails, and the
            # C++ front end does not always mark the errors in the dump;
            # the object file exists only after a successful compilation
            # (the Makefile deletes both before compiling)
            obj = d[:-len('.tree')] + '.o'
            if not os.path.isfile(obj):
                raise ToolError('the %s build of the module did not compile '
                                '(no object file %s); check the compiler '
                                'output above' % (what, obj))
        out.append('Self-test : not part of this run (tool qualification:'
                   ' %s --selftest, make instr_selftest)' % TOOL_NAME)
        mod, results = check_dumps(read_text(args.test_dump),
                                   read_text(args.prod_dump),
                                   args.allow_calls, args.allow_assign)
        n_same = 0
        for fn, v, findings, n, listing in results:
            if v == 'PASS' and n == 0 and not args.verbose:
                n_same += 1     # identical in both builds (C++: mostly
                continue        # inline functions of the included headers)
            out.append('  %-28s %-6s test-only statements: %d' % (fn, v, n))
            for f in findings:
                out.append('      %s' % f)
            if args.verbose:
                for raw in listing:
                    out.append('        | %s' % raw)
        if n_same:
            out.append('  (%d more function%s identical in both builds; '
                       '-v lists all)' % (n_same, '' if n_same == 1 else 's'))
        out.append('Verdict   : %s' % mod)
        status = {'PASS': 0, 'N/A': 0, 'FAIL': 1, 'REVIEW': 2}[mod]
    except ToolError as ex:
        out.append('TOOL ERROR: %s' % ex)
        out.append('Verdict   : ERROR')
        status = 3
    finally:
        out.append('=' * 78)
        text = '\n'.join(out) + '\n'
        if args.report == '-':
            sys.stdout.write(text)
        else:
            # the full report goes to the file only (the Makefile displays
            # the file); the console gets a one-line summary
            with open(args.report, 'a', encoding='utf-8') as f:
                f.write(text)
            verdict = [l for l in out if l.startswith('Verdict')]
            sys.stdout.write('%s: %s -- %s (report in %s)\n'
                             % (TOOL_NAME, args.source or '<no source>',
                                verdict[-1].split(':', 1)[1].strip()
                                if verdict else '?', args.report))
    return status


if __name__ == '__main__':
    sys.exit(main())

#!/usr/bin/env python3
# Replays an APS5_DIAG_TRACE write-tracker trace through eviction policies for the
# DriverPieces rings and scores every logged StoredOver block check and StoredAllSince block check
# against an unbounded tracker. A policy may answer "stored over" more often than the unbounded
# tracker (false positive) but never less (false negative), and may prove coverage less often but
# never more. The rule the traced build runs differs from the logged answer in 0 checks.
import struct
import sys

BLOCK = 65536
REC = struct.Struct('<IIIIQQ')


class Exact:
    def __init__(self):
        self.whole = 0
        self.pwhole = 0
        self.pieces = []
        self.ppieces = []

    def store(self, complete, b, e, g):
        ring = self.pieces if complete else self.ppieces
        ring[:] = [p for p in ring if not (p[1] >= b and p[2] <= e)]
        ring.append((g, b, e))

    def stored_over(self, b, e, g):
        if self.whole > g or self.pwhole > g:
            return True
        return any(p[0] > g and p[1] < e and b < p[2] for p in self.pieces) or any(p[0] > g and p[1] < e and b < p[2] for p in self.ppieces)

    def covers(self, b, e, g):
        if g == 0 or b >= e:
            return False
        if self.whole > g:
            return True
        return covered(self.pieces, b, e, g)


def covered(pieces, b, e, g):
    while b < e:
        c = b
        for p in pieces:
            if (p[3] if len(p) > 3 else p[0]) > g and p[1] <= b and p[2] > c:
                c = p[2]
        if c == b:
            return False
        b = c
    return True


class Ring:
    def __init__(self, policy):
        self.policy = policy
        self.slots = [None] * policy.slots
        self.dropped = 0
        self.extent = None
        self.forgotten = []

    def contained_replace(self, b, e, g):
        hit = None
        for i, p in enumerate(self.slots):
            if p is None or p[1] < b or p[2] > e:
                continue
            if hit is None:
                hit = i
            else:
                self.slots[i] = None
        if hit is not None:
            self.slots[hit] = (g, b, e, g)
            return True
        return False

    def store(self, b, e, g):
        if self.contained_replace(b, e, g):
            return
        for i, p in enumerate(self.slots):
            if p is None:
                self.slots[i] = (g, b, e, g)
                return
        self.policy.overflow(self, (g, b, e, g))

    def drop(self, piece):
        if self.policy.forgotten:
            self.forgotten.append((piece[0], piece[1], piece[2]))
            if len(self.forgotten) > self.policy.forgotten:
                self.forgotten.sort()
                a, b = self.forgotten[0], self.forgotten[1]
                self.forgotten[:2] = [(max(a[0], b[0]), min(a[1], b[1]), max(a[2], b[2]))]
            return
        self.dropped = max(self.dropped, piece[0])
        if self.extent is None:
            self.extent = (piece[1], piece[2])
        else:
            self.extent = (min(self.extent[0], piece[1]), max(self.extent[1], piece[2]))

    def dropped_over(self, b, e, g):
        if self.policy.forgotten:
            return any(f[0] > g and f[1] < e and b < f[2] for f in self.forgotten)
        if self.dropped <= g:
            return False
        if not self.policy.ranged:
            return True
        return self.extent[0] < e and b < self.extent[1]

    def over(self, b, e, g):
        return self.dropped_over(b, e, g) or any(p is not None and p[0] > g and p[1] < e and b < p[2] for p in self.slots)


class Policy:
    ranged = False

    def __init__(self, name, slots=4, forgotten=0):
        self.name = name
        self.slots = slots
        self.forgotten = forgotten
        if forgotten:
            self.ranged = True

    def overflow(self, ring, new):
        oldest = min(range(len(ring.slots)), key=lambda i: ring.slots[i][0])
        ring.drop(ring.slots[oldest])
        ring.slots[oldest] = new


class Ranged(Policy):
    ranged = True


def gap(a, b):
    lo, hi = (a, b) if a[1] <= b[1] else (b, a)
    return hi[1] - lo[2]


class Merge(Policy):
    def __init__(self, name, mode, slots=4, ranged=False):
        super().__init__(name, slots)
        self.mode = mode
        self.ranged = ranged

    def overflow(self, ring, new):
        pieces = ring.slots + [new]
        best = None
        for i in range(len(pieces)):
            for j in range(i + 1, len(pieces)):
                d = gap(pieces[i], pieces[j])
                if self.mode == 'gapped' and d <= 0:
                    continue
                if self.mode == 'generation':
                    lo, hi = sorted((pieces[i], pieces[j]))
                    key = (max(d, 0) + (lo[2] - lo[1]) * (1 if hi[0] > lo[0] else 0), d)
                elif self.mode == 'inflation':
                    a, b = pieces[i], pieces[j]
                    union = max(a[2], b[2]) - min(a[1], b[1])
                    if a[0] == b[0]:
                        cost = union - (a[2] - a[1]) - (b[2] - b[1]) + max(0, min(a[2], b[2]) - max(a[1], b[1]))
                    else:
                        newer = a if a[0] > b[0] else b
                        cost = union - (newer[2] - newer[1])
                    key = (cost, max(a[0], b[0]))
                else:
                    key = (max(d, 0), 0)
                if best is None or key < best[0]:
                    best = (key, i, j)
        if best is None:
            return Policy.overflow(self, ring, new)
        _, i, j = best
        a, b = pieces[i], pieces[j]
        contiguous = max(a[1], b[1]) <= min(a[2], b[2])
        merged = (max(a[0], b[0]), min(a[1], b[1]), max(a[2], b[2]), min(a[3], b[3]) if self.mode == 'inflation' and contiguous else 0)
        if self.mode == 'inflation':
            ring.slots[i] = merged
            if j < len(ring.slots):
                ring.slots[j] = new
            return
        rest = [p for k, p in enumerate(pieces) if k not in (i, j)]
        ring.slots = rest + [merged]


class Entry:
    def __init__(self, policy):
        self.whole = 0
        self.pwhole = 0
        self.ring = Ring(policy)
        self.pring = Ring(policy)


POLICIES = [
    Policy('current (4 slots, oldest to block-wide dropped)'),
    Ranged('dropped keeps its extent (hull)'),
    Policy('8 slots, oldest to block-wide dropped', 8),
    Merge('merge nearest pair', 'nearest'),
    Merge('merge nearest pair with a gap, else oldest to dropped', 'gapped'),
    Merge('merge cheapest pair (gap + older piece bytes)', 'generation'),
    Ranged('8 slots, dropped keeps its extent', 8),
    Merge('merge nearest pair with a gap, else oldest to dropped with its extent', 'gapped', ranged=True),
    Merge('merge the pair that raises the fewest bytes, 4 slots', 'inflation'),
    Merge('merge the pair that raises the fewest bytes, 8 slots', 'inflation', 8),
    Policy('4 slots, 4 forgotten ranges merging the two oldest', 4, 4),
    Policy('8 slots, 4 forgotten ranges merging the two oldest', 8, 4),
    Policy('8 slots, 2 forgotten ranges merging the two oldest', 8, 2),
]


def main(path):
    exact = {}
    states = [dict() for _ in POLICIES]
    over_total = over_exact = 0
    fp = [0] * len(POLICIES)
    fn = [0] * len(POLICIES)
    cov_total = cov_exact = 0
    cov_lost = [0] * len(POLICIES)
    cov_bad = [0] * len(POLICIES)
    mismatch = [0] * len(POLICIES)
    last_over = None
    markers = []
    counts = {}
    data = open(path, 'rb').read()
    for off in range(0, len(data) - REC.size + 1, REC.size):
        kind, b, e, flag, block, g = REC.unpack_from(data, off)
        counts[kind] = counts.get(kind, 0) + 1
        if kind in (1, 2):
            complete = flag == 1
            x = exact.setdefault(block, Exact())
            if kind == 2:
                if complete:
                    exact[block] = Exact()
                    exact[block].whole = g
                else:
                    x.pwhole = g
                    x.ppieces = []
            else:
                x.store(complete, b, e, g)
            for k, policy in enumerate(POLICIES):
                st = states[k].get(block)
                if st is None:
                    st = states[k][block] = Entry(policy)
                if kind == 2:
                    if complete:
                        st = states[k][block] = Entry(policy)
                        st.whole = g
                    else:
                        st.pwhole = g
                        st.pring = Ring(policy)
                else:
                    (st.ring if complete else st.pring).store(b, e, g)
        elif kind == 3:
            exact.pop(block, None)
            for s in states:
                s.pop(block, None)
        elif kind == 4:
            x = exact.get(block)
            if x is None:
                continue
            truth = x.stored_over(b, e, g)
            over_total += 1
            over_exact += truth
            answers = []
            for k in range(len(POLICIES)):
                st = states[k][block]
                a = st.whole > g or st.pwhole > g or st.ring.over(b, e, g) or st.pring.over(b, e, g)
                answers.append(a)
                fp[k] += a and not truth
                fn[k] += truth and not a
            for k in range(len(POLICIES)):
                mismatch[k] += answers[k] != (flag == 1)
            last_over = (block, b, e, g, truth, answers)
        elif kind == 5:
            markers.append((block, e, g, last_over))
        elif kind == 6:
            x = exact.get(block)
            if x is None:
                continue
            truth = x.covers(b, e, g)
            cov_total += 1
            cov_exact += truth
            for k in range(len(POLICIES)):
                st = states[k][block]
                a = g != 0 and b < e and not (st.ring.dropped_over(b, e, g) if POLICIES[k].ranged else st.ring.dropped_over(0, BLOCK, g)) and (st.whole > g or covered(list(p for p in st.ring.slots if p is not None), b, e, g))
                cov_lost[k] += truth and not a
                cov_bad[k] += a and not truth
    print(f'records {sum(counts.values())}: ' + ', '.join(f'kind {k} x{v}' for k, v in sorted(counts.items())))
    print(f'StoredOver block checks {over_total}, stored over by the unbounded tracker {over_exact}')
    print(f'StoredAllSince block checks {cov_total}, covered by the unbounded tracker {cov_exact}')
    for k, policy in enumerate(POLICIES):
        print(f'  {policy.name}: false stored-over {fp[k]}, missed stored-over {fn[k]}, coverage lost {cov_lost[k]}, coverage claimed wrongly {cov_bad[k]}, differs from the logged answer {mismatch[k]}')
    for address, size, g, last in markers:
        print(f'failing check 0x{address:x}+0x{size:x} gen {g}')
        if last is not None:
            block, b, e, lg, truth, answers = last
            print(f'  last block check: block {block} part 0x{b:x}-0x{e:x} gen {lg}, unbounded {int(truth)}, policies ' + ' '.join(str(int(a)) for a in answers))


if __name__ == '__main__':
    main(sys.argv[1])

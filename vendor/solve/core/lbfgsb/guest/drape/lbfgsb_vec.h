// lbfgsb_vec -- the vector side of the in-guest L-BFGS-B: a fixed set of
// device buffers and the Lean-emitted kernels (kernels/drape) that act on
// them, behind one interface with two backends (vec_cpu: the slangc cpp
// emits; vec_rd: the SPIR-V through rdc::Device).
//
// The driver (lbfgsb.h) never touches a vector element. It describes a
// *phase* as a list of Ops and hands it to run(): the CPU backend executes
// it at once; the GPU backend records it into ONE compute list and submits
// without waiting. The driver reads the phase's scalars (the SC buffer) with
// read() on a later tick, which is when the fence is known done (AGENTS.md
// rule 4). Every op is one kernel dispatch; AGENTS.md rule 2 (every vector
// and matrix operation of L-BFGS-B is a Lean kernel) holds by construction.
//
// Buffers are named by `Buf` and sized by (n, mcap) once, in setup(). Scalar
// results land in SC at fixed slots; dot products are df32 (hi, lo) pairs.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace lbv {

// Device buffers. Float unless noted; lengths in 4-byte words.
enum Buf : int {
	X,    // the current point / trial point            n
	G,    // its gradient (uploaded by the caller)      n
	LB,   // bounds (+-inf as +-FLT_MAX)                n
	UB,   //                                            n
	XP,   // the line search's start point              n
	GP,   // its gradient                               n
	XLO,  // the line search's best point so far         n
	GLO,  //                                            n
	D,    // the search direction                       n
	DCP,  // xcp - x (the pathological-direction reset) n
	XCP,  // the generalized Cauchy point               n
	SV,   // s = x - xp                                  n
	YV,   // y = g - gp                                  n
	SM,   // S history, column-major                    n * mcap
	YM,   // Y history                                   n * mcap
	ST,   // uint: lb_compact state                      8
	BF,   // compact BFGS factors                        4 + 4 mcap^2 + mcap
	DOTS, // lb_multi_dot products for lb_compact        4 mcap + 6
	VECC, // c = W^T (xcp - x)                           2 mcap
	WK,   // lb_cauchy / lb_subspace scratch             10 n
	ISET, // uint: Cauchy / subspace index sets          8 + 3 n
	SC,   // scalar results (slots below)                32
	VIN,  // lb_apply_m inputs (G1 only)                 (2 mcap) (2 mcap + 3)
	VOUT, //                                             (2 mcap) (2 mcap + 3)
	kNumBufs
};

// SC slots.
enum Slot : uint32_t {
	SC_DG = 0,    // g.d (hi, lo)
	SC_XX = 2,    // x.x (hi, lo)
	SC_NN = 4,    // (xcp-x).(xcp-x) (hi, lo)
	SC_PG = 6,    // ||P(x - g) - x||_inf
	SC_SMAX = 7,  // max feasible step along d (FLT_MAX: none)
	SC_SMAX0 = 8, // G1: step_max along drt0
	SC_AUX = 10,  // G1: g.drt (hi, lo)
	kScWords = 32
};

// One kernel dispatch.
struct Op {
	enum Kind {
		BoxProject, // x = min(max(x, lb), ub)                     a = x
		PgInf,      // SC[u0] = ||P(x-g)-x||_inf                   a = x, b = g
		StepMax,    // SC[u0] = max step along d                   a = x, b = d
		MultiDot,   // c[u3 + 2j..] = A[:, j] . v, j < u0 (df32)   a = A, b = v, c = dst; u0 ncols, u1 aStride, u3 dstOff
		Compact,    // lb_compact, mode u0 (0 add, 1 reset)
		RingStore,  // lb_ring_store SV, YV -> SM, YM (gated on ST[3])
		ApplyM,     // lb_apply_m VIN[u0..] -> VOUT[u1..]
		Cauchy,     // lb_cauchy
		Subspace,   // lb_subspace, maxit u0
		Saxpby,     // c = fa * a + fb * b
	};
	Kind kind;
	int a = -1, b = -1, c = -1;
	uint32_t u0 = 0, u1 = 0, u3 = 0;
	float fa = 0.0f, fb = 0.0f;
	// A saxpby whose coefficients change between phases (the trial step, the
	// normalisation): the GPU backend keeps one params block for it and
	// updates it, instead of one block per coefficient value.
	bool dyn = false;
};

inline Op box_project(Buf x) {
	Op o{ Op::BoxProject };
	o.a = x;
	return o;
}
inline Op pg_inf(Buf x, Buf g, uint32_t slot) {
	Op o{ Op::PgInf };
	o.a = x;
	o.b = g;
	o.u0 = slot;
	return o;
}
inline Op step_max(Buf x, Buf d, uint32_t slot) {
	Op o{ Op::StepMax };
	o.a = x;
	o.b = d;
	o.u0 = slot;
	return o;
}
// A^T v over `ncols` columns of stride n (aStride 0 = n), into dst at dstOff.
inline Op multi_dot(Buf A, uint32_t ncols, Buf v, Buf dst, uint32_t dstOff) {
	Op o{ Op::MultiDot };
	o.a = A;
	o.b = v;
	o.c = dst;
	o.u0 = ncols;
	o.u3 = dstOff;
	return o;
}
// One dot product a.b as a (hi, lo) pair: lb_multi_dot with one column (the
// same df32 sum as Cloth's dot_reduce, but with a destination offset, so every
// scalar of a phase lands in SC and is read back in one call).
inline Op dot(Buf a, Buf b, uint32_t slot) { return multi_dot(a, 1, b, SC, slot); }
inline Op compact(uint32_t mode) {
	Op o{ Op::Compact };
	o.u0 = mode;
	return o;
}
inline Op ring_store() { return Op{ Op::RingStore }; }
inline Op apply_m(uint32_t inOff, uint32_t outOff) {
	Op o{ Op::ApplyM };
	o.u0 = inOff;
	o.u1 = outOff;
	return o;
}
inline Op cauchy() { return Op{ Op::Cauchy }; }
inline Op subspace(uint32_t maxit) {
	Op o{ Op::Subspace };
	o.u0 = maxit;
	return o;
}
inline Op saxpby(Buf dst, Buf x, Buf y, float a, float b, bool dyn = false) {
	Op o{ Op::Saxpby };
	o.a = x;
	o.b = y;
	o.c = dst;
	o.fa = a;
	o.fb = b;
	o.dyn = dyn;
	return o;
}
// dst = src (saxpby 1*src + 0*other; `other` is any finite buffer that is
// neither, so no buffer is bound twice in one set).
inline Op copy(Buf dst, Buf src) {
	Buf other = LB;
	while (other == dst || other == src) {
		other = Buf(int(other) + 1);
	}
	return saxpby(dst, src, other, 1.0f, 0.0f);
}

// Words per buffer for (n, mcap).
inline size_t buf_words(Buf b, uint32_t n, uint32_t mc) {
	switch (b) {
		case SM:
		case YM:
			return size_t(n) * mc;
		case ST:
			return 8;
		case BF:
			return 4 + 4 * size_t(mc) * mc + mc;
		case DOTS:
			return 4 * size_t(mc) + 6;
		case VECC:
			return 2 * size_t(mc);
		case WK:
			return 10 * size_t(n);
		case ISET:
			return 8 + 3 * size_t(n);
		case SC:
			return kScWords;
		case VIN:
		case VOUT:
			return 2 * size_t(mc) * (2 * size_t(mc) + 3);
		default:
			return n;
	}
}

// The kernels cap the history at 16 pairs (lb_mv's float w[32]).
constexpr uint32_t kMaxHistory = 16;

class Vec {
public:
	virtual ~Vec() = default;
	virtual const char *name() const = 0;
	// Allocate every buffer, zero-filled, for dimension n and history mcap.
	// Reallocates (and drops cached GPU state) when (n, mcap) change.
	virtual bool setup(uint32_t n, uint32_t mcap, std::string &err) = 0;
	// Host -> buffer, `words` 4-byte words at word offset `off`. On the GPU a
	// submit still in flight is waited out first (callers upload only on a
	// later tick than the submit).
	virtual bool upload(Buf b, const void *data, size_t words, size_t off = 0) = 0;
	// Execute (cpu) or record into one compute list and submit (rd).
	virtual bool run(const std::vector<Op> &ops) = 0;
	// A submit is in flight.
	virtual bool pending() const = 0;
	virtual void sync() = 0;
	// Buffer -> host (waits out a pending submit: call on a later tick).
	virtual bool read(Buf b, void *out, size_t words, size_t off = 0) = 0;
	virtual const std::string &error() const = 0;

	uint32_t n() const { return n_; }
	uint32_t mcap() const { return mc_; }
	// Phases run and dispatches made (information for the gates).
	int64_t phases() const { return phases_; }
	int64_t dispatches() const { return dispatches_; }

protected:
	uint32_t n_ = 0, mc_ = 0;
	int64_t phases_ = 0, dispatches_ = 0;
};

} // namespace lbv

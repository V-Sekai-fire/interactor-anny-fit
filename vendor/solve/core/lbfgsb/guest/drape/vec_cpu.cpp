// SPDX-License-Identifier: Apache-2.0 OR MIT
#include "vec_cpu.h"

#include <cstring>

#include "slang-cpp-prelude.h"

// Each slangc emit puts main_0 / GlobalParams_0 at file scope under
// extern "C"; emptying the EXTERN_C macros lets every emit live in its own
// namespace in one TU (guest/avbd/avbd_cpu.cpp, tests/drape_kernels).
#undef SLANG_PRELUDE_EXTERN_C
#undef SLANG_PRELUDE_EXTERN_C_START
#undef SLANG_PRELUDE_EXTERN_C_END
#define SLANG_PRELUDE_EXTERN_C
#define SLANG_PRELUDE_EXTERN_C_START
#define SLANG_PRELUDE_EXTERN_C_END

namespace lbk_md {
#include "../../kernels/drape/cpp/lb_multi_dot_serial_emit.cpp"
}
namespace lbk_cmp {
#include "../../kernels/drape/cpp/lb_compact_emit.cpp"
}
namespace lbk_rs {
#include "../../kernels/drape/cpp/lb_ring_store_emit.cpp"
}
namespace lbk_am {
#include "../../kernels/drape/cpp/lb_apply_m_emit.cpp"
}
namespace lbk_pg {
#include "../../kernels/drape/cpp/lb_pg_inf_serial_emit.cpp"
}
namespace lbk_sm {
#include "../../kernels/drape/cpp/lb_step_max_serial_emit.cpp"
}
namespace lbk_cau {
#include "../../kernels/drape/cpp/lb_cauchy_emit.cpp"
}
namespace lbk_sub {
#include "../../kernels/drape/cpp/lb_subspace_emit.cpp"
}
namespace lbk_bp {
#include "../../kernels/drape/cpp/lb_box_project_emit.cpp"
}
namespace lbk_sx {
#include "../../kernels/drape/cpp/saxpby_emit.cpp"
}

namespace {

using ThreadFn = void (*)(ComputeThreadVaryingInput *, void *, void *);

// dispatchThreads(count): main_0_Thread runs one thread at the id we hand it.
inline void dispatch(uint32_t count, ThreadFn fn, void *gp) {
	for (uint32_t lane = 0; lane < count; ++lane) {
		ComputeThreadVaryingInput t{};
		t.groupID = uint3(0u, 0u, 0u);
		t.groupThreadID = uint3(lane, 0u, 0u);
		fn(&t, nullptr, gp);
	}
}

// Point a slang buffer view at one of our word vectors.
template <class B>
void bindw(B &b, std::vector<uint32_t> &w) {
	b.data = reinterpret_cast<decltype(b.data)>(w.data());
	b.count = w.size();
}

} // namespace

bool VecCpu::setup(uint32_t n, uint32_t mcap, std::string &err) {
	if (n == 0 || mcap == 0 || mcap > lbv::kMaxHistory) {
		err = "vec cpu: need n > 0 and 0 < m <= 16";
		return false;
	}
	n_ = n;
	mc_ = mcap;
	for (int b = 0; b < lbv::kNumBufs; ++b) {
		b_[b].assign(lbv::buf_words(lbv::Buf(b), n, mcap), 0u);
	}
	return true;
}

bool VecCpu::upload(lbv::Buf b, const void *data, size_t words, size_t off) {
	std::vector<uint32_t> &v = b_[b];
	if (off + words > v.size()) {
		err_ = "vec cpu: upload past the end of buffer " + std::to_string(int(b));
		return false;
	}
	std::memcpy(v.data() + off, data, words * 4);
	return true;
}

bool VecCpu::read(lbv::Buf b, void *out, size_t words, size_t off) {
	std::vector<uint32_t> &v = b_[b];
	if (off + words > v.size()) {
		err_ = "vec cpu: read past the end of buffer " + std::to_string(int(b));
		return false;
	}
	std::memcpy(out, v.data() + off, words * 4);
	return true;
}

bool VecCpu::run(const std::vector<lbv::Op> &ops) {
	using lbv::Op;
	auto W = [this](int b) -> std::vector<uint32_t> & { return b_[b]; };
	for (const Op &o : ops) {
		switch (o.kind) {
			case Op::BoxProject: {
				lbk_bp::LbBoxProjectParams_0 p{ n_ };
				lbk_bp::GlobalParams_0 gp{};
				gp.params_0 = &p;
				bindw(gp.x_0, W(o.a));
				bindw(gp.lb_0, W(lbv::LB));
				bindw(gp.ub_0, W(lbv::UB));
				dispatch(n_, &lbk_bp::main_0_Thread, &gp);
				break;
			}
			case Op::PgInf: {
				lbk_pg::LbPgInfParams_0 p{ n_, o.u0 };
				lbk_pg::GlobalParams_0 gp{};
				gp.params_0 = &p;
				bindw(gp.x_0, W(o.a));
				bindw(gp.g_0, W(o.b));
				bindw(gp.lb_0, W(lbv::LB));
				bindw(gp.ub_0, W(lbv::UB));
				bindw(gp.dst_0, W(lbv::SC));
				dispatch(1, &lbk_pg::main_0_Thread, &gp);
				break;
			}
			case Op::StepMax: {
				lbk_sm::LbStepMaxParams_0 p{ n_, o.u0 };
				lbk_sm::GlobalParams_0 gp{};
				gp.params_0 = &p;
				bindw(gp.x_0, W(o.a));
				bindw(gp.d_0, W(o.b));
				bindw(gp.lb_0, W(lbv::LB));
				bindw(gp.ub_0, W(lbv::UB));
				bindw(gp.dst_0, W(lbv::SC));
				dispatch(1, &lbk_sm::main_0_Thread, &gp);
				break;
			}
			case Op::MultiDot: {
				lbk_md::LbMultiDotParams_0 p{ n_, o.u0, 0u, o.u1 ? o.u1 : n_, 0u, o.u3 };
				lbk_md::GlobalParams_0 gp{};
				gp.params_0 = &p;
				bindw(gp.a_0, W(o.a));
				bindw(gp.vv_0, W(o.b));
				bindw(gp.dst_0, W(o.c));
				dispatch(o.u0, &lbk_md::main_0_Thread, &gp);
				break;
			}
			case Op::Compact: {
				lbk_cmp::LbCompactParams_0 p{ mc_, o.u0 };
				lbk_cmp::GlobalParams_0 gp{};
				gp.params_0 = &p;
				bindw(gp.dots_0, W(lbv::DOTS));
				bindw(gp.st_0, W(lbv::ST));
				bindw(gp.bf_0, W(lbv::BF));
				dispatch(1, &lbk_cmp::main_0_Thread, &gp);
				break;
			}
			case Op::RingStore: {
				lbk_rs::LbRingStoreParams_0 p{ n_ };
				lbk_rs::GlobalParams_0 gp{};
				gp.params_0 = &p;
				bindw(gp.st_0, W(lbv::ST));
				bindw(gp.s_0, W(lbv::SV));
				bindw(gp.y_0, W(lbv::YV));
				bindw(gp.S_0, W(lbv::SM));
				bindw(gp.Y_0, W(lbv::YM));
				dispatch(n_, &lbk_rs::main_0_Thread, &gp);
				break;
			}
			case Op::ApplyM: {
				lbk_am::LbApplyMParams_0 p{ mc_, o.u0, o.u1 };
				lbk_am::GlobalParams_0 gp{};
				gp.params_0 = &p;
				bindw(gp.st_0, W(lbv::ST));
				bindw(gp.bf_0, W(lbv::BF));
				bindw(gp.vin_0, W(lbv::VIN));
				bindw(gp.vout_0, W(lbv::VOUT));
				dispatch(1, &lbk_am::main_0_Thread, &gp);
				break;
			}
			case Op::Cauchy: {
				lbk_cau::LbCauchyParams_0 p{ n_, mc_ };
				lbk_cau::GlobalParams_0 gp{};
				gp.params_0 = &p;
				bindw(gp.x_0, W(lbv::X));
				bindw(gp.g_0, W(lbv::G));
				bindw(gp.lb_0, W(lbv::LB));
				bindw(gp.ub_0, W(lbv::UB));
				bindw(gp.S_0, W(lbv::SM));
				bindw(gp.Y_0, W(lbv::YM));
				bindw(gp.st_0, W(lbv::ST));
				bindw(gp.bf_0, W(lbv::BF));
				bindw(gp.xcp_0, W(lbv::XCP));
				bindw(gp.vecc_0, W(lbv::VECC));
				bindw(gp.wk_0, W(lbv::WK));
				bindw(gp.iset_0, W(lbv::ISET));
				dispatch(1, &lbk_cau::main_0_Thread, &gp);
				break;
			}
			case Op::Subspace: {
				lbk_sub::LbSubspaceParams_0 p{ n_, mc_, o.u0 };
				lbk_sub::GlobalParams_0 gp{};
				gp.params_0 = &p;
				bindw(gp.x_0, W(lbv::X));
				bindw(gp.g_0, W(lbv::G));
				bindw(gp.lb_0, W(lbv::LB));
				bindw(gp.ub_0, W(lbv::UB));
				bindw(gp.S_0, W(lbv::SM));
				bindw(gp.Y_0, W(lbv::YM));
				bindw(gp.st_0, W(lbv::ST));
				bindw(gp.bf_0, W(lbv::BF));
				bindw(gp.xcp_0, W(lbv::XCP));
				bindw(gp.vecc_0, W(lbv::VECC));
				bindw(gp.iset_0, W(lbv::ISET));
				bindw(gp.drt_0, W(lbv::D));
				bindw(gp.wk_0, W(lbv::WK));
				dispatch(1, &lbk_sub::main_0_Thread, &gp);
				break;
			}
			case Op::Saxpby: {
				lbk_sx::SaxpbyParams_0 p{ n_, o.fa, o.fb };
				lbk_sx::GlobalParams_0 gp{};
				gp.params_0 = &p;
				bindw(gp.x_0, W(o.a));
				bindw(gp.y_0, W(o.b));
				bindw(gp.dst_0, W(o.c));
				dispatch(n_, &lbk_sx::main_0_Thread, &gp);
				break;
			}
		}
		++dispatches_;
	}
	++phases_;
	return true;
}

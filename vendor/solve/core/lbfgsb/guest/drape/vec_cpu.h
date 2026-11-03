// VecCpu -- the L-BFGS-B vector backend on the CPU: the Lean-emitted kernels
// as slangc -target cpp (kernels/drape/cpp), one thread at a time, the
// guest/avbd/avbd_cpu.cpp pattern. The groupshared reductions (lb_pg_inf,
// lb_step_max, lb_multi_dot) run as their *_serial siblings, which is what
// slangc -target cpp accepts. run() executes a phase at once; nothing is
// ever pending.
// SPDX-License-Identifier: Apache-2.0 OR MIT
#pragma once

#include <vector>

#include "lbfgsb_vec.h"

class VecCpu : public lbv::Vec {
public:
	const char *name() const override { return "cpu"; }
	bool setup(uint32_t n, uint32_t mcap, std::string &err) override;
	bool upload(lbv::Buf b, const void *data, size_t words, size_t off = 0) override;
	bool run(const std::vector<lbv::Op> &ops) override;
	bool pending() const override { return false; }
	void sync() override {}
	bool read(lbv::Buf b, void *out, size_t words, size_t off = 0) override;
	const std::string &error() const override { return err_; }

private:
	std::vector<uint32_t> b_[lbv::kNumBufs];
	std::string err_;
};

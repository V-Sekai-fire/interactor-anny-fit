// VecRd -- the L-BFGS-B vector backend on the GPU: the Lean-emitted kernels
// as SPIR-V (kernels/drape/gen.sh embeds them) through rdc::Device, the only
// way this guest reaches the GPU.
//
// A phase (a list of lbv::Ops) becomes ONE compute list and one submit, and
// run() returns without waiting (AGENTS.md rule 4); the caller reads on a
// later tick, and read() waits out the fence then (it is known done).
//
// Params without push constants, the AvbdRd way: every dispatch site (kernel,
// buffers, params) has its own small uniform buffer and a cached uniform set,
// both made on first use. A site whose params change between phases (a
// "dynamic" saxpby: the trial step, the normalisation) keeps one block and
// is rewritten with buffer_update before list_begin, which is the only place
// Godot accepts it. Every returned RID is permanent (rd_compute.h) and freed
// here. A barrier follows every dispatch but the last: nearly every op of a
// phase reads what the one before wrote, and the render graph does not order
// same-buffer dispatches inside one list (AGENTS.md facts).
// SPDX-License-Identifier: Apache-2.0 OR MIT
#pragma once

#include <map>
#include <string>
#include <vector>

#include "rd_compute.h"
#include "../drape_table.h"
#include "lbfgsb_vec.h"

class VecRd : public lbv::Vec {
public:
	explicit VecRd(rdc::Device &dev);
	~VecRd() override;

	// Every kernel has a shader and a pipeline.
	bool ok() const { return ok_; }
	const char *name() const override { return "rd"; }
	bool setup(uint32_t n, uint32_t mcap, std::string &err) override;
	bool upload(lbv::Buf b, const void *data, size_t words, size_t off = 0) override;
	bool run(const std::vector<lbv::Op> &ops) override;
	bool pending() const override { return pending_; }
	void sync() override;
	bool read(lbv::Buf b, void *out, size_t words, size_t off = 0) override;
	const std::string &error() const override { return err_; }

	// Uniform sets and params blocks made so far (information).
	size_t sites() const { return sites_.size(); }

private:
	struct Kernel {
		::RID shader, pipeline;
		const cloth::drape_table::KernelDesc *desc = nullptr;
	};
	struct Site {
		::RID params, set;
		std::vector<uint32_t> words; // the params block as last written
	};
	bool load_kernels();
	void free_buffers();
	void free_sites();

	rdc::Device &d_;
	bool ok_ = false, pending_ = false;
	std::string err_;
	std::map<std::string, Kernel> kernels_;
	::RID bufs_[lbv::kNumBufs];
	size_t words_[lbv::kNumBufs] = {};
	std::map<std::string, Site> sites_;
};

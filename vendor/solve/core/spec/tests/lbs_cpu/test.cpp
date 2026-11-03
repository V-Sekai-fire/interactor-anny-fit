// SPDX-License-Identifier: MIT
// Copyright (c) 2026-present K. S. Ernest (iFire) Lee
//
// CPU host harness for the Lean-emitted LBS kernel.  Dispatches slangc's
// -target cpp output of spec/slang/lbs.slang over synthetic data and checks the
// skinning against hand-computed vertices — proof that Sinew.SlangCodegen.Lbs is
// numerically correct on the CPU deform path (the macOS / fallback path).
//
// Build + run: spec/tests/lbs_cpu/run.sh
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "lbs.gen.cpp"  // slangc -target cpp output: GlobalParams_0, lbs(), Vector<>

int main() {
	const uint32_t V = 4, J = 2;
	// 4 rest verts.
	float bind[V * 3] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 2, 3, 4};
	// weights (V*J row-major): v0->j0, v1->j1, v2->50/50, v3->j0.
	float w[V * J] = {1, 0, 0, 1, 0.5f, 0.5f, 1, 0};
	// bone (J*3 rows [R|t]): j0 = identity, t=0 ; j1 = identity R, t=(10,20,30).
	float bone[J * 3 * 4] = {1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,
	                         1, 0, 0, 10, 0, 1, 0, 20, 0, 0, 1, 30};
	float verts[V * 3] = {0};

	GlobalParams_0 gp{};
	gp.bind_0.data = reinterpret_cast<Vector<float, 3> *>(bind);
	gp.bind_0.count = V;
	gp.weights_0.data = w;
	gp.weights_0.count = V * J;
	gp.bone_0.data = reinterpret_cast<Vector<float, 4> *>(bone);
	gp.bone_0.count = J * 3;
	gp.verts_0.data = reinterpret_cast<Vector<float, 3> *>(verts);
	gp.verts_0.count = V;
	// vertex/joint counts derive from the buffer sizes (GetDimensions) — no uniforms.

	ComputeVaryingInput vi{};
	uint32_t groups = (V + 63) / 64;
	vi.startGroupID.x = 0;
	vi.startGroupID.y = 0;
	vi.startGroupID.z = 0;
	vi.endGroupID.x = groups;
	vi.endGroupID.y = 1;
	vi.endGroupID.z = 1;
	lbs(&vi, nullptr, &gp);

	// v0 = j0*(1,0,0) = (1,0,0); v1 = j1*(0,1,0) = (10,21,30);
	// v2 = 0.5*(0,0,1) + 0.5*((0,0,1)+(10,20,30)) = (5,10,16); v3 = j0*(2,3,4) = (2,3,4).
	float exp[V * 3] = {1, 0, 0, 10, 21, 30, 5, 10, 16.0f, 2, 3, 4};
	double e = 0;
	for (uint32_t i = 0; i < V * 3; i++)
		e = std::max(e, (double)std::fabs(verts[i] - exp[i]));
	for (uint32_t i = 0; i < V; i++)
		printf("v%u = (%.3f, %.3f, %.3f)  exp (%.3f, %.3f, %.3f)\n", i, verts[i * 3],
		       verts[i * 3 + 1], verts[i * 3 + 2], exp[i * 3], exp[i * 3 + 1], exp[i * 3 + 2]);
	printf("max|err| = %.3e  %s\n", e, e < 1e-5 ? "OK" : "FAIL");
	return e < 1e-5 ? 0 : 1;
}

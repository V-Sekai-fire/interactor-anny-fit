// The C++ emission of the rotation-star kernels, one translation unit each: each unit includes
// its kernel's kernels/cpp/<k>_emit.cpp and exports one C entry point. build.sh compiles this
// file once per kernel with -DRS_KERNEL=<n> and keeps only that unit's *_run symbol global, so
// both kernels link into one test without their shared prelude names colliding.
#include <cstddef>
#include <cstdint>

#define BUF(name, ptr, n) g.name##_0.data = (ptr); g.name##_0.count = (n)
#define GO(n)                                            \
	ComputeVaryingInput vi{};                            \
	vi.startGroupID = uint3(0u, 0u, 0u);                 \
	vi.endGroupID = uint3(((n) + 63u) / 64u, 1u, 1u);    \
	main_0(&vi, nullptr, &g)

#if RS_KERNEL == 1
#include "star_kabsch_emit.cpp"
extern "C" void star_kabsch_run(uint32_t N, const float *tgt, const float *src, float *rot) {
	StarKabschParams_0 p{ N };
	GlobalParams_0 g{};
	g.params_0 = &p;
	BUF(tgt, (float *)tgt, 21 * N); BUF(src, (float *)src, 21 * N); BUF(rot, rot, 9 * N);
	GO(N);
}
#elif RS_KERNEL == 2
#include "swing_twist_emit.cpp"
extern "C" void swing_twist_run(uint32_t N, const float *rot, const uint32_t *axis, float *swing,
		float *twist, float *angle) {
	SwingTwistParams_0 p{ N };
	GlobalParams_0 g{};
	g.params_0 = &p;
	BUF(rot, (float *)rot, 9 * N); BUF(axis, (uint32_t *)axis, N);
	BUF(swing, swing, 9 * N); BUF(twist, twist, 9 * N); BUF(angle, angle, N);
	GO(N);
}
#endif

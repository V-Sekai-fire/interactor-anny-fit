# garment_forms/kernels

`sdf_spline_hessian_emit.cpp` is the tricubic B-spline sampler (value,
gradient, symmetric Hessian) that `SdfSpline.cpp` calls. It reproduces the
OpenVDB fork's `SplineSampler::sampleHessian` bit for bit (0 ULP on OpenVDB's
own stencils, interactor-dress-on Gate 6a).

It is generated, not hand-written. The source of truth is
[V-Sekai-fire/interactor-dress-on](https://github.com/V-Sekai-fire/interactor-dress-on)
`lean/Fit/SdfSplineHessian.lean` (LeanSlang, `native_decide`-pinned), emitted
to `sdf_spline_hessian.slang` by `lake exe emit_fit` and compiled with
`slangc -target cpp` (slangc 2026.13.1). Regenerate it there, not here.

`slang-rt/` is slangc's C++ prelude (Slang, MIT/Apache-2.0 per the Slang
project), which the emitted file includes. Build with `-ffp-contract=off`
where bit-for-bit results matter: slangc folds each accumulation into one
expression and FMA contraction changes the last bits.

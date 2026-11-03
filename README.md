# interactor-anny-fit

The ANNY-SOMA-X body fit as a godot-sandbox guest: the phenotype and local-change sliders fitted to a character's segment lengths with the in-guest L-BFGS-B, on the CPU and on RenderingDevice.

## What it is for

`sinew-mocap/solve` holds the host-side fit (`core/lbfgsb/tests/anny_fit`) and its checks: the golden phenotype, the residual, a control, and CPU against SPIR-V. This guest runs the same fit inside Godot, with the solver's vector operations on the GPU (`VecRd`), so the whole fit is one guest job.

## Status

Skeleton only: no guest source yet. The plan is in `sinew-mocap/solve` `core/lbfgsb/tests/anny_fit` and the session notes.

## Licence

Apache-2.0 OR MIT, at your option; see LICENSE-APACHE and LICENSE-MIT.

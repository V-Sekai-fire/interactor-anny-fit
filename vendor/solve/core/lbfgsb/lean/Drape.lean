import Drape.SlangCodegen.Dsl
import Drape.SlangCodegen.Common
import Drape.SlangCodegen.LbBoxProject
import Drape.SlangCodegen.LbPgInf
import Drape.SlangCodegen.LbPgInfSerial
import Drape.SlangCodegen.LbStepMax
import Drape.SlangCodegen.LbStepMaxSerial
import Drape.SlangCodegen.LbRingStore
import Drape.SlangCodegen.LbMultiDot
import Drape.SlangCodegen.LbMultiDotSerial
import Drape.SlangCodegen.LbCompact
import Drape.SlangCodegen.LbApplyM
import Drape.SlangCodegen.LbCauchy
import Drape.SlangCodegen.LbSubspace

/-!
# `Drape` — the drape stage's Lean-emitted kernels

`Drape.SlangCodegen.*` holds the L-BFGS-B kernels of drape.elf
(interactor-dress-on plan, Cut 5): LBFGSpp's compact-form L-BFGS-B
(`LBFGSB.h`, `BFGSMat.h`, `Cauchy.h`, `SubspaceMin.h`) split into
device kernels, with M·v in Schur/Cholesky form. `lake exe emit_drape`
writes them, with the Cloth kernels the driver reuses (`saxpby`,
`dot_reduce`, `dot_reduce_serial`), for `kernels/drape/gen.sh`.

Every kernel module exports `shader`, `expected` and two `native_decide`
examples pinning `emit shader = expected` and the entry point name.
`Dsl` and `Common` are notation and shared pieces, not kernels.
-/

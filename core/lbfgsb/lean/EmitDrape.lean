import LeanSlang
import Drape
import Cloth.SlangCodegen.Saxpby
import Cloth.SlangCodegen.DotReduce
import Cloth.SlangCodegen.DotReduceSerial

/-!
# `emit_drape` — write the drape kernels as Slang

For each `Drape.SlangCodegen.*` kernel, and the Cloth kernels the
L-BFGS-B driver reuses (`saxpby`, `dot_reduce`, `dot_reduce_serial`),
writes `<outDir>/<name>.slang`. `kernels/drape/gen.sh` takes it from
there (slangc cpp + spirv, binding table, embedded SPIR-V).

    lake exe emit_drape /path/to/output/dir
-/

open LeanSlang

private def kernels : List (String × SlangShaderModule) :=
  [ ("lb_box_project",      Drape.SlangCodegen.LbBoxProject.shader)
  , ("lb_pg_inf",           Drape.SlangCodegen.LbPgInf.shader)
  , ("lb_pg_inf_serial",    Drape.SlangCodegen.LbPgInfSerial.shader)
  , ("lb_step_max",         Drape.SlangCodegen.LbStepMax.shader)
  , ("lb_step_max_serial",  Drape.SlangCodegen.LbStepMaxSerial.shader)
  , ("lb_ring_store",       Drape.SlangCodegen.LbRingStore.shader)
  , ("lb_multi_dot",        Drape.SlangCodegen.LbMultiDot.shader)
  , ("lb_multi_dot_serial", Drape.SlangCodegen.LbMultiDotSerial.shader)
  , ("lb_compact",          Drape.SlangCodegen.LbCompact.shader)
  , ("lb_apply_m",          Drape.SlangCodegen.LbApplyM.shader)
  , ("lb_cauchy",           Drape.SlangCodegen.LbCauchy.shader)
  , ("lb_subspace",         Drape.SlangCodegen.LbSubspace.shader)
  , ("saxpby",              Cloth.SlangCodegen.Saxpby.shader)
  , ("dot_reduce",          Cloth.SlangCodegen.DotReduce.shader)
  , ("dot_reduce_serial",   Cloth.SlangCodegen.DotReduceSerial.shader)
  ]

def main (args : List String) : IO UInt32 := do
  let outDir := args.headD "."
  IO.FS.createDirAll outDir
  for (name, m) in kernels do
    let path := outDir ++ "/" ++ name ++ ".slang"
    IO.FS.writeFile path (LeanSlang.emit m ++ "\n")
    IO.println s!"wrote {path}"
  return 0

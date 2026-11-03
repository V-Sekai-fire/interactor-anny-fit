-- SPDX-License-Identifier: MIT
import RotationStar

/-!
# `rotation_star_oracle` — the cases the C port and the kernels are tested against

    lake exe rotation_star_oracle <file>

One case per line, every float as the 16 hex digits of its IEEE-754 binary64 bits, so the C
test reads exactly the doubles Lean computed:

    K <21 tgt> <21 src> <9 R>                      starKabsch tgt src = R (row-major)
    S <9 R> <ax> <9 swing> <9 twist> <angle>        swingTwist R ax
    A <n> <3n tgt> <3n src> <9 R>                   Align.align (sinew's recipe)

Inputs come from a fixed 64-bit LCG (Knuth's MMIX constants), so the file is reproducible and
`tests/oracle/gen.sh --check` can compare it with the committed copy.
-/

open RotationStar

def hex16 (x : Float) : String :=
  let s := String.ofList (Nat.toDigits 16 x.toBits.toNat)
  "".pushn '0' (16 - s.length) ++ s

def fv (v : V3) : List String := [hex16 v.x, hex16 v.y, hex16 v.z]
def fm (m : M3) : List String := fv m.r0 ++ fv m.r1 ++ fv m.r2
def fstar (s : Star) : List String := s.points.toList.flatMap fv

/-- MMIX LCG state → next state and a float in [−1, 1) from the top 53 bits. -/
def lcg (s : UInt64) : UInt64 × Float :=
  let s' := s * 6364136223846793005 + 1442695040888963407
  (s', (s' >>> 11).toFloat / 4503599627370496.0 - 1.0)

def rv (s : UInt64) : UInt64 × V3 :=
  let (s, a) := lcg s; let (s, b) := lcg s; let (s, c) := lcg s
  (s, ⟨a, b, c⟩)

/-- A random frame from the 6D pair, retried until the pair is well conditioned. -/
partial def rframe (s : UInt64) : UInt64 × M3 :=
  let (s, a) := rv s
  let (s, b) := rv s
  if a.norm > 0.2 && b.norm > 0.2 && (a.cross b).norm > 0.2 * a.norm * b.norm then
    (s, M3.of6D a b)
  else rframe s

def kline (tgt src : Star) : String :=
  " ".intercalate (["K"] ++ fstar tgt ++ fstar src ++ fm (starKabsch tgt src))

def sline (r : M3) (ax : Nat) : String :=
  let (r0, t, ang) := swingTwist r ax
  " ".intercalate (["S"] ++ fm r ++ [toString ax] ++ fm r0 ++ fm t ++ [hex16 ang])

def aline (pairs : Array (V3 × V3)) : String :=
  let r := M3.ofArr (Align.align pairs)
  " ".intercalate (["A", toString pairs.size] ++ pairs.toList.flatMap (fun p => fv p.1) ++
    pairs.toList.flatMap (fun p => fv p.2) ++ fm r)

/-- Rotation by `ang` about the unit axis `k` (Rodrigues), for the split's special cases. -/
def rotAbout (k : V3) (ang : Float) : M3 :=
  let c := Float.cos ang; let s := Float.sin ang
  M3.ofCols (rod k c s V3.ex) (rod k c s V3.ey) (rod k c s V3.ez)

def lines : List String := Id.run do
  let mut out : List String := []
  let mut s : UInt64 := 20261025
  -- The 24 signed-permutation frames, exact.
  for r in Spec.signedPerms do
    out := out ++ [kline (star r ⟨3, -2, 5⟩) (star M3.id ⟨-1, 4, 2⟩)]
  -- Random frames: plain, against a second frame, with noise, rank 1, rank 2, mirrored.
  for _ in [0:32] do
    let (s1, r) := rframe s
    let (s2, q) := rframe s1
    let (s3, oa) := rv s2
    let (s4, ob) := rv s3
    s := s4
    let ta := star r (oa.scale 10.0)
    let sb := star M3.id (ob.scale 10.0)
    out := out ++ [kline ta sb, kline ta (star q (ob.scale 10.0))]
    -- noise up to 0.02 on every target point
    let mut pts := ta.points
    for k in [0:7] do
      let (s5, n) := rv s
      s := s5
      pts := pts.set! k (pts[k]!.add (n.scale 0.02))
    out := out ++ [kline (Star.ofPoints pts) sb]
    let o := ob.scale 10.0
    out := out ++ [kline ta { sb with py := o, ny := o, pz := o, nz := o }]
    out := out ++ [kline ta { sb with pz := o, nz := o }]
    -- A mirrored target (the reflection fix). Its arms get distinct lengths 1, 0.6, 0.3: with
    -- equal ones every axis is an equally good flip and the nearest rotation is not unique.
    let mir := star (M3.ofCols r.c0 (r.c1.scale 0.6) (r.c2.scale (-0.3))) (oa.scale 10.0)
    out := out ++ [kline mir sb]
  -- Rank 1 with the source arm and the target arm aligned and antipodal (the fallbacks).
  let o : V3 := ⟨1, 2, 3⟩
  let deg : Star := { star M3.id o with py := o, ny := o, pz := o, nz := o }
  out := out ++ [kline (star M3.id o) deg,
                 kline (star (rotAbout V3.ez 3.141592653589793) o) deg]
  -- The split: random frames about each axis, a pure twist, a half-turn swing (antipodal).
  for _ in [0:32] do
    let (s1, r) := rframe s
    s := s1
    for ax in [0, 1, 2] do
      out := out ++ [sline r ax]
  for ax in [0, 1, 2] do
    out := out ++ [sline (rotAbout (V3.axisUnit ax) 0.7) ax,
                   sline (rotAbout (V3.axisUnit ((ax + 1) % 3)) 3.141592653589793) ax,
                   sline M3.id ax]
  -- sinew's recipe: AlignTest.lean's 120° turn (the axis cycle x → y → z → x) at N = 5, 2, 1,
  -- and random frames at N = 5.
  let cyc := M3.ofCols V3.ey V3.ez V3.ex
  let srcs : Array V3 := #[⟨1,0,0⟩, ⟨0,1,0⟩, ⟨0,0,1⟩, ⟨1,1,0⟩, ⟨0.3,-0.7,0.5⟩]
  let pairsOf (r : M3) (b : Array V3) := b.map fun v => (r.mul v, v)
  out := out ++ [aline (pairsOf cyc srcs), aline (pairsOf cyc (srcs.extract 0 2)),
                 aline (pairsOf cyc #[⟨0,1,0⟩])]
  for _ in [0:16] do
    let (s1, r) := rframe s
    s := s1
    out := out ++ [aline (pairsOf r srcs)]
  return out

def main (args : List String) : IO UInt32 := do
  let path := args.headD "oracle.txt"
  IO.FS.writeFile path ("\n".intercalate lines ++ "\n")
  IO.println s!"wrote {path} ({lines.length} cases)"
  return 0

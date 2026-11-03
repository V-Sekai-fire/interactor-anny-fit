-- SPDX-License-Identifier: MIT
import Plausible
import RotationStar.Props

/-!
# `rotation_star_test` — the property checks and their planted controls

Every positive property must survive the generated cases; every control must be refuted.
Fixed seed, so a run is reproducible. Exit 1 on any wrong outcome.

    lake exe rotation_star_test
-/

open Plausible RotationStar.Props

def cfg : Configuration := { numInst := 3000, randomSeed := some 20261025, quiet := true }

def refuted {p : Prop} : TestResult p → Bool
  | .failure .. => true
  | _ => false

def held {p : Prop} : TestResult p → Bool
  | .success _ => true
  | _ => false

def report (name : String) (wantHold : Bool) (ok : Bool) : IO Bool := do
  let what := if wantHold then "holds" else "control refuted"
  IO.println s!"{if ok then "PASS" else "FAIL"} {name} ({what})"
  return ok

def main : IO UInt32 := do
  let mut ok := true
  ok := (← report "canonical completion: rotation, maps v0 to u0, no twist" true
    (held (← Testable.checkIO (NamedBinder "t" <| ∀ t : TCase, canonOK t = true) cfg))) && ok
  ok := (← report "any-perpendicular completion has no twist" false
    (refuted (← Testable.checkIO (NamedBinder "t" <| ∀ t : TCase, anyHasMinimalTwist t = true) cfg))) && ok
  ok := (← report "star of R recovers R" true
    (held (← Testable.checkIO (NamedBinder "c" <| ∀ c : RCase, starRecovers c = true) cfg))) && ok
  ok := (← report "two stars give R Qᵀ" true
    (held (← Testable.checkIO (NamedBinder "c" <| ∀ c : RCase, starRelative c = true) cfg))) && ok
  ok := (← report "perturbed star recovers R exactly" false
    (refuted (← Testable.checkIO (NamedBinder "c" <| ∀ c : RCase, perturbedStarRecovers c = true) cfg))) && ok
  ok := (← report "rank-1 star: shortest arc, no twist" true
    (held (← Testable.checkIO (NamedBinder "c" <| ∀ c : RCase, rank1Minimal c = true) cfg))) && ok
  ok := (← report "rank-1 with any-perpendicular completion has no twist" false
    (refuted (← Testable.checkIO (NamedBinder "c" <| ∀ c : RCase, rank1AnyMinimal c = true) cfg))) && ok
  ok := (← report "swing-twist split recovers swing, twist, angle" true
    (held (← Testable.checkIO (NamedBinder "c" <| ∀ c : RCase, splitRecovers c = true) cfg))) && ok
  ok := (← report "split with the wrong swing is a pure twist" false
    (refuted (← Testable.checkIO (NamedBinder "c" <| ∀ c : RCase, splitAnyPure c = true) cfg))) && ok
  ok := (← report s!"one-sided Jacobi settles within {RotationStar.sweeps} sweeps" true
    (held (← Testable.checkIO (NamedBinder "c" <| ∀ c : RCase, svdConverged c = true) cfg))) && ok
  ok := (← report "one sweep is enough" false
    (refuted (← Testable.checkIO (NamedBinder "c" <| ∀ c : RCase, svdOneSweep c = true) cfg))) && ok
  ok := (← report "sinew align recovers R from 5 pairs" true
    (held (← Testable.checkIO (NamedBinder "c" <| ∀ c : RCase, alignRecovers c = true) cfg))) && ok
  IO.println (if ok then "OK" else "FAILED")
  return if ok then 0 else 1

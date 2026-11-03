-- SPDX-License-Identifier: MIT
import LeanSlang

/-!
# `RotationStar.Slang.Dsl` — terse constructors for LeanSlang trees

The subset of lbfgsb's `Drape.SlangCodegen.Dsl` and contract-anny-kernels'
`Anny.SlangCodegen.Common` the rotation kernels use, copied so this library needs only
LeanSlang. Every helper expands to LeanSlang constructors and nothing here changes what
`LeanSlang.emit` prints, so a generator written with these helpers emits the same text as the
same generator written with the originals (the anny kernels' pins check exactly that).

A 3×3 matrix is nine scalar locals `<pfx>0 … <pfx>8` (row-major) and a 3-vector three locals
`<pfx>0 … <pfx>2`; every loop over components runs in Lean, so the Slang is straight-line code.
-/

namespace RotationStar.Slang.Dsl

open LeanSlang

abbrev E := SlangExpr
abbrev St := SlangStmt

def fT : SlangType := .scalar .float
def uT : SlangType := .scalar .uint
def bT : SlangType := .scalar .bool
def u3T : SlangType := .vec .uint 3

scoped instance : HAdd E E E := ⟨fun a b => .bin "+" a b⟩
scoped instance : HSub E E E := ⟨fun a b => .bin "-" a b⟩
scoped instance : HMul E E E := ⟨fun a b => .bin "*" a b⟩
scoped instance : HDiv E E E := ⟨fun a b => .bin "/" a b⟩
scoped instance : Neg E := ⟨fun a => .un "-" a⟩

def v (s : String) : E := .var s
/-- A float literal printed with six decimals; keep to values that prints exactly. -/
def fl (x : Float) : E := .litFloat x
/-- A float literal printed exactly (shortest round-tripping binary32). -/
def flx (x : Float) : E := .litFloatExact x
def u (n : Nat) : E := .litUint n
def at_ (buf : String) (i : E) : E := .index (.var buf) i
def p (field : String) : E := .member (.var "params") field
def call (f : String) (args : List E) : E := .call f args
def sel (c t f : E) : E := .ternary c t f

def lt (a b : E) : E := .bin "<" a b
def le (a b : E) : E := .bin "<=" a b
def gt (a b : E) : E := .bin ">" a b
def ge (a b : E) : E := .bin ">=" a b
def eq (a b : E) : E := .bin "==" a b
def and_ (a b : E) : E := .bin "&&" a b
def fmax (a b : E) : E := .call "max" [a, b]
def fabs (a : E) : E := .call "abs" [a]

def let_ (ty : SlangType) (n : String) (e : E) : St := .declare ty n (some e)
def setv (n : String) (r : E) : St := .assign (.var n) r
def setAt (buf : String) (i r : E) : St := .assign (.index (.var buf) i) r
def if_ (c : E) (t : List St) (e : List St := []) : St := .ifThen c t e
def for_ (n : String) (lo hi : E) (body : List St) : St := .forCount n lo hi body
def ret : St := .ret none

def fld (n : String) (ty : SlangType) : SlangBinding :=
  ⟨n, ty, Semantic.none, none, none, .qIn⟩
def glob (n : String) (ty : SlangType) (b : Nat) : SlangBinding :=
  ⟨n, ty, Semantic.none, some b, some 0, .qIn⟩
def roF (n : String) (b : Nat) : SlangBinding := glob n (.roBuf fT) b
def rwF (n : String) (b : Nat) : SlangBinding := glob n (.rwBuf fT) b
def roU (n : String) (b : Nat) : SlangBinding := glob n (.roBuf uT) b
def paramsCB (structName : String) : SlangBinding := glob "params" (.const structName) 0

/-- A compute entry point `main`. -/
def entry (tx : Nat) (params : List SlangBinding) (body : List St) : SlangFunctionDecl :=
  { attrs := [.shaderCompute, .numthreads tx 1 1], name := "main", params := params, body := body }
def dtid : SlangBinding := ⟨"tid", u3T, .svDispatchThreadId, none, none, .qIn⟩

/-- The scalar `<pfx><k>`. -/
def s (pfx : String) (k : Nat) : E := v (pfx ++ toString k)
/-- Row i, column j of the 3×3 matrix `pfx`. -/
def m (pfx : String) (i j : Nat) : E := s pfx (3 * i + j)

def r3 : List Nat := [0, 1, 2]
def r9 : List Nat := List.range 9

/-- Declare the 3-vector `pfx` with components `f k`. -/
def vec (pfx : String) (f : Nat → E) : List St := r3.map fun k => let_ fT (pfx ++ toString k) (f k)
/-- Declare the 3×3 matrix `pfx` with entries `f i j`. -/
def mat (pfx : String) (f : Nat → Nat → E) : List St :=
  r9.map fun k => let_ fT (pfx ++ toString k) (f (k / 3) (k % 3))
/-- `c = aᵀ · b`. -/
def matTMul (c a b : String) : List St :=
  mat c fun i j => m a 0 i * m b 0 j + m a 1 i * m b 1 j + m a 2 i * m b 2 j
/-- `a · b` of two 3-vectors. -/
def dot3 (a b : String) : E := s a 0 * s b 0 + s a 1 * s b 1 + s a 2 * s b 2
/-- `c = a × b`. -/
def cross (c a b : String) : List St :=
  vec c fun i =>
    let j := (i + 1) % 3
    let k := (i + 2) % 3
    s a j * s b k - s a k * s b j

/-- Component `i` of the unit axis `ax` (a runtime uint). -/
def axisUnit (ax : E) (i : Nat) : E := sel (eq ax (u i)) (fl 1.0) (fl 0.0)

/-- The 3×3 at `buf[base … base+8]` (row-major) into `pfx`. -/
def load9 (pfx buf : String) (base : E) : List St :=
  mat pfx fun i j => at_ buf (base + u (3 * i + j))
/-- Write the 3×3 `r` to `buf[base … base+8]`. -/
def store9 (buf : String) (base : E) (r : String) : List St :=
  r9.map fun k => setAt buf (base + u k) (s r k)

end RotationStar.Slang.Dsl

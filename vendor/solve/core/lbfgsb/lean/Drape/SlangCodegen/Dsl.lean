import LeanSlang

/-!
# `Drape.SlangCodegen.Dsl` — terse constructors for LeanSlang trees

The L-BFGS-B kernels are long single-thread programs (breakpoint sort,
Cauchy sweep, primal-dual subspace loop). Written with raw
`SlangExpr`/`SlangStmt` constructors they would be unreadable, so this
module adds only notation: every helper below expands to exactly one
LeanSlang constructor, and nothing here changes what `LeanSlang.emit`
prints. The `native_decide` pins in each kernel module are therefore
still pins of the emitted text, not of this notation.

Operators build `.bin` nodes, so `a + b * c` in Lean is the tree
`(a + (b * c))` and emits with explicit parentheses.
-/

namespace Drape.SlangCodegen.Dsl

open LeanSlang

abbrev E := SlangExpr
abbrev St := SlangStmt

def fT : SlangType := .scalar .float
def uT : SlangType := .scalar .uint
def iT : SlangType := .scalar .int
def bT : SlangType := .scalar .bool
def u3T : SlangType := .vec .uint 3

scoped instance : HAdd E E E := ⟨fun a b => .bin "+" a b⟩
scoped instance : HSub E E E := ⟨fun a b => .bin "-" a b⟩
scoped instance : HMul E E E := ⟨fun a b => .bin "*" a b⟩
scoped instance : HDiv E E E := ⟨fun a b => .bin "/" a b⟩
scoped instance : HMod E E E := ⟨fun a b => .bin "%" a b⟩
scoped instance : Neg E := ⟨fun a => .un "-" a⟩

/-- A variable. -/
def v (s : String) : E := .var s
/-- A float literal. Keep to values `toString` prints exactly (it uses
    six decimals); small or huge constants go through `bits`. -/
def fl (x : Float) : E := .litFloat x
/-- A uint literal. -/
def u (n : Nat) : E := .litUint n
/-- `asfloat(<bits>u)`: an exact float constant from its IEEE bits. -/
def bits (n : Nat) : E := .call "asfloat" [.litUint n]
/-- `a[i]`. -/
def ix (a i : E) : E := .index a i
/-- `buf[i]` for a named buffer or array. -/
def at_ (buf : String) (i : E) : E := .index (.var buf) i
/-- `params.<field>`. -/
def p (field : String) : E := .member (.var "params") field
def call (f : String) (args : List E) : E := .call f args
def sel (c t f : E) : E := .ternary c t f
def toF (e : E) : E := .cast fT e
def toU (e : E) : E := .cast uT e

def lt (a b : E) : E := .bin "<" a b
def le (a b : E) : E := .bin "<=" a b
def gt (a b : E) : E := .bin ">" a b
def ge (a b : E) : E := .bin ">=" a b
def eq (a b : E) : E := .bin "==" a b
def ne (a b : E) : E := .bin "!=" a b
def and_ (a b : E) : E := .bin "&&" a b
def or_ (a b : E) : E := .bin "||" a b
def not_ (a : E) : E := .un "!" a
def fmin (a b : E) : E := .call "min" [a, b]
def fmax (a b : E) : E := .call "max" [a, b]
def fabs (a : E) : E := .call "abs" [a]

/-- FLT_MAX: the "no bound / no breakpoint" value (no `inf` literal). -/
def fltMax : E := bits 2139095039
/-- FLT_EPSILON = 2^-23. -/
def fltEps : E := bits 872415232
/-- DBL_EPSILON = 2^-52, exact in float32 (0x25800000): LBFGSpp's
`numeric_limits<double>::epsilon()` where it is an ABSOLUTE guard (the
Cauchy step's fpp, the subspace fallbacks' g.d). As a guard against a
value being numerically zero it must keep LBFGSpp's scale: FLT_EPSILON there
fired on the sphere demo's first iteration (|g| 1.2e-5, fpp = g.g 1.4e-10),
shrank the Cauchy step 8000 times and left xcp = x. -/
def dblEps : E := bits 629145600
/-- 1e30 (0x7149F2CA): a bound at or beyond this magnitude is absent. -/
def unbounded : E := bits 1900671690

def let_ (ty : SlangType) (n : String) (e : E) : St := .declare ty n (some e)
def decl (ty : SlangType) (n : String) : St := .declare ty n none
def arr (ty : SlangType) (n : String) (size : Nat) : St := .declareArray ty n size
def set (l r : E) : St := .assign l r
def setv (n : String) (r : E) : St := .assign (.var n) r
def setAt (buf : String) (i r : E) : St := .assign (.index (.var buf) i) r
def do_ (e : E) : St := .expr e
def if_ (c : E) (t : List St) (e : List St := []) : St := .ifThen c t e
def for_ (n : String) (lo hi : E) (body : List St) : St := .forCount n lo hi body
def while_ (c : E) (body : List St) : St := .whileLoop c body
def ret : St := .ret none

/-- A `ConstantBuffer` params struct field. -/
def fld (n : String) (ty : SlangType) : SlangBinding :=
  ⟨n, ty, Semantic.none, none, none, .qIn⟩
/-- A function parameter. -/
def arg (n : String) (ty : SlangType) (q : ParamQualifier := .qIn) : SlangBinding :=
  ⟨n, ty, Semantic.none, none, none, q⟩
/-- A global at `[[vk::binding(b, 0)]]`. -/
def glob (n : String) (ty : SlangType) (b : Nat) : SlangBinding :=
  ⟨n, ty, Semantic.none, some b, some 0, .qIn⟩
def roF (n : String) (b : Nat) : SlangBinding := glob n (.roBuf fT) b
def rwF (n : String) (b : Nat) : SlangBinding := glob n (.rwBuf fT) b
def roU (n : String) (b : Nat) : SlangBinding := glob n (.roBuf uT) b
def rwU (n : String) (b : Nat) : SlangBinding := glob n (.rwBuf uT) b
def paramsCB (structName : String) : SlangBinding := glob "params" (.const structName) 0

/-- A compute entry point `main`. -/
def entry (tx : Nat) (params : List SlangBinding) (body : List St) : SlangFunctionDecl :=
  { attrs := [.shaderCompute, .numthreads tx 1 1], name := "main", params := params, body := body }
def dtid : SlangBinding := ⟨"tid", u3T, .svDispatchThreadId, none, none, .qIn⟩
def gtid : SlangBinding := ⟨"gtid", u3T, .svGroupThreadId, none, none, .qIn⟩
def gid : SlangBinding := ⟨"gid", u3T, .svGroupId, none, none, .qIn⟩

end Drape.SlangCodegen.Dsl

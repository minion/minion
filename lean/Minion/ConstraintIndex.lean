import Minion.Semantics

/-! A binary index whose lookup remains logarithmic under kernel reduction.
    Unlike runtime arrays, its logical representation is itself a branching tree. -/
namespace Minion.ConstraintIndex

inductive Index (α : Type) where
  | empty
  | node (value : Option α) (left right : Index α)
  deriving Repr

/-- Diagnostic height; the empty index has height zero. -/
def height : Index α → Nat
  | .empty => 0
  | .node _ left right => 1 + max (height left) (height right)

def address (i : Nat) : List Bool :=
  if i = 0 then [] else (i % 2 == 1) :: address (i / 2)
termination_by i
decreasing_by exact Nat.div_lt_self (by omega) (by omega)

private def decode : List Bool → Nat
  | [] => 0
  | b :: bs => (if b then 1 else 0) + 2 * decode bs

private theorem decode_address (i : Nat) : decode (address i) = i := by
  induction i using Nat.strongRecOn with
  | ind i ih =>
    rw [address]
    split
    · rename_i h; subst i; rfl
    · rename_i h
      have hr := ih (i / 2) (Nat.div_lt_self (by omega) (by omega))
      simp only [decode, hr]
      split <;> simp_all only [beq_iff_eq] <;> omega

private theorem address_inj (i j : Nat) : address i = address j ↔ i = j := by
  constructor
  · intro h
    simpa only [decode_address] using congrArg decode h
  · exact congrArg address

def read : Index α → List Bool → Option α
  | .empty, _ => none
  | .node value _ _, [] => value
  | .node _ left right, b :: bs => read (if b then right else left) bs

def write (t : Index α) (p : List Bool) (value : α) : Index α :=
  match p, t with
  | [], .empty => .node (some value) .empty .empty
  | [], .node _ left right => .node (some value) left right
  | b :: bs, .empty =>
    if b then .node none .empty (write .empty bs value)
    else .node none (write .empty bs value) .empty
  | b :: bs, .node old left right =>
    if b then .node old left (write right bs value)
    else .node old (write left bs value) right

theorem read_write (t : Index α) (p q : List Bool) (value : α) :
    read (write t p value) q = if p = q then some value else read t q := by
  induction p generalizing t q with
  | nil => cases t <;> cases q <;> simp [write, read]
  | cons b bs ih =>
    cases t <;> cases q with
    | nil => cases b <;> simp [write, read]
    | cons c cs => cases b <;> cases c <;> simp [write, read, ih]

def get (t : Index α) (i : Nat) : Option α := read t (address i)
def set (t : Index α) (i : Nat) (value : α) : Index α := write t (address i) value

theorem get_set (t : Index α) (i j : Nat) (value : α) :
    get (set t i value) j = if i = j then some value else get t j := by
  simp only [get, set, read_write, address_inj]

def buildFrom (start : Nat) : List α → Index α
  | [] => .empty
  | x :: xs => set (buildFrom (start + 1) xs) start x

theorem get_buildFrom (xs : List α) (start i : Nat) :
    get (buildFrom start xs) i = if start ≤ i then xs[i - start]? else none := by
  induction xs generalizing start with
  | nil => simp [buildFrom, get, read]
  | cons x xs ih =>
    rw [buildFrom, get_set, ih]
    by_cases he : start = i
    · subst i; simp
    · by_cases h : start ≤ i
      · have hnext : start + 1 ≤ i := by omega
        have hd : i - start = (i - (start + 1)) + 1 := by omega
        simp [he, h, hnext, hd]
      · have hnext : ¬start + 1 ≤ i := by omega
        simp [he, h, hnext]

def build (xs : List α) : Index α := buildFrom 0 xs

theorem get_build (xs : List α) (i : Nat) : get (build xs) i = xs[i]? := by
  simp [build, get_buildFrom]

end Minion.ConstraintIndex

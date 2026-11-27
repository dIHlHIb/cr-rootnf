/- SPDX-License-Identifier: MIT
   Copyright (c) 2026 dIHlHIb
   Developed with AI assistance. -/
import Std

namespace RootInterval

structure Fraction where
  num : Nat
  den : Nat
  positive : 0 < den

def le (a b : Fraction) := a.num * b.den ≤ b.num * a.den
def lt (a b : Fraction) := a.num * b.den < b.num * a.den
def mul (a b : Fraction) : Fraction :=
  ⟨a.num * b.num, a.den * b.den, Nat.mul_pos a.positive b.positive⟩

theorem trans {a b c : Fraction} (h : le a b) (j : le b c) : le a c := by
  apply Nat.le_of_mul_le_mul_right (c := b.den) _ b.positive
  calc
    (a.num*c.den)*b.den = (a.num*b.den)*c.den := by ac_rfl
    _ ≤ (b.num*a.den)*c.den := Nat.mul_le_mul h (Nat.le_refl _)
    _ = (b.num*c.den)*a.den := by ac_rfl
    _ ≤ (c.num*b.den)*a.den := Nat.mul_le_mul j (Nat.le_refl _)
    _ = (c.num*a.den)*b.den := by ac_rfl

theorem mul_mono {a b c d : Fraction} (h : le a b) (j : le c d) :
    le (mul a c) (mul b d) := by
  unfold le at h j ⊢
  simpa only [mul, Nat.mul_assoc, Nat.mul_left_comm, Nat.mul_comm] using Nat.mul_le_mul h j

def down (q : Fraction) (d : Nat) : Fraction := {q with num := d*(q.num/d)}
def up (q : Fraction) (d : Nat) : Fraction :=
  {q with num := d*(q.num/d) + (if q.num%d = 0 then 0 else d)}

theorem down_le (q : Fraction) (d : Nat) : le (down q d) q := by
  apply Nat.mul_le_mul _ (Nat.le_refl _)
  simpa [down, Nat.mul_comm] using Nat.div_mul_le_self q.num d

theorem le_up (q : Fraction) {d : Nat} (hd : d > 0) : le q (up q d) := by
  apply Nat.mul_le_mul _ (Nat.le_refl _)
  have sum := Nat.div_add_mod q.num d
  have rem := Nat.mod_lt q.num hd
  simp only [up]
  split <;> omega

structure Interval where
  lo : Fraction
  hi : Fraction

def covers (I : Interval) (x : Fraction) := le I.lo x ∧ le x I.hi

-- The two endpoints may be rounded at different precisions and in different binades.
def product (unit : Fraction → Nat) (a b : Interval) : Interval :=
  let lo := mul a.lo b.lo
  let hi := mul a.hi b.hi
  ⟨down lo (unit lo + 1), up hi (unit hi + 1)⟩

theorem product_sound {A B : Interval} {x y : Fraction} (unit : Fraction → Nat)
    (hx : covers A x) (hy : covers B y) : covers (product unit A B) (mul x y) :=
  ⟨trans (down_le _ _) (mul_mono hx.1 hy.1),
   trans (mul_mono hx.2 hy.2) (le_up _ (Nat.zero_lt_succ _))⟩

def degree : List Bool → Nat
  | [] => 0
  | bit :: rest => (if bit then 1 else 0) + 2 * degree rest

def run (unit : Fraction → Nat) : List Bool → Interval → Interval → Interval
  | [], acc, _ => acc
  | bit :: rest, acc, base =>
    run unit rest (if bit then product unit acc base else acc) (product unit base base)

def value (acc base : Fraction) (n : Nat) : Fraction :=
  ⟨acc.num * base.num^n, acc.den * base.den^n, Nat.mul_pos acc.positive (Nat.pow_pos base.positive)⟩

theorem run_sound (unit : Fraction → Nat) (bits : List Bool) (A B : Interval)
    (a b : Fraction) (ha : covers A a) (hb : covers B b) :
    covers (run unit bits A B) (value a b (degree bits)) := by
  induction bits generalizing A B a b with
  | nil => simpa [run, degree, value] using ha
  | cons bit rest ih =>
    have sq := product_sound unit hb hb
    cases bit with
    | false =>
      have h := ih A (product unit B B) a (mul b b) ha sq
      simpa [run, degree, value, mul, Nat.pow_mul, Nat.pow_succ] using h
    | true =>
      have h := ih (product unit A B) (product unit B B) (mul a b) (mul b b)
                    (product_sound unit ha hb) sq
      simpa [run, degree, value, mul, Nat.pow_add, Nat.pow_mul, Nat.pow_succ, Nat.mul_assoc] using h

theorem strict_after {a b c : Fraction} (hab : le a b) (hbc : lt b c) : lt a c := by
  apply Nat.lt_of_mul_lt_mul_right (a := b.den)
  calc
    (a.num*c.den)*b.den = (a.num*b.den)*c.den := by ac_rfl
    _ ≤ (b.num*a.den)*c.den := Nat.mul_le_mul hab (Nat.le_refl _)
    _ = (b.num*c.den)*a.den := by ac_rfl
    _ < (c.num*b.den)*a.den := Nat.mul_lt_mul_of_pos_right hbc a.positive
    _ = (c.num*a.den)*b.den := by ac_rfl

theorem strict_before {a b c : Fraction} (hab : lt a b) (hbc : le b c) : lt a c := by
  apply Nat.lt_of_mul_lt_mul_right (a := b.den)
  calc
    (a.num*c.den)*b.den = (a.num*b.den)*c.den := by ac_rfl
    _ < (b.num*a.den)*c.den := Nat.mul_lt_mul_of_pos_right hab c.positive
    _ = (b.num*c.den)*a.den := by ac_rfl
    _ ≤ (c.num*b.den)*a.den := Nat.mul_le_mul hbc (Nat.le_refl _)
    _ = (c.num*a.den)*b.den := by ac_rfl

theorem decisions_sound (u : Fraction → Nat) (bits : List Bool) (A B : Interval)
    (a b reference : Fraction) (ha : covers A a) (hb : covers B b) :
    (lt (run u bits A B).hi reference → lt (value a b (degree bits)) reference) ∧
    (lt reference (run u bits A B).lo → lt reference (value a b (degree bits))) := by
  have h := run_sound u bits A B a b ha hb
  exact ⟨strict_after h.2, fun hr => strict_before hr h.1⟩

#print axioms run_sound
#print axioms decisions_sound

-- Bound for the uncropped integer product used on the reciprocal-degree path.
theorem significand_bound (m g n : Nat) (hm : m < 2^24) (hg : g < 2^25) :
    m * g^n < 2^(24 + 25*n) := by
  calc
    _ ≤ m * (2^25)^n := Nat.mul_le_mul_left _ (Nat.pow_le_pow_left (Nat.le_of_lt hg) _)
    _ < 2^24 * (2^25)^n := Nat.mul_lt_mul_of_pos_right hm (Nat.pow_pos (by decide))
    _ = _ := by rw [Nat.pow_add, Nat.pow_mul]

-- Only degrees through 15 need nontrivial odd perfect powers in the lookup.
theorem exact_degree_lt_16 (r n : Nat) (hr : 3 ≤ r) (hfit : r^n < 2^24) : n < 16 := by
  apply Classical.byContradiction
  intro hn
  have he : 16 ≤ n := by omega
  have h1 : 3^16 ≤ 3^n := Nat.pow_le_pow_right (by decide) he
  have h2 : 3^n ≤ r^n := Nat.pow_le_pow_left hr n
  have h3 : 2^24 ≤ (3:Nat)^16 := by decide
  omega

#print axioms significand_bound
#print axioms exact_degree_lt_16

-- Count a shared square twice when its error propagates to both factors.
def errorCount : List Bool → Nat → Nat → Nat
  | [], a, _ => a
  | bit :: rest, a, b => errorCount rest (if bit then a+b+1 else a) (b+b+1)

theorem error_count_formula (bits : List Bool) (a b : Nat) :
    errorCount bits a b = a + degree bits * (b+1) := by
  induction bits generalizing a b with
  | nil => simp [degree, errorCount]
  | cons bit rest ih =>
    cases bit <;> simp [degree, errorCount, ih, Nat.add_mul, Nat.mul_add, Nat.two_mul,
                        Nat.mul_assoc] <;> omega

#print axioms error_count_formula

end RootInterval

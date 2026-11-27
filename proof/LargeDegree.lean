/- SPDX-License-Identifier: MIT
   Copyright (c) 2026 dIHlHIb
   Developed with AI assistance. -/
import Std

namespace RootLargeDegree

-- Integer form of (1 + 1/d)^k ≥ 1 + k/d.
theorem bernoulli (d k : Nat) :
    (d+k) * d^k ≤ d * (d+1)^k := by
  induction k with
  | zero => simp
  | succ k ih =>
    have linear : (d+(k+1))*d ≤ (d+k)*(d+1) := by
      simp only [Nat.mul_add, Nat.add_mul, Nat.one_mul, Nat.mul_one]
      omega
    calc
      _ = ((d+(k+1))*d)*d^k := by rw [Nat.pow_succ]; ac_rfl
      _ ≤ ((d+k)*(d+1))*d^k := Nat.mul_le_mul_right _ linear
      _ = ((d+k)*d^k)*(d+1) := by ac_rfl
      _ ≤ (d*(d+1)^k)*(d+1) := Nat.mul_le_mul_right _ ih
      _ = _ := by rw [Nat.pow_succ]; ac_rfl

theorem block (d k : Nat) (hd : 0 < d) (hk : d ≤ 2*k) :
    3*d^k ≤ 2*(d+1)^k := by
  apply Nat.le_of_mul_le_mul_left (c := d) _ hd
  calc
    _ = (3*d)*d^k := by ac_rfl
    _ ≤ (2*(d+k))*d^k := Nat.mul_le_mul_right _ (by omega)
    _ = 2*((d+k)*d^k) := by ac_rfl
    _ ≤ 2*(d*(d+1)^k) := Nat.mul_le_mul_left _ (bernoulli d k)
    _ = _ := by ac_rfl

theorem amplified (d k : Nat) (hd : 0 < d) (hk : d ≤ 2*k) :
    2^149 * d^(k*256) < (d+1)^(k*256) := by
  have gain : (2:Nat)^256 * 2^149 < 3^256 := by decide
  have raised := Nat.pow_le_pow_left (block d k hd hk) 256
  simp only [Nat.mul_pow, ← Nat.pow_mul] at raised
  apply Nat.lt_of_mul_lt_mul_left (a := 2^256)
  calc
    _ = (2^256 * 2^149) * d^(k*256) := by ac_rfl
    _ < 3^256 * d^(k*256) := Nat.mul_lt_mul_of_pos_right gain (Nat.pow_pos hd)
    _ ≤ _ := raised

theorem at_index (d u k n : Nat) (hd : 0 < d) (hk : d ≤ 2*k)
    (hu : u = d+1) (hn : n = k*256) : 2^149 * d^n < u^n := by
  subst u
  subst n
  exact amplified d k hd hk

-- (1 + 2^-25)^(2^32) > 2^149, after clearing positive denominators.
theorem above_one : 2^149 * (2^25)^(2^32) < ((2:Nat)^25+1)^(2^32) := by
  exact at_index (2^25) (2^25+1) (2^24) (2^32) (by decide) (by decide) rfl (by decide)

-- (1 - 2^-25)^(2^32) < 2^-149, also as an integer inequality.
theorem below_one : 2^149 * (2^25-1)^(2^32) < ((2:Nat)^25)^(2^32) := by
  exact at_index (2^25-1) (2^25) (2^24) (2^32) (by decide) (by decide) (by decide) (by decide)

#print axioms above_one
#print axioms below_one

end RootLargeDegree

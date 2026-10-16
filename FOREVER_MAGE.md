# Forever mage module: notes

Branch `class/mage` (from `foundation`), 2026-10-16. Spec: forever-utility `simc/specs/mage.md`.
Code: `engine/class_modules/sc_mage.cpp`, `engine/class_modules/apl/mage.{hpp,cpp}`, profiles in `profiles/Mage/`.

## What is implemented

All values (damage, cost, cast time, coefficients, per-level scaling, talent per-rank values) come from the
70425 client data through `max_rank()` / `parse_all_passive_talents()` / `parse_effects()`.

- **Shared**: ranked Fireball, Frostbolt, Arcane Missiles (channel + per-rank missile spell), Fire Blast, Scorch,
  Frostfire Bolt, Frost Blast, Cone of Cold, Frost Nova, Arcane Explosion, Blizzard and Flamestrike (Forever
  tick spells), Evocation, Arcane Intellect, Mage/Ice/Frost Armor, `mana_gem`. Hit talents (property 16) are
  parsed per action. Mage Armor and Evocation feed `composite_mana_regen_while_casting()`, Evocation feeds
  `composite_spirit_regen_multiplier()`. The APL picks the tree with the most points (`primary_tree.<tree>`
  expression is also available).
- **Arcane**: Arcane Blast (stacks: +15% Int as spell power via `composite_spell_power`, +175% cost per stack,
  15 s not refreshed), Arcane Wrath (+5% max mana as spell power), Arcane Concentration / Clearcasting,
  Missile Barrage (faster missiles kept for the whole channel), Presence of Mind, Arcane Power, Arcane
  Meditation (aura 134, engine), Arcane Mind / Instability / Impact (passives).
- **Fire**: Pyroblast, Heating Up (-25% Pyro cast per stack, Fire Blast extra stack), Ignite (rolling per-mage
  residual DoT), Improved Scorch / Fire Vulnerability (caster-only), Master of Elements, Combustion (3 crit
  charges, +10% crit per fire hit), Fire Power (per own fire effect, max 4), Blast Wave, passives.
- **Frost**: Ice Lance (x3 vs Frozen), Fingers of Frost, Shatter, Frostbite (behind `mage.target_freezable`),
  Winter's Chill (70425 hotfix: +5% damage taken and +7% coefficient per stack for the next Cone of Cold),
  Cold Snap, passives.

Options: `mage.cone_of_cold_in_range=1`, `mage.target_frozen=0`, `mage.target_freezable=0`.

## ESTIMATES (module-side guesses, search the source for ESTIMATE)

1. Base spell/melee crit 0.2% (vanilla mage); base stats L2-60 are the foundation's ESTIMATE rows.
2. Ice Lance spell power coefficient 0.143 (data has 0 on every rank; TBC value). Hook V7.
3. Fire Power counts Ignite, the Fireball / Pyroblast / Flamestrike DoTs and Fire Vulnerability (once). Hook V4.
4. Ignite ticks are 40% of the crit amount and are not multiplied again (no target-debuff double dip); only
   pure fire direct crits ignite.
5. Arcane Concentration: no low-rank proc penalty, 1 s ICD from data, each missile can proc. Hook V8.
6. Missile Barrage needs the cast to hit; Arcane Missiles hits do not proc it (not in the 70425 text).
7. Fingers of Frost: a charge is used by every damage spell cast; while charges remain all spells count the
   target as Frozen (Shatter, Ice Lance).
8. Winter's Chill: stacks additive (5% damage taken via aura 271, 7% coefficient per stack added to the base
   coefficient), consumed by the next Cone of Cold. Hook V5.
9. Combustion: a non-crit fire hit adds a +10% stack, a fire crit uses one of 3 charges (vanilla rule).
10. Mana gems: one of each gem the mage can conjure, conjured before the pull, 60 s shared cooldown.
11. `mage.cone_of_cold_in_range` defaults on (Patchwerk assumes the mage can stand within 10 yd).
12. Evocation: spirit regen x16 with 100% kept while casting (vanilla reading of aura 110/134). Hook V11.
13. Presence of Mind is used up by the next mage spell with a cast time.
14. Mage melee attack power Str - 10.

## Not implemented (spec items)

- Defensive / utility: Ice Block, Ice Barrier, Mana Shield, Fire/Frost Ward, Counterspell, Blink, Polymorph,
  conjure food/water, Amplify/Dampen Magic (no DPS effect in a Patchwerk sim).
- Wand (Shoot) and Wand Specialization.
- `rank=N` downranking option on ranked actions.
- AoE APL lists (the AoE spells exist as actions).
- Improved Blizzard chill (Blizzard does not proc Fingers of Frost / Frostbite), Impact stun, Arcane Subtlety
  resistance reduction (no resistance model), Burning Soul / Improved Channeling pushback.
- Low-rank spell power / proc chance penalty (engine option `classic_low_rank_penalty`, off by default).
- Mana gem / armor conjure time before the pull, Evocation interruption by damage.

## Proposed shared-engine changes

1. `optimal_raid=1` adds the engine's (retail style) `arcane_intellect` raid buff on top of the mage's own Arcane
   Intellect (+3 Int at 60). Make the raid buff the classic flat value, or skip it when a mage provides it.
2. Parse aura 174 (spell damage from % of a stat) and aura 270 (damage taken from caster, by school) in
   `parse_effects`; the mage handles both class-locally (Arcane Blast, Fire Vulnerability).
3. Property 16 (hit) talents are not passive-parsed; every class has to `parse_effects( talent, PARSE_PASSIVE )`
   per action. A player-level passive for spell hit by school would remove the boilerplate.
4. `class_talents=` input shows `Talents: CAAAA...` in the report: the generated trait hash looks empty for
   Forever trees, so exported profiles cannot round-trip the talents.
5. A low-rank proc chance penalty helper (Oct 1 notes) shared with Arcane Concentration, Heating Up, etc.

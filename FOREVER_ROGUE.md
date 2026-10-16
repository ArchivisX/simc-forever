# Forever rogue module notes

Branch `class/rogue` (based on `foundation`). Files: `engine/class_modules/sc_rogue.cpp`,
`engine/class_modules/apl/rogue.{cpp,hpp}` and `profiles/Rogue/*.simc`. Spec: `forever-utility/simc/specs/rogue.md`.

## Implemented

Ranks come from explicit id lists through `forever::max_rank`. Damage, costs, durations and talent per-rank values
come from the 70425 data, through `parse_all_passive_talents()` and `parse_effects()`.

- **Resources.** Energy uses the engine's classic 2 s tick. Regen is not hasted. Adrenaline Rush doubles regen
  (40 per tick). Combo points are a player resource (max 5).
- **Expressions.** `energy.time_to_tick` and `stealthed`, plus the standard `combo_points`, `buff.*`, `debuff.*`,
  `dot.*` and `talent.*`.
- **Builders.** Sinister Strike, Backstab (dagger, behind), Ambush (dagger, behind, Stealth or Cutthroat),
  Garrote (Stealth), Ghostly Strike (180% with a dagger) and Hemorrhage.
  - Hemorrhage does 145% with a dagger and applies a debuff giving Rupture +15% (aura 271, parsed as a target effect).
  - Mutilate is a cast that rolls the attack table, then makes an MH and an OH hit (which cannot miss) and grants 2 CP.
    It does +20% vs a target with Deadly Poison, and Cold Blood covers both hands.
- **Finishers.** On a hit they spend all CP; a miss keeps them.
  - Eviscerate: data min/max of the base points, plus `pp_cp × CP`.
  - Rupture: 6 s + 2 s per CP; tick = bp + pp_cp × CP.
  - Slice and Dice (aura 319, 6 s + 3 s per CP; Improved SnD applies to the whole duration) and Venom.
  - Expose Armor: −450 per CP at rank 5, applied before the % armor ignore.
- **Cooldowns.** Cold Blood, Adrenaline Rush, Blade Flurry (+20% speed and a copy of each melee hit onto a second
  target), Premeditation (+2 CP while stealthed, lost after 20 s), Preparation (resets every other cooldown) and
  Stealth (precombat).
- **Poisons.** `apply_poison,mh=instant|deadly,oh=instant|deadly`.
  - Ranks are the application spells (Instant 8679–11340, Deadly 2823–25351). Proc chance per landed white or
    yellow hit of that hand is the application spell's proc chance (Improved Poisons is parsed into it), plus
    Venom. The proc spell comes from the item enchantment named by the application spell's effect #1.
  - Instant is a direct Nature hit. Deadly is a 12 s DoT that stacks to 5.
  - Poisons roll Nature spell hit, with partial resists from the engine.
- **Coded talents.**
  - Seal Fate (0.5 s ICD from the data, so once per Mutilate), Ruthlessness, Relentless Strikes.
  - Puncturing Wounds (extra CP on Backstab), Initiative, Cutthroat, Quietus, Thousand Cuts.
  - Improved Expose Armor (refund at 5 CP).
  - Hack and Slash in all three branches (sword/axe extra MH swing with a 0.2 s ICD, dagger/fist crit, mace armor %).
  - Dual Wield Specialization (aura 122), Precision (melee and spell hit, auras 54/55), Murder (aura 168,
    Humanoid/Giant).
  - Coup de Grace: +2% Eviscerate per Deadly Poison stack.
- **Auto-parsed talents.** Malice, Lethality, Improved Eviscerate, Aggression, Improved Sinister Strike, Flawless
  Execution, Vile Poisons, Opportunity, Improved Ambush, Serrated Blades (armor % and Rupture), Vigor, Weapon
  Expertise (engine expertise reduces dodge/parry), Dirty Deeds and Endurance. Debug output confirms the curve
  values, for example Lethality 20% and Improved Eviscerate 20%.
- **APL.** `rogue_apl::rogue()` is plain action lists: precombat variables pick the tree from its signature
  talents (`variable.assassination`, `variable.subtlety`), Mutilate from two daggers (`variable.mutilate`) and
  Slice and Dice from the level (`variable.snd`); the default list calls the `assassination`, `combat` or
  `subtlety` list. Backstab needs a main hand dagger, otherwise Sinister Strike.
  - Assassination: Mutilate, finish at 4 CP, Deadly Poison MH and Instant Poison OH.
  - Combat: SS/Backstab with BF, AR and Instant/Instant.
  - Subtlety: Stealth, Premeditation and Ambush opener, Hemorrhage builder, Rupture with Hemorrhage up, Ghostly
    Strike.
  - Slice and Dice only from level 42 (rank 2): rank 1 loses to Eviscerate (L25 −8%).

## Independent check (2026-10-16)

Fixed: Subtlety Preparation never fired (it waited on an untalented Cold Blood); Preparation reset items, potions
and racials; the Subtlety sample failed the tier gates (and now drops 5 dead Cutthroat points); finisher combo
points moved into the action state; poisons and talent combo point gains read from data; the APL is plain action
lists. DPS of all four samples is unchanged within error.

## Results (1000 iterations, 300 s, level 63 boss, default raid buffs)

| Profile | DPS | Top abilities by damage % |
|---|---|---|
| rogue-60-assassination | 218 | MH white 22, Deadly Poison 18.5, Eviscerate 17, Mutilate MH 11.4 + OH 11.3, OH white 11 |
| rogue-60-combat | 180 | Sinister Strike 31, MH white 28, OH white 16.5, Eviscerate 13, Instant Poison 8.4 |
| rogue-60-subtlety | 175 | MH white 27, Hemorrhage 24, Rupture 18, OH white 14, Instant Poison 10, Ghostly Strike 5 |
| rogue-25-combat (60 s, L25 mob) | 46 | Sinister Strike 35, MH white 32, Eviscerate 15, OH white 13, Instant Poison 5 |

- **Energy.** Energy cap uptime is 0–0.25%. Waiting is 58–72%, which is inherent to a 10 energy/s budget with
  1 s GCDs.
- **Attack table checks.** White miss is 24% (8% + 19% DW − 3% Precision), glance 40%, Instant Poison miss
  13.8% (17% − 3%).
- **Warrior comparison.** The foundation `Sanity/warrior-60-melee` sample gives 68 DPS on this build: untalented,
  Arcanite Reaper, Rend and Heroic Strike. The same rogue profiles with weapons only (two blues, no armor, 51 talents)
  give 143–187 DPS.

## ESTIMATES (search `ESTIMATE` and the 2026-10-16 comments in sc_rogue.cpp)

1. Base crit is 0% (vanilla). AP = Str + Agi + 2·L − 20 (vanilla, not measured). Base stat rows above 30 are the
   foundation's vanilla estimates.
2. A finisher or builder that misses, is dodged or is parried costs 20% of its energy (vanilla, same as the warrior
   rage rule).
3. The Mutilate OH hit has no off-hand penalty. The engine's 0.5 is undone.
4. The per-CP duration (SnD and Venom +3 s, Rupture +2 s) comes from the tooltips.
5. SnD and Blade Flurry speed stack multiplicatively (V-R7).
6. Coup de Grace is baseline for every rogue from level 30. The data aura points at the Deadly Poison spells, so it
   is coded from the tooltip (V-R10).
7. Poisons proc from white and yellow hits of their hand, have no AP scaling (data) and have unlimited charges.
   Improved Poisons' "no charge" part is ignored.
8. The Hack and Slash extra attack is a main-hand white swing. It can be triggered by a sword or axe in either hand,
   and the DW miss penalty applies.
9. Blade Flurry copies the final damage to one other enemy and ignores armor.
10. Rupture, Garrote and Eviscerate have no AP term. The tooltips say "increased by Attack Power" but the data has
    no coefficient (V-R5/V-R6).
11. The Cold Blood cooldown starts on cast (the data says on use). Premeditation's CP are lost after 20 s.
12. Murder applies to the default sim target (Humanoid).
13. Thousand Cuts gains a stack on every Rupture tick, and the next BS or Hemorrhage uses all stacks.

## Spec items NOT implemented

- **Abilities.** Gouge, Kidney Shot (and Improved Kidney Shot), Cheap Shot (so Initiative and Dirty Deeds only
  apply to Garrote), Riposte, Evasion, Sprint, Vanish (no re-stealth: openers happen once per fight), Kick, Feint,
  Sap and Blind.
- **Poisons.** Wound, Crippling and Mind-numbing poisons. The SoD poisons (Occult, Sebacious, Atrophic, Numbing).
  Poison charges and sharpening-stone coexistence.
- **Talents and defensive effects.** Setup, Remorseless Attacks (no kills on Patchwerk), the Ghostly Strike dodge
  buff, Deflection, Lightning Reflexes and the other "c" talents. Dirty Deeds' positional rule (Garrote position is
  not checked).
- **Venom.** Implemented, but not in the default APL: at 4 CP it loses about 2% to Eviscerate with the sample gear.
  Add `venom,if=combo_points>=4&buff.venom.remains<1` to test it.
- **APL and expressions.** No `apl_variant=` option, and no `energy.next_tick_amount` expression.
- **Combo points.** They are on the player, not on the target. A finisher's points are snapshot in
  `rogue_action_state_t` (retail pattern), so a Rupture keeps its own value.
- **Talent strings.** Profiles use `class_talents=` because there is no rogue addon export. Shared issue: the
  generated `talents=` hash prints `CAAAA…`, which the mage agent also saw.

## Proposed shared-engine changes (implemented class-locally where needed)

1. Parse aura 54/55 (melee/spell hit %) as passive player modifiers (Precision). The rogue overrides
   `composite_melee_hit` and `composite_spell_hit`.
2. Fix `player_t::composite_spell_hit()`, which adds expertise (a retail rule). That gives classic classes with
   expertise talents extra spell hit. The rogue override drops it.
3. Parse aura 122 (`A_MOD_OFFHAND_DAMAGE_PCT`, Dual Wield Specialization) and aura 168 (damage vs creature type,
   Murder).
4. Add a shared armor-reduction debuff for aura 674 (Sunder Armor and Expose Armor, raid-wide, stacking rules). Both
   modules currently only reduce armor for their own attacks.
5. Add a per-action opt-out for the off-hand 0.5 penalty in `action_t::calculate_direct_amount` (Mutilate).
6. Fix the `average_range=0` double range. Classic effects already get min/max from the Variance, and
   `amount_delta = m_delta` then applies the range a second time (Eviscerate sets `amount_delta = 0`).
7. Make `generate_traits_hash` produce a real hash from `class_talents=` input.
8. Fix item hit rating: `composite_melee_hit_rating` is commented out of `composite_melee_hit`.
9. Record zero-damage attack casts (Mutilate) as hits. The report shows only their misses and dodges (cosmetic).
10. `player_t::init_talents` (player.cpp, the trait curve loop around line 3245): skip a curve whose effect
    no longer exists (`spell->effectN( eff_idx ).id() == 0`). Heightened Senses (30894) has curves for effect #1,
    which 70425 removed, and `register_effect( 0 )` throws "Invalid effect id '0'": any build with that talent
    fails init.
11. Move the "Discount Power On Miss" rule (20% of the cost on a miss, dodge or parry) from the warrior and rogue
    modules into `action_t::consume_resource` behind a `classic_*` option; both modules carry the same copy.
12. Measure the rogue base stat rows (V-R0), the energy tick (V-R1), and poison and Rupture AP scaling
    (V-R4/V-R5).

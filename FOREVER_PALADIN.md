# Forever paladin module: notes

Branch `class/paladin`, based on `foundation`. Spec: `forever-utility/simc/specs/paladin.md`. Data: build 70425.
Files: `engine/class_modules/sc_paladin.cpp`, `engine/class_modules/apl/paladin.{cpp,hpp}`, `profiles/Paladin/`.

## Implemented

**Resource:** mana with the foundation spirit/5SR model. Reverence (aura 134) is picked up by the core.
Costs come from the data: flat costs, % of base mana (Judgement 6%, Kings 8%), Benediction, Holy Conduit,
Twist of Light -20%, Swift Judgement (free next Judgement).

**Seals** (one at a time; the buff from the best known rank):
- Seal of Righteousness: per-swing Holy damage (ESTIMATE formula below), and Judgement of Righteousness.
- Seal of Command (11 pt talent): 70% weapon damage Holy proc (20424) at `paladin.soc_ppm` (default 7) with
  the 1 s ICD from the data, and Judgement of Command (half damage; no stun model).
- Seal of Fury: flat Holy proc per swing (the seal's trigger spell), and Judgement of Fury.
- Seal of the Crusader: +AP, +40% attack speed with proportionally less damage per white hit, and Judgement
  of the Crusader (target debuff). Melee swings refresh it, and so does Holy Strike with Sacred Arbiter.
- **Judgement** (20271): off the GCD, 10 s (8 s with Improved Judgement 2/2), does **not** consume the seal.
  Sanctified Judgement refunds part of the judged seal's cost.
- **Twist of Light**: replacing SoC/SoR/SoF leaves an Echo, and the next white swing applies the old seal too.

**Abilities:**
- Holy Strike (all 8 ranks): Holy, melee table, normalized; sp 0.429 is read from the weapon effect.
- Consecration: ground effect with 8 pulses. Pulses hit all targets for E1, and the first 4 for E1+E2.
- Exorcism and Holy Wrath: Undead and Demon only (`enemy=Name` + `race=undead`).
- Hammer of Wrath: ≤20% health.
- Holy Shock (21 pt): the cast carries cost and cooldown; the damage is a separate spell per rank.
- Light's Vigil (31 pt): marks the target. The next Holy Shock has no cooldown, adds extra damage and refunds 75%.
- Divine Favor (16 Holy): next Holy Shock crits.
- Swift Judgement (16 Prot): resets Judgement, and the next one is free.
- Blessing of Might and Blessing of Kings: self buffs, one at a time.

**Talents:**
- Passive talents come from `parse_all_passive_talents()`, with the curve rank values: Conviction, Precision,
  Divine Strength/Intellect, Improved Judgement, Benediction, Holy Conduit, Improved Seals, Sacred Arbiter
  +20%, Two-Handed Spec, Instrument of Law and Purifying Power.
- Class-local code: Vengeance (20050, rank value per stack, 3 stacks, Physical+Holy), Vindication (self AP%),
  Champion of the Light (SP += 10/20/30% Int; aura 174), Divine Precision (hit, parsed per action), Holy Power
  (flat crit points), Consecrated Ground, Sanctified Judgement and Sacred Arbiter's refresh.

**Default APL:** BoM → SotC → JotC once → Seal of Command (or SoR without the talent) → reseal → HoW →
Divine Favor/Holy Shock → Light's Vigil → Holy Strike → Judgement → Twist (mana > 50%, always back to SoC) →
Exorcism → Consecration (AoE or mana > 90%).

## Sample results (1000 iterations, 300 s, level 63 boss, default `optimal_raid=1`)

| Profile | DPS | Top abilities |
|---|---|---|
| `paladin-60-retribution` (Undead, 20/0/31 Twist) | 212.7 | melee 35.6%, SoC proc 22.1%, Holy Strike 21.3%, JoC 16.0%, HoW 2.6% |
| `paladin-60-shockadin` (Undead, 21/0/30) | 211.2 | melee 32.8%, SoC proc 21.3%, Holy Strike 20.3%, JoC 16.6%, Holy Shock 8.0% |
| `paladin-25-retribution` (Undead, 16 Ret, vs L25) | 56.6 | melee 50.4%, SoC proc 19.2%, Holy Strike 15.7%, JoC 14.8% |
| `Sanity/warrior-60-melee` (no talents, Arcanite Reaper only) | 68.0 | heroic strike 47%, melee 37%, rend 15% |
| paladin with the same gear (Arcanite Reaper only, human), no talents | 99.3 | |
| paladin with the same gear, retribution talents | 166.4 | |

Mana, measured at seed 7:
- Retribution sits around 40–45% and spends under 10% of the fight below 10% mana.
- Shockadin drains to about 15% after 2 minutes and spends 26% of the fight below 10%.
- Leveling L25 runs dry late in the fight (41% of the time below 10%; the L25 pool is 782).
- Waiting is about 70%, which is normal for a melee GCD class.

Profilesets:
- No Twist of Light (Instrument of Law 2/2 instead): 213.7 vs 212.2. Twisting is about DPS-neutral because mana binds.
- Seal of Command at 4 PPM (the beta report): 198.4.
- Undead boss with Exorcism: +4.6 DPS.

## ESTIMATES (verify in game)

1. Seal of Command at 7 PPM: the data has 100% chance and no PPM record, so the rate is server side. The beta
   reports 3–4/min. Option `paladin.soc_ppm=`.
2. Seal of Righteousness per hit is not in the data.
   - We use the vanilla formula: 1H s×0.85×speed/100, 2H s×1.2×speed/100, times (1 + Improved Seals).
     It reproduces the tooltip ends (s/87 and s/25) at 1.35 s and 3.33 s.
   - SP coefficients are the flat tooltip values: 0.0765 (1H) and 0.12 (2H).
   - It can't miss and crits as a spell.
3. Holy effects with "Spell Type: Melee" use the melee table (respecting No Active Defense/Always Hit) and
   crit at 200%. That covers the SoC/SoF procs, Judgements and Holy Strike.
4. Judgement of the Crusader's flat bonus is applied like wowsims/classic, as extra spell power scaled by
   each Holy spell's coefficient.
5. Judgement of Command always deals half damage, because stuns aren't modelled.
6. Holy Strike = (weapon + bonus) × 50%, following the server's effect order. The tooltip says
   "50% weapon damage plus X".
7. Base crit is vanilla: 0.7% melee and 3.336% spell. Base stats for L31–60 are the foundation's vanilla rows.
8. Echo (no spell id is known): it lasts until the next white swing, which consumes it hit or miss. Seal of
   Justice is not modelled.
9. Seal of the Crusader: per-hit damage drops by 1/1.4, so white DPS is unchanged.
10. Vindication: 100% proc (data) on white swings and Holy Strike. The target AP reduction is not modelled.
11. Consecrated Ground: the target always counts as one of the first 4.
12. Holy Power (A108 P7 "percent") is treated as additive crit points.

## Not implemented

- **Healing (out of scope):** Holy Light, Flash of Light, the heal halves of Holy Shock and Light's Vigil,
  Illumination, Infusion of Light, Healing Light, Lay on Hands.
- **Tanking:** Holy Shield, Reckoning, Righteous Fury threat, Templar's Bulwark, Improved Seal of Fury, Shield
  Specialization, Redoubt, Iron Creed, and the Seal of Fury absorb. There is no Protection DPS profile.
- **Utility seals and spells:** Seal of Justice, Light and Wisdom and their Judgements; Retribution Aura;
  Repentance; Hammer of Justice. Hammer of the Righteous is unconfirmed in Forever.
- **Undead racials:** Touch of the Grave is disabled upstream; Cannibalize is not modelled.
- Racial weapon skill.
- One-Handed Spec on Holy damage: per the data it is Physical only.
- **Talent strings:** `save=` writes `talents=CAAAA…`, which then fails with "Wrong specialization" (no paladin
  spec), so the profiles use `class_talents=` with trait names.
- The SoD passives that appear in "Affecting Spells" (Seal Cost Reduction 1249005, Judgement Damage Increase
  459601 and 1291086) are not in any SkillLineAbility, so they are ignored.

## Proposed shared-engine changes (class-local workarounds in place)

1. **Aura 174** (spell damage from a % of a stat) in the core. Paladin applies Champion of the Light in
   `composite_spell_power`.
2. **A108 property 7** (percent crit modifier): classic means additive crit points. simc multiplies a zero base.
3. **Property 16 hit** is only parsed per action (`parse_effects(spell, PARSE_PASSIVE)`). It could be a
   player-level passive field.
4. `parse_effect_data` ignores the SP coefficient on weapon-damage effects (E121/E31): Holy Strike 0.429,
   SoC 0.29.
5. `action_t::calculate_weapon_damage` scales weapon base damage by `composite_attack_power_multiplier`. That
   is a retail rule, so classic AP% buffs (Vindication, the optimal_raid battle_shout aura) also raise weapon damage.
6. `target_race=` does not reach the default Fluffy_Pillow. `enemy=Boss race=undead` works.
7. The APL `gcd` expression is 0 inside off-GCD actions. That is why `divine_favor` uses
   `cooldown.holy_shock.remains<1.5`.
8. The talent hash export/import fails for classes with no specialization.
9. Attack `miss_chance` can go negative (it is skipped by `build_table`, so harmless).

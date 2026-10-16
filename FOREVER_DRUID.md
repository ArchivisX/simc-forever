# Forever druid module: notes

Branch `class/druid`, built on `foundation`. Spec: `forever-utility/simc/specs/druid.md`. Data: simc generated 1.60.1.70425.
Scope: **Feral (Cat) and Balance damage**. Bear tanking and Restoration healing are out of scope for now.

## Files

- `engine/class_modules/sc_druid.cpp`: the module (`druid_t : parse_player_effects_t`, `druid_action_t<Base> : parse_action_effects_t<Base>`).
- `engine/class_modules/apl/druid.cpp`: default APL between `//druid_apl_start` and `//druid_apl_end`.
- `profiles/Sanity/druid-60-feral.simc`, `druid-60-balance.simc`, `druid-25-feral.simc`.
  - Profiles use `class_talents=name:rank/...`. The trait hash simc writes back for druids is empty (`CAAAA…`), so a `talents=` export string cannot be produced from simc yet. Addon `talents=` imports should still parse.

## Implemented

**Shared**
- Forms: `cat_form`, `moonkin_form`, `cancelform`. Costs are % of base mana from the spell data; Natural Shapeshifter applies.
- Primary resource: energy if the APL uses `cat_form`, otherwise mana.
- Omen of Clarity (baseline from 20) and Clearcasting.
  - The next affected ability is free (Clearcasting's affected-spell list, so Wrath and free abilities are excluded).
  - It is consumed before the hit can proc a new one.
  - Proc data is from the client: 100% chance, 10 s ICD, halved in Moonkin Form.
- Mark of the Wild (self, stat buff), Innervate (spirit regen ×5 and 100% while casting), Faerie Fire.
  - Faerie Fire armor is client aura 674, applied like warrior Sunder: it affects this druid's own attacks only.
  - In Cat Form Faerie Fire is free, with a 6 s CD and a 1.0 s GCD (passive 3025).
- Passive talents through `parse_all_passive_talents()` with per-rank trait curves. Verified: Improved Wrath 5 (Wrath 55.2 mana), Eclipse r3 −500 ms, Vengeance (2.0× spell crits).

**Feral (Cat)**
- Cat weapon: the equipped main hand's DPS at a fixed 1.0 s swing. Abilities use it as their weapon.
- Cat AP: 3025 effect 1 (12 + 2/lvl = 2×level) + Agility + Predatory Strikes (% of level). Base AP is Str×2 − 20.
- Abilities:
  - Claw, Shred (requires `position=back`), Rake (direct + bleed).
  - Rip: per-CP ticks, with the combo points snapshotted on application.
  - Ferocious Bite: per-CP damage, plus 2.7 per energy above the cost; spends all energy.
  - Shifting Power (16 pt): +40 energy, not usable at full energy.
  - Berserk (31 pt): +100% builder crit, off GCD.
- Energy refund: 80% on miss/dodge (Discount Power On Miss). Finishers keep their combo points on a miss.
- Talents with code: Heart of the Wild (Str in Cat), Predatory Strikes, Blood Frenzy (extra CP on builder crit), Leader of the Pack (self +3% melee crit in Cat), Rend and Tear, Natural Instinct (parsed, P15).
- Talents through passive parsing: Sharpened Claws (A290), Ferocity, Shredding Attacks, Savage Fury, Improved Shifting Power, Naturalist, Genesis.
- 16-pt milestone: Shifting Power is implemented. Primal Bite is bear-only, so it is not.

**Balance**
- Wrath, Starfire, Moonfire (direct + DoT), Insect Swarm (16 pt; talent gated).
- Moonkin Form (31 pt) with the Moonkin Aura: self +3% spell crit.
- Eclipse: Wrath gives 2 charges (max 4); each Starfire uses one and loses the talent's effect #2 from its cast time.
- Nature's Grace: a non-periodic spell crit gives +10% cast speed and −10% GCD for 3 s.
- Talents through passive parsing: Improved Wrath/Moonfire/Starfire, Moonglow, Moonfury, Vengeance, Nature's Splendor, Nature's Majesty.
- Nature's Reach hit (aura 54/55) is applied in the class; see the shared changes below.
- Default APL:
  - Feral: FF → Berserk → Shifting Power ≤40 energy → Rip at 5 CP → FB at 5 CP while Rip has >6 s left → Shred → Claw.
  - Below 32 (no Ferocious Bite): Rip at 4+ CP and Rake. Rake has no AP scaling and loses DPS at 60 (−4 DPS in profileset tests).
  - Balance: Innervate <30% mana → IS → MF → Starfire with Eclipse → Wrath.

## Results (1000 iterations, 300 s, Patchwerk vs level +3; 25: vs level 25)

| Profile | DPS | Top abilities (damage %) | Resource |
|---|---|---|---|
| druid-60-feral | 272 | Rip 35, Shred 35, melee 27, Claw 3, FB 1 (Rip refreshes before 5 CP + >6 s comes up) | Energy: waiting ~42% (cat norm at 1.0 s GCD), cap 0.1%; mana not OOM (Shifting Power is mana-limited) |
| druid-60-balance | 233 | Starfire 54, Moonfire 23, Insect Swarm 14, Wrath 10 | Mana: no waiting, Innervate once, Clearcasting ~42 procs |
| druid-25-feral | 68 | melee 39, Shred 26, Rip 22, Rake 10, Claw 2 | Energy, as above |

Sanity vs warrior, Arcanite Reaper only, no other gear, no talents:

| Actor | DPS |
|---|---|
| Warrior, default APL | 94 |
| Warrior, `warrior-60-melee.simc` APL | 68 |
| Druid Cat (FF/Rip/FB/Shred/Claw) | 125 |
| Druid Cat with `druid.omen_of_clarity_chance=0` | 98 |

The Cat's edge comes from Cat AP (+120 + Agility), Rip, and the client Omen of Clarity data (one free ability every ~10 s).

## ESTIMATES (search the source for ESTIMATE)

| Item | Value | Source |
|---|---|---|
| Rip AP share | 1% AP per tick per CP, capped at 4 CP (`druid.rip_ap_per_cp=`) | vanilla, wowsims/classic; data has a dummy effect only |
| Ferocious Bite AP share | 3% AP per CP (`druid.ferocious_bite_ap_per_cp=`) | vanilla |
| Rake AP | 0 | vanilla and data |
| Base crit | 0.9% melee, 1.8% spell | wowsims/classic `base_stats.go` |
| Omen of Clarity | client 100% / 10 s ICD (`druid.omen_of_clarity_chance=` to override) | the server may roll lower; Blizzard says low ranks proc less. **Measure this.** It is the biggest single Cat uncertainty (~+25% Cat DPS). |
| Rend and Tear | applies to the direct damage of Cat yellow attacks (not white swings, not Rip ticks) | tooltip reading |
| Ferocious Bite with Clearcasting | the base cost is free; only energy above the normal cost converts | assumption |
| Energy | engine `classic_energy_tick` (20 per 2 s) | the tick model is unverified for druids (spec §7.1) |

## NOT implemented

- **Bear:** Bear/Dire Bear Form, Maul (on-next-swing queue), Swipe (3% AP), Lacerate, Primal Bite (16 pt, alternative to Shifting Power), Demoralizing Roar, Enrage, Bash, Growl, Frenzied Regeneration, Feral Charge, rage generation, Furor rage, Natural Reaction, Thick Hide.
- **Restoration and healing:** all heals, Swiftmend (16 pt), Wild Growth (31 pt), Nature's Swiftness, Living Spirit (parsed as a Spirit passive only), Tranquility, Rebirth/Revive.
- **Furor:** out-of-Cat energy regen and carry-over. The engine regenerates energy in every form; the modelled rotations never leave Cat.
- Hurricane: the data is odd (dummy plus a trigger). Single-target focus.
- Prowl, Ravage, Pounce openers. Entangling Roots, Nature's Grasp, Thorns, Barkskin.
- Tiger's Fury, deliberately: removed in Forever, and 5217 has acq 3.
- Insect Swarm −2% hit debuff. Starfire stun (Improved Starfire effect #2).
- **Sharpened Claws is not form gated:** the stance mask is ignored by passive parsing. Harmless for the Cat build; a Balance build that took it would gain crit.
- Leader of the Pack and Moonkin Aura buff only the druid, not the raid.
- Wolfshead Helm +5 Shifting Power energy. Weapon enchant procs in forms.
- Powershifting doesn't exist (correct for Forever). Shapeshift mid-fight GCD/mana dancing is not in the APL.

## Proposed shared-engine changes (not made here)

1. **A54/A55 player hit auras** (Modify Hit% / Modify Spell Hit%) are not parsed into player hit. Nature's Reach needed a class override. The same gap likely affects other classes' hit talents (Precision, Suppression-style).
2. **Zero-damage landed debuff applications** (Faerie Fire, Sunder-type) are not recorded in the result table, so the report shows Miss=100%. The druid works around this in `faerie_fire_t::assess_damage`. Generalise the 1356b3e4b4 rule from DoTs to all harmful actions.
3. **Stance-mask gating** for passive talent effects (Sharpened Claws, Predatory Strikes, Blood Frenzy, Heart of the Wild) in `parse_passive_effects`.
4. **Trait hash write-back** for classic class trees (`Talents: CAAAA…` in the report) so simc-generated profiles round-trip `talents=`.
5. **Raid auras** (Leader of the Pack, Moonkin Aura) as sim-level buffs/overrides, like `override.mark_of_the_wild`.
6. A shared **on-next-swing queue** (warrior `heroic_strike_queue_t`) before Bear Maul.

# Forever priest module: notes

Branch `class/priest` (based on `foundation`). Spec: `forever-utility/simc/specs/priest.md`. Data: build 70425.
DPS only. **Healing is out of scope for now** (spec phase 3).

## Files
- `engine/class_modules/sc_priest.cpp`: the module.
- `engine/class_modules/apl/priest.{hpp,cpp}`: default APL. The header had the warlock namespace; it is fixed.
- `profiles/Sanity/priest-60-shadow.simc`, `priest-60-smite.simc`, `priest-25-shadow.simc`: samples.

## Implemented
Ranks come from explicit trainer id lists through `forever::max_rank`. SoD copies such as SW:D 401955 are excluded.
Talent abilities are gated on the talent node, with the best trainer rank the player knows.

**Shadow**
- Shadow Word: Pain, Mind Blast, Devouring Plague (leech DoT, ticks crit), Shadow Word: Death (18 s).
- Mind Flay: a channel, from the talent.
- Vampiric Embrace: debuff only.
- Shadowform: +10% Shadow damage, −50% Shadow cost, ×2.0 Shadow crits. `priest_t` is a `parse_player_effects_t`:
  the A79 parts of Shadowform and Power Infusion and Shadow Weaving's A270 come from spell data
  (`priest_t::parse_player_effects`); only the A72 cost part is code.
- Talents with class code:
  - Shadow Focus: hit.
  - Shadow Weaving: 2%/stack, caster-only, triggers on a landed cast (not on wand shots: Shoot is a ranged
    attack, the proc is Magic Hostile Spell). The debuff's A270 is parsed by `parse_player_effects()`.
  - Improved Mind Flay: ramp.
  - Dark Inspiration: from Mind Blast and from a full Mind Flay channel.
  - Early Demise.
  - Improved Mind Blast: see the shared-engine change below.
- Passive parse: Darkness, Improved SW:P (8 ticks), Twin Disciplines, Devouring Contagion cost, Mental Strength, Meditation.

**Smite / Holy Fire**
- Smite and Holy Fire.
- Penance: a 2 s channel, 3 bolts at 0/1/2 s, per-rank bolt spells.
- Holy Nova: AoE.
- Inner Focus: consumed by the next spell. −100% cost; +25% crit only for spells with direct damage.
- Power Infusion: self cast, +20%.
- Talents with class code:
  - Power in Light: own SW:P or Holy Fire on the target.
  - Holy Precision: hit.
  - Searing Light: Holy Purpose → free Holy Nova.
  - Spiritual Guidance: aura 174, % of Spirit added to spell power.
- Passive parse: Divine Fury, Holy Specialization, Mental Agility, Searing Light damage.

**Other**
- Power Word: Fortitude, Inner Fire, Divine Spirit (precombat).
- Starshards (Night Elf) and Chastise (Dwarf).
- Wand filler: `shoot`. Wand Specialization applies to the wand only.
- Base stats: AP = Str − 10.

**Options**
- `priest.spirit_tap_uptime=0..1`: Spirit Tap as an average uptime. The default is 0.
- `priest.wand=min:max:speed[:school]`: the same numbers as the addon export's `weapon_damage=...ranged:min:max:speed`.
  The school defaults to arcane.

**APL**
- Precombat: buffs, then Shadowform.
- `shadow` list, used when the Mind Flay talent is taken:
  1. SW:P
  2. DP
  3. Inner Focus → Mind Blast
  4. SW:D below 20% target health (Early Demise) or above 50% mana. Both builds are mana limited on a 300 s
     fight, and Mind Flay does ~2× the damage per mana.
  5. Starshards
  6. Mind Flay
  7. wand
- `smite` list, used otherwise:
  1. Power Infusion
  2. SW:P
  3. Holy Fire
  4. Inner Focus → Penance
  5. Holy Nova on Holy Purpose (free, from Searing Light)
  6. Mind Blast, SW:D above 50% mana (17% miss without Shadow Focus). No DP: ~0.95 damage per mana without
     Shadowform.
  7. Smite
  8. wand

## Results (1000 iterations, 300 s Patchwerk vs level 63, default raid buffs)

| Profile | DPS | Top 5 by damage | Mana |
|---|---|---|---|
| L60 Shadow (14/0/37) | 297.6 | Mind Flay 60.0%, SW:P 15.0%, Mind Blast 12.5%, DP 7.8%, SW:D 2.3% | OOM late; wand 2.3%; Waiting 0% |
| L60 Smite (31/20/0) | 132.6 | Holy Fire 29.5%, Smite 27.1%, wand 18.5%, SW:P 10.7%, Holy Nova 5.9% | OOM early (no Shadowform cost cut); Waiting 0% |
| L25 Shadow leveling (vs L25) | 22.2 | Mind Flay 49.5%, wand 35.2%, Mind Blast 6.6%, SW:P 4.7%, DP 4.1% | OOM within ~1 min, then wand |

(Independent check 2026-10-16: before the wand/Shadow Weaving fix and the APL changes these were 294.6 / 124.0 / 23.2.)

Spot checks:
- Mind Blast R9 crit is ≈2× hit in Shadowform. The cooldown is 5.5 s, but Mind Flay is never clipped, so Mind Blast
  lands every ~8-9.5 s.
- Mind Flay ticks run 209→251→301 (×1.2 per tick at 2/2).
- SW:P lasts 24 s.
- DP R6 costs 985 → 246 (Shadowform + Devouring Contagion 2/2).
- Shadow Weaving and Dark Inspiration are up about 98% of the fight.
- Smite misses 1% with Holy Precision 3/3; shadow spells miss ~12% with Shadow Focus 5/5 vs +3.
- Penance: 3.1 bolts per cast.

**Sanity vs warrior.** `profiles/Sanity/warrior-60-melee.simc` gives **67.9 DPS** on this base. That is not ~96: the
profile has no talents and only rend and heroic strike. The comparable priest has only the same staff, the wand, and no
gear stats:

| Priest | DPS |
|---|---|
| No talents (smite APL) | 55.6 |
| Smite talents | 69.4 |
| Shadow talents | 161.2 |

Untalented, the priest and the warrior are in the same range. Shadow's lead comes from 51 talent points (Shadowform, Darkness,
Shadow Weaving, Improved Mind Flay) that the untalented warrior sample does not have. Smite being far below Shadow matches
the spec's headline finding: Forever's Smite and Holy Fire top ranks are about 45% of Classic.

## ESTIMATES introduced
1. Base crit (vanilla values): spell 0.8%, melee 3%.
2. Improved Mind Flay compounds (1, 1.2, 1.44). The linear alternative is 1, 1.2, 1.4 (spec V5).
3. Dark Inspiration (aura 80) is treated as a total Spirit multiplier. Its value comes from the talent rank (3…15%).
4. Spirit Tap is modelled as an option-driven average uptime: +100% Spirit and +50% regen while casting, times the uptime.
5. Shadow Weaving rolls once per landed cast, including DoT applications, and not per tick.
6. Inner Focus:
   - Its crit applies only to spells with a direct damage effect.
   - The Penance cast consumes it, so the bolts may lose the crit.
7. Penance bolts roll hit and crit individually. They are not hasted.
8. Power Infusion is self cast.
9. The wand uses the spell hit and crit tables (crits ×1.5). It has no GCD and no mana cost. The profiles' wand numbers are
   estimates.
10. DoT crits follow the data flag "Periodic Can Crit" (SW:P, Mind Flay, DP, Holy Fire). Only DP is confirmed (spec V6).
11. Profile gear is stat stand-ins (`gear_*`), and talents are set by name (`class_talents=`). No priest addon export
    exists yet.
12. Spirit regen uses the engine's measured Forever formula, which was measured on other classes and is unverified for
    priest (V8).
13. Chastise's humanoid-only restriction is not checked.

## Spec items NOT implemented
- **All healing** (phase 3):
  - Heals and absorbs: heals, Renew, PW:Shield, PoH, Binding Heal, PoM, the Penance heal, Divine Aegis.
  - Healing talents: Renewed Hope, Soul Warding, Spiritual Healing, Improved Renew, Inspiration, Litany of Light, Spirit of
    Redemption.
  - Spiritual Guidance's healing part.
- Vampiric Embrace's party heal. VE is not in the APL (option `priest.use_vampiric_embrace` not added).
- Real Spirit Tap procs on kills.
- Devouring Contagion's spread on death.
- SW:D backlash self-damage.
- Mind Flay clipping and the spec's profilesets.
- The low-rank SP penalty is engine option `classic_low_rank_penalty`, off by default.
- Utility/racials without DPS value:
  - Silence, Psychic Scream, Fade, Mana Burn, Fear Ward, Shadow Protection, Hex of Weakness, Feedback, Elune's Grace
  - Touch of Weakness, Shadowguard
  - Desperate Prayer, Divine Grace, Confounding Flash, Contingency Plan
  - Dark Sacrifice (Undead mana)
  - Shadowfiend (probably unobtainable)
- (c) talents: Blackout, Martyrdom, Silent Resolve, Improved Inner Fire, Improved Mana Burn, Shadow Reach, Improved Psychic
  Scream, Twilight Focus, Spell Warding, Holy Reach, Blessed Recovery.

## Proposed shared-engine changes (class-local workarounds in place)
1. **Ranged slot.**
   - `ranged=` is an unknown option, and `util::slot_type_string` has no `SLOT_RANGED`.
   - simc's weapon damage tables are retail scaled: a level 20 wand comes out about 5× too strong.
   - Item data carries no weapon damage school.
   - Workaround: `priest.wand=min:max:speed[:school]`. Hunters, mages and warlocks need the same.
2. **Category cooldown beats talent cooldown mods.** `action_t::init` prefers `charge_cooldown()` (the classic category
   cooldown), so A107 cooldown talents are lost, for example Improved Mind Blast's 8 → 5.5 s. Workaround: Mind Blast uses
   the modified `data().cooldown()`. Engine fix: in the passive spell modification in `player.cpp` (the `P_COOLDOWN`
   case next to the `A_MODIFY_CATEGORY_COOLDOWN` handling), also modify `category_cooldown` when it equals `cooldown`,
   as the client applies SPELLMOD_COOLDOWN to both recovery times.
3. **"Requires weapon" passives.** Wand Specialization (A79, all magic schools) is applied to every spell by the passive
   parse. Workaround: `deregister_passive_spell` plus a wand-only multiplier. The engine should skip or limit
   equipped-item-gated passives.
4. **Hit (property 16) in the player passive field map.** It would remove the per-action `parse_effects(...,
   PARSE_PASSIVE)` for Shadow Focus and Holy Precision.
5. **Aura 72** (Modify Power Cost% by school, Shadowform) and **aura 174** (spell power from % of a stat, Spiritual
   Guidance) as generic parse support.
6. **HTML report talents.** `report_html_player.cpp` `print_html_talents` sizes the class grid from
   `class_columns()`, which skips traits whose `id_spec` does not match the actor's spec. Priest traits reach column 13
   (Improved Mind Flay) but the grid gets 12 columns, so `class_traits.at( row ).at( col - 1 )` throws and the HTML report
   is cut short ("vector::_M_range_check: __n (which is 12)"). Use the max column of all class traits, or of the
   player's selected traits. Druid and rogue samples hit the same error.
7. **Foundation contract:** the warrior L60 sample is 67.9 DPS on `foundation` @ 7857febf22, not ~96.

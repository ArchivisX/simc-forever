# Forever hunter module: notes

Branch `class/hunter` (from `foundation`), 2026-10-16. Spec: forever-utility `simc/specs/hunter.md`.
Code: `engine/class_modules/sc_hunter.cpp`, `engine/class_modules/apl/hunter.{hpp,cpp}`, profiles
`profiles/Sanity/hunter-{60-bm,60-mm,60-sv,25-bm}.simc`.

## What is implemented

Damage, costs, cast times, cooldowns, per-level scaling and talent per-rank values come from the 70425 data
(`max_rank()`, `parse_all_class_passives()`, `parse_all_passive_talents()`). Verified in sims: Efficiency
costs, Improved Arcane Shot (4.5 s), Rapid Killing (Rapid Fire 3 min), Barrage, Mortal Shots, Lethal Attacks,
Improved Stings, Lightning Reflexes, Bestial Discipline (mana while casting), Careful Aim per rank.

- **Core**: ranged AP (2 Agi + 2 L - 10, ranged AP auras, Hunter's Mark on the target), melee AP
  (Str + Agi + 2 L - 20). `ranged=` slot option, `ammo_dps=`, `quiver_haste=`, `pet_happiness=`, `tracking=`.
- **Auto Shot**: repeating, interval = speed / (Quick Shots x Rapid Fire x quiver). The last 0.5 s (aim) can't
  overlap a cast or channel; the shot then fires 0.5 s after it ends (proc `auto_shot_clipped`). Instants
  don't delay it. Starting fires after 0.5 s. APL expressions `auto_shot.time_to_next/.speed/.in_aim`,
  `pet.active`, `active_hawks`.
- **Shots**: Aimed (2.0 s, shared 6 s cooldown with Multi), Multi (3 targets), Arcane (cooldown shared with
  Summon Hawk, Improved Arcane Shot only shortens its own), Serpent Sting, Sniper Shot, Volley (channel + per-rank
  tick spell), Hunter's Mark (target debuff).
- **Buffs**: Aspects of the Hawk / Beast / Falcon / Viper (exclusive; Viper -10% damage, 10% max mana per 3 s),
  Trueshot Aura, Rapid Fire (+40% ranged haste and melee speed), Deadly Aspects (Quick Shots / Quick Strikes).
  Hunter's Mark, Aspect of the Hawk and Trueshot Aura use the strongest known rank (r4 71 < r3 98,
  TSA r5 50 < r4 75): lower ranks stay castable.
- **Beast Mastery**: cat and wolf pets (`summon_pet,type=cat|wolf`): melee, Claw, Bite, Furious Howl, focus,
  AP from the hunter's ranged AP, Unleashed Fury, Ferocity, Frenzy, Bestial Wrath, Intimidation, Focused
  Fire, Bestial Discipline focus. Summon Hawk: dive bomb (bp + 5% RAP) plus up to 2 hawks for 18 s.
- **Marksmanship**: Lone Wolf (+20% with no active pet), Ranged Weapon Specialization (ranged attacks only),
  Rapid Recuperation (Serpent Sting -> regen while casting).
- **Survival**: Raptor Strike (on next swing), Mongoose Bite (Expose Prey only), Strider Kick, Lacerating
  Strikes (residual bleed, 40% of Mongoose Bite), Lacerate, Wing Clip, Resourcefulness, Improved Tracking,
  Surefooted (+hit), Savage Strikes and Predator's Edge (data).
- **APL**: talent-picked tree (Survival talents -> melee SV, Lone Wolf -> MM, otherwise BM with a pet). Viper
  below 15% mana, Hawk back at 60%; Aimed/Sniper cast right after an Auto Shot (clip at most 0.5 s).

## ESTIMATES (search the source for ESTIMATE)

1. Pet: melee 2.0 s for level x 1.0 +-20%, AP = 22% of hunter ranged AP (TBC rule), 5% crit, no hit from the
   owner, focus 100 (+50 from 415429 data) at 5/s, happy x1.25.
2. Hawks: one attack per 2 s per hawk for 30% of the dive bomb damage, hunter crit + Ferocity.
3. Viper mana on autos: 1% base mana per second of weapon speed.
4. Shot cast times not reduced by ranged haste.
5. Base crit 0 beyond Agility; base stats L2-60 are the foundation rows.
6. Improved Tracking: always on (`tracking=0` to disable); crit damage bonus from the second curve (FLAG).
7. Profiles: ammo 17.5 DPS (Thorium arrows), 15% quiver at 60; 7.5 / 10% at 25.
8. Aspect of the Falcon: Quick Strikes from melee and Quick Shots from Auto Shot at the Deadly Aspects chance.

## Not implemented (spec items)

- Per-family pet table (speed modifiers, family abilities beyond cat/wolf), pet happiness decay, Prowl.
- Counterattack, Deterrence, Scatter Shot, traps, stings other than Serpent, Concussive/Distracting/
  Tranquilizing Shot, Kill Command/Readiness (not obtainable).
- Rapid Killing's on-kill damage buff and Rapid Recuperation 100% after it (no kills on Patchwerk).
- Mongoose Bite after the hunter dodges (no incoming damage), melee+ranged weaving, the 8 yd dead zone.
- Off-hand melee (Predator's Edge off-hand part), racial weapon/bow skills, movement blocking the aim.
- Spirit regen formula and pet scaling not validated for hunters (V-H6/V-H7).

## Proposed shared-engine changes

1. `player_t::create_options`: a `ranged=` slot option and `util::slot_type_string(SLOT_RANGED)`
   (the module binds `ranged=` itself).
2. Hit: `composite_melee_hit()` ignores hit rating (line commented out) and aura 54 "Modify Hit%" has no
   passive mapping; the module adds both locally (Surefooted, gear hit).
3. A real ranged attack power stat (ITEM_MOD_RANGED_ATTACK_POWER maps to melee AP; Battle Shout's AP
   multiplier also lifts ranged AP).
4. `ranged_attack_t::schedule_execute` is an old copy of `action_t::schedule_execute` (no `start_gcd()`,
   cast-while-casting poll); channels through it error, so Volley is a spell.
5. Aura 580/598 (AP from % Intellect, Careful Aim) in the passive parser.
6. A shared on-next-swing queue (Raptor Strike copies the warrior pattern).

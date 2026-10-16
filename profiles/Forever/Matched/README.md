# Matched-gear class comparison (level 60, Forever 1.60.1.70425)

One profile per DPS tree, all built from the **same stat budget** so the classes can be compared.
The class sample profiles elsewhere in `profiles/` each use their own invented gear and can't be compared.

None of these profiles use real items. Each profile sets its stat totals with `gear_<stat>=` (these replace
the summed item stats) and its weapons with `weapon=<type>_<speed>speed_<min>min_<max>max`. Base stats
for levels 31-60 are still the vanilla ESTIMATE rows, so absolute numbers are only as good as those.

## Stat budget

| | Physical (melee and hunters) | Caster |
|---|---|---|
| Primary stats | 320 points of Str + Agi (+ Int for mana users), split below | Int 150, Spirit 100 |
| Stamina | 250 | 250 |
| Spell power (`+spell damage`) | 0 | 280 |
| Hit rating (10 = 1%) | 40 (4%) | 30 (3%) |
| Crit rating | 28 | 28 |
| Attack power, haste, mp5, armor | 0 | 0 |

Physical split of the 320 points, following each class's real itemization:

| Group | Str | Agi | Int | Profiles |
|---|---|---|---|---|
| Strength melee | 200 | 120 | 0 | warrior arms, fury |
| Agility melee | 120 | 200 | 0 | rogue x3, feral druid |
| Strength hybrid (pays for mana) | 170 | 90 | 60 | retribution paladin, enhancement shaman |
| Hunter (Strength adds no ranged AP) | 0 | 260 | 60 | hunter x3 |

Casters: mage x3, shadow priest, balance druid, elemental shaman, warlock x3.

## Weapons (DPS matched per weapon type)

| Type | Speed | Damage | DPS | Used by |
|---|---|---|---|---|
| Two-hand | 3.60 | 180-252 | 60.0 | arms (axe), retribution (sword), enhancement and feral (mace), hunters (polearm), casters (staff, stats in the budget) |
| One-hand, main hand | 2.60 | 95-134 | 44.0 | fury (axe), combat (sword) |
| One-hand, fast | 1.80 | 66-92 | 43.9 | daggers (assassination, subtlety, both hands), fury and combat off hands |
| Bow | 3.00 | 100-140 | 40.0 | hunters |

Hunter kit (vanilla ESTIMATE): `ammo_dps=17.5` (Thorium Headed Arrow) and `quiver_haste=15`.
Shadow priest: the module's own wand option `priest.wand=64:120:1.7:arcane` (ESTIMATE, about 54 DPS), fired
only when nothing else is affordable. The mage and warlock modules have no wand action.

## Race

Horde races without a damage racial: **Undead** (warrior, rogue, priest, mage, warlock, paladin) and
**Tauren** (druid, hunter, shaman, which can't be Undead).

## Talents and action lists

- Talents use `class_talents=`. Mage, priest, druid, hunter, rogue and paladin reuse the builds of the class
  sample profiles (each file names its source). Warrior, shaman and warlock builds are new here:
  Arms 31/20/0, Fury 17/34/0, Elemental 31/20/0, Enhancement 20/31/0, Affliction 31/0/20,
  Demonology 0/31/20, Destruction 17/0/34.
- Every profile uses its class's **default APL**. The only exception is the mage: its default APL drinks a
  mana potion and a Dark Rune on every 60 s cooldown, and no other class does. The mage profiles therefore
  carry the default APL with only those two lines removed (mana gems and Evocation stay).
- Some trees are limited by what the module implements today: no Lava Burst, Demonic Sacrifice, Conflagrate,
  Shadowburn, Siphon Life or Wrack actions, and only the imp as a warlock pet. The shaman APL is shared, so
  elemental also melees with Windfury Weapon on its staff.

## Consumables and buffs

- No flasks, elixirs, food, potions, runes or weapon oils for anyone. Class-made items (mana gems, poisons,
  shaman weapon imbues) are class abilities and stay.
- Raid buffs: the default `optimal_raid=1` (flat best-rank Arcane Intellect, Mark of the Wild, Fortitude,
  Battle Shout; a class that casts one of these casts its own).

## How to run

Same settings for every profile:

```bash
simc <profile>.simc iterations=2000 max_time=300 fight_style=Patchwerk target_level=63
```

Results and commentary for 2026-10-16 are in the Forever Hub repo, `docs/class-comparison-2026-10-16.md`.

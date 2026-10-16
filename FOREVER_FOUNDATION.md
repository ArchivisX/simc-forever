# Forever foundation: contract for class modules

Branch `foundation` of the ArchivisX fork of simc-forever (based on upstream `main` @ `7b243543e7`,
2026-10-15). This file is the contract for the six new class modules (mage, priest, druid, hunter,
rogue, paladin). It may be deleted before anything goes upstream.

Everything below is either already in the engine (use it, don't re-implement) or a helper in
`engine/class_modules/forever_helpers.hpp` (namespace `forever`). File/function names are exact.

## 1. Module skeleton (copy upstream's style exactly)

Follow the warrior/shaman/warlock modules as refactored by the upstream dev on 2026-10-12/13. Read
`git log -p -- engine/class_modules/sc_warlock.cpp`.

- `#include "simulationcraft.hpp"`, `#include "action/parse_effects.hpp"`,
  `#include "class_modules/apl/<class>.hpp"`, plus `#include "class_modules/forever_helpers.hpp"`.
- `template <class Base> struct <class>_action_t : public parse_action_effects_t<Base>` with
  `private: using ab = parse_action_effects_t<Base>; public: using base_t = <class>_action_t<Base>;`,
  a forwarding constructor and `p()` const/non-const accessors.
  `using <class>_spell_t = <class>_action_t<spell_t>;` and `<class>_attack_t = ...<melee_attack_t>`.
- `init_spells()`: call `player_t::init_spells()`, then `parse_all_class_passives(); parse_all_passive_talents();`.
  Passive talents and class passives then apply automatically, including per-rank values.
- `init_action_list()`: same pattern as the warlock module, with the APL in `engine/class_modules/apl/<class>.cpp`
  between the `//<class>_apl_start` and `//<class>_apl_end` markers. `default_potion/flask/food` return `"disabled"`.
- Abilities: `find_spell( max_rank( player, <spell>_ranks ).spell_id )`. Values (damage, cost, cast time,
  duration, tick time, SP/AP coefficients, per-level scaling) come from spell data automatically. Do **not**
  port numbers from classic wikis or wowsims. Forever changed many of them, for example Shadow Bolt R10
  is 268 base and both R9 and R10 are level 60.

## 2. Shared helpers (`engine/class_modules/forever_helpers.hpp`)

| Helper | What it does |
|---|---|
| `forever::spell_rank_t{ id }` | One rank. For multi-id ranks, define your own struct with a `spell_id` member, like shaman's `totem_rank_t`. |
| `forever::max_rank( player, std::array ranks )` | The highest rank the player knows. `player_t::find_spell(id)` rejects spells above the player's level. Ties on spell level go to the later list entry, so list ranks in rank order. Same semantics as the module-local copies. |
| `forever::max_rank_spell( player, ranks )` | `find_spell( max_rank(...).spell_id )` in one call. |
| `forever::rank_spells( player, "Client Name" )` | All "Rank N" spells with that exact name in the player's (or pet owner's) class spell family, ordered worst to best: level, then rank number, then lowest id. Scans all spell data, so call it at init only. |
| `forever::find_rank_spell( player, "Client Name" )` | The best known rank by name, or `spell_data_t::not_found()`. Verified at L60: Shadow Bolt 25307, Immolate 25309, Drain Life 11700 (not the SoD copy 403689). |
| `forever::rank_number( player, spell )` | `"Rank 7"` → 7, and 0 if the spell has no rank text. |
| `forever::talent( player, "Talent Name" )` / `forever::talent_rank( player, name )` | `find_talent_spell( talent_tree::CLASS, name )` and its `.rank()`. Use `.ok()` to gate talent abilities. |
| `forever::base_mana( player )` | Class base mana at the player's level (`dbc->resource_base`), for "X% of base mana" costs. |
| `forever::classic_direct_coefficient( cast_time )`, `classic_dot_coefficient( duration )`, `classic_channel_coefficient( duration )` | Vanilla SP coefficient fallbacks: cast/3.5 (cast clamped to 1.5–3.5), duration/15, duration/3.5. Use them **only** when the data coefficient is 0, and leave a comment saying you did. |
| `forever::classic_low_level_penalty( spell )` | 1 − (20 − level)·0.0375 for spells below level 20. The engine applies it when `classic_low_rank_penalty=1`. |
| `forever::rage_conversion( level )`, `rage_from_damage_dealt( level, dmg )`, `rage_from_damage_taken( level, dmg )` | Vanilla rage: c = 0.0091107836·L² + 3.225598133·L + 4.2652911; dealt 7.5·d/c; taken 2.5·d/c. |
| `forever::forever_rage_per_swing( weapon )` | The **measured** Forever warrior white-swing rage: speed × 3.45 (1H) or × 4.5 (2H), doubled on crit by the caller. `sc_warrior.cpp` uses this, not the damage formula. A bear druid should start with this one. |

**Talents.** `talents=` strings from the simc addon parse into Trait class-tree nodes; the
`player_t::init_talents` debug log shows `adding class talent X rank=a/b`. The import string carries the
whole selection, so milestone nodes (11/16/21/31 points; the 16-point node is new in Forever) arrive
like any other node. simc does not enforce point gates. Per-rank values come from the trait curves
(op=set) automatically. For profiles, copy real `talents=` lines from addon exports
(`profiles/LowLevel/*.simc`).

## 3. Engine rules now in place (sim options, all `classic_*`)

The rules are toggled with sim options, all named `classic_*`. All but two default to on.
Sources: magey/classic-warrior wiki "Attack table", wowsims/classic `NewAttackTable`
(sim/core/target.go), the royalgiraffe resist guide, wowwiki Rage/Spirit, and vmangos/cmangos.

| Option (default) | Rule | Where |
|---|---|---|
| `classic_weapon_skill=1` | Miss vs NPCs: +0.1% per defense point over weapon skill, or +0.2% per point once the gap exceeds 10. That is 8% vs +3 at 300 skill; there is no hit suppression and no reduction vs lower targets. Dodge: target dodge + 0.1% per point (6.5% vs +3). Parry is unchanged from upstream (14% vs +3, front only). | `attack_t::miss_chance/dodge_chance` |
| `classic_glancing=1` | White melee vs NPCs glances at 10% + 2% per defense point over 5·level (10/20/30/40% at +0..+3). Damage multiplier is 0.55–0.75 (avg 65%) at a 15-point skill gap, 0.80–0.90 at 10, and 0.91–0.99 at 5. | `melee_attack_t::glance_chance`, `action_t::calculate_direct_amount` |
| `classic_dual_wield_miss=1` | White swings miss 19% more often while `dual_wield()` (27% vs +3). | `attack_t::miss_chance` |
| `classic_crit_suppression=1` | Melee crit vs NPCs: −0.2% per defense point over 5·level (−0.04% under), and −1.8% more at +3. That is −4.8% vs a boss. In-game the 1.8% applies only to aura crit, so low-crit actors are over-suppressed. | `attack_t::classic_crit_suppression()` |
| `classic_partial_resists=1` | Non-binary magic spells vs higher-level NPCs lose 2% per level on average (6% at +3). Physical, holy and `spell_t::binary_resist = true` spells are exempt. Resistance scores are not modelled. | `spell_t::classic_average_resist()` |
| `classic_energy_tick=1` | Energy arrives in 2 s ticks: 20 per tick at 10/s, and buffs to regen still apply. Threshold wakeups land on the tick. | `player_t::regen`, `min_threshold_trigger` |
| `classic_vanilla_spirit_regen=0` | 1 switches to the 1.12 per-class spirit formulas, per 2 s: druid/hunter/paladin 15+spi/5, mage/priest 13+spi/4, shaman 17+spi/5, warlock 8+spi/4. The default is the **measured Forever** formula: 0.25/s per spirit for the first 50, then 0.125/s. It matches all three addon exports (L10 warlock, L11 warrior, L20 shaman). Verify it for your class. | `player_t::mana_regen_from_spirit` |
| `classic_low_rank_penalty=0` | 1 applies the vanilla below-level-20 SP coefficient penalty to player spells. | `action_t::init` |

These rules already existed upstream and are unchanged:
- Spell miss: 4/5/6% at +0/+1/+2 and 17% at +3, minimum 1% (`spell_t::miss_chance`).
- Spell crit is ×1.5.
- Crit from agility and intellect per class and level comes from `PlayerExpectedStat` data (`base.attack_crit_per_agility`, `base.spell_crit_per_intellect`).
- Base 5% dodge and miss.

**Player hooks** (virtual, in `player_t`; override them in your class):
- `composite_weapon_skill( const weapon_t* )`: defaults to 5·level plus the player option
  `weapon_skill_bonus=`. Racial weapon skills (Human swords and maces, Orc axes, …) are not applied;
  override if needed.
- `composite_mana_regen_while_casting()`: the fraction of spirit regen kept inside the five second rule.
  It defaults to `classic.mana_regen_while_casting`, which comes from **aura 134** passives such as
  Meditation and Arcane Meditation. Add buff values here: Mage Armor (aura 134 50%), Innervate (134 100%).
- `composite_spirit_regen_multiplier()`: defaults to 1. Use it for Innervate, whose aura 110 is +400%,
  so return 5.
- `composite_mp5()`: flat mana per 5 s that ignores the five second rule. It sums item mod 43, aura 85
  passives and the player option `mp5=`.
- Helpers: `mana_regen_from_spirit()`, `mana_regen_while_casting()`, `mana_regen_from_mp5()`,
  `recent_cast()` (the five second rule).

The five second rule starts when mana is spent: `player_t::resource_loss` sets `last_cast`. Gaining
mana (Life Tap, potions) does not start it.

**Weapon damage.** `weapon_multiplier = 1` gives weapon damage + speed·AP/14. Use
`normalize_weapon_speed = true` for classic instants (1.7 dagger, 2.4 1H, 3.3 2H, 2.8 ranged).
`player_t::default_ap_type()` is now `NO_WEAPON`: classic AP has no weapon-DPS part, and
coefficient-based abilities scale with plain AP.

Attack power per class is not in the core yet. Set it in `init_base_stats` before calling
`player_t::init_base_stats()` (`base.attack_power_per_strength/agility`), then set
`base.stats.attack_power` after. Vanilla values:

| Class | Melee AP | Ranged AP |
|---|---|---|
| Warrior, paladin | Str·2 + 3·L − 20 | |
| Shaman | Str·2 + 2·L − 20 | |
| Rogue | Str + Agi + 2·L − 20 | |
| Hunter | Str + Agi + 2·L − 20 | Agi·2 + 2·L − 10 |
| Druid | Str·2 − 20 (forms add) | |
| Mage, priest, warlock | Str − 10 | |

Check yours against the addon export's `# attack_power=` line.

**Items.**
- Item mod 42, "+X spell damage", now counts as spell power. simc has a single SP stat, so it also boosts heals.
- Item mod 43, mp5, now counts.
- **Still ignored:** healing-only (41) and per-school damage (85 fire, 87 frost, 88 shadow, 89 likely arcane, 86 likely nature).
- Crit and hit items are ratings, for example Robe of the Archmage's "+1% crit" is 14 crit rating, about +1.1% spell crit at L60 through the client combat rating tables.

**Consumables.**
- `mana_potion,type=auto|minor|lesser|mana|greater|superior|major` and `dark_rune,type=dark|demonic`.
- Values come from the Restore Mana spell data. The `potion` and `rune` cooldowns are 60 s in 1.60.1 data.
- Only minor, greater, superior and major exist in the spell data (70334 and 70425); `lesser` and `mana` fail with an error.
- Item potions via `potion=` don't work, because consumables aren't in the generated item data.

**Damage ranges.** Classic effects have no scaling class. Their range is now the Variance fraction of the
base value: min/max = avg·(1 ∓ delta/2). Shadow Bolt R10 is 268 ± 5.5%. simc's default is
`average_range=1`, which always rolls the average; pass `average_range=0` to see ranges.

**Report.** Landed zero-damage DoT applications are recorded, so the Miss/Dodge % of Rend or Corruption
is now of all attempts. Spell hit/miss talents (Add Flat Modifier property 16) are parsed into
`hit_chance_effects`.

## 3b. What the existing classes now have (examples to copy)

- **Warlock**: Life Tap (Dummy amount → mana, health cost), Bane of Agony (Forever's name for Curse of
  Agony; per-tick ramp via `calculate_tick_amount`), Searing Pain, channeled Drain Life
  (`channeled = true`), and an APL with Life Tap. The imp's mana regen is scaled above L10.
- **Warrior**:
  - Overpower: a dodge-triggered 5 s window, the vanilla value; the data has none.
  - Execute: extra rage → damage from the effect's chain amplitude.
  - Slam (Forever: 18 s cooldown).
  - Mortal Strike and Bloodthirst, gated on the talent's `.ok()`.
  - Whirlwind.
  - Cleave, through the generalised on-next-swing queue `heroic_strike_queue_t` pattern.
  - Sunder Armor (own attacks only; aura 674 is unknown to simc).
- **Shaman**:
  - Self buffs can't miss (`may_miss = false` for non-harmful self casts; do the same in your class).
  - Lightning Bolt, Chain Lightning and Frost Shock, with explicit rank ids to avoid SoD duplicates.
  - Stormstrike with its +20% nature charge. The talent spell keeps spell level 40, so the module gates
    on the talent node instead.
  - Windfury Weapon: 20% and a 1.5 s ICD from driver 439431, 2 extra attacks.
- **Season of Discovery leftovers** sit in the data next to the real ranks under the same name (Drain
  Life 403677+, Lightning Bolt 408439+, Slam 1310196+). Use explicit id lists or `find_rank_spell`, and
  check `SkillLineAbility.AcquireMethod` (3 = not obtainable) in our DB2 export.

## 4. Base stats

`engine/dbc/sc_extra_data.inc` `__gt_class_stats_by_level` has hand-measured beta rows, which are
untouched and authoritative. Every other row up to level 60 is marked `// ESTIMATE`. Those rows are
original 1.12 values from cmangos classic-db `player_levelstats` (commit `28ef6259`), made race-neutral.

The measured rows match vanilla exactly for Str, Agi, Sta and Int, and Spirit is +0/+1. The L20 shaman
estimate reproduces the addon sheet within ±2 per stat.

Rows ≤30 will be replaced by beta measurements and rows 31–60 by measured launch values (launch
2026-11-04). Base mana and health per level are client data and cover 1–60.

## 5. Known gaps / risks (don't assume these work)

- **Game data is build 1.60.1.70425** (commit `[dbc] Update Forever game data to 1.60.1.70425`, the upstream
  data bot's output from simulationcraft/simc#11959, cherry-picked). Arcane Wrath, Frost Blast and Dark Inspiration
  are present. `SpellDataDump/*.txt` is regenerated too, so grep it for ids, ranks and coefficients.
- Resistance scores are not modelled; binary spells have no extra miss. There is no spell crit suppression.
- DoT ticks are not affected by melee crit suppression. Rend ticks can crit because the data says
  "Periodic Can Crit".
- `profiles/CI.simc` (all three beta characters in one raid) fights a level 23 target, so the L10 warlock misses 100%.
  The same happens upstream. Sim classes alone for meaningful numbers.
- Not implemented:
  - Racial weapon skill
  - Touch of the Grave (disabled upstream: wrong spell id)
  - Aura 174 (spell damage from % of a stat)
  - Combo points on the target (simc's are on the player; this only matters when switching targets)
  - Auto Shot clipping and ammo
  - Temporary-enchant procs (poisons)
  - A shared on-next-swing queue (see warrior `heroic_strike_queue_t`)
- Imp mana regen is a level-7 fit, scaled by max mana above level 10. The imp's HP and mana tables are guesses.
- The default Forever spirit formula is measured on 3 characters at L10–20. 60 is extrapolated.
- Low rank SP penalty: Forever's rule is unknown, so the option defaults off.
- Holy is excluded from level-based partial resists. That exclusion is unverified.

## 6. How to build and run the sanity sims

```bash
cd /var/home/swike/dev/simc-forever-wt/<your worktree>
nice -n 19 make -C engine release -j4 SC_NO_NETWORKING=1 CXX=g++      # never more than -j6 total
engine/simc profiles/LowLevel/warrior-11.simc iterations=500          # upstream beta characters
engine/simc profiles/LowLevel/shaman-20.simc  iterations=500
engine/simc profiles/LowLevel/warlock-10.simc iterations=500
engine/simc profiles/Sanity/warrior-60-melee.simc  iterations=1000    # Miss ~8, Dodge ~6.5, Glance ~40%
engine/simc profiles/Sanity/warrior-60-dual-wield.simc iterations=1000 # white Miss ~27%
engine/simc profiles/Sanity/warlock-60-mana.simc   iterations=1000    # Gains: Mana Regen/mana_potion/dark_rune
engine/simc profiles/Sanity/warlock-60-mana.simc iterations=1 max_time=40 log=1 | grep "Mana Regen"   # 5SR: mp5 only
```

**Per-class acceptance**:
1. Your class's beta addon export (if any) runs without errors.
2. The report's Core/Attack/Spell stats match the export's `# character_stats` / `# attack_power` /
   `# crit` / `# regen` comment lines.
3. An L60 profile shows every implemented ability with Count > 0 and plausible Miss/Glance/Resist.
4. Mana or energy behaves: Waiting% sane, OOM plausible for casters.

Toggle any `classic_*` option to 0 to compare against upstream behaviour.

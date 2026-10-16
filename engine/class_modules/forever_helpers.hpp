// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
//
// Shared helpers for WoW: Forever class modules (classic ranked spells, talents, mana, rage,
// spell coefficients). Header only, include after simulationcraft.hpp. See FOREVER_FOUNDATION.md.

#pragma once

#include "simulationcraft.hpp"
#include "dbc/spelltext_data.hpp"

#include <algorithm>
#include <cmath>
#include <string_view>
#include <vector>

namespace forever
{
// ==========================================================================
// Ranked spells
// ==========================================================================

// One rank of a spell, by spell id. Modules with extra per-rank ids (buff/driver/totem aura) define
// their own struct with a spell_id member and use max_rank() the same way.
struct spell_rank_t
{
  unsigned spell_id;
};

// Highest rank the player knows: player_t::find_spell( id ) only returns spells whose spell level is
// at or below the player's level, so the best ok() rank by spell level wins. Returns a value
// initialized rank (spell_id 0, i.e. find_spell -> not_found) if no rank is available.
template <typename RANKS>
auto max_rank( const player_t* player, const RANKS& ranks )
{
  typename RANKS::value_type best {};
  unsigned best_level = 0;

  for ( const auto& rank : ranks )
  {
    const spell_data_t* spell = player->find_spell( rank.spell_id );

    if ( spell->ok() && spell->level() >= best_level )
    {
      best       = rank;
      best_level = spell->level();
    }
  }

  return best;
}

// Convenience: spell data of the highest known rank in an id list.
template <typename RANKS>
const spell_data_t* max_rank_spell( const player_t* player, const RANKS& ranks )
{
  return player->find_spell( max_rank( player, ranks ).spell_id );
}

// Rank number from the client rank text ("Rank 7" -> 7), 0 if the spell has no rank text.
inline unsigned rank_number( const player_t* player, const spell_data_t* spell )
{
  const char* text = player->dbc->spell_text( spell->id() ).rank();
  if ( !text )
    return 0;

  std::string_view sv( text );
  if ( sv.rfind( "Rank ", 0 ) != 0 )
    return 0;

  return util::to_unsigned( std::string( sv.substr( 5 ) ) );
}

// All ranks of a class spell by name, best last, regardless of player level.
// Matches the client spell name exactly and the player's (or a pet's owner's) class spell family, and
// keeps only spells with "Rank N" text unless the name has no ranked entries at all. Spells with
// SkillLineAbility AcquireMethod 3 (Season of Discovery leftovers) are not filtered here; prefer
// explicit id lists (max_rank) when a name is ambiguous. Scans all spell data, call at init only.
inline std::vector<const spell_data_t*> rank_spells( const player_t* player, std::string_view name )
{
  const player_t* owner = player->get_owner_or_self();
  int family            = dbc::get_class_spell_family( owner->type );

  std::vector<const spell_data_t*> ranked, unranked;
  for ( const spell_data_t& spell : spell_data_t::data( player->dbc->ptr ) )
  {
    if ( spell.name_cstr() == nullptr || name != spell.name_cstr() )
      continue;
    if ( family != 0 && as<int>( spell.class_family() ) != family )
      continue;

    if ( rank_number( player, &spell ) > 0 )
      ranked.push_back( &spell );
    else
      unranked.push_back( &spell );
  }

  // Order: spell level, then rank number (Forever has several ranks at the same level, e.g. Shadow Bolt
  // 9 and 10 are both level 60), then the higher spell id first so that among duplicates of the same rank
  // (e.g. Season of Discovery copies) the original, lowest id ends up last = preferred.
  auto& out = ranked.empty() ? unranked : ranked;
  range::sort( out, [ player ]( const spell_data_t* a, const spell_data_t* b ) {
    if ( a->level() != b->level() )
      return a->level() < b->level();
    unsigned ra = rank_number( player, a ), rb = rank_number( player, b );
    if ( ra != rb )
      return ra < rb;
    return a->id() > b->id();
  } );
  return out;
}

// Highest rank of a class spell by name that the player knows (spell level <= player level), or
// spell_data_t::not_found(). Ties on spell level go to the higher rank number, then the lowest spell id.
inline const spell_data_t* find_rank_spell( const player_t* player, std::string_view name )
{
  const spell_data_t* best = spell_data_t::not_found();

  for ( const spell_data_t* spell : rank_spells( player, name ) )
  {
    const spell_data_t* known = player->find_spell( spell->id() );
    if ( known->ok() )
      best = known;  // rank_spells() is ordered worst to best
  }

  return best;
}

// ==========================================================================
// Talents
// ==========================================================================

// Forever classic 3-tree talents are retail Trait class-tree nodes: the talents= import string from
// the addon selects nodes and ranks, and player_t::init_talents applies the per-rank curve values.
// Milestone nodes (11/16/21/31 points) are ordinary single-rank nodes in the same tree.
inline player_talent_t talent( const player_t* player, std::string_view name )
{
  return player->find_talent_spell( talent_tree::CLASS, name );
}

// Selected rank of a talent by name, 0 if not taken.
inline unsigned talent_rank( const player_t* player, std::string_view name )
{
  return talent( player, name ).rank();
}

// ==========================================================================
// Mana
// ==========================================================================

// Class base mana at the player's level (client data), for "X% of base mana" costs.
inline double base_mana( const player_t* player )
{
  return player->dbc->resource_base( player->type, player->level() );
}

// ==========================================================================
// Spell power coefficients (fallbacks only)
// ==========================================================================

// Prefer the spell data coefficients (SP Coefficient on the effect, picked up automatically as
// spell_power_mod.direct/tick). Forever client data has them for the class spells checked so far
// (e.g. Shadow Bolt R1 0.486, Corruption R1 0.2). Use these vanilla rules only when data has 0.

// Direct spells: cast time / 3.5 s, cast time clamped to [1.5, 3.5] s.
inline double classic_direct_coefficient( timespan_t cast_time )
{
  return std::clamp( cast_time.total_seconds(), 1.5, 3.5 ) / 3.5;
}

// Pure DoTs: duration / 15 s, capped at 1. Channels: duration / 3.5 s, capped at 1.
inline double classic_dot_coefficient( timespan_t duration )
{
  return std::min( duration.total_seconds() / 15.0, 1.0 );
}

inline double classic_channel_coefficient( timespan_t duration )
{
  return std::min( duration.total_seconds() / 3.5, 1.0 );
}

// Spells learned below level 20 get 1 - (20 - spell level) * 0.0375 of the coefficient (vanilla).
// Forever's low rank rule is unknown (Oct 1 2026 beta notes only say "less"); the engine applies this
// to all player spells when the sim option classic_low_rank_penalty=1 (default off).
inline double classic_low_level_penalty( const spell_data_t* spell )
{
  if ( !spell->ok() || spell->level() == 0 || spell->level() >= 20 )
    return 1.0;

  return std::max( 0.0, 1.0 - ( 20.0 - spell->level() ) * 0.0375 );
}

// ==========================================================================
// Rage
// ==========================================================================

// Vanilla rage conversion value c(level) (wowwiki "Rage", patch 1.12).
inline double rage_conversion( int level )
{
  return 0.0091107836 * level * level + 3.225598133 * level + 4.2652911;
}

// Vanilla rage from dealing / taking damage.
inline double rage_from_damage_dealt( int level, double damage )
{
  return 7.5 * damage / rage_conversion( level );
}

inline double rage_from_damage_taken( int level, double damage )
{
  return 2.5 * damage / rage_conversion( level );
}

// Rage per landed white swing as measured on the Forever beta for warriors (Dave, 2026-10): weapon
// speed times 3.45 (one hand) or 4.5 (two hand), doubled on crit by the caller. This is what
// sc_warrior.cpp uses; Forever does NOT appear to use the damage based vanilla formula above.
inline double forever_rage_per_swing( const weapon_t& w )
{
  double mult = w.group() == WEAPON_2H ? 4.5 : 3.45;
  return w.swing_time.total_seconds() * mult;
}
}  // namespace forever

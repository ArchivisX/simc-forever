// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "player/talent.hpp"
#include "player/player.hpp"
#include "dbc/dbc.hpp"
#include "sim/sim.hpp"

// Invalid, trait does not exist in db2
player_talent_t::player_talent_t() :
  m_player( nullptr ), m_trait( &( trait_data_t::nil() ) ), m_spell( spell_data_t::not_found() ),
  m_rank( 0 )
{ }

// Not found, trait exists but is not found amongst player's selected traits
player_talent_t::player_talent_t( const player_t* player ) :
  m_player( player ), m_trait( &( trait_data_t::nil() ) ), m_spell( spell_data_t::not_found() ),
  m_rank( 0 )
{ }

// Found and valid. Forever: a selected trait is known regardless of the spell's level (talent trees are gated
// by points spent, and Forever trait spells keep their classic spell level, e.g. Stormstrike 17364 is level 40 but
// can be taken at level 20), so skip player_t::find_spell()'s level check.
player_talent_t::player_talent_t( const player_t* player, const trait_data_t* trait, unsigned rank ) :
  m_player( player ), m_trait( trait ),
  m_spell( trait->id_spell ? dbc::find_spell( player, trait->id_spell ) : spell_data_t::not_found() ),
  m_rank ( rank )
{ }

const spell_data_t* player_talent_t::find_override_spell( bool require_talent ) const
{
  return ( !require_talent || enabled() ) ? m_player->find_spell( m_trait->id_override_spell ) : spell_data_t::not_found();
}

const spell_data_t* player_talent_t::find_replaced_spell( bool require_talent ) const
{
  return ( !require_talent || enabled() ) ? m_player->find_spell( m_trait->id_replace_spell ) : spell_data_t::not_found();
}

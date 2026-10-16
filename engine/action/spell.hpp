// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "spell_base.hpp"

struct spell_t : public spell_base_t
{
public:
  /// Forever: classic binary spell (all-or-nothing resist, no partial resists). Set in class modules for
  /// spells with non-damage effects (e.g. Frostbolt's slow) once confirmed in-game.
  bool binary_resist = false;

  spell_t( util::string_view token, player_t* p );
  spell_t( util::string_view token, player_t* p, const spell_data_t* s );

  // Harmful Spell Overrides
  result_amount_type amount_type( const action_state_t* /* state */, bool /* periodic */ = false ) const override;
  result_amount_type report_amount_type( const action_state_t* /* state */ ) const override;
  double miss_chance( double hit, player_t* t ) const override;
  double composite_hit() const override;
  double composite_target_multiplier( player_t* ) const override;
  /// Forever: average damage lost to classic partial resists against the target, 0..0.75
  virtual double classic_average_resist( const player_t* target ) const;
};

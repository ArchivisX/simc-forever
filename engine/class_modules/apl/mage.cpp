#include "class_modules/apl/mage.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace mage_apl
{
std::string potion( const player_t* )
{
  return "disabled";
}

std::string flask( const player_t* )
{
  return "disabled";
}

std::string food( const player_t* )
{
  return "disabled";
}

std::string rune( const player_t* )
{
  return "disabled";
}

std::string temporary_enchant( const player_t* )
{
  return "disabled";
}
//mage_apl_start
namespace
{
// Shared precombat: Arcane Intellect, the best armor for the level, and the mana block
void precombat_and_mana( player_t* p )
{
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* mana      = p->get_action_priority_list( "mana" );

  precombat->add_action( "arcane_intellect" );
  if ( p->true_level >= 34 )
    precombat->add_action( "mage_armor", "Mage Armor: 50% of spirit regen keeps ticking while casting" );
  else if ( p->true_level >= 30 )
    precombat->add_action( "ice_armor" );
  else
    precombat->add_action( "frost_armor" );
  precombat->add_action( "snapshot_stats" );

  mana->add_action( "mana_gem", "Mana gems (one of each the mage can conjure, 60 s shared cooldown) when the whole gem fits" );
  mana->add_action( "mana_potion", "Potions and runes share 60 s category cooldowns in Forever 1.60.1 data" );
  if ( p->true_level >= 58 )
    mana->add_action( "dark_rune" );
  mana->add_action( "evocation,if=mana.pct<15" );
}
}  // namespace

void arcane( player_t* p )
{
  precombat_and_mana( p );

  action_priority_list_t* default_ = p->get_action_priority_list( "default" );

  default_->add_action( "call_action_list,name=mana" );
  default_->add_action( "arcane_power,if=buff.arcane_blast.stack>=3" );
  default_->add_action( "presence_of_mind,if=cooldown.arcane_wrath.ready" );
  default_->add_action( "arcane_wrath", "Arcane Wrath: 1.0 coefficient + 5% of max mana as spell damage, 6 s cooldown" );
  default_->add_action( "arcane_missiles,if=buff.missile_barrage.react" );
  default_->add_action( "arcane_blast,if=buff.arcane_blast.stack<3|buff.clearcasting.react",
                        "Build the Arcane Blast stack (Intellect as spell damage, not refreshed), spend it on Wrath and Missiles" );
  default_->add_action( "arcane_missiles,if=mana.pct>30" );
  default_->add_action( "frostbolt" );
}

void fire( player_t* p )
{
  precombat_and_mana( p );

  action_priority_list_t* default_ = p->get_action_priority_list( "default" );

  default_->add_action( "call_action_list,name=mana" );
  default_->add_action( "combustion" );
  default_->add_action( "scorch,if=talent.improved_scorch&(debuff.fire_vulnerability.stack<5|debuff.fire_vulnerability.remains<4)",
                        "Improved Scorch: Fire Vulnerability is caster-only in Forever, keep 5 stacks up" );
  default_->add_action( "pyroblast,if=buff.heating_up.stack>=3", "Heating Up: 3 stacks make Pyroblast a 1.5 s cast" );
  default_->add_action( "fire_blast,if=talent.heating_up&mana.pct>30" );
  default_->add_action( "fireball" );
}

void frost( player_t* p )
{
  precombat_and_mana( p );

  action_priority_list_t* default_ = p->get_action_priority_list( "default" );

  default_->add_action( "call_action_list,name=mana" );
  default_->add_action( "cold_snap,if=cooldown.cone_of_cold.remains>5&debuff.winters_chill.stack>=debuff.winters_chill.max_stack" );
  default_->add_action( "cone_of_cold,if=!talent.winters_chill|debuff.winters_chill.stack>=debuff.winters_chill.max_stack",
                        "Winter's Chill: each stack adds 5% damage and 7% coefficient to the next Cone of Cold "
                        "(needs mage.cone_of_cold_in_range=1, the default)" );
  default_->add_action( "ice_lance,if=buff.fingers_of_frost.react|target_frozen" );
  default_->add_action( "frostbolt" );
}
//mage_apl_end
}  // namespace mage_apl

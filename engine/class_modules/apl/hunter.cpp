#include "class_modules/apl/hunter.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace hunter_apl
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
//hunter_apl_start
// Forever 2026-10-16. The tree is picked from the talents (Survival > Marksmanship/Lone Wolf > Beast Mastery).
// Auto Shot clipping: hunter casts (Aimed, Sniper) are gated on not delaying the next Auto Shot (the cast plus
// the 0.5 s aim fits before the next shot). Instant Multi-Shot shares Aimed Shot's cooldown and beats a clipping
// Aimed Shot (2026-10-16 sims: +3-4% at L60 BM/MM with a 2.5 s Auto Shot), so it takes the cooldown otherwise.
static bool has_talent( const player_t* p, std::string_view name )
{
  return p->find_talent_spell( talent_tree::CLASS, name ).rank() > 0;
}

static void ranged_mana( action_priority_list_t* default_ )
{
  default_->add_action( "aspect_of_the_viper,if=mana.pct<15", "Mana: Viper (10% max mana every 3 s, -10% damage) when low" );
  default_->add_action( "aspect_of_the_hawk,if=!buff.aspect_of_the_viper.up|mana.pct>=60" );
}

static void ranged_shots( action_priority_list_t* default_ )
{
  default_->add_action( "serpent_sting,if=!ticking&target.time_to_die>12" );
  default_->add_action( "sniper_shot,if=auto_shot.time_to_next>=cast_time+0.5",
                        "Casts only when they fit before the next Auto Shot (no clipping)" );
  default_->add_action( "aimed_shot,if=auto_shot.time_to_next>=cast_time+0.5" );
  default_->add_action( "multi_shot", "Shares the Aimed Shot cooldown" );
  default_->add_action( "arcane_shot,if=mana.pct>=20|buff.aspect_of_the_viper.up" );
}

void beast_mastery( player_t* p )
{
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* default_  = p->get_action_priority_list( "default" );

  precombat->add_action( "summon_pet,type=cat" );
  precombat->add_action( "aspect_of_the_hawk" );
  precombat->add_action( "trueshot_aura" );
  precombat->add_action( "hunters_mark" );

  default_->add_action( "auto_shot" );
  default_->add_action( "summon_pet,type=cat" );
  default_->add_action( "hunters_mark,if=!debuff.hunters_mark.up" );
  ranged_mana( default_ );
  default_->add_action( "bestial_wrath" );
  default_->add_action( "intimidation" );
  default_->add_action( "rapid_fire" );
  default_->add_action( "summon_hawk", "Shares the Arcane Shot cooldown; keeps up to 2 hawks" );
  ranged_shots( default_ );
}

void marksmanship( player_t* p )
{
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* default_  = p->get_action_priority_list( "default" );

  precombat->add_action( "aspect_of_the_hawk" );
  precombat->add_action( "trueshot_aura" );
  precombat->add_action( "hunters_mark" );

  default_->add_action( "auto_shot", "Lone Wolf: no pet" );
  default_->add_action( "hunters_mark,if=!debuff.hunters_mark.up" );
  ranged_mana( default_ );
  default_->add_action( "rapid_fire" );
  ranged_shots( default_ );
}

void survival( player_t* p )
{
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* default_  = p->get_action_priority_list( "default" );

  precombat->add_action( "aspect_of_the_falcon" );
  precombat->add_action( "aspect_of_the_beast,if=!action.aspect_of_the_falcon.enabled", "Falcon is level 60; Beast (melee AP) below that" );
  precombat->add_action( "trueshot_aura" );
  precombat->add_action( "hunters_mark" );

  default_->add_action( "auto_attack", "Melee Survival: in melee range, no Auto Shot (8 yd dead zone)" );
  default_->add_action( "hunters_mark,if=!debuff.hunters_mark.up" );
  default_->add_action( "aspect_of_the_viper,if=mana.pct<10" );
  default_->add_action( "aspect_of_the_falcon,if=!buff.aspect_of_the_viper.up|mana.pct>=50" );
  default_->add_action( "aspect_of_the_beast,if=!action.aspect_of_the_falcon.enabled&(!buff.aspect_of_the_viper.up|mana.pct>=50)" );
  default_->add_action( "rapid_fire" );
  default_->add_action( "mongoose_bite", "Only usable with Expose Prey (Hunter's Mark target procs)" );
  default_->add_action( "strider_kick" );
  default_->add_action( "raptor_strike", "On next swing" );
  default_->add_action( "lacerate,if=!ticking&target.time_to_die>12" );
}

void hunter( player_t* p )
{
  if ( has_talent( p, "Lacerating Strikes" ) || has_talent( p, "Strider Kick" ) || has_talent( p, "Expose Prey" ) )
    survival( p );
  else if ( has_talent( p, "Lone Wolf" ) )
    marksmanship( p );
  else
    beast_mastery( p );
}
//hunter_apl_end
}  // namespace hunter_apl

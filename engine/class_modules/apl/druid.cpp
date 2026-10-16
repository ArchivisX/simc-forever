#include "class_modules/apl/druid.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace druid_apl
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
//druid_apl_start
// Feral (Cat) when the druid has Cat Form and feral talents, Balance otherwise
static bool feral( const player_t* p )
{
  if ( p->level() < 20 )
    return false;

  for ( auto name : { "Shifting Power", "Berserk", "Predatory Strikes", "Shredding Attacks", "Savage Fury",
                      "Ferocity", "Sharpened Claws", "Blood Frenzy" } )
    if ( p->find_talent_spell( talent_tree::CLASS, name ).rank() > 0 )
      return true;

  return false;
}

void druid( player_t* p )
{
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* default_  = p->get_action_priority_list( "default" );

  precombat->add_action( "mark_of_the_wild" );

  if ( feral( p ) )
  {
    precombat->add_action( "cat_form" );
    precombat->add_action( "snapshot_stats" );

    default_->add_action( "auto_attack" );
    default_->add_action( "cat_form", "Shapeshifting is only used if something knocked us out of Cat Form" );
    default_->add_action( "faerie_fire,if=!debuff.faerie_fire.up", "Free in Cat Form, 6 s cooldown" );
    default_->add_action( "berserk,if=energy>=40", "Talent abilities are ignored unless the talent is selected" );
    default_->add_action( "shifting_power,if=energy<=40" );
    default_->add_action( "rip,if=combo_points>=5&!ticking&target.time_to_die>8" );
    default_->add_action( "rake,if=!ticking&target.time_to_die>6" );
    if ( p->level() >= 32 )
      default_->add_action( "ferocious_bite,if=combo_points>=5&dot.rip.ticking&!buff.clearcasting.up" );
    else
      default_->add_action( "rip,if=combo_points>=4&!ticking&target.time_to_die>6", "No Ferocious Bite before 32" );
    default_->add_action( "shred", "Shred is only usable from behind (position=back)" );
    default_->add_action( "claw" );
  }
  else
  {
    precombat->add_action( "moonkin_form" );
    precombat->add_action( "snapshot_stats" );
    precombat->add_action( "wrath" );

    default_->add_action( "innervate,if=mana.pct<30" );
    default_->add_action( "moonkin_form" );
    default_->add_action( "insect_swarm,if=!ticking&target.time_to_die>8" );
    default_->add_action( "moonfire,if=!ticking&target.time_to_die>8" );
    default_->add_action( "starfire,if=buff.eclipse.up|!talent.eclipse.enabled" );
    default_->add_action( "wrath" );
    default_->add_action( "starfire" );
  }
}
//druid_apl_end
}  // namespace druid_apl

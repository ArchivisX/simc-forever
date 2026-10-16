#include "class_modules/apl/rogue.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace rogue_apl
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

//rogue_apl_start
void rogue( player_t* p )
{
  action_priority_list_t* precombat     = p->get_action_priority_list( "precombat" );
  action_priority_list_t* default_      = p->get_action_priority_list( "default" );
  action_priority_list_t* assassination = p->get_action_priority_list( "assassination" );
  action_priority_list_t* combat        = p->get_action_priority_list( "combat" );
  action_priority_list_t* subtlety      = p->get_action_priority_list( "subtlety" );

  precombat->add_action( "variable,name=assassination,value=talent.mutilate|talent.seal_fate|talent.vile_poisons", "The deepest tree's signature talents pick the rotation; Combat otherwise" );
  precombat->add_action( "variable,name=subtlety,value=!variable.assassination&(talent.hemorrhage|talent.ghostly_strike|talent.premeditation)" );
  precombat->add_action( "variable,name=mutilate,value=talent.mutilate&main_hand.dagger&off_hand.dagger", "Mutilate needs a dagger in each hand" );
  precombat->add_action( "variable,name=snd,value=level>=42", "Slice and Dice rank 1 (+20%) loses to Eviscerate (L25 Combat sims: -8%); rank 2 (+30%, level 42) is kept up" );
  precombat->add_action( "apply_poison,mh=deadly,oh=instant,if=variable.assassination", "Deadly Poison keeps the target Poisoned (Mutilate) and feeds Coup de Grace" );
  precombat->add_action( "apply_poison,mh=instant,oh=instant,if=!variable.assassination" );
  precombat->add_action( "stealth" );
  precombat->add_action( "premeditation" );

  default_->add_action( "auto_attack", "Not usable while stealthed: the opener comes first" );
  default_->add_action( "call_action_list,name=assassination,if=variable.assassination" );
  default_->add_action( "call_action_list,name=subtlety,if=variable.subtlety" );
  default_->add_action( "call_action_list,name=combat,if=!variable.assassination&!variable.subtlety" );

  assassination->add_action( "ambush,if=stealthed" );
  assassination->add_action( "garrote,if=stealthed" );
  assassination->add_action( "slice_and_dice,if=variable.snd&!buff.slice_and_dice.up&combo_points>=2" );
  assassination->add_action( "cold_blood,if=combo_points>=4" );
  assassination->add_action( "slice_and_dice,if=variable.snd&combo_points>=4&buff.slice_and_dice.remains<2" );
  assassination->add_action( "eviscerate,if=variable.mutilate&combo_points>=4", "Mutilate awards 2 combo points (3 with Seal Fate), so finish at 4. Venom loses to Eviscerate at 4 combo points with the sample gear (2026-10-16 sims, -2%): venom,if=combo_points>=4&buff.venom.remains<1" );
  assassination->add_action( "mutilate,if=variable.mutilate&combo_points<=3" );
  assassination->add_action( "eviscerate,if=combo_points>=5" );
  assassination->add_action( "backstab,if=combo_points<5", "Needs a main hand dagger and position=back" );
  assassination->add_action( "sinister_strike,if=combo_points<5&!main_hand.dagger" );

  combat->add_action( "slice_and_dice,if=variable.snd&!buff.slice_and_dice.up&combo_points>=2" );
  combat->add_action( "blade_flurry,if=!variable.snd|buff.slice_and_dice.up" );
  combat->add_action( "adrenaline_rush,if=(!variable.snd|buff.slice_and_dice.up)&energy<=50" );
  combat->add_action( "slice_and_dice,if=variable.snd&combo_points>=5&buff.slice_and_dice.remains<3" );
  combat->add_action( "eviscerate,if=combo_points>=5" );
  combat->add_action( "backstab,if=combo_points<5" );
  combat->add_action( "sinister_strike,if=combo_points<5&!main_hand.dagger" );

  subtlety->add_action( "ambush,if=stealthed|buff.cutthroat.up", "Cutthroat procs allow Ambush out of Stealth" );
  subtlety->add_action( "garrote,if=stealthed" );
  subtlety->add_action( "slice_and_dice,if=variable.snd&!buff.slice_and_dice.up&combo_points>=2" );
  subtlety->add_action( "preparation,if=cooldown.ghostly_strike.remains>10&(!talent.cold_blood|cooldown.cold_blood.remains>30)", "Without Vanish, Preparation's only target here is Ghostly Strike (and Cold Blood in hybrid builds)" );
  subtlety->add_action( "rupture,if=combo_points>=5&!dot.rupture.ticking&debuff.hemorrhage.up&target.time_to_die>10", "Hemorrhage's debuff and Serrated Blades make Rupture the big finisher" );
  subtlety->add_action( "slice_and_dice,if=variable.snd&combo_points>=4&buff.slice_and_dice.remains<3" );
  subtlety->add_action( "eviscerate,if=combo_points>=5" );
  subtlety->add_action( "ghostly_strike,if=combo_points<5" );
  subtlety->add_action( "hemorrhage,if=combo_points<5", "Hemorrhage (35 energy, 145% with a dagger) builds faster per energy than Backstab (2026-10-16 sims: +2% over Hemorrhage upkeep + Backstab)" );
  subtlety->add_action( "backstab,if=combo_points<5&!talent.hemorrhage" );
  subtlety->add_action( "sinister_strike,if=combo_points<5&!talent.hemorrhage&!main_hand.dagger" );
}
//rogue_apl_end

}  // namespace rogue_apl

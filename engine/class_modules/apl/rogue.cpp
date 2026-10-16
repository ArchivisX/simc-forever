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
// The default APL follows the deepest tree's signature talents: Mutilate, Seal Fate or Vile Poisons
// (Assassination); Hemorrhage, Ghostly Strike or Premeditation (Subtlety); otherwise Combat. The builder follows
// the equipped weapons: Mutilate needs two daggers, Backstab a main hand dagger (and position=back).
void rogue( player_t* p )
{
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* default_  = p->get_action_priority_list( "default" );

  auto has = [ p ]( std::string_view name ) { return p->find_talent_spell( talent_tree::CLASS, name ).ok(); };

  bool mh_dagger = p->equipped_weapons[ SLOT_MAIN_HAND ].type == WEAPON_DAGGER;
  bool oh_dagger = p->equipped_weapons[ SLOT_OFF_HAND ].type == WEAPON_DAGGER;

  bool assassination = has( "Mutilate" ) || has( "Seal Fate" ) || has( "Vile Poisons" );
  bool subtlety      = !assassination && ( has( "Hemorrhage" ) || has( "Ghostly Strike" ) || has( "Premeditation" ) );

  std::string builder = mh_dagger ? "backstab" : "sinister_strike";

  // Slice and Dice rank 1 (+20%, levels 10-41) loses to Eviscerate in sims (L25 Combat: -8%); rank 2 (+30%) is
  // about even with it at 60, so the buff is only kept up from level 42.
  bool snd = p->true_level >= 42;
  auto add_snd = [ snd ]( action_priority_list_t* list, std::string_view action ) {
    if ( snd )
      list->add_action( action );
  };

  if ( assassination )
  {
    bool mutilate = has( "Mutilate" ) && mh_dagger && oh_dagger;

    precombat->add_action( "apply_poison,mh=deadly,oh=instant", "Deadly Poison keeps the target Poisoned (Mutilate) and feeds Coup de Grace" );
    precombat->add_action( "stealth" );

    default_->add_action( "auto_attack", "Not usable while stealthed: the opener comes first" );
    default_->add_action( "ambush,if=stealthed" );
    default_->add_action( "garrote,if=stealthed" );
    add_snd( default_, "slice_and_dice,if=!buff.slice_and_dice.up&combo_points>=2" );
    default_->add_action( "cold_blood,if=combo_points>=4" );
    add_snd( default_, "slice_and_dice,if=combo_points>=4&buff.slice_and_dice.remains<2" );
    if ( mutilate )
    {
      // Venom (31 points) is a finisher too: at 4 combo points it loses to Eviscerate with the sample gear
      // (2026-10-16 sims, -2%), so it is left to the user: venom,if=combo_points>=4&buff.venom.remains<1
      default_->add_action( "eviscerate,if=combo_points>=4", "Mutilate awards 2 combo points (3 with Seal Fate), so finish at 4" );
      default_->add_action( "mutilate,if=combo_points<=3" );
    }
    else
    {
      default_->add_action( "eviscerate,if=combo_points>=5" );
      default_->add_action( builder + ",if=combo_points<5" );
    }
  }
  else if ( subtlety )
  {
    precombat->add_action( "apply_poison,mh=instant,oh=instant" );
    precombat->add_action( "stealth" );
    precombat->add_action( "premeditation" );

    default_->add_action( "auto_attack", "Not usable while stealthed: the opener comes first" );
    default_->add_action( "ambush,if=stealthed|buff.cutthroat.up", "Cutthroat procs allow Ambush out of Stealth" );
    default_->add_action( "garrote,if=stealthed" );
    add_snd( default_, "slice_and_dice,if=!buff.slice_and_dice.up&combo_points>=2" );
    default_->add_action( "preparation,if=cooldown.ghostly_strike.remains>10&cooldown.cold_blood.remains>30" );
    default_->add_action( "rupture,if=combo_points>=5&!dot.rupture.ticking&debuff.hemorrhage.up&target.time_to_die>10",
                          "Hemorrhage's debuff and Serrated Blades make Rupture the big finisher" );
    add_snd( default_, "slice_and_dice,if=combo_points>=4&buff.slice_and_dice.remains<3" );
    default_->add_action( "eviscerate,if=combo_points>=5" );
    default_->add_action( "ghostly_strike,if=combo_points<5" );
    // Hemorrhage (35 energy, 145% with a dagger) builds combo points faster per energy than Backstab and finishers
    // carry Subtlety (2026-10-16 sims: Hemorrhage only +2% over Hemorrhage upkeep + Backstab)
    default_->add_action( "hemorrhage,if=combo_points<5" );
    default_->add_action( builder + ",if=combo_points<5&!talent.hemorrhage" );
  }
  else
  {
    precombat->add_action( "apply_poison,mh=instant,oh=instant" );
    precombat->add_action( "stealth" );

    default_->add_action( "auto_attack", "Not usable while stealthed: the opener comes first" );
    add_snd( default_, "slice_and_dice,if=!buff.slice_and_dice.up&combo_points>=2" );
    default_->add_action( snd ? "blade_flurry,if=buff.slice_and_dice.up" : "blade_flurry" );
    default_->add_action( snd ? "adrenaline_rush,if=buff.slice_and_dice.up&energy<=50" : "adrenaline_rush,if=energy<=50" );
    add_snd( default_, "slice_and_dice,if=combo_points>=5&buff.slice_and_dice.remains<3" );
    default_->add_action( "eviscerate,if=combo_points>=5" );
    default_->add_action( builder + ",if=combo_points<5" );
  }
}
//rogue_apl_end

}  // namespace rogue_apl

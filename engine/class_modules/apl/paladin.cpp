#include "class_modules/apl/paladin.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace paladin_apl
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
//paladin_apl_start
void paladin( player_t* p )
{
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* default_  = p->get_action_priority_list( "default" );

  precombat->add_action( "blessing_of_might" );
  precombat->add_action( "seal_of_the_crusader", "Open with Judgement of the Crusader; melee swings and Holy Strike (Sacred Arbiter) keep it up" );
  precombat->add_action( "snapshot_stats" );

  default_->add_action( "auto_attack" );
  default_->add_action( "blessing_of_might,if=!buff.blessing_of_might.up&!buff.blessing_of_kings.up" );
  default_->add_action( "judgement,if=buff.seal_of_the_crusader.up&!debuff.judgement_of_the_crusader.up" );
  default_->add_action( "seal_of_command,if=!buff.seal_of_command.up&!buff.seal_of_righteousness.up", "Main seal: Seal of Command with the talent, otherwise Seal of Righteousness" );
  default_->add_action( "seal_of_righteousness,if=!talent.seal_of_command&!buff.seal_of_righteousness.up" );
  default_->add_action( "seal_of_command,if=buff.seal_of_command.up&buff.seal_of_command.remains<3", "Keep the seal up before spending mana elsewhere" );
  default_->add_action( "seal_of_righteousness,if=!talent.seal_of_command&buff.seal_of_righteousness.up&buff.seal_of_righteousness.remains<3" );
  default_->add_action( "hammer_of_wrath", "Only usable at or below 20% target health" );
  default_->add_action( "divine_favor,if=talent.holy_shock&cooldown.holy_shock.remains<1.5" );
  default_->add_action( "holy_shock,if=mana.pct>20", "Holy talents are ignored unless selected; keep mana for seals and Judgement" );
  default_->add_action( "lights_vigil,if=cooldown.holy_shock.remains<cast_time&mana.pct>40" );
  default_->add_action( "holy_strike" );
  default_->add_action( "judgement,if=!buff.seal_of_the_crusader.up" );
  default_->add_action( "exorcism,if=mana.pct>30", "Only usable on Undead and Demon targets (enemy race)" );
  default_->add_action( "seal_of_righteousness,if=talent.twist_of_light&buff.seal_of_command.up&swing.mh.remains>gcd&mana.pct>95&target.health.pct>20", "Twist of Light: swap seals right after a swing so the next swing carries both. Two seals buy two Seal of Righteousness hits, worse damage per mana than Hammer of Wrath, so only twist with mana that would overflow" );
  default_->add_action( "seal_of_command,if=talent.twist_of_light&buff.seal_of_righteousness.up&swing.mh.remains>gcd", "Always twist back to Seal of Command" );
  default_->add_action( "consecration,if=spell_targets.consecration>=3", "Poor damage per mana on a single target" );
}
//paladin_apl_end

}  // namespace paladin_apl

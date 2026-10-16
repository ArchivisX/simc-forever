#include "class_modules/apl/priest.hpp"

#include "player/action_priority_list.hpp"
#include "player/player.hpp"

namespace priest_apl
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

//priest_apl_start
void priest( player_t* p )
{
  action_priority_list_t* precombat = p->get_action_priority_list( "precombat" );
  action_priority_list_t* default_  = p->get_action_priority_list( "default" );
  action_priority_list_t* shadow    = p->get_action_priority_list( "shadow" );
  action_priority_list_t* smite     = p->get_action_priority_list( "smite" );

  precombat->add_action( "power_word_fortitude" );
  precombat->add_action( "inner_fire" );
  precombat->add_action( "divine_spirit" );
  precombat->add_action( "shadowform" );

  default_->add_action( "run_action_list,name=shadow,if=talent.mind_flay", "Shadow from the 11 point Mind Flay talent, otherwise Smite / Holy Fire" );
  default_->add_action( "run_action_list,name=smite" );

  shadow->add_action( "shadowform,if=!buff.shadowform.up" );
  shadow->add_action( "power_infusion" );
  shadow->add_action( "shadow_word_pain,if=!ticking" );
  shadow->add_action( "devouring_plague,if=!ticking" );
  shadow->add_action( "inner_focus,if=cooldown.mind_blast.ready", "Free Mind Blast with +25% crit" );
  shadow->add_action( "mind_blast" );
  shadow->add_action( "shadow_word_death,if=target.health.pct<20|mana.pct>50", "Mind Flay does more damage per mana; Shadow Word: Death is for spare mana and the Early Demise window" );
  shadow->add_action( "starshards" );
  shadow->add_action( "mind_flay" );
  shadow->add_action( "shoot", "Wand when nothing else is affordable" );

  smite->add_action( "power_infusion" );
  smite->add_action( "shadow_word_pain,if=!ticking", "Also enables Power in Light" );
  smite->add_action( "holy_fire,if=!ticking" );
  smite->add_action( "inner_focus,if=cooldown.penance.ready|!talent.penance" );
  smite->add_action( "penance" );
  smite->add_action( "holy_nova,if=buff.holy_purpose.up", "Free Holy Nova from Searing Light" );
  smite->add_action( "mind_blast,if=mana.pct>50", "Shadow spells without Shadow Focus miss often; Devouring Plague costs too much mana without Shadowform" );
  smite->add_action( "shadow_word_death,if=mana.pct>50" );
  smite->add_action( "starshards" );
  smite->add_action( "smite" );
  smite->add_action( "shoot", "Wand when nothing else is affordable" );
}
//priest_apl_end
}  // namespace priest_apl

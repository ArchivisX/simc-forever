// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "action/parse_effects.hpp"
#include "class_modules/apl/warrior.hpp"

namespace
{
// ==========================================================================
// Warrior
// ==========================================================================

struct spell_rank_t
{
  unsigned spell_id;
};
struct bloodrage_rank_t
{
  unsigned spell_id;
  unsigned buff_id;
};

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
      best = rank;
      best_level = spell->level();
    }
  }

  return best;
}

constexpr std::array heroic_strike_ranks {
  spell_rank_t{ 78 },
  spell_rank_t{ 284 },
  spell_rank_t{ 285 },
  spell_rank_t{ 1608 },
  spell_rank_t{ 11564 },
  spell_rank_t{ 11565 },
  spell_rank_t{ 11566 },
  spell_rank_t{ 11567 },
  spell_rank_t{ 25286 },
};

constexpr std::array rend_ranks {
  spell_rank_t{ 772 },
  spell_rank_t{ 6546 },
  spell_rank_t{ 6547 },
  spell_rank_t{ 6548 },
  spell_rank_t{ 11572 },
  spell_rank_t{ 11573 },
  spell_rank_t{ 11574 },
};

constexpr std::array battle_shout_ranks {
  spell_rank_t{ 5242 },
  spell_rank_t{ 6192 },
  spell_rank_t{ 6673 },
  spell_rank_t{ 11549 },
  spell_rank_t{ 11550 },
  spell_rank_t{ 11551 },
  spell_rank_t{ 25289 },
};

constexpr std::array bloodrage_ranks {
  bloodrage_rank_t{ 2687, 29131 },
};

constexpr std::array charge_ranks {
  spell_rank_t{ 100 },
  spell_rank_t{ 6178 },
  spell_rank_t{ 11578 },
};

constexpr std::array thunder_clap_ranks {
  spell_rank_t{ 6343 },
  spell_rank_t{ 8198 },
  spell_rank_t{ 8204 },
  spell_rank_t{ 8205 },
  spell_rank_t{ 11580 },
  spell_rank_t{ 11581 },
};

constexpr std::array overpower_ranks {
  spell_rank_t{ 7384 },
  spell_rank_t{ 7887 },
  spell_rank_t{ 11584 },
  spell_rank_t{ 11585 },
};

constexpr std::array execute_ranks {
  spell_rank_t{ 5308 },
  spell_rank_t{ 20658 },
  spell_rank_t{ 20660 },
  spell_rank_t{ 20661 },
  spell_rank_t{ 20662 },
};

// 2026-10-16: Forever's rank 1 is a new level 20 spell. 1310196-1310200 are the Improved Slam versions
// of these ranks (see slam_t), not taught by the trainer.
constexpr std::array slam_ranks {
  spell_rank_t{ 1240193 },
  spell_rank_t{ 1464 },
  spell_rank_t{ 8820 },
  spell_rank_t{ 11604 },
  spell_rank_t{ 11605 },
};

constexpr std::array mortal_strike_ranks {
  spell_rank_t{ 12294 },
  spell_rank_t{ 21551 },
  spell_rank_t{ 21552 },
  spell_rank_t{ 21553 },
};

constexpr std::array bloodthirst_ranks {
  spell_rank_t{ 23881 },
  spell_rank_t{ 23892 },
  spell_rank_t{ 23893 },
  spell_rank_t{ 23894 },
};

constexpr std::array whirlwind_ranks {
  spell_rank_t{ 1680 },
};

constexpr std::array cleave_ranks {
  spell_rank_t{ 845 },
  spell_rank_t{ 7369 },
  spell_rank_t{ 11608 },
  spell_rank_t{ 11609 },
  spell_rank_t{ 20569 },
};

constexpr std::array sunder_armor_ranks {
  spell_rank_t{ 7386 },
  spell_rank_t{ 7405 },
  spell_rank_t{ 8380 },
  spell_rank_t{ 11596 },
  spell_rank_t{ 11597 },
};

// Execute is only usable on targets at or below this health percentage (spell description, no data field)
constexpr double execute_health_pct = 20.0;

// 2026-10-16: The client marks the Overpower window with a warrior combo point on the dodging target
// (Overpower lists "1 - 5 Combo Points" as a second power cost); no spell data carries the window length.
// Vanilla kept it for 5 seconds after the dodge.
constexpr timespan_t overpower_window = 5_s;


struct warrior_t : public player_t
{
  warrior_t( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) :
    player_t( sim, WARRIOR, name, r ) {}

  resource_e primary_resource() const override { return RESOURCE_RAGE; }

  void init_base_stats() override;
  void init_gains() override;
  void create_actions() override;
  void create_buffs() override;
  void init_spells() override;

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
  void init_action_list() override;

  std::string default_potion() const override   { return warrior_apl::potion( this ); }
  std::string default_flask() const override    { return warrior_apl::flask( this ); }
  std::string default_food() const override     { return warrior_apl::food( this ); }

  double rage_per_swing( const weapon_t& w ) const;

  double composite_player_target_armor( player_t* t ) const override;
  buff_t* get_sunder_armor( player_t* t );

  struct talents_t
  {
    // Arms
    // Row 1
    player_talent_t deflection;

    player_talent_t improved_slam;

    // Row 7
    player_talent_t mortal_strike;

    // Fury
    // Row 7
    player_talent_t bloodthirst;

  } talent;

  struct buffs_t
  {
    buff_t* battle_shout = nullptr;
    buff_t* bloodrage    = nullptr;

    buff_t* heroic_strike_queued = nullptr;
    buff_t* cleave_queued        = nullptr;
    buff_t* overpower            = nullptr;
  } buff;

  struct gains_t
  {
    gain_t* melee          = nullptr;
    gain_t* melee_off_hand = nullptr;
  } gain;

  action_t* heroic_strike = nullptr;
  action_t* cleave        = nullptr;

  // Sunder Armor debuff per target, owned by the target's buff list
  target_specific_t<buff_t> sunder_armor { false };

  // only one on next swing ability can be queued at a time
  bool next_swing_queued() const
  { return buff.heroic_strike_queued->check() || buff.cleave_queued->check(); }

  // a queued on next swing ability removes the dual wield miss penalty from white swings
  bool classic_next_swing_queued() const override
  { return next_swing_queued(); }
};

template <class Base>
struct warrior_action_t : public parse_action_effects_t<Base>
{
private:
  using ab = parse_action_effects_t<Base>;

public:
  using base_t = warrior_action_t<Base>;

  template <typename... Args>
  warrior_action_t( Args&&... args ) : ab( std::forward<Args>( args )... ) {}

  warrior_t* p()
  { return debug_cast<warrior_t*>( this->player ); }

  const warrior_t* p() const
  { return debug_cast<const warrior_t*>( this->player ); }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    // Any dodged warrior attack (white or yellow) opens the Overpower window
    if ( s->result == RESULT_DODGE )
      p()->buff.overpower->trigger();
  }
};

using warrior_attack_t = warrior_action_t<melee_attack_t>;
using warrior_spell_t  = warrior_action_t<spell_t>;

// TODO: Party buff version
struct battle_shout_t : public warrior_spell_t
{
  battle_shout_t( warrior_t* player, util::string_view options_str ) :
    warrior_spell_t( "battle_shout", player, player->find_spell( max_rank( player, battle_shout_ranks ).spell_id ) )
  {
    parse_options( options_str );

    harmful  = false;
    may_miss = false;

    target   = player;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    // TODO: Legacy talent "Permanence" can extend the duration

    p()->buff.battle_shout->trigger();
  }
};

struct bloodrage_t : public warrior_spell_t
{
  bloodrage_t( warrior_t* player, util::string_view options_str ) :
    warrior_spell_t( "bloodrage", player, player->find_spell( max_rank( player, bloodrage_ranks ).spell_id ) )
  {
    parse_options( options_str );

    harmful  = false;
    may_miss = false;

    target   = player;
  }

  void execute() override
  {
    warrior_spell_t::execute();

    p()->buff.bloodrage->trigger();
    p()->resource_loss( RESOURCE_HEALTH, 100 * data().cost( POWER_HEALTH ), nullptr, this );
  }
};

/////////////////////////////////////////////////////////////////////////////////////
//
// Melee
//
/////////////////////////////////////////////////////////////////////////////////////

struct warrior_melee_t : public warrior_attack_t
{
  warrior_melee_t( util::string_view name, warrior_t* player, weapon_t* w )
    : warrior_attack_t( name, player, spell_data_t::nil() )
  {
    weapon            = w;
    weapon_multiplier = 1.0;
    base_execute_time = w->swing_time;
    school            = SCHOOL_PHYSICAL;
    trigger_gcd       = 0_ms;

    background        = true;
    repeating         = true;
    may_glance        = true;
    may_crit          = true;
    may_dodge         = true;
    may_parry         = true;
    special           = false;
  }

  // try to trigger a queued on next swing ability in place of this swing
  bool trigger_next_swing( action_t* ability, buff_t* queued )
  {
    if ( !queued->check() )
      return false;

    if ( p()->resource_available( RESOURCE_RAGE, ability->cost() ) )
    {
      ability->execute_on_target( target );
      schedule_execute();
      return true;
    }
    // didn't have enough rage, cancel the tracking queue buff
    queued->expire();
    return false;
  }

  void execute() override
  {
    // on next swing abilities replace a main hand swing only
    // TODO: in vanilla a queued on next swing ability also removes the dual wield miss penalty from off hand swings
    if ( weapon->slot == SLOT_MAIN_HAND &&
         ( trigger_next_swing( p()->heroic_strike, p()->buff.heroic_strike_queued ) ||
           trigger_next_swing( p()->cleave, p()->buff.cleave_queued ) ) )
      return;

    warrior_attack_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      double mult = s->result == RESULT_CRIT ? 2.0 : 1.0;
      p()->resource_gain( RESOURCE_RAGE, mult * p()->rage_per_swing( *weapon ),
                          weapon->slot == SLOT_OFF_HAND ? p()->gain.melee_off_hand : p()->gain.melee, this );
    }
  }
};

struct warrior_auto_attack_t : public warrior_attack_t
{
  warrior_auto_attack_t( warrior_t* p, util::string_view options_str )
    : warrior_attack_t( "auto_attack", p, spell_data_t::nil() )
  {
    parse_options( options_str );
    trigger_gcd           = 0_ms;
    ignore_false_positive = true;

    assert( p->equipped_weapons[ SLOT_MAIN_HAND ].type != WEAPON_NONE );

    p->main_hand_attack = new warrior_melee_t( "melee", p, &p->equipped_weapons[ SLOT_MAIN_HAND ] );

    // the off hand penalty (50% damage) is applied by the engine for any SLOT_OFF_HAND weapon
    if ( p->equipped_weapons[ SLOT_OFF_HAND ].type != WEAPON_NONE )
      p->off_hand_attack = new warrior_melee_t( "melee_off_hand", p, &p->equipped_weapons[ SLOT_OFF_HAND ] );
  }

  void execute() override
  {
    if ( player->main_hand_attack->execute_event == nullptr )
      player->main_hand_attack->schedule_execute();

    if ( player->off_hand_attack && player->off_hand_attack->execute_event == nullptr )
      player->off_hand_attack->schedule_execute();
  }

  bool ready() override
  {
    if ( player->is_moving() )
    {
      return false;
    }
    return player->main_hand_attack->execute_event == nullptr ||
           ( player->off_hand_attack && player->off_hand_attack->execute_event == nullptr );
  }
};

double warrior_t::rage_per_swing( const weapon_t& w ) const
{
  // log observations on low level warrior
  double mult = w.group() == WEAPON_2H ? 4.5 : 3.45;
  // 2026-10-16: off hand swings are assumed to generate half, like the vanilla hit factor (1.75 vs 3.5).
  // Not measured on Forever yet.
  if ( w.slot == SLOT_OFF_HAND )
    mult *= 0.5;
  return w.swing_time.total_seconds() * mult;
}

buff_t* warrior_t::get_sunder_armor( player_t* t )
{
  buff_t*& debuff = sunder_armor[ t ];
  if ( !debuff )
    debuff = make_buff( actor_pair_t( t, this ), "sunder_armor",
                        find_spell( max_rank( this, sunder_armor_ranks ).spell_id ) );
  return debuff;
}

double warrior_t::composite_player_target_armor( player_t* t ) const
{
  double a = player_t::composite_player_target_armor( t );

  if ( const buff_t* sunder = sunder_armor[ t ] )
    a = std::max( 0.0, a + sunder->check() * sunder->data().effectN( 1 ).base_value() );

  return a;
}

/////////////////////////////////////////////////////////////////////////////////////
//
// Abilities
//
/////////////////////////////////////////////////////////////////////////////////////

struct rend_t : public warrior_attack_t
{
  rend_t( warrior_t* player, util::string_view options_str ) :
    warrior_attack_t( "rend", player, player->find_spell( max_rank( player, rend_ranks ).spell_id ) )
  {
    parse_options( options_str );

    dot_behavior = DOT_CLIP;

    // TODO: Verify this at higher levels
    attack_power_mod.tick = 0.016;
  }
};

// APL action to queue up an on next swing ability (heroic strike, cleave)
// Checks to make sure warrior has enough rage to queue it up
// The actual execute in the melee swing will check rage again to make sure that rage
// wasn't consumed mid-swing
// TODO: Is there a better way to handle On Next Swing?
// TODO: look into generalizing this pattern for hunter (raptor strike), and druid (maul)
struct next_swing_queue_t : public action_t
{
  action_t* ability;
  buff_t* queued;

  next_swing_queue_t( warrior_t* p, util::string_view name, util::string_view options_str, action_t* ability,
                      buff_t* queued )
    : action_t( ACTION_OTHER, name, p ), ability( ability ), queued( queued )
  {
    parse_options( options_str );

    trigger_gcd = 0_ms;
    harmful = false;
    ignore_false_positive = true;

    // ability not learned yet
    if ( !ability->data().ok() )
      background = true;
  }

  warrior_t* p() const
  { return debug_cast<warrior_t*>( player ); }

  bool ready() override
  {
    // already queued
    if ( p()->next_swing_queued() )
      return false;

    // not enough rage
    if ( !p()->resource_available( RESOURCE_RAGE, ability->cost() ) )
      return false;

    return action_t::ready();
  }

  void execute() override
  {
    queued->trigger();
  }
};

struct heroic_strike_t : public warrior_attack_t
{
  heroic_strike_t( warrior_t* player, util::string_view options_str ) :
    warrior_attack_t( "heroic_strike", player, player->find_spell( max_rank( player, heroic_strike_ranks ).spell_id ) )
  {
    parse_options( options_str );

    trigger_gcd = 0_ms;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    p()->buff.heroic_strike_queued->expire();
  }
};

// On next swing like Heroic Strike; the second target comes from the effect's chain targets.
struct cleave_t : public warrior_attack_t
{
  cleave_t( warrior_t* player, util::string_view options_str ) :
    warrior_attack_t( "cleave", player, player->find_spell( max_rank( player, cleave_ranks ).spell_id ) )
  {
    parse_options( options_str );

    trigger_gcd = 0_ms;
  }

  void execute() override
  {
    warrior_attack_t::execute();

    p()->buff.cleave_queued->expire();
  }
};

// Up to max targets (4) from the spell data, normalized main hand weapon damage (the spell has a single
// weapon damage effect, so no off hand strike as in later expansions).
// TODO: Stances are not modelled (Whirlwind needs Berserker Stance).
struct whirlwind_t : public warrior_attack_t
{
  whirlwind_t( warrior_t* player, util::string_view options_str ) :
    warrior_attack_t( "whirlwind", player, player->find_spell( max_rank( player, whirlwind_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }
};

// 2026-10-16: The armor reduction is client aura type 674 (unknown to simc) with the flat armor per stack
// as its (negative) base value; it stacks to 5 and lasts 30 s per the spell data.
// TODO: Only reduces armor for this warrior's own attacks (no raid wide armor debuff yet).
struct sunder_armor_t : public warrior_attack_t
{
  sunder_armor_t( warrior_t* player, util::string_view options_str ) :
    warrior_attack_t( "sunder_armor", player, player->find_spell( max_rank( player, sunder_armor_ranks ).spell_id ) )
  {
    parse_options( options_str );

    // create the debuff on the default target up front so debuff.sunder_armor APL expressions resolve
    if ( data().ok() && target )
      player->get_sunder_armor( target );
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
      p()->get_sunder_armor( s->target )->trigger();
  }
};

struct charge_t : public warrior_attack_t
{
  charge_t( warrior_t* player, util::string_view options_str ) :
    warrior_attack_t( "charge", player, player->find_spell( max_rank( player, charge_ranks ).spell_id ) )
  {
    parse_options( options_str );

    harmful = false;
    may_miss = false;
  }

  bool ready() override
  {
    return !player->in_combat && warrior_attack_t::ready();
  }
};

struct thunder_clap_t : public warrior_attack_t
{
  thunder_clap_t( warrior_t* player, util::string_view options_str ) :
    warrior_attack_t( "thunder_clap", player, player->find_spell( max_rank( player, thunder_clap_ranks ).spell_id ) )
  {
    parse_options( options_str );

    aoe = data().max_targets();

    // TODO: Verify this at higher levels
    attack_power_mod.direct = 0.05;

    // TODO: attack speed slow
  }
};

// Overpower is No Active Defense in the spell data, so it cannot be dodged, parried or blocked.
// TODO: Stances are not modelled, the warrior is assumed to be in Battle Stance.
struct overpower_t : public warrior_attack_t
{
  overpower_t( warrior_t* player, util::string_view options_str ) :
    warrior_attack_t( "overpower", player, player->find_spell( max_rank( player, overpower_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }

  bool ready() override
  {
    if ( !p()->buff.overpower->check() )
      return false;

    return warrior_attack_t::ready();
  }

  void execute() override
  {
    warrior_attack_t::execute();

    p()->buff.overpower->expire();
  }
};

// 2026-10-16: Execute's damage is a Dummy effect: base value = flat damage, and the effect's chain
// amplitude is damage per point of extra rage in internal rage units (tenths), which the tooltip shows
// as "$*10;F1" (0.3 -> 3 damage per rage at rank 1, 1.5 -> 15 at rank 5). All rage above the cost is
// consumed on a hit.
// TODO: Unknown whether a missed/dodged Execute also loses the extra rage; we keep it (only the
// discounted base cost is paid).
// TODO: Stances are not modelled (Execute needs Battle or Berserker Stance).
struct execute_t : public warrior_attack_t
{
  double rage_damage;
  double extra_rage;

  execute_t( warrior_t* player, util::string_view options_str ) :
    warrior_attack_t( "execute", player, player->find_spell( max_rank( player, execute_ranks ).spell_id ) ),
    rage_damage( 0.0 ), extra_rage( 0.0 )
  {
    parse_options( options_str );

    const auto& dmg = data().effectN( 1 );
    base_dd_min = base_dd_max = dmg.base_value();
    // client rage is stored in tenths of a point (power type display multiplier 0.1)
    rage_damage = dmg.chain_multiplier() / power_type_data_t::multiplier( RESOURCE_RAGE );
  }

  bool target_ready( player_t* candidate_target ) override
  {
    if ( candidate_target->health_percentage() > execute_health_pct )
      return false;

    return warrior_attack_t::target_ready( candidate_target );
  }

  void execute() override
  {
    extra_rage = std::max( 0.0, p()->resources.current[ RESOURCE_RAGE ] - cost() );

    warrior_attack_t::execute();
  }

  double bonus_da( const action_state_t* s ) const override
  {
    return warrior_attack_t::bonus_da( s ) + extra_rage * rage_damage;
  }

  void consume_resource() override
  {
    warrior_attack_t::consume_resource();

    if ( extra_rage > 0 && result_is_hit( execute_state->result ) )
    {
      player->resource_loss( RESOURCE_RAGE, extra_rage, nullptr, this );
      stats->consume_resource( RESOURCE_RAGE, extra_rage );
      last_resource_cost += extra_rage;
    }
  }
};

// Slam has a cast time (and in Forever an 18 s cooldown) from the spell data.
// 2026-10-16: Forever's Improved Slam replaces the known rank with a copy (Override Action Spell effects:
// misc value = the replaced rank, base value = the replacement) that has "Do Not Reset Combat Timers"; the
// trainer ranks restart the main hand swing timer when the cast finishes, as in vanilla. The swing timer
// pausing during the cast is not modelled, so a white swing may still land mid-cast.
// TODO: Verify the swing timer behaviour on Forever.
struct slam_t : public warrior_attack_t
{
  static const spell_data_t* slam_spell( warrior_t* p )
  {
    const spell_data_t* spell = p->find_spell( max_rank( p, slam_ranks ).spell_id );

    for ( const auto& eff : p->talent.improved_slam->effects() )
    {
      if ( eff.subtype() == A_OVERRIDE_ACTION_SPELL && as<unsigned>( eff.misc_value1() ) == spell->id() )
        return p->find_spell( as<unsigned>( eff.base_value() ) );
    }

    return spell;
  }

  slam_t( warrior_t* player, util::string_view options_str ) :
    warrior_attack_t( "slam", player, slam_spell( player ) )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    warrior_attack_t::execute();

    if ( data().flags( SX_DO_NOT_RESET_COMBAT_TIMERS ) )
      return;

    action_t* swing = player->main_hand_attack;
    if ( swing && swing->execute_event )
      swing->reschedule_execute( swing->execute_time() );
  }
};

// Talent abilities: the talent teaches rank 1, higher ranks come from the trainer, so the highest known
// rank is only used if the talent is taken.
template <typename RANKS>
const spell_data_t* talent_rank_spell( const warrior_t* p, const player_talent_t& talent, const RANKS& ranks )
{
  return talent.ok() ? p->find_spell( max_rank( p, ranks ).spell_id ) : spell_data_t::not_found();
}

struct mortal_strike_t : public warrior_attack_t
{
  mortal_strike_t( warrior_t* player, util::string_view options_str ) :
    warrior_attack_t( "mortal_strike", player, talent_rank_spell( player, player->talent.mortal_strike, mortal_strike_ranks ) )
  {
    parse_options( options_str );
  }
};

// 2026-10-16: Forever Bloodthirst is physical School Damage (effect 1) plus a Dummy effect 2 holding the
// attack power percentage (45% at all ranks), and it also grants a movement speed buff (effect 3, not
// modelled). Vanilla's heal on hit is gone.
struct bloodthirst_t : public warrior_attack_t
{
  bloodthirst_t( warrior_t* player, util::string_view options_str ) :
    warrior_attack_t( "bloodthirst", player, talent_rank_spell( player, player->talent.bloodthirst, bloodthirst_ranks ) )
  {
    parse_options( options_str );

    attack_power_mod.direct = data().effectN( 2 ).percent();
  }
};

/////////////////////////////////////////////////////////////////////////////////////
//
// Stats, Actions, Oh my
//
/////////////////////////////////////////////////////////////////////////////////////

void warrior_t::init_base_stats()
{
  base.attack_power_per_strength = 2.0;

  player_t::init_base_stats();


  base.parry_per_strength = 0.0;

  // TODO - can this base stuff be moved into core?

  base.all_crit           = 0.0;
  base.spell_crit_chance  = 0.0;

  base.dodge_per_agility = base.attack_crit_per_agility;
  auto base_dodge = ( dbc->race_base( race ).agility + dbc->attribute_base( type, level() ).agility )
               * base.dodge_per_agility;

  base.attack_crit_chance = get_passive_player_value( 0.0, "attack_crit" );
  base.parry              = get_passive_player_value( 0.05, "parry" );
  base.block              = get_passive_player_value( 0.05, "block" );
  base.dodge              = get_passive_player_value( base_dodge, "dodge" );

  base.stats.attack_power = 3.0 * level() - 20.0;
}

void warrior_t::init_gains()
{
  player_t::init_gains();

  gain.melee          = get_gain( "melee" );
  gain.melee_off_hand = get_gain( "melee_off_hand" );
}

void warrior_t::create_actions()
{
  heroic_strike = new heroic_strike_t( this, "" );
  cleave        = new cleave_t( this, "" );

  player_t::create_actions();
}

void warrior_t::create_buffs()
{
  buff.heroic_strike_queued = make_buff( this, "heroic_strike_queued" );
  buff.cleave_queued        = make_buff( this, "cleave_queued" );

  buff.overpower = make_buff( this, "overpower" )
                     ->set_duration( overpower_window );

  buff.battle_shout = make_buff<stat_buff_t>( this, "battle_shout",
                        find_spell( max_rank( this, battle_shout_ranks ).spell_id ) );

  buff.bloodrage = make_buff( this, "bloodrage",
                        find_spell( max_rank( this, bloodrage_ranks ).buff_id ) )
                        ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                            resource_gain( RESOURCE_RAGE, 1.0, get_gain( "bloodrage" ) ); } );

  player_t::create_buffs();
}

void warrior_t::init_spells()
{
  player_t::init_spells();

  talent.improved_slam = find_talent_spell( talent_tree::CLASS, "Improved Slam" );
  talent.mortal_strike = find_talent_spell( talent_tree::CLASS, "Mortal Strike" );
  talent.bloodthirst   = find_talent_spell( talent_tree::CLASS, "Bloodthirst" );

  parse_all_class_passives();
  parse_all_passive_talents();
  parse_raid_buffs();
}

action_t* warrior_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "auto_attack" ) return new warrior_auto_attack_t( this, options_str );

  if ( name == "battle_shout" ) return new battle_shout_t( this, options_str );
  if ( name == "bloodrage" ) return new bloodrage_t( this, options_str );
  if ( name == "charge" ) return new charge_t( this, options_str );

  if ( name == "rend" ) return new rend_t( this, options_str );
  if ( name == "heroic_strike" )
    return new next_swing_queue_t( this, "heroic_strike_queue", options_str, heroic_strike, buff.heroic_strike_queued );
  if ( name == "cleave" )
    return new next_swing_queue_t( this, "cleave_queue", options_str, cleave, buff.cleave_queued );
  if ( name == "whirlwind" ) return new whirlwind_t( this, options_str );
  if ( name == "sunder_armor" ) return new sunder_armor_t( this, options_str );


  if ( name == "thunder_clap" ) return new thunder_clap_t( this, options_str );
  if ( name == "overpower" ) return new overpower_t( this, options_str );
  if ( name == "execute" ) return new execute_t( this, options_str );
  if ( name == "slam" ) return new slam_t( this, options_str );
  if ( name == "mortal_strike" ) return new mortal_strike_t( this, options_str );
  if ( name == "bloodthirst" ) return new bloodthirst_t( this, options_str );

  return player_t::create_action( name, options_str );
}

void warrior_t::init_action_list()
{
  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  clear_action_priority_lists();
  warrior_apl::warrior( this );
  use_default_action_list = true;
  player_t::init_action_list();
}

struct warrior_module_t : public module_t
{
  warrior_module_t() : module_t( WARRIOR ) {}

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new warrior_t( sim, name, r );
    return p;
  }

  bool valid() const override
  {
    return true;
  }

  void register_hotfixes() const override {}

  void register_actor_initializers( sim_t* ) const override {}
};
}  // UNNAMED NAMESPACE

const module_t* module_t::warrior()
{
  static warrior_module_t m;
  return &m;
}

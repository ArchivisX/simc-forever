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

// 2026-10-16: Forever's rank 1 is a new level 20 spell; 1310196-1310200 are a second copy of the ranks
// that is not in the warrior SkillLineAbility list, so we use the ids the trainer teaches.
constexpr std::array slam_ranks {
  spell_rank_t{ 1240193 },
  spell_rank_t{ 1464 },
  spell_rank_t{ 8820 },
  spell_rank_t{ 11604 },
  spell_rank_t{ 11605 },
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

  struct talents_t
  {
    // Arms
    // Row 1
    player_talent_t deflection;

  } talent;

  struct buffs_t
  {
    buff_t* battle_shout = nullptr;
    buff_t* bloodrage    = nullptr;

    buff_t* heroic_strike_queued = nullptr;
    buff_t* overpower            = nullptr;
  } buff;

  struct gains_t
  {
    gain_t* melee = nullptr;
  } gain;

  action_t* heroic_strike = nullptr;
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

  void consume_resource() override
  {
    resource_e cr = this->current_resource();

    auto discountOnMiss = this->data().flags( spell_attribute::SX_DISCOUNT_ON_MISS );

    if (
        cr == RESOURCE_RAGE
        && this->cost() > 0
        && discountOnMiss
        && !this->proc
        && !this->result_is_hit( this->execute_state->result ) )
    {
      this->last_resource_cost = 0.2 * this->cost();

      this->player->resource_loss( cr, this->last_resource_cost, nullptr, this );
      this->stats->consume_resource( cr, this->last_resource_cost );

      this->sim->print_log("{} consumes {} {} for {} ({})",
          *this->player, this->last_resource_cost, cr, *this, this->player->resources.current[ cr ] );
    }
    else
    {
      ab::consume_resource();
    }
  }

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

  void execute() override
  {
    // try to trigger a queued heroic strike
    if ( p()->buff.heroic_strike_queued->check() )
    {
      if ( p()->resource_available( RESOURCE_RAGE, p()->heroic_strike->cost() ) )
      {
        p()->heroic_strike->execute_on_target( target );
        schedule_execute();
        return;
      }
      // didn't have enough rage, cancel the tracking queue buff
      p()->buff.heroic_strike_queued->expire();
    }

    warrior_attack_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    warrior_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      double mult = s->result == RESULT_CRIT ? 2.0 : 1.0;
      p()->resource_gain( RESOURCE_RAGE, mult * p()->rage_per_swing( *weapon ), p()->gain.melee, this );
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
  }

  void execute() override
  {
    player->main_hand_attack->schedule_execute();
  }

  bool ready() override
  {
    if ( player->is_moving() )
    {
      return false;
    }
    return player->main_hand_attack->execute_event == nullptr;
  }
};

double warrior_t::rage_per_swing( const weapon_t& w ) const
{
  // log observations on low level warrior
  double mult = w.group() == WEAPON_2H ? 4.5 : 3.45;
  return w.swing_time.total_seconds() * mult;
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

// APL action to queue up heroic strike
// Checks to make sure warrior has enough rage to queue it up
// The actual HS execute in the melee swing will check rage again to make sure that rage
// wasn't consumed mid-swing
// TODO: Is there a better way to handle On Next Swing?
// TODO: look into generalizing this pattern for cleave, hunter (raptor strike), and druid (maul)
struct heroic_strike_queue_t : public action_t
{
  heroic_strike_queue_t( warrior_t* p, util::string_view options_str )
    : action_t( ACTION_OTHER, "heroic_strike_queue", p )
  {
    parse_options( options_str );

    trigger_gcd = 0_ms;
    harmful = false;
    ignore_false_positive = true;
  }

  warrior_t* p() const
  { return debug_cast<warrior_t*>( player ); }

  bool ready() override
  {
    // already queued
    if ( p()->buff.heroic_strike_queued->check() )
      return false;

    // not enough rage
    if ( !p()->resource_available( RESOURCE_RAGE, p()->heroic_strike->cost() ) )
      return false;

    return action_t::ready();
  }

  void execute() override
  {
    p()->buff.heroic_strike_queued->trigger();
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
    rage_damage = dmg.chain_multiplier() * RAGE_UNITS_PER_RAGE;
  }

  // client rage is stored in tenths of a point
  static constexpr double RAGE_UNITS_PER_RAGE = 10.0;

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
// 2026-10-16: In vanilla, finishing a Slam restarts the main hand swing timer; we model that. The
// swing timer pausing during the cast is not modelled, so a white swing may still land mid-cast.
// TODO: Verify both on Forever.
struct slam_t : public warrior_attack_t
{
  slam_t( warrior_t* player, util::string_view options_str ) :
    warrior_attack_t( "slam", player, player->find_spell( max_rank( player, slam_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    warrior_attack_t::execute();

    action_t* swing = player->main_hand_attack;
    if ( swing && swing->execute_event )
      swing->reschedule_execute( swing->execute_time() );
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

  gain.melee = get_gain( "melee" );
}

void warrior_t::create_actions()
{
  heroic_strike = new heroic_strike_t( this, "" );

  player_t::create_actions();
}

void warrior_t::create_buffs()
{
  buff.heroic_strike_queued = make_buff( this, "heroic_strike_queued" );

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

  parse_all_class_passives();
  parse_all_passive_talents();
}

action_t* warrior_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "auto_attack" ) return new warrior_auto_attack_t( this, options_str );

  if ( name == "battle_shout" ) return new battle_shout_t( this, options_str );
  if ( name == "bloodrage" ) return new bloodrage_t( this, options_str );
  if ( name == "charge" ) return new charge_t( this, options_str );

  if ( name == "rend" ) return new rend_t( this, options_str );
  if ( name == "heroic_strike" ) return new heroic_strike_queue_t( this, options_str );


  if ( name == "thunder_clap" ) return new thunder_clap_t( this, options_str );
  if ( name == "overpower" ) return new overpower_t( this, options_str );
  if ( name == "execute" ) return new execute_t( this, options_str );
  if ( name == "slam" ) return new slam_t( this, options_str );

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

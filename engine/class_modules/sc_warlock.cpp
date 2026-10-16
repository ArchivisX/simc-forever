#include "simulationcraft.hpp"
#include "action/parse_effects.hpp"

#include "class_modules/apl/warlock.hpp"

namespace warlock
{
struct spell_rank_t
{
  unsigned spell_id;
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

constexpr std::array shadow_bolt_ranks {
  spell_rank_t{ 686 },
  spell_rank_t{ 695 },
  spell_rank_t{ 705 },
  spell_rank_t{ 1088 },
  spell_rank_t{ 1106 },
  spell_rank_t{ 7641 },
  spell_rank_t{ 11659 },
  spell_rank_t{ 11660 },
  spell_rank_t{ 11661 },
  spell_rank_t{ 25307 },
};
constexpr std::array immolate_ranks {
  spell_rank_t{ 348 },
  spell_rank_t{ 707 },
  spell_rank_t{ 1094 },
  spell_rank_t{ 2941 },
  spell_rank_t{ 11665 },
  spell_rank_t{ 11667 },
  spell_rank_t{ 11668 },
  spell_rank_t{ 25309 },
};
constexpr std::array corruption_ranks {
  spell_rank_t{ 172 },
  spell_rank_t{ 6222 },
  spell_rank_t{ 6223 },
  spell_rank_t{ 7648 },
  spell_rank_t{ 11671 },
  spell_rank_t{ 11672 },
  spell_rank_t{ 25311 },
};

// Forever renamed Curse of Agony to Bane of Agony
constexpr std::array bane_of_agony_ranks {
  spell_rank_t{ 980 },    // 8
  spell_rank_t{ 1014 },   // 18
  spell_rank_t{ 6217 },   // 28
  spell_rank_t{ 11711 },  // 38
  spell_rank_t{ 11712 },  // 48
  spell_rank_t{ 11713 },  // 58
};

constexpr std::array searing_pain_ranks {
  spell_rank_t{ 5676 },   // 18
  spell_rank_t{ 17919 },  // 26
  spell_rank_t{ 17920 },  // 34
  spell_rank_t{ 17921 },  // 42
  spell_rank_t{ 17922 },  // 50
  spell_rank_t{ 17923 },  // 58
};

// The 403677-403689 "Drain Life" ranks are Season of Discovery leftovers (15 s duration), not the trainer spell
constexpr std::array drain_life_ranks {
  spell_rank_t{ 689 },    // 14
  spell_rank_t{ 699 },    // 22
  spell_rank_t{ 709 },    // 30
  spell_rank_t{ 7651 },   // 38
  spell_rank_t{ 11699 },  // 46
  spell_rank_t{ 11700 },  // 54
};

constexpr std::array life_tap_ranks {
  spell_rank_t{ 1454 },   // 6
  spell_rank_t{ 1455 },   // 16
  spell_rank_t{ 1456 },   // 26
  spell_rank_t{ 11687 },  // 36
  spell_rank_t{ 11688 },  // 46
  spell_rank_t{ 11689 },  // 56
};

constexpr std::array firebolt_ranks {
  spell_rank_t{ 3110 },   // 1
  spell_rank_t{ 7799 },   // 8
  spell_rank_t{ 7800 },   // 18
  spell_rank_t{ 7801 },   // 28
  spell_rank_t{ 7802 },   // 38
  spell_rank_t{ 11762 },  // 48
  spell_rank_t{ 11763 },  // 58
};

struct warlock_t final : public player_t
{
  warlock_t( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) :
    player_t( sim, WARLOCK, name, r ) {}

  resource_e primary_resource() const override { return RESOURCE_MANA; }

  std::string default_potion() const override   { return warlock_apl::potion( this ); }
  std::string default_flask() const override    { return warlock_apl::flask( this ); }
  std::string default_food() const override     { return warlock_apl::food( this ); }

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
  void init_action_list() override;
  void init_base_stats() override;
  void init_spells() override;

  void create_pets() override;
  void init_gains() override;

  struct gains_t
  {
    gain_t* life_tap = nullptr;
  } gain;

  struct talents_t
  {
    player_talent_t improved_life_tap;
  } talent;

  struct pets_t
  {
    pet_t* imp = nullptr;
  } pets;
};

struct warlock_module_t : public module_t
{
  warlock_module_t() : module_t( WARLOCK )
  { }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new warlock_t( sim, name, r );
    return p;
  }

  bool valid() const override
  { return true; }

  void register_hotfixes() const override {}

  void register_actor_initializers( sim_t* ) const override {}
};

template <class Base>
struct warlock_action_t : public parse_action_effects_t<Base>
{
private:
  using ab = parse_action_effects_t<Base>;

public:
  using base_t = warlock_action_t<Base>;

  template <typename... Args>
  warlock_action_t( Args&&... args ) : ab( std::forward<Args>( args )... ) {}

  warlock_t* p()
  { return debug_cast<warlock_t*>( this->player ); }

  const warlock_t* p() const
  { return debug_cast<const warlock_t*>( this->player ); }
};

using warlock_spell_t  = warlock_action_t<spell_t>;

// Shadow Bolt

struct shadow_bolt_t : public warlock_spell_t
{
  shadow_bolt_t( warlock_t* player, util::string_view options_str ) :
    warlock_spell_t( "shadow_bolt", player, player->find_spell( max_rank( player, shadow_bolt_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }
};

// Immolate

struct immolate_t : public warlock_spell_t
{
  immolate_t( warlock_t* player, util::string_view options_str ) :
    warlock_spell_t( "immolate", player, player->find_spell( max_rank( player, immolate_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }
};

// Corruption

struct corruption_t : public warlock_spell_t
{
  corruption_t( warlock_t* player, util::string_view options_str ) :
    warlock_spell_t( "corruption", player, player->find_spell( max_rank( player, corruption_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }
};

// Searing Pain

struct searing_pain_t : public warlock_spell_t
{
  searing_pain_t( warlock_t* player, util::string_view options_str ) :
    warlock_spell_t( "searing_pain", player, player->find_spell( max_rank( player, searing_pain_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }
};

// Drain Life
// Channeled Periodic Health Leech, ticking every second. The heal to the warlock is not modelled.

struct drain_life_t : public warlock_spell_t
{
  drain_life_t( warlock_t* player, util::string_view options_str ) :
    warlock_spell_t( "drain_life", player, player->find_spell( max_rank( player, drain_life_ranks ).spell_id ) )
  {
    parse_options( options_str );

    channeled = true;
  }
};

// Bane of Agony (Curse of Agony)
// "This damage is dealt slowly at first, and builds up as the Bane reaches its full duration."
// The tick amount in the data is the average. Vanilla Curse of Agony dealt 50% of it for the first third
// of the ticks, 100% for the second and 150% for the last third (sum unchanged); the Forever description
// matches, the exact split is unverified (2026-10-16).

struct bane_of_agony_t : public warlock_spell_t
{
  bane_of_agony_t( warlock_t* player, util::string_view options_str ) :
    warlock_spell_t( "bane_of_agony", player, player->find_spell( max_rank( player, bane_of_agony_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }

  // The ramp depends on the tick number, so it is applied per tick rather than snapshot on application
  double calculate_tick_amount( action_state_t* s, double dot_multiplier ) const override
  {
    if ( dot_t* d = find_dot( s->target ); d && d->num_ticks() > 0 )
    {
      double progress = static_cast<double>( d->current_tick ) / d->num_ticks();
      if ( progress <= 1.0 / 3.0 )
        dot_multiplier *= 0.5;
      else if ( progress > 2.0 / 3.0 )
        dot_multiplier *= 1.5;
    }

    return warlock_spell_t::calculate_tick_amount( s, dot_multiplier );
  }
};

// Life Tap
// Converts health into mana. The amount is the Dummy effect (base + points per level up to the rank's max
// level), and the health cost equals it. No spell power scaling in the client data (2026-10-16). The spell
// has "Don't Block Mana Regen", and gaining mana never starts the five second rule.
// The data's health cost is 0%, so the Dummy amount is used as the health cost: Life Tap is not usable
// without that much health (incoming healing is not modelled). Improved Life Tap (Dummy, per rank percent
// from the trait curve) increases the mana only.

struct life_tap_t : public warlock_spell_t
{
  life_tap_t( warlock_t* player, util::string_view options_str ) :
    warlock_spell_t( "life_tap", player, player->find_spell( max_rank( player, life_tap_ranks ).spell_id ) )
  {
    parse_options( options_str );

    harmful  = false;
    may_miss = false;
    may_crit = false;
    target   = player;

    resource_current               = RESOURCE_HEALTH;
    base_costs[ RESOURCE_HEALTH ] = data().effectN( 1 ).average( player );
  }

  void execute() override
  {
    warlock_spell_t::execute();

    double mana = data().effectN( 1 ).average( player ) * ( 1.0 + p()->talent.improved_life_tap->effectN( 1 ).percent() );

    p()->resource_gain( RESOURCE_MANA, mana, p()->gain.life_tap, this );
  }
};

struct summon_imp_t final : public warlock_spell_t
{
  summon_imp_t( warlock_t* player, std::string_view options_str ) :
    warlock_spell_t( "summon_imp", player, player->find_spell( 688 ) )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    warlock_spell_t::execute();
    p()->pets.imp->summon();
  }
};

///////////////////////////////////////////
//
// Demons
//
///////////////////////////////////////////

namespace pets {
  struct warlock_pet_t : public pet_t
  {
    warlock_pet_t( sim_t* sim, warlock_t* owner, std::string_view pet_name, pet_e pet_type, bool guardian = false ) :
      pet_t( sim, owner, pet_name, pet_type, guardian, false )
    {
    }

    const warlock_t* o() const
    { return static_cast<warlock_t*>( owner ); }

    warlock_t* o()
    { return static_cast<warlock_t*>( owner ); }
  };

  template <class Base>
  struct warlock_pet_action_t : parse_action_effects_t<Base>
  {
  private:
    using ab = parse_action_effects_t<Base>;
  public:
    using base_t = warlock_pet_action_t<Base>;

    template <typename... Args>
    warlock_pet_action_t( Args&&... args ) : ab( std::forward<Args>( args )... ) {}

    warlock_pet_t* p()
    { return debug_cast<warlock_pet_t*>( this->player ); }

    const warlock_pet_t* p() const
    { return debug_cast<const warlock_pet_t*>( this->player ); }

    warlock_t* o()
    { return debug_cast<warlock_t*>( p()->owner ); }

    const warlock_t* o() const
    { return debug_cast<const warlock_t*>( p()->owner ); }
  };

  using warlock_pet_attack_t = warlock_pet_action_t<melee_attack_t>;
  using warlock_pet_spell_t  = warlock_pet_action_t<spell_t>;

  namespace imp {
    struct imp_pet_t : warlock_pet_t
    {
      imp_pet_t( sim_t* sim, warlock_t* owner ) :
        warlock_pet_t( sim, owner, "imp", PET_IMP )
      {
        // from level 6-10 imp character sheet
        owner_coeff.armor = 0.35;
        owner_coeff.ap_from_sp = 0.17;

        // TODO: Imp HP looks to come from a table
        owner_coeff.health = 1.5;
        stamina_per_owner = 0;

        // matches in-game stats but int might not actually do anything on the pet
        intellect_per_owner = 0.3;
      }

      resource_e primary_resource() const override { return RESOURCE_MANA; }

      // TODO: figure this out - probably another table of int/mana at every level
      // fit to obsevations level 3-8 but starts to break down at level 9
      static double base_mana( int level )
      { return std::floor( 29 + 9.5 * level ); }

      // copies owner's crit which matches imp char sheet
      double pet_crit() const override
      { return current_pet_stats.composite_spell_crit; }

      double resource_regen_per_second( resource_e r ) const override
      {
        if ( r != RESOURCE_MANA )
        {
          return pet_t::resource_regen_per_second( r );
        }

        // TODO - this is at level 7, probably need a table for this
        // 2026-10-16: above level 10 scale it with the imp's max mana (124 at level 10)
        // so higher level imps don't run dry after a few casts. Levels <= 10 are unchanged. Unverified.
        double scale = std::max( 1.0, resources.max[ RESOURCE_MANA ] / base_mana( 10 ) );
        return ( recent_cast() ? 1.65 : 2.73 ) * scale;
      }

      void init_base_stats() override
      {
        warlock_pet_t::init_base_stats();

        resources.base[ RESOURCE_MANA ] = base_mana( level() );
      }

      void init_action_list() override
      {
        action_list_str = "firebolt";
        warlock_pet_t::init_action_list();
      }

      action_t* create_action( std::string_view, std::string_view ) override;
    };

    // TODO: There's some funky delay on beta. Some casts are back-to-back, some have a 0.5-1.0s gap even when
    // the imp has mana
    struct firebolt_t : warlock_pet_spell_t
    {
      firebolt_t( std::string_view n, imp_pet_t* p, std::string_view options_str  ) :
        warlock_pet_spell_t( n, p, p->find_spell( max_rank( p, firebolt_ranks ).spell_id ) )
      {
        parse_options( options_str );

        // beta casts have been wonky with gaps in between casts
        //ability_lag.mean   = 500_ms;
        //ability_lag.stddev = 100_ms;
      }
    };

    action_t* imp_pet_t::create_action( std::string_view name, std::string_view options_str )
    {
      if ( name == "firebolt" ) return new firebolt_t( name, this, options_str );

      return warlock_pet_t::create_action( name, options_str );
    }
  }
}

action_t* warlock_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "shadow_bolt" )
    return new shadow_bolt_t( this, options_str );
  if ( name == "immolate" )
    return new immolate_t( this, options_str );
  if ( name == "corruption" )
    return new corruption_t( this, options_str );

  if ( name == "searing_pain" )
    return new searing_pain_t( this, options_str );
  if ( name == "drain_life" )
    return new drain_life_t( this, options_str );
  if ( name == "bane_of_agony" || name == "curse_of_agony" )
    return new bane_of_agony_t( this, options_str );
  if ( name == "life_tap" )
    return new life_tap_t( this, options_str );

  if ( name == "summon_imp" )
    return new summon_imp_t( this, options_str );

  return player_t::create_action( name, options_str );
}

void warlock_t::create_pets()
{
  player_t::create_pets();

  pets.imp = new pets::imp::imp_pet_t( sim, this );
}

void warlock_t::init_gains()
{
  player_t::init_gains();

  gain.life_tap = get_gain( "life_tap" );
}

void warlock_t::init_base_stats()
{
  player_t::init_base_stats();

  base.attack_crit_chance = get_passive_player_value(0.02, "all_crit");
  base.spell_crit_chance = get_passive_player_value(0.01701, "all_crit");
}

void warlock_t::init_spells()
{
  player_t::init_spells();

  talent.improved_life_tap = find_talent_spell( talent_tree::CLASS, "Improved Life Tap" );

  parse_all_class_passives();
  parse_all_passive_talents();
  parse_raid_buffs();
}

void warlock_t::init_action_list()
{
  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  clear_action_priority_lists();
  warlock_apl::warlock( this );
  use_default_action_list = true;
  player_t::init_action_list();
}

}  // namespace warlock

const module_t* module_t::warlock()
{
  static warlock::warlock_module_t m;
  return &m;
}

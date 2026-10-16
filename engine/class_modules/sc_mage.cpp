// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
//
// WoW: Forever mage. Values come from the client spell data (build 70425); module-side guesses are
// marked ESTIMATE. See FOREVER_MAGE.md at the repository root for the list of estimates and gaps.

#include "simulationcraft.hpp"
#include "action/parse_effects.hpp"
#include "action/residual_action.hpp"
#include "class_modules/apl/mage.hpp"
#include "class_modules/forever_helpers.hpp"

namespace
{
using forever::max_rank;
using forever::spell_rank_t;

// ==========================================================================
// Ranks (explicit ids: the client also has Season of Discovery copies of several mage spells)
// ==========================================================================

constexpr std::array fireball_ranks {
  spell_rank_t{ 133 },   spell_rank_t{ 143 },   spell_rank_t{ 145 },   spell_rank_t{ 3140 },
  spell_rank_t{ 8400 },  spell_rank_t{ 8401 },  spell_rank_t{ 8402 },  spell_rank_t{ 10148 },
  spell_rank_t{ 10149 }, spell_rank_t{ 10150 }, spell_rank_t{ 10151 }, spell_rank_t{ 25306 },
};
constexpr std::array frostbolt_ranks {
  spell_rank_t{ 116 },   spell_rank_t{ 205 },   spell_rank_t{ 837 },   spell_rank_t{ 7322 },
  spell_rank_t{ 8406 },  spell_rank_t{ 8407 },  spell_rank_t{ 8408 },  spell_rank_t{ 10179 },
  spell_rank_t{ 10180 }, spell_rank_t{ 10181 }, spell_rank_t{ 25304 },
};
// The SoD Fire Blast copies (400616-400623) have a 15 s cooldown and are not taught
constexpr std::array fire_blast_ranks {
  spell_rank_t{ 2136 }, spell_rank_t{ 2137 }, spell_rank_t{ 2138 },  spell_rank_t{ 8412 },
  spell_rank_t{ 8413 }, spell_rank_t{ 10197 }, spell_rank_t{ 10199 },
};
constexpr std::array scorch_ranks {
  spell_rank_t{ 2948 },  spell_rank_t{ 8444 },  spell_rank_t{ 8445 }, spell_rank_t{ 8446 },
  spell_rank_t{ 10205 }, spell_rank_t{ 10206 }, spell_rank_t{ 10207 },
};
constexpr std::array pyroblast_ranks {
  spell_rank_t{ 11366 }, spell_rank_t{ 12505 }, spell_rank_t{ 12522 }, spell_rank_t{ 12523 },
  spell_rank_t{ 12524 }, spell_rank_t{ 12525 }, spell_rank_t{ 12526 }, spell_rank_t{ 18809 },
};
constexpr std::array blast_wave_ranks {
  spell_rank_t{ 11113 }, spell_rank_t{ 13018 }, spell_rank_t{ 13019 }, spell_rank_t{ 13020 }, spell_rank_t{ 13021 },
};
constexpr std::array arcane_blast_ranks {
  spell_rank_t{ 400574 }, spell_rank_t{ 1239696 }, spell_rank_t{ 1239697 }, spell_rank_t{ 1239699 },
  spell_rank_t{ 1239700 },
};
constexpr std::array arcane_wrath_ranks {
  spell_rank_t{ 1326079 }, spell_rank_t{ 1326080 }, spell_rank_t{ 1326081 }, spell_rank_t{ 1326082 },
};
constexpr std::array ice_lance_ranks {
  spell_rank_t{ 1312002 }, spell_rank_t{ 400640 },  spell_rank_t{ 1240044 }, spell_rank_t{ 1240045 },
  spell_rank_t{ 1240046 }, spell_rank_t{ 1240047 },
};
constexpr std::array frostfire_bolt_ranks {
  spell_rank_t{ 401502 }, spell_rank_t{ 1237312 }, spell_rank_t{ 1237313 },
};
constexpr std::array frost_blast_ranks {
  spell_rank_t{ 1326053 }, spell_rank_t{ 1326054 }, spell_rank_t{ 1326055 },
};
constexpr std::array cone_of_cold_ranks {
  spell_rank_t{ 120 }, spell_rank_t{ 8492 }, spell_rank_t{ 10159 }, spell_rank_t{ 10160 }, spell_rank_t{ 10161 },
};
constexpr std::array frost_nova_ranks {
  spell_rank_t{ 122 }, spell_rank_t{ 865 }, spell_rank_t{ 6131 }, spell_rank_t{ 10230 },
};
constexpr std::array arcane_explosion_ranks {
  spell_rank_t{ 1449 },  spell_rank_t{ 8437 },  spell_rank_t{ 8438 }, spell_rank_t{ 8439 },
  spell_rank_t{ 10201 }, spell_rank_t{ 10202 },
};
constexpr std::array arcane_intellect_ranks {
  spell_rank_t{ 1459 }, spell_rank_t{ 1460 }, spell_rank_t{ 1461 }, spell_rank_t{ 10156 }, spell_rank_t{ 10157 },
};
constexpr std::array mage_armor_ranks {
  spell_rank_t{ 6117 }, spell_rank_t{ 22782 }, spell_rank_t{ 22783 },
};
constexpr std::array ice_armor_ranks {
  spell_rank_t{ 7302 }, spell_rank_t{ 7320 }, spell_rank_t{ 10219 }, spell_rank_t{ 10220 },
};
constexpr std::array frost_armor_ranks {
  spell_rank_t{ 168 }, spell_rank_t{ 7300 }, spell_rank_t{ 7301 },
};

// Channels / area spells whose damage comes from a separate (triggered) spell per rank
struct tick_rank_t
{
  unsigned spell_id;  // the castable spell
  unsigned tick_id;   // the damage spell
};
constexpr std::array arcane_missiles_ranks {
  tick_rank_t{ 5143, 7268 },   tick_rank_t{ 5144, 7269 },   tick_rank_t{ 5145, 7270 },   tick_rank_t{ 8416, 8419 },
  tick_rank_t{ 8417, 8418 },   tick_rank_t{ 10211, 10273 }, tick_rank_t{ 10212, 10274 }, tick_rank_t{ 25345, 25346 },
};
// Forever (70425): Blizzard and Flamestrike damage comes from new per-rank tick spells
constexpr std::array blizzard_ranks {
  tick_rank_t{ 10, 1279976 },    tick_rank_t{ 6141, 1279977 },  tick_rank_t{ 8427, 1279978 },
  tick_rank_t{ 10185, 1279979 }, tick_rank_t{ 10186, 1279980 }, tick_rank_t{ 10187, 1279949 },
};
constexpr std::array flamestrike_ranks {
  tick_rank_t{ 2120, 1279983 },  tick_rank_t{ 2121, 1279985 },  tick_rank_t{ 8422, 1279987 },
  tick_rank_t{ 8423, 1279988 },  tick_rank_t{ 10215, 1279989 }, tick_rank_t{ 10216, 1279990 },
};

// Mana gems: conjure spell (decides availability by level) and the restore spell of the item
struct mana_gem_rank_t
{
  unsigned spell_id;    // Conjure Mana X
  unsigned restore_id;  // Replenish Mana (item use)
};
constexpr std::array mana_gem_ranks {
  mana_gem_rank_t{ 759, 5405 },     // Agate, 28
  mana_gem_rank_t{ 3552, 10052 },   // Jade, 38
  mana_gem_rank_t{ 10053, 10057 },  // Citrine, 48
  mana_gem_rank_t{ 10054, 10058 },  // Ruby, 58
};

enum class mage_tree_e
{
  ARCANE,
  FIRE,
  FROST
};

struct mage_t;

// ==========================================================================
// Target data
// ==========================================================================

struct mage_td_t : public actor_target_data_t
{
  struct debuffs_t
  {
    buff_t* fire_vulnerability = nullptr;
    buff_t* winters_chill      = nullptr;
    buff_t* frostbite          = nullptr;
  } debuff;

  struct dots_t
  {
    dot_t* ignite = nullptr;
  } dots;

  mage_td_t( player_t* target, mage_t* p );
};

// ==========================================================================
// Mage
// ==========================================================================

struct mage_t final : public player_t
{
  mage_t( sim_t* sim, std::string_view name, race_e r = RACE_NONE ) : player_t( sim, MAGE, name, r ) {}

  struct options_t
  {
    // Cone of Cold is a 10 yd cone: off by default would make the Winter's Chill build useless on Patchwerk,
    // so the default assumes the mage stands close enough. Turn off for a ranged-only fight.
    bool cone_of_cold_in_range = true;
    // Treat the target as permanently Frozen (trash, leveling, PvP). Bosses are root immune.
    bool target_frozen = false;
    // Let Frostbite (and Frost Nova) actually freeze the target. Off by default (bosses are root immune).
    bool target_freezable = false;
  } options;

  struct talents_t
  {
    // Arcane
    player_talent_t arcane_focus;
    player_talent_t missile_barrage;
    player_talent_t arcane_concentration;
    player_talent_t arcane_blast;
    player_talent_t arcane_wrath;
    player_talent_t presence_of_mind;
    player_talent_t arcane_power;
    // Fire
    player_talent_t ignite;
    player_talent_t pyroblast;
    player_talent_t improved_scorch;
    player_talent_t heating_up;
    player_talent_t master_of_elements;
    player_talent_t blast_wave;
    player_talent_t fire_power;
    player_talent_t combustion;
    // Frost
    player_talent_t elemental_precision;
    player_talent_t frostbite;
    player_talent_t ice_lance;
    player_talent_t shatter;
    player_talent_t cold_snap;
    player_talent_t fingers_of_frost;
    player_talent_t winters_chill;
  } talent;

  struct buffs_t
  {
    buff_t* arcane_blast      = nullptr;
    buff_t* clearcasting      = nullptr;
    buff_t* missile_barrage   = nullptr;
    buff_t* presence_of_mind  = nullptr;
    buff_t* arcane_power      = nullptr;
    buff_t* heating_up        = nullptr;
    buff_t* combustion        = nullptr;  // the activated aura, stacks = crit charges left
    buff_t* combustion_crit   = nullptr;  // +10% fire crit per stack
    buff_t* fingers_of_frost  = nullptr;
    buff_t* evocation         = nullptr;
    buff_t* arcane_intellect  = nullptr;
    buff_t* mage_armor        = nullptr;
    buff_t* ice_armor         = nullptr;
    buff_t* frost_armor       = nullptr;
  } buff;

  struct gains_t
  {
    gain_t* master_of_elements = nullptr;
    gain_t* mana_gem           = nullptr;
    gain_t* evocation          = nullptr;
  } gain;

  struct procs_t
  {
    proc_t* clearcasting     = nullptr;
    proc_t* missile_barrage  = nullptr;
    proc_t* fingers_of_frost = nullptr;
    proc_t* winters_chill    = nullptr;
    proc_t* heating_up       = nullptr;
  } proc;

  struct spells_t
  {
    const spell_data_t* ignite             = spell_data_t::not_found();
    const spell_data_t* fire_vulnerability = spell_data_t::not_found();
    const spell_data_t* winters_chill      = spell_data_t::not_found();
  } spell;

  action_t* ignite = nullptr;
  cooldown_t* arcane_concentration_icd = nullptr;

  target_specific_t<mage_td_t> target_data;

  resource_e primary_resource() const override { return RESOURCE_MANA; }
  role_e primary_role() const override { return ROLE_SPELL; }

  std::string default_potion() const override { return mage_apl::potion( this ); }
  std::string default_flask() const override  { return mage_apl::flask( this ); }
  std::string default_food() const override   { return mage_apl::food( this ); }

  mage_td_t* get_target_data( player_t* target ) const override
  {
    mage_td_t*& td = target_data[ target ];
    if ( !td )
      td = new mage_td_t( target, const_cast<mage_t*>( this ) );
    return td;
  }

  void create_options() override;
  action_t* create_action( std::string_view name, std::string_view options_str ) override;
  void create_actions() override;
  void create_buffs() override;
  void init_base_stats() override;
  void init_spells() override;
  void init_gains() override;
  void init_procs() override;
  void init_action_list() override;
  void invalidate_cache( cache_e ) override;
  std::unique_ptr<expr_t> create_expression( std::string_view name ) override;
  void copy_from( player_t* source ) override;

  double composite_spell_power( school_e school ) const override;
  double composite_mana_regen_while_casting() const override;
  double composite_spirit_regen_multiplier() const override;

  mage_tree_e primary_tree() const;
  bool target_is_frozen( player_t* t ) const;
  bool frozen_for_action( player_t* t ) const;
  int fire_effects_on( player_t* t ) const;
};

mage_td_t::mage_td_t( player_t* target, mage_t* p ) : actor_target_data_t( target, p )
{
  // Improved Scorch (Forever): Fire damage taken from this mage only, +3% per stack
  debuff.fire_vulnerability = make_buff( *this, "fire_vulnerability", p->spell.fire_vulnerability )
                                ->set_default_value_from_effect( 1 );

  // Winter's Chill (70425 + hotfix): +5% Cone of Cold damage taken and +7% Cone of Cold spell power
  // coefficient per stack, consumed by the next Cone of Cold
  debuff.winters_chill = make_buff( *this, "winters_chill", p->spell.winters_chill );
  if ( p->talent.winters_chill.ok() )
    debuff.winters_chill->set_max_stack( std::max( 1, as<int>( p->talent.winters_chill->effectN( 1 ).base_value() ) ) );

  debuff.frostbite = make_buff( *this, "frostbite", p->find_spell( 12494 ) );

  dots.ignite = target->get_dot( "ignite", p );
}

// ==========================================================================
// Mage actions
// ==========================================================================

template <class Base>
struct mage_action_t : public parse_action_effects_t<Base>
{
private:
  using ab = parse_action_effects_t<Base>;

public:
  using base_t = mage_action_t<Base>;

  // talent / buff interactions, decided from the spell data whitelists at construction
  bool affected_by_clearcasting     = false;
  bool affected_by_presence_of_mind = false;
  bool affected_by_missile_barrage  = false;
  bool affected_by_heating_up       = false;
  bool affected_by_combustion       = false;
  // proc behaviour (set by the derived spells)
  bool triggers_arcane_concentration = true;
  bool is_chill                      = false;  // Fingers of Frost / Frostbite source
  bool triggers_winters_chill        = false;
  bool triggers_heating_up           = false;
  double missile_barrage_chance      = 0.0;  // fraction of the talent's effect #1

  template <typename... Args>
  mage_action_t( Args&&... args ) : ab( std::forward<Args>( args )... )
  {
    this->may_crit = true;
  }

  mage_t* p()
  { return debug_cast<mage_t*>( this->player ); }

  const mage_t* p() const
  { return debug_cast<const mage_t*>( this->player ); }

  mage_td_t* td( player_t* t ) const
  { return p()->get_target_data( t ); }

  void init() override
  {
    ab::init();

    // Not in the constructor: buffs are created after actions are constructed for some precombat lists
    parse_mage_effects();
  }

  bool effects_parsed = false;

  void parse_mage_effects()
  {
    if ( effects_parsed )
      return;
    effects_parsed = true;

    auto& b = p()->buff;

    // Spell hit talents use property 16, which the passive parser ignores; parse them on the action
    this->parse_effects( p()->talent.arcane_focus, PARSE_PASSIVE );
    this->parse_effects( p()->talent.elemental_precision, PARSE_PASSIVE );

    this->parse_effects( b.arcane_power );
    this->parse_effects( b.presence_of_mind );
    this->parse_effects( b.clearcasting );
    this->parse_effects( b.missile_barrage );
    this->parse_effects( b.heating_up );
    this->parse_effects( b.combustion_crit );
    // effect #1 (aura 174, +15% of Intellect as spell damage) is applied in mage_t::composite_spell_power
    this->parse_effects( b.arcane_blast, 0b1U );

    // Winter's Chill: aura 271 (damage taken from the caster's spells, Cone of Cold only)
    this->parse_target_effects( []( actor_target_data_t* t ) {
      return static_cast<mage_td_t*>( t )->debuff.winters_chill->check();
    }, p()->spell.winters_chill );

    auto affected = [ this ]( buff_t* buff ) {
      return buff && buff->data().ok() && this->data().affected_by( buff->data().effectN( 1 ) );
    };
    affected_by_clearcasting     = affected( b.clearcasting );
    affected_by_presence_of_mind = affected( b.presence_of_mind );
    affected_by_missile_barrage  = affected( b.missile_barrage );
    affected_by_heating_up       = affected( b.heating_up );
    affected_by_combustion       = affected( b.combustion_crit );
  }

  bool is_fire() const
  { return dbc::is_school( this->get_school(), SCHOOL_FIRE ); }

  bool is_frost() const
  { return dbc::is_school( this->get_school(), SCHOOL_FROST ); }

  // Fire Power: "Effects which mix multiple schools of magic do not qualify"
  bool is_pure_fire() const
  { return this->get_school() == SCHOOL_FIRE; }

  // Self buffs can't miss
  void init_finished() override
  {
    if ( !this->harmful )
      this->may_miss = false;

    ab::init_finished();
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = ab::composite_target_multiplier( t );

    // Improved Scorch: Fire Vulnerability is aura 270 (damage taken from caster, by school), not parsed
    if ( is_fire() )
      m *= 1.0 + td( t )->debuff.fire_vulnerability->check_stack_value();

    // Fire Power (70425): +s1% per own Fire effect on the target, up to 4 effects, pure Fire spells only.
    // ESTIMATE: which effects count is unverified (V4); see mage_t::fire_effects_on.
    if ( is_pure_fire() && p()->talent.fire_power.ok() )
      m *= 1.0 + p()->talent.fire_power->effectN( 1 ).percent() * std::min( 4, p()->fire_effects_on( t ) );

    return m;
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    double c = ab::composite_target_crit_chance( t );

    // Shatter: +17/33/50% crit vs Frozen targets (dummy aura)
    if ( p()->talent.shatter.ok() && p()->frozen_for_action( t ) )
      c += p()->talent.shatter->effectN( 1 ).percent();

    return c;
  }

  void execute() override
  {
    // Consumed charges are decided before the cast resolves, so a proc from this cast survives
    bool consume_cc   = affected_by_clearcasting && p()->buff.clearcasting->check();
    bool consume_pom  = affected_by_presence_of_mind && p()->buff.presence_of_mind->check() &&
                        this->base_execute_time > 0_ms && !this->background;
    bool consume_mb   = affected_by_missile_barrage && p()->buff.missile_barrage->check();
    bool consume_fof  = this->harmful && !this->background && p()->buff.fingers_of_frost->check() &&
                        ( this->spell_power_mod.direct > 0 || this->base_dd_max > 0 || this->tick_action );

    ab::execute();

    if ( consume_cc )
      p()->buff.clearcasting->decrement();
    if ( consume_pom )
      p()->buff.presence_of_mind->expire();
    if ( consume_mb )
      p()->buff.missile_barrage->decrement();
    if ( consume_fof )
      p()->buff.fingers_of_frost->decrement();

    if ( this->harmful && this->result_is_hit( this->execute_state->result ) )
      trigger_cast_procs( this->execute_state );
  }

  void trigger_cast_procs( action_state_t* s )
  {
    // Missile Barrage: Arcane Blast m1%, Arcane Wrath/Fireball/Frostbolt/Frostfire Bolt m1/2 %
    if ( missile_barrage_chance > 0 && p()->talent.missile_barrage.ok() &&
         this->rng().roll( p()->talent.missile_barrage->effectN( 1 ).percent() * missile_barrage_chance ) )
    {
      p()->buff.missile_barrage->trigger();
      p()->proc.missile_barrage->occur();
    }

    // Chill effects: Fingers of Frost and Frostbite
    if ( is_chill )
    {
      if ( p()->talent.fingers_of_frost.ok() &&
           this->rng().roll( p()->talent.fingers_of_frost->effectN( 2 ).percent() ) )
      {
        p()->buff.fingers_of_frost->trigger( p()->buff.fingers_of_frost->max_stack() );
        p()->proc.fingers_of_frost->occur();
      }

      if ( p()->talent.frostbite.ok() && p()->options.target_freezable &&
           this->rng().roll( p()->talent.frostbite->effectN( 1 ).percent() ) )
        td( s->target )->debuff.frostbite->trigger();
    }

    // Winter's Chill: Frostbolt, Frost Blast and Fingers of Frost empowered Ice Lance
    if ( triggers_winters_chill && p()->talent.winters_chill.ok() &&
         this->rng().roll( p()->talent.winters_chill->effectN( 2 ).percent() ) )
    {
      td( s->target )->debuff.winters_chill->trigger();
      p()->proc.winters_chill->occur();
    }
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( !this->result_is_hit( s->result ) || s->result_type != result_amount_type::DMG_DIRECT )
      return;

    trigger_damage_procs( s );
  }

  void trigger_damage_procs( action_state_t* s )
  {
    // Arcane Concentration: chance after any damage spell hits, 1 s internal cooldown.
    // ESTIMATE: the "reduced chance for low-rank spells" (Oct 1 notes) is not modelled.
    if ( triggers_arcane_concentration && p()->talent.arcane_concentration.ok() &&
         p()->arcane_concentration_icd->up() &&
         this->rng().roll( p()->talent.arcane_concentration->effectN( 1 ).percent() ) )
    {
      p()->buff.clearcasting->trigger();
      p()->proc.clearcasting->occur();
      p()->arcane_concentration_icd->start();
    }

    bool crit = s->result == RESULT_CRIT;

    // Ignite: fire critical strikes burn for x% of the damage over 4 s (rolling, one per mage)
    if ( crit && is_fire() && p()->ignite && s->result_amount > 0 )
      residual_action::trigger( p()->ignite, s->target, s->result_amount * p()->talent.ignite->effectN( 1 ).percent() );

    // Master of Elements: fire and frost crits refund x% of the base mana cost
    if ( crit && ( is_fire() || is_frost() ) && p()->talent.master_of_elements.ok() )
    {
      double base_cost = this->base_costs[ RESOURCE_MANA ].base;
      if ( base_cost > 0 )
        p()->resource_gain( RESOURCE_MANA, base_cost * p()->talent.master_of_elements->effectN( 1 ).percent(),
                            p()->gain.master_of_elements, this );
    }

    // Heating Up: non-periodic crits of Fireball, Frostfire Bolt, Fire Blast and Scorch
    if ( triggers_heating_up && crit && p()->talent.heating_up.ok() )
    {
      p()->buff.heating_up->trigger();
      p()->proc.heating_up->occur();
    }

    // Combustion: each fire hit adds +10% fire crit, each fire crit uses a charge (3)
    if ( affected_by_combustion && p()->buff.combustion->check() )
    {
      if ( crit )
      {
        p()->buff.combustion->decrement();
        if ( !p()->buff.combustion->check() )
          p()->buff.combustion_crit->expire();
      }
      else
      {
        p()->buff.combustion_crit->trigger();
      }
    }
  }
};

using mage_spell_t = mage_action_t<spell_t>;

// Talent abilities: the talent teaches rank 1, higher ranks come from the trainer. Taking the talent node is
// the gate (some talent spells keep a classic spell level above the Forever tree's level gates).
template <typename RANKS>
const spell_data_t* talent_rank_spell( const mage_t* p, const player_talent_t& talent, const RANKS& ranks )
{
  if ( talent.rank() == 0 )
    return spell_data_t::not_found();

  const spell_data_t* s = p->find_spell( max_rank( p, ranks ).spell_id );
  if ( !s->ok() )
    s = dbc::find_spell( p, talent->id() );
  return s;
}

// ==========================================================================
// Arcane
// ==========================================================================

struct arcane_missile_tick_t : public mage_spell_t
{
  arcane_missile_tick_t( mage_t* p, unsigned id ) : mage_spell_t( "arcane_missiles_tick", p, dbc::find_spell( p, id ) )
  {
    background = dual = true;
    direct_tick = false;
  }
};

// Arcane Missiles: channel (periodic trigger every 1 s) that fires one missile spell per tick.
struct arcane_missiles_t : public mage_spell_t
{
  arcane_missiles_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "arcane_missiles", p, p->find_spell( max_rank( p, arcane_missiles_ranks ).spell_id ) )
  {
    parse_options( options_str );

    channeled    = true;
    may_miss     = false;
    may_crit     = false;
    hasted_ticks = false;
    triggers_arcane_concentration = false;  // the missiles proc it

    if ( data().ok() )
    {
      tick_action = new arcane_missile_tick_t( p, max_rank( p, arcane_missiles_ranks ).tick_id );
      add_child( tick_action );
      const auto& periodic = data().effectN( 1 );
      base_tick_time = periodic.period() > 0_ms ? periodic.period() : 1_s;
      dot_duration   = data().duration();
    }
  }

  // Missile Barrage is used up when the channel starts, but its faster missiles (flat tick time, effect #2)
  // last for the whole channel, so the empowered state is remembered per channel.
  bool barrage_channel = false;

  void execute() override
  {
    barrage_channel = p()->buff.missile_barrage->check() > 0;

    mage_spell_t::execute();
  }

  timespan_t tick_time( const action_state_t* s ) const override
  {
    timespan_t t = mage_spell_t::tick_time( s );

    if ( barrage_channel && !p()->buff.missile_barrage->check() )
      t += p()->buff.missile_barrage->data().effectN( 2 ).time_value();

    return std::max( 100_ms, t );
  }

  void last_tick( dot_t* d ) override
  {
    mage_spell_t::last_tick( d );
    barrage_channel = false;
  }
};

// Arcane Blast (11-point gold talent + trainer ranks R2-R5). Each cast adds a stack of 400573: +15% of
// Intellect as spell damage (mage_t::composite_spell_power) and +175% Arcane Blast mana cost. The stack
// lasts 15 s from the first application and is not refreshed.
struct arcane_blast_t : public mage_spell_t
{
  arcane_blast_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "arcane_blast", p, talent_rank_spell( p, p->talent.arcane_blast, arcane_blast_ranks ) )
  {
    parse_options( options_str );
    missile_barrage_chance = 1.0;
  }

  void execute() override
  {
    mage_spell_t::execute();

    p()->buff.arcane_blast->trigger();
  }
};

// Arcane Wrath (16-point gold talent, 70425): gains spell damage equal to 5% of maximum mana (effect #2)
struct arcane_wrath_t : public mage_spell_t
{
  double max_mana_sp;

  arcane_wrath_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "arcane_wrath", p, talent_rank_spell( p, p->talent.arcane_wrath, arcane_wrath_ranks ) ),
    max_mana_sp( data().effectN( 2 ).percent() )
  {
    parse_options( options_str );
    missile_barrage_chance = 0.5;
  }

  double composite_total_spell_power() const override
  {
    return mage_spell_t::composite_total_spell_power() +
           std::floor( max_mana_sp * p()->resources.max[ RESOURCE_MANA ] );
  }
};

struct arcane_explosion_t : public mage_spell_t
{
  arcane_explosion_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "arcane_explosion", p, p->find_spell( max_rank( p, arcane_explosion_ranks ).spell_id ) )
  {
    parse_options( options_str );
    aoe = -1;
  }
};

struct presence_of_mind_t : public mage_spell_t
{
  presence_of_mind_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "presence_of_mind", p, p->talent.presence_of_mind.rank() ? dbc::find_spell( p, 12043 )
                                                                              : spell_data_t::not_found() )
  {
    parse_options( options_str );
    harmful = false;
    target  = p;
  }

  void execute() override
  {
    mage_spell_t::execute();
    p()->buff.presence_of_mind->trigger();
  }
};

struct arcane_power_t : public mage_spell_t
{
  arcane_power_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "arcane_power", p, p->talent.arcane_power.rank() ? dbc::find_spell( p, 12042 )
                                                                      : spell_data_t::not_found() )
  {
    parse_options( options_str );
    harmful = false;
    target  = p;
  }

  void execute() override
  {
    mage_spell_t::execute();
    p()->buff.arcane_power->trigger();
  }
};

// Evocation: 8 s channel, +1500% mana regeneration (aura 110) and 100% of it while casting (aura 134)
struct evocation_t : public mage_spell_t
{
  evocation_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "evocation", p, p->find_spell( 12051 ) )
  {
    parse_options( options_str );
    harmful      = false;
    channeled    = true;
    hasted_ticks = false;
    target       = p;
    base_tick_time = 1_s;
    dot_duration   = data().duration();
    triggers_arcane_concentration = false;
  }

  void execute() override
  {
    mage_spell_t::execute();
    p()->buff.evocation->trigger();
  }

  void last_tick( dot_t* d ) override
  {
    mage_spell_t::last_tick( d );
    p()->buff.evocation->expire();
  }
};

// ==========================================================================
// Fire
// ==========================================================================

struct fireball_t : public mage_spell_t
{
  fireball_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "fireball", p, p->find_spell( max_rank( p, fireball_ranks ).spell_id ) )
  {
    parse_options( options_str );
    triggers_heating_up    = true;
    missile_barrage_chance = 0.5;
  }
};

struct fire_blast_t : public mage_spell_t
{
  fire_blast_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "fire_blast", p, p->find_spell( max_rank( p, fire_blast_ranks ).spell_id ) )
  {
    parse_options( options_str );
    triggers_heating_up = true;
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    // Heating Up: Fire Blast grants an additional stack every time it deals damage
    if ( result_is_hit( s->result ) && p()->talent.heating_up.ok() )
      p()->buff.heating_up->trigger();
  }
};

struct scorch_t : public mage_spell_t
{
  scorch_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "scorch", p, p->find_spell( max_rank( p, scorch_ranks ).spell_id ) )
  {
    parse_options( options_str );
    triggers_heating_up = true;
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    // Improved Scorch: 33/67/100% chance to add a Fire Vulnerability stack (Forever: this mage only)
    if ( result_is_hit( s->result ) && p()->talent.improved_scorch.ok() &&
         rng().roll( p()->talent.improved_scorch->effectN( 1 ).percent() ) )
      td( s->target )->debuff.fire_vulnerability->trigger();
  }
};

// Pyroblast (11-point gold talent + trainer ranks). Heating Up stacks cut the cast time and are consumed.
struct pyroblast_t : public mage_spell_t
{
  pyroblast_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "pyroblast", p, talent_rank_spell( p, p->talent.pyroblast, pyroblast_ranks ) )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    bool consume = p()->buff.heating_up->check();

    mage_spell_t::execute();

    if ( consume )
      p()->buff.heating_up->expire();
  }
};

struct blast_wave_t : public mage_spell_t
{
  blast_wave_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "blast_wave", p, talent_rank_spell( p, p->talent.blast_wave, blast_wave_ranks ) )
  {
    parse_options( options_str );
    aoe = -1;
  }
};

struct flamestrike_tick_t : public mage_spell_t
{
  flamestrike_tick_t( mage_t* p, unsigned id ) : mage_spell_t( "flamestrike_tick", p, dbc::find_spell( p, id ) )
  {
    background = dual = true;
    aoe = -1;
    triggers_arcane_concentration = false;
  }
};

// Flamestrike: direct AoE plus a 4 x 2 s ground effect from the rank's tick spell (one per mage)
struct flamestrike_t : public mage_spell_t
{
  flamestrike_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "flamestrike", p, p->find_spell( max_rank( p, flamestrike_ranks ).spell_id ) )
  {
    parse_options( options_str );
    aoe = -1;

    if ( data().ok() )
    {
      tick_action = new flamestrike_tick_t( p, max_rank( p, flamestrike_ranks ).tick_id );
      add_child( tick_action );
      base_tick_time = 2_s;
      dot_duration   = data().duration();
      hasted_ticks   = false;
    }
  }
};

struct combustion_t : public mage_spell_t
{
  combustion_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "combustion", p, p->talent.combustion.rank() ? dbc::find_spell( p, 11129 )
                                                                  : spell_data_t::not_found() )
  {
    parse_options( options_str );
    harmful = false;
    target  = p;
  }

  void execute() override
  {
    mage_spell_t::execute();
    p()->buff.combustion->trigger( p()->buff.combustion->max_stack() );
  }

  bool verify_actor_level() const override
  { return true; }
};

// Ignite: rolling per-mage DoT (Forever), 412538 ticks every 2 s for 4 s. The amount is taken from the
// critical strike, which already includes all damage multipliers, so the ticks are not modified again.
struct ignite_t : public residual_action::residual_periodic_action_t<mage_spell_t>
{
  ignite_t( mage_t* p ) : residual_action_t( "ignite", p, p->spell.ignite )
  {
    school         = SCHOOL_FIRE;
    base_tick_time = 2_s;
    dot_duration   = 4_s;
    hasted_ticks   = false;
    tick_may_crit  = false;
    may_miss       = false;
    triggers_arcane_concentration = false;
  }

  void init() override
  {
    residual_action_t::init();
    snapshot_flags = update_flags = 0;
  }
};

// ==========================================================================
// Frost
// ==========================================================================

struct frostbolt_t : public mage_spell_t
{
  frostbolt_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "frostbolt", p, p->find_spell( max_rank( p, frostbolt_ranks ).spell_id ) )
  {
    parse_options( options_str );
    is_chill               = true;
    triggers_winters_chill = true;
    missile_barrage_chance = 0.5;
  }
};

// Frost Blast (70425, baseline): 1.5 s weak chill nuke
struct frost_blast_t : public mage_spell_t
{
  frost_blast_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "frost_blast", p, p->find_spell( max_rank( p, frost_blast_ranks ).spell_id ) )
  {
    parse_options( options_str );
    is_chill               = true;
    triggers_winters_chill = true;
  }
};

// Frostfire Bolt: fire + frost, counts as both (not for Fire Power)
struct frostfire_bolt_t : public mage_spell_t
{
  frostfire_bolt_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "frostfire_bolt", p, p->find_spell( max_rank( p, frostfire_bolt_ranks ).spell_id ) )
  {
    parse_options( options_str );
    is_chill               = true;
    triggers_heating_up    = true;
    missile_barrage_chance = 0.5;
  }
};

// Ice Lance (11-point gold talent + trainer ranks): +200% (effect #2) against Frozen targets.
struct ice_lance_t : public mage_spell_t
{
  double frozen_multiplier;

  ice_lance_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "ice_lance", p, talent_rank_spell( p, p->talent.ice_lance, ice_lance_ranks ) ),
    frozen_multiplier( 1.0 + data().effectN( 2 ).percent() )
  {
    parse_options( options_str );

    // ESTIMATE: the client data has no spell power coefficient on any rank. TBC's Ice Lance used 0.143
    // (1/7, an instant with a x3 frozen multiplier); use that until measured (validation hook V7).
    if ( spell_power_mod.direct == 0 )
      spell_power_mod.direct = 0.143;
  }

  void execute() override
  {
    // Fingers of Frost empowered Ice Lances can apply Winter's Chill
    triggers_winters_chill = p()->buff.fingers_of_frost->check() > 0;

    mage_spell_t::execute();
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double m = mage_spell_t::composite_target_da_multiplier( t );

    if ( p()->frozen_for_action( t ) )
      m *= frozen_multiplier;

    return m;
  }
};

// Cone of Cold: Winter's Chill stacks add +7% spell power coefficient each (dummy effect #2) and are consumed
struct cone_of_cold_t : public mage_spell_t
{
  double wc_coefficient;

  cone_of_cold_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "cone_of_cold", p, p->find_spell( max_rank( p, cone_of_cold_ranks ).spell_id ) ),
    wc_coefficient( p->spell.winters_chill->effectN( 2 ).percent() )
  {
    parse_options( options_str );
    aoe      = -1;
    is_chill = true;
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    double c = mage_spell_t::spell_direct_power_coefficient( s );

    if ( p()->talent.winters_chill.ok() )
      c += wc_coefficient * td( s->target )->debuff.winters_chill->check();

    return c;
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    td( s->target )->debuff.winters_chill->expire();
  }

  bool ready() override
  {
    if ( !p()->options.cone_of_cold_in_range )
      return false;

    return mage_spell_t::ready();
  }
};

struct frost_nova_t : public mage_spell_t
{
  frost_nova_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "frost_nova", p, p->find_spell( max_rank( p, frost_nova_ranks ).spell_id ) )
  {
    parse_options( options_str );
    aoe = -1;
  }

  void impact( action_state_t* s ) override
  {
    mage_spell_t::impact( s );

    if ( result_is_hit( s->result ) && p()->options.target_freezable )
      td( s->target )->debuff.frostbite->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() );
  }
};

struct blizzard_tick_t : public mage_spell_t
{
  blizzard_tick_t( mage_t* p, unsigned id ) : mage_spell_t( "blizzard_tick", p, dbc::find_spell( p, id ) )
  {
    background = dual = true;
    aoe = -1;
    triggers_arcane_concentration = false;
  }
};

// Blizzard: 8 s channel, one tick spell per second (Forever 70425 tick spells)
struct blizzard_t : public mage_spell_t
{
  blizzard_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "blizzard", p, p->find_spell( max_rank( p, blizzard_ranks ).spell_id ) )
  {
    parse_options( options_str );
    channeled    = true;
    may_miss     = false;
    may_crit     = false;
    hasted_ticks = false;
    triggers_arcane_concentration = false;
    // The channel's own dummy effect has a stray coefficient; damage is only the tick spell
    base_dd_min = base_dd_max = 0;
    spell_power_mod.direct    = 0;

    if ( data().ok() )
    {
      tick_action = new blizzard_tick_t( p, max_rank( p, blizzard_ranks ).tick_id );
      add_child( tick_action );
      base_tick_time = 1_s;
      dot_duration   = data().duration();
    }
  }
};

// Cold Snap: finishes the cooldown of the other Frost spells
struct cold_snap_t : public mage_spell_t
{
  std::vector<cooldown_t*> frost_cooldowns;

  cold_snap_t( mage_t* p, std::string_view options_str ) :
    mage_spell_t( "cold_snap", p, p->talent.cold_snap.rank() ? dbc::find_spell( p, 12472 )
                                                                : spell_data_t::not_found() )
  {
    parse_options( options_str );
    harmful = false;
    target  = p;

    for ( auto name : { "cone_of_cold", "frost_nova", "ice_block", "ice_barrier" } )
      frost_cooldowns.push_back( p->get_cooldown( name ) );
  }

  void execute() override
  {
    mage_spell_t::execute();

    for ( auto cd : frost_cooldowns )
      cd->reset( false );
  }
};

// ==========================================================================
// Buff spells and mana
// ==========================================================================

struct mage_buff_spell_t : public mage_spell_t
{
  buff_t* buff;
  std::vector<buff_t*> exclusive;

  mage_buff_spell_t( std::string_view n, mage_t* p, const spell_data_t* s, buff_t* b, std::string_view options_str ) :
    mage_spell_t( n, p, s ), buff( b )
  {
    parse_options( options_str );
    harmful  = false;
    may_crit = false;
    target   = p;
    triggers_arcane_concentration = false;
  }

  void execute() override
  {
    mage_spell_t::execute();

    for ( auto b : exclusive )
      if ( b != buff )
        b->expire();
    buff->trigger();
  }

  bool ready() override
  {
    if ( buff->check() )
      return false;

    return mage_spell_t::ready();
  }
};

// Mana gem: one charge of every gem the mage can conjure at their level (conjured before the fight), best
// first; all share the 60 s category cooldown of the restore spells (70425 data; vanilla was 120 s).
struct mana_gem_t : public action_t
{
  std::vector<const spell_data_t*> gems;
  size_t used = 0;

  mana_gem_t( mage_t* p, std::string_view options_str ) : action_t( ACTION_USE, "mana_gem", p )
  {
    parse_options( options_str );
    harmful     = false;
    trigger_gcd = 0_ms;
    background  = false;

    for ( const auto& r : mana_gem_ranks )
      if ( p->find_spell( r.spell_id )->ok() )
        gems.push_back( dbc::find_spell( p, r.restore_id ) );
    std::reverse( gems.begin(), gems.end() );

    cooldown = p->get_cooldown( "mana_gem" );
    if ( !gems.empty() )
    {
      cooldown->duration = std::max( gems.front()->cooldown(), gems.front()->category_cooldown() );
      s_data_reporting   = gems.front();
    }
  }

  const spell_data_t* next_gem() const
  { return used < gems.size() ? gems[ used ] : spell_data_t::not_found(); }

  void execute() override
  {
    const auto& e = next_gem()->effectN( 1 );
    double amount = rng().range( e.min( player ), e.max( player ) );
    player->resource_gain( RESOURCE_MANA, amount, debug_cast<mage_t*>( player )->gain.mana_gem, this );
    used++;
    update_ready();
  }

  bool ready() override
  {
    if ( !next_gem()->ok() )
      return false;

    if ( player->resources.max[ RESOURCE_MANA ] - player->resources.current[ RESOURCE_MANA ] <
         next_gem()->effectN( 1 ).max( player ) )
      return false;

    return action_t::ready();
  }

  void reset() override
  {
    action_t::reset();
    used = 0;
  }
};

// ==========================================================================
// mage_t
// ==========================================================================

void mage_t::create_options()
{
  player_t::create_options();

  add_option( opt_bool( "mage.cone_of_cold_in_range", options.cone_of_cold_in_range ) );
  add_option( opt_bool( "mage.target_frozen", options.target_frozen ) );
  add_option( opt_bool( "mage.target_freezable", options.target_freezable ) );
}

void mage_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  options = debug_cast<mage_t*>( source )->options;
}

action_t* mage_t::create_action( std::string_view name, std::string_view options_str )
{
  // Arcane
  if ( name == "arcane_missiles" )   return new arcane_missiles_t( this, options_str );
  if ( name == "arcane_blast" )      return new arcane_blast_t( this, options_str );
  if ( name == "arcane_wrath" )      return new arcane_wrath_t( this, options_str );
  if ( name == "arcane_explosion" )  return new arcane_explosion_t( this, options_str );
  if ( name == "presence_of_mind" )  return new presence_of_mind_t( this, options_str );
  if ( name == "arcane_power" )      return new arcane_power_t( this, options_str );
  if ( name == "evocation" )         return new evocation_t( this, options_str );
  // Fire
  if ( name == "fireball" )          return new fireball_t( this, options_str );
  if ( name == "fire_blast" )        return new fire_blast_t( this, options_str );
  if ( name == "scorch" )            return new scorch_t( this, options_str );
  if ( name == "pyroblast" )         return new pyroblast_t( this, options_str );
  if ( name == "blast_wave" )        return new blast_wave_t( this, options_str );
  if ( name == "flamestrike" )       return new flamestrike_t( this, options_str );
  if ( name == "combustion" )        return new combustion_t( this, options_str );
  // Frost
  if ( name == "frostbolt" )         return new frostbolt_t( this, options_str );
  if ( name == "frost_blast" )       return new frost_blast_t( this, options_str );
  if ( name == "frostfire_bolt" )    return new frostfire_bolt_t( this, options_str );
  if ( name == "ice_lance" )         return new ice_lance_t( this, options_str );
  if ( name == "cone_of_cold" )      return new cone_of_cold_t( this, options_str );
  if ( name == "frost_nova" )        return new frost_nova_t( this, options_str );
  if ( name == "blizzard" )          return new blizzard_t( this, options_str );
  if ( name == "cold_snap" )         return new cold_snap_t( this, options_str );
  // Buffs and mana
  if ( name == "arcane_intellect" )
    return new mage_buff_spell_t( name, this, find_spell( max_rank( this, arcane_intellect_ranks ).spell_id ),
                                  buff.arcane_intellect, options_str );
  if ( name == "mage_armor" || name == "ice_armor" || name == "frost_armor" )
  {
    const spell_data_t* s = name == "mage_armor"  ? find_spell( max_rank( this, mage_armor_ranks ).spell_id )
                          : name == "ice_armor"   ? find_spell( max_rank( this, ice_armor_ranks ).spell_id )
                                                  : find_spell( max_rank( this, frost_armor_ranks ).spell_id );
    buff_t* b = name == "mage_armor" ? buff.mage_armor : name == "ice_armor" ? buff.ice_armor : buff.frost_armor;
    auto a = new mage_buff_spell_t( name, this, s, b, options_str );
    a->exclusive = { buff.mage_armor, buff.ice_armor, buff.frost_armor };
    return a;
  }
  if ( name == "mana_gem" )          return new mana_gem_t( this, options_str );

  return player_t::create_action( name, options_str );
}

void mage_t::create_actions()
{
  if ( talent.ignite.ok() )
    ignite = new ignite_t( this );

  player_t::create_actions();
}

void mage_t::create_buffs()
{
  player_t::create_buffs();

  // Arcane Blast 400573: 4 stacks, 15 s from the first application (not refreshed)
  buff.arcane_blast = make_buff( this, "arcane_blast", find_spell( 400573 ) )
                        ->set_refresh_behavior( buff_refresh_behavior::DISABLED )
                        ->add_invalidate( CACHE_SPELL_POWER );

  buff.clearcasting     = make_buff( this, "clearcasting", find_spell( 12536 ) )->set_chance( 1.0 );
  buff.missile_barrage  = make_buff( this, "missile_barrage", find_spell( 400589 ) )->set_chance( 1.0 );
  buff.presence_of_mind = make_buff( this, "presence_of_mind", find_spell( 12043 ) )->set_cooldown( 0_ms );
  buff.arcane_power     = make_buff( this, "arcane_power", find_spell( 12042 ) )->set_cooldown( 0_ms );
  buff.heating_up       = make_buff( this, "heating_up", find_spell( 400625 ) )->set_chance( 1.0 );

  // Combustion: the activated aura holds the crit charges (3), 28682 the stacking crit bonus
  buff.combustion      = make_buff( this, "combustion", find_spell( 11129 ) )->set_cooldown( 0_ms );
  buff.combustion_crit = make_buff( this, "combustion_crit", find_spell( 28682 ) );

  // Fingers of Frost: charges = talent effect #1 (1/2)
  buff.fingers_of_frost = make_buff( this, "fingers_of_frost", find_spell( 400669 ) )
                            ->set_max_stack( std::max( 1, as<int>( talent.fingers_of_frost->effectN( 1 ).base_value() ) ) );

  buff.evocation = make_buff( this, "evocation", find_spell( 12051 ) )->set_cooldown( 0_ms );

  buff.arcane_intellect =
    make_buff<stat_buff_t>( this, "arcane_intellect", find_spell( max_rank( this, arcane_intellect_ranks ).spell_id ) );
  buff.mage_armor  = make_buff( this, "mage_armor", find_spell( max_rank( this, mage_armor_ranks ).spell_id ) );
  buff.ice_armor   = make_buff( this, "ice_armor", find_spell( max_rank( this, ice_armor_ranks ).spell_id ) );
  buff.frost_armor = make_buff( this, "frost_armor", find_spell( max_rank( this, frost_armor_ranks ).spell_id ) );
}

void mage_t::init_base_stats()
{
  // Vanilla mage melee attack power: Str - 10
  base.attack_power_per_strength = 1.0;

  player_t::init_base_stats();

  base.stats.attack_power = -10.0;

  // ESTIMATE: original classic base crit (wowsims/classic base_stats.go: mage 0.2% spell, 3.2% melee? -> use
  // the spell value for both, melee is irrelevant for the module). Verify against a beta character sheet.
  base.attack_crit_chance = get_passive_player_value( 0.002, "all_crit" );
  base.spell_crit_chance  = get_passive_player_value( 0.002, "all_crit" );
}

void mage_t::init_spells()
{
  player_t::init_spells();

  parse_all_class_passives();
  parse_all_passive_talents();

  auto t = [ this ]( std::string_view n ) { return find_talent_spell( talent_tree::CLASS, n ); };

  talent.arcane_focus         = t( "Arcane Focus" );
  talent.missile_barrage      = t( "Missile Barrage" );
  talent.arcane_concentration = t( "Arcane Concentration" );
  talent.arcane_blast         = t( "Arcane Blast" );
  talent.arcane_wrath         = t( "Arcane Wrath" );
  talent.presence_of_mind     = t( "Presence of Mind" );
  talent.arcane_power         = t( "Arcane Power" );

  talent.ignite             = t( "Ignite" );
  talent.pyroblast          = t( "Pyroblast" );
  talent.improved_scorch    = t( "Improved Scorch" );
  talent.heating_up         = t( "Heating Up" );
  talent.master_of_elements = t( "Master of Elements" );
  talent.blast_wave         = t( "Blast Wave" );
  talent.fire_power         = t( "Fire Power" );
  talent.combustion         = t( "Combustion" );

  talent.elemental_precision = t( "Elemental Precision" );
  talent.frostbite           = t( "Frostbite" );
  talent.ice_lance           = t( "Ice Lance" );
  talent.shatter             = t( "Shatter" );
  talent.cold_snap           = t( "Cold Snap" );
  talent.fingers_of_frost    = t( "Fingers of Frost" );
  talent.winters_chill       = t( "Winter's Chill" );

  spell.ignite             = find_spell( 412538 );
  spell.fire_vulnerability = find_spell( 22959 );
  spell.winters_chill      = talent.winters_chill.ok() ? find_spell( 12579 ) : spell_data_t::not_found();

  arcane_concentration_icd = get_cooldown( "arcane_concentration_icd" );
  arcane_concentration_icd->duration = talent.arcane_concentration->internal_cooldown();
}

void mage_t::init_gains()
{
  player_t::init_gains();

  gain.master_of_elements = get_gain( "master_of_elements" );
  gain.mana_gem           = get_gain( "mana_gem" );
  gain.evocation          = get_gain( "evocation" );
}

void mage_t::init_procs()
{
  player_t::init_procs();

  proc.clearcasting     = get_proc( "clearcasting" );
  proc.missile_barrage  = get_proc( "missile_barrage" );
  proc.fingers_of_frost = get_proc( "fingers_of_frost" );
  proc.winters_chill    = get_proc( "winters_chill" );
  proc.heating_up       = get_proc( "heating_up" );
}

void mage_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  if ( c == CACHE_INTELLECT && buff.arcane_blast && buff.arcane_blast->check() )
    player_t::invalidate_cache( CACHE_SPELL_POWER );
}

// Arcane Blast (aura 174): +15% of Intellect as spell damage per stack, all schools
double mage_t::composite_spell_power( school_e school ) const
{
  double sp = player_t::composite_spell_power( school );

  if ( buff.arcane_blast->check() )
    sp += buff.arcane_blast->data().effectN( 1 ).percent() * buff.arcane_blast->check() * cache.intellect();

  return sp;
}

// Mage Armor (50%) and Evocation (100%) keep spirit regen going while casting (aura 134, buffs)
double mage_t::composite_mana_regen_while_casting() const
{
  double f = player_t::composite_mana_regen_while_casting();

  if ( buff.mage_armor->check() )
    f += buff.mage_armor->data().effectN( 2 ).percent();
  if ( buff.evocation->check() )
    f += buff.evocation->data().effectN( 2 ).percent();

  return std::min( f, 1.0 );
}

// Evocation: +1500% mana regeneration (aura 110)
double mage_t::composite_spirit_regen_multiplier() const
{
  double m = player_t::composite_spirit_regen_multiplier();

  if ( buff.evocation->check() )
    m *= 1.0 + buff.evocation->data().effectN( 1 ).percent();

  return m;
}

// Talent tree with the most points: Arcane is columns 1-4, Fire 5-8, Frost 9-12 of trait tree 1112
mage_tree_e mage_t::primary_tree() const
{
  std::array<unsigned, 3> points {};
  for ( const auto& [ tree, entry, rank ] : player_traits )
  {
    if ( tree != talent_tree::CLASS || rank == 0 )
      continue;
    const trait_data_t* trait = trait_data_t::find( entry, is_ptr() );
    if ( !trait || trait->col <= 0 )
      continue;
    points[ std::min( 2, ( trait->col - 1 ) / 4 ) ] += rank;
  }

  if ( points[ 1 ] >= points[ 0 ] && points[ 1 ] >= points[ 2 ] && points[ 1 ] > 0 )
    return mage_tree_e::FIRE;
  if ( points[ 2 ] > points[ 0 ] )
    return mage_tree_e::FROST;
  if ( points[ 0 ] > 0 )
    return mage_tree_e::ARCANE;
  return mage_tree_e::FIRE;
}

bool mage_t::target_is_frozen( player_t* t ) const
{
  return options.target_frozen || get_target_data( t )->debuff.frostbite->check();
}

// Shatter / Ice Lance: Frozen, or Fingers of Frost charges ("treats your next spells as if the target were Frozen")
bool mage_t::frozen_for_action( player_t* t ) const
{
  return target_is_frozen( t ) || buff.fingers_of_frost->check();
}

// Fire Power: own Fire effects on the target. ESTIMATE (V4): Ignite, the Fireball, Pyroblast and Flamestrike
// DoTs and Fire Vulnerability (once, regardless of stacks) count.
int mage_t::fire_effects_on( player_t* t ) const
{
  const mage_td_t* td = get_target_data( t );
  int n = 0;

  if ( td->dots.ignite->is_ticking() )
    n++;
  if ( td->debuff.fire_vulnerability->check() )
    n++;

  for ( auto name : { "fireball", "pyroblast", "flamestrike" } )
    if ( dot_t* d = t->find_dot( name, const_cast<mage_t*>( this ) ); d && d->is_ticking() )
      n++;

  return n;
}

std::unique_ptr<expr_t> mage_t::create_expression( std::string_view name )
{
  auto splits = util::string_split<std::string_view>( name, "." );

  if ( splits.size() == 2 && splits[ 0 ] == "primary_tree" )
  {
    mage_tree_e tree;
    if ( splits[ 1 ] == "arcane" )
      tree = mage_tree_e::ARCANE;
    else if ( splits[ 1 ] == "fire" )
      tree = mage_tree_e::FIRE;
    else if ( splits[ 1 ] == "frost" )
      tree = mage_tree_e::FROST;
    else
      throw std::invalid_argument( fmt::format( "Unknown talent tree '{}'.", splits[ 1 ] ) );

    return expr_t::create_constant( name, primary_tree() == tree );
  }

  if ( name == "target_frozen" )
    return make_fn_expr( name, [ this ] { return target_is_frozen( target ); } );

  return player_t::create_expression( name );
}

void mage_t::init_action_list()
{
  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  clear_action_priority_lists();

  switch ( primary_tree() )
  {
    case mage_tree_e::ARCANE: mage_apl::arcane( this ); break;
    case mage_tree_e::FROST:  mage_apl::frost( this ); break;
    default:                  mage_apl::fire( this ); break;
  }

  use_default_action_list = true;
  player_t::init_action_list();
}

// MAGE MODULE INTERFACE ====================================================

struct mage_module_t final : public module_t
{
public:
  mage_module_t() : module_t( MAGE ) {}

  player_t* create_player( sim_t* sim, std::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new mage_t( sim, name, r );
    return p;
  }

  void register_hotfixes() const override {}

  bool valid() const override { return true; }

  void register_actor_initializers( sim_t* ) const override {}
};

}  // UNNAMED NAMESPACE

const module_t* module_t::mage()
{
  static mage_module_t m;
  return &m;
}

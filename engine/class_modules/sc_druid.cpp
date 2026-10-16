// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
//
// WoW: Forever druid. Feral (Cat) and Balance damage dealers. Bear tanking and Restoration healing are not
// modelled yet (see FOREVER_DRUID.md at the repository root).

#include "simulationcraft.hpp"
#include "action/parse_effects.hpp"
#include "class_modules/apl/druid.hpp"
#include "class_modules/forever_helpers.hpp"

namespace
{
using forever::max_rank;
using forever::spell_rank_t;

// ==========================================================================
// Ranks (explicit ids: the client also carries Season of Discovery copies of several druid spells)
// ==========================================================================

constexpr std::array wrath_ranks {
  spell_rank_t{ 5176 },  // 1
  spell_rank_t{ 5177 },  // 6
  spell_rank_t{ 5178 },  // 14
  spell_rank_t{ 5179 },  // 22
  spell_rank_t{ 5180 },  // 30
  spell_rank_t{ 6780 },  // 38
  spell_rank_t{ 8905 },  // 46
  spell_rank_t{ 9912 },  // 54
};

constexpr std::array starfire_ranks {
  spell_rank_t{ 2912 },   // 20
  spell_rank_t{ 8949 },   // 26
  spell_rank_t{ 8950 },   // 34
  spell_rank_t{ 8951 },   // 42
  spell_rank_t{ 9875 },   // 50
  spell_rank_t{ 9876 },   // 58
  spell_rank_t{ 25298 },  // 60
};

constexpr std::array moonfire_ranks {
  spell_rank_t{ 8921 },  // 4
  spell_rank_t{ 8924 },  // 10
  spell_rank_t{ 8925 },  // 16
  spell_rank_t{ 8926 },  // 22
  spell_rank_t{ 8927 },  // 28
  spell_rank_t{ 8928 },  // 34
  spell_rank_t{ 8929 },  // 40
  spell_rank_t{ 9833 },  // 46
  spell_rank_t{ 9834 },  // 52
  spell_rank_t{ 9835 },  // 58
};

// Talent ability (16 point Balance milestone); rank 1 comes with the talent, the rest from the trainer
constexpr std::array insect_swarm_ranks {
  spell_rank_t{ 5570 },   // 20
  spell_rank_t{ 24974 },  // 30
  spell_rank_t{ 24975 },  // 40
  spell_rank_t{ 24976 },  // 50
  spell_rank_t{ 24977 },  // 60
};

constexpr std::array faerie_fire_ranks {
  spell_rank_t{ 770 },   // 18
  spell_rank_t{ 778 },   // 30
  spell_rank_t{ 9749 },  // 42
  spell_rank_t{ 9907 },  // 54
};

constexpr std::array mark_of_the_wild_ranks {
  spell_rank_t{ 1126 },  // 1
  spell_rank_t{ 5232 },  // 10
  spell_rank_t{ 6756 },  // 20
  spell_rank_t{ 5234 },  // 30
  spell_rank_t{ 8907 },  // 40
  spell_rank_t{ 9884 },  // 50
  spell_rank_t{ 9885 },  // 60
};

constexpr std::array claw_ranks {
  spell_rank_t{ 1082 },  // 20
  spell_rank_t{ 3029 },  // 28
  spell_rank_t{ 5201 },  // 38
  spell_rank_t{ 9849 },  // 48
  spell_rank_t{ 9850 },  // 58
};

constexpr std::array shred_ranks {
  spell_rank_t{ 5221 },  // 20
  spell_rank_t{ 6800 },  // 30
  spell_rank_t{ 8992 },  // 38
  spell_rank_t{ 9829 },  // 46
  spell_rank_t{ 9830 },  // 54
};

constexpr std::array rake_ranks {
  spell_rank_t{ 1822 },  // 24
  spell_rank_t{ 1823 },  // 34
  spell_rank_t{ 1824 },  // 44
  spell_rank_t{ 9904 },  // 54
};

constexpr std::array rip_ranks {
  spell_rank_t{ 1079 },  // 20
  spell_rank_t{ 9492 },  // 28
  spell_rank_t{ 9493 },  // 36
  spell_rank_t{ 9752 },  // 44
  spell_rank_t{ 9894 },  // 52
  spell_rank_t{ 9896 },  // 60
};

constexpr std::array ferocious_bite_ranks {
  spell_rank_t{ 22568 },  // 32
  spell_rank_t{ 22827 },  // 40
  spell_rank_t{ 22828 },  // 48
  spell_rank_t{ 22829 },  // 56
  spell_rank_t{ 31018 },  // 60
};

// Spell ids without ranks
constexpr unsigned CAT_FORM            = 768;
constexpr unsigned CAT_FORM_PASSIVE    = 3025;
constexpr unsigned MOONKIN_FORM        = 24858;
constexpr unsigned MOONKIN_AURA        = 24907;
constexpr unsigned OMEN_OF_CLARITY     = 16864;
constexpr unsigned CLEARCASTING        = 16870;
constexpr unsigned NATURES_GRACE_BUFF  = 16886;
constexpr unsigned ECLIPSE_BUFF        = 408255;
constexpr unsigned LEADER_OF_THE_PACK  = 24932;
constexpr unsigned INNERVATE           = 29166;

// ESTIMATE (2026-10-16): the client data has no attack power share for the cat finishers (Rip's effect #2 and
// Ferocious Bite's tooltip "Damage is increased by your Attack Power" are not backed by a coefficient). Vanilla
// values (wowsims/classic): Rip 1% of attack power per tick per combo point, capped at 4 combo points (24% of
// AP over a full Rip); Ferocious Bite 3% of attack power per combo point. Rake has no attack power share in
// vanilla or in the data. Override with druid.rip_ap_per_cp= / druid.ferocious_bite_ap_per_cp=.
constexpr double RIP_AP_PER_CP_PER_TICK = 0.01;
constexpr int    RIP_AP_MAX_CP          = 4;
constexpr double FEROCIOUS_BITE_AP_PER_CP = 0.03;

// Omen of Clarity (2026-10-16): SpellAuraOptions for 16864 in 1.60.1.70425 is ProcChance 100, ProcCategoryRecovery
// 10 s, no SpellProcsPerMinute row. Moonkin Form halves the internal cooldown (effect #6, -50% proc cooldown) and
// raises the chance by 100% (no effect at 100%). We use the client values; the server may roll a lower chance
// (Blizzard: low ranks proc less). Override with druid.omen_of_clarity_chance=.

struct druid_t;

struct druid_td_t : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* rake         = nullptr;
    dot_t* rip          = nullptr;
    dot_t* moonfire     = nullptr;
    dot_t* insect_swarm = nullptr;
  } dots;

  struct debuffs_t
  {
    buff_t* faerie_fire = nullptr;
  } debuff;

  druid_td_t( player_t* target, druid_t* p );

  // Rend and Tear: Rake's bleed (effect #2) and Rip
  bool bleeding() const
  { return dots.rake->is_ticking() || dots.rip->is_ticking(); }
};

struct druid_t final : public parse_player_effects_t
{
  druid_t( sim_t* sim, std::string_view name, race_e r = RACE_NIGHT_ELF )
    : parse_player_effects_t( sim, DRUID, name, r ), target_data()
  {}

  // Cat Form fixes the swing at 1.0 s over the equipped weapon's damage per second (official, 2026-09-30)
  weapon_t cat_weapon;

  // decided in init_action_list: energy for actors that use Cat Form, mana otherwise
  bool cat_dps = false;

  struct options_t
  {
    double rip_ap_per_cp          = RIP_AP_PER_CP_PER_TICK;
    double ferocious_bite_ap_per_cp = FEROCIOUS_BITE_AP_PER_CP;
    double omen_of_clarity_chance = -1.0;  // < 0: client data
  } options;

  struct talents_t
  {
    // Balance
    player_talent_t insect_swarm;
    player_talent_t natures_reach;
    player_talent_t natures_grace;
    player_talent_t eclipse;
    player_talent_t moonkin_form;

    // Feral
    player_talent_t heart_of_the_wild;
    player_talent_t predatory_strikes;
    player_talent_t blood_frenzy;
    player_talent_t shifting_power;
    player_talent_t leader_of_the_pack;
    player_talent_t rend_and_tear;
    player_talent_t berserk;

    // Restoration
    player_talent_t furor;
  } talent;

  struct spells_t
  {
    const spell_data_t* cat_form_passive = spell_data_t::not_found();
    const spell_data_t* omen_of_clarity  = spell_data_t::not_found();
    const spell_data_t* moonkin_aura     = spell_data_t::not_found();
    const spell_data_t* leader_of_the_pack = spell_data_t::not_found();
  } spell;

  struct buffs_t
  {
    buff_t* cat_form         = nullptr;
    buff_t* moonkin_form     = nullptr;
    buff_t* clearcasting     = nullptr;
    buff_t* natures_grace    = nullptr;
    buff_t* eclipse          = nullptr;
    buff_t* berserk          = nullptr;
    buff_t* innervate        = nullptr;
    buff_t* mark_of_the_wild = nullptr;
  } buff;

  struct gains_t
  {
    gain_t* blood_frenzy   = nullptr;
    gain_t* shifting_power = nullptr;
  } gain;

  struct procs_t
  {
    proc_t* clearcasting = nullptr;
    proc_t* blood_frenzy = nullptr;
  } proc;

  cooldown_t* omen_icd = nullptr;

  mutable target_specific_t<druid_td_t> target_data;

  resource_e primary_resource() const override
  { return cat_dps ? RESOURCE_ENERGY : RESOURCE_MANA; }

  std::string default_potion() const override   { return druid_apl::potion( this ); }
  std::string default_flask() const override    { return druid_apl::flask( this ); }
  std::string default_food() const override     { return druid_apl::food( this ); }

  druid_td_t* get_target_data( player_t* target ) const override
  {
    druid_td_t*& td = target_data[ target ];
    if ( !td )
      td = new druid_td_t( target, const_cast<druid_t*>( this ) );
    return td;
  }

  void create_options() override;
  void init_base_stats() override;
  void init_items() override;
  void init_spells() override;
  void init_gains() override;
  void init_procs() override;
  void create_buffs() override;
  action_t* create_action( std::string_view name, std::string_view options_str ) override;
  void init_action_list() override;
  void reset() override;

  double composite_melee_attack_power() const override;
  double composite_attribute_multiplier( attribute_e attr ) const override;
  double composite_melee_crit_chance() const override;
  double composite_spell_crit_chance() const override;
  double composite_spell_haste() const override;
  double composite_melee_hit() const override;
  double composite_spell_hit() const override;
  double composite_player_target_armor( player_t* t ) const override;
  double composite_mana_regen_while_casting() const override;
  double composite_spirit_regen_multiplier() const override;
  void invalidate_cache( cache_e c ) override;

  // Forms
  void shapeshift( buff_t* form );
  void cancel_forms();

  void trigger_omen_of_clarity( action_state_t* s );
};

druid_td_t::druid_td_t( player_t* target, druid_t* p ) : actor_target_data_t( target, p )
{
  dots.rake         = target->get_dot( "rake", p );
  dots.rip          = target->get_dot( "rip", p );
  dots.moonfire     = target->get_dot( "moonfire", p );
  dots.insect_swarm = target->get_dot( "insect_swarm", p );

  // 2026-10-16: the armor reduction is client aura type 674 (unknown to simc) with the flat armor as its
  // negative base value, as for warrior Sunder Armor. Only this druid's own attacks see it.
  debuff.faerie_fire = make_buff( *this, "faerie_fire", p->find_spell( max_rank( p, faerie_fire_ranks ).spell_id ) )
                         ->set_default_value( p->find_spell( max_rank( p, faerie_fire_ranks ).spell_id )
                                                ->effectN( 1 ).base_value() );
}

// ==========================================================================
// Druid action templates
// ==========================================================================

template <class Base>
struct druid_action_t : public parse_action_effects_t<Base>
{
private:
  using ab = parse_action_effects_t<Base>;

public:
  using base_t = druid_action_t<Base>;

  // Clearcasting (Omen of Clarity) makes the next affected ability free; Wrath and free abilities don't use it
  bool affected_by_clearcasting = false;
  bool clearcasting_pending     = false;

  // which form the action needs: cat abilities need Cat Form, caster spells need caster or Moonkin Form
  bool requires_cat_form    = false;
  bool requires_caster_form = false;

  template <typename... Args>
  druid_action_t( Args&&... args ) : ab( std::forward<Args>( args )... )
  {
    affected_by_clearcasting = this->data().affected_by( this->p()->find_spell( CLEARCASTING )->effectN( 1 ) );

    // Berserk: +100% crit for combo point builders (effect #1), Primal Bite effects are bear only
    this->parse_effects( p()->buff.berserk );
    // Nature's Grace: -10% global cooldown (effect #2); the cast speed part (effect #1) is player haste
    this->parse_effects( p()->buff.natures_grace, 0b1U );
  }

  druid_t* p()
  { return debug_cast<druid_t*>( this->player ); }

  const druid_t* p() const
  { return debug_cast<const druid_t*>( this->player ); }

  druid_td_t* td( player_t* t ) const
  { return p()->get_target_data( t ); }

  bool clearcasting_active() const
  {
    return clearcasting_pending ||
           ( affected_by_clearcasting && p()->buff.clearcasting->check() && this->base_cost() > 0 );
  }

  double cost_pct_multiplier() const override
  {
    double m = ab::cost_pct_multiplier();
    if ( clearcasting_active() )
      m = 0.0;
    return m;
  }

  bool ready() override
  {
    if ( requires_cat_form && !p()->buff.cat_form->check() )
      return false;
    if ( requires_caster_form && p()->buff.cat_form->check() )
      return false;

    return ab::ready();
  }

  void execute() override
  {
    // Use up Clearcasting before the hit can proc a new one
    clearcasting_pending = affected_by_clearcasting && p()->buff.clearcasting->check() && this->base_cost() > 0;
    if ( clearcasting_pending )
      p()->buff.clearcasting->expire();

    ab::execute();

    clearcasting_pending = false;
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( this->harmful && !this->background_proc() )
      p()->trigger_omen_of_clarity( s );
  }

  // procs from the actions' own impacts only, not from ticks or the proc actions themselves
  bool background_proc() const
  { return this->proc; }
};

using druid_spell_t  = druid_action_t<spell_t>;
using druid_attack_t = druid_action_t<melee_attack_t>;

// ==========================================================================
// Cat abilities
// ==========================================================================

// Cat abilities use the Cat Form weapon (equipped weapon damage per second at a 1.0 s swing), cost energy, have a
// 1.0 s global cooldown from the spell data and award combo points through their Energize effect.
struct cat_attack_t : public druid_attack_t
{
  bool combo_point_builder = false;
  bool rend_and_tear       = true;   // melee ability bonus against bleeding targets

  cat_attack_t( std::string_view name, druid_t* player, const spell_data_t* s, std::string_view options_str )
    : druid_attack_t( name, player, s )
  {
    parse_options( options_str );

    requires_cat_form = true;
    may_glance        = false;
    special           = true;

    if ( weapon )
      weapon = &player->cat_weapon;

    // The client lists an SP coefficient of 1 on most feral effects; physical abilities don't scale with spell power
    spell_power_mod.direct = 0.0;
    spell_power_mod.tick   = 0.0;

    combo_point_builder = energize_type != action_energize::NONE && energize_resource == RESOURCE_COMBO_POINT;
  }

  // Abilities with "Discount Power On Miss" refund 80% of the energy if they miss, are dodged or parried
  // (vanilla rule, as in the warrior module).
  void consume_resource() override
  {
    resource_e cr = current_resource();

    if ( cr == RESOURCE_ENERGY && cost() > 0 && data().flags( spell_attribute::SX_DISCOUNT_ON_MISS ) &&
         !proc && !result_is_hit( execute_state->result ) )
    {
      last_resource_cost = 0.2 * cost();
      player->resource_loss( cr, last_resource_cost, nullptr, this );
      stats->consume_resource( cr, last_resource_cost );
      return;
    }

    druid_attack_t::consume_resource();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = druid_attack_t::composite_da_multiplier( s );

    // ESTIMATE (2026-10-16): Rend and Tear "melee abilities" read as the cat yellow attacks' direct damage
    if ( rend_and_tear && p()->talent.rend_and_tear.ok() && td( s->target )->bleeding() )
      m *= 1.0 + p()->talent.rend_and_tear->effectN( 1 ).percent();

    return m;
  }

  void impact( action_state_t* s ) override
  {
    druid_attack_t::impact( s );

    // Blood Frenzy: non-periodic crits from combo point builders have a chance to award an extra combo point
    if ( combo_point_builder && s->result == RESULT_CRIT && p()->talent.blood_frenzy.ok() &&
         rng().roll( p()->talent.blood_frenzy->effectN( 2 ).percent() ) )
    {
      p()->resource_gain( RESOURCE_COMBO_POINT, 1, p()->gain.blood_frenzy, this );
      p()->proc.blood_frenzy->occur();
    }
  }
};

struct claw_t : public cat_attack_t
{
  claw_t( druid_t* p, std::string_view options_str )
    : cat_attack_t( "claw", p, p->find_spell( max_rank( p, claw_ranks ).spell_id ), options_str )
  {}
};

// Must be behind the target (sim option position=back)
struct shred_t : public cat_attack_t
{
  shred_t( druid_t* p, std::string_view options_str )
    : cat_attack_t( "shred", p, p->find_spell( max_rank( p, shred_ranks ).spell_id ), options_str )
  {}

  bool ready() override
  {
    if ( player->position() != POSITION_BACK && player->position() != POSITION_RANGED_BACK )
      return false;

    return cat_attack_t::ready();
  }
};

// Direct physical damage (effect #1, armor applies) plus a bleed ticking every 3 s for 9 s (effect #2).
struct rake_t : public cat_attack_t
{
  rake_t( druid_t* p, std::string_view options_str )
    : cat_attack_t( "rake", p, p->find_spell( max_rank( p, rake_ranks ).spell_id ), options_str )
  {
    dot_behavior = DOT_REFRESH_DURATION;
  }
};

// Finishers: spend all combo points (a second power cost of 1-5 combo points in the spell data).
// Combo points live on the druid (simc), not on the target.
struct cat_finisher_state_t : public action_state_t
{
  int combo_points = 0;

  cat_finisher_state_t( action_t* a, player_t* t ) : action_state_t( a, t ) {}

  void initialize() override
  {
    action_state_t::initialize();
    combo_points = 0;
  }

  void copy_state( const action_state_t* s ) override
  {
    action_state_t::copy_state( s );
    combo_points = debug_cast<const cat_finisher_state_t*>( s )->combo_points;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s ) << " combo_points=" << combo_points;
    return s;
  }
};

struct cat_finisher_t : public cat_attack_t
{
  cat_finisher_t( std::string_view name, druid_t* p, const spell_data_t* s, std::string_view options_str )
    : cat_attack_t( name, p, s, options_str )
  {}

  action_state_t* new_state() override
  { return new cat_finisher_state_t( this, target ); }

  // only the initial snapshot reads the combo points; dot ticks update the state through update_state()
  using cat_attack_t::snapshot_state;
  void snapshot_state( action_state_t* s, unsigned flags, result_amount_type rt ) override
  {
    debug_cast<cat_finisher_state_t*>( s )->combo_points =
        as<int>( p()->resources.current[ RESOURCE_COMBO_POINT ] );
    cat_attack_t::snapshot_state( s, flags, rt );
  }

  static int cp( const action_state_t* s )
  { return debug_cast<const cat_finisher_state_t*>( s )->combo_points; }

  bool ready() override
  {
    if ( p()->resources.current[ RESOURCE_COMBO_POINT ] < 1 )
      return false;

    return cat_attack_t::ready();
  }

  void consume_resource() override
  {
    cat_attack_t::consume_resource();

    // a missed finisher keeps its combo points
    if ( result_is_hit( execute_state->result ) )
    {
      double cps = p()->resources.current[ RESOURCE_COMBO_POINT ];
      if ( cps > 0 )
      {
        p()->resource_loss( RESOURCE_COMBO_POINT, cps, nullptr, this );
        stats->consume_resource( RESOURCE_COMBO_POINT, cps );
      }
    }
  }
};

// Rip: each tick deals base + points per combo point (effect #1), every 2 s for 12 s. Bleed, ignores armor.
struct rip_t : public cat_finisher_t
{
  double per_cp;

  rip_t( druid_t* p, std::string_view options_str )
    : cat_finisher_t( "rip", p, p->find_spell( max_rank( p, rip_ranks ).spell_id ), options_str ),
      per_cp( data().effectN( 1 ).pp_combo_points() )
  {
    dot_behavior  = DOT_REFRESH_DURATION;
    rend_and_tear = false;
  }

  double bonus_ta( const action_state_t* s ) const override
  {
    int c = cp( s );
    return cat_finisher_t::bonus_ta( s ) + per_cp * c +
           p()->options.rip_ap_per_cp * std::min( c, RIP_AP_MAX_CP ) * s->composite_attack_power();
  }
};

// Ferocious Bite: base + points per combo point (effect #1) and every point of energy above the cost converted
// into effect #2 / 100 damage (2.7 at rank 5). All remaining energy is spent.
// 2026-10-16: with Clearcasting the base cost is free; we still only convert the energy above the normal cost.
struct ferocious_bite_t : public cat_finisher_t
{
  double per_cp;
  double per_energy;
  double extra_energy = 0.0;

  ferocious_bite_t( druid_t* p, std::string_view options_str )
    : cat_finisher_t( "ferocious_bite", p, p->find_spell( max_rank( p, ferocious_bite_ranks ).spell_id ),
                      options_str ),
      per_cp( data().effectN( 1 ).pp_combo_points() ),
      per_energy( data().effectN( 2 ).base_value() / 100.0 )
  {}

  double normal_cost() const
  {
    auto& bc = base_costs[ RESOURCE_ENERGY ];
    return std::max( 0.0, ( bc.base + bc.flat_add + cost_flat_modifier() ) * bc.pct_mul );
  }

  void execute() override
  {
    extra_energy = std::max( 0.0, p()->resources.current[ RESOURCE_ENERGY ] - normal_cost() );

    cat_finisher_t::execute();
  }

  double bonus_da( const action_state_t* s ) const override
  {
    int c = cp( s );
    return cat_finisher_t::bonus_da( s ) + per_cp * c + per_energy * extra_energy +
           p()->options.ferocious_bite_ap_per_cp * c * s->composite_attack_power();
  }

  void consume_resource() override
  {
    cat_finisher_t::consume_resource();

    if ( extra_energy > 0 && result_is_hit( execute_state->result ) )
    {
      double e = std::min( extra_energy, p()->resources.current[ RESOURCE_ENERGY ] );
      p()->resource_loss( RESOURCE_ENERGY, e, nullptr, this );
      stats->consume_resource( RESOURCE_ENERGY, e );
      last_resource_cost += e;
    }
  }
};

// Shifting Power (16 point Feral milestone): Cat Form only, converts 55% of base mana into 40 energy. Can't be
// used at full energy (beta notes 10-08).
struct shifting_power_t : public druid_spell_t
{
  shifting_power_t( druid_t* p, std::string_view options_str )
    : druid_spell_t( "shifting_power", p,
                     p->talent.shifting_power.ok() ? p->talent.shifting_power.spell() : spell_data_t::not_found() )
  {
    parse_options( options_str );

    harmful           = false;
    may_miss          = false;
    target            = p;
    requires_cat_form = true;
    energize_type     = action_energize::NONE;
  }

  bool ready() override
  {
    if ( p()->resources.current[ RESOURCE_ENERGY ] >= p()->resources.max[ RESOURCE_ENERGY ] )
      return false;

    return druid_spell_t::ready();
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->resource_gain( RESOURCE_ENERGY, data().effectN( 1 ).base_value(), p()->gain.shifting_power, this );
  }
};

// Berserk (31 point Feral talent): off the global cooldown, any feral form
struct berserk_t : public druid_spell_t
{
  berserk_t( druid_t* p, std::string_view options_str )
    : druid_spell_t( "berserk", p, p->talent.berserk.ok() ? p->talent.berserk.spell() : spell_data_t::not_found() )
  {
    parse_options( options_str );

    harmful     = false;
    may_miss    = false;
    target      = p;
    trigger_gcd = 0_ms;
    requires_cat_form = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.berserk->trigger();
  }
};

// ==========================================================================
// Balance spells
// ==========================================================================

struct balance_spell_t : public druid_spell_t
{
  balance_spell_t( std::string_view name, druid_t* p, const spell_data_t* s, std::string_view options_str )
    : druid_spell_t( name, p, s )
  {
    parse_options( options_str );

    requires_caster_form = true;
  }

  // Cast speed (Nature's Grace). TODO: remove once spell_base_t::execute_time_pct_multiplier() is restored in the
  // engine; it applied composite_haste() to every spell cast until b617b5ff0f removed it with Chilled Clarity.
  double execute_time_pct_multiplier() const override
  { return druid_spell_t::execute_time_pct_multiplier() * composite_haste(); }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );

    // Nature's Grace: non-periodic spell crits
    if ( s->result == RESULT_CRIT && p()->talent.natures_grace.ok() )
      p()->buff.natures_grace->trigger();
  }
};

// Wrath: Eclipse gives charges for the next Starfires (2 per Wrath at any rank, up to 4)
struct wrath_t : public balance_spell_t
{
  wrath_t( druid_t* p, std::string_view options_str )
    : balance_spell_t( "wrath", p, p->find_spell( max_rank( p, wrath_ranks ).spell_id ), options_str )
  {}

  void execute() override
  {
    balance_spell_t::execute();

    if ( p()->talent.eclipse.ok() )
      p()->buff.eclipse->trigger( as<int>( p()->talent.eclipse->effectN( 3 ).base_value() ) );
  }
};

// Starfire: each Eclipse charge takes the talent's effect #2 (-0.17/-0.33/-0.5 s) off one cast
struct starfire_t : public balance_spell_t
{
  starfire_t( druid_t* p, std::string_view options_str )
    : balance_spell_t( "starfire", p, p->find_spell( max_rank( p, starfire_ranks ).spell_id ), options_str )
  {}

  timespan_t execute_time_flat_modifier() const override
  {
    timespan_t t = balance_spell_t::execute_time_flat_modifier();

    if ( p()->buff.eclipse->check() )
      t += timespan_t::from_millis( p()->talent.eclipse->effectN( 2 ).base_value() );

    return t;
  }

  void execute() override
  {
    balance_spell_t::execute();

    p()->buff.eclipse->decrement();
  }
};

// Moonfire: direct (effect #2) and a dot ticking every 3 s (effect #1)
struct moonfire_t : public balance_spell_t
{
  moonfire_t( druid_t* p, std::string_view options_str )
    : balance_spell_t( "moonfire", p, p->find_spell( max_rank( p, moonfire_ranks ).spell_id ), options_str )
  {
    dot_behavior = DOT_REFRESH_DURATION;
  }
};

// Insect Swarm (16 point Balance milestone). The -2% hit debuff on the target is not modelled.
struct insect_swarm_t : public balance_spell_t
{
  insect_swarm_t( druid_t* p, std::string_view options_str )
    : balance_spell_t( "insect_swarm", p,
                       p->talent.insect_swarm.ok() ? p->find_spell( max_rank( p, insect_swarm_ranks ).spell_id )
                                                   : spell_data_t::not_found(),
                       options_str )
  {
    dot_behavior = DOT_REFRESH_DURATION;
  }
};

// Faerie Fire: armor debuff. In a feral form it is free, has a 6 s cooldown and a 1.0 s global cooldown (Cat Form
// passive 3025 effects #3-#5, the merged Faerie Fire (Feral)).
struct faerie_fire_t : public druid_spell_t
{
  timespan_t caster_gcd;

  faerie_fire_t( druid_t* p, std::string_view options_str )
    : druid_spell_t( "faerie_fire", p, p->find_spell( max_rank( p, faerie_fire_ranks ).spell_id ) ),
      caster_gcd( trigger_gcd )
  {
    parse_options( options_str );

    // create the debuff on the default target up front so debuff.faerie_fire APL expressions resolve
    if ( data().ok() && target )
      td( target );
  }

  bool in_cat() const
  { return p()->buff.cat_form->check(); }

  double cost_pct_multiplier() const override
  {
    if ( in_cat() )
      return 1.0 + p()->spell.cat_form_passive->effectN( 3 ).percent();
    return druid_spell_t::cost_pct_multiplier();
  }

  void execute() override
  {
    if ( in_cat() )
    {
      cooldown->duration = timespan_t::from_millis( p()->spell.cat_form_passive->effectN( 4 ).base_value() );
      trigger_gcd = caster_gcd + timespan_t::from_millis( p()->spell.cat_form_passive->effectN( 5 ).base_value() );
    }
    else
    {
      cooldown->duration = data().cooldown();
      trigger_gcd        = caster_gcd;
    }

    druid_spell_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
      td( s->target )->debuff.faerie_fire->trigger();
  }

  // Record landed (zero damage) applications as well, so the report's miss% is of all casts
  void assess_damage( result_amount_type rt, action_state_t* s ) override
  {
    druid_spell_t::assess_damage( rt, s );

    if ( rt == result_amount_type::DMG_DIRECT && s->result_raw == 0 && result_is_hit( s->result ) )
      record_data( s );
  }
};

// ==========================================================================
// Utility and forms
// ==========================================================================

struct mark_of_the_wild_t : public druid_spell_t
{
  mark_of_the_wild_t( druid_t* p, std::string_view options_str )
    : druid_spell_t( "mark_of_the_wild", p, p->find_spell( max_rank( p, mark_of_the_wild_ranks ).spell_id ) )
  {
    parse_options( options_str );

    harmful  = false;
    may_miss = false;
    target   = p;
    requires_caster_form = true;
  }

  bool ready() override
  {
    if ( p()->buff.mark_of_the_wild->check() )
      return false;

    return druid_spell_t::ready();
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.mark_of_the_wild->trigger();
  }
};

// Innervate (self cast): spirit regen +400% (aura 110) and 100% of it kept while casting (aura 134) for 20 s
struct innervate_t : public druid_spell_t
{
  innervate_t( druid_t* p, std::string_view options_str )
    : druid_spell_t( "innervate", p, p->find_spell( INNERVATE ) )
  {
    parse_options( options_str );

    harmful  = false;
    may_miss = false;
    target   = p;
    requires_caster_form = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->buff.innervate->trigger();
  }
};

// Shapeshift: costs a % of base mana (spell data), 1.5 s global cooldown. Natural Shapeshifter applies through
// the passive talent parsing.
struct shapeshift_t : public druid_spell_t
{
  buff_t* druid_t::buffs_t::*form;

  shapeshift_t( druid_t* p, std::string_view name, const spell_data_t* s, buff_t* druid_t::buffs_t::*form,
                std::string_view options_str )
    : druid_spell_t( name, p, s ), form( form )
  {
    parse_options( options_str );

    harmful  = false;
    may_miss = false;
    target   = p;
  }

  bool ready() override
  {
    if ( ( p()->buff.*form )->check() )
      return false;

    return druid_spell_t::ready();
  }

  void execute() override
  {
    druid_spell_t::execute();

    p()->shapeshift( p()->buff.*form );
  }
};

// cancelform: back to caster form
struct cancelform_t : public action_t
{
  cancelform_t( druid_t* p, std::string_view options_str ) : action_t( ACTION_OTHER, "cancelform", p )
  {
    parse_options( options_str );
    trigger_gcd = 0_ms;
    harmful     = false;
    ignore_false_positive = true;
  }

  druid_t* p() const
  { return debug_cast<druid_t*>( player ); }

  bool ready() override
  {
    if ( !p()->buff.cat_form->check() && !p()->buff.moonkin_form->check() )
      return false;
    return action_t::ready();
  }

  void execute() override
  { p()->cancel_forms(); }
};

// ==========================================================================
// Melee
// ==========================================================================

struct druid_melee_t : public druid_attack_t
{
  druid_melee_t( druid_t* player )
    : druid_attack_t( "melee", player, spell_data_t::nil() )
  {
    weapon            = &player->equipped_weapons[ SLOT_MAIN_HAND ];
    weapon_multiplier = 1.0;
    base_execute_time = weapon->swing_time;
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

  // switch between the equipped weapon and the Cat Form weapon
  void set_form_weapon()
  {
    weapon            = p()->buff.cat_form->check() ? &p()->cat_weapon : &p()->equipped_weapons[ SLOT_MAIN_HAND ];
    base_execute_time = weapon->swing_time;
  }
};

struct druid_auto_attack_t : public druid_attack_t
{
  druid_auto_attack_t( druid_t* p, std::string_view options_str )
    : druid_attack_t( "auto_attack", p, spell_data_t::nil() )
  {
    parse_options( options_str );
    trigger_gcd           = 0_ms;
    ignore_false_positive = true;

    if ( !p->main_hand_attack )
      p->main_hand_attack = new druid_melee_t( p );
  }

  void execute() override
  {
    debug_cast<druid_melee_t*>( player->main_hand_attack )->set_form_weapon();
    player->main_hand_attack->schedule_execute();
  }

  bool ready() override
  {
    if ( player->is_moving() )
      return false;
    if ( player->equipped_weapons[ SLOT_MAIN_HAND ].type == WEAPON_NONE && !p()->buff.cat_form->check() )
      return false;
    return player->main_hand_attack->execute_event == nullptr;
  }
};

// ==========================================================================
// druid_t
// ==========================================================================

void druid_t::shapeshift( buff_t* form )
{
  cancel_forms();

  form->trigger();

  if ( main_hand_attack )
  {
    auto melee = debug_cast<druid_melee_t*>( main_hand_attack );
    melee->set_form_weapon();
    if ( melee->execute_event )
      melee->reschedule_execute( melee->execute_time() );
  }
}

void druid_t::cancel_forms()
{
  buff.cat_form->expire();
  buff.moonkin_form->expire();

  if ( main_hand_attack )
    debug_cast<druid_melee_t*>( main_hand_attack )->set_form_weapon();
}

// Omen of Clarity: white and yellow melee and hostile spells, with an internal cooldown (halved in Moonkin Form)
void druid_t::trigger_omen_of_clarity( action_state_t* s )
{
  if ( !spell.omen_of_clarity->ok() || !s->action->result_is_hit( s->result ) || omen_icd->down() )
    return;

  if ( s->result_type != result_amount_type::DMG_DIRECT && s->result_type != result_amount_type::NONE )
    return;

  double chance = options.omen_of_clarity_chance >= 0 ? options.omen_of_clarity_chance
                                                      : spell.omen_of_clarity->proc_chance();
  timespan_t icd = spell.omen_of_clarity->internal_cooldown();

  if ( buff.moonkin_form->check() )
  {
    const spell_data_t& moonkin = buff.moonkin_form->data();
    chance *= 1.0 + moonkin.effectN( 5 ).percent();
    icd *= 1.0 + moonkin.effectN( 6 ).percent();
  }

  if ( !rng().roll( std::min( 1.0, chance ) ) )
    return;

  omen_icd->start( icd );
  buff.clearcasting->trigger();
  proc.clearcasting->occur();
}

void druid_t::create_options()
{
  parse_player_effects_t::create_options();

  add_option( opt_float( "druid.rip_ap_per_cp", options.rip_ap_per_cp ) );
  add_option( opt_float( "druid.ferocious_bite_ap_per_cp", options.ferocious_bite_ap_per_cp ) );
  add_option( opt_float( "druid.omen_of_clarity_chance", options.omen_of_clarity_chance ) );
}

void druid_t::init_base_stats()
{
  // Druid melee attack power: 2 per strength - 20 (vanilla, foundation table); forms add their own
  base.attack_power_per_strength = 2.0;
  base.attack_power_per_agility  = 0.0;

  parse_player_effects_t::init_base_stats();

  base.stats.attack_power = -20.0;

  // ESTIMATE: vanilla base crit (wowsims/classic base_stats.go), druid 0.9% melee / 1.8% spell
  base.attack_crit_chance = get_passive_player_value( 0.009, "all_crit" );
  base.spell_crit_chance  = get_passive_player_value( 0.018, "all_crit" );
}

// Cat Form weapon: the equipped main hand's damage per second at a fixed 1.0 s swing (official, 2026-09-30).
// Without a weapon this is a 1.0 s swing with no weapon damage.
void druid_t::init_items()
{
  parse_player_effects_t::init_items();

  const weapon_t& mh = equipped_weapons[ SLOT_MAIN_HAND ];
  cat_weapon         = weapon_t( WEAPON_BEAST, 0.0, 1_s, SCHOOL_PHYSICAL );
  cat_weapon.slot    = SLOT_MAIN_HAND;
  if ( mh.type != WEAPON_NONE && mh.swing_time > 0_ms )
  {
    double speed       = mh.swing_time.total_seconds();
    cat_weapon.min_dmg = mh.min_dmg / speed;
    cat_weapon.max_dmg = mh.max_dmg / speed;
    cat_weapon.damage  = ( cat_weapon.min_dmg + cat_weapon.max_dmg ) / 2.0;
    cat_weapon.dps     = cat_weapon.damage;
  }
}

void druid_t::init_spells()
{
  parse_player_effects_t::init_spells();

  talent.insect_swarm       = forever::talent( this, "Insect Swarm" );
  talent.natures_reach      = forever::talent( this, "Nature's Reach" );
  talent.natures_grace      = forever::talent( this, "Nature's Grace" );
  talent.eclipse            = forever::talent( this, "Eclipse" );
  talent.moonkin_form       = forever::talent( this, "Moonkin Form" );

  talent.heart_of_the_wild  = forever::talent( this, "Heart of the Wild" );
  talent.predatory_strikes  = forever::talent( this, "Predatory Strikes" );
  talent.blood_frenzy       = forever::talent( this, "Blood Frenzy" );
  talent.shifting_power     = forever::talent( this, "Shifting Power" );
  talent.leader_of_the_pack = forever::talent( this, "Leader of the Pack" );
  talent.rend_and_tear      = forever::talent( this, "Rend and Tear" );
  talent.berserk            = forever::talent( this, "Berserk" );

  talent.furor              = forever::talent( this, "Furor" );

  spell.cat_form_passive    = find_spell( CAT_FORM_PASSIVE );
  spell.omen_of_clarity     = find_spell( OMEN_OF_CLARITY );
  spell.moonkin_aura        = find_spell( MOONKIN_AURA );
  spell.leader_of_the_pack  = find_spell( LEADER_OF_THE_PACK );

  parse_all_class_passives();
  parse_all_passive_talents();

  // Sharpened Claws (Modify Critical Strike%, aura 290) is form gated by its stance mask (Cat, Bear, Dire Bear),
  // which the passive parsing ignores. Every modelled feral rotation stays in Cat Form; a Balance build that
  // takes it would wrongly gain the crit. TODO: gate on the form.
}

void druid_t::init_gains()
{
  parse_player_effects_t::init_gains();

  gain.blood_frenzy   = get_gain( "blood_frenzy" );
  gain.shifting_power = get_gain( "shifting_power" );
}

void druid_t::init_procs()
{
  parse_player_effects_t::init_procs();

  proc.clearcasting = get_proc( "clearcasting" );
  proc.blood_frenzy = get_proc( "blood_frenzy" );
}

void druid_t::create_buffs()
{
  parse_player_effects_t::create_buffs();

  buff.cat_form = make_buff( this, "cat_form", find_spell( CAT_FORM ) )
                    ->add_invalidate( CACHE_ATTACK_POWER )
                    ->add_invalidate( CACHE_STRENGTH )
                    ->add_invalidate( CACHE_CRIT_CHANCE );

  buff.moonkin_form = make_buff( this, "moonkin_form", talent.moonkin_form.ok() ? talent.moonkin_form.spell()
                                                                                   : spell_data_t::not_found() )
                        ->add_invalidate( CACHE_SPELL_CRIT_CHANCE );

  buff.clearcasting = make_buff( this, "clearcasting", find_spell( CLEARCASTING ) );

  buff.natures_grace = make_buff( this, "natures_grace", find_spell( NATURES_GRACE_BUFF ) )
                         ->set_default_value_from_effect( 1 )
                         ->add_invalidate( CACHE_SPELL_HASTE );

  buff.eclipse = make_buff( this, "eclipse", talent.eclipse.ok() ? find_spell( ECLIPSE_BUFF )
                                                                  : spell_data_t::not_found() );

  buff.berserk = make_buff( this, "berserk", talent.berserk.ok() ? talent.berserk.spell()
                                                                  : spell_data_t::not_found() )
                   ->set_cooldown( 0_ms );

  buff.innervate = make_buff( this, "innervate", find_spell( INNERVATE ) )->set_cooldown( 0_ms );

  buff.mark_of_the_wild = make_buff<stat_buff_t>( this, "mark_of_the_wild",
                                                  find_spell( max_rank( this, mark_of_the_wild_ranks ).spell_id ) );

  omen_icd = get_cooldown( "omen_of_clarity_icd" );
}

action_t* druid_t::create_action( std::string_view name, std::string_view options_str )
{
  if ( name == "auto_attack" ) return new druid_auto_attack_t( this, options_str );

  // forms
  if ( name == "cat_form" )
    return new shapeshift_t( this, name, find_spell( CAT_FORM ), &buffs_t::cat_form, options_str );
  if ( name == "moonkin_form" )
    return new shapeshift_t( this, name, talent.moonkin_form.ok() ? talent.moonkin_form.spell()
                                                                  : spell_data_t::not_found(),
                             &buffs_t::moonkin_form, options_str );
  if ( name == "cancelform" ) return new cancelform_t( this, options_str );

  // cat
  if ( name == "claw" ) return new claw_t( this, options_str );
  if ( name == "shred" ) return new shred_t( this, options_str );
  if ( name == "rake" ) return new rake_t( this, options_str );
  if ( name == "rip" ) return new rip_t( this, options_str );
  if ( name == "ferocious_bite" ) return new ferocious_bite_t( this, options_str );
  if ( name == "shifting_power" ) return new shifting_power_t( this, options_str );
  if ( name == "berserk" ) return new berserk_t( this, options_str );

  // balance
  if ( name == "wrath" ) return new wrath_t( this, options_str );
  if ( name == "starfire" ) return new starfire_t( this, options_str );
  if ( name == "moonfire" ) return new moonfire_t( this, options_str );
  if ( name == "insect_swarm" ) return new insect_swarm_t( this, options_str );

  // utility
  if ( name == "faerie_fire" ) return new faerie_fire_t( this, options_str );
  if ( name == "mark_of_the_wild" ) return new mark_of_the_wild_t( this, options_str );
  if ( name == "innervate" ) return new innervate_t( this, options_str );

  return parse_player_effects_t::create_action( name, options_str );
}

void druid_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();
    druid_apl::druid( this );
    use_default_action_list = true;
  }

  // Energy is the primary resource (resource thresholds, report) for actors that shapeshift into Cat Form
  cat_dps = util::str_in_str_ci( action_list_str, "cat_form" );
  for ( const auto* apl : action_priority_list )
  {
    if ( util::str_in_str_ci( apl->action_list_str, "cat_form" ) )
      cat_dps = true;
    for ( const auto& a : apl->action_list )
      if ( util::str_in_str_ci( a.action_, "cat_form" ) )
        cat_dps = true;
  }

  parse_player_effects_t::init_action_list();
}

void druid_t::reset()
{
  parse_player_effects_t::reset();

  if ( main_hand_attack )
    debug_cast<druid_melee_t*>( main_hand_attack )->set_form_weapon();
}

// Forms: Cat Form passive 3025 adds 12 + 2 per level attack power (2 x level) plus agility (Cat Form tooltip);
// Predatory Strikes adds 50/100/150% of the druid's level in any feral form.
double druid_t::composite_melee_attack_power() const
{
  double ap = parse_player_effects_t::composite_melee_attack_power();

  if ( buff.cat_form->check() )
  {
    ap += spell.cat_form_passive->effectN( 1 ).average( this );
    ap += cache.agility();

    if ( talent.predatory_strikes.ok() )
      ap += std::floor( talent.predatory_strikes->effectN( 1 ).percent() * level() );
  }

  return ap;
}

// Heart of the Wild: Strength +2..10% in Cat Form (effect #3)
double druid_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = parse_player_effects_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STRENGTH && buff.cat_form->check() && talent.heart_of_the_wild.ok() )
    m *= 1.0 + talent.heart_of_the_wild->effectN( 3 ).percent();

  return m;
}

// Leader of the Pack: +3% melee crit in a feral form (raid aura 24932; only the druid is modelled)
double druid_t::composite_melee_crit_chance() const
{
  double c = parse_player_effects_t::composite_melee_crit_chance();

  if ( buff.cat_form->check() && talent.leader_of_the_pack.ok() )
    c += spell.leader_of_the_pack->effectN( 1 ).percent();

  return c;
}

// Moonkin Aura: +3% spell crit while in Moonkin Form (raid aura 24907; only the druid is modelled)
double druid_t::composite_spell_crit_chance() const
{
  double c = parse_player_effects_t::composite_spell_crit_chance();

  if ( buff.moonkin_form->check() )
    c += spell.moonkin_aura->effectN( 1 ).percent();

  return c;
}

// Nature's Grace: +10% casting speed for 3 s
double druid_t::composite_spell_haste() const
{
  double h = parse_player_effects_t::composite_spell_haste();

  if ( buff.natures_grace->check() )
    h /= 1.0 + buff.natures_grace->check_value();

  return h;
}

// Nature's Reach: Modify Hit% (aura 54, effect #2) and Modify Spell Hit% (aura 55, effect #3). The engine's
// passive parsing does not handle these player auras yet (proposed shared change in FOREVER_DRUID.md).
double druid_t::composite_melee_hit() const
{
  double h = parse_player_effects_t::composite_melee_hit();

  if ( talent.natures_reach.ok() )
    h += talent.natures_reach->effectN( 2 ).percent();

  return h;
}

double druid_t::composite_spell_hit() const
{
  double h = parse_player_effects_t::composite_spell_hit();

  if ( talent.natures_reach.ok() )
    h += talent.natures_reach->effectN( 3 ).percent();

  return h;
}

double druid_t::composite_player_target_armor( player_t* t ) const
{
  double a = parse_player_effects_t::composite_player_target_armor( t );

  if ( const druid_td_t* td = target_data[ t ] )
    if ( td->debuff.faerie_fire->check() )
      a = std::max( 0.0, a + td->debuff.faerie_fire->check_value() );

  return a;
}

// Innervate: aura 134 (100% of spirit regen while casting) and aura 110 (+400% spirit regen)
double druid_t::composite_mana_regen_while_casting() const
{
  double r = parse_player_effects_t::composite_mana_regen_while_casting();

  if ( buff.innervate->check() )
    r = std::max( r, buff.innervate->data().effectN( 1 ).percent() );

  return r;
}

double druid_t::composite_spirit_regen_multiplier() const
{
  double m = parse_player_effects_t::composite_spirit_regen_multiplier();

  if ( buff.innervate->check() )
    m *= 1.0 + buff.innervate->data().effectN( 2 ).percent();

  return m;
}

void druid_t::invalidate_cache( cache_e c )
{
  parse_player_effects_t::invalidate_cache( c );

  // Cat Form attack power includes agility
  if ( c == CACHE_AGILITY && buff.cat_form && buff.cat_form->check() )
    parse_player_effects_t::invalidate_cache( CACHE_ATTACK_POWER );
}

// DRUID MODULE INTERFACE ===================================================
struct druid_module_t final : public module_t
{
  druid_module_t() : module_t( DRUID ) {}

  player_t* create_player( sim_t* sim, std::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new druid_t( sim, name, r );
    return p;
  }

  bool valid() const override { return true; }

  void register_hotfixes() const override {}

  void register_actor_initializers( sim_t* ) const override {}
};
}  // UNNAMED NAMESPACE

const module_t* module_t::druid()
{
  static druid_module_t m;
  return &m;
}

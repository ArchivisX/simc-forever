// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "action/parse_effects.hpp"
#include "class_modules/apl/rogue.hpp"
#include "class_modules/forever_helpers.hpp"

namespace
{
// ==========================================================================
// Rogue
// ==========================================================================

using forever::max_rank;
using forever::spell_rank_t;

struct mutilate_rank_t
{
  unsigned spell_id;  // the cast (energy cost, combo points, poison bonus)
  unsigned mh_id;     // main hand hit
  unsigned oh_id;     // off hand hit
};

constexpr std::array sinister_strike_ranks {
  spell_rank_t{ 1752 },
  spell_rank_t{ 1757 },
  spell_rank_t{ 1758 },
  spell_rank_t{ 1759 },
  spell_rank_t{ 1760 },
  spell_rank_t{ 8621 },
  spell_rank_t{ 11293 },
  spell_rank_t{ 11294 },
};

constexpr std::array backstab_ranks {
  spell_rank_t{ 53 },
  spell_rank_t{ 2589 },
  spell_rank_t{ 2590 },
  spell_rank_t{ 2591 },
  spell_rank_t{ 8721 },
  spell_rank_t{ 11279 },
  spell_rank_t{ 11280 },
  spell_rank_t{ 11281 },
  spell_rank_t{ 25300 },
};

constexpr std::array ambush_ranks {
  spell_rank_t{ 8676 },
  spell_rank_t{ 8724 },
  spell_rank_t{ 8725 },
  spell_rank_t{ 11267 },
  spell_rank_t{ 11268 },
  spell_rank_t{ 11269 },
};

constexpr std::array garrote_ranks {
  spell_rank_t{ 703 },
  spell_rank_t{ 8631 },
  spell_rank_t{ 8632 },
  spell_rank_t{ 8633 },
  spell_rank_t{ 11289 },
  spell_rank_t{ 11290 },
};

constexpr std::array eviscerate_ranks {
  spell_rank_t{ 2098 },
  spell_rank_t{ 6760 },
  spell_rank_t{ 6761 },
  spell_rank_t{ 6762 },
  spell_rank_t{ 8623 },
  spell_rank_t{ 8624 },
  spell_rank_t{ 11299 },
  spell_rank_t{ 11300 },
  spell_rank_t{ 31016 },
};

constexpr std::array slice_and_dice_ranks {
  spell_rank_t{ 5171 },
  spell_rank_t{ 6774 },
};

constexpr std::array rupture_ranks {
  spell_rank_t{ 1943 },
  spell_rank_t{ 8639 },
  spell_rank_t{ 8640 },
  spell_rank_t{ 11273 },
  spell_rank_t{ 11274 },
  spell_rank_t{ 11275 },
};

constexpr std::array expose_armor_ranks {
  spell_rank_t{ 8647 },
  spell_rank_t{ 8649 },
  spell_rank_t{ 8650 },
  spell_rank_t{ 11197 },
  spell_rank_t{ 11198 },
};

constexpr std::array stealth_ranks {
  spell_rank_t{ 1784 },
  spell_rank_t{ 1785 },
  spell_rank_t{ 1786 },
  spell_rank_t{ 1787 },
};

// 2026-10-16: rank 1 is the 21 point talent; ranks 2-4 are trained (SupercedesSpell chain in skill line 253)
constexpr std::array mutilate_ranks {
  //                 cast       mh       oh
  mutilate_rank_t{ 1310707, 1310706, 1310705 },
  mutilate_rank_t{  399956,  399960,  399961 },
  mutilate_rank_t{ 1241582, 1241585, 1241588 },
  mutilate_rank_t{ 1241584, 1241586, 1241590 },
};

// Poison application spells (the temporary weapon enchant). Their proc chance is in the spell data (Improved
// Poisons is parsed into it), and effect #1 (type 360) names the item enchantment whose spell is the proc.
constexpr std::array instant_poison_ranks {
  spell_rank_t{  8679 },
  spell_rank_t{  8686 },
  spell_rank_t{  8688 },
  spell_rank_t{ 11338 },
  spell_rank_t{ 11339 },
  spell_rank_t{ 11340 },
};

constexpr std::array deadly_poison_ranks {
  spell_rank_t{  2823 },
  spell_rank_t{  2824 },
  spell_rank_t{ 11355 },
  spell_rank_t{ 11356 },
  spell_rank_t{ 25351 },
};

// Duration per combo point; only the 1 point base duration (6 s) is in the spell data, the per point part is
// in the tooltips (Slice and Dice / Venom 9-21 s, Rupture 8-16 s).
constexpr timespan_t slice_and_dice_per_cp = 3_s;
constexpr timespan_t venom_per_cp          = 3_s;
constexpr timespan_t rupture_per_cp        = 2_s;

// Premeditation's combo points are lost if not used within the buff duration (20 s)
constexpr double premeditation_cp = 2.0;

enum class poison_e
{
  NONE,
  INSTANT,
  DEADLY,
};

poison_e parse_poison( util::string_view s )
{
  if ( util::str_compare_ci( s, "instant" ) )
    return poison_e::INSTANT;
  if ( util::str_compare_ci( s, "deadly" ) )
    return poison_e::DEADLY;
  return poison_e::NONE;
}

struct rogue_t;
struct rogue_poison_t;

struct rogue_td_t : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* deadly_poison = nullptr;
    dot_t* garrote       = nullptr;
    dot_t* rupture       = nullptr;
  } dots;

  struct debuffs_t
  {
    buff_t* hemorrhage   = nullptr;
    buff_t* expose_armor = nullptr;
  } debuff;

  rogue_td_t( player_t* target, rogue_t* p );

  bool poisoned() const
  { return dots.deadly_poison->is_ticking(); }
};

struct rogue_t final : public player_t
{
  rogue_t( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) :
    player_t( sim, ROGUE, name, r ) {}

  resource_e primary_resource() const override { return RESOURCE_ENERGY; }
  role_e primary_role() const override { return ROLE_ATTACK; }

  void init_base_stats() override;
  void init_spells() override;
  void init_gains() override;
  void init_procs() override;
  void create_buffs() override;
  void create_actions() override;
  void reset() override;

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
  void init_action_list() override;
  std::unique_ptr<expr_t> create_expression( util::string_view name ) override;

  std::string default_potion() const override   { return rogue_apl::potion( this ); }
  std::string default_flask() const override    { return rogue_apl::flask( this ); }
  std::string default_food() const override     { return rogue_apl::food( this ); }

  double resource_regen_per_second( resource_e ) const override;
  double composite_melee_auto_attack_speed() const override;
  double composite_melee_hit() const override;
  double composite_spell_hit() const override;
  double composite_player_target_multiplier( player_t*, school_e ) const override;
  double composite_player_target_armor( player_t* ) const override;

  rogue_td_t* get_target_data( player_t* target ) const override
  {
    rogue_td_t*& td = target_data[ target ];
    if ( !td )
      td = new rogue_td_t( target, const_cast<rogue_t*>( this ) );
    return td;
  }

  // Talent abilities: the talent teaches rank 1, trained ranks are only usable with the talent
  template <typename RANKS>
  const spell_data_t* talent_rank_spell( const player_talent_t& t, const RANKS& ranks ) const
  { return t.ok() ? find_spell( max_rank( this, ranks ).spell_id ) : spell_data_t::not_found(); }

  void trigger_combo_point_gain( double amount, gain_t* gain, action_t* action = nullptr );
  void trigger_seal_fate( const action_state_t* s );
  void trigger_poisons( const action_state_t* s );
  void trigger_hack_and_slash( const action_state_t* s );
  void trigger_blade_flurry( const action_state_t* s );

  struct talents_t
  {
    // Assassination
    player_talent_t malice;
    player_talent_t ruthlessness;
    player_talent_t murder;
    player_talent_t improved_slice_and_dice;
    player_talent_t relentless_strikes;
    player_talent_t improved_expose_armor;
    player_talent_t lethality;
    player_talent_t vile_poisons;
    player_talent_t cold_blood;
    player_talent_t improved_poisons;
    player_talent_t vigor;
    player_talent_t mutilate;
    player_talent_t seal_fate;
    player_talent_t venom;

    // Combat
    player_talent_t improved_eviscerate;
    player_talent_t improved_sinister_strike;
    player_talent_t puncturing_wounds;
    player_talent_t precision;
    player_talent_t flawless_execution;
    player_talent_t dual_wield_specialization;
    player_talent_t blade_flurry;
    player_talent_t hack_and_slash;
    player_talent_t weapon_expertise;
    player_talent_t aggression;
    player_talent_t adrenaline_rush;

    // Subtlety
    player_talent_t opportunity;
    player_talent_t improved_ambush;
    player_talent_t initiative;
    player_talent_t ghostly_strike;
    player_talent_t premeditation;
    player_talent_t serrated_blades;
    player_talent_t dirty_deeds;
    player_talent_t preparation;
    player_talent_t hemorrhage;
    player_talent_t quietus;
    player_talent_t cutthroat;
    player_talent_t thousand_cuts;
  } talent;

  struct spells_t
  {
    const spell_data_t* coup_de_grace            = spell_data_t::not_found();
    const spell_data_t* relentless_strikes_energy = spell_data_t::not_found();
    const spell_data_t* cutthroat_buff            = spell_data_t::not_found();
    const spell_data_t* thousand_cuts_buff        = spell_data_t::not_found();
    const spell_data_t* hack_and_slash_dagger     = spell_data_t::not_found();
    const spell_data_t* hack_and_slash_mace       = spell_data_t::not_found();
  } spell;

  struct buffs_t
  {
    buff_t* stealth         = nullptr;
    buff_t* slice_and_dice  = nullptr;
    buff_t* blade_flurry    = nullptr;
    buff_t* adrenaline_rush = nullptr;
    buff_t* cold_blood      = nullptr;
    buff_t* venom           = nullptr;
    buff_t* premeditation   = nullptr;
    buff_t* cutthroat       = nullptr;
    buff_t* thousand_cuts   = nullptr;
  } buff;

  struct gains_t
  {
    gain_t* relentless_strikes    = nullptr;
    gain_t* ruthlessness          = nullptr;
    gain_t* seal_fate             = nullptr;
    gain_t* puncturing_wounds     = nullptr;
    gain_t* initiative            = nullptr;
    gain_t* premeditation         = nullptr;
    gain_t* improved_expose_armor = nullptr;
  } gain;

  struct procs_t
  {
    proc_t* seal_fate      = nullptr;
    proc_t* hack_and_slash = nullptr;
    proc_t* cutthroat      = nullptr;
    proc_t* instant_poison = nullptr;
    proc_t* deadly_poison  = nullptr;
  } proc;

  struct cooldowns_t
  {
    cooldown_t* seal_fate_icd      = nullptr;
    cooldown_t* hack_and_slash_icd = nullptr;
  } cooldowns;

  // Weapon poisons, set by the apply_poison action
  poison_e mh_poison = poison_e::NONE;
  poison_e oh_poison = poison_e::NONE;

  rogue_poison_t* instant_poison = nullptr;
  rogue_poison_t* deadly_poison  = nullptr;
  action_t* hack_and_slash_swing = nullptr;
  action_t* blade_flurry_attack  = nullptr;

  mutable target_specific_t<rogue_td_t> target_data;
};

rogue_td_t::rogue_td_t( player_t* target, rogue_t* p ) : actor_target_data_t( target, p )
{
  dots.deadly_poison = target->get_dot( "deadly_poison", p );
  dots.garrote       = target->get_dot( "garrote", p );
  dots.rupture       = target->get_dot( "rupture", p );

  debuff.hemorrhage = make_buff( *this, "hemorrhage", p->talent.hemorrhage.spell() );

  // 2026-10-16: aura 674 (unknown to simc), flat armor per combo point; the value is set per cast
  debuff.expose_armor = make_buff( *this, "expose_armor", p->find_spell( max_rank( p, expose_armor_ranks ).spell_id ) )
                          ->set_max_stack( 1 );
}

// ==========================================================================
// Rogue Action Base
// ==========================================================================

// Combo points spent by a finisher, snapshot with the cast so a running bleed keeps the value it was cast with
struct rogue_action_state_t : public action_state_t
{
  int cp = 0;

  using action_state_t::action_state_t;

  void initialize() override
  {
    action_state_t::initialize();
    cp = 0;
  }

  void copy_state( const action_state_t* s ) override
  {
    action_state_t::copy_state( s );
    cp = debug_cast<const rogue_action_state_t*>( s )->cp;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s ) << " cp=" << cp;
    return s;
  }
};

template <class Base>
struct rogue_action_t : public parse_action_effects_t<Base>
{
private:
  using ab = parse_action_effects_t<Base>;

public:
  using base_t = rogue_action_t<Base>;

  bool requires_stealth       = false;  // openers: Stealth (or Cutthroat for Ambush)
  bool affected_by_cold_blood = false;
  bool seal_fate_source       = false;  // a combo point builder (or a Mutilate hand)
  bool consumes_thousand_cuts = false;

  template <typename... Args>
  rogue_action_t( Args&&... args ) : ab( std::forward<Args>( args )... )
  {
    // 1 s GCD for every rogue ability, not affected by haste
    this->gcd_type = gcd_haste_type::NONE;

    // Rogue spell data carries SP coefficients of 1 on weapon and finisher effects; rogue damage does
    // not scale with spell power.
    this->spell_power_mod.direct = 0;
    this->spell_power_mod.tick   = 0;

    seal_fate_source = this->energize_resource == RESOURCE_COMBO_POINT && this->energize_type != action_energize::NONE &&
                       this->energize_amount > 0;

    if ( p()->talent.cold_blood.ok() )
    {
      affected_by_cold_blood = this->data().affected_by( p()->talent.cold_blood->effectN( 1 ) );
      this->parse_effects( p()->buff.cold_blood );
    }
    if ( p()->talent.venom.ok() )
      this->parse_effects( p()->buff.venom );
    if ( p()->talent.thousand_cuts.ok() )
    {
      consumes_thousand_cuts = this->data().affected_by( p()->spell.thousand_cuts_buff->effectN( 1 ) );
      this->parse_effects( p()->buff.thousand_cuts );
    }
    if ( p()->talent.hemorrhage.ok() )
      this->parse_target_effects( []( actor_target_data_t* td ) {
        return static_cast<rogue_td_t*>( td )->debuff.hemorrhage->check();
      }, p()->talent.hemorrhage.spell() );
  }

  rogue_t* p()
  { return debug_cast<rogue_t*>( this->player ); }

  const rogue_t* p() const
  { return debug_cast<const rogue_t*>( this->player ); }

  rogue_td_t* td( player_t* t ) const
  { return p()->get_target_data( t ); }

  static const rogue_action_state_t* cast_state( const action_state_t* s )
  { return debug_cast<const rogue_action_state_t*>( s ); }

  static rogue_action_state_t* cast_state( action_state_t* s )
  { return debug_cast<rogue_action_state_t*>( s ); }

  action_state_t* new_state() override
  { return new rogue_action_state_t( this, this->target ); }

  // Finishers spend every combo point the rogue has
  virtual bool consumes_combo_points() const
  { return false; }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    cast_state( s )->cp = consumes_combo_points() ? as<int>( p()->resources.current[ RESOURCE_COMBO_POINT ] ) : 0;

    ab::snapshot_state( s, rt );
  }

  bool ready() override
  {
    if ( requires_stealth && !p()->buff.stealth->check() )
      return false;

    return ab::ready();
  }

  // Abilities with "Discount Power On Miss" only cost 20% of their energy on a miss, dodge or parry
  // (vanilla rule, same as the warrior module's rage handling)
  void consume_resource() override
  {
    resource_e cr = this->current_resource();

    if ( cr == RESOURCE_ENERGY && this->cost() > 0 && this->data().flags( spell_attribute::SX_DISCOUNT_ON_MISS ) &&
         !this->proc && this->execute_state && !this->result_is_hit( this->execute_state->result ) )
    {
      this->last_resource_cost = 0.2 * this->cost();

      this->player->resource_loss( cr, this->last_resource_cost, nullptr, this );
      this->stats->consume_resource( cr, this->last_resource_cost );
    }
    else
    {
      ab::consume_resource();
    }
  }

  void execute() override
  {
    ab::execute();

    // Any harmful action breaks Stealth
    if ( this->harmful && !this->background )
      p()->buff.stealth->expire();

    if ( affected_by_cold_blood && !this->background )
      p()->buff.cold_blood->expire();

    if ( consumes_thousand_cuts )
      p()->buff.thousand_cuts->expire();
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( seal_fate_source && s->result == RESULT_CRIT )
      p()->trigger_seal_fate( s );
  }

  // Hack and Slash (dagger/fist branch): crit with daggers and fist weapons
  double composite_crit_chance() const override
  {
    double c = ab::composite_crit_chance();

    if ( this->weapon && p()->talent.hack_and_slash.ok() &&
         ( this->weapon->type == WEAPON_DAGGER || this->weapon->type == WEAPON_FIST ) )
      c += p()->talent.hack_and_slash->effectN( 1 ).percent();

    return c;
  }

  // Hack and Slash (mace branch): attacks with maces ignore part of the target's armor
  double composite_target_armor( const action_state_t* s ) const override
  {
    double a = ab::composite_target_armor( s );

    if ( this->weapon && this->weapon->type == WEAPON_MACE && p()->talent.hack_and_slash.ok() )
      a *= 1.0 - p()->talent.hack_and_slash->effectN( 2 ).percent();

    return a;
  }
};

using rogue_attack_t = rogue_action_t<melee_attack_t>;
using rogue_spell_t  = rogue_action_t<spell_t>;

// Weapon strikes: on a landed hit, the hand's poison, Hack and Slash and Blade Flurry
struct rogue_strike_t : public rogue_attack_t
{
  rogue_strike_t( util::string_view name, rogue_t* p, const spell_data_t* s ) : rogue_attack_t( name, p, s )
  {
    // classic: all instant weapon strikes use normalized weapon speed
    normalize_weapon_speed = true;
  }

  void impact( action_state_t* s ) override
  {
    rogue_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      p()->trigger_poisons( s );
      p()->trigger_hack_and_slash( s );
      p()->trigger_blade_flurry( s );
    }
  }
};

// ==========================================================================
// Melee
// ==========================================================================

struct rogue_melee_t : public rogue_attack_t
{
  rogue_melee_t( util::string_view name, rogue_t* player, weapon_t* w, bool repeat = true )
    : rogue_attack_t( name, player, spell_data_t::nil() )
  {
    weapon            = w;
    weapon_multiplier = 1.0;
    base_execute_time = w->swing_time;
    school            = SCHOOL_PHYSICAL;
    trigger_gcd       = 0_ms;

    background        = true;
    repeating         = repeat;
    may_glance        = true;
    may_crit          = true;
    may_dodge         = true;
    may_parry         = true;
    special           = false;
  }

  // Dual Wield Specialization: off hand damage (aura 122, unknown to the passive parser)
  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = rogue_attack_t::composite_da_multiplier( s );

    if ( weapon->slot == SLOT_OFF_HAND && p()->talent.dual_wield_specialization.ok() )
      m *= 1.0 + p()->talent.dual_wield_specialization->effectN( 1 ).percent();

    return m;
  }

  void impact( action_state_t* s ) override
  {
    rogue_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      p()->trigger_poisons( s );
      p()->trigger_hack_and_slash( s );
      p()->trigger_blade_flurry( s );
    }
  }
};

struct rogue_auto_attack_t : public rogue_attack_t
{
  rogue_auto_attack_t( rogue_t* p, util::string_view options_str )
    : rogue_attack_t( "auto_attack", p, spell_data_t::nil() )
  {
    parse_options( options_str );
    trigger_gcd           = 0_ms;
    ignore_false_positive = true;
    harmful               = false;

    assert( p->equipped_weapons[ SLOT_MAIN_HAND ].type != WEAPON_NONE );

    p->main_hand_attack = new rogue_melee_t( "melee_main_hand", p, &p->equipped_weapons[ SLOT_MAIN_HAND ] );

    if ( p->equipped_weapons[ SLOT_OFF_HAND ].type != WEAPON_NONE )
    {
      p->off_hand_attack = new rogue_melee_t( "melee_off_hand", p, &p->equipped_weapons[ SLOT_OFF_HAND ] );
      p->off_hand_attack->id = 1;
    }
  }

  void execute() override
  {
    player->main_hand_attack->schedule_execute();
    if ( player->off_hand_attack )
      player->off_hand_attack->schedule_execute();
  }

  bool ready() override
  {
    // white swings would break Stealth before the opener
    if ( p()->buff.stealth->check() || player->is_moving() )
      return false;

    return player->main_hand_attack->execute_event == nullptr;
  }
};

// Hack and Slash (sword/axe branch) extra attack: an immediate main hand white swing
struct hack_and_slash_swing_t : public rogue_melee_t
{
  hack_and_slash_swing_t( rogue_t* p ) :
    rogue_melee_t( "hack_and_slash", p, &p->equipped_weapons[ SLOT_MAIN_HAND ], false )
  {
    base_execute_time = 0_ms;
  }
};

// Blade Flurry: the damage of a melee hit is copied to one other nearby target. Assumed to copy the final
// damage (vanilla behaviour), so it ignores armor and can't miss or crit on its own.
struct blade_flurry_attack_t : public rogue_attack_t
{
  blade_flurry_attack_t( rogue_t* p ) : rogue_attack_t( "blade_flurry_attack", p, spell_data_t::nil() )
  {
    background = proc = true;
    may_miss = may_dodge = may_parry = may_block = may_crit = may_glance = false;
    school        = SCHOOL_PHYSICAL;
    ignores_armor = true;
    trigger_gcd   = 0_ms;
    base_dd_min = base_dd_max = 0;
  }

  // the copied amount already includes all multipliers
  double composite_da_multiplier( const action_state_t* ) const override
  { return 1.0; }

  double composite_target_multiplier( player_t* ) const override
  { return 1.0; }

};

// ==========================================================================
// Poisons
// ==========================================================================

// The proc of a poison application spell: its effect #1 (type 360) names the item enchantment, whose spell is the proc
const spell_data_t* poison_proc_spell( const rogue_t* p, const spell_data_t* application )
{
  if ( !application->ok() )
    return spell_data_t::not_found();

  const auto& enchant = p->dbc->item_enchantment( as<unsigned>( application->effectN( 1 ).misc_value1() ) );
  return p->find_spell( enchant.ench_prop[ 0 ] );
}

// A weapon poison's proc; the application spell holds the proc chance per landed hit
struct rogue_poison_t : public rogue_spell_t
{
  const spell_data_t* application;

  rogue_poison_t( util::string_view name, rogue_t* p, const spell_data_t* application ) :
    rogue_spell_t( name, p, poison_proc_spell( p, application ) ), application( application )
  {
    background = proc = true;
    callbacks  = false;
  }
};

struct instant_poison_t : public rogue_poison_t
{
  instant_poison_t( rogue_t* p ) :
    rogue_poison_t( "instant_poison", p, p->find_spell( max_rank( p, instant_poison_ranks ).spell_id ) )
  {}
};

// Deadly Poison: Nature DoT, 12 s, ticks every 3 s, stacks to 5 (each application adds a stack and
// refreshes the duration)
struct deadly_poison_t : public rogue_poison_t
{
  deadly_poison_t( rogue_t* p ) :
    rogue_poison_t( "deadly_poison", p, p->find_spell( max_rank( p, deadly_poison_ranks ).spell_id ) )
  {
    dot_behavior = DOT_REFRESH_DURATION;
  }
};

// Precombat: apply_poison,mh=instant|deadly,oh=instant|deadly
struct apply_poison_t : public action_t
{
  poison_e mh = poison_e::NONE, oh = poison_e::NONE;
  bool executed = false;

  apply_poison_t( rogue_t* p, util::string_view options_str ) : action_t( ACTION_OTHER, "apply_poison", p )
  {
    std::string mh_str, oh_str;
    add_option( opt_string( "mh", mh_str ) );
    add_option( opt_string( "oh", oh_str ) );
    parse_options( options_str );

    mh = parse_poison( mh_str );
    oh = parse_poison( oh_str );

    if ( !mh_str.empty() && mh == poison_e::NONE )
      sim->error( "{} apply_poison: unknown main hand poison '{}' (instant, deadly)", *p, mh_str );
    if ( !oh_str.empty() && oh == poison_e::NONE )
      sim->error( "{} apply_poison: unknown off hand poison '{}' (instant, deadly)", *p, oh_str );

    trigger_gcd = 0_ms;
    harmful     = false;
    ignore_false_positive = true;
  }

  rogue_t* p() const
  { return debug_cast<rogue_t*>( player ); }

  void execute() override
  {
    auto known = [ this ]( poison_e poison ) {
      if ( poison == poison_e::INSTANT && !p()->instant_poison->data().ok() )
        return poison_e::NONE;
      if ( poison == poison_e::DEADLY && !p()->deadly_poison->data().ok() )
        return poison_e::NONE;
      return poison;
    };

    p()->mh_poison = known( mh );
    if ( player->equipped_weapons[ SLOT_OFF_HAND ].type != WEAPON_NONE )
      p()->oh_poison = known( oh );

    executed = true;
  }

  bool ready() override
  { return !executed && action_t::ready(); }

  void reset() override
  {
    action_t::reset();
    executed = false;
  }
};

// ==========================================================================
// Combo Point Builders
// ==========================================================================

struct sinister_strike_t : public rogue_strike_t
{
  sinister_strike_t( rogue_t* p, util::string_view options_str ) :
    rogue_strike_t( "sinister_strike", p, p->find_spell( max_rank( p, sinister_strike_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = rogue_strike_t::composite_da_multiplier( s );

    if ( p()->talent.quietus.ok() && s->target->health_percentage() < p()->talent.quietus->effectN( 2 ).base_value() )
      m *= 1.0 + p()->talent.quietus->effectN( 1 ).percent();

    return m;
  }
};

// Daggers in the main hand, and behind the target
struct backstab_t : public rogue_strike_t
{
  backstab_t( rogue_t* p, util::string_view options_str ) :
    rogue_strike_t( "backstab", p, p->find_spell( max_rank( p, backstab_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }

  bool ready() override
  {
    if ( weapon == nullptr || weapon->type != WEAPON_DAGGER )
      return false;

    if ( player->position() != POSITION_BACK && player->position() != POSITION_RANGED_BACK )
      return false;

    return rogue_strike_t::ready();
  }

  void impact( action_state_t* s ) override
  {
    rogue_strike_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    // Puncturing Wounds: chance for an extra combo point
    if ( p()->talent.puncturing_wounds.ok() && rng().roll( p()->talent.puncturing_wounds->effectN( 2 ).percent() ) )
      p()->trigger_combo_point_gain( 1, p()->gain.puncturing_wounds, this );

    // Cutthroat: chance that the next Ambush within 10 s doesn't need Stealth
    if ( p()->talent.cutthroat.ok() && rng().roll( p()->talent.cutthroat->effectN( 1 ).percent() ) )
    {
      p()->buff.cutthroat->trigger();
      p()->proc.cutthroat->occur();
    }
  }
};

// Stealth openers: Initiative adds a combo point
template <class Base>
struct rogue_opener_t : public Base
{
  template <typename... Args>
  rogue_opener_t( Args&&... args ) : Base( std::forward<Args>( args )... )
  {
    this->requires_stealth = true;
  }

  void impact( action_state_t* s ) override
  {
    Base::impact( s );

    if ( this->result_is_hit( s->result ) && this->p()->talent.initiative.ok() &&
         this->rng().roll( this->p()->talent.initiative->effectN( 1 ).percent() ) )
      this->p()->trigger_combo_point_gain( 1, this->p()->gain.initiative, this );
  }
};

// Dagger in the main hand, behind the target, and Stealth (or a Cutthroat proc)
struct ambush_t : public rogue_opener_t<rogue_strike_t>
{
  ambush_t( rogue_t* p, util::string_view options_str ) :
    rogue_opener_t<rogue_strike_t>( "ambush", p, p->find_spell( max_rank( p, ambush_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }

  bool ready() override
  {
    if ( weapon == nullptr || weapon->type != WEAPON_DAGGER )
      return false;

    if ( player->position() != POSITION_BACK && player->position() != POSITION_RANGED_BACK )
      return false;

    // Cutthroat lets Ambush skip the Stealth requirement
    requires_stealth = !p()->buff.cutthroat->check();

    return rogue_opener_t<rogue_strike_t>::ready();
  }

  void execute() override
  {
    bool cutthroat = !p()->buff.stealth->check() && p()->buff.cutthroat->check();

    rogue_opener_t<rogue_strike_t>::execute();

    if ( cutthroat )
      p()->buff.cutthroat->decrement();
  }
};

// Bleed opener (also usable from the front with Dirty Deeds; positioning is not checked)
struct garrote_t : public rogue_opener_t<rogue_attack_t>
{
  garrote_t( rogue_t* p, util::string_view options_str ) :
    rogue_opener_t<rogue_attack_t>( "garrote", p, p->find_spell( max_rank( p, garrote_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }
};

// 125% weapon damage, 180% with a dagger in the main hand (Dummy effect #4)
struct ghostly_strike_t : public rogue_strike_t
{
  ghostly_strike_t( rogue_t* p, util::string_view options_str ) :
    rogue_strike_t( "ghostly_strike", p, p->talent.ghostly_strike.ok() ? p->talent.ghostly_strike.spell() : spell_data_t::not_found() )
  {
    parse_options( options_str );

    if ( data().ok() && weapon && weapon->type == WEAPON_DAGGER )
      weapon_multiplier = data().effectN( 4 ).percent();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = rogue_strike_t::composite_da_multiplier( s );

    if ( p()->talent.quietus.ok() && s->target->health_percentage() < p()->talent.quietus->effectN( 2 ).base_value() )
      m *= 1.0 + p()->talent.quietus->effectN( 1 ).percent();

    return m;
  }
};

// Forever Hemorrhage: 100% normalized weapon damage, 145% with a dagger (Dummy effect #5), and a 15 s debuff
// that increases the rogue's Rupture damage (effect #3, aura 271, parsed as a target effect)
struct hemorrhage_t : public rogue_strike_t
{
  hemorrhage_t( rogue_t* p, util::string_view options_str ) :
    rogue_strike_t( "hemorrhage", p, p->talent.hemorrhage.ok() ? p->talent.hemorrhage.spell() : spell_data_t::not_found() )
  {
    parse_options( options_str );

    if ( data().ok() && weapon && weapon->type == WEAPON_DAGGER )
      weapon_multiplier = data().effectN( 5 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    rogue_strike_t::impact( s );

    if ( result_is_hit( s->result ) )
      td( s->target )->debuff.hemorrhage->trigger();
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = rogue_strike_t::composite_da_multiplier( s );

    if ( p()->talent.quietus.ok() && s->target->health_percentage() < p()->talent.quietus->effectN( 2 ).base_value() )
      m *= 1.0 + p()->talent.quietus->effectN( 1 ).percent();

    return m;
  }

  // Gated by the talent node; the trained ranks share the talent spell
  bool verify_actor_level() const override
  { return true; }
};

// Mutilate (Forever, 21 point Assassination talent): the cast rolls the attack table and awards 2 combo
// points; on a hit both weapons strike (the hand spells can't miss, be dodged or parried) for
// 75% x (normalized weapon damage + bp) each, +20% vs Poisoned targets (Dummy effect #4).
// Each hand crits and procs on its own; Seal Fate can trigger once per cast (its 0.5 s ICD).
struct mutilate_t : public rogue_attack_t
{
  struct mutilate_hit_t : public rogue_strike_t
  {
    double poisoned_bonus = 0;

    mutilate_hit_t( util::string_view name, rogue_t* p, const spell_data_t* s, double bonus ) :
      rogue_strike_t( name, p, s ), poisoned_bonus( bonus )
    {
      background       = true;
      seal_fate_source = true;

      // No Attack Miss/Dodge/Parry: the cast already rolled the attack table
      may_miss = may_dodge = may_parry = may_block = false;
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = rogue_strike_t::composite_da_multiplier( s );

      if ( td( s->target )->poisoned() )
        m *= 1.0 + poisoned_bonus;

      // ESTIMATE (2026-10-16): the off hand hit is assumed to have no off hand penalty (both hand spells are
      // 75% weapon damage). The engine halves every off hand weapon attack, so undo that here.
      if ( weapon && weapon->slot == SLOT_OFF_HAND )
        m *= 2.0;

      return m;
    }
  };

  mutilate_hit_t* mh_hit = nullptr;
  mutilate_hit_t* oh_hit = nullptr;

  mutilate_t( rogue_t* p, util::string_view options_str ) :
    rogue_attack_t( "mutilate", p, p->talent_rank_spell( p->talent.mutilate, mutilate_ranks ) )
  {
    parse_options( options_str );

    may_crit = false;

    if ( !data().ok() )
      return;

    auto rank = max_rank( p, mutilate_ranks );
    double bonus = data().effectN( 4 ).percent();

    mh_hit = new mutilate_hit_t( "mutilate_mh", p, p->find_spell( rank.mh_id ), bonus );
    oh_hit = new mutilate_hit_t( "mutilate_oh", p, p->find_spell( rank.oh_id ), bonus );
    add_child( mh_hit );
    add_child( oh_hit );
  }

  bool ready() override
  {
    if ( !player->dual_wield() || player->equipped_weapons[ SLOT_MAIN_HAND ].type != WEAPON_DAGGER ||
         player->equipped_weapons[ SLOT_OFF_HAND ].type != WEAPON_DAGGER )
      return false;

    return rogue_attack_t::ready();
  }

  void impact( action_state_t* s ) override
  {
    rogue_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      mh_hit->execute_on_target( s->target );
      oh_hit->execute_on_target( s->target );
    }
  }

  void execute() override
  {
    rogue_attack_t::execute();

    // Cold Blood covers both hands of the same cast
    p()->buff.cold_blood->expire();
  }
};

// ==========================================================================
// Finishers
// ==========================================================================

// All finishers spend every combo point (player resource, max 5) on a successful cast; a missed finisher keeps
// them. Relentless Strikes, Ruthlessness and Premeditation hook in here.
template <class Base>
struct rogue_finisher_t : public Base
{
  template <typename... Args>
  rogue_finisher_t( Args&&... args ) : Base( std::forward<Args>( args )... ) {}

  bool consumes_combo_points() const override
  { return true; }

  bool ready() override
  {
    if ( this->p()->resources.current[ RESOURCE_COMBO_POINT ] < 1 )
      return false;

    return Base::ready();
  }

  void execute() override
  {
    Base::execute();

    if ( this->result_is_hit( this->execute_state->result ) )
      spend_combo_points( this->execute_state );
  }

  virtual void spend_combo_points( const action_state_t* s )
  {
    rogue_t* p = this->p();
    double cp  = this->cast_state( s )->cp;

    p->resource_loss( RESOURCE_COMBO_POINT, cp, nullptr, this );
    this->stats->consume_resource( RESOURCE_COMBO_POINT, cp );

    p->buff.premeditation->expire();

    // Relentless Strikes: 20% per combo point to restore 25 energy
    if ( p->talent.relentless_strikes.ok() )
    {
      const auto& eff = p->talent.relentless_strikes->effectN( 1 );
      double per_cp   = std::max( eff.base_value(), eff.pp_combo_points() ) / 100.0;
      if ( this->rng().roll( per_cp * cp ) )
        p->resource_gain( RESOURCE_ENERGY, p->spell.relentless_strikes_energy->effectN( 1 ).base_value(),
                          p->gain.relentless_strikes, this );
    }

    // Ruthlessness: chance for a combo point after the finisher
    if ( p->talent.ruthlessness.ok() && this->rng().roll( p->talent.ruthlessness->effectN( 1 ).percent() ) )
      p->trigger_combo_point_gain( 1, p->gain.ruthlessness, this );
  }
};

// Eviscerate: bp +- bp * Variance / 2 (the data min/max) plus a flat amount per combo point; no attack power
// coefficient in the data (vanilla 1.12). Coup de Grace (Forever, level 30 Assassination passive) adds 2% per
// Deadly Poison application on the target (implemented from the tooltip; its aura in the data points at the
// Deadly Poison spells).
struct eviscerate_t : public rogue_finisher_t<rogue_attack_t>
{
  double per_cp;

  eviscerate_t( rogue_t* p, util::string_view options_str ) :
    rogue_finisher_t<rogue_attack_t>( "eviscerate", p, p->find_spell( max_rank( p, eviscerate_ranks ).spell_id ) ),
    per_cp( data().effectN( 1 ).pp_combo_points() )
  {
    parse_options( options_str );

    // the range is applied to the base points only (base_dd_min/max), not to the combo point part
    amount_delta = 0;
  }

  double base_da_min( const action_state_t* s ) const override
  { return rogue_finisher_t::base_da_min( s ) + per_cp * cast_state( s )->cp; }

  double base_da_max( const action_state_t* s ) const override
  { return rogue_finisher_t::base_da_max( s ) + per_cp * cast_state( s )->cp; }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = rogue_finisher_t::composite_da_multiplier( s );

    if ( p()->spell.coup_de_grace->ok() )
      m *= 1.0 + p()->spell.coup_de_grace->effectN( 1 ).percent() * td( s->target )->dots.deadly_poison->current_stack();

    return m;
  }
};

// Rupture: bleed every 2 s for 6 s + 2 s per combo point, tick = bp + pp_cp x combo points.
// 2026-10-16: the tooltip says "increased by your Attack Power" but the data has no coefficient; the data alone
// gives less than vanilla (validation hook V-R5), no attack power is added here.
struct rupture_t : public rogue_finisher_t<rogue_attack_t>
{
  double per_cp;

  rupture_t( rogue_t* p, util::string_view options_str ) :
    rogue_finisher_t<rogue_attack_t>( "rupture", p, p->find_spell( max_rank( p, rupture_ranks ).spell_id ) ),
    per_cp( data().effectN( 1 ).pp_combo_points() )
  {
    parse_options( options_str );

    dot_behavior = DOT_CLIP;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return rogue_finisher_t::composite_dot_duration( s ) + rupture_per_cp * cast_state( s )->cp;
  }

  // the dot keeps a copy of the cast's state, so each tick uses the combo points it was cast with
  double base_ta( const action_state_t* s ) const override
  { return rogue_finisher_t::base_ta( s ) + per_cp * cast_state( s )->cp; }

  void tick( dot_t* d ) override
  {
    rogue_finisher_t::tick( d );

    // Thousand Cuts: each periodic tick reduces the next Backstab/Hemorrhage cost
    if ( p()->talent.thousand_cuts.ok() )
      p()->buff.thousand_cuts->trigger();
  }
};

// Slice and Dice: melee auto attack speed (aura 319), 6 s + 3 s per combo point, Improved Slice and Dice extends it
struct slice_and_dice_t : public rogue_finisher_t<rogue_spell_t>
{
  slice_and_dice_t( rogue_t* p, util::string_view options_str ) :
    rogue_finisher_t<rogue_spell_t>( "slice_and_dice", p, p->find_spell( max_rank( p, slice_and_dice_ranks ).spell_id ) )
  {
    parse_options( options_str );

    harmful  = false;
    may_miss = false;
    target   = p;
  }

  void execute() override
  {
    rogue_finisher_t::execute();

    // data().duration() already carries Improved Slice and Dice (parsed passive), the per point part gets the
    // same multiplier
    double pct          = p()->get_passive_value( data(), "duration" )[ 2 ];
    timespan_t duration = data().duration() + slice_and_dice_per_cp * cast_state( execute_state )->cp * pct;
    p()->buff.slice_and_dice->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
  }
};

// Venom (Forever, 31 point Assassination talent): poison damage +30% and poison application chance +10%,
// 6 s + 3 s per combo point
struct venom_t : public rogue_finisher_t<rogue_spell_t>
{
  venom_t( rogue_t* p, util::string_view options_str ) :
    rogue_finisher_t<rogue_spell_t>( "venom", p, p->talent.venom.ok() ? p->talent.venom.spell() : spell_data_t::not_found() )
  {
    parse_options( options_str );

    harmful  = false;
    may_miss = false;
    target   = p;
  }

  void execute() override
  {
    rogue_finisher_t::execute();

    p()->buff.venom->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0,
                              data().duration() + venom_per_cp * cast_state( execute_state )->cp );
  }
};

// Expose Armor: armor reduction per combo point (rank 5: 450), 30 s. Forever: no longer exclusive with Sunder
// Armor. Only reduces armor for this rogue's own attacks (no raid wide armor debuff yet).
// Improved Expose Armor refunds combo points on a 5 combo point cast.
struct expose_armor_t : public rogue_finisher_t<rogue_attack_t>
{
  expose_armor_t( rogue_t* p, util::string_view options_str ) :
    rogue_finisher_t<rogue_attack_t>( "expose_armor", p, p->find_spell( max_rank( p, expose_armor_ranks ).spell_id ) )
  {
    parse_options( options_str );

    if ( data().ok() && target )
      p->get_target_data( target );
  }

  void impact( action_state_t* s ) override
  {
    rogue_finisher_t::impact( s );

    if ( result_is_hit( s->result ) )
      td( s->target )->debuff.expose_armor->trigger( 1, data().effectN( 1 ).pp_combo_points() * cast_state( s )->cp );
  }

  void spend_combo_points( const action_state_t* s ) override
  {
    rogue_finisher_t::spend_combo_points( s );

    const auto& t = p()->talent.improved_expose_armor;
    if ( t.ok() && cast_state( s )->cp >= t->effectN( 3 ).base_value() )
      p()->trigger_combo_point_gain( t->effectN( 2 ).base_value(), p()->gain.improved_expose_armor, this );
  }
};

// ==========================================================================
// Cooldowns and utility
// ==========================================================================

struct rogue_buff_spell_t : public rogue_spell_t
{
  buff_t* buff;

  rogue_buff_spell_t( util::string_view name, rogue_t* p, const spell_data_t* s, buff_t* b, util::string_view options_str ) :
    rogue_spell_t( name, p, s ), buff( b )
  {
    parse_options( options_str );

    harmful  = false;
    may_miss = false;
    target   = p;
  }

  void execute() override
  {
    rogue_spell_t::execute();

    buff->trigger();
  }
};

struct adrenaline_rush_t : public rogue_buff_spell_t
{
  adrenaline_rush_t( rogue_t* p, util::string_view options_str ) :
    rogue_buff_spell_t( "adrenaline_rush", p, p->talent.adrenaline_rush.ok() ? p->talent.adrenaline_rush.spell() : spell_data_t::not_found(),
                        p->buff.adrenaline_rush, options_str )
  {}
};

struct blade_flurry_t : public rogue_buff_spell_t
{
  blade_flurry_t( rogue_t* p, util::string_view options_str ) :
    rogue_buff_spell_t( "blade_flurry", p, p->talent.blade_flurry.ok() ? p->talent.blade_flurry.spell() : spell_data_t::not_found(),
                        p->buff.blade_flurry, options_str )
  {}
};

// Off the GCD (no GCD in the data)
struct cold_blood_t : public rogue_buff_spell_t
{
  cold_blood_t( rogue_t* p, util::string_view options_str ) :
    rogue_buff_spell_t( "cold_blood", p, p->talent.cold_blood.ok() ? p->talent.cold_blood.spell() : spell_data_t::not_found(),
                        p->buff.cold_blood, options_str )
  {}

  // Cooldown On Event: in game the cooldown starts when the buff is used; modelled as starting on cast
};

// Premeditation: +2 combo points while stealthed, lost if not used within 20 s. Off the GCD.
struct premeditation_t : public rogue_spell_t
{
  premeditation_t( rogue_t* p, util::string_view options_str ) :
    rogue_spell_t( "premeditation", p, p->talent.premeditation.ok() ? p->talent.premeditation.spell() : spell_data_t::not_found() )
  {
    parse_options( options_str );

    harmful  = false;
    may_miss = false;
    target   = p;
    energize_type = action_energize::NONE;  // granted manually so the premeditation gain is tracked
  }

  bool ready() override
  {
    if ( !p()->buff.stealth->check() )
      return false;

    return rogue_spell_t::ready();
  }

  void execute() override
  {
    rogue_spell_t::execute();

    p()->trigger_combo_point_gain( data().effectN( 1 ).base_value(), p()->gain.premeditation, this );
    p()->buff.premeditation->trigger();
  }
};

// Preparation: finishes the cooldown of every other rogue ability (not items, potions or racials)
struct preparation_t : public rogue_spell_t
{
  preparation_t( rogue_t* p, util::string_view options_str ) :
    rogue_spell_t( "preparation", p, p->talent.preparation.ok() ? p->talent.preparation.spell() : spell_data_t::not_found() )
  {
    parse_options( options_str );

    harmful  = false;
    may_miss = false;
    target   = p;
  }

  void execute() override
  {
    rogue_spell_t::execute();

    for ( action_t* a : player->action_list )
    {
      if ( a != this && a->data().is_class( ROGUE ) && a->cooldown && a->cooldown != cooldown &&
           a->cooldown->duration > 0_ms )
        a->cooldown->reset( false );
    }
  }
};

// Stealth: precombat only (Not In Combat), enables the openers, broken by any harmful action
struct stealth_t : public rogue_spell_t
{
  stealth_t( rogue_t* p, util::string_view options_str ) :
    rogue_spell_t( "stealth", p, p->find_spell( max_rank( p, stealth_ranks ).spell_id ) )
  {
    parse_options( options_str );

    harmful     = false;
    may_miss    = false;
    target      = p;
    trigger_gcd = 0_ms;
  }

  bool ready() override
  {
    if ( player->in_combat || p()->buff.stealth->check() )
      return false;

    return rogue_spell_t::ready();
  }

  void execute() override
  {
    rogue_spell_t::execute();

    p()->buff.stealth->trigger();
  }
};

// ==========================================================================
// Procs
// ==========================================================================

void rogue_t::trigger_combo_point_gain( double amount, gain_t* g, action_t* a )
{
  resource_gain( RESOURCE_COMBO_POINT, amount, g, a );
}

// Seal Fate: crits from combo point builders have a chance to add a combo point; the 0.5 s internal cooldown
// (data) limits Mutilate to one extra point per cast.
void rogue_t::trigger_seal_fate( const action_state_t* s )
{
  if ( !talent.seal_fate.ok() || cooldowns.seal_fate_icd->down() )
    return;

  if ( !rng().roll( talent.seal_fate->effectN( 1 ).percent() ) )
    return;

  cooldowns.seal_fate_icd->start();
  proc.seal_fate->occur();
  trigger_combo_point_gain( 1, gain.seal_fate, s->action );
}

// Weapon poisons: chance per landed hit with that weapon (white or yellow), from the application spell (which carries
// Improved Poisons), plus Venom. The poison then rolls its own (Nature) spell hit.
void rogue_t::trigger_poisons( const action_state_t* s )
{
  const weapon_t* w = s->action->weapon;
  if ( !w )
    return;

  poison_e poison = w->slot == SLOT_OFF_HAND ? oh_poison : mh_poison;
  if ( poison == poison_e::NONE )
    return;

  rogue_poison_t* a = poison == poison_e::INSTANT ? instant_poison : deadly_poison;
  double chance     = a->application->proc_chance();

  if ( buff.venom->check() )
    chance += buff.venom->data().effectN( 4 ).percent();

  if ( !rng().roll( chance ) )
    return;

  ( poison == poison_e::INSTANT ? proc.instant_poison : proc.deadly_poison )->occur();
  a->execute_on_target( s->target );
}

// Hack and Slash (sword/axe branch): chance on a landed melee hit for an extra main hand swing, 0.2 s ICD (data)
void rogue_t::trigger_hack_and_slash( const action_state_t* s )
{
  if ( !hack_and_slash_swing || cooldowns.hack_and_slash_icd->down() )
    return;

  const weapon_t* w = s->action->weapon;
  if ( !w || ( w->type != WEAPON_SWORD && w->type != WEAPON_AXE ) )
    return;

  if ( !rng().roll( talent.hack_and_slash->effectN( 3 ).percent() ) )
    return;

  cooldowns.hack_and_slash_icd->start();
  proc.hack_and_slash->occur();
  hack_and_slash_swing->execute_on_target( s->target );
}

void rogue_t::trigger_blade_flurry( const action_state_t* s )
{
  if ( !blade_flurry_attack || !buff.blade_flurry->check() || s->result_amount <= 0 )
    return;

  if ( s->action == blade_flurry_attack )
    return;

  for ( player_t* t : sim->target_non_sleeping_list )
  {
    if ( t == s->target || !t->is_enemy() )
      continue;

    blade_flurry_attack->base_dd_min = blade_flurry_attack->base_dd_max = s->result_amount;
    blade_flurry_attack->execute_on_target( t );
    return;
  }
}

// ==========================================================================
// Rogue Character Definition
// ==========================================================================

void rogue_t::init_base_stats()
{
  // vanilla rogue: AP = Str + Agi + 2 x level - 20
  base.attack_power_per_strength = 1.0;
  base.attack_power_per_agility  = 1.0;

  player_t::init_base_stats();

  base.stats.attack_power = 2.0 * level() - 20.0;

  // ESTIMATE: vanilla rogue base crit is 0% (crit comes from agility); Malice adds through "all_crit"
  double all_crit         = get_passive_player_value( 0.0, "all_crit" );
  base.attack_crit_chance = get_passive_player_value( all_crit, "attack_crit" );
  base.spell_crit_chance  = get_passive_player_value( all_crit, "spell_crit" );

  // classic energy regen is not affected by melee haste
  resources.hasted[ RESOURCE_ENERGY ] = false;
}

void rogue_t::init_spells()
{
  player_t::init_spells();

  auto t = [ this ]( std::string_view name ) { return find_talent_spell( talent_tree::CLASS, name ); };

  talent.malice                  = t( "Malice" );
  talent.ruthlessness            = t( "Ruthlessness" );
  talent.murder                  = t( "Murder" );
  talent.improved_slice_and_dice = t( "Improved Slice and Dice" );
  talent.relentless_strikes      = t( "Relentless Strikes" );
  talent.improved_expose_armor   = t( "Improved Expose Armor" );
  talent.lethality               = t( "Lethality" );
  talent.vile_poisons            = t( "Vile Poisons" );
  talent.cold_blood              = t( "Cold Blood" );
  talent.improved_poisons        = t( "Improved Poisons" );
  talent.vigor                   = t( "Vigor" );
  talent.mutilate                = t( "Mutilate" );
  talent.seal_fate               = t( "Seal Fate" );
  talent.venom                   = t( "Venom" );

  talent.improved_eviscerate       = t( "Improved Eviscerate" );
  talent.improved_sinister_strike  = t( "Improved Sinister Strike" );
  talent.puncturing_wounds         = t( "Puncturing Wounds" );
  talent.precision                 = t( "Precision" );
  talent.flawless_execution        = t( "Flawless Execution" );
  talent.dual_wield_specialization = t( "Dual Wield Specialization" );
  talent.blade_flurry              = t( "Blade Flurry" );
  talent.hack_and_slash            = t( "Hack and Slash" );
  talent.weapon_expertise          = t( "Weapon Expertise" );
  talent.aggression                = t( "Aggression" );
  talent.adrenaline_rush           = t( "Adrenaline Rush" );

  talent.opportunity     = t( "Opportunity" );
  talent.improved_ambush = t( "Improved Ambush" );
  talent.initiative      = t( "Initiative" );
  talent.ghostly_strike  = t( "Ghostly Strike" );
  talent.premeditation   = t( "Premeditation" );
  talent.serrated_blades = t( "Serrated Blades" );
  talent.dirty_deeds     = t( "Dirty Deeds" );
  talent.preparation     = t( "Preparation" );
  talent.hemorrhage      = t( "Hemorrhage" );
  talent.quietus         = t( "Quietus" );
  talent.cutthroat       = t( "Cutthroat" );
  talent.thousand_cuts   = t( "Thousand Cuts" );

  // 2026-10-16: Coup de Grace is a hidden level 30 passive in the Assassination skill line (AcquireMethod 0), not a
  // talent; assumed baseline for every rogue from level 30 (validation hook V-R10).
  spell.coup_de_grace             = find_spell( 1310709 );
  spell.relentless_strikes_energy = find_spell( 14181 );
  spell.cutthroat_buff            = find_spell( 462707 );
  spell.thousand_cuts_buff        = find_spell( 1310723 );
  spell.hack_and_slash_dagger     = find_spell( 13706 );
  spell.hack_and_slash_mace       = find_spell( 13709 );

  // Hack and Slash and Dual Wield Specialization are dummies / unknown auras, applied in code
  parse_all_class_passives();
  parse_all_passive_talents();
}

void rogue_t::init_gains()
{
  player_t::init_gains();

  gain.relentless_strikes    = get_gain( "relentless_strikes" );
  gain.ruthlessness          = get_gain( "ruthlessness" );
  gain.seal_fate             = get_gain( "seal_fate" );
  gain.puncturing_wounds     = get_gain( "puncturing_wounds" );
  gain.initiative            = get_gain( "initiative" );
  gain.premeditation         = get_gain( "premeditation" );
  gain.improved_expose_armor = get_gain( "improved_expose_armor" );
}

void rogue_t::init_procs()
{
  player_t::init_procs();

  proc.seal_fate      = get_proc( "seal_fate" );
  proc.hack_and_slash = get_proc( "hack_and_slash" );
  proc.cutthroat      = get_proc( "cutthroat" );
  proc.instant_poison = get_proc( "instant_poison" );
  proc.deadly_poison  = get_proc( "deadly_poison" );
}

void rogue_t::create_buffs()
{
  player_t::create_buffs();

  buff.stealth = make_buff( this, "stealth", find_spell( max_rank( this, stealth_ranks ).spell_id ) )
                   ->set_cooldown( 0_ms );

  buff.slice_and_dice = make_buff( this, "slice_and_dice", find_spell( max_rank( this, slice_and_dice_ranks ).spell_id ) )
                          ->set_default_value_from_effect_type( A_MOD_MELEE_AUTO_ATTACK_SPEED )
                          ->add_invalidate( CACHE_AUTO_ATTACK_SPEED );

  buff.blade_flurry = make_buff( this, "blade_flurry", talent.blade_flurry.spell() )
                        ->set_default_value_from_effect_type( A_MOD_MELEE_AUTO_ATTACK_SPEED )
                        ->set_cooldown( 0_ms )
                        ->add_invalidate( CACHE_AUTO_ATTACK_SPEED );

  // Energy regeneration +100% (aura 110), see resource_regen_per_second
  buff.adrenaline_rush = make_buff( this, "adrenaline_rush", talent.adrenaline_rush.spell() )
                           ->set_default_value_from_effect_type( A_MOD_POWER_REGEN_PERCENT )
                           ->set_cooldown( 0_ms );

  buff.cold_blood = make_buff( this, "cold_blood", talent.cold_blood.spell() )
                      ->set_cooldown( 0_ms );

  buff.venom = make_buff( this, "venom", talent.venom.spell() )
                 ->set_cooldown( 0_ms );

  buff.premeditation = make_buff( this, "premeditation", talent.premeditation.spell() )
                         ->set_cooldown( 0_ms )
                         ->set_quiet( true )
                         ->set_expire_callback( [ this ]( buff_t*, int, timespan_t remains ) {
                             // unused Premeditation combo points are lost when it runs out
                             if ( remains <= 0_ms )
                               resource_loss( RESOURCE_COMBO_POINT,
                                              std::min( premeditation_cp, resources.current[ RESOURCE_COMBO_POINT ] ) );
                           } );

  buff.cutthroat = make_buff( this, "cutthroat", spell.cutthroat_buff );

  // Thousand Cuts: data says "Proc Trigger Spell" every Rupture tick (1.9 s ICD), 1310723 = -3 energy per stack
  buff.thousand_cuts = make_buff( this, "thousand_cuts", talent.thousand_cuts.ok() ? spell.thousand_cuts_buff : spell_data_t::not_found() );
}

void rogue_t::create_actions()
{
  instant_poison = new instant_poison_t( this );
  deadly_poison  = new deadly_poison_t( this );

  if ( talent.hack_and_slash.ok() && equipped_weapons[ SLOT_MAIN_HAND ].type != WEAPON_NONE )
  {
    hack_and_slash_swing    = new hack_and_slash_swing_t( this );
    cooldowns.hack_and_slash_icd = get_cooldown( "hack_and_slash_icd" );
    cooldowns.hack_and_slash_icd->duration = talent.hack_and_slash->internal_cooldown();
  }

  if ( talent.blade_flurry.ok() )
    blade_flurry_attack = new blade_flurry_attack_t( this );

  cooldowns.seal_fate_icd = get_cooldown( "seal_fate_icd" );
  cooldowns.seal_fate_icd->duration = talent.seal_fate.ok() ? talent.seal_fate->internal_cooldown() : 0_ms;

  player_t::create_actions();
}

void rogue_t::reset()
{
  player_t::reset();

  mh_poison = poison_e::NONE;
  oh_poison = poison_e::NONE;
}

action_t* rogue_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "auto_attack" ) return new rogue_auto_attack_t( this, options_str );
  if ( name == "apply_poison" ) return new apply_poison_t( this, options_str );
  if ( name == "stealth" ) return new stealth_t( this, options_str );

  if ( name == "sinister_strike" ) return new sinister_strike_t( this, options_str );
  if ( name == "backstab" ) return new backstab_t( this, options_str );
  if ( name == "ambush" ) return new ambush_t( this, options_str );
  if ( name == "garrote" ) return new garrote_t( this, options_str );
  if ( name == "ghostly_strike" ) return new ghostly_strike_t( this, options_str );
  if ( name == "hemorrhage" ) return new hemorrhage_t( this, options_str );
  if ( name == "mutilate" ) return new mutilate_t( this, options_str );

  if ( name == "eviscerate" ) return new eviscerate_t( this, options_str );
  if ( name == "rupture" ) return new rupture_t( this, options_str );
  if ( name == "slice_and_dice" ) return new slice_and_dice_t( this, options_str );
  if ( name == "venom" ) return new venom_t( this, options_str );
  if ( name == "expose_armor" ) return new expose_armor_t( this, options_str );

  if ( name == "adrenaline_rush" ) return new adrenaline_rush_t( this, options_str );
  if ( name == "blade_flurry" ) return new blade_flurry_t( this, options_str );
  if ( name == "cold_blood" ) return new cold_blood_t( this, options_str );
  if ( name == "premeditation" ) return new premeditation_t( this, options_str );
  if ( name == "preparation" ) return new preparation_t( this, options_str );

  return player_t::create_action( name, options_str );
}

void rogue_t::init_action_list()
{
  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  clear_action_priority_lists();
  rogue_apl::rogue( this );
  use_default_action_list = true;
  player_t::init_action_list();
}

std::unique_ptr<expr_t> rogue_t::create_expression( util::string_view name )
{
  // Forever: seconds until the next 2 s energy tick
  if ( util::str_compare_ci( name, "energy.time_to_tick" ) )
    return make_fn_expr( name, [ this ] { return time_to_next_energy_tick().total_seconds(); } );

  if ( util::str_compare_ci( name, "stealthed" ) )
    return make_fn_expr( name, [ this ] { return buff.stealth->check() > 0; } );

  return player_t::create_expression( name );
}

// Adrenaline Rush doubles energy regeneration; with classic ticks that is 40 energy per 2 s tick
double rogue_t::resource_regen_per_second( resource_e r ) const
{
  double reg = player_t::resource_regen_per_second( r );

  if ( r == RESOURCE_ENERGY && buff.adrenaline_rush->check() )
    reg *= 1.0 + buff.adrenaline_rush->check_value();

  return reg;
}

// Slice and Dice and Blade Flurry (aura 319), assumed multiplicative with each other (validation hook V-R7)
double rogue_t::composite_melee_auto_attack_speed() const
{
  double s = player_t::composite_melee_auto_attack_speed();

  if ( buff.slice_and_dice->check() )
    s /= 1.0 + buff.slice_and_dice->check_value();

  if ( buff.blade_flurry->check() )
    s /= 1.0 + buff.blade_flurry->check_value();

  return s;
}

// Precision: melee hit (aura 54) and, in Forever, spell hit (aura 55, poisons); neither aura is parsed by the
// engine's passive system
double rogue_t::composite_melee_hit() const
{
  double h = player_t::composite_melee_hit();

  if ( talent.precision.ok() )
    h += talent.precision->effectN( 1 ).percent();

  return h;
}

// The engine adds expertise to spell hit (a retail rule); a rogue's expertise (Weapon Expertise) only reduces
// dodge and parry, so spell hit (poisons) is plain hit plus Precision.
double rogue_t::composite_spell_hit() const
{
  double h = current.hit;

  if ( talent.precision.ok() )
    h += talent.precision->effectN( 2 ).percent();

  return h;
}

// Murder: all damage vs Humanoid and Giant targets (aura 168, not parsed by the engine)
double rogue_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  if ( talent.murder.ok() && ( target->race == RACE_HUMANOID || target->race == RACE_GIANT ) )
    m *= 1.0 + talent.murder->effectN( 1 ).percent();

  return m;
}

// Expose Armor (own attacks only) before the percentage armor ignore (Serrated Blades, aura 280)
double rogue_t::composite_player_target_armor( player_t* t ) const
{
  double a = t->cache.armor();

  if ( const rogue_td_t* td = target_data[ t ] )
    if ( td->debuff.expose_armor->check() )
      a = std::max( 0.0, a + td->debuff.expose_armor->check_value() );

  return a * current.armor_penetration;
}

// ==========================================================================
// Rogue Module Interface
// ==========================================================================

class rogue_module_t : public module_t
{
public:
  rogue_module_t() : module_t( ROGUE ) {}

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    return new rogue_t( sim, name, r );
  }

  bool valid() const override
  { return true; }

  void register_hotfixes() const override {}

  void register_actor_initializers( sim_t* ) const override {}
};

}  // UNNAMED NAMESPACE

const module_t* module_t::rogue()
{
  static rogue_module_t m;
  return &m;
}

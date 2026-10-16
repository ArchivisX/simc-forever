// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
//
// WoW: Forever paladin. Retribution (seals, Judgement, Holy Strike) plus the Holy damage tools
// (Holy Shock, Exorcism, Consecration). Healing and tanking are not modelled. See FOREVER_PALADIN.md.

#include "simulationcraft.hpp"
#include "action/parse_effects.hpp"
#include "class_modules/apl/paladin.hpp"
#include "class_modules/forever_helpers.hpp"
#include "player/ground_aoe.hpp"

namespace
{
// ==========================================================================
// Paladin
// ==========================================================================

using forever::max_rank;
using forever::max_rank_spell;
using forever::spell_rank_t;

struct consecration_rank_t
{
  unsigned spell_id;  // the cast
  unsigned tick_id;   // the damage pulse (Forever 1280345+, no data link from the cast)
};
struct holy_shock_rank_t
{
  unsigned spell_id;   // the cast (cost, cooldown)
  unsigned damage_id;  // the enemy damage spell
};
struct lights_vigil_rank_t
{
  unsigned spell_id;   // the cast
  unsigned debuff_id;  // the mark on the target
  unsigned damage_id;  // the damage the next Holy Shock adds
};

// Seal of Righteousness rank 1: 20154 is not trainable (AcquireMethod 2) and has no Judgement effect;
// 21084 is the taught copy. Same level, so list 21084 last.
constexpr std::array seal_of_righteousness_ranks {
  spell_rank_t{ 20154 }, spell_rank_t{ 21084 }, spell_rank_t{ 20287 }, spell_rank_t{ 20288 },
  spell_rank_t{ 20289 }, spell_rank_t{ 20290 }, spell_rank_t{ 20291 }, spell_rank_t{ 20292 },
  spell_rank_t{ 20293 },
};

constexpr std::array seal_of_command_ranks {
  spell_rank_t{ 20375 }, spell_rank_t{ 20915 }, spell_rank_t{ 20918 }, spell_rank_t{ 20919 }, spell_rank_t{ 20920 },
};

// Forever's new Seal of Fury; ranks 1 and 2 are new ids
constexpr std::array seal_of_fury_ranks {
  spell_rank_t{ 1311649 }, spell_rank_t{ 1311656 }, spell_rank_t{ 20163 }, spell_rank_t{ 20419 },
  spell_rank_t{ 20421 },   spell_rank_t{ 20422 },   spell_rank_t{ 20423 },
};

constexpr std::array seal_of_the_crusader_ranks {
  spell_rank_t{ 21082 }, spell_rank_t{ 20162 }, spell_rank_t{ 20305 },
  spell_rank_t{ 20306 }, spell_rank_t{ 20307 }, spell_rank_t{ 20308 },
};

constexpr std::array holy_strike_ranks {
  spell_rank_t{ 679 },  spell_rank_t{ 678 },  spell_rank_t{ 1866 },  spell_rank_t{ 680 },
  spell_rank_t{ 2495 }, spell_rank_t{ 5569 }, spell_rank_t{ 10332 }, spell_rank_t{ 10333 },
};

constexpr std::array consecration_ranks {
  consecration_rank_t{ 26573, 1280345 }, consecration_rank_t{ 20116, 1280346 }, consecration_rank_t{ 20922, 1280347 },
  consecration_rank_t{ 20923, 1280348 }, consecration_rank_t{ 20924, 1280349 },
};

// Explicit ids: the Season of Discovery copies 415068-415073 share the name (AcquireMethod 3)
constexpr std::array exorcism_ranks {
  spell_rank_t{ 879 }, spell_rank_t{ 5614 }, spell_rank_t{ 5615 }, spell_rank_t{ 10312 }, spell_rank_t{ 10313 },
  spell_rank_t{ 10314 },
};

constexpr std::array hammer_of_wrath_ranks {
  spell_rank_t{ 24275 }, spell_rank_t{ 24274 }, spell_rank_t{ 24239 },
};

constexpr std::array holy_wrath_ranks {
  spell_rank_t{ 2812 }, spell_rank_t{ 10318 },
};

constexpr std::array holy_shock_ranks {
  holy_shock_rank_t{ 1311606, 1311604 }, holy_shock_rank_t{ 20473, 25912 }, holy_shock_rank_t{ 20929, 25911 },
  holy_shock_rank_t{ 20930, 25902 },
};

constexpr std::array lights_vigil_ranks {
  lights_vigil_rank_t{ 1310911, 1310909, 1310914 }, lights_vigil_rank_t{ 1311590, 1311593, 1311592 },
  lights_vigil_rank_t{ 1311595, 1311597, 1311598 },
};

constexpr std::array blessing_of_might_ranks {
  spell_rank_t{ 19740 }, spell_rank_t{ 19834 }, spell_rank_t{ 19835 }, spell_rank_t{ 19836 },
  spell_rank_t{ 19837 }, spell_rank_t{ 19838 }, spell_rank_t{ 25291 },
};

constexpr std::array blessing_of_kings_ranks {
  spell_rank_t{ 20217 },
};

constexpr unsigned judgement_id       = 20271;
constexpr unsigned vengeance_buff_id  = 20050;
constexpr unsigned vindication_buff_id = 440667;  // only the duration is used; the self buff value is the talent's

// Hammer of Wrath is only usable on targets at or below this health percentage (spell description)
constexpr double hammer_of_wrath_health_pct = 20.0;

// 2026-10-16: Judgement of Command deals half damage unless the target is stunned or incapacitated
// (spell description "$/2;20966s1"). Stuns are not modelled, so it always deals half.
constexpr double judgement_of_command_unstunned = 0.5;

// 2026-10-16 ESTIMATE: Seal of Command has proc chance 100, no PPM record and a 1 s internal cooldown in
// the client data, so the real rate is server side. Vanilla was 7 procs per minute (chance = 7 * weapon
// speed / 60). A beta report counts 3-4 per minute; override with paladin.soc_ppm=.
constexpr double seal_of_command_default_ppm = 7.0;

// 2026-10-16 ESTIMATE: Seal of Righteousness per swing damage is server side. The tooltip ranges from
// (s/87 + 0.0765 SP) to (s/25 + 0.12 SP) times (1 + Improved Seals), "slower weapons cause more Holy damage".
// We use the vanilla per weapon speed formula (1H: s * 0.85 * speed / 100, 2H: s * 1.2 * speed / 100), which
// reproduces the tooltip ends at 1.35 s (1H) and 3.33 s (2H), with the flat tooltip SP coefficients.
constexpr double sor_1h_factor  = 0.85;
constexpr double sor_2h_factor  = 1.2;
constexpr double sor_1h_sp_coef = 0.0765;
constexpr double sor_2h_sp_coef = 0.12;

// The client spell data skips effect indices (seal effects are #1 and #3), so look effects up by index.
const spelleffect_data_t& effect_at( const spell_data_t* spell, unsigned idx )
{
  for ( const spelleffect_data_t& eff : spell->effects() )
    if ( eff.id() != 0 && eff.index() + 1 == idx )
      return eff;

  return spelleffect_data_t::nil();
}

// Spell of a talent, or not_found if the talent isn't taken
const spell_data_t* talent_spell( const player_talent_t& t )
{
  return t.ok() ? t.spell() : spell_data_t::not_found();
}

enum seal_e
{
  SEAL_NONE = 0,
  SEAL_RIGHTEOUSNESS,
  SEAL_COMMAND,
  SEAL_FURY,
  SEAL_CRUSADER,
  SEAL_MAX
};

struct paladin_t;

struct paladin_td_t : public actor_target_data_t
{
  struct debuffs_t
  {
    buff_t* judgement_of_the_crusader = nullptr;
    buff_t* lights_vigil              = nullptr;
  } debuff;

  paladin_td_t( player_t* target, paladin_t* p );
};

struct paladin_t final : public player_t
{
  paladin_t( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) :
    player_t( sim, PALADIN, name, r ) {}

  resource_e primary_resource() const override { return RESOURCE_MANA; }
  role_e primary_role() const override { return ROLE_ATTACK; }

  void init_base_stats() override;
  void init_spells() override;
  void init_gains() override;
  void create_buffs() override;
  void create_actions() override;
  void create_options() override;
  void copy_from( player_t* source ) override;
  void reset() override;

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
  void init_action_list() override;

  std::string default_potion() const override   { return paladin_apl::potion( this ); }
  std::string default_flask() const override    { return paladin_apl::flask( this ); }
  std::string default_food() const override     { return paladin_apl::food( this ); }

  double composite_spell_power( school_e school ) const override;
  double composite_attack_power_multiplier() const override;
  double composite_player_multiplier( school_e school ) const override;
  double composite_melee_auto_attack_speed() const override;
  void invalidate_cache( cache_e c ) override;

  paladin_td_t* get_target_data( player_t* target ) const override
  {
    paladin_td_t*& td = target_data[ target ];
    if ( !td )
      td = new paladin_td_t( target, const_cast<paladin_t*>( this ) );
    return td;
  }

  struct options_t
  {
    double soc_ppm = seal_of_command_default_ppm;
  } options;

  struct talents_t
  {
    // Holy
    player_talent_t improved_seals;
    player_talent_t divine_favor;       // 16
    player_talent_t holy_shock;         // 21
    player_talent_t consecrated_ground;
    player_talent_t divine_precision;
    player_talent_t holy_power;
    player_talent_t lights_vigil;       // 31

    // Protection
    player_talent_t swift_judgement;    // 16

    // Retribution
    player_talent_t vindication;
    player_talent_t sanctified_judgement;
    player_talent_t seal_of_command;    // 11
    player_talent_t sacred_arbiter;     // 16
    player_talent_t vengeance;
    player_talent_t champion_of_the_light;
    player_talent_t twist_of_light;     // 31
  } talent;

  struct buffs_t
  {
    std::array<buff_t*, SEAL_MAX> seal {};
    std::array<buff_t*, SEAL_MAX> echo {};

    buff_t* blessing_of_might = nullptr;
    buff_t* blessing_of_kings = nullptr;

    buff_t* vengeance      = nullptr;
    buff_t* vindication    = nullptr;
    buff_t* divine_favor   = nullptr;
    buff_t* swift_judgement = nullptr;
  } buff;

  struct gains_t
  {
    gain_t* sanctified_judgement = nullptr;
    gain_t* lights_vigil         = nullptr;
  } gain;

  // the active seal, its proc and its Judgement
  seal_e active_seal = SEAL_NONE;
  std::array<double, SEAL_MAX> seal_cost {};
  std::array<action_t*, SEAL_MAX> seal_proc {};
  std::array<action_t*, SEAL_MAX> judgement_action {};

  cooldown_t* seal_of_command_icd = nullptr;
  cooldown_t* judgement_cd        = nullptr;

  // Consecrated Ground: Holy damage bonus while the target stands in our Consecration
  timespan_t consecration_expires = timespan_t::min();

  mutable target_specific_t<paladin_td_t> target_data;

  void activate_seal( seal_e seal, double cost );
  void trigger_seals( action_state_t* s );
  void trigger_vengeance( const action_state_t* s );
  void trigger_vindication( const action_state_t* s );
  void refresh_judgements( player_t* t );

  bool consecration_active() const
  { return consecration_expires > sim->current_time(); }

  const char* seal_name( seal_e s ) const;
};

paladin_td_t::paladin_td_t( player_t* target, paladin_t* p ) : actor_target_data_t( target, p )
{
  // Judgement of the Crusader: flat Holy damage taken (effect #1, A14), from the active seal's rank
  const spell_data_t* sotc = max_rank_spell( p, seal_of_the_crusader_ranks );
  const spell_data_t* jotc = sotc->ok() ? dbc::find_spell( p, as<unsigned>( effect_at( sotc, 3 ).base_value() ) )
                                        : spell_data_t::not_found();
  debuff.judgement_of_the_crusader = make_buff( *this, "judgement_of_the_crusader", jotc )
    ->set_default_value( jotc->ok() ? jotc->effectN( 1 ).base_value() : 0.0 );

  const spell_data_t* vigil = p->talent.lights_vigil.ok()
                                ? dbc::find_spell( p, max_rank( p, lights_vigil_ranks ).debuff_id )
                                : spell_data_t::not_found();
  debuff.lights_vigil = make_buff( *this, "lights_vigil", vigil );
}

template <class Base>
struct paladin_action_t : public parse_action_effects_t<Base>
{
private:
  using ab = parse_action_effects_t<Base>;

public:
  using base_t = paladin_action_t<Base>;

  bool is_holy = false;
  double flat_crit_bonus = 0.0;

  template <typename... Args>
  paladin_action_t( Args&&... args ) : ab( std::forward<Args>( args )... )
  {
    // conditional buffs whose spell modifiers name the affected spells
    this->parse_effects( p()->buff.divine_favor );
    this->parse_effects( p()->buff.swift_judgement );

    // Divine Precision: Holy spell hit (flat modifier property 16) is an action scoped effect
    if ( p()->talent.divine_precision.ok() )
      this->parse_effects( p()->talent.divine_precision.spell(), PARSE_PASSIVE );

    // Holy Power: classic "Add Percent Modifier: Spell Critical Chance" adds crit chance points (+1-5% to the
    // listed spells, +3-15% to Holy Shock and Holy Strike); simc would multiply the spells' zero base crit.
    if ( p()->talent.holy_power.ok() )
    {
      for ( const spelleffect_data_t& eff : p()->talent.holy_power->effects() )
        if ( eff.subtype() == A_ADD_PCT_MODIFIER && eff.misc_value1() == P_CRIT && this->data().affected_by( eff ) )
          flat_crit_bonus += eff.percent();
    }

    is_holy = dbc::is_school( this->school, SCHOOL_HOLY );
  }

  paladin_t* p()
  { return debug_cast<paladin_t*>( this->player ); }

  const paladin_t* p() const
  { return debug_cast<const paladin_t*>( this->player ); }

  paladin_td_t* td( player_t* t ) const
  { return p()->get_target_data( t ); }

  void init() override
  {
    ab::init();

    is_holy = dbc::is_school( this->school, SCHOOL_HOLY );
  }

  // 2026-10-16 ESTIMATE: Judgement of the Crusader's "Holy damage taken increased by up to X" is applied like
  // vanilla (wowsims/classic): extra spell power against that target, scaled by the spell's coefficient.
  double jotc_bonus( const action_state_t* s, double coefficient ) const
  {
    if ( !is_holy || coefficient <= 0 )
      return 0.0;

    const paladin_td_t* t = p()->target_data[ s->target ];
    if ( !t )
      return 0.0;

    return t->debuff.judgement_of_the_crusader->check_value() * coefficient;
  }

  double composite_crit_chance() const override
  { return ab::composite_crit_chance() + flat_crit_bonus; }

  double bonus_da( const action_state_t* s ) const override
  {
    return ab::bonus_da( s ) + jotc_bonus( s, this->spell_direct_power_coefficient( s ) );
  }

  double bonus_ta( const action_state_t* s ) const override
  {
    return ab::bonus_ta( s ) + jotc_bonus( s, this->spell_tick_power_coefficient( s ) );
  }

  // Consecrated Ground: Holy spells deal more damage to the first 4 enemies in Consecration. Every target in a
  // single target sim is one of the first four.
  double composite_target_multiplier( player_t* t ) const override
  {
    double m = ab::composite_target_multiplier( t );

    if ( is_holy && p()->talent.consecrated_ground.ok() && p()->consecration_active() )
      m *= 1.0 + p()->talent.consecrated_ground->effectN( 1 ).percent();

    return m;
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    p()->trigger_vengeance( s );
  }
};

using paladin_spell_t  = paladin_action_t<spell_t>;
using paladin_attack_t = paladin_action_t<melee_attack_t>;

// Self buffs can't miss and have no target
struct paladin_self_spell_t : public paladin_spell_t
{
  paladin_self_spell_t( util::string_view name, paladin_t* p, const spell_data_t* s, util::string_view options_str ) :
    paladin_spell_t( name, p, s )
  {
    parse_options( options_str );

    harmful  = false;
    may_miss = false;
    may_crit = false;
    target   = p;
  }
};

// Holy effects with "Spell Type: Melee" (seal procs, Judgements, Holy Strike) are melee attacks: they roll the
// melee table unless the data says No Active Defense / Always Hit, and crit as melee (200%), like vanilla.
// They never glance. 2026-10-16 ESTIMATE: Forever's table and crit multiplier for these is unmeasured.
struct paladin_holy_attack_t : public paladin_attack_t
{
  paladin_holy_attack_t( util::string_view name, paladin_t* p, const spell_data_t* s ) :
    paladin_attack_t( name, p, s )
  {
    may_glance = false;
    special    = true;

    // simc only reads the SP coefficient of School Damage effects; Holy Strike (effect #1, Normalized Weapon
    // Damage, sp 0.429) and the Seal of Command proc (Weapon Damage%, sp 0.29) carry theirs on weapon effects.
    if ( spell_power_mod.direct == 0 )
    {
      for ( const spelleffect_data_t& eff : data().effects() )
      {
        if ( ( eff.type() == E_NORMALIZED_WEAPON_DMG || eff.type() == E_WEAPON_DAMAGE ||
               eff.type() == E_WEAPON_PERCENT_DAMAGE ) && eff.sp_coeff() > 0 )
        {
          spell_power_mod.direct = eff.sp_coeff();
          break;
        }
      }
    }
  }
};

/////////////////////////////////////////////////////////////////////////////////////
//
// Seal procs and Judgements
//
/////////////////////////////////////////////////////////////////////////////////////

// Seal of Righteousness: Holy damage on every white swing, scaled with weapon speed (see the ESTIMATE above).
// The client has no proc spell; we model it as a spell that can't miss and crits as a spell.
struct seal_of_righteousness_proc_t : public paladin_spell_t
{
  seal_of_righteousness_proc_t( paladin_t* p, const spell_data_t* seal ) :
    paladin_spell_t( "seal_of_righteousness_proc", p, spell_data_t::nil() )
  {
    background = true;
    proc       = true;
    may_miss   = false;
    may_crit   = true;
    school     = SCHOOL_HOLY;
    id         = seal->id();

    const weapon_t& w = p->equipped_weapons[ SLOT_MAIN_HAND ];
    bool two_hand     = w.group() == WEAPON_2H;
    double speed      = w.swing_time.total_seconds();
    double value      = seal->ok() ? seal->effectN( 1 ).average( p, p->level() ) : 0.0;
    double mult       = 1.0 + p->talent.improved_seals->effectN( 1 ).percent();

    base_dd_min = base_dd_max = value * ( two_hand ? sor_2h_factor : sor_1h_factor ) * speed / 100.0 * mult;
    spell_power_mod.direct    = ( two_hand ? sor_2h_sp_coef : sor_1h_sp_coef ) * mult;
  }

  void init() override
  {
    paladin_spell_t::init();
    is_holy = true;
  }
};

// Seal of Command: 70% weapon damage as Holy (20424), a melee table attack
struct seal_of_command_proc_t : public paladin_holy_attack_t
{
  seal_of_command_proc_t( paladin_t* p, const spell_data_t* seal ) :
    paladin_holy_attack_t( "seal_of_command_proc", p, seal->ok() ? seal->effectN( 1 ).trigger() : spell_data_t::not_found() )
  {
    background = true;
    proc       = true;
    weapon     = &p->equipped_weapons[ SLOT_MAIN_HAND ];
  }
};

// Seal of Fury: flat Holy damage per white swing (the seal's effect #1 trigger spell). Always hits.
// The absorb with a shield equipped is not modelled.
struct seal_of_fury_proc_t : public paladin_holy_attack_t
{
  seal_of_fury_proc_t( paladin_t* p, const spell_data_t* seal ) :
    paladin_holy_attack_t( "seal_of_fury_proc", p, seal->ok() ? seal->effectN( 1 ).trigger() : spell_data_t::not_found() )
  {
    background = true;
    proc       = true;
  }
};

struct judgement_of_righteousness_t : public paladin_holy_attack_t
{
  judgement_of_righteousness_t( paladin_t* p, const spell_data_t* s ) :
    paladin_holy_attack_t( "judgement_of_righteousness", p, s )
  {
    background = true;
  }
};

struct judgement_of_command_t : public paladin_holy_attack_t
{
  judgement_of_command_t( paladin_t* p, const spell_data_t* s ) :
    paladin_holy_attack_t( "judgement_of_command", p, s )
  {
    background = true;
  }

  double action_multiplier() const override
  { return paladin_holy_attack_t::action_multiplier() * judgement_of_command_unstunned; }
};

struct judgement_of_fury_t : public paladin_holy_attack_t
{
  judgement_of_fury_t( paladin_t* p, const spell_data_t* s ) :
    paladin_holy_attack_t( "judgement_of_fury", p, s )
  {
    background = true;
  }
};

// Judgement of the Crusader: target debuff, flat Holy damage taken (see jotc_bonus)
struct judgement_of_the_crusader_t : public paladin_holy_attack_t
{
  judgement_of_the_crusader_t( paladin_t* p, const spell_data_t* s ) :
    paladin_holy_attack_t( "judgement_of_the_crusader", p, s )
  {
    background = true;
    may_crit   = false;
  }

  void impact( action_state_t* s ) override
  {
    paladin_holy_attack_t::impact( s );

    if ( result_is_hit( s->result ) )
      td( s->target )->debuff.judgement_of_the_crusader->trigger();
  }
};

/////////////////////////////////////////////////////////////////////////////////////
//
// Melee
//
/////////////////////////////////////////////////////////////////////////////////////

struct paladin_melee_t : public paladin_attack_t
{
  paladin_melee_t( util::string_view name, paladin_t* player, weapon_t* w ) :
    paladin_attack_t( name, player, spell_data_t::nil() )
  {
    weapon            = w;
    weapon_multiplier = 1.0;
    base_execute_time = w->swing_time;
    school            = SCHOOL_PHYSICAL;
    trigger_gcd       = 0_ms;

    background = true;
    repeating  = true;
    may_glance = true;
    may_crit   = true;
    may_dodge  = true;
    may_parry  = true;
    special    = false;
  }

  // Seal of the Crusader: faster swings deal proportionally less damage (vanilla "Normalized wDPS"),
  // so white DPS is unchanged; the gain is attack power and more seal rolls.
  double action_multiplier() const override
  {
    double m = paladin_attack_t::action_multiplier();

    const buff_t* sotc = p()->buff.seal[ SEAL_CRUSADER ];
    if ( sotc->check() )
      m /= 1.0 + effect_at( &sotc->data(), 2 ).percent();

    return m;
  }

  void impact( action_state_t* s ) override
  {
    paladin_attack_t::impact( s );

    p()->trigger_seals( s );
    p()->trigger_vindication( s );

    // Judgement of the Crusader: "Your melee strikes will refresh the spell's duration."
    if ( result_is_hit( s->result ) )
    {
      buff_t* jotc = td( s->target )->debuff.judgement_of_the_crusader;
      if ( jotc->check() )
        jotc->refresh();
    }
  }
};

struct paladin_auto_attack_t : public paladin_attack_t
{
  paladin_auto_attack_t( paladin_t* p, util::string_view options_str ) :
    paladin_attack_t( "auto_attack", p, spell_data_t::nil() )
  {
    parse_options( options_str );
    trigger_gcd           = 0_ms;
    ignore_false_positive = true;

    if ( p->equipped_weapons[ SLOT_MAIN_HAND ].type == WEAPON_NONE )
    {
      background = true;
      return;
    }

    p->main_hand_attack = new paladin_melee_t( "melee", p, &p->equipped_weapons[ SLOT_MAIN_HAND ] );
  }

  void execute() override
  {
    player->main_hand_attack->schedule_execute();
  }

  bool ready() override
  {
    if ( player->is_moving() )
      return false;

    return player->main_hand_attack->execute_event == nullptr;
  }
};

/////////////////////////////////////////////////////////////////////////////////////
//
// Seals, Judgement, blessings
//
/////////////////////////////////////////////////////////////////////////////////////

struct seal_t : public paladin_self_spell_t
{
  seal_e seal;

  seal_t( util::string_view name, paladin_t* p, const spell_data_t* s, seal_e seal, util::string_view options_str ) :
    paladin_self_spell_t( name, p, s, options_str ), seal( seal )
  {}

  void execute() override
  {
    paladin_self_spell_t::execute();

    p()->activate_seal( seal, last_resource_cost );
  }
};

// Seal of Command is the 11 point Retribution talent: the talent teaches rank 1, the trainer the rest
struct seal_of_command_t : public seal_t
{
  seal_of_command_t( paladin_t* p, util::string_view options_str ) :
    seal_t( "seal_of_command", p,
            p->talent.seal_of_command.ok() ? max_rank_spell( p, seal_of_command_ranks ) : spell_data_t::not_found(),
            SEAL_COMMAND, options_str )
  {}
};

// Judgement (20271): off the global cooldown, 10 s, casts the active seal's Judgement. Forever: does not
// consume the seal.
struct judgement_t : public paladin_spell_t
{
  judgement_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "judgement", p, p->find_spell( judgement_id ) )
  {
    parse_options( options_str );

    may_miss = false;
    may_crit = false;
    // the spell has no Start Recovery Category data, it is off the global cooldown
    trigger_gcd = 0_ms;
    use_off_gcd = true;

    p->judgement_cd = cooldown;
  }

  bool ready() override
  {
    if ( p()->active_seal == SEAL_NONE || !p()->judgement_action[ p()->active_seal ] )
      return false;

    return paladin_spell_t::ready();
  }

  void execute() override
  {
    seal_e seal = p()->active_seal;

    paladin_spell_t::execute();

    p()->buff.swift_judgement->expire();

    p()->judgement_action[ seal ]->execute_on_target( target );

    // Sanctified Judgement: a chance to return part of the judged seal's mana cost
    const auto& sanctified = p()->talent.sanctified_judgement;
    if ( sanctified.ok() && rng().roll( sanctified->effectN( 1 ).percent() ) )
    {
      p()->resource_gain( RESOURCE_MANA, sanctified->effectN( 2 ).percent() * p()->seal_cost[ seal ],
                          p()->gain.sanctified_judgement, this );
    }
  }
};

struct blessing_of_might_t : public paladin_self_spell_t
{
  blessing_of_might_t( paladin_t* p, util::string_view options_str ) :
    paladin_self_spell_t( "blessing_of_might", p, max_rank_spell( p, blessing_of_might_ranks ), options_str )
  {}

  void execute() override
  {
    paladin_self_spell_t::execute();

    // one Blessing per paladin on a target
    p()->buff.blessing_of_kings->expire();
    p()->buff.blessing_of_might->trigger();
  }
};

struct blessing_of_kings_t : public paladin_self_spell_t
{
  blessing_of_kings_t( paladin_t* p, util::string_view options_str ) :
    paladin_self_spell_t( "blessing_of_kings", p, max_rank_spell( p, blessing_of_kings_ranks ), options_str )
  {}

  void execute() override
  {
    paladin_self_spell_t::execute();

    p()->buff.blessing_of_might->expire();
    p()->buff.blessing_of_kings->trigger();
  }
};

// Swift Judgement (16 point Protection talent): finishes the Judgement cooldown, the next Judgement is free
struct swift_judgement_t : public paladin_self_spell_t
{
  swift_judgement_t( paladin_t* p, util::string_view options_str ) :
    paladin_self_spell_t( "swift_judgement", p, talent_spell( p->talent.swift_judgement ), options_str )
  {
    if ( !p->talent.swift_judgement.ok() )
      background = true;
    trigger_gcd = 0_ms;
    use_off_gcd = true;
  }

  void execute() override
  {
    paladin_self_spell_t::execute();

    if ( p()->judgement_cd )
      p()->judgement_cd->reset( true );
    p()->buff.swift_judgement->trigger();
  }
};

// Divine Favor (16 point Holy talent): the next Holy Shock crits (effect #1, parsed per action)
struct divine_favor_t : public paladin_self_spell_t
{
  divine_favor_t( paladin_t* p, util::string_view options_str ) :
    paladin_self_spell_t( "divine_favor", p, talent_spell( p->talent.divine_favor ), options_str )
  {
    if ( !p->talent.divine_favor.ok() )
      background = true;
    trigger_gcd = 0_ms;
    use_off_gcd = true;
  }

  void execute() override
  {
    paladin_self_spell_t::execute();

    p()->buff.divine_favor->trigger();
  }
};

/////////////////////////////////////////////////////////////////////////////////////
//
// Abilities
//
/////////////////////////////////////////////////////////////////////////////////////

// Holy Strike (Forever baseline): Holy school weapon strike on the melee table. Effect #1 is normalized
// weapon damage + bonus (sp 0.429), effect #2 the weapon percentage. simc applies the percentage to weapon
// damage plus the bonus, like the server's effect order; the tooltip reads "50% weapon damage plus an
// additional X". 2026-10-16: verify in game which one is right.
// Sacred Arbiter (16 point Ret): +20% (parsed), and Holy Strike refreshes all Judgements on the target.
struct holy_strike_t : public paladin_holy_attack_t
{
  holy_strike_t( paladin_t* p, util::string_view options_str ) :
    paladin_holy_attack_t( "holy_strike", p, max_rank_spell( p, holy_strike_ranks ) )
  {
    parse_options( options_str );
  }

  void impact( action_state_t* s ) override
  {
    paladin_holy_attack_t::impact( s );

    p()->trigger_vindication( s );

    if ( result_is_hit( s->result ) && p()->talent.sacred_arbiter.ok() )
      p()->refresh_judgements( s->target );
  }
};

// Consecration (Forever: baseline at 20). A ground effect pulsing every second for 8 s; each pulse hits every
// enemy for effect #1 and the first 4 enemies also for effect #2 (sp 0.042).
struct consecration_tick_t : public paladin_spell_t
{
  double all_damage, first_damage;
  int first_targets;

  consecration_tick_t( paladin_t* p, const spell_data_t* tick, const spell_data_t* cast ) :
    paladin_spell_t( "consecration_tick", p, tick ), all_damage( 0 ), first_damage( 0 ), first_targets( 4 )
  {
    background = dual = true;
    direct_tick = true;
    aoe = -1;

    if ( tick->ok() )
    {
      all_damage   = tick->effectN( 1 ).average( p, p->level() );
      first_damage = tick->effectN( 2 ).average( p, p->level() );
      spell_power_mod.direct = tick->effectN( 2 ).sp_coeff();
    }
    if ( cast->ok() )
      first_targets = as<int>( effect_at( cast, 3 ).base_value() );

    base_dd_min = base_dd_max = 0.0;
  }

  double base_da_min( const action_state_t* s ) const override
  { return s->chain_target < first_targets ? all_damage + first_damage : all_damage; }

  double base_da_max( const action_state_t* s ) const override
  { return base_da_min( s ); }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  { return s->chain_target < first_targets ? paladin_spell_t::spell_direct_power_coefficient( s ) : 0.0; }
};

struct consecration_t : public paladin_spell_t
{
  action_t* tick;

  consecration_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "consecration", p, max_rank_spell( p, consecration_ranks ) ), tick( nullptr )
  {
    parse_options( options_str );

    may_miss = may_crit = false;
    harmful = true;

    tick = new consecration_tick_t( p, p->find_spell( max_rank( p, consecration_ranks ).tick_id ), &data() );
    add_child( tick );
  }

  void execute() override
  {
    paladin_spell_t::execute();

    timespan_t duration = data().duration();
    p()->consecration_expires = sim->current_time() + duration;

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
                                     .target( execute_state->target )
                                     .x( p()->x_position )
                                     .y( p()->y_position )
                                     .pulse_time( effect_at( &data(), 3 ).period() )
                                     .duration( duration )
                                     .action( tick ) );
  }
};

// Exorcism and Holy Wrath only work on Undead and Demon targets (spell descriptions); set target_race=.
bool undead_or_demon( const player_t* t )
{
  return t->race == RACE_UNDEAD || t->race == RACE_DEMON;
}

struct exorcism_t : public paladin_spell_t
{
  exorcism_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "exorcism", p, max_rank_spell( p, exorcism_ranks ) )
  {
    parse_options( options_str );
  }

  bool target_ready( player_t* t ) override
  {
    if ( !undead_or_demon( t ) )
      return false;

    return paladin_spell_t::target_ready( t );
  }
};

struct holy_wrath_t : public paladin_spell_t
{
  holy_wrath_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "holy_wrath", p, max_rank_spell( p, holy_wrath_ranks ) )
  {
    parse_options( options_str );
    aoe = -1;
  }

  bool target_ready( player_t* t ) override
  {
    if ( !undead_or_demon( t ) )
      return false;

    return paladin_spell_t::target_ready( t );
  }

  // only Undead and Demons are hit
  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    paladin_spell_t::available_targets( tl );
    range::erase_remove( tl, []( player_t* t ) { return !undead_or_demon( t ); } );
    return tl.size();
  }
};

struct hammer_of_wrath_t : public paladin_spell_t
{
  hammer_of_wrath_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "hammer_of_wrath", p, max_rank_spell( p, hammer_of_wrath_ranks ) )
  {
    parse_options( options_str );
  }

  bool target_ready( player_t* t ) override
  {
    if ( t->health_percentage() > hammer_of_wrath_health_pct )
      return false;

    return paladin_spell_t::target_ready( t );
  }
};

// Holy Shock (21 point Holy talent, Forever 10 s cooldown): the cast holds cost and cooldown, the damage is a
// separate spell per rank. Light's Vigil on the target: no cooldown, extra damage and a mana refund.
struct holy_shock_damage_t : public paladin_spell_t
{
  holy_shock_damage_t( paladin_t* p, const spell_data_t* s ) :
    paladin_spell_t( "holy_shock_damage", p, s )
  {
    background = dual = true;
  }

  void execute() override
  {
    paladin_spell_t::execute();

    p()->buff.divine_favor->expire();
  }
};

struct lights_vigil_damage_t : public paladin_spell_t
{
  lights_vigil_damage_t( paladin_t* p, const spell_data_t* s ) :
    paladin_spell_t( "lights_vigil_damage", p, s )
  {
    background = dual = true;
  }
};

struct holy_shock_t : public paladin_spell_t
{
  action_t* damage;
  action_t* vigil_damage;
  action_t* vigil_cast;

  holy_shock_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "holy_shock", p,
                     p->talent.holy_shock.ok() ? max_rank_spell( p, holy_shock_ranks ) : spell_data_t::not_found() ),
    damage( nullptr ), vigil_damage( nullptr ), vigil_cast( nullptr )
  {
    parse_options( options_str );

    may_miss = may_crit = false;

    damage = new holy_shock_damage_t( p, p->find_spell( max_rank( p, holy_shock_ranks ).damage_id ) );
    add_child( damage );

    if ( p->talent.lights_vigil.ok() )
    {
      vigil_damage = new lights_vigil_damage_t( p, p->find_spell( max_rank( p, lights_vigil_ranks ).damage_id ) );
      add_child( vigil_damage );
    }
  }

  void execute() override
  {
    paladin_spell_t::execute();

    damage->execute_on_target( target );

    buff_t* vigil = td( target )->debuff.lights_vigil;
    if ( vigil_damage && vigil->check() )
    {
      vigil->expire();
      cooldown->reset( false );
      vigil_damage->execute_on_target( target );

      action_t* cast = p()->find_action( "lights_vigil" );
      double refund  = cast ? effect_at( &cast->data(), 2 ).percent() * cast->last_resource_cost : 0.0;
      p()->resource_gain( RESOURCE_MANA, refund, p()->gain.lights_vigil, this );
    }
  }
};

// Light's Vigil (31 point Holy talent): marks the target for the next Holy Shock
struct lights_vigil_t : public paladin_spell_t
{
  lights_vigil_t( paladin_t* p, util::string_view options_str ) :
    paladin_spell_t( "lights_vigil", p,
                     p->talent.lights_vigil.ok() ? max_rank_spell( p, lights_vigil_ranks ) : spell_data_t::not_found() )
  {
    parse_options( options_str );

    may_miss = may_crit = false;
  }

  void impact( action_state_t* s ) override
  {
    paladin_spell_t::impact( s );

    td( s->target )->debuff.lights_vigil->trigger();
  }
};

/////////////////////////////////////////////////////////////////////////////////////
//
// Paladin
//
/////////////////////////////////////////////////////////////////////////////////////

const char* paladin_t::seal_name( seal_e s ) const
{
  switch ( s )
  {
    case SEAL_RIGHTEOUSNESS: return "righteousness";
    case SEAL_COMMAND:       return "command";
    case SEAL_FURY:          return "fury";
    case SEAL_CRUSADER:      return "the_crusader";
    default:                 return "none";
  }
}

// Only one seal at a time. Twist of Light (31 point Ret): replacing Seal of Command, Righteousness or Fury
// (or Justice, not modelled) with a different seal leaves an Echo of the old seal for the next melee swing.
void paladin_t::activate_seal( seal_e seal, double cost )
{
  seal_e old = active_seal;

  if ( old != SEAL_NONE && old != seal )
  {
    buff.seal[ old ]->expire();

    if ( talent.twist_of_light.ok() && buff.echo[ old ] )
      buff.echo[ old ]->trigger();
  }

  buff.seal[ seal ]->trigger();
  active_seal       = seal;
  seal_cost[ seal ] = cost;
}

// White swings apply the active seal and any Echo (consumed by the swing whether it lands or not).
// 2026-10-16: Twist of Light and Echo can't be tested on the level 30 beta; the Echo behaviour follows the
// talent text only.
void paladin_t::trigger_seals( action_state_t* s )
{
  bool hit = s->action->result_is_hit( s->result );

  for ( seal_e seal : { SEAL_RIGHTEOUSNESS, SEAL_COMMAND, SEAL_FURY } )
  {
    bool active = buff.seal[ seal ]->check();
    bool echo   = buff.echo[ seal ] && buff.echo[ seal ]->check();

    if ( echo )
      buff.echo[ seal ]->expire();

    if ( !hit || !( active || echo ) || !seal_proc[ seal ] )
      continue;

    if ( seal == SEAL_COMMAND )
    {
      if ( seal_of_command_icd->down() )
        continue;

      double chance = options.soc_ppm * equipped_weapons[ SLOT_MAIN_HAND ].swing_time.total_seconds() / 60.0;
      if ( !rng().roll( chance ) )
        continue;

      seal_of_command_icd->start();
    }

    seal_proc[ seal ]->execute_on_target( s->target );
  }
}

// Vengeance (Forever): a non-periodic crit grants a stack of +N% Physical and Holy damage for 30 s, 3 stacks
void paladin_t::trigger_vengeance( const action_state_t* s )
{
  if ( !talent.vengeance.ok() || s->result != RESULT_CRIT || s->action->treat_as_periodic )
    return;

  buff.vengeance->trigger();
}

// Vindication (Forever): damaging melee attacks raise the paladin's attack power by N% for 30 s (the target
// attack power reduction is not modelled). 100% proc chance in the data.
void paladin_t::trigger_vindication( const action_state_t* s )
{
  if ( !talent.vindication.ok() || !s->action->result_is_hit( s->result ) )
    return;

  buff.vindication->trigger();
}

// Sacred Arbiter: Holy Strike refreshes all Judgement effects on the target
void paladin_t::refresh_judgements( player_t* t )
{
  buff_t* jotc = get_target_data( t )->debuff.judgement_of_the_crusader;
  if ( jotc->check() )
    jotc->refresh();
}

double paladin_t::composite_spell_power( school_e school ) const
{
  double sp = player_t::composite_spell_power( school );

  // Champion of the Light: spell damage from a percentage of Intellect (aura 174, not in the core)
  if ( talent.champion_of_the_light.ok() )
    sp += talent.champion_of_the_light->effectN( 1 ).percent() * cache.intellect();

  return sp;
}

double paladin_t::composite_attack_power_multiplier() const
{
  double m = player_t::composite_attack_power_multiplier();

  m *= 1.0 + buff.vindication->check_value();

  return m;
}

double paladin_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( dbc::is_school( school, SCHOOL_PHYSICAL ) || dbc::is_school( school, SCHOOL_HOLY ) )
    m *= 1.0 + buff.vengeance->check_stack_value();

  return m;
}

double paladin_t::composite_melee_auto_attack_speed() const
{
  double s = player_t::composite_melee_auto_attack_speed();

  // Seal of the Crusader: +40% attack speed (effect #2, A9)
  if ( buff.seal[ SEAL_CRUSADER ]->check() )
    s /= 1.0 + effect_at( &buff.seal[ SEAL_CRUSADER ]->data(), 2 ).percent();

  return s;
}

void paladin_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  if ( c == CACHE_INTELLECT && talent.champion_of_the_light.ok() )
    invalidate_cache( CACHE_SPELL_POWER );
}

void paladin_t::init_base_stats()
{
  // Vanilla melee AP: Str * 2 + 3 * level - 20 (FOREVER_FOUNDATION.md)
  base.attack_power_per_strength = 2.0;

  player_t::init_base_stats();

  base.stats.attack_power = 3.0 * level() - 20.0;

  // 2026-10-16 ESTIMATE: vanilla paladin base crit (wowsims/classic base_stats.go: 0.7% melee, 3.336% spell),
  // crit from Agility / Intellect comes from the client tables. Verify against an addon export.
  base.attack_crit_chance = get_passive_player_value( get_passive_player_value( 0.007, "attack_crit" ), "all_crit" );
  base.spell_crit_chance  = get_passive_player_value( get_passive_player_value( 0.03336, "spell_crit" ), "all_crit" );

  base.dodge_per_agility = base.attack_crit_per_agility;
  base.parry             = get_passive_player_value( 0.05, "parry" );
  base.block             = get_passive_player_value( 0.05, "block" );
}

void paladin_t::init_spells()
{
  player_t::init_spells();

  talent.improved_seals        = forever::talent( this, "Improved Seals" );
  talent.divine_favor          = forever::talent( this, "Divine Favor" );
  talent.holy_shock            = forever::talent( this, "Holy Shock" );
  talent.consecrated_ground    = forever::talent( this, "Consecrated Ground" );
  talent.divine_precision      = forever::talent( this, "Divine Precision" );
  talent.holy_power            = forever::talent( this, "Holy Power" );
  talent.lights_vigil          = forever::talent( this, "Light's Vigil" );
  talent.swift_judgement       = forever::talent( this, "Swift Judgement" );
  talent.vindication           = forever::talent( this, "Vindication" );
  talent.sanctified_judgement  = forever::talent( this, "Sanctified Judgement" );
  talent.seal_of_command       = forever::talent( this, "Seal of Command" );
  talent.sacred_arbiter        = forever::talent( this, "Sacred Arbiter" );
  talent.vengeance             = forever::talent( this, "Vengeance" );
  talent.champion_of_the_light = forever::talent( this, "Champion of the Light" );
  talent.twist_of_light        = forever::talent( this, "Twist of Light" );

  parse_all_class_passives();
  parse_all_passive_talents();

  // Aura 174 is not in the core; Champion of the Light is applied in composite_spell_power
  if ( talent.champion_of_the_light.ok() )
    deregister_passive_spell( talent.champion_of_the_light.spell() );
}

void paladin_t::init_gains()
{
  player_t::init_gains();

  gain.sanctified_judgement = get_gain( "sanctified_judgement" );
  gain.lights_vigil         = get_gain( "lights_vigil" );
}

void paladin_t::create_buffs()
{
  player_t::create_buffs();

  auto make_seal = [ this ]( seal_e seal, const spell_data_t* s ) {
    buff_t* b = make_buff( this, fmt::format( "seal_of_{}", seal_name( seal ) ), s )
                  ->set_cooldown( 0_ms )
                  ->set_stack_change_callback( [ this, seal ]( buff_t*, int, int new_ ) {
                      if ( !new_ && active_seal == seal )
                        active_seal = SEAL_NONE;
                    } );
    buff.seal[ seal ] = b;
  };

  make_seal( SEAL_RIGHTEOUSNESS, max_rank_spell( this, seal_of_righteousness_ranks ) );
  make_seal( SEAL_COMMAND, talent.seal_of_command.ok() ? max_rank_spell( this, seal_of_command_ranks )
                                                       : spell_data_t::not_found() );
  make_seal( SEAL_FURY, max_rank_spell( this, seal_of_fury_ranks ) );

  // Seal of the Crusader: attack power (effect #1, A99) and attack speed (effect #2)
  const spell_data_t* sotc = max_rank_spell( this, seal_of_the_crusader_ranks );
  buff.seal[ SEAL_CRUSADER ] = make_buff<stat_buff_t>( this, "seal_of_the_crusader", sotc )
                                 ->set_cooldown( 0_ms )
                                 ->add_invalidate( CACHE_AUTO_ATTACK_SPEED )
                                 ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
                                     if ( !new_ && active_seal == SEAL_CRUSADER )
                                       active_seal = SEAL_NONE;
                                   } );

  // Twist of Light Echoes. 2026-10-16: the Echo buff id is unknown; it lasts until the next swing.
  for ( seal_e seal : { SEAL_RIGHTEOUSNESS, SEAL_COMMAND, SEAL_FURY } )
  {
    buff.echo[ seal ] = make_buff( this, fmt::format( "echo_of_{}", seal_name( seal ) ) )
                          ->set_duration( buff.seal[ seal ]->buff_duration() );
  }

  buff.blessing_of_might = make_buff<stat_buff_t>( this, "blessing_of_might", max_rank_spell( this, blessing_of_might_ranks ) );

  buff.blessing_of_kings = make_buff( this, "blessing_of_kings", max_rank_spell( this, blessing_of_kings_ranks ) )
                             ->set_default_value_from_effect( 1 )
                             ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH )
                             ->set_pct_buff_type( STAT_PCT_BUFF_AGILITY )
                             ->set_pct_buff_type( STAT_PCT_BUFF_STAMINA )
                             ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT )
                             ->set_pct_buff_type( STAT_PCT_BUFF_SPIRIT );

  // Vengeance: the per stack value is the talent rank's (1/2/3%), the buff spell carries duration and stacks
  buff.vengeance = make_buff( this, "vengeance", talent.vengeance.ok() ? find_spell( vengeance_buff_id )
                                                                       : spell_data_t::not_found() )
                     ->set_default_value( talent.vengeance->effectN( 1 ).percent() )
                     ->set_schools_from_effect( 1 )
                     ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.vindication = make_buff( this, "vindication", talent.vindication.ok() ? find_spell( vindication_buff_id )
                                                                             : spell_data_t::not_found() )
                       ->set_default_value( talent.vindication->effectN( 1 ).percent() )
                       ->add_invalidate( CACHE_ATTACK_POWER );

  buff.divine_favor = make_buff( this, "divine_favor", talent_spell( talent.divine_favor ) )
                        ->set_cooldown( 0_ms );

  buff.swift_judgement = make_buff( this, "swift_judgement", talent_spell( talent.swift_judgement ) )
                           ->set_cooldown( 0_ms );
}

void paladin_t::create_actions()
{
  seal_of_command_icd = get_cooldown( "seal_of_command_icd" );

  // Seal procs and Judgements, from the best known rank of each seal
  const spell_data_t* sor  = max_rank_spell( this, seal_of_righteousness_ranks );
  const spell_data_t* soc  = talent.seal_of_command.ok() ? max_rank_spell( this, seal_of_command_ranks )
                                                         : spell_data_t::not_found();
  const spell_data_t* sof  = max_rank_spell( this, seal_of_fury_ranks );
  const spell_data_t* sotc = max_rank_spell( this, seal_of_the_crusader_ranks );

  auto judgement_spell = [ this ]( const spell_data_t* seal ) {
    return seal->ok() ? find_spell( as<unsigned>( effect_at( seal, 3 ).base_value() ) ) : spell_data_t::not_found();
  };

  if ( sor->ok() )
  {
    seal_proc[ SEAL_RIGHTEOUSNESS ] = new seal_of_righteousness_proc_t( this, sor );
    const spell_data_t* jor = judgement_spell( sor );
    if ( jor->ok() )
      judgement_action[ SEAL_RIGHTEOUSNESS ] = new judgement_of_righteousness_t( this, jor );
  }

  if ( soc->ok() )
  {
    seal_proc[ SEAL_COMMAND ] = new seal_of_command_proc_t( this, soc );
    seal_of_command_icd->duration = soc->internal_cooldown();

    // the seal names a dummy whose effect #1 names the damage spell
    const spell_data_t* joc = judgement_spell( soc );
    if ( joc->ok() )
      judgement_action[ SEAL_COMMAND ] =
        new judgement_of_command_t( this, find_spell( as<unsigned>( joc->effectN( 1 ).base_value() ) ) );
  }

  if ( sof->ok() )
  {
    seal_proc[ SEAL_FURY ] = new seal_of_fury_proc_t( this, sof );
    const spell_data_t* jof = judgement_spell( sof );
    if ( jof->ok() )
      judgement_action[ SEAL_FURY ] = new judgement_of_fury_t( this, jof );
  }

  if ( sotc->ok() )
  {
    const spell_data_t* jotc = judgement_spell( sotc );
    if ( jotc->ok() )
      judgement_action[ SEAL_CRUSADER ] = new judgement_of_the_crusader_t( this, jotc );
  }

  player_t::create_actions();
}

void paladin_t::create_options()
{
  player_t::create_options();

  add_option( opt_float( "paladin.soc_ppm", options.soc_ppm, 0.0, 60.0 ) );
}

void paladin_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  options = debug_cast<paladin_t*>( source )->options;
}

void paladin_t::reset()
{
  player_t::reset();

  active_seal          = SEAL_NONE;
  consecration_expires = timespan_t::min();
  seal_cost.fill( 0.0 );
}

action_t* paladin_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "auto_attack" ) return new paladin_auto_attack_t( this, options_str );

  if ( name == "seal_of_righteousness" )
    return new seal_t( name, this, max_rank_spell( this, seal_of_righteousness_ranks ), SEAL_RIGHTEOUSNESS, options_str );
  if ( name == "seal_of_command" ) return new seal_of_command_t( this, options_str );
  if ( name == "seal_of_fury" )
    return new seal_t( name, this, max_rank_spell( this, seal_of_fury_ranks ), SEAL_FURY, options_str );
  if ( name == "seal_of_the_crusader" )
    return new seal_t( name, this, max_rank_spell( this, seal_of_the_crusader_ranks ), SEAL_CRUSADER, options_str );
  if ( name == "judgement" ) return new judgement_t( this, options_str );

  if ( name == "blessing_of_might" ) return new blessing_of_might_t( this, options_str );
  if ( name == "blessing_of_kings" ) return new blessing_of_kings_t( this, options_str );

  if ( name == "holy_strike" ) return new holy_strike_t( this, options_str );
  if ( name == "consecration" ) return new consecration_t( this, options_str );
  if ( name == "exorcism" ) return new exorcism_t( this, options_str );
  if ( name == "holy_wrath" ) return new holy_wrath_t( this, options_str );
  if ( name == "hammer_of_wrath" ) return new hammer_of_wrath_t( this, options_str );
  if ( name == "holy_shock" ) return new holy_shock_t( this, options_str );
  if ( name == "lights_vigil" ) return new lights_vigil_t( this, options_str );
  if ( name == "divine_favor" ) return new divine_favor_t( this, options_str );
  if ( name == "swift_judgement" ) return new swift_judgement_t( this, options_str );

  return player_t::create_action( name, options_str );
}

void paladin_t::init_action_list()
{
  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  clear_action_priority_lists();
  paladin_apl::paladin( this );
  use_default_action_list = true;
  player_t::init_action_list();
}

// PALADIN MODULE INTERFACE =================================================

struct paladin_module_t : public module_t
{
  paladin_module_t() : module_t( PALADIN ) {}

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new paladin_t( sim, name, r );
    return p;
  }

  bool valid() const override
  {
    return true;
  }

  void register_actor_initializers( sim_t* ) const override {}

  void register_hotfixes() const override {}
};
}  // UNNAMED NAMESPACE

const module_t* module_t::paladin()
{
  static paladin_module_t m;
  return &m;
}

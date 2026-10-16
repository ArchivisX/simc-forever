// ==========================================================================
// Priest Sim File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================
//
// WoW: Forever priest (client 1.60.1.70425). DPS only: Shadow, and a Smite / Holy Fire / Penance build.
// Healing is not modelled yet. Values come from spell data; notes in FOREVER_PRIEST.md at the repo root.

#include "simulationcraft.hpp"
#include "action/parse_effects.hpp"

#include "class_modules/apl/priest.hpp"
#include "class_modules/forever_helpers.hpp"

namespace priestspace
{
using forever::spell_rank_t;
using forever::max_rank;

// ==========================================================================
// Spell ranks (trainer ids; Season of Discovery copies such as SW:D 401955 are left out)
// ==========================================================================

constexpr std::array smite_ranks {
  spell_rank_t{ 585 },    // 1
  spell_rank_t{ 591 },    // 6
  spell_rank_t{ 598 },    // 14
  spell_rank_t{ 984 },    // 22
  spell_rank_t{ 1004 },   // 30
  spell_rank_t{ 6060 },   // 38
  spell_rank_t{ 10933 },  // 46
  spell_rank_t{ 10934 },  // 54
};

constexpr std::array holy_fire_ranks {
  spell_rank_t{ 14914 },  // 20
  spell_rank_t{ 15262 },  // 24
  spell_rank_t{ 15263 },  // 30
  spell_rank_t{ 15264 },  // 36
  spell_rank_t{ 15265 },  // 42
  spell_rank_t{ 15266 },  // 48
  spell_rank_t{ 15267 },  // 54
  spell_rank_t{ 15261 },  // 60
};

constexpr std::array mind_blast_ranks {
  spell_rank_t{ 8092 },   // 10
  spell_rank_t{ 8102 },   // 16
  spell_rank_t{ 8103 },   // 22
  spell_rank_t{ 8104 },   // 28
  spell_rank_t{ 8105 },   // 34
  spell_rank_t{ 8106 },   // 40
  spell_rank_t{ 10945 },  // 46
  spell_rank_t{ 10946 },  // 52
  spell_rank_t{ 10947 },  // 58
};

constexpr std::array shadow_word_pain_ranks {
  spell_rank_t{ 589 },    // 4
  spell_rank_t{ 594 },    // 10
  spell_rank_t{ 970 },    // 18
  spell_rank_t{ 992 },    // 26
  spell_rank_t{ 2767 },   // 34
  spell_rank_t{ 10892 },  // 42
  spell_rank_t{ 10893 },  // 50
  spell_rank_t{ 10894 },  // 58
};

// Shadow 11 point talent
constexpr std::array mind_flay_ranks {
  spell_rank_t{ 15407 },  // 20
  spell_rank_t{ 17311 },  // 28
  spell_rank_t{ 17312 },  // 36
  spell_rank_t{ 17313 },  // 44
  spell_rank_t{ 17314 },  // 52
  spell_rank_t{ 18807 },  // 60
};

// Forever: baseline for every race from level 20, 24 s cooldown
constexpr std::array devouring_plague_ranks {
  spell_rank_t{ 2944 },   // 20
  spell_rank_t{ 19276 },  // 28
  spell_rank_t{ 19277 },  // 36
  spell_rank_t{ 19278 },  // 44
  spell_rank_t{ 19279 },  // 52
  spell_rank_t{ 19280 },  // 60
};

// Forever baseline spell (the SoD 401955 copy is not obtainable)
constexpr std::array shadow_word_death_ranks {
  spell_rank_t{ 1309595 },  // 32
  spell_rank_t{ 1309633 },  // 40
  spell_rank_t{ 1309635 },  // 48
  spell_rank_t{ 1309636 },  // 56
};

// Holy 11 point talent; the damage spells (the party heal is a separate spell per rank)
constexpr std::array holy_nova_ranks {
  spell_rank_t{ 15237 },  // 20
  spell_rank_t{ 15430 },  // 28
  spell_rank_t{ 15431 },  // 36
  spell_rank_t{ 27799 },  // 44
  spell_rank_t{ 27800 },  // 52
  spell_rank_t{ 27801 },  // 60
};

// Night Elf racial
constexpr std::array starshards_ranks {
  spell_rank_t{ 10797 },  // 10
  spell_rank_t{ 19296 },  // 18
  spell_rank_t{ 19299 },  // 26
  spell_rank_t{ 19302 },  // 34
  spell_rank_t{ 19303 },  // 42
  spell_rank_t{ 19304 },  // 50
  spell_rank_t{ 19305 },  // 58
};

// Dwarf racial (humanoid targets only)
constexpr std::array chastise_ranks {
  spell_rank_t{ 1277331 },  // 20
  spell_rank_t{ 1277332 },  // 30
  spell_rank_t{ 1277333 },  // 40
  spell_rank_t{ 1277334 },  // 50
  spell_rank_t{ 1277335 },  // 60
};

constexpr std::array power_word_fortitude_ranks {
  spell_rank_t{ 1243 },   // 1
  spell_rank_t{ 1244 },   // 12
  spell_rank_t{ 1245 },   // 24
  spell_rank_t{ 2791 },   // 36
  spell_rank_t{ 10937 },  // 48
  spell_rank_t{ 10938 },  // 60
};

constexpr std::array inner_fire_ranks {
  spell_rank_t{ 588 },    // 12
  spell_rank_t{ 7128 },   // 20
  spell_rank_t{ 602 },    // 30
  spell_rank_t{ 1006 },   // 40
  spell_rank_t{ 10951 },  // 50
  spell_rank_t{ 10952 },  // 60
};

// Forever: baseline from level 30
constexpr std::array divine_spirit_ranks {
  spell_rank_t{ 14752 },  // 30
  spell_rank_t{ 14818 },  // 40
  spell_rank_t{ 14819 },  // 50
  spell_rank_t{ 27841 },  // 60
};

// Penance (Discipline 21 point talent): the cast is a dummy, the channel ticks a damage bolt instantly and
// every second for 2 seconds (3 bolts). The bolt spells are flagged AcquireMethod 3 in SkillLineAbility,
// but Forever's Penance triggers them.
struct penance_rank_t
{
  unsigned spell_id;    // the cast (cost, cooldown)
  unsigned channel_id;  // the channel (duration, tick period)
  unsigned bolt_id;     // the damage bolt
};

constexpr std::array penance_ranks {
  penance_rank_t{ 402174, 402261, 402284 },     // 30
  penance_rank_t{ 1240720, 1240734, 1240727 },  // 40
  penance_rank_t{ 1240721, 1240736, 1240730 },  // 50
  penance_rank_t{ 1316995, 1316994, 1316993 },  // 60
};

struct priest_t;

struct priest_td_t : public actor_target_data_t
{
  struct dots_t
  {
    dot_t* shadow_word_pain = nullptr;
    dot_t* holy_fire = nullptr;
    dot_t* devouring_plague = nullptr;
  } dots;

  struct debuffs_t
  {
    buff_t* shadow_weaving = nullptr;
    buff_t* vampiric_embrace = nullptr;
  } debuff;

  priest_td_t( player_t* target, priest_t* p );
};

struct priest_t final : public parse_player_effects_t
{
  priest_t( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) :
    parse_player_effects_t( sim, PRIEST, name, r ) {}

  resource_e primary_resource() const override { return RESOURCE_MANA; }

  std::string default_potion() const override   { return priest_apl::potion( this ); }
  std::string default_flask() const override    { return priest_apl::flask( this ); }
  std::string default_food() const override     { return priest_apl::food( this ); }

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
  void init_action_list() override;
  void init_base_stats() override;
  void init_spells() override;
  void create_buffs() override;
  void create_options() override;
  void copy_from( player_t* source ) override;
  void init_finished() override;
  void parse_player_effects();

  double composite_spell_power( school_e school ) const override;
  double composite_attribute_multiplier( attribute_e attr ) const override;
  double composite_mana_regen_while_casting() const override;
  void invalidate_cache( cache_e c ) override;

  priest_td_t* get_target_data( player_t* target ) const override
  {
    priest_td_t*& td = target_data[ target ];
    if ( !td )
      td = new priest_td_t( target, const_cast<priest_t*>( this ) );
    return td;
  }

  // Talent ability: the talent node gates it, the highest trainer rank the player knows is used. Below the
  // first trainer rank's level fall back to the talent spell itself (Forever talent rows unlock by points, not
  // by the classic spell level, see the shaman Stormstrike note).
  template <typename RANKS>
  const spell_data_t* talent_ability( const player_talent_t& t, const RANKS& ranks ) const
  {
    if ( t.rank() == 0 )
      return spell_data_t::not_found();

    if ( const spell_data_t* s = forever::max_rank_spell( this, ranks ); s->ok() )
      return s;

    return dbc::find_spell( this, t.trait()->id_spell );
  }

  struct talents_t
  {
    // Discipline
    player_talent_t power_in_light;
    player_talent_t wand_specialization;
    player_talent_t twin_disciplines;
    player_talent_t holy_precision;
    player_talent_t mental_strength;
    player_talent_t mental_agility;
    player_talent_t inner_focus;        // 11
    player_talent_t meditation;
    player_talent_t penance;            // 21
    player_talent_t power_infusion;     // 31

    // Holy
    player_talent_t holy_specialization;
    player_talent_t divine_fury;
    player_talent_t holy_nova;          // 11
    player_talent_t searing_light;
    player_talent_t spiritual_guidance;

    // Shadow
    player_talent_t shadow_focus;
    player_talent_t spirit_tap;
    player_talent_t improved_mind_blast;
    player_talent_t improved_shadow_word_pain;
    player_talent_t mind_flay;          // 11
    player_talent_t improved_mind_flay;
    player_talent_t dark_inspiration;
    player_talent_t vampiric_embrace;   // 16
    player_talent_t shadow_weaving;
    player_talent_t devouring_contagion;
    player_talent_t early_demise;
    player_talent_t darkness;
    player_talent_t shadowform;         // 31
  } talent;

  struct spells_t
  {
    const spell_data_t* shadow_weaving   = spell_data_t::not_found();  // 15258, the debuff
    const spell_data_t* dark_inspiration = spell_data_t::not_found();  // 1326168, the buff
    const spell_data_t* spirit_tap       = spell_data_t::not_found();  // 15271, the buff
    const spell_data_t* holy_purpose     = spell_data_t::not_found();  // 1284536
  } spell;

  struct buffs_t
  {
    buff_t* shadowform = nullptr;
    buff_t* inner_focus = nullptr;
    buff_t* power_infusion = nullptr;
    buff_t* dark_inspiration = nullptr;
    buff_t* holy_purpose = nullptr;
    buff_t* inner_fire = nullptr;
    buff_t* power_word_fortitude = nullptr;
    buff_t* divine_spirit = nullptr;
  } buff;

  struct options_t
  {
    // Spirit Tap only triggers on kills (or when a Vampiric Embrace target dies), which a single target fight
    // never has. This is the average fraction of the fight it is up, applied as a permanent partial buff.
    double spirit_tap_uptime = 0.0;

    // Wand as min:max:speed[:school] (the engine has no ranged slot yet), see shoot_t
    std::string wand;
  } options;

  target_specific_t<priest_td_t> target_data;

  void trigger_shadow_weaving( action_state_t* s );
};

priest_td_t::priest_td_t( player_t* target, priest_t* p ) : actor_target_data_t( target, p )
{
  dots.shadow_word_pain = target->get_dot( "shadow_word_pain", p );
  dots.holy_fire        = target->get_dot( "holy_fire", p );
  dots.devouring_plague = target->get_dot( "devouring_plague", p );

  // Forever: +2% Shadow damage taken from the priest per stack (vanilla 3%), caster only. See
  // priest_t::parse_player_effects.
  debuff.shadow_weaving = make_buff( *this, "shadow_weaving", p->spell.shadow_weaving );

  debuff.vampiric_embrace = make_buff( *this, "vampiric_embrace", p->talent.vampiric_embrace.spell() );
}

struct priest_module_t final : public module_t
{
  priest_module_t() : module_t( PRIEST ) {}

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    auto p = new priest_t( sim, name, r );
    return p;
  }

  bool valid() const override
  {
    return true;
  }

  void register_hotfixes() const override {}

  void register_actor_initializers( sim_t* ) const override {}
};

// ==========================================================================
// Priest action base
// ==========================================================================

template <class Base>
struct priest_action_t : public parse_action_effects_t<Base>
{
private:
  using ab = parse_action_effects_t<Base>;

public:
  using base_t = priest_action_t<Base>;

  template <typename... Args>
  priest_action_t( Args&&... args ) : ab( std::forward<Args>( args )... )
  {
    // Shadow Focus / Holy Precision: classic spell hit modifiers (property 16). The player-level passive parse
    // has no hit field, so they are parsed per action.
    if ( p()->talent.shadow_focus.ok() )
      ab::parse_effects( p()->talent.shadow_focus.spell(), PARSE_PASSIVE );
    if ( p()->talent.holy_precision.ok() )
      ab::parse_effects( p()->talent.holy_precision.spell(), PARSE_PASSIVE );

    // Shadowform effect #5: +100% critical strike damage bonus for the whitelisted Shadow spells. The damage
    // part (A79) is parsed in priest_t::parse_player_effects, the cost part (A72) in cost_pct_multiplier.
    ab::parse_effects( p()->buff.shadowform );

    // Inner Focus: next spell is free and, if it is not a periodic spell, gets +25% crit (Oct 1 2026 beta
    // notes: no longer boosts periodic crit). Consumed by the next affected spell.
    bool has_direct = false;
    for ( const auto& eff : this->data().effects() )
      if ( eff.type() == E_SCHOOL_DAMAGE )
        has_direct = true;
    ab::parse_effects( p()->buff.inner_focus, CONSUME_BUFF, has_direct ? 0U : 0b10U );
  }

  priest_t* p()
  { return debug_cast<priest_t*>( this->player ); }

  const priest_t* p() const
  { return debug_cast<const priest_t*>( this->player ); }

  priest_td_t* td( player_t* t ) const
  { return p()->get_target_data( t ); }

  // Shadowform effect #4: Modify Power Cost% (aura 72) -50% for Shadow spells, not a spell modifier
  double cost_pct_multiplier() const override
  {
    double m = ab::cost_pct_multiplier();

    if ( p()->buff.shadowform->check() && dbc::is_school( this->get_school(), SCHOOL_SHADOW ) )
      m *= 1.0 + p()->buff.shadowform->data().effectN( 4 ).percent();

    return m;
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    p()->trigger_shadow_weaving( s );
  }
};

using priest_spell_t = priest_action_t<spell_t>;

// Self buffs can't miss
struct priest_buff_spell_t : public priest_spell_t
{
  buff_t* buff;

  priest_buff_spell_t( util::string_view n, priest_t* p, const spell_data_t* s, buff_t* b,
                       util::string_view options_str ) :
    priest_spell_t( n, p, s ), buff( b )
  {
    parse_options( options_str );

    harmful  = false;
    may_miss = false;
    may_crit = false;
    target   = p;
  }

  void execute() override
  {
    priest_spell_t::execute();

    buff->trigger();
  }

  bool ready() override
  {
    if ( buff->check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Shadow Weaving: each landed Shadow damage spell has a 33/67/100% chance to add a stack of the caster-only
// debuff (Proc Flags: Magic Hostile Spell, i.e. on cast landing, not per DoT tick). Oct 1 2026 beta notes:
// "can no longer fail to apply", read as no extra resist roll.
void priest_t::trigger_shadow_weaving( action_state_t* s )
{
  if ( !talent.shadow_weaving.ok() )
    return;

  const action_t* a = s->action;
  if ( !a->harmful || a->background || !dbc::is_school( a->get_school(), SCHOOL_SHADOW ) )
    return;

  if ( !a->result_is_hit( s->result ) )
    return;

  if ( rng().roll( talent.shadow_weaving->effectN( 1 ).percent() ) )
    get_target_data( s->target )->debuff.shadow_weaving->trigger();
}

// ==========================================================================
// Shadow
// ==========================================================================

// Shadow Word: Pain. Improved Shadow Word: Pain adds +3 s (one tick) per rank through the passive parse.
struct shadow_word_pain_t : public priest_spell_t
{
  shadow_word_pain_t( priest_t* p, util::string_view options_str ) :
    priest_spell_t( "shadow_word_pain", p, forever::max_rank_spell( p, shadow_word_pain_ranks ) )
  {
    parse_options( options_str );
  }
};

// Mind Blast: Dark Inspiration on cast
struct mind_blast_t : public priest_spell_t
{
  mind_blast_t( priest_t* p, util::string_view options_str ) :
    priest_spell_t( "mind_blast", p, forever::max_rank_spell( p, mind_blast_ranks ) )
  {
    parse_options( options_str );

    // Improved Mind Blast (A107 cooldown -0.5 s/rank) modifies the spell's cooldown, but Mind Blast also has an
    // 8 s category cooldown, which action_t::init_spell_data prefers and no talent touches. Use the modified one.
    if ( data().cooldown() > 0_ms && data().cooldown() < cooldown->duration )
      cooldown->duration = data().cooldown();
  }

  void execute() override
  {
    priest_spell_t::execute();

    p()->buff.dark_inspiration->trigger();
  }
};

// Mind Flay (Shadow 11 point talent): 3 s channel, 1 s ticks.
// Improved Mind Flay: "Each instance of damage of Mind Flay deals X% more damage than its previous instance".
// ESTIMATE: read literally as compounding (1, 1.2, 1.44 at 2/2); a linear 1, 1.2, 1.4 is the alternative
// (spec V5). Dark Inspiration triggers only on a fully channeled Mind Flay.
struct mind_flay_t : public priest_spell_t
{
  double ramp;

  mind_flay_t( priest_t* p, util::string_view options_str ) :
    priest_spell_t( "mind_flay", p, p->talent_ability( p->talent.mind_flay, mind_flay_ranks ) ),
    ramp( p->talent.improved_mind_flay.ok() ? p->talent.improved_mind_flay->effectN( 1 ).percent() : 0.0 )
  {
    parse_options( options_str );

    channeled = true;
  }

  bool verify_actor_level() const override
  { return true; }

  double calculate_tick_amount( action_state_t* s, double dot_multiplier ) const override
  {
    if ( ramp > 0 )
    {
      if ( dot_t* d = find_dot( s->target ); d && d->current_tick > 1 )
        dot_multiplier *= std::pow( 1.0 + ramp, d->current_tick - 1 );
    }

    return priest_spell_t::calculate_tick_amount( s, dot_multiplier );
  }

  void last_tick( dot_t* d ) override
  {
    bool full_channel = d->current_tick >= d->num_ticks();

    priest_spell_t::last_tick( d );

    if ( full_channel )
      p()->buff.dark_inspiration->trigger();
  }
};

// Devouring Plague: periodic health leech, 8 ticks x 3 s, 24 s cooldown (Forever). Can crit (Oct 1 2026 notes,
// "Periodic Can Crit" in data). The heal to the priest is not modelled. Devouring Contagion's cost reduction
// comes from the passive parse; its spread on target death is not modelled (single target).
struct devouring_plague_t : public priest_spell_t
{
  devouring_plague_t( priest_t* p, util::string_view options_str ) :
    priest_spell_t( "devouring_plague", p, forever::max_rank_spell( p, devouring_plague_ranks ) )
  {
    parse_options( options_str );
  }
};

// Shadow Word: Death (Forever baseline, level 32): instant, 18 s cooldown.
// Effect #3: the priest takes 10% of the damage attempted if the target survives. Not modelled (self damage
// has no DPS effect here and simc actors would not heal it back).
// Early Demise: +15/30% crit chance against targets at or below 20% health.
struct shadow_word_death_t : public priest_spell_t
{
  double early_demise_crit;
  double early_demise_health;

  shadow_word_death_t( priest_t* p, util::string_view options_str ) :
    priest_spell_t( "shadow_word_death", p, forever::max_rank_spell( p, shadow_word_death_ranks ) ),
    early_demise_crit( p->talent.early_demise.ok() ? p->talent.early_demise->effectN( 1 ).percent() : 0.0 ),
    early_demise_health( p->talent.early_demise.ok() ? p->talent.early_demise->effectN( 2 ).base_value() : 0.0 )
  {
    parse_options( options_str );
  }

  double composite_target_crit_chance( player_t* t ) const override
  {
    double c = priest_spell_t::composite_target_crit_chance( t );

    if ( early_demise_crit > 0 && t->health_percentage() <= early_demise_health )
      c += early_demise_crit;

    return c;
  }
};

// Vampiric Embrace (Shadow 16 point talent): 30 s debuff, 30 s cooldown in 70425. The party heal is not
// modelled; it only costs a GCD and mana here.
struct vampiric_embrace_t : public priest_spell_t
{
  vampiric_embrace_t( priest_t* p, util::string_view options_str ) :
    priest_spell_t( "vampiric_embrace", p, p->talent.vampiric_embrace.ok() ? p->talent.vampiric_embrace.spell()
                                                                         : spell_data_t::not_found() )
  {
    parse_options( options_str );

    may_crit = false;
  }

  bool verify_actor_level() const override
  { return true; }

  void impact( action_state_t* s ) override
  {
    priest_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
      td( s->target )->debuff.vampiric_embrace->trigger();
  }
};

// Shadowform (Shadow 31 point talent): stance buff, cast precombat
struct shadowform_t : public priest_buff_spell_t
{
  shadowform_t( priest_t* p, util::string_view options_str ) :
    priest_buff_spell_t( "shadowform", p, p->talent.shadowform.ok() ? p->talent.shadowform.spell()
                                                                    : spell_data_t::not_found(),
                         p->buff.shadowform, options_str )
  {}

  bool verify_actor_level() const override
  { return true; }
};

// ==========================================================================
// Holy / Discipline damage
// ==========================================================================

// Power in Light: +2..10% Smite and Penance damage to targets with the priest's Holy Fire or Shadow Word: Pain
double power_in_light_multiplier( const priest_t* p, priest_td_t* td )
{
  if ( !p->talent.power_in_light.ok() )
    return 1.0;

  if ( td->dots.shadow_word_pain->is_ticking() || td->dots.holy_fire->is_ticking() )
    return 1.0 + p->talent.power_in_light->effectN( 1 ).percent();

  return 1.0;
}

// Smite: Divine Fury (cast time), Holy Specialization (crit) and Twin Disciplines come from the passive parse
struct smite_t : public priest_spell_t
{
  smite_t( priest_t* p, util::string_view options_str ) :
    priest_spell_t( "smite", p, forever::max_rank_spell( p, smite_ranks ) )
  {
    parse_options( options_str );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    return priest_spell_t::composite_target_multiplier( t ) * power_in_light_multiplier( p(), td( t ) );
  }
};

// Holy Fire: direct damage + 5 ticks every 2 s. Searing Light: each tick has a 5/10% chance to make the next
// Holy Nova free (Holy Purpose); its +Holy damage part comes from the passive parse.
struct holy_fire_t : public priest_spell_t
{
  holy_fire_t( priest_t* p, util::string_view options_str ) :
    priest_spell_t( "holy_fire", p, forever::max_rank_spell( p, holy_fire_ranks ) )
  {
    parse_options( options_str );
  }

  void tick( dot_t* d ) override
  {
    priest_spell_t::tick( d );

    if ( p()->talent.searing_light.ok() && rng().roll( p()->talent.searing_light->effectN( 2 ).percent() ) )
      p()->buff.holy_purpose->trigger();
  }
};

// Holy Nova (Holy 11 point talent): point blank AoE damage; the party heal is not modelled
struct holy_nova_t : public priest_spell_t
{
  holy_nova_t( priest_t* p, util::string_view options_str ) :
    priest_spell_t( "holy_nova", p, p->talent_ability( p->talent.holy_nova, holy_nova_ranks ) )
  {
    parse_options( options_str );

    aoe = -1;

    parse_effects( p->buff.holy_purpose, CONSUME_BUFF );
  }

  bool verify_actor_level() const override
  { return true; }
};

// Penance bolt: one per channel tick
struct penance_bolt_t : public priest_spell_t
{
  penance_bolt_t( priest_t* p, const spell_data_t* s ) : priest_spell_t( "penance_bolt", p, s )
  {
    background = dual = true;
    direct_tick = true;
  }

  bool verify_actor_level() const override
  { return true; }

  double composite_target_multiplier( player_t* t ) const override
  {
    return priest_spell_t::composite_target_multiplier( t ) * power_in_light_multiplier( p(), td( t ) );
  }
};

// Penance (Discipline 21 point talent): 2 s channel with a bolt instantly and every second (3 bolts)
struct penance_t : public priest_spell_t
{
  penance_t( priest_t* p, util::string_view options_str ) :
    priest_spell_t( "penance", p, p->talent.penance.rank() > 0
                                    ? dbc::find_spell( p, max_rank_penance( p ).spell_id )
                                    : spell_data_t::not_found() )
  {
    parse_options( options_str );

    if ( !data().ok() )
      return;

    auto rank = max_rank_penance( p );
    const spell_data_t* channel = dbc::find_spell( p, rank.channel_id );

    channeled      = true;
    tick_zero      = true;
    hasted_ticks   = false;
    may_crit       = false;
    may_miss       = false;
    dot_duration   = channel->duration();
    base_tick_time = channel->effectN( 2 ).period();

    tick_action = new penance_bolt_t( p, dbc::find_spell( p, rank.bolt_id ) );
    add_child( tick_action );
  }

  // Highest rank by player level; rank 1 when the talent is taken below its spell level 30
  static penance_rank_t max_rank_penance( const priest_t* p )
  {
    auto rank = max_rank( p, penance_ranks );
    return rank.spell_id ? rank : penance_ranks[ 0 ];
  }

  bool verify_actor_level() const override
  { return true; }
};

// ==========================================================================
// Wand
// ==========================================================================

// Shoot (5019, auto repeat): the filler when out of mana. Modelled as a "cast" of one wand swing: weapon damage,
// no attack power or spell power, no GCD, no mana (so the five second rule runs out while wanding).
// The engine has no ranged item slot yet, and simc's weapon damage tables are retail scaled (a level 20 wand
// comes out ~5x too strong), so the wand is the option priest.wand=min:max:speed[:school], the same numbers as
// the addon export's "weapon_damage=...ranged:min:max:speed" line. School defaults to arcane (no priest school
// bonus applies); priest.wand=18:34:1.7:shadow makes a Shadow wand.
// ESTIMATES: spell hit and spell crit (x1.5) tables.
// Wand Specialization (+13/25%) is applied here only, see priest_t::init_spells.
struct shoot_t : public priest_spell_t
{
  shoot_t( priest_t* p, util::string_view options_str ) :
    priest_spell_t( "shoot", p, p->find_spell( 5019 ) )
  {
    parse_options( options_str );

    // Shoot's Weapon Damage effect would otherwise pick the main hand weapon
    weapon            = nullptr;
    weapon_multiplier = 0.0;

    auto splits = util::string_split<std::string_view>( p->options.wand, ":" );
    if ( splits.size() < 3 )
    {
      background = true;  // no wand: never usable
      return;
    }

    base_dd_min       = util::to_double( splits[ 0 ] );
    base_dd_max       = util::to_double( splits[ 1 ] );
    base_execute_time = timespan_t::from_seconds( util::to_double( splits[ 2 ] ) );
    school            = splits.size() > 3 ? util::parse_school_type( splits[ 3 ] ) : SCHOOL_ARCANE;
    if ( school == SCHOOL_NONE || school == SCHOOL_PHYSICAL )
      school = SCHOOL_ARCANE;
    trigger_gcd       = 0_ms;
    may_crit          = true;
    spell_power_mod.direct = 0.0;

    if ( base_execute_time <= 0_ms || base_dd_max < base_dd_min )
      throw std::invalid_argument( fmt::format( "Invalid priest.wand '{}', expected min:max:speed[:school]",
                                                p->options.wand ) );

    if ( p->talent.wand_specialization.ok() )
      base_multiplier *= 1.0 + p->talent.wand_specialization->effectN( 1 ).percent();
  }
};

// ==========================================================================
// Racials
// ==========================================================================

// Starshards (Night Elf): 6 s channel, 1 s ticks, 30 s cooldown in Forever
struct starshards_t : public priest_spell_t
{
  starshards_t( priest_t* p, util::string_view options_str ) :
    priest_spell_t( "starshards", p, p->race == RACE_NIGHT_ELF ? forever::max_rank_spell( p, starshards_ranks )
                                                               : spell_data_t::not_found() )
  {
    parse_options( options_str );

    channeled = data().flags( spell_attribute::SX_CHANNELED );
  }
};

// Chastise (Dwarf): instant Holy damage, humanoids only (not checked: Patchwerk-style targets have no type)
struct chastise_t : public priest_spell_t
{
  chastise_t( priest_t* p, util::string_view options_str ) :
    priest_spell_t( "chastise", p, p->race == RACE_DWARF ? forever::max_rank_spell( p, chastise_ranks )
                                                         : spell_data_t::not_found() )
  {
    parse_options( options_str );
  }
};

// ==========================================================================
// Cooldowns and buffs
// ==========================================================================

// Inner Focus (Discipline 11 point talent): off the GCD, consumed by the next spell (see priest_action_t)
struct inner_focus_t : public priest_buff_spell_t
{
  inner_focus_t( priest_t* p, util::string_view options_str ) :
    priest_buff_spell_t( "inner_focus", p, p->talent.inner_focus.ok() ? p->talent.inner_focus.spell()
                                                                      : spell_data_t::not_found(),
                         p->buff.inner_focus, options_str )
  {}

  bool verify_actor_level() const override
  { return true; }
};

// Power Infusion (Discipline 31 point talent): self cast in the sim
struct power_infusion_t : public priest_buff_spell_t
{
  power_infusion_t( priest_t* p, util::string_view options_str ) :
    priest_buff_spell_t( "power_infusion", p, p->talent.power_infusion.rank() > 0
                                                ? dbc::find_spell( p, p->talent.power_infusion.trait()->id_spell )
                                                : spell_data_t::not_found(),
                         p->buff.power_infusion, options_str )
  {}

  bool verify_actor_level() const override
  { return true; }
};

struct power_word_fortitude_t : public priest_buff_spell_t
{
  power_word_fortitude_t( priest_t* p, util::string_view options_str ) :
    priest_buff_spell_t( "power_word_fortitude", p, forever::max_rank_spell( p, power_word_fortitude_ranks ),
                         p->buff.power_word_fortitude, options_str )
  {}
};

// Inner Fire: armor (aura 674, not modelled); cast for the GCD/mana only
struct inner_fire_t : public priest_buff_spell_t
{
  inner_fire_t( priest_t* p, util::string_view options_str ) :
    priest_buff_spell_t( "inner_fire", p, forever::max_rank_spell( p, inner_fire_ranks ), p->buff.inner_fire,
                         options_str )
  {}
};

struct divine_spirit_t : public priest_buff_spell_t
{
  divine_spirit_t( priest_t* p, util::string_view options_str ) :
    priest_buff_spell_t( "divine_spirit", p, forever::max_rank_spell( p, divine_spirit_ranks ),
                         p->buff.divine_spirit, options_str )
  {}
};

// ==========================================================================
// priest_t
// ==========================================================================

action_t* priest_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "shadow_word_pain" ) return new shadow_word_pain_t( this, options_str );
  if ( name == "mind_blast" ) return new mind_blast_t( this, options_str );
  if ( name == "mind_flay" ) return new mind_flay_t( this, options_str );
  if ( name == "devouring_plague" ) return new devouring_plague_t( this, options_str );
  if ( name == "shadow_word_death" ) return new shadow_word_death_t( this, options_str );
  if ( name == "vampiric_embrace" ) return new vampiric_embrace_t( this, options_str );
  if ( name == "shadowform" ) return new shadowform_t( this, options_str );

  if ( name == "smite" ) return new smite_t( this, options_str );
  if ( name == "holy_fire" ) return new holy_fire_t( this, options_str );
  if ( name == "holy_nova" ) return new holy_nova_t( this, options_str );
  if ( name == "penance" ) return new penance_t( this, options_str );

  if ( name == "shoot" || name == "wand" ) return new shoot_t( this, options_str );

  if ( name == "starshards" ) return new starshards_t( this, options_str );
  if ( name == "chastise" ) return new chastise_t( this, options_str );

  if ( name == "inner_focus" ) return new inner_focus_t( this, options_str );
  if ( name == "power_infusion" ) return new power_infusion_t( this, options_str );
  if ( name == "power_word_fortitude" ) return new power_word_fortitude_t( this, options_str );
  if ( name == "inner_fire" ) return new inner_fire_t( this, options_str );
  if ( name == "divine_spirit" ) return new divine_spirit_t( this, options_str );

  return parse_player_effects_t::create_action( name, options_str );
}

void priest_t::init_base_stats()
{
  // Classic: melee attack power = Str - 10
  base.attack_power_per_strength = 1.0;

  parse_player_effects_t::init_base_stats();

  base.stats.attack_power = -10.0;

  // ESTIMATE: vanilla priest base crit (spell 0.8%, melee 3%), as the warlock/shaman modules do for theirs
  base.attack_crit_chance = get_passive_player_value( 0.03, "all_crit" );
  base.spell_crit_chance  = get_passive_player_value( 0.008, "all_crit" );
}

void priest_t::init_spells()
{
  parse_player_effects_t::init_spells();

  auto t = [ this ]( std::string_view n ) { return forever::talent( this, n ); };

  talent.power_in_light      = t( "Power in Light" );
  talent.wand_specialization = t( "Wand Specialization" );
  talent.twin_disciplines    = t( "Twin Disciplines" );
  talent.holy_precision      = t( "Holy Precision" );
  talent.mental_strength     = t( "Mental Strength" );
  talent.mental_agility      = t( "Mental Agility" );
  talent.inner_focus         = t( "Inner Focus" );
  talent.meditation          = t( "Meditation" );
  talent.penance             = t( "Penance" );
  talent.power_infusion      = t( "Power Infusion" );

  talent.holy_specialization = t( "Holy Specialization" );
  talent.divine_fury         = t( "Divine Fury" );
  talent.holy_nova           = t( "Holy Nova" );
  talent.searing_light       = t( "Searing Light" );
  talent.spiritual_guidance  = t( "Spiritual Guidance" );

  talent.shadow_focus              = t( "Shadow Focus" );
  talent.spirit_tap                = t( "Spirit Tap" );
  talent.improved_mind_blast       = t( "Improved Mind Blast" );
  talent.improved_shadow_word_pain = t( "Improved Shadow Word: Pain" );
  talent.mind_flay                 = t( "Mind Flay" );
  talent.improved_mind_flay        = t( "Improved Mind Flay" );
  talent.dark_inspiration          = t( "Dark Inspiration" );
  talent.vampiric_embrace          = t( "Vampiric Embrace" );
  talent.shadow_weaving            = t( "Shadow Weaving" );
  talent.devouring_contagion       = t( "Devouring Contagion" );
  talent.early_demise              = t( "Early Demise" );
  talent.darkness                  = t( "Darkness" );
  talent.shadowform                = t( "Shadowform" );

  spell.shadow_weaving   = find_spell( 15258 );
  spell.dark_inspiration = find_spell( 1326168 );
  spell.spirit_tap       = find_spell( 15271 );
  spell.holy_purpose     = find_spell( 1284536 );

  // Wand Specialization is Modify Damage Done% for all magic schools with "Requires weapon: Wand"; the passive
  // parse would apply it to every spell. Only the wand gets it (shoot_t).
  deregister_passive_spell( talent.wand_specialization.spell() );

  parse_all_class_passives();
  parse_all_passive_talents();
}

void priest_t::create_buffs()
{
  parse_player_effects_t::create_buffs();

  buff.shadowform = make_buff( this, "shadowform", talent.shadowform.spell() );

  buff.inner_focus = make_buff( this, "inner_focus", talent.inner_focus.spell() );

  buff.power_infusion = make_buff( this, "power_infusion",
                                   talent.power_infusion.rank() > 0
                                     ? dbc::find_spell( this, talent.power_infusion.trait()->id_spell )
                                     : spell_data_t::not_found() )
                          ->set_cooldown( 0_ms );

  // Dark Inspiration: the buff spell's base points are the max rank; the talent rank sets the value (3..15%)
  buff.dark_inspiration = make_buff( this, "dark_inspiration", spell.dark_inspiration )
                            ->set_default_value( talent.dark_inspiration.ok()
                                                   ? talent.dark_inspiration->effectN( 1 ).percent()
                                                   : 0.0 )
                            ->set_chance( talent.dark_inspiration.ok() ? 1.0 : 0.0 )
                            ->add_invalidate( CACHE_SPIRIT );

  buff.holy_purpose = make_buff( this, "holy_purpose", spell.holy_purpose );

  buff.inner_fire = make_buff( this, "inner_fire", forever::max_rank_spell( this, inner_fire_ranks ) );

  buff.power_word_fortitude =
    make_buff<stat_buff_t>( this, "power_word_fortitude", forever::max_rank_spell( this, power_word_fortitude_ranks ) );

  buff.divine_spirit =
    make_buff<stat_buff_t>( this, "divine_spirit", forever::max_rank_spell( this, divine_spirit_ranks ) );
}

void priest_t::create_options()
{
  parse_player_effects_t::create_options();

  add_option( opt_float( "priest.spirit_tap_uptime", options.spirit_tap_uptime, 0.0, 1.0 ) );
  add_option( opt_string( "priest.wand", options.wand ) );
}

void priest_t::copy_from( player_t* source )
{
  parse_player_effects_t::copy_from( source );

  options = debug_cast<priest_t*>( source )->options;
}

void priest_t::init_finished()
{
  parse_player_effects();

  parse_player_effects_t::init_finished();
}

// Player level buff and debuff effects from spell data. Effects that act on spells (A107/A108, e.g. Shadowform's
// crit bonus, Inner Focus) are parsed per action in priest_action_t.
void priest_t::parse_player_effects()
{
  // Shadowform: +10% Shadow damage (A79), -15% Physical damage taken (A87). The cost part (A72) is in
  // priest_action_t::cost_pct_multiplier.
  parse_effects( buff.shadowform );

  // Power Infusion: +20% damage for all magic schools (A79)
  parse_effects( buff.power_infusion );

  // Shadow Weaving: +2% Shadow damage taken from the priest per stack (A270)
  parse_target_effects( []( actor_target_data_t* td ) {
    return static_cast<priest_td_t*>( td )->debuff.shadow_weaving->check();
  }, spell.shadow_weaving );
}

// Spiritual Guidance effect #1 (aura 174, not in the engine): spell damage +1/3/5/6/8% of total Spirit.
// The healing part (aura 175) is not modelled (simc has a single spell power stat).
double priest_t::composite_spell_power( school_e school ) const
{
  double sp = parse_player_effects_t::composite_spell_power( school );

  if ( talent.spiritual_guidance.ok() )
    sp += talent.spiritual_guidance->effectN( 1 ).percent() * cache.spirit();

  return sp;
}

double priest_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = parse_player_effects_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_SPIRIT )
  {
    // Dark Inspiration: aura 80 on the buff, treated as a total Spirit multiplier
    m *= 1.0 + buff.dark_inspiration->check_value();

    // Spirit Tap: +100% Spirit while up, averaged over the option's uptime
    if ( talent.spirit_tap.ok() )
      m *= 1.0 + spell.spirit_tap->effectN( 1 ).percent() * options.spirit_tap_uptime;
  }

  return m;
}

double priest_t::composite_mana_regen_while_casting() const
{
  double r = parse_player_effects_t::composite_mana_regen_while_casting();

  // Spirit Tap effect #2 (aura 134 50%), averaged over the option's uptime
  if ( talent.spirit_tap.ok() )
    r += spell.spirit_tap->effectN( 2 ).percent() * options.spirit_tap_uptime;

  return std::min( r, 1.0 );
}

void priest_t::invalidate_cache( cache_e c )
{
  parse_player_effects_t::invalidate_cache( c );

  if ( c == CACHE_SPIRIT && talent.spiritual_guidance.ok() )
    invalidate_cache( CACHE_SPELL_POWER );
}

void priest_t::init_action_list()
{
  if ( !action_list_str.empty() )
  {
    parse_player_effects_t::init_action_list();
    return;
  }

  clear_action_priority_lists();
  priest_apl::priest( this );
  use_default_action_list = true;
  parse_player_effects_t::init_action_list();
}

}  // namespace priestspace

const module_t* module_t::priest()
{
  static priestspace::priest_module_t m;
  return &m;
}

// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
//
// WoW: Forever hunter (2026-10-16). Data: client build 1.60.1.70425. Spec and validation hooks:
// forever-utility simc/specs/hunter.md. Values marked ESTIMATE are vanilla (or reasoned) placeholders for
// server side or unknown Forever numbers, so the sim gives sensible relative results until measured.

#include "simulationcraft.hpp"
#include "action/parse_effects.hpp"
#include "action/residual_action.hpp"
#include "class_modules/apl/hunter.hpp"
#include "class_modules/forever_helpers.hpp"

namespace
{
using forever::max_rank;
using forever::spell_rank_t;

// ==========================================================================
// Ranks
// ==========================================================================

constexpr std::array aimed_shot_ranks {
  spell_rank_t{ 19434 },  // 20
  spell_rank_t{ 20900 },  // 28
  spell_rank_t{ 20901 },  // 36
  spell_rank_t{ 20902 },  // 44
  spell_rank_t{ 20903 },  // 52
  spell_rank_t{ 20904 },  // 60
};

// Forever: a single rank, no flat bonus
constexpr std::array multi_shot_ranks {
  spell_rank_t{ 2643 },  // 18
};

constexpr std::array arcane_shot_ranks {
  spell_rank_t{ 3044 },   // 6
  spell_rank_t{ 14281 },  // 12
  spell_rank_t{ 14282 },  // 20
  spell_rank_t{ 14283 },  // 28
  spell_rank_t{ 14284 },  // 36
  spell_rank_t{ 14285 },  // 44
  spell_rank_t{ 14286 },  // 52
  spell_rank_t{ 14287 },  // 60
};

// Explicit ids: 425728-425737 are Season of Discovery copies (12 s, SP coefficient, AcquireMethod 3)
constexpr std::array serpent_sting_ranks {
  spell_rank_t{ 1978 },   // 4
  spell_rank_t{ 13549 },  // 10
  spell_rank_t{ 13550 },  // 18
  spell_rank_t{ 13551 },  // 26
  spell_rank_t{ 13552 },  // 34
  spell_rank_t{ 13553 },  // 42
  spell_rank_t{ 13554 },  // 50
  spell_rank_t{ 13555 },  // 58
  spell_rank_t{ 25295 },  // 60
};

// Rank 4 (58) is 71 ranged AP, below rank 3's 98 (same in 70245, 70425 and simc 70334). Lower ranks stay
// castable, so best_value_rank() picks the strongest known rank like a player would.
constexpr std::array hunters_mark_ranks {
  spell_rank_t{ 1130 },   // 6
  spell_rank_t{ 14323 },  // 22
  spell_rank_t{ 14324 },  // 40
  spell_rank_t{ 14325 },  // 58
};

// Rank 6 (58) is 55 ranged AP, below rank 5's 90; rank 7 (60) is 120.
constexpr std::array aspect_of_the_hawk_ranks {
  spell_rank_t{ 13165 },  // 10
  spell_rank_t{ 14318 },  // 18
  spell_rank_t{ 14319 },  // 28
  spell_rank_t{ 14320 },  // 38
  spell_rank_t{ 14321 },  // 48
  spell_rank_t{ 14322 },  // 58
  spell_rank_t{ 25296 },  // 60
};

// Forever: melee attack power (effect #2) instead of the vanilla untrackable/dodge aspect
constexpr std::array aspect_of_the_beast_ranks {
  spell_rank_t{ 13161 },    // 30
  spell_rank_t{ 1299445 },  // 40
  spell_rank_t{ 1299446 },  // 50
  spell_rank_t{ 1299447 },  // 60
};

constexpr std::array aspect_of_the_falcon_ranks {
  spell_rank_t{ 469145 },  // 60
};

constexpr std::array aspect_of_the_viper_ranks {
  spell_rank_t{ 415423 },  // 1, AcquireMethod 0 (baseline in Forever)
};

// Rank 1 comes from the 16 point Marksmanship talent, ranks 2-5 are trained. Rank 5 (60) is 50 ranged AP,
// below rank 4's 75 (FLAG in the spec); best_value_rank() picks the strongest known rank.
constexpr std::array trueshot_aura_ranks {
  spell_rank_t{ 1299346 },  // 25
  spell_rank_t{ 1299348 },  // 32
  spell_rank_t{ 19506 },    // 40
  spell_rank_t{ 20905 },    // 50
  spell_rank_t{ 20906 },    // 60
};

constexpr std::array rapid_fire_ranks {
  spell_rank_t{ 3045 },  // 26
};

struct volley_rank_t
{
  unsigned spell_id;   // the channel (Periodic Dummy every 1 s)
  unsigned damage_id;  // the arcane damage per tick
};

constexpr std::array volley_ranks {
  volley_rank_t{ 1510, 1279721 },   // 40
  volley_rank_t{ 14294, 1279719 },  // 50
  volley_rank_t{ 14295, 1279715 },  // 58
};

// 31 point Marksmanship talent (rank 1), ranks 2-3 trained
constexpr std::array sniper_shot_ranks {
  spell_rank_t{ 1310687 },  // 40
  spell_rank_t{ 1310785 },  // 48
  spell_rank_t{ 1310786 },  // 58
};

// 16 point Beast Mastery talent (rank 1), ranks 2-4 trained
constexpr std::array summon_hawk_ranks {
  spell_rank_t{ 1293241 },  // 25
  spell_rank_t{ 1293525 },  // 36
  spell_rank_t{ 1293526 },  // 48
  spell_rank_t{ 1293527 },  // 60
};

// Forever halves the high rank flat bonus (rank 8 is +70, vanilla +140)
constexpr std::array raptor_strike_ranks {
  spell_rank_t{ 2973 },   // 1
  spell_rank_t{ 14260 },  // 8
  spell_rank_t{ 14261 },  // 16
  spell_rank_t{ 14262 },  // 24
  spell_rank_t{ 14263 },  // 32
  spell_rank_t{ 14264 },  // 40
  spell_rank_t{ 14265 },  // 48
  spell_rank_t{ 14266 },  // 56
};

constexpr std::array mongoose_bite_ranks {
  spell_rank_t{ 1495 },   // 16
  spell_rank_t{ 14269 },  // 30
  spell_rank_t{ 14270 },  // 44
  spell_rank_t{ 14271 },  // 58
};

constexpr std::array wing_clip_ranks {
  spell_rank_t{ 2974 },   // 12
  spell_rank_t{ 14267 },  // 38
  spell_rank_t{ 14268 },  // 60
};

// Survival skill line, levels 30-60 (trainer availability unverified, spec V-H10)
constexpr std::array lacerate_ranks {
  spell_rank_t{ 24118 },    // 30
  spell_rank_t{ 24119 },    // 40
  spell_rank_t{ 24120 },    // 50
  spell_rank_t{ 1299332 },  // 60
};

// Pet abilities (pet skill lines 208 Wolf, 209 Cat, 270 Generic)
constexpr std::array claw_ranks {
  spell_rank_t{ 16827 },  // 1
  spell_rank_t{ 16828 },  // 8
  spell_rank_t{ 16829 },  // 16
  spell_rank_t{ 16830 },  // 24
  spell_rank_t{ 16831 },  // 32
  spell_rank_t{ 16832 },  // 40
  spell_rank_t{ 3010 },   // 48
  spell_rank_t{ 3009 },   // 56
};

constexpr std::array bite_ranks {
  spell_rank_t{ 17253 },  // 1
  spell_rank_t{ 17255 },  // 8
  spell_rank_t{ 17256 },  // 16
  spell_rank_t{ 17257 },  // 24
  spell_rank_t{ 17258 },  // 32
  spell_rank_t{ 17259 },  // 40
  spell_rank_t{ 17260 },  // 48
  spell_rank_t{ 17261 },  // 56
};

constexpr std::array furious_howl_ranks {
  spell_rank_t{ 24604 },  // 10
  spell_rank_t{ 24605 },  // 24
  spell_rank_t{ 24603 },  // 40
  spell_rank_t{ 24597 },  // 56
};

// Spell ids without ranks
constexpr unsigned AUTO_SHOT          = 75;
constexpr unsigned QUICK_SHOTS        = 6150;     // Aspect of the Hawk proc (Deadly Aspects)
constexpr unsigned QUICK_STRIKES      = 1299448;  // Aspect of the Beast proc (Deadly Aspects)
constexpr unsigned QUICK_STRIKES_FALC = 469144;   // Aspect of the Falcon proc
constexpr unsigned BESTIAL_WRATH      = 19574;
constexpr unsigned INTIMIDATION       = 19577;
constexpr unsigned STRIDER_KICK       = 1317257;
constexpr unsigned LACERATING_BLEED   = 1310536;
constexpr unsigned EXPOSE_PREY_BUFF   = 1310726;
constexpr unsigned RAPID_RECUP_BUFF   = 1242512;
constexpr unsigned RESOURCEFUL_BUFF   = 1242688;
constexpr unsigned FRENZY_BUFF        = 19615;
constexpr unsigned SUMMON_HAWK_PET    = 1293248;  // Summon Guardian NPC 264850, 18 s

// ==========================================================================
// ESTIMATES (server side / not in client data). Keep this list in sync with FOREVER_HUNTER.md.
// ==========================================================================

// Pet focus regeneration per second. Not in client data; vanilla community value ~5/s (spec: unverified).
constexpr double PET_FOCUS_REGEN = 5.0;
// Pet base focus (vanilla 100). Hunter Pet Scaling 415429 effect #16 adds +50 max focus on top (data).
constexpr double PET_BASE_FOCUS = 100.0;
// Pet melee: vanilla 2.0 s base swing. Damage per hit avg = level * 1.0, +-20%. Vanilla creature table fit.
constexpr double PET_SWING_TIME   = 2.0;
constexpr double PET_DAMAGE_LEVEL = 1.0;
constexpr double PET_DAMAGE_RANGE = 0.2;
// Forever: "pets gain stats based on the hunter's gear" (415429 is all scripted, bp 0). Use the TBC rule of
// pet AP = 22% of the hunter's ranged AP until measured (spec V-H6).
constexpr double PET_AP_FROM_RAP = 0.22;
// Vanilla pets: 5% base crit, no hit from the owner.
constexpr double PET_BASE_CRIT = 0.05;
// Happiness (vanilla): happy 125% damage, content 100%, unhappy 75%. Option pet_happiness, default happy.
// Summon Hawk (16 point BM talent): hawk NPC damage/AI unknown. Each active hawk attacks every 2 s for 30%
// of the dive bomb's damage (bp + 5% RAP), physical, with the hunter's crit.
constexpr double HAWK_SWING_TIME   = 2.0;
constexpr double HAWK_DAMAGE_RATIO = 0.30;
// Aspect of the Viper: mana on each landed auto attack "based on weapon speed"; amount unknown.
// ESTIMATE 1% of base mana per second of weapon speed. The 10% max mana every 3 s is client data.
constexpr double VIPER_MANA_PER_SPEED = 0.01;
// Auto Shot windup ("aim"): the last 0.5 s of each cycle cannot overlap a cast (beta hotfix, spec section 1).
constexpr timespan_t AUTO_SHOT_AIM = 500_ms;

struct hunter_t;

namespace pets
{
struct hunter_pet_t;
}

// ==========================================================================
// Target data
// ==========================================================================

struct hunter_td_t : public actor_target_data_t
{
  struct debuffs_t
  {
    buff_t* hunters_mark = nullptr;
  } debuff;

  hunter_td_t( player_t* target, hunter_t* p );
};

// ==========================================================================
// Hunter
// ==========================================================================

struct hunter_t final : public player_t
{
  hunter_t( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) : player_t( sim, HUNTER, name, r ) {}

  // Options
  struct options_t
  {
    double ammo_dps      = 0.0;  // arrows/bullets: + ammo_dps * weapon speed (normalized 2.8 for specials)
    double quiver_haste  = 0.0;  // quiver / ammo pouch ranged haste, percent (e.g. 15)
    std::string pet_happiness = "happy";
    bool tracking        = true;  // Improved Tracking: target is of the tracked creature type
  } options;

  struct talents_t
  {
    // Beast Mastery
    player_talent_t deadly_aspects;
    player_talent_t focused_fire;
    player_talent_t unleashed_fury;
    player_talent_t ferocity;
    player_talent_t summon_hawk;
    player_talent_t intimidation;
    player_talent_t bestial_discipline;
    player_talent_t frenzy;
    player_talent_t bestial_wrath;

    // Marksmanship
    player_talent_t careful_aim;
    player_talent_t lone_wolf;
    player_talent_t trueshot_aura;
    player_talent_t rapid_recuperation;
    player_talent_t ranged_weapon_specialization;
    player_talent_t sniper_shot;

    // Survival
    player_talent_t improved_tracking;
    player_talent_t surefooted;
    player_talent_t resourcefulness;
    player_talent_t expose_prey;
    player_talent_t strider_kick;
    player_talent_t lacerating_strikes;
  } talent;

  struct buffs_t
  {
    buff_t* aspect_of_the_hawk   = nullptr;
    buff_t* aspect_of_the_beast  = nullptr;
    buff_t* aspect_of_the_falcon = nullptr;
    buff_t* aspect_of_the_viper  = nullptr;
    buff_t* trueshot_aura        = nullptr;
    buff_t* rapid_fire           = nullptr;
    buff_t* quick_shots          = nullptr;
    buff_t* quick_strikes        = nullptr;
    buff_t* furious_howl         = nullptr;
    buff_t* expose_prey          = nullptr;
    buff_t* rapid_recuperation   = nullptr;
    buff_t* resourcefulness      = nullptr;
    buff_t* raptor_strike_queued = nullptr;
    std::array<buff_t*, 2> hawk {};
  } buff;

  struct gains_t
  {
    gain_t* viper_periodic = nullptr;
    gain_t* viper_auto     = nullptr;
  } gain;

  struct procs_t
  {
    proc_t* auto_shot_clipped = nullptr;
  } proc;

  struct pets_t
  {
    pets::hunter_pet_t* cat    = nullptr;
    pets::hunter_pet_t* wolf   = nullptr;
    pets::hunter_pet_t* active = nullptr;
  } pets;

  action_t* auto_shot     = nullptr;
  action_t* raptor_strike = nullptr;
  action_t* hawk_strike   = nullptr;
  action_t* lacerating_strikes = nullptr;

  // End of the last hunter cast (Aimed Shot, Sniper Shot, Volley channel), for Auto Shot clipping
  timespan_t last_cast_end = timespan_t::min();

  // Strongest known rank values (lower ranks stay castable, see the rank lists)
  double hunters_mark_value  = 0.0;
  double trueshot_aura_value = 0.0;
  double hawk_aspect_value   = 0.0;

  target_specific_t<hunter_td_t> target_data;

  resource_e primary_resource() const override { return RESOURCE_MANA; }
  role_e primary_role() const override { return ROLE_ATTACK; }
  stat_e convert_hybrid_stat( stat_e s ) const override;

  std::string default_potion() const override { return hunter_apl::potion( this ); }
  std::string default_flask() const override  { return hunter_apl::flask( this ); }
  std::string default_food() const override   { return hunter_apl::food( this ); }

  void create_options() override;
  void copy_from( player_t* source ) override;
  action_t* create_action( util::string_view name, util::string_view options_str ) override;
  void create_actions() override;
  void create_pets() override;
  void create_buffs() override;
  void init_base_stats() override;
  void init_spells() override;
  void init_gains() override;
  void init_procs() override;
  void init_action_list() override;
  void reset() override;
  std::unique_ptr<expr_t> create_expression( util::string_view name ) override;

  double composite_melee_attack_power() const override;
  double composite_melee_hit() const override;
  double composite_melee_auto_attack_speed() const override;
  double composite_player_multiplier( school_e ) const override;
  double composite_player_critical_damage_multiplier( const action_state_t*, school_e ) const override;
  double composite_mana_regen_while_casting() const override;

  hunter_td_t* get_target_data( player_t* target ) const override
  {
    hunter_td_t*& td = target_data[ target ];
    if ( !td )
      td = new hunter_td_t( target, const_cast<hunter_t*>( this ) );
    return td;
  }

  bool pet_active() const;
  double ranged_attack_power( player_t* target ) const;
  double ranged_speed_multiplier() const;
  double pet_happiness_multiplier() const;
  timespan_t auto_shot_blocked_until() const;
  void trigger_expose_prey( const action_state_t* s );
  void trigger_resourcefulness( const action_state_t* s );
  void summon_hawk();
  int active_hawks() const;
};

hunter_td_t::hunter_td_t( player_t* target, hunter_t* p ) : actor_target_data_t( target, p )
{
  debuff.hunters_mark = make_buff( *this, "hunters_mark", p->find_spell( max_rank( p, hunters_mark_ranks ).spell_id ) )
                          ->set_default_value( p->hunters_mark_value );
}

// Highest effect value over the known ranks (for spells whose top rank is weaker than the one below)
template <typename RANKS>
const spell_data_t* best_value_rank( const player_t* p, const RANKS& ranks, unsigned effect_index )
{
  const spell_data_t* best = spell_data_t::not_found();
  double best_value        = -1;
  for ( const auto& rank : ranks )
  {
    const spell_data_t* s = p->find_spell( rank.spell_id );
    if ( s->ok() && s->effectN( effect_index ).base_value() > best_value )
    {
      best       = s;
      best_value = s->effectN( effect_index ).base_value();
    }
  }
  return best;
}

// ==========================================================================
// Pets
// ==========================================================================

namespace pets
{
struct hunter_pet_t : public pet_t
{
  struct buffs_t
  {
    buff_t* bestial_wrath = nullptr;
    buff_t* frenzy        = nullptr;
    buff_t* intimidation  = nullptr;
  } buff;

  hunter_pet_t( hunter_t* owner, std::string_view name, pet_e type ) : pet_t( owner->sim, owner, name, type )
  {
    stamina_per_owner   = 0;
    intellect_per_owner = 0;

    equipped_weapons[ SLOT_MAIN_HAND ].type       = WEAPON_BEAST;
    equipped_weapons[ SLOT_MAIN_HAND ].swing_time = timespan_t::from_seconds( PET_SWING_TIME );
  }

  hunter_t* o() { return debug_cast<hunter_t*>( owner ); }
  const hunter_t* o() const { return debug_cast<const hunter_t*>( owner ); }

  resource_e primary_resource() const override { return RESOURCE_FOCUS; }

  void init_base_stats() override
  {
    pet_t::init_base_stats();

    // Pets fight from behind (no parry/block) and start with the vanilla pet crit
    base.position            = POSITION_BACK;
    base.attack_crit_chance  = PET_BASE_CRIT;
    base.spell_crit_chance   = PET_BASE_CRIT;

    double avg = PET_DAMAGE_LEVEL * level();
    equipped_weapons[ SLOT_MAIN_HAND ].min_dmg = avg * ( 1.0 - PET_DAMAGE_RANGE );
    equipped_weapons[ SLOT_MAIN_HAND ].max_dmg = avg * ( 1.0 + PET_DAMAGE_RANGE );
    equipped_weapons[ SLOT_MAIN_HAND ].damage  = avg;
    equipped_weapons[ SLOT_MAIN_HAND ].dps     = avg / PET_SWING_TIME;

    // Hunter Pet Scaling 415429 effect #16: +50 max focus (data)
    double extra_focus = 0;
    for ( const auto& e : o()->find_spell( 415429 )->effects() )
      if ( e.subtype() == A_MOD_MAX_RESOURCE && e.resource_gain_type() == RESOURCE_FOCUS )
        extra_focus += e.base_value();

    resources.active_resource[ RESOURCE_FOCUS ] = true;
    resources.base[ RESOURCE_FOCUS ]            = PET_BASE_FOCUS + extra_focus;
  }

  void create_buffs() override
  {
    pet_t::create_buffs();

    const spell_data_t* bw = o()->talent.bestial_wrath.ok() ? o()->find_spell( BESTIAL_WRATH ) : spell_data_t::not_found();
    buff.bestial_wrath = make_buff( this, "bestial_wrath", bw )
                           ->set_default_value( bw->effectN( 2 ).percent() )
                           ->set_cooldown( 0_ms );

    const spell_data_t* frenzy = o()->find_spell( FRENZY_BUFF );
    buff.frenzy = make_buff( this, "frenzy", frenzy )
                    ->set_default_value( frenzy->effectN( 1 ).percent() )
                    ->set_chance( o()->talent.frenzy->effectN( 1 ).percent() )
                    ->add_invalidate( CACHE_AUTO_ATTACK_SPEED );

    const spell_data_t* intim = o()->talent.intimidation.ok() ? o()->find_spell( INTIMIDATION ) : spell_data_t::not_found();
    buff.intimidation = make_buff( this, "intimidation", intim )
                          ->set_default_value( intim->effectN( 2 ).percent() )
                          ->set_cooldown( 0_ms );
  }

  // Focus regen (ESTIMATE), Bestial Discipline +10/20% (talent effect #1)
  double resource_regen_per_second( resource_e r ) const override
  {
    if ( r == RESOURCE_FOCUS )
      return PET_FOCUS_REGEN * ( 1.0 + o()->talent.bestial_discipline->effectN( 1 ).percent() );

    return pet_t::resource_regen_per_second( r );
  }

  // Forever pets scale with the hunter's gear: AP from the hunter's ranged AP (ESTIMATE coefficient)
  void update_stats() override
  {
    pet_t::update_stats();
    current_pet_stats.attack_power_from_ap = o()->ranged_attack_power( nullptr ) * PET_AP_FROM_RAP;
  }

  double composite_melee_attack_power() const override
  { return current_pet_stats.attack_power_from_ap; }

  // Vanilla pets don't inherit the owner's hit, crit or haste
  double composite_melee_hit() const override { return 0.0; }
  double composite_spell_hit() const override { return 0.0; }
  double composite_melee_expertise( const weapon_t* ) const override { return 0.0; }

  double pet_crit() const override
  {
    return base.attack_crit_chance + o()->talent.ferocity->effectN( 1 ).percent();
  }

  double composite_melee_haste() const override { return 1.0; }

  double composite_melee_auto_attack_speed() const override
  {
    return 1.0 / ( 1.0 + buff.frenzy->check_value() );
  }

  double composite_player_multiplier( school_e s ) const override
  {
    double m = pet_t::composite_player_multiplier( s );

    m *= o()->pet_happiness_multiplier();
    m *= 1.0 + o()->talent.unleashed_fury->effectN( 1 ).percent();
    m *= 1.0 + o()->talent.focused_fire->effectN( 1 ).percent();
    m *= 1.0 + buff.bestial_wrath->check_value();

    return m;
  }

  // Pets don't receive the hunter's buffs; take only owner crit damage talents that name pets (none yet)
  double composite_player_critical_damage_multiplier( const action_state_t* s, school_e school ) const override
  { return player_t::composite_player_critical_damage_multiplier( s, school ); }

  void summon( timespan_t duration = timespan_t::zero() ) override
  {
    pet_t::summon( duration );
    o()->pets.active = this;
  }

  void dismiss( bool expired = false ) override
  {
    pet_t::dismiss( expired );
    if ( o()->pets.active == this )
      o()->pets.active = nullptr;
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
  void init_action_list() override;
};

template <class Base>
struct hunter_pet_action_t : public parse_action_effects_t<Base>
{
private:
  using ab = parse_action_effects_t<Base>;

public:
  using base_t = hunter_pet_action_t<Base>;

  template <typename... Args>
  hunter_pet_action_t( Args&&... args ) : ab( std::forward<Args>( args )... ) {}

  hunter_pet_t* p() { return debug_cast<hunter_pet_t*>( this->player ); }
  const hunter_pet_t* p() const { return debug_cast<const hunter_pet_t*>( this->player ); }
  hunter_t* o() { return p()->o(); }
  const hunter_t* o() const { return p()->o(); }

  // Intimidation: the next successful pet attack gains +100% crit (and stuns)
  double composite_crit_chance() const override
  {
    return ab::composite_crit_chance() + p()->buff.intimidation->check_value();
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( !this->result_is_hit( s->result ) )
      return;

    p()->buff.intimidation->expire();

    // Frenzy: chance on a pet crit for +30% attack speed
    if ( s->result == RESULT_CRIT )
      p()->buff.frenzy->trigger();
  }
};

using hunter_pet_attack_t = hunter_pet_action_t<melee_attack_t>;

struct pet_melee_t : public hunter_pet_attack_t
{
  pet_melee_t( hunter_pet_t* p ) : hunter_pet_attack_t( "melee", p, spell_data_t::nil() )
  {
    weapon            = &p->equipped_weapons[ SLOT_MAIN_HAND ];
    weapon_multiplier = 1.0;
    base_execute_time = weapon->swing_time;
    school            = SCHOOL_PHYSICAL;
    trigger_gcd       = 0_ms;
    background = repeating = true;
    may_glance = may_crit = true;
    special    = false;
  }
};

struct pet_auto_attack_t : public hunter_pet_attack_t
{
  pet_auto_attack_t( hunter_pet_t* p, util::string_view options_str )
    : hunter_pet_attack_t( "auto_attack", p, spell_data_t::nil() )
  {
    parse_options( options_str );
    trigger_gcd           = 0_ms;
    ignore_false_positive = true;
    p->main_hand_attack   = new pet_melee_t( p );
  }

  void execute() override { player->main_hand_attack->schedule_execute(); }

  bool ready() override { return player->main_hand_attack->execute_event == nullptr; }
};

// Claw and Bite: flat physical damage (bp +- variance), no AP coefficient in the data (vanilla behaviour)
struct pet_special_t : public hunter_pet_attack_t
{
  pet_special_t( util::string_view name, hunter_pet_t* p, const spell_data_t* s, util::string_view options_str )
    : hunter_pet_attack_t( name, p, s )
  {
    parse_options( options_str );
  }
};

struct furious_howl_t : public hunter_pet_attack_t
{
  furious_howl_t( hunter_pet_t* p, util::string_view options_str )
    : hunter_pet_attack_t( "furious_howl", p, p->find_spell( max_rank( p, furious_howl_ranks ).spell_id ) )
  {
    parse_options( options_str );
    harmful = may_miss = may_crit = false;
    target  = p;
  }

  void execute() override
  {
    hunter_pet_attack_t::execute();
    o()->buff.furious_howl->trigger( 1, data().effectN( 1 ).average( p() ) );
  }
};

action_t* hunter_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "auto_attack" )
    return new pet_auto_attack_t( this, options_str );
  if ( name == "claw" )
    return new pet_special_t( "claw", this, find_spell( max_rank( this, claw_ranks ).spell_id ), options_str );
  if ( name == "bite" )
    return new pet_special_t( "bite", this, find_spell( max_rank( this, bite_ranks ).spell_id ), options_str );
  if ( name == "furious_howl" )
    return new furious_howl_t( this, options_str );

  return pet_t::create_action( name, options_str );
}

void hunter_pet_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    action_list_str = "auto_attack";
    action_list_str += "/bite";
    if ( pet_type == PET_WOLF )
    {
      action_list_str += "/furious_howl";
    }
    else
    {
      // keep enough focus for the next Bite
      action_list_str += "/claw,if=focus>=60|cooldown.bite.remains>3";
    }
  }

  pet_t::init_action_list();
}
}  // namespace pets

// ==========================================================================
// Hunter actions
// ==========================================================================

template <class Base>
struct hunter_action_t : public parse_action_effects_t<Base>
{
private:
  using ab = parse_action_effects_t<Base>;

public:
  using base_t = hunter_action_t<Base>;

  // Shared category cooldowns (Aimed/Multi-Shot, Arcane Shot/Summon Hawk). Separate cooldown_t objects, because
  // Improved Arcane Shot shortens only Arcane Shot's duration; the spell mod also applies to the category
  // recovery it starts, so the partner gets this action's modified duration.
  std::vector<cooldown_t*> shared_cooldowns;

  template <typename... Args>
  hunter_action_t( Args&&... args ) : ab( std::forward<Args>( args )... ) {}

  hunter_t* p() { return debug_cast<hunter_t*>( this->player ); }
  const hunter_t* p() const { return debug_cast<const hunter_t*>( this->player ); }

  hunter_td_t* td( player_t* t ) const { return p()->get_target_data( t ); }

  // Self buffs can't miss
  void init() override
  {
    ab::init();

    if ( !this->harmful )
    {
      this->may_miss = this->may_dodge = this->may_parry = this->may_block = false;
    }
  }

  void execute() override
  {
    ab::execute();

    for ( cooldown_t* cd : shared_cooldowns )
      cd->start( this->cooldown_duration() );

    if ( this->base_execute_time > 0_ms && !this->background )
      p()->last_cast_end = this->sim->current_time();
  }

  void last_tick( dot_t* d ) override
  {
    ab::last_tick( d );

    if ( this->channeled )
      p()->last_cast_end = this->sim->current_time();
  }

  void impact( action_state_t* s ) override
  {
    ab::impact( s );

    if ( this->harmful && this->result_is_hit( s->result ) && s->result_type == result_amount_type::DMG_DIRECT )
    {
      p()->trigger_expose_prey( s );
      p()->trigger_resourcefulness( s );
    }
  }
};

using hunter_spell_t  = hunter_action_t<spell_t>;
using hunter_attack_t = hunter_action_t<melee_attack_t>;

// Ranged attacks use the ranged attack power (2 Agi + 2 L - 10, ranged AP auras, Hunter's Mark on the target),
// ammo damage, and Ranged Weapon Specialization.
struct hunter_ranged_attack_t : public hunter_action_t<ranged_attack_t>
{
  bool uses_ranged_weapon = false;

  hunter_ranged_attack_t( util::string_view name, hunter_t* p, const spell_data_t* s )
    : base_t( name, p, s )
  {
    uses_ranged_weapon = s->flags( spell_attribute::SX_RANGED_ABILITY ) || s->id() == AUTO_SHOT;

    if ( uses_ranged_weapon && p->equipped_weapons[ SLOT_RANGED ].type != WEAPON_NONE )
      weapon = &p->equipped_weapons[ SLOT_RANGED ];

    // Classic ranged attacks don't glance and can't be parried or blocked (ranged_attack_t)
    may_glance = false;
  }

  double composite_total_attack_power() const override
  {
    return p()->ranged_attack_power( target );
  }

  // Classic shot cast times (Aimed 2.0 s, Sniper 4.0 s) are not reduced by ranged haste. attack_t applies the
  // melee auto attack speed here, so go back to the plain action value. ESTIMATE (unverified on Forever).
  double execute_time_pct_multiplier() const override
  { return action_t::execute_time_pct_multiplier(); }

  // Ammo: + ammo DPS times weapon speed (2.8 s for normalized shots). Vanilla rule, spec V-H3.
  double bonus_da( const action_state_t* s ) const override
  {
    double b = base_t::bonus_da( s );

    if ( weapon && weapon_multiplier > 0 && weapon->slot == SLOT_RANGED && p()->options.ammo_dps > 0 )
    {
      timespan_t speed = normalize_weapon_speed ? weapon->get_normalized_speed() : weapon->swing_time;
      b += p()->options.ammo_dps * speed.total_seconds();
    }

    return b;
  }

  double action_multiplier() const override
  {
    double m = base_t::action_multiplier();

    // Ranged Weapon Specialization: "+X% damage you deal with ranged weapons" (the aura 79 is equip gated
    // to bows/guns/crossbows, applied here to ranged weapon attacks only, vanilla behaviour)
    if ( uses_ranged_weapon )
      m *= 1.0 + p()->talent.ranged_weapon_specialization->effectN( 1 ).percent();

    return m;
  }
};

// --------------------------------------------------------------------------
// Auto Shot
// --------------------------------------------------------------------------

// Repeating ranged attack. Interval = ranged weapon speed / ranged haste (Quick Shots, Rapid Fire, quiver,
// multiplicative). The last 0.5 s of each cycle ("aim") cannot overlap a cast or channel: if one does,
// the shot fires 0.5 s after it ends (clipping). Instant shots don't delay it.
struct auto_shot_t : public hunter_ranged_attack_t
{
  timespan_t delay_override = timespan_t::min();

  auto_shot_t( hunter_t* p ) : hunter_ranged_attack_t( "auto_shot", p, p->find_spell( AUTO_SHOT ) )
  {
    school            = SCHOOL_PHYSICAL;
    weapon_multiplier = 1.0;
    base_execute_time = weapon ? weapon->swing_time : 3_s;
    trigger_gcd       = 0_ms;
    background = repeating = true;
    special    = false;
    may_crit   = true;
  }

  timespan_t execute_time() const override
  {
    if ( delay_override >= 0_ms )
      return delay_override;

    return base_execute_time * p()->ranged_speed_multiplier();
  }

  void schedule_execute( action_state_t* s = nullptr ) override
  {
    hunter_ranged_attack_t::schedule_execute( s );
    delay_override = timespan_t::min();
  }

  void execute() override
  {
    timespan_t free_at = p()->auto_shot_blocked_until();
    if ( free_at > sim->current_time() )
    {
      p()->proc.auto_shot_clipped->occur();
      delay_override = free_at - sim->current_time();
      schedule_execute();
      return;
    }

    hunter_ranged_attack_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    // Deadly Aspects: Quick Shots from Aspect of the Hawk (or Falcon) on Auto Shot
    if ( p()->talent.deadly_aspects.ok() &&
         ( p()->buff.aspect_of_the_hawk->check() || p()->buff.aspect_of_the_falcon->check() ) )
      p()->buff.quick_shots->trigger( 1, buff_t::DEFAULT_VALUE(), p()->talent.deadly_aspects->effectN( 1 ).percent() );

    if ( p()->buff.aspect_of_the_viper->check() )
      p()->resource_gain( RESOURCE_MANA,
                          forever::base_mana( p() ) * VIPER_MANA_PER_SPEED * weapon->swing_time.total_seconds(),
                          p()->gain.viper_auto, this );
  }
};

struct start_auto_shot_t : public action_t
{
  start_auto_shot_t( hunter_t* p, util::string_view options_str ) : action_t( ACTION_OTHER, "auto_shot_start", p )
  {
    parse_options( options_str );
    trigger_gcd           = 0_ms;
    harmful               = false;
    ignore_false_positive = true;

    if ( !p->auto_shot || p->equipped_weapons[ SLOT_RANGED ].type == WEAPON_NONE )
      background = true;
  }

  hunter_t* p() const { return debug_cast<hunter_t*>( player ); }

  // Starting Auto Shot fires after the 0.5 s aim (or when the current cast allows it), not a full cycle later
  void execute() override
  {
    auto shot = debug_cast<auto_shot_t*>( p()->auto_shot );
    shot->delay_override = std::max( AUTO_SHOT_AIM, p()->auto_shot_blocked_until() - sim->current_time() );
    shot->schedule_execute();
  }

  bool ready() override
  {
    if ( player->is_moving() )
      return false;
    return p()->auto_shot->execute_event == nullptr;
  }
};

// --------------------------------------------------------------------------
// Melee
// --------------------------------------------------------------------------

struct hunter_melee_t : public hunter_attack_t
{
  hunter_melee_t( hunter_t* p, weapon_t* w ) : hunter_attack_t( "melee", p, spell_data_t::nil() )
  {
    weapon            = w;
    weapon_multiplier = 1.0;
    base_execute_time = w->swing_time;
    school            = SCHOOL_PHYSICAL;
    trigger_gcd       = 0_ms;
    background = repeating = true;
    may_glance = may_crit = may_dodge = may_parry = true;
    special    = false;
  }

  void execute() override
  {
    // Raptor Strike replaces the swing (on next swing), if there's still mana for it
    if ( p()->buff.raptor_strike_queued->check() )
    {
      p()->buff.raptor_strike_queued->expire();
      if ( p()->raptor_strike->cooldown->up() && p()->resource_available( RESOURCE_MANA, p()->raptor_strike->cost() ) )
      {
        p()->raptor_strike->execute_on_target( target );
        schedule_execute();
        return;
      }
    }

    hunter_attack_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    hunter_attack_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    // Deadly Aspects: Quick Strikes from Aspect of the Beast (white melee) or Falcon
    if ( p()->talent.deadly_aspects.ok() && p()->buff.aspect_of_the_beast->check() )
      p()->buff.quick_strikes->trigger( 1, buff_t::DEFAULT_VALUE(), p()->talent.deadly_aspects->effectN( 2 ).percent() );
    if ( p()->talent.deadly_aspects.ok() && p()->buff.aspect_of_the_falcon->check() )
      p()->buff.quick_strikes->trigger( 1, buff_t::DEFAULT_VALUE(), p()->talent.deadly_aspects->effectN( 1 ).percent() );

    if ( p()->buff.aspect_of_the_viper->check() )
      p()->resource_gain( RESOURCE_MANA,
                          forever::base_mana( p() ) * VIPER_MANA_PER_SPEED * weapon->swing_time.total_seconds(),
                          p()->gain.viper_auto, this );
  }
};

struct hunter_auto_attack_t : public hunter_attack_t
{
  hunter_auto_attack_t( hunter_t* p, util::string_view options_str )
    : hunter_attack_t( "auto_attack", p, spell_data_t::nil() )
  {
    parse_options( options_str );
    trigger_gcd           = 0_ms;
    ignore_false_positive = true;

    if ( p->equipped_weapons[ SLOT_MAIN_HAND ].type == WEAPON_NONE )
    {
      background = true;
      return;
    }

    p->main_hand_attack = new hunter_melee_t( p, &p->equipped_weapons[ SLOT_MAIN_HAND ] );
  }

  void execute() override { player->main_hand_attack->schedule_execute(); }

  bool ready() override
  {
    if ( player->is_moving() )
      return false;
    return player->main_hand_attack->execute_event == nullptr;
  }
};

// --------------------------------------------------------------------------
// Shots
// --------------------------------------------------------------------------

// Normalized ranged weapon damage + bp (no wd% effect), 2.0 s cast (Forever), category cooldown 6 s shared
// with Multi-Shot. Barrage, Mortal Shots and Efficiency come from the spell data.
struct aimed_shot_t : public hunter_ranged_attack_t
{
  aimed_shot_t( hunter_t* p, util::string_view options_str )
    : hunter_ranged_attack_t( "aimed_shot", p, p->find_spell( max_rank( p, aimed_shot_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }
};

// Normalized ranged weapon damage, 3 targets (chain targets), no flat bonus in Forever
struct multi_shot_t : public hunter_ranged_attack_t
{
  multi_shot_t( hunter_t* p, util::string_view options_str )
    : hunter_ranged_attack_t( "multi_shot", p, p->find_spell( max_rank( p, multi_shot_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }
};

// Instant arcane damage, flat bp, no AP coefficient. Category cooldown shared with Summon Hawk; Improved
// Arcane Shot only shortens Arcane Shot's own cooldown (client data).
struct arcane_shot_t : public hunter_ranged_attack_t
{
  arcane_shot_t( hunter_t* p, util::string_view options_str )
    : hunter_ranged_attack_t( "arcane_shot", p, p->find_spell( max_rank( p, arcane_shot_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }
};

// Nature DoT (5 ticks of 3 s). Improved Stings comes from the spell data. Rapid Recuperation: the application
// grants a share of mana regen while casting.
struct serpent_sting_t : public hunter_ranged_attack_t
{
  serpent_sting_t( hunter_t* p, util::string_view options_str )
    : hunter_ranged_attack_t( "serpent_sting", p, p->find_spell( max_rank( p, serpent_sting_ranks ).spell_id ) )
  {
    parse_options( options_str );
    weapon_multiplier = 0;
  }

  void impact( action_state_t* s ) override
  {
    hunter_ranged_attack_t::impact( s );

    if ( result_is_hit( s->result ) && p()->talent.rapid_recuperation.ok() )
      p()->buff.rapid_recuperation->trigger();
  }
};

// 31 point Marksmanship talent: normalized weapon damage + bp, 4.0 s cast, 15 s cooldown
struct sniper_shot_t : public hunter_ranged_attack_t
{
  sniper_shot_t( hunter_t* p, util::string_view options_str )
    : hunter_ranged_attack_t( "sniper_shot", p,
                              p->talent.sniper_shot.ok() ? p->find_spell( max_rank( p, sniper_shot_ranks ).spell_id )
                                                         : spell_data_t::not_found() )
  {
    parse_options( options_str );
  }
};

// 16 point Beast Mastery talent: the dive bomb (bp + 5% RAP, effect #4) and a hawk for 18 s, at most 2
// (effect #3). The hawk's own damage is an ESTIMATE (see hawk_strike_t).
struct summon_hawk_t : public hunter_ranged_attack_t
{
  summon_hawk_t( hunter_t* p, util::string_view options_str )
    : hunter_ranged_attack_t( "summon_hawk", p,
                              p->talent.summon_hawk.ok() ? p->find_spell( max_rank( p, summon_hawk_ranks ).spell_id )
                                                         : spell_data_t::not_found() )
  {
    parse_options( options_str );
    // client Min Travel Time 1 s
    min_travel_time = 1.0;
  }

  // Gated by the talent node; the trained ranks have their own levels
  bool verify_actor_level() const override { return true; }

  double bonus_da( const action_state_t* s ) const override
  {
    return hunter_ranged_attack_t::bonus_da( s ) + data().effectN( 4 ).percent() * s->composite_attack_power();
  }

  double action_multiplier() const override
  {
    return hunter_ranged_attack_t::action_multiplier() * ( 1.0 + p()->talent.unleashed_fury->effectN( 1 ).percent() );
  }

  double composite_crit_chance() const override
  {
    return hunter_ranged_attack_t::composite_crit_chance() + p()->talent.ferocity->effectN( 1 ).percent();
  }

  void execute() override
  {
    hunter_ranged_attack_t::execute();
    p()->summon_hawk();
  }
};

// ESTIMATE: one active hawk's attack, every 2 s for 30% of the dive bomb formula.
struct hawk_strike_t : public hunter_attack_t
{
  const spell_data_t* dive;

  hawk_strike_t( hunter_t* p ) : hunter_attack_t( "hawk_strike", p, spell_data_t::nil() ),
    dive( p->find_spell( max_rank( p, summon_hawk_ranks ).spell_id ) )
  {
    background = true;
    school     = SCHOOL_PHYSICAL;
    may_crit   = true;
    may_glance = false;
    may_parry  = may_block = false;  // attacks from the air, behind the target's facing doesn't matter
    trigger_gcd = 0_ms;
  }

  double base_da_min( const action_state_t* ) const override
  {
    return HAWK_DAMAGE_RATIO * ( dive->effectN( 1 ).base_value() + dive->effectN( 4 ).percent() * p()->ranged_attack_power( target ) );
  }

  double base_da_max( const action_state_t* s ) const override { return base_da_min( s ); }

  double action_multiplier() const override
  {
    return hunter_attack_t::action_multiplier() * ( 1.0 + p()->talent.unleashed_fury->effectN( 1 ).percent() );
  }

  double composite_crit_chance() const override
  {
    return hunter_attack_t::composite_crit_chance() + p()->talent.ferocity->effectN( 1 ).percent();
  }
};

// Channel 6 s, a tick every 1 s fires the rank's arcane damage spell at everything in 8 yd. AoE (phase 2).
// The channel is a spell (ranged_attack_t's scheduling doesn't support channels); each tick is a ranged attack.
struct volley_t : public hunter_spell_t
{
  struct volley_tick_t : public hunter_ranged_attack_t
  {
    volley_tick_t( hunter_t* p, const spell_data_t* s ) : hunter_ranged_attack_t( "volley_tick", p, s )
    {
      background = dual = true;
      aoe        = -1;
      weapon     = nullptr;
      weapon_multiplier  = 0;
      uses_ranged_weapon = true;
    }
  };

  volley_t( hunter_t* p, util::string_view options_str )
    : hunter_spell_t( "volley", p, p->find_spell( max_rank( p, volley_ranks ).spell_id ) )
  {
    parse_options( options_str );
    channeled    = true;
    tick_zero    = false;
    hasted_ticks = false;
    may_miss = may_crit = false;
    base_dd_min = base_dd_max = 0;
    spell_power_mod.direct = 0;
    travel_speed = 0;  // the channel is placed at the target area, the data velocity is for the arrows

    if ( data().ok() )
    {
      tick_action    = new volley_tick_t( p, p->find_spell( max_rank( p, volley_ranks ).damage_id ) );
      base_tick_time = data().effectN( 3 ).period();
      dot_duration   = data().duration();
      add_child( tick_action );
    }
  }
};

// Debuff, 120 s: ranged AP for all attackers against the target. Uses the strongest known rank.
struct hunters_mark_t : public hunter_spell_t
{
  hunters_mark_t( hunter_t* p, util::string_view options_str )
    : hunter_spell_t( "hunters_mark", p, best_value_rank( p, hunters_mark_ranks, 2 ) )
  {
    parse_options( options_str );
    harmful  = false;
    may_miss = may_crit = false;
  }

  void execute() override
  {
    hunter_spell_t::execute();
    td( target )->debuff.hunters_mark->trigger();
  }
};

// --------------------------------------------------------------------------
// Aspects (one at a time) and buffs
// --------------------------------------------------------------------------

struct aspect_t : public hunter_spell_t
{
  buff_t* hunter_t::buffs_t::*aspect;

  aspect_t( hunter_t* p, util::string_view name, const spell_data_t* s, buff_t* hunter_t::buffs_t::*aspect,
            util::string_view options_str )
    : hunter_spell_t( name, p, s ), aspect( aspect )
  {
    parse_options( options_str );
    harmful = false;
    target  = p;
  }

  void execute() override
  {
    hunter_spell_t::execute();

    for ( buff_t* b : { p()->buff.aspect_of_the_hawk, p()->buff.aspect_of_the_beast, p()->buff.aspect_of_the_falcon,
                        p()->buff.aspect_of_the_viper } )
      if ( b != p()->buff.*aspect )
        b->expire();

    ( p()->buff.*aspect )->trigger();
  }

  bool ready() override
  {
    if ( ( p()->buff.*aspect )->check() )
      return false;
    return hunter_spell_t::ready();
  }
};

struct trueshot_aura_t : public hunter_spell_t
{
  trueshot_aura_t( hunter_t* p, util::string_view options_str )
    : hunter_spell_t( "trueshot_aura", p,
                      p->talent.trueshot_aura.ok() ? best_value_rank( p, trueshot_aura_ranks, 1 ) : spell_data_t::not_found() )
  {
    parse_options( options_str );
    harmful = false;
    target  = p;
  }

  bool verify_actor_level() const override { return true; }

  void execute() override
  {
    hunter_spell_t::execute();
    p()->buff.trueshot_aura->trigger();
  }

  bool ready() override
  {
    if ( p()->buff.trueshot_aura->check() )
      return false;
    return hunter_spell_t::ready();
  }
};

// +40% ranged haste (aura 140) and +40% melee auto attack speed (aura 319, Forever). Rapid Killing's
// cooldown reduction comes from the spell data.
struct rapid_fire_t : public hunter_spell_t
{
  rapid_fire_t( hunter_t* p, util::string_view options_str )
    : hunter_spell_t( "rapid_fire", p, p->find_spell( max_rank( p, rapid_fire_ranks ).spell_id ) )
  {
    parse_options( options_str );
    harmful = false;
    target  = p;
  }

  void execute() override
  {
    hunter_spell_t::execute();
    p()->buff.rapid_fire->trigger();
  }
};

struct bestial_wrath_t : public hunter_spell_t
{
  bestial_wrath_t( hunter_t* p, util::string_view options_str )
    : hunter_spell_t( "bestial_wrath", p, p->talent.bestial_wrath.ok() ? p->find_spell( BESTIAL_WRATH ) : spell_data_t::not_found() )
  {
    parse_options( options_str );
    harmful = false;
    target  = p;
  }

  bool verify_actor_level() const override { return true; }

  void execute() override
  {
    hunter_spell_t::execute();
    p()->pets.active->buff.bestial_wrath->trigger();
  }

  bool ready() override
  {
    if ( !p()->pet_active() )
      return false;
    return hunter_spell_t::ready();
  }
};

struct intimidation_t : public hunter_spell_t
{
  intimidation_t( hunter_t* p, util::string_view options_str )
    : hunter_spell_t( "intimidation", p, p->talent.intimidation.ok() ? p->find_spell( INTIMIDATION ) : spell_data_t::not_found() )
  {
    parse_options( options_str );
    harmful = false;
    target  = p;
  }

  bool verify_actor_level() const override { return true; }

  void execute() override
  {
    hunter_spell_t::execute();
    p()->pets.active->buff.intimidation->trigger();
  }

  bool ready() override
  {
    if ( !p()->pet_active() )
      return false;
    return hunter_spell_t::ready();
  }
};

struct summon_pet_t : public hunter_spell_t
{
  std::string type_str = "cat";

  summon_pet_t( hunter_t* p, util::string_view options_str ) : hunter_spell_t( "summon_pet", p, spell_data_t::nil() )
  {
    add_option( opt_string( "type", type_str ) );
    parse_options( options_str );

    harmful     = false;
    trigger_gcd = 0_ms;
    ignore_false_positive = true;

    if ( !util::str_compare_ci( type_str, "cat" ) && !util::str_compare_ci( type_str, "wolf" ) )
      throw std::invalid_argument( fmt::format( "summon_pet: unknown pet type '{}' (cat, wolf)", type_str ) );
  }

  // pets are created after the owner's actions
  pets::hunter_pet_t* pet() const
  { return util::str_compare_ci( type_str, "wolf" ) ? p()->pets.wolf : p()->pets.cat; }

  void execute() override
  {
    hunter_spell_t::execute();
    if ( p()->pets.active && p()->pets.active != pet() )
      p()->pets.active->dismiss();
    pet()->summon();
  }

  bool ready() override
  {
    if ( !pet() || p()->pet_active() )
      return false;
    return hunter_spell_t::ready();
  }
};

// --------------------------------------------------------------------------
// Melee abilities (Survival / weaving)
// --------------------------------------------------------------------------

// On next swing (weapon damage + bp, not normalized), 6 s cooldown, no GCD
struct raptor_strike_t : public hunter_attack_t
{
  raptor_strike_t( hunter_t* p ) : hunter_attack_t( "raptor_strike", p, p->find_spell( max_rank( p, raptor_strike_ranks ).spell_id ) )
  {
    background  = true;
    trigger_gcd = 0_ms;
    weapon      = &p->equipped_weapons[ SLOT_MAIN_HAND ];
  }
};

// APL action that queues Raptor Strike on the next swing (warrior heroic_strike pattern)
struct raptor_strike_queue_t : public action_t
{
  raptor_strike_queue_t( hunter_t* p, util::string_view options_str )
    : action_t( ACTION_OTHER, "raptor_strike_queue", p )
  {
    parse_options( options_str );
    trigger_gcd = 0_ms;
    harmful     = false;
    ignore_false_positive = true;

    if ( !p->raptor_strike->data().ok() || !p->main_hand_attack )
      background = true;
  }

  hunter_t* p() const { return debug_cast<hunter_t*>( player ); }

  bool ready() override
  {
    if ( !p()->main_hand_attack || !p()->main_hand_attack->execute_event )
      return false;
    if ( p()->buff.raptor_strike_queued->check() )
      return false;
    if ( !p()->raptor_strike->cooldown->up() || !p()->resource_available( RESOURCE_MANA, p()->raptor_strike->cost() ) )
      return false;
    return action_t::ready();
  }

  void execute() override { p()->buff.raptor_strike_queued->trigger(); }
};

// Lacerating Strikes bleed: 40% of the Mongoose Bite damage over the bleed's 21 s (residual, rolls over)
struct lacerating_strikes_t : public residual_action::residual_periodic_action_t<hunter_attack_t>
{
  lacerating_strikes_t( hunter_t* p ) : residual_periodic_action_t( "lacerating_strikes", p, p->find_spell( LACERATING_BLEED ) )
  {
    may_miss = may_dodge = may_parry = may_block = false;
    // the amount is a share of the (already multiplied, crit included) Mongoose Bite damage
    tick_may_crit = false;
  }

  void init() override
  {
    residual_periodic_action_t::init();
    snapshot_flags = update_flags = 0;
  }
};

// Normalized weapon damage + bp. Usable after the hunter dodges (not modelled, no incoming damage) or with
// an Expose Prey proc (Forever talent).
struct mongoose_bite_t : public hunter_attack_t
{
  mongoose_bite_t( hunter_t* p, util::string_view options_str )
    : hunter_attack_t( "mongoose_bite", p, p->find_spell( max_rank( p, mongoose_bite_ranks ).spell_id ) )
  {
    parse_options( options_str );
    weapon = &p->equipped_weapons[ SLOT_MAIN_HAND ];
  }

  bool ready() override
  {
    if ( !p()->buff.expose_prey->check() )
      return false;
    return hunter_attack_t::ready();
  }

  // consume the activation first: the bite's own hit may proc a new Expose Prey (Yellow Melee)
  void execute() override
  {
    p()->buff.expose_prey->expire();
    hunter_attack_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    hunter_attack_t::impact( s );

    if ( p()->lacerating_strikes && result_is_hit( s->result ) )
      residual_action::trigger( p()->lacerating_strikes, s->target,
                                p()->talent.lacerating_strikes->effectN( 1 ).percent() * s->result_amount );
  }
};

// 21 point Survival talent: 100% weapon damage (effect #2, plus a normalized effect #1 with 0 bp)
struct strider_kick_t : public hunter_attack_t
{
  strider_kick_t( hunter_t* p, util::string_view options_str )
    : hunter_attack_t( "strider_kick", p, p->talent.strider_kick.ok() ? p->find_spell( STRIDER_KICK ) : spell_data_t::not_found() )
  {
    parse_options( options_str );
    weapon = &p->equipped_weapons[ SLOT_MAIN_HAND ];
  }

  bool verify_actor_level() const override { return true; }
};

struct wing_clip_t : public hunter_attack_t
{
  wing_clip_t( hunter_t* p, util::string_view options_str )
    : hunter_attack_t( "wing_clip", p, p->find_spell( max_rank( p, wing_clip_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }
};

// Bleed, 7 ticks of 3 s, base + points per level from the data
struct lacerate_t : public hunter_attack_t
{
  lacerate_t( hunter_t* p, util::string_view options_str )
    : hunter_attack_t( "lacerate", p, p->find_spell( max_rank( p, lacerate_ranks ).spell_id ) )
  {
    parse_options( options_str );
  }
};

// ==========================================================================
// hunter_t
// ==========================================================================

stat_e hunter_t::convert_hybrid_stat( stat_e s ) const
{
  switch ( s )
  {
    case STAT_STR_AGI_INT:
    case STAT_AGI_INT:
    case STAT_STR_AGI:
      return STAT_AGILITY;
    case STAT_STR_INT:
      return STAT_NONE;
    default:
      return s;
  }
}

void hunter_t::create_options()
{
  player_t::create_options();

  // The core has no option for the ranged slot (SLOT_RANGED = 17) yet; proposed as a shared change in
  // FOREVER_HUNTER.md. Until then the hunter module binds "ranged=" itself.
  add_option( opt_string( "ranged", items[ SLOT_RANGED ].options_str ) );

  add_option( opt_float( "ammo_dps", options.ammo_dps ) );
  add_option( opt_float( "quiver_haste", options.quiver_haste ) );
  add_option( opt_string( "pet_happiness", options.pet_happiness ) );
  add_option( opt_bool( "tracking", options.tracking ) );
}

void hunter_t::copy_from( player_t* source )
{
  player_t::copy_from( source );
  options = debug_cast<hunter_t*>( source )->options;
}

void hunter_t::init_base_stats()
{
  // Vanilla hunter attack power (ChrClasses: 1 AP per Str and Agi): melee Str + Agi + 2 L - 20.
  // Ranged AP (2 Agi + 2 L - 10) is computed in ranged_attack_power().
  base.attack_power_per_strength = 1.0;
  base.attack_power_per_agility  = 1.0;

  player_t::init_base_stats();

  base.stats.attack_power = 2.0 * level() - 20.0;

  // ESTIMATE: vanilla hunters have no base melee/ranged crit beyond Agility (cmangos classic). Passive crit
  // talents (Lethal Attacks, aura 290 "all_crit") still apply.
  base.all_crit           = get_passive_player_value( 0.0, "all_crit" );
  base.attack_crit_chance = get_passive_player_value( base.all_crit, "attack_crit" );
  base.spell_crit_chance  = get_passive_player_value( base.all_crit, "spell_crit" );
}

void hunter_t::init_spells()
{
  player_t::init_spells();

  // Beast Mastery
  talent.deadly_aspects     = forever::talent( this, "Deadly Aspects" );
  talent.focused_fire       = forever::talent( this, "Focused Fire" );
  talent.unleashed_fury     = forever::talent( this, "Unleashed Fury" );
  talent.ferocity           = forever::talent( this, "Ferocity" );
  talent.summon_hawk        = forever::talent( this, "Summon Hawk" );
  talent.intimidation       = forever::talent( this, "Intimidation" );
  talent.bestial_discipline = forever::talent( this, "Bestial Discipline" );
  talent.frenzy             = forever::talent( this, "Frenzy" );
  talent.bestial_wrath      = forever::talent( this, "Bestial Wrath" );

  // Marksmanship
  talent.careful_aim                  = forever::talent( this, "Careful Aim" );
  talent.lone_wolf                    = forever::talent( this, "Lone Wolf" );
  talent.trueshot_aura                = forever::talent( this, "Trueshot Aura" );
  talent.rapid_recuperation           = forever::talent( this, "Rapid Recuperation" );
  talent.ranged_weapon_specialization = forever::talent( this, "Ranged Weapon Specialization" );
  talent.sniper_shot                  = forever::talent( this, "Sniper Shot" );

  // Survival
  talent.improved_tracking  = forever::talent( this, "Improved Tracking" );
  talent.surefooted         = forever::talent( this, "Surefooted" );
  talent.resourcefulness    = forever::talent( this, "Resourcefulness" );
  talent.expose_prey        = forever::talent( this, "Expose Prey" );
  talent.strider_kick       = forever::talent( this, "Strider Kick" );
  talent.lacerating_strikes = forever::talent( this, "Lacerating Strikes" );

  // Talents with conditions or pet/hawk targets are handled in code, keep the auto parser off them:
  // * Lone Wolf (aura 79 +20%) only applies without an active pet
  // * Ranged Weapon Specialization (aura 79) is for ranged weapon damage only
  // * Unleashed Fury / Ferocity are flat "Spell Effect 1" mods meant for the pet passive and the hawks; on
  //   Summon Hawk they would add +15/+10 flat damage
  // * Improved Tracking needs the target's creature type
  register_passive_effect_mask( talent.lone_wolf.spell(), 0b11 );
  register_passive_effect_mask( talent.ranged_weapon_specialization.spell(), 0b1 );
  register_passive_effect_mask( talent.unleashed_fury.spell(), 0b1 );
  register_passive_effect_mask( talent.ferocity.spell(), 0b1 );
  register_passive_effect_mask( talent.improved_tracking.spell(), 0b11 );

  parse_all_class_passives();
  parse_all_passive_talents();

  hunters_mark_value  = best_value_rank( this, hunters_mark_ranks, 2 )->effectN( 2 ).base_value();
  trueshot_aura_value = talent.trueshot_aura.ok() ? best_value_rank( this, trueshot_aura_ranks, 1 )->effectN( 1 ).base_value() : 0.0;
  hawk_aspect_value   = best_value_rank( this, aspect_of_the_hawk_ranks, 1 )->effectN( 1 ).base_value();
}

void hunter_t::create_pets()
{
  player_t::create_pets();

  pets.cat  = new pets::hunter_pet_t( this, "cat", PET_CAT );
  pets.wolf = new pets::hunter_pet_t( this, "wolf", PET_WOLF );
}

void hunter_t::create_actions()
{
  auto_shot     = new auto_shot_t( this );
  raptor_strike = new raptor_strike_t( this );
  if ( talent.summon_hawk.ok() )
    hawk_strike = new hawk_strike_t( this );
  if ( talent.lacerating_strikes.ok() )
    lacerating_strikes = new lacerating_strikes_t( this );

  player_t::create_actions();
}

void hunter_t::create_buffs()
{
  player_t::create_buffs();

  const spell_data_t* hawk = find_spell( max_rank( this, aspect_of_the_hawk_ranks ).spell_id );
  // Aspects: the spell's proc chance (Deadly Aspects target, Falcon 5%) is for the Quick Shots/Strikes proc,
  // not the aura, so the buffs always apply
  buff.aspect_of_the_hawk = make_buff( this, "aspect_of_the_hawk", hawk )
                              ->set_chance( 1.0 )
                              ->set_default_value( hawk_aspect_value )
                              ->add_invalidate( CACHE_ATTACK_POWER );

  const spell_data_t* beast = find_spell( max_rank( this, aspect_of_the_beast_ranks ).spell_id );
  buff.aspect_of_the_beast = make_buff( this, "aspect_of_the_beast", beast )
                               ->set_chance( 1.0 )
                               ->set_default_value( beast->effectN( 2 ).base_value() )
                               ->add_invalidate( CACHE_ATTACK_POWER );

  // Melee and ranged AP equal to the highest known Aspect of the Hawk (description; data bp 0)
  buff.aspect_of_the_falcon = make_buff( this, "aspect_of_the_falcon", find_spell( max_rank( this, aspect_of_the_falcon_ranks ).spell_id ) )
                                ->set_chance( 1.0 )
                                ->set_default_value( hawk_aspect_value )
                                ->add_invalidate( CACHE_ATTACK_POWER );

  // -10% damage done (effect #2), +10% of max mana every 3 s (effect #1, aura period 3 s)
  const spell_data_t* viper = find_spell( max_rank( this, aspect_of_the_viper_ranks ).spell_id );
  buff.aspect_of_the_viper = make_buff( this, "aspect_of_the_viper", viper )
                               ->set_chance( 1.0 )
                               ->set_default_value( viper->effectN( 2 ).percent() )
                               ->set_period( viper->effectN( 1 ).period() > 0_ms ? viper->effectN( 1 ).period() : 3_s )
                               ->set_tick_callback( [ this, viper ]( buff_t*, int, timespan_t ) {
                                   resource_gain( RESOURCE_MANA, viper->effectN( 1 ).percent() * resources.max[ RESOURCE_MANA ],
                                                  gain.viper_periodic );
                                 } );

  buff.trueshot_aura = make_buff( this, "trueshot_aura", best_value_rank( this, trueshot_aura_ranks, 1 ) )
                         ->set_chance( 1.0 )
                         ->set_default_value( trueshot_aura_value );

  const spell_data_t* rf = find_spell( max_rank( this, rapid_fire_ranks ).spell_id );
  buff.rapid_fire = make_buff( this, "rapid_fire", rf )
                      ->set_default_value( rf->effectN( 1 ).percent() )
                      ->set_cooldown( 0_ms )
                      ->add_invalidate( CACHE_AUTO_ATTACK_SPEED );

  buff.quick_shots = make_buff( this, "quick_shots", find_spell( QUICK_SHOTS ) )
                       ->set_default_value( find_spell( QUICK_SHOTS )->effectN( 1 ).percent() );

  buff.quick_strikes = make_buff( this, "quick_strikes", find_spell( QUICK_STRIKES ) )
                         ->set_default_value( find_spell( QUICK_STRIKES )->effectN( 1 ).percent() )
                         ->add_invalidate( CACHE_AUTO_ATTACK_SPEED );

  buff.furious_howl = make_buff( this, "furious_howl", find_spell( max_rank( this, furious_howl_ranks ).spell_id ) )
                        ->add_invalidate( CACHE_ATTACK_POWER );

  buff.expose_prey = make_buff( this, "expose_prey", find_spell( EXPOSE_PREY_BUFF ) )
                       ->set_chance( talent.expose_prey->effectN( 1 ).percent() );

  // Rapid Recuperation: 25/50% (talent effect #1) of mana regen continues while casting, 15 s
  buff.rapid_recuperation = make_buff( this, "rapid_recuperation", find_spell( RAPID_RECUP_BUFF ) )
                              ->set_default_value( talent.rapid_recuperation->effectN( 1 ).percent() );

  // Resourcefulness: on melee/ranged ability crits, 50/100% chance (talent effect #2) of 50% regen while casting
  // (1242688 effect #1, aura 134) for 30 s. The trait's third curve has no spell effect to land on.
  buff.resourcefulness = make_buff( this, "resourcefulness", find_spell( RESOURCEFUL_BUFF ) )
                           ->set_default_value_from_effect_type( A_MOD_MANA_REGEN_INTERRUPT )
                           ->set_chance( talent.resourcefulness->effectN( 2 ).percent() );

  buff.raptor_strike_queued = make_buff( this, "raptor_strike_queued" );

  // Up to 2 hawks (Summon Hawk effect #3), each its own 18 s guardian
  const spell_data_t* hawk_pet = find_spell( SUMMON_HAWK_PET );
  for ( size_t i = 0; i < buff.hawk.size(); i++ )
  {
    buff.hawk[ i ] = make_buff( this, fmt::format( "hawk_{}", i + 1 ), hawk_pet )
                       ->set_duration( hawk_pet->duration() > 0_ms ? hawk_pet->duration() : 18_s )
                       ->set_period( timespan_t::from_seconds( HAWK_SWING_TIME ) )
                       ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                           if ( hawk_strike && target && !target->is_sleeping() )
                             hawk_strike->execute_on_target( target );
                         } );
  }
}

void hunter_t::init_gains()
{
  player_t::init_gains();

  gain.viper_periodic = get_gain( "aspect_of_the_viper" );
  gain.viper_auto     = get_gain( "aspect_of_the_viper_autos" );
}

void hunter_t::init_procs()
{
  player_t::init_procs();

  proc.auto_shot_clipped = get_proc( "auto_shot_clipped" );
}

void hunter_t::reset()
{
  player_t::reset();

  last_cast_end = timespan_t::min();
  pets.active   = nullptr;
}

action_t* hunter_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "auto_shot" )
    return new start_auto_shot_t( this, options_str );
  if ( name == "auto_attack" )
    return new hunter_auto_attack_t( this, options_str );

  if ( name == "aimed_shot" )
  {
    auto a = new aimed_shot_t( this, options_str );
    a->shared_cooldowns.push_back( get_cooldown( "multi_shot" ) );
    return a;
  }
  if ( name == "multi_shot" )
  {
    auto a = new multi_shot_t( this, options_str );
    a->shared_cooldowns.push_back( get_cooldown( "aimed_shot" ) );
    return a;
  }
  if ( name == "arcane_shot" )
  {
    auto a = new arcane_shot_t( this, options_str );
    a->shared_cooldowns.push_back( get_cooldown( "summon_hawk" ) );
    return a;
  }
  if ( name == "summon_hawk" )
  {
    auto a = new summon_hawk_t( this, options_str );
    a->shared_cooldowns.push_back( get_cooldown( "arcane_shot" ) );
    return a;
  }
  if ( name == "serpent_sting" )
    return new serpent_sting_t( this, options_str );
  if ( name == "sniper_shot" )
    return new sniper_shot_t( this, options_str );
  if ( name == "volley" )
    return new volley_t( this, options_str );
  if ( name == "hunters_mark" )
    return new hunters_mark_t( this, options_str );

  if ( name == "aspect_of_the_hawk" )
    return new aspect_t( this, name, find_spell( max_rank( this, aspect_of_the_hawk_ranks ).spell_id ), &buffs_t::aspect_of_the_hawk, options_str );
  if ( name == "aspect_of_the_beast" )
    return new aspect_t( this, name, find_spell( max_rank( this, aspect_of_the_beast_ranks ).spell_id ), &buffs_t::aspect_of_the_beast, options_str );
  if ( name == "aspect_of_the_falcon" )
    return new aspect_t( this, name, find_spell( max_rank( this, aspect_of_the_falcon_ranks ).spell_id ), &buffs_t::aspect_of_the_falcon, options_str );
  if ( name == "aspect_of_the_viper" )
    return new aspect_t( this, name, find_spell( max_rank( this, aspect_of_the_viper_ranks ).spell_id ), &buffs_t::aspect_of_the_viper, options_str );
  if ( name == "trueshot_aura" )
    return new trueshot_aura_t( this, options_str );
  if ( name == "rapid_fire" )
    return new rapid_fire_t( this, options_str );
  if ( name == "bestial_wrath" )
    return new bestial_wrath_t( this, options_str );
  if ( name == "intimidation" )
    return new intimidation_t( this, options_str );
  if ( name == "summon_pet" )
    return new summon_pet_t( this, options_str );

  if ( name == "raptor_strike" )
    return new raptor_strike_queue_t( this, options_str );
  if ( name == "mongoose_bite" )
    return new mongoose_bite_t( this, options_str );
  if ( name == "strider_kick" )
    return new strider_kick_t( this, options_str );
  if ( name == "wing_clip" )
    return new wing_clip_t( this, options_str );
  if ( name == "lacerate" )
    return new lacerate_t( this, options_str );

  return player_t::create_action( name, options_str );
}

void hunter_t::init_action_list()
{
  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }

  clear_action_priority_lists();
  hunter_apl::hunter( this );
  use_default_action_list = true;
  player_t::init_action_list();
}

std::unique_ptr<expr_t> hunter_t::create_expression( util::string_view name )
{
  auto splits = util::string_split<util::string_view>( name, "." );

  if ( splits.size() == 2 && splits[ 0 ] == "auto_shot" )
  {
    // time until the next Auto Shot lands (0 when not shooting)
    if ( splits[ 1 ] == "time_to_next" )
      return make_fn_expr( name, [ this ] {
        if ( !auto_shot || !auto_shot->execute_event )
          return 0.0;
        return ( auto_shot->execute_event->occurs() - sim->current_time() ).total_seconds();
      } );
    // current (hasted) Auto Shot interval
    if ( splits[ 1 ] == "speed" )
      return make_fn_expr( name, [ this ] {
        return ( auto_shot ? auto_shot->base_execute_time * ranged_speed_multiplier() : 0_ms ).total_seconds();
      } );
    // the next shot is in its 0.5 s aim (windup)
    if ( splits[ 1 ] == "in_aim" )
      return make_fn_expr( name, [ this ] {
        if ( !auto_shot || !auto_shot->execute_event )
          return false;
        return auto_shot->execute_event->occurs() - sim->current_time() <= AUTO_SHOT_AIM;
      } );
  }

  if ( name == "pet.active" )
    return make_fn_expr( name, [ this ] { return pet_active(); } );

  if ( name == "active_hawks" )
    return make_fn_expr( name, [ this ] { return active_hawks(); } );

  return player_t::create_expression( name );
}

bool hunter_t::pet_active() const
{
  return pets.active && !pets.active->is_sleeping();
}

double hunter_t::pet_happiness_multiplier() const
{
  if ( util::str_compare_ci( options.pet_happiness, "unhappy" ) )
    return 0.75;
  if ( util::str_compare_ci( options.pet_happiness, "content" ) )
    return 1.0;
  return 1.25;
}

// Hit: the core ignores aura 54 "Modify Hit%" passives and hit rating (composite_melee_hit has the rating
// line commented out). Class-local until the shared change lands: Surefooted (+1-3%, effect #3) and gear
// hit rating through the client combat rating table.
double hunter_t::composite_melee_hit() const
{
  double h = player_t::composite_melee_hit();

  h += talent.surefooted->effectN( 3 ).percent();
  h += composite_melee_hit_rating() / current.rating.attack_hit;

  return h;
}

double hunter_t::composite_melee_attack_power() const
{
  double ap = player_t::composite_melee_attack_power();

  ap += buff.aspect_of_the_beast->check_value();
  ap += buff.aspect_of_the_falcon->check_value();
  ap += buff.furious_howl->check_value();

  // Careful Aim: AP from Intellect (auras 580/598, melee and ranged), unknown to simc's parser
  ap += talent.careful_aim->effectN( 1 ).percent() * cache.intellect();

  return ap;
}

double hunter_t::ranged_attack_power( player_t* t ) const
{
  // base.stats.attack_power holds the melee level base (2 L - 20) plus gear; ranged base is 2 L - 10
  double ap = current.stats.attack_power + 10.0 + 2.0 * cache.agility();

  ap += buff.aspect_of_the_hawk->check_value();
  ap += buff.aspect_of_the_falcon->check_value();
  ap += buff.trueshot_aura->check_value();
  ap += talent.careful_aim->effectN( 2 ).percent() * cache.intellect();

  if ( t )
  {
    if ( const hunter_td_t* td = target_data[ t ] )
      ap += td->debuff.hunters_mark->check_value();
  }

  return std::round( ap * composite_attack_power_multiplier() );
}

double hunter_t::ranged_speed_multiplier() const
{
  double haste = ( 1.0 + buff.quick_shots->check_value() ) * ( 1.0 + buff.rapid_fire->check_value() ) *
                 ( 1.0 + options.quiver_haste / 100.0 );
  return 1.0 / haste;
}

double hunter_t::composite_melee_auto_attack_speed() const
{
  double s = player_t::composite_melee_auto_attack_speed();

  s /= 1.0 + buff.rapid_fire->check_value();
  s /= 1.0 + buff.quick_strikes->check_value();

  return s;
}

double hunter_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  // Lone Wolf: +20% (effect #2) while no pet is active
  if ( talent.lone_wolf.ok() && !pet_active() )
    m *= 1.0 + talent.lone_wolf->effectN( 2 ).percent();

  // Focused Fire: +1/2% while the pet is active
  if ( talent.focused_fire.ok() && pet_active() )
    m *= 1.0 + talent.focused_fire->effectN( 1 ).percent();

  // Improved Tracking: +1-5% damage against the tracked creature type (option tracking, default on: every
  // raid boss type is trackable)
  if ( options.tracking && talent.improved_tracking.ok() )
    m *= 1.0 + talent.improved_tracking->effectN( 1 ).percent();

  m *= 1.0 + buff.aspect_of_the_viper->check_value();

  return m;
}

double hunter_t::composite_player_critical_damage_multiplier( const action_state_t* s, school_e school ) const
{
  double m = player_t::composite_player_critical_damage_multiplier( s, school );

  // Improved Tracking: the trait has a second curve (1-5, "Effect#2") but the spell has a single effect; the spec
  // reads it as +1-5% critical strike damage. FLAG: applied with the effect #1 value until verified.
  if ( options.tracking && talent.improved_tracking.ok() )
    m *= 1.0 + talent.improved_tracking->effectN( 1 ).percent();

  return m;
}

double hunter_t::composite_mana_regen_while_casting() const
{
  // Bestial Discipline (aura 134 passive) is in the base value
  double f = player_t::composite_mana_regen_while_casting();

  f += buff.rapid_recuperation->check_value();
  f += buff.resourcefulness->check_value();

  return std::min( f, 1.0 );
}

timespan_t hunter_t::auto_shot_blocked_until() const
{
  timespan_t now = sim->current_time();
  timespan_t end = now;

  if ( executing && executing->execute_event && executing->time_to_execute > 0_ms )
    end = executing->execute_event->occurs();
  else if ( channeling )
    end = now + channeling->get_dot()->remains();
  else if ( last_cast_end >= 0_ms && last_cast_end + AUTO_SHOT_AIM > now )
    return last_cast_end + AUTO_SHOT_AIM;
  else
    return now;

  return end + AUTO_SHOT_AIM;
}

// Expose Prey: melee and ranged hits against a Hunter's Mark target have a 5/10% chance to enable Mongoose Bite
void hunter_t::trigger_expose_prey( const action_state_t* s )
{
  if ( !talent.expose_prey.ok() || !s->target )
    return;

  if ( const hunter_td_t* td = target_data[ s->target ]; td && td->debuff.hunters_mark->check() )
    buff.expose_prey->trigger();
}

void hunter_t::trigger_resourcefulness( const action_state_t* s )
{
  if ( !talent.resourcefulness.ok() || s->result != RESULT_CRIT || !s->action->special )
    return;

  buff.resourcefulness->trigger();
}

void hunter_t::summon_hawk()
{
  if ( !hawk_strike )
    return;

  // a free hawk slot, else replace the hawk with the least time left
  buff_t* slot = nullptr;
  for ( buff_t* b : buff.hawk )
  {
    if ( !b->check() )
    {
      slot = b;
      break;
    }
    if ( !slot || b->remains() < slot->remains() )
      slot = b;
  }

  slot->expire();
  slot->trigger();
}

int hunter_t::active_hawks() const
{
  int n = 0;
  for ( const buff_t* b : buff.hawk )
    n += b->check() ? 1 : 0;
  return n;
}

// ==========================================================================
// Module
// ==========================================================================

struct hunter_module_t : public module_t
{
  hunter_module_t() : module_t( HUNTER ) {}

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    return new hunter_t( sim, name, r );
  }

  bool valid() const override { return true; }

  void register_hotfixes() const override {}

  void register_actor_initializers( sim_t* ) const override {}
};
}  // UNNAMED NAMESPACE

const module_t* module_t::hunter()
{
  static hunter_module_t m;
  return &m;
}

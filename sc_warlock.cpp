// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Warlock
// ==========================================================================

struct warlock_t : public player_t
{
  enum pet_type_t { PET_NONE=0, FELGUARD, FELHUNTER, IMP, VOIDWALKER, SUCCUBUS };

  spell_t* active_corruption;
  spell_t* active_curse;
  spell_t* active_immolate;

  // Buffs
  int8_t buffs_amplify_curse;
  int8_t buffs_flame_shadow;
  int8_t buffs_nightfall;
  int8_t buffs_pet_active;
  int8_t buffs_pet_sacrifice;
  int8_t buffs_shadow_flame;
  int8_t buffs_shadow_vulnerability;
  int8_t buffs_shadow_vulnerability_charges;

  // Expirations
  event_t* expirations_shadow_flame;
  event_t* expirations_flame_shadow;
  event_t* expirations_shadow_vulnerability;

  // Gains
  gain_t* gains_dark_pact;
  gain_t* gains_life_tap;
  gain_t* gains_sacrifice;

  // Procs
  proc_t* procs_nightfall;

  // Up-Times
  uptime_t* uptimes_shadow_flame;
  uptime_t* uptimes_flame_shadow;
  uptime_t* uptimes_shadow_vulnerability;

  struct talents_t
  {
    int8_t  amplify_curse;
    int8_t  backlash;
    int8_t  bane;
    int8_t  cataclysm;
    int8_t  conflagrate;
    int8_t  contagion;
    int8_t  dark_pact;
    int8_t  demonic_aegis;
    int8_t  demonic_embrace;
    int8_t  demonic_knowledge;
    int8_t  demonic_tactics;
    int8_t  demonic_sacrifice;
    int8_t  destructive_reach;
    int8_t  devastation;
    int8_t  emberstorm;
    int8_t  empowered_corruption;
    int8_t  fel_domination;
    int8_t  fel_intellect;
    int8_t  fel_stamina;
    int8_t  improved_curse_of_agony;
    int8_t  improved_corruption;
    int8_t  improved_drain_soul;
    int8_t  improved_fire_bolt;
    int8_t  improved_immolate;
    int8_t  improved_imp;
    int8_t  improved_lash_of_pain;
    int8_t  improved_life_tap;
    int8_t  improved_searing_pain;
    int8_t  improved_shadow_bolt;
    int8_t  improved_succubus;
    int8_t  improved_voidwalker;
    int8_t  malediction;
    int8_t  mana_feed;
    int8_t  master_conjuror;
    int8_t  master_demonologist;
    int8_t  master_summoner;
    int8_t  nightfall;
    int8_t  ruin;
    int8_t  shadow_and_flame;
    int8_t  shadow_burn;
    int8_t  shadow_mastery;
    int8_t  siphon_life;
    int8_t  soul_link;
    int8_t  soul_leech;
    int8_t  soul_siphon;
    int8_t  summon_felguard;
    int8_t  suppression;
    int8_t  unholy_power;
    int8_t  unstable_affliction;
    
    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int8_t blue_promises;
    glyphs_t() { memset( (void*) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  warlock_t( sim_t* sim, std::string& name ) : player_t( sim, WARLOCK, name ) 
  {
    // Active
    active_corruption = 0;
    active_immolate   = 0;

    // Buffs
    buffs_amplify_curse                = 0;
    buffs_flame_shadow                 = 0;
    buffs_nightfall                    = 0;
    buffs_pet_active                   = 0;
    buffs_pet_sacrifice                = 0;
    buffs_shadow_flame                 = 0;
    buffs_shadow_vulnerability         = 0;
    buffs_shadow_vulnerability_charges = 0;

    // Expirations
    expirations_flame_shadow           = 0;
    expirations_shadow_flame           = 0;
    expirations_shadow_vulnerability   = 0;

    // Gains
    gains_dark_pact = get_gain( "dark_pact" );
    gains_life_tap  = get_gain( "life_tap"  );
    gains_sacrifice = get_gain( "sacrifice" );

    // Procs
    procs_nightfall = get_proc( "nightfall" );

    // Up-Times
    uptimes_flame_shadow         = get_uptime( "flame_shadow" );
    uptimes_shadow_flame         = get_uptime( "shadow_flame" );
    uptimes_shadow_vulnerability = get_uptime( "shadow_vulnerability" );
  }

  // Character Definition
  virtual void      init_base();
  virtual void      reset();
  virtual void      parse_talents( const std::string& talent_string );
  virtual bool      parse_option ( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );
  virtual int       primary_resource() { return RESOURCE_MANA; }

  // Event Tracking
  virtual void regen( double periodicity );
};

// ==========================================================================
// Warlock Spell
// ==========================================================================

struct warlock_spell_t : public spell_t
{
  warlock_spell_t( const char* n, player_t* player, int8_t s, int8_t t ) : 
    spell_t( n, player, RESOURCE_MANA, s, t ) 
  {
  }

  // Overridden Methods
  virtual double haste();
  virtual void   player_buff();
  virtual void   execute();

  // Passthru Methods
  virtual double cost()          { return spell_t::cost();         }
  virtual double execute_time()  { return spell_t::execute_time(); }
  virtual void   tick()          { spell_t::tick();                }
  virtual void   last_tick()     { spell_t::last_tick();           }
  virtual bool   ready()         { return spell_t::ready();        }
};

// ==========================================================================
// Pet Voidwalker
// ==========================================================================

struct voidwalker_pet_t : public pet_t
{
  struct melee_t : public attack_t
  {
    melee_t( player_t* player ) : 
      attack_t( "melee", player, RESOURCE_NONE, SCHOOL_SHADOW )
    {
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_dd = 1;
      background = true;
      repeating = true;
    }
    virtual void execute()
    {
      attack_t::execute();
      if( result_is_hit() )
      {
	if( ! sim_t::WotLK )
	{
	  enchant_t::trigger_flametongue_totem( this );
	  enchant_t::trigger_windfury_totem( this );
	}
      }
    }
    void player_buff()
    {
      attack_t::player_buff();

      player_t* o = player -> cast_pet() -> owner;

      player_power += 0.57 * ( o -> spell_power[ SCHOOL_MAX    ] + 
			       o -> spell_power[ SCHOOL_SHADOW ] );

      // Arbitrary until I figure out base stats.
      player_crit = 0.05;
    }
  };

  melee_t* melee;

  voidwalker_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name ) :
    pet_t( sim, owner, pet_name )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.damage     = 110;
    main_hand_weapon.swing_time = 1.5;

    melee = new melee_t( this );
  }
  virtual void init_base()
  {
    attribute_base[ ATTR_STRENGTH  ] = 153;
    attribute_base[ ATTR_AGILITY   ] = 108;
    attribute_base[ ATTR_STAMINA   ] = 280;
    attribute_base[ ATTR_INTELLECT ] = 133;

    base_attack_power = -20;
    initial_attack_power_per_strength = 2.0;
  }
  virtual void reset()
  {
    player_t::reset();
  }
  virtual void schedule_ready()
  {
    assert(0);
  }
  virtual void summon()
  {
    player_t* o = cast_pet() -> owner;
    if( sim -> log ) report_t::log( sim, "%s summons Voidwalker.", o -> name() );
    attribute_initial[ ATTR_STAMINA   ] = attribute[ ATTR_STAMINA   ] = attribute_base[ ATTR_STAMINA   ] + ( 0.30 * o -> attribute[ ATTR_STAMINA   ] );
    attribute_initial[ ATTR_INTELLECT ] = attribute[ ATTR_INTELLECT ] = attribute_base[ ATTR_INTELLECT ] + ( 0.30 * o -> attribute[ ATTR_INTELLECT ] );
    // Kick-off repeating attack
    melee -> execute();
  }
  virtual void dismiss()
  {
    if( sim -> log ) report_t::log( sim, "%s's Voidwalker is dismissed.", cast_pet() -> owner -> name() );
    melee -> cancel();
  }
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// trigger_tier5_4pc ========================================================

static void trigger_tier5_4pc( spell_t* s,
			       spell_t* dot_spell )
{
  warlock_t* p = s -> player -> cast_warlock();

  if( p -> gear.tier5_4pc )
  {
    if( dot_spell )
    {
      // Increase remaining DoT -BASE- damage by 10%

      double original_base_dot = dot_spell -> base_dot;

      dot_spell -> base_dot *= 1.10;
      
      double delta_base_dot = ( dot_spell -> base_dot - original_base_dot );
      
      dot_spell -> dot += delta_base_dot * dot_spell -> base_multiplier * dot_spell -> player_multiplier;
    }
  }
}

// trigger_tier4_2pc ========================================================

static void trigger_tier4_2pc( spell_t* s )
{
  struct expiration_t : public event_t
  {
    int8_t school;
    expiration_t( sim_t* sim, warlock_t* p, spell_t* s ) : event_t( sim, p ), school( s -> school )
    {
      name = "Tier4 2-Piece Buff Expiration";
      if( school == SCHOOL_SHADOW )
      {
	p -> aura_gain( "flame_shadow" );
	p -> buffs_flame_shadow = 1;
      }
      else
      {
	p -> aura_gain( "shadow_flame" );
	p -> buffs_shadow_flame = 1;
      }
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      warlock_t* p = player -> cast_warlock();
      if( school == SCHOOL_SHADOW )
      {
	p -> aura_loss( "flame_shadow" );
	p -> buffs_flame_shadow = 0;
	p -> expirations_flame_shadow = 0;
      }
      else
      {
	p -> aura_loss( "shadow_flame" );
	p -> buffs_shadow_flame = 0;
	p -> expirations_shadow_flame = 0;
      }
    }
  };
  
  warlock_t* p = s -> player -> cast_warlock();

  if( p -> gear.tier4_2pc )
  {
    if( rand_t::roll( 0.05 ) )
    {
      event_t*& e = ( s -> school == SCHOOL_SHADOW ) ? p -> expirations_shadow_flame : p -> expirations_flame_shadow;

      if( e )
      {
	e -> reschedule( 15.0 );
      }
      else
      {
	e = new expiration_t( s -> sim, p, s );
      }
    }
  }
}

// trigger_improved_shadow_bolt =============================================

static void trigger_improved_shadow_bolt( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim ) : event_t( sim )
    {
      name = "Shadow Vulnerability Expiration";
      target_t* t = sim -> target;
      if( sim -> log ) report_t::log( sim, "Target %s loses Shadow Vulnerability", t -> name() );
      sim -> add_event( this, 12.0 );
    }
    virtual void execute()
    {
      target_t* t = sim -> target;
      if( sim -> log ) report_t::log( sim, "Target %s loses Shadow Vulnerability", t -> name() );
      t -> debuffs.shadow_vulnerability = 0;
      t -> debuffs.shadow_vulnerability_charges = 0;
      t -> expirations.shadow_vulnerability = 0;
    }
  };
  
  warlock_t* p = s -> player -> cast_warlock();

  if( p -> talents.improved_shadow_bolt )
  {
    target_t* t = s -> sim -> target;
    t -> debuffs.shadow_vulnerability = p -> talents.improved_shadow_bolt * 4;
    t -> debuffs.shadow_vulnerability_charges = 4;
    
    event_t*& e = t -> expirations.shadow_vulnerability;

    if( e )
    {
      e -> reschedule( 12.0 );
    }
    else
    {
      e = new expiration_t( s -> sim );
    }
  }
}

// trigger_nightfall ========================================================

static void trigger_nightfall( spell_t* s )
{
  warlock_t* p = s -> player -> cast_warlock();

  if( p -> talents.nightfall && ! p -> buffs_nightfall )
  {
    if( rand_t::roll( 0.02 * p -> talents.nightfall ) )
    {
      p -> procs_nightfall -> occur();
      p -> aura_gain( "Nightfall" );
      p -> buffs_nightfall = 1;
    }
  }
}

// trigger_soul_leech =======================================================

static void trigger_soul_leech( spell_t* s )
{
  warlock_t* p = s -> player -> cast_warlock();

  if( p -> talents.soul_leech )
  {
    if( rand_t::roll( 0.10 * p -> talents.soul_leech ) )
    {
      p -> resource_gain( RESOURCE_HEALTH, s -> dd * 0.30 );
    }
  }
}

// trigger_ashtongue_talisman ===============================================

static void trigger_ashtongue_talisman( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Ashtongue Talisman Expiration";
      player -> aura_gain( "Ashtongue Talisman" );
      player -> spell_power[ SCHOOL_MAX ] += 220;
      sim -> add_event( this, 5.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Ashtongue Talisman" );
      player -> spell_power[ SCHOOL_MAX ] -= 220;
      player -> expirations.ashtongue_talisman = 0;
    }
  };

  player_t* p = s -> player;

  if( p -> gear.ashtongue_talisman && rand_t::roll( 0.20 ) )
  {
    p -> procs.ashtongue_talisman -> occur();

    event_t*& e = p -> expirations.ashtongue_talisman;

    if( e )
    {
      e -> reschedule( 5.0 );
    }
    else
    {
      e = new expiration_t( s -> sim, p );
    }
  }
}

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Warlock Spell
// ==========================================================================

// warlock_spell_t::haste ===================================================

double warlock_spell_t::haste()
{
  return spell_t::haste();
}

// warlock_spell_t::player_buff =============================================

void warlock_spell_t::player_buff()
{
  warlock_t* p = player -> cast_warlock();
  spell_t::player_buff();
  if( school == SCHOOL_SHADOW )
  {
    if( p -> buffs_pet_sacrifice == warlock_t::FELGUARD ) player_multiplier *= 1.10;
    if( p -> buffs_pet_sacrifice == warlock_t::SUCCUBUS ) player_multiplier *= 1.15;
    if( p -> buffs_flame_shadow ) player_power += 135;
    p -> uptimes_flame_shadow -> update( p -> buffs_flame_shadow );
  }
  else if( school == SCHOOL_FIRE )
  {
    if( p -> buffs_pet_sacrifice == warlock_t::IMP ) player_multiplier *= 1.15;
    if( p -> buffs_shadow_flame ) player_power += 135;
    p -> uptimes_shadow_flame -> update( p -> buffs_shadow_flame );
  }
  if( p -> talents.master_demonologist )
  {
    if( p -> buffs_pet_sacrifice == warlock_t::FELGUARD ) player_multiplier *= 1.0 + p -> talents.master_demonologist * 0.01;
    if( p -> buffs_pet_sacrifice == warlock_t::SUCCUBUS ) player_multiplier *= 1.0 + p -> talents.master_demonologist * 0.02;
  }
  if( p -> talents.demonic_tactics && 
      p -> buffs_pet_active != warlock_t::PET_NONE )
  {
    player_crit += p -> talents.demonic_tactics * 0.01;
  }
}

// warlock_spell_t::execute ==================================================

void warlock_spell_t::execute()
{
  spell_t::execute();
  if( result_is_hit() )
  {
    trigger_tier4_2pc( this );
  }
}

// Curse of Elements Spell ===================================================

struct curse_of_elements_t : public warlock_spell_t
{
  curse_of_elements_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "curse_of_elements", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 67, 3, 0, 0, 0, 260 },
      { 56, 2, 0, 0, 0, 200 },
      { 0,  0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 0; 
    base_cost         = rank -> cost;
    base_hit          = p -> talents.suppression * 0.02;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, warlock_t* p ) : event_t( sim, p )
      {
	name = "Cure of Elements Expiration";
	target_t* t = sim -> target;
	t -> debuffs.curse_of_elements = 10 + p -> talents.malediction;
	t -> debuffs.affliction_effects++;
	sim -> add_event( this, 300.0 );
      }
      virtual void execute()
      {
	warlock_t* p = player -> cast_warlock();
	target_t*  t = sim -> target;
	p -> active_curse = 0;
	t -> debuffs.curse_of_elements = 0;
	t -> expirations.curse_of_elements = 0;
	t -> debuffs.affliction_effects--;
      }
    };

    warlock_spell_t::execute(); 

    if( result_is_hit() )
    {
      warlock_t* p = player -> cast_warlock();
      target_t* t = sim -> target;
      event_t::early( t -> expirations.curse_of_elements );
      t -> expirations.curse_of_elements = new expiration_t( sim, p );
      p -> active_curse = this;
    }
  }
  
  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if( p -> active_curse != 0 )
      return false;

    if( ! warlock_spell_t::ready() ) 
      return false;

    target_t* t = sim -> target;

    return std::max( t -> debuffs.curse_of_elements, t -> debuffs.earth_and_moon ) < 10 + p -> talents.malediction;
  }
};

// Curse of Agony Spell ===========================================================

struct curse_of_agony_t : public warlock_spell_t
{
  curse_of_agony_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "curse_of_agony", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 67, 7, 0, 0, 1356, 265 },
      { 58, 6, 0, 0, 1044, 215 },
      { 48, 5, 0, 0,  780, 170 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
      
    base_execute_time = 0; 
    base_duration     = 24.0; 
    num_ticks         = 8;
    dot_power_mod     = 1.2;

    base_cost = rank -> cost;
    base_multiplier *= 1.0 + p -> talents.shadow_mastery * 0.02;
    base_multiplier *= 1.0 + p -> talents.contagion * 0.01;
    base_multiplier *= 1.0 + p -> talents.improved_curse_of_agony * 0.05;
    base_hit        += p -> talents.suppression * 0.02;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    base_dot = rank -> dot;
    if( p -> buffs_amplify_curse ) base_dot *= 1.50;
    warlock_spell_t::execute();
    if( result_is_hit() )
    {
      p -> active_curse = this;
      sim -> target -> debuffs.affliction_effects++;
    }
    p -> aura_loss( "Amplify Curse" );
    p -> buffs_amplify_curse = 0;
  }

  virtual void last_tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::last_tick(); 
    p -> active_curse = 0;
    sim -> target -> debuffs.affliction_effects--;
  }
  
  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if( p -> active_curse != 0 )
      return false;

    if( ! warlock_spell_t::ready() ) 
      return false;

    return true;
  }
};

// Curse of Doom Spell ===========================================================

struct curse_of_doom_t : public warlock_spell_t
{
  curse_of_doom_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "curse_of_doom", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 70, 2, 0, 0, 4200, 380 },
      { 60, 1, 0, 0, 3200, 300 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
      
    base_execute_time = 0; 
    base_duration     = 60.0; 
    num_ticks         = 1;
    dot_power_mod     = 2.0;

    base_cost = rank -> cost;
    base_hit  += p -> talents.suppression * 0.02;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    base_dot = rank -> dot;
    if( p -> buffs_amplify_curse ) base_dot *= 1.50;
    warlock_spell_t::execute();
    if( result_is_hit() )
    {
      p -> active_curse = this;
      sim -> target -> debuffs.affliction_effects++;
    }
    p -> aura_loss( "Amplify Curse" );
    p -> buffs_amplify_curse = 0;
  }

  virtual void last_tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::last_tick(); 
    p -> active_curse = 0;
    sim -> target -> debuffs.affliction_effects--;
  }
  
  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if( p -> active_curse != 0 )
      return false;

    if( ! warlock_spell_t::ready() ) 
      return false;

    return true;
  }
};

// Amplify Curse Spell =======================================================

struct amplify_curse_t : public warlock_spell_t
{
  amplify_curse_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "amplify_curse", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();
    assert( p -> talents.amplify_curse );
    trigger_gcd = 0;
    cooldown    = 180;
  }

  virtual void execute() 
  {
    warlock_t* p = player -> cast_warlock();
    p -> aura_gain( "Amplify Curse" );
    p -> buffs_amplify_curse = 1;
    update_ready();
  }
};

// Shadow Bolt Spell ===========================================================

struct shadow_bolt_t : public warlock_spell_t
{
  int32_t nightfall;

  shadow_bolt_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "shadow_bolt", player, SCHOOL_SHADOW, TREE_DESTRUCTION ), nightfall(0)
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "rank",      OPT_INT8, &rank_index },
      { "nightfall", OPT_INT8, &nightfall  },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 69, 11, 541, 603, 0, 420 },
      { 60, 10, 482, 538, 0, 380 },
      { 60,  9, 455, 507, 0, 370 },
      { 52,  8, 360, 402, 0, 315 },
      { 44,  7, 281, 315, 0, 265 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 3.0; 
    may_crit          = true;
    dd_power_mod      = (3.0/3.5); 
      
    base_cost          = rank -> cost;
    base_cost         *= 1.0 -  p -> talents.cataclysm * 0.01;
    base_execute_time -=  p -> talents.bane * 0.1;
    base_multiplier   *= 1.0 + p -> talents.shadow_mastery * 0.02;
    base_crit         += p -> talents.devastation * 0.01;
    base_crit         += p -> talents.backlash * 0.01;
    dd_power_mod      += p -> talents.shadow_and_flame * 0.04;
    if( p -> talents.ruin ) base_crit_bonus *= 2.0;

    if( p -> gear.tier6_4pc ) base_multiplier *= 1.06;
  }

  virtual double execute_time()
  {
    warlock_t* p = player -> cast_warlock();
    if( p -> buffs_nightfall ) return 0;
    return warlock_spell_t::execute_time();
  }
  
  virtual void schedule_execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::schedule_execute(); 
    if( p -> buffs_nightfall )
    {
      p -> aura_loss( "Nightfall" );
      p -> buffs_nightfall = 0;
    }
  }
  
  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    warlock_spell_t::execute(); 

    if( result_is_hit() )
    {
      trigger_soul_leech( this );
      trigger_tier5_4pc( this, p -> active_corruption );
      if( result == RESULT_CRIT ) 
      {
	trigger_improved_shadow_bolt( this );
      }
    }
  }
  
  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if( ! warlock_spell_t::ready() )
      return false;

    if( nightfall )
      if( ! p -> buffs_nightfall )
	return false;

    return true;
  }
};

// Death Coil Spell ===========================================================

struct death_coil_t : public warlock_spell_t
{
  death_coil_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "death_coil", player, SCHOOL_SHADOW, TREE_DESTRUCTION )
  {
    warlock_t* p = player -> cast_warlock();
    
    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 68, 4, 519, 519, 0, 600 },
      { 58, 3, 400, 400, 0, 480 },
      { 50, 2, 319, 319, 0, 420 },
      { 42, 1, 244, 244, 0, 365 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
      
    base_execute_time = 0; 
    may_crit          = true; 
    binary            = true; 
    cooldown          = 120;
    dd_power_mod      = ( 1.5 / 3.5 ) / 2.0; 
      
    base_cost        = rank -> cost;
    base_cost       *= 1.0 -  p -> talents.cataclysm * 0.01;
    base_multiplier *= 1.0 + p -> talents.shadow_mastery * 0.02;
    if( p -> talents.ruin ) base_crit_bonus *= 2.0;
  }

  virtual void execute() 
  {
    warlock_spell_t::execute();
    player -> resource_gain( RESOURCE_HEALTH, dd );
  }
};

// Shadow Burn Spell ===========================================================

struct shadow_burn_t : public warlock_spell_t
{
  shadow_burn_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "shadow_burn", player, SCHOOL_SHADOW, TREE_DESTRUCTION )
  {
    warlock_t* p = player -> cast_warlock();

    assert( p -> talents.shadow_burn );

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 68, 4, 519, 519, 0, 600 },
      { 58, 3, 400, 400, 0, 480 },
      { 50, 2, 319, 319, 0, 420 },
      { 42, 1, 244, 244, 0, 365 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
      
    may_crit     = true; 
    cooldown     = 15;
    dd_power_mod = ( 1.5 / 3.5 ); 
      
    base_cost        = rank -> cost;
    base_cost       *= 1.0 -  p -> talents.cataclysm * 0.01;
    base_multiplier *= 1.0 + p -> talents.shadow_mastery * 0.02;
    if( p -> talents.ruin ) base_crit_bonus *= 2.0;
   }

  virtual void execute()
  {
    warlock_spell_t::execute(); 
    if( result_is_hit() )
    {
      trigger_soul_leech( this );
    }
  }
};

// Corruption Spell ===========================================================

struct corruption_t : public warlock_spell_t
{
  double initial_base_dot;

  corruption_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "corruption", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 65, 8, 0, 0, 900, 370 },
      { 60, 7, 0, 0, 822, 340 },
      { 54, 6, 0, 0, 666, 290 },
      { 44, 5, 0, 0, 486, 225 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );

    base_dot          = rank -> dot;
    base_execute_time = 2.0; 
    base_duration     = 18.0; 
    num_ticks         = 6;
    dot_power_mod     = 0.93;

    if( p -> gear.tier4_4pc )
    {
      double adjust = ( num_ticks + 1 ) / (double) num_ticks;

      base_dot      *= adjust;
      dot_power_mod *= adjust;
      base_duration *= adjust;
      num_ticks     += 1;
    }
    initial_base_dot = base_dot;
      
    base_cost          = rank -> cost;
    base_execute_time -= p -> talents.improved_corruption * 0.4;
    base_multiplier   *= 1.0 + p -> talents.shadow_mastery * 0.02;
    base_multiplier   *= 1.0 + p -> talents.contagion * 0.01;
    base_hit          += p -> talents.suppression * 0.02;
    dot_power_mod     += p -> talents.empowered_corruption * 0.12;

    p -> active_corruption = this;
  }

  virtual void execute()
  {
    base_dot = initial_base_dot;
    warlock_spell_t::execute();
  }

  virtual void tick()
  {
    warlock_spell_t::tick(); 
    trigger_nightfall( this );
    trigger_ashtongue_talisman( this );
    if( player -> gear.tier6_2pc ) player -> resource_gain( RESOURCE_HEALTH, 70 );
  }

  virtual void last_tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::last_tick(); 
    p -> active_corruption = 0;
  }
};

// Drain Life Spell ===========================================================

struct drain_life_t : public warlock_spell_t
{
  drain_life_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "drain_life", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 69, 8, 0, 0, 540, 425 },
      { 62, 7, 0, 0, 435, 355 },
      { 54, 6, 0, 0, 355, 300 },
      { 46, 5, 0, 0, 275, 240 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 0; 
    base_duration     = 5.0; 
    num_ticks         = 5; 
    channeled         = true; 
    binary            = true;
    dot_power_mod     = (5.0/3.5)/2.0; 

    base_cost        = rank -> cost;
    base_multiplier *= 1.0 + p -> talents.shadow_mastery * 0.02;
    base_hit        += p -> talents.suppression * 0.02;
  }

  virtual void player_buff()
  {
    warlock_spell_t::player_buff();

    warlock_t* p = player -> cast_warlock();
    target_t*  t = sim -> target;

    double base_multiplier[] = { 0, 0.02, 0.04 };
    double  max_multiplier[] = { 0, 0.24, 0.60 };

    assert( p -> talents.soul_siphon >= 0 &&
	    p -> talents.soul_siphon <= 2 );

    double base = base_multiplier[ p -> talents.soul_siphon ];
    double max  =  max_multiplier[ p -> talents.soul_siphon ];

    double multiplier = t -> debuffs.affliction_effects * base;
    if( multiplier > max ) multiplier = max;

    player_multiplier *= 1.0 + multiplier;    
  }

  virtual void tick()
  {
    warlock_spell_t::tick(); 
    trigger_nightfall( this );
  }
};

// Siphon Life Spell ===========================================================

struct siphon_life_t : public warlock_spell_t
{
  siphon_life_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "siphon_life", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();

    assert( p -> talents.siphon_life );

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 70, 6, 0, 0, 630, 410 },
      { 63, 5, 0, 0, 520, 350 },
      { 58, 4, 0, 0, 450, 310 },
      { 48, 3, 0, 0, 330, 250 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
     
    base_execute_time = 0; 
    base_duration     = 30.0; 
    num_ticks         = 10; 
    binary            = true;
    dot_power_mod     = (30.0/15.0)/2.0; 

    base_cost        = rank -> cost;
    base_multiplier *= 1.0 + p -> talents.shadow_mastery * 0.02;
    base_hit        += p -> talents.suppression * 0.02;
  }

  virtual void tick()
  {
    warlock_spell_t::tick(); 
    player -> resource_gain( RESOURCE_HEALTH, dot_tick );
  }
};

// Unstable Affliction Spell ======================================================

struct unstable_affliction_t : public warlock_spell_t
{
  unstable_affliction_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "unstable_affliction", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();

    assert( p -> talents.unstable_affliction );
    
    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 70, 3,  0, 0, 1050, 400 },
      { 60, 2,  0, 0,  930, 315 },
      { 50, 1,  0, 0,  660, 270 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 1.5; 
    base_duration     = 18.0; 
    num_ticks         = 6;
    dot_power_mod     = (18.0/15.0); 
    
    base_cost        = rank -> cost;
    base_multiplier *= 1.0 + p -> talents.shadow_mastery * 0.02;
    base_hit        += p -> talents.suppression * 0.02;
  }
};

// Immolate Spell =============================================================

struct immolate_t : public warlock_spell_t
{
  double initial_base_dot;

  immolate_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "immolate", player, SCHOOL_FIRE, TREE_DESTRUCTION )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 69, 9, 327, 327, 615, 445 },
      { 60, 8, 279, 279, 510, 380 },
      { 60, 7, 258, 258, 485, 370 },
      { 50, 6, 192, 192, 365, 295 },
      { 40, 5, 134, 134, 255, 220 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );

    base_dot          = rank -> dot;
    base_execute_time = 2.0; 
    may_crit          = true;
    base_duration     = 15.0; 
    num_ticks         = 5;
    dd_power_mod      = 0.20; 
    dot_power_mod     = 0.65; 

    if( p -> gear.tier4_4pc )
    {
      double adjust = ( num_ticks + 1 ) / (double) num_ticks;

      base_dot      *= adjust;
      dot_power_mod *= adjust;
      base_duration *= adjust;
      num_ticks++;
    }
    initial_base_dot = base_dot;
      
    base_cost          = rank -> cost;
    base_cost         *= 1.0 -  p -> talents.cataclysm * 0.01;
    base_dd            = ( rank -> dd_min + rank -> dd_max ) / 2.0;
    base_dd           *= 1.0 + p -> talents.improved_immolate * 0.05;
    base_execute_time -= p -> talents.bane * 0.1;
    base_multiplier   *= 1.0 + p -> talents.emberstorm * 0.02;
    base_crit         += p -> talents.devastation * 0.01;
    base_crit         += p -> talents.backlash * 0.01;
    if( p -> talents.ruin ) base_crit_bonus *= 2.0;

    p -> active_immolate = this;
  }

  virtual void execute()
  {
    base_dot = initial_base_dot;
    warlock_spell_t::execute();
  }

  virtual void tick()
  {
    warlock_spell_t::tick(); 
    if( player -> gear.tier6_2pc ) player -> resource_gain( RESOURCE_HEALTH, 35 );
  }

  virtual void last_tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::last_tick(); 
    p -> active_immolate = 0;
  }
};

// Conflagrate Spell =========================================================

struct conflagrate_t : public warlock_spell_t
{
  conflagrate_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "conflagrate", player, SCHOOL_FIRE, TREE_DESTRUCTION )
  {
    warlock_t* p = player -> cast_warlock();

    assert( p -> talents.conflagrate );

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 70, 6, 579, 721, 0, 305 },
      { 65, 5, 512, 638, 0, 280 },
      { 60, 4, 447, 557, 0, 255 },
      { 54, 3, 373, 479, 0, 230 },
      { 48, 2, 316, 396, 0, 200 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
     
    base_execute_time = 0; 
    may_crit          = true;
    dd_power_mod      = (1.5/3.5);

    base_cost        = rank -> cost;
    base_cost       *= 1.0 -  p -> talents.cataclysm * 0.01;
    base_multiplier *= 1.0 + p -> talents.emberstorm * 0.02;
    base_crit       += p -> talents.devastation * 0.01;
    base_crit       += p -> talents.backlash * 0.01;
    if( p -> talents.ruin ) base_crit_bonus = 2.0;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute(); 
    if( result_is_hit() )
    {
      trigger_soul_leech( this );
    }
    p -> active_immolate -> cancel();
    p -> active_immolate = 0;
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if( ! spell_t::ready() ) 
      return false;

    if( ! p -> active_immolate )
      return false;

    int ticks_remaining = ( p -> active_immolate -> num_ticks - 
			    p -> active_immolate -> current_tick );

    return( ticks_remaining == 1 );
  }
};

// Incinerate Spell =========================================================

struct incinerate_t : public warlock_spell_t
{
  incinerate_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "incinerate", player, SCHOOL_FIRE, TREE_DESTRUCTION )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 70, 2, 429, 497, 0, 300 },
      { 64, 1, 357, 413, 0, 256 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
     
    base_execute_time = 2.5; 
    may_crit          = true;
    dd_power_mod      = (2.5/3.5); 

    base_cost        = rank -> cost;
    base_cost       *= 1.0 -  p -> talents.cataclysm * 0.01;
    base_multiplier *= 1.0 + p -> talents.emberstorm * 0.02;
    base_crit       += p -> talents.devastation * 0.01;
    base_crit       += p -> talents.backlash * 0.01;
    dd_power_mod    += p -> talents.shadow_and_flame * 0.04;
    if( p -> talents.ruin ) base_crit_bonus *= 2.0;

    if( p -> gear.tier6_4pc ) base_multiplier *= 1.06;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    base_dd = 0;
    get_base_damage();
    if( p -> active_immolate )
    {
      switch( rank -> index )
      {
      case 2: base_dd += 116; break;
      case 1: base_dd += 97;  break;
      default: assert(0);
      }
    }
    warlock_spell_t::execute(); 
    if( result_is_hit() )
    {
      trigger_soul_leech( this );
      trigger_tier5_4pc( this, p -> active_immolate );
    }
  }
};

// Searing Pain Spell =========================================================

struct searing_pain_t : public warlock_spell_t
{
  searing_pain_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "searing_pain", player, SCHOOL_FIRE, TREE_DESTRUCTION )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 70, 8, 270, 320, 0, 205 },
      { 65, 7, 243, 287, 0, 191 },
      { 58, 6, 204, 240, 0, 168 },
      { 50, 5, 158, 188, 0, 141 },
      { 42, 4, 122, 146, 0, 118 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
     
    base_execute_time = 1.5; 
    may_crit          = true;
    dd_power_mod      = (1.5/3.5); 

    base_cost        = rank -> cost;
    base_cost       *= 1.0 -  p -> talents.cataclysm * 0.01;
    base_multiplier *= 1.0 + p -> talents.emberstorm * 0.02;
    base_crit       += p -> talents.devastation * 0.01;
    base_crit       += p -> talents.backlash * 0.01;
    base_crit       += p -> talents.improved_searing_pain * 0.04;
    if( p -> talents.ruin ) base_crit_bonus *= 2.0;
  }

  virtual void execute()
  {
    warlock_spell_t::execute(); 
    if( result_is_hit() )
    {
      trigger_soul_leech( this );
    }
  }
};

// Soul Fire Spell ============================================================

struct soul_fire_t : public warlock_spell_t
{
  soul_fire_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "soul_fire", player, SCHOOL_FIRE, TREE_DESTRUCTION )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 70, 4, 1003, 1257, 0, 455 },
      { 64, 3,  839, 1051, 0, 390 },
      { 56, 2,  703,  881, 0, 335 },
      { 48, 1,  623,  783, 0, 305 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
     
    base_execute_time = 6.0; 
    may_crit          = true; 
    cooldown          = 60;
    dd_power_mod      = (6.0/3.5); 

    base_cost          = rank -> cost;
    base_cost         *= 1.0 -  p -> talents.cataclysm * 0.01;
    base_execute_time -= p -> talents.bane * 0.4;
    base_multiplier   *= 1.0 + p -> talents.emberstorm * 0.02;
    base_crit         += p -> talents.devastation * 0.01;
    base_crit         += p -> talents.backlash * 0.01;
    if( p -> talents.ruin ) base_crit_bonus *= 1.0;
  }

  virtual void execute()
  {
    warlock_spell_t::execute(); 
    if( result_is_hit() )
    {
      trigger_soul_leech( this );
    }
  }
};

// Life Tap Spell ===========================================================

struct life_tap_t : public warlock_spell_t
{
  life_tap_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "life_tap", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();

    static rank_t ranks[] =
    {
      { 68, 7, 580, 580, 0, 0 },
      { 56, 6, 420, 420, 0, 0 },
      { 46, 5, 300, 300, 0, 0 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 0.0; 
    dd_power_mod      = 0.80;
    base_multiplier  *= 1.0 + p -> talents.shadow_mastery * 0.02;

    get_base_damage();
  }

  virtual void execute() 
  {
    warlock_t* p = player -> cast_warlock();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    player_buff();
    double dmg = ( base_dd + ( dd_power_mod * p -> composite_spell_power( SCHOOL_SHADOW ) ) ) * base_multiplier * player_multiplier;
    p -> resource_loss( RESOURCE_HEALTH, dmg );
    p -> resource_gain( RESOURCE_MANA, dmg * ( 1.0 + p -> talents.improved_life_tap * 0.10 ), p -> gains_life_tap );
  }

  virtual bool ready()
  {
    return( player -> resource_current[ RESOURCE_MANA ] < 1000 );
  }
};

// Dark Pact Spell =========================================================

struct dark_pact_t : public warlock_spell_t
{
  dark_pact_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "dark_pact", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();

    static rank_t ranks[] =
    {
      { 70, 3, 700, 700, 0, 0 },
      { 60, 2, 545, 545, 0, 0 },
      { 50, 1, 440, 440, 0, 0 },
	{ 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );

    base_execute_time = 0.0; 
    dd_power_mod      = 0.96;
    base_multiplier  *= 1.0 + p -> talents.shadow_mastery * 0.02;

    get_base_damage();
  }

  virtual void execute() 
  {
    warlock_t* p = player -> cast_warlock();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    player_buff();
    double mana = ( base_dd + ( dd_power_mod * p -> composite_spell_power( SCHOOL_SHADOW ) ) ) * base_multiplier * player_multiplier;
    p -> resource_gain( RESOURCE_MANA, mana, p -> gains_dark_pact );
  }

  virtual bool ready()
  {
    return( player -> resource_current[ RESOURCE_MANA ] < 1000 );
  }
};

// ==========================================================================
// Warlock Event Tracking
// ==========================================================================

// ==========================================================================
// Warlock Character Definition
// ==========================================================================

// warlock_t::create_action =================================================

action_t* warlock_t::create_action( const std::string& name,
				    const std::string& options_str )
{
  if( name == "amplify_curse"       ) return new       amplify_curse_t( this, options_str );
  if( name == "conflagrate"         ) return new         conflagrate_t( this, options_str );
  if( name == "corruption"          ) return new          corruption_t( this, options_str );
  if( name == "curse_of_agony"      ) return new      curse_of_agony_t( this, options_str );
  if( name == "curse_of_doom"       ) return new       curse_of_doom_t( this, options_str );
  if( name == "curse_of_elements"   ) return new   curse_of_elements_t( this, options_str );
  if( name == "dark_pact"           ) return new           dark_pact_t( this, options_str );
  if( name == "death_coil"          ) return new          death_coil_t( this, options_str );
  if( name == "drain_life"          ) return new          drain_life_t( this, options_str );
//if( name == "fel_armor"           ) return new           fel_armor_t( this, options_str );
  if( name == "immolate"            ) return new            immolate_t( this, options_str );
  if( name == "incinerate"          ) return new          incinerate_t( this, options_str );
  if( name == "life_tap"            ) return new            life_tap_t( this, options_str );
  if( name == "shadow_bolt"         ) return new         shadow_bolt_t( this, options_str );
  if( name == "shadow_burn"         ) return new         shadow_burn_t( this, options_str );
  if( name == "searing_pain"        ) return new        searing_pain_t( this, options_str );
  if( name == "soul_fire"           ) return new           soul_fire_t( this, options_str );
  if( name == "siphon_life"         ) return new         siphon_life_t( this, options_str );
  if( name == "unstable_affliction" ) return new unstable_affliction_t( this, options_str );

  return 0;
}

// warlock_t::create_pet =====================================================

pet_t* warlock_t::create_pet( const std::string& pet_name )
{
  if( pet_name == "voidwalker" ) return new voidwalker_pet_t( sim, this, pet_name );

  return 0;
}

// warlock_t::init_base ======================================================

void warlock_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] =  40;
  attribute_base[ ATTR_AGILITY   ] =  45;
  attribute_base[ ATTR_STAMINA   ] =  60;
  attribute_base[ ATTR_INTELLECT ] = 145;
  attribute_base[ ATTR_SPIRIT    ] = 155;

  base_spell_crit = 0.0125;
  initial_spell_crit_per_intellect = rating_t::interpolate( level, 0.01/60.0, 0.01/80.0, 0.01/166.6 );

  base_attack_power = -10;
  base_attack_crit  = 0.03;
  initial_attack_power_per_strength = 1.0;
  initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/16.0, 0.01/24.9, 0.01/52.1 );

  // FIXME! Make this level-specific.
  resource_base[ RESOURCE_HEALTH ] = 3200;
  resource_base[ RESOURCE_MANA   ] = rating_t::interpolate( level, 1383, 2620, 3863 );

  health_per_stamina = 10;
  mana_per_intellect = 15;
}

// warlock_t::reset ==========================================================

void warlock_t::reset()
{
  warlock_t::reset();

  // Active
  active_corruption = 0;
  active_immolate   = 0;

  // Buffs
  buffs_amplify_curse                = 0;
  buffs_flame_shadow                 = 0;
  buffs_nightfall                    = 0;
  buffs_pet_active                   = 0;
  buffs_pet_sacrifice                = 0;
  buffs_shadow_flame                 = 0;
  buffs_shadow_vulnerability         = 0;
  buffs_shadow_vulnerability_charges = 0;

  // Expirations
  expirations_flame_shadow           = 0;
  expirations_shadow_flame           = 0;
  expirations_shadow_vulnerability   = 0;
}

// warlock_t::regen ==========================================================

void warlock_t::regen( double periodicity )
{
  double spirit_regen = periodicity * spirit_regen_per_second();

  if( buffs.innervate )
  {
    spirit_regen *= 4.0;
  }
  else if( recent_cast() )
  {
    spirit_regen = 0;
  }

  double mp5_regen = periodicity * mp5 / 5.0;

  resource_gain( RESOURCE_MANA, spirit_regen, gains.spirit_regen );
  resource_gain( RESOURCE_MANA,    mp5_regen, gains.mp5_regen    );

  if( buffs_pet_sacrifice == FELGUARD )
  {
    double sacrifice_regen = periodicity * resource_max[ RESOURCE_MANA ] * 0.02 / 4.0;

    resource_gain( RESOURCE_MANA, sacrifice_regen, gains_sacrifice );
  }

  if( buffs_pet_sacrifice == FELHUNTER )
  {
    double sacrifice_regen = periodicity * resource_max[ RESOURCE_MANA ] * 0.03 / 4.0;

    resource_gain( RESOURCE_MANA, sacrifice_regen, gains_sacrifice );
  }

  if( buffs.replenishment )
  {
    double replenishment_regen = periodicity * resource_max[ RESOURCE_MANA ] * 0.005 / 1.0;

    resource_gain( RESOURCE_MANA, replenishment_regen, gains.replenishment );
  }

  if( buffs.water_elemental_regen )
  {
    double water_elemental_regen = periodicity * resource_max[ RESOURCE_MANA ] * 0.006 / 5.0;

    resource_gain( RESOURCE_MANA, water_elemental_regen, gains.water_elemental_regen );
  }
}

// warlock_t::parse_talents ================================================

void warlock_t::parse_talents( const std::string& talent_string )
{
  if( talent_string.size() == 64 )
  {
    talent_translation_t translation[] =
    {
      {  1,  &( talents.suppression             ) },
      {  2,  &( talents.improved_corruption     ) },
      {  4,  &( talents.improved_drain_soul     ) },
      {  5,  &( talents.improved_life_tap       ) },
      {  6,  &( talents.soul_siphon             ) },
      {  7,  &( talents.improved_curse_of_agony ) },
      {  9,  &( talents.amplify_curse           ) },
      { 11,  &( talents.nightfall               ) },
      { 12,  &( talents.empowered_corruption    ) },
      { 14,  &( talents.siphon_life             ) },
      { 16,  &( talents.shadow_mastery          ) },
      { 17,  &( talents.contagion               ) },
      { 18,  &( talents.dark_pact               ) },
      { 20,  &( talents.malediction             ) },
      { 21,  &( talents.unstable_affliction     ) },
      { 23,  &( talents.improved_imp            ) },
      { 24,  &( talents.demonic_embrace         ) },
      { 26,  &( talents.improved_voidwalker     ) },
      { 27,  &( talents.fel_intellect           ) },
      { 28,  &( talents.improved_succubus       ) },
      { 29,  &( talents.fel_domination          ) },
      { 30,  &( talents.fel_stamina             ) },
      { 31,  &( talents.demonic_aegis           ) },
      { 32,  &( talents.master_summoner         ) },
      { 33,  &( talents.unholy_power            ) },
      { 35,  &( talents.demonic_sacrifice       ) },
      { 36,  &( talents.master_conjuror         ) },
      { 37,  &( talents.mana_feed               ) },
      { 38,  &( talents.master_demonologist     ) },
      { 40,  &( talents.soul_link               ) },
      { 41,  &( talents.demonic_knowledge       ) },
      { 42,  &( talents.demonic_tactics         ) },
      { 43,  &( talents.summon_felguard         ) },
      { 44,  &( talents.improved_shadow_bolt    ) },
      { 45,  &( talents.cataclysm               ) },
      { 46,  &( talents.bane                    ) },
      { 48,  &( talents.improved_fire_bolt      ) },
      { 49,  &( talents.improved_lash_of_pain   ) },
      { 50,  &( talents.devastation             ) },
      { 51,  &( talents.shadow_burn             ) },
      { 53,  &( talents.destructive_reach       ) },
      { 54,  &( talents.improved_searing_pain   ) },
      { 56,  &( talents.improved_immolate       ) },
      { 57,  &( talents.ruin                    ) },
      { 59,  &( talents.emberstorm              ) },
      { 60,  &( talents.backlash                ) },
      { 61,  &( talents.conflagrate             ) },
      { 62,  &( talents.soul_leech              ) },
      { 63,  &( talents.shadow_and_flame        ) },
      { 0, NULL }
    };
    player_t::parse_talents( translation, talent_string );
  }
  else if( talent_string.size() == 81 )
  {
    talent_translation_t translation[] =
    {
      { 0, NULL }
    };
    player_t::parse_talents( translation, talent_string );
  }
  else
  {
    fprintf( sim -> output_file, "Malformed Warlock talent string.  Number encoding should have length 64 for Burning Crusade or 80 for Wrath of the Lich King.\n" );
    assert( 0 );
  }
}

// warlock_t::parse_option =================================================

bool warlock_t::parse_option( const std::string& name,
			      const std::string& value )
{
  option_t options[] =
  {
    { "amplify_curse",           OPT_INT8,  &( talents.amplify_curse           ) },
    { "backlash",                OPT_INT8,  &( talents.backlash                ) },
    { "bane",                    OPT_INT8,  &( talents.bane                    ) },
    { "cataclysm",               OPT_INT8,  &( talents.cataclysm               ) },
    { "conflagrate",             OPT_INT8,  &( talents.conflagrate             ) },
    { "contagion",               OPT_INT8,  &( talents.contagion               ) },
    { "dark_pact",               OPT_INT8,  &( talents.dark_pact               ) },
    { "demonic_aegis",           OPT_INT8,  &( talents.demonic_aegis           ) },
    { "demonic_embrace",         OPT_INT8,  &( talents.demonic_embrace         ) },
    { "demonic_knowledge",       OPT_INT8,  &( talents.demonic_knowledge       ) },
    { "demonic_sacrifice",       OPT_INT8,  &( talents.demonic_sacrifice       ) },
    { "demonic_tactics",         OPT_INT8,  &( talents.demonic_tactics         ) },
    { "destructive_reach",       OPT_INT8,  &( talents.destructive_reach       ) },
    { "devastation",             OPT_INT8,  &( talents.devastation             ) },
    { "emberstorm",              OPT_INT8,  &( talents.emberstorm              ) },
    { "empowered_corruption",    OPT_INT8,  &( talents.empowered_corruption    ) },
    { "fel_domination",          OPT_INT8,  &( talents.fel_domination          ) },
    { "fel_intellect",           OPT_INT8,  &( talents.fel_intellect           ) },
    { "fel_stamina",             OPT_INT8,  &( talents.fel_stamina             ) },
    { "improved_curse_of_agony", OPT_INT8,  &( talents.improved_curse_of_agony ) },
    { "improved_corruption",     OPT_INT8,  &( talents.improved_corruption     ) },
    { "improved_drain_soul",     OPT_INT8,  &( talents.improved_drain_soul     ) },
    { "improved_fire_bolt",      OPT_INT8,  &( talents.improved_fire_bolt      ) },
    { "improved_immolate",       OPT_INT8,  &( talents.improved_immolate       ) },
    { "improved_imp",            OPT_INT8,  &( talents.improved_imp            ) },
    { "improved_lash_of_pain",   OPT_INT8,  &( talents.improved_lash_of_pain   ) },
    { "improved_life_tap",       OPT_INT8,  &( talents.improved_life_tap       ) },
    { "improved_searing_pain",   OPT_INT8,  &( talents.improved_searing_pain   ) },
    { "improved_shadow_bolt",    OPT_INT8,  &( talents.improved_shadow_bolt    ) },
    { "improved_succubus",       OPT_INT8,  &( talents.improved_succubus       ) },
    { "improved_voidwalker",     OPT_INT8,  &( talents.improved_voidwalker     ) },
    { "malediction",             OPT_INT8,  &( talents.malediction             ) },
    { "mana_feed",               OPT_INT8,  &( talents.mana_feed               ) },
    { "master_conjuror",         OPT_INT8,  &( talents.master_conjuror         ) },
    { "master_demonologist",     OPT_INT8,  &( talents.master_demonologist     ) },
    { "master_summoner",         OPT_INT8,  &( talents.master_summoner         ) },
    { "nightfall",               OPT_INT8,  &( talents.nightfall               ) },
    { "ruin",                    OPT_INT8,  &( talents.ruin                    ) },
    { "shadow_and_flame",        OPT_INT8,  &( talents.shadow_and_flame        ) },
    { "shadow_burn",             OPT_INT8,  &( talents.shadow_burn             ) },
    { "shadow_mastery",          OPT_INT8,  &( talents.shadow_mastery          ) },
    { "siphon_life",             OPT_INT8,  &( talents.siphon_life             ) },
    { "soul_leech",              OPT_INT8,  &( talents.soul_leech              ) },
    { "soul_link",               OPT_INT8,  &( talents.soul_link               ) },
    { "soul_siphon",             OPT_INT8,  &( talents.soul_siphon             ) },
    { "summon_felguard",         OPT_INT8,  &( talents.summon_felguard         ) },
    { "suppression",             OPT_INT8,  &( talents.suppression             ) },
    { "unholy_power",            OPT_INT8,  &( talents.unholy_power            ) },
    { "unstable_affliction",     OPT_INT8,  &( talents.unstable_affliction     ) },
    // Glyphs
    { "glyph_blue_promises",       OPT_INT8,  &( glyphs.blue_promises              ) },
    { NULL, OPT_UNKNOWN }
  };

  if( name.empty() )
  {
    player_t::parse_option( std::string(), std::string() );
    option_t::print( sim, options );
    return false;
  }

  if( player_t::parse_option( name, value ) ) return true;

  return option_t::parse( sim, options, name, value );
}

// player_t::create_warlock ================================================

player_t* player_t::create_warlock( sim_t*       sim, 
				    std::string& name ) 
{
  return new warlock_t( sim, name );
}



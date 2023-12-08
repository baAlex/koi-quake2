/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * The "camera" through which the player looks into the game.
 *
 * =======================================================================
 */

#include "../header/local.h"
#include "../header/new.h"
#include "../monster/misc/player.h"


static void sPlayPainSound(edict_t* ent)
{
	/* 'pain_debounce_time' is shared between multiple player sounds,
	    acting like a voice allocator, as all sounds use the same channel */
	if (ent->health <= 0 || level.time < ent->pain_debounce_time)
		return;

	int l;
	if (ent->health < 25)
		l = 25;
	else if (ent->health < 50)
		l = 50;
	else if (ent->health < 75)
		l = 75;
	else
		l = 100;

	const int r = 1 + (randk() & 1);
	gi.sound(ent, CHAN_VOICE, gi.soundindex(va("*pain%i_%i.wav", l, r)), 1, ATTN_NORM, 0);
	ent->pain_debounce_time = level.time + 0.7f;
}

static void sPlayBurnSound(edict_t* ent)
{
	if (ent->health <= 0 || level.time < ent->pain_debounce_time) /* Like here */
		return;

	if (randk() & 1)
	{
		gi.sound(ent, CHAN_VOICE,
		gi.soundindex("player/burn1.wav"), 1, ATTN_NORM, 0);
	}
	else
	{
		gi.sound(ent, CHAN_VOICE,
		gi.soundindex("player/burn2.wav"), 1, ATTN_NORM, 0);
	}

	ent->pain_debounce_time = level.time + 1.0f;
}

static void sPlayComputerSound(edict_t* ent)
{
	/* Here we wait, but without setting the timer later, thrus
	   lowering priority of this sound */
	if (level.time > ent->pain_debounce_time)
		gi.sound(ent, CHAN_VOICE, gi.soundindex("misc/pc_up.wav"), 1, ATTN_STATIC, 0);
}


static inline float sDamageFlashAlphaFormula(const edict_t* ent)
{
	/* It has to transmit current health in a plain simpler manner */
	return max((float)(ent->max_health - ent->health) / (float)(ent->max_health), DAMAGE_FLASH_MIN);
}


static void sDamageFeedback(edict_t* ent)
{
	/* Clear Hud flashes, just in case */ /* baAlex, TODO: move to hud.c */
	ent->client->ps.stats[STAT_FLASHES] = 0;

	/* Is player in a mode where feedback doesn't matter? */
	if ((ent->flags & FL_GODMODE) || ent->client->invincible_framenum > level.framenum)
	{
		ent->client->damage_blood = 0; /* Clear damage */
		return;
	}

	/* Flash screen if health is critical */
	if (ent->health > 0 && ent->health < CRITICAL_HEALTH_FLASH_AT &&
	    ent->client->wait2 < level.time)
	{
		ent->client->damage_alpha = max(ent->client->damage_alpha, CRITICAL_HEALTH_FLASH);
		ent->client->wait2 = level.time + CRITICAL_HEALTH_FLASH_DELAY;
	}

	/* Is there damage to react to? */
	if (ent->client->damage_blood <= 0)
		return;

	sPlayPainSound(ent);
	ent->client->ps.stats[STAT_FLASHES] |= 1; /* Flash the Hud */ /* baAlex, TODO: move to hud.c */
	ent->client->damage_alpha = max(ent->client->damage_alpha, sDamageFlashAlphaFormula(ent));

	/* Clear damage */
	ent->client->damage_blood = 0;
	ent->client->damage_armor = 0;      /* baAlex, TODO: Remove */
	ent->client->damage_parmor = 0;     /* Ditto */
	ent->client->damage_knockback = 0;  /* Ditto */

	/* Death/gib sound is now aggregated and played here */
	/*if (ent->sounds)
	{
		gi.sound (ent, CHAN_VOICE, ent->sounds, 1, ATTN_NORM, 0);
		ent->sounds = 0;
	}*/

	/* Start a pain animation if still in the player model */
	if ((ent->client->anim_priority < ANIM_PAIN) && (ent->s.modelindex == 255))
	{
		static int i; /* baAlex, dear god, FIXME! */

		ent->client->anim_priority = ANIM_PAIN;

		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			ent->s.frame = FRAME_crpain1 - 1;
			ent->client->anim_end = FRAME_crpain4;
		}
		else
		{
			i = (i + 1) % 3;

			switch (i)
			{
				case 0:
					ent->s.frame = FRAME_pain101 - 1;
					ent->client->anim_end = FRAME_pain104;
					break;
				case 1:
					ent->s.frame = FRAME_pain201 - 1;
					ent->client->anim_end = FRAME_pain204;
					break;
				case 2:
					ent->s.frame = FRAME_pain301 - 1;
					ent->client->anim_end = FRAME_pain304;
					break;
			}
		}
	}
}

void
SV_AddBlend(float r, float g, float b, float a, float *v_blend)
{
	float a2, a3;

	if (!v_blend)
	{
		return;
	}

	if (a <= 0)
	{
		return;
	}

	a2 = v_blend[3] + (1 - v_blend[3]) * a; /* new total alpha */
	a3 = v_blend[3] / a2; /* fraction of color from old */

	v_blend[0] = v_blend[0] * a3 + r * (1 - a3);
	v_blend[1] = v_blend[1] * a3 + g * (1 - a3);
	v_blend[2] = v_blend[2] * a3 + b * (1 - a3);
	v_blend[3] = a2;
}

void
SV_CalcBlend(edict_t *ent)
{
	int contents;
	vec3_t vieworg;
	int remaining;

	if (!ent)
	{
		return;
	}

	ent->client->ps.blend[0] = ent->client->ps.blend[1] =
		ent->client->ps.blend[2] = ent->client->ps.blend[3] = 0;

	/* add for contents */
	VectorAdd(ent->s.origin, ent->client->ps.viewoffset, vieworg);
	contents = gi.pointcontents(vieworg);

	if (contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER))
	{
		ent->client->ps.rdflags |= RDF_UNDERWATER;
	}
	else
	{
		ent->client->ps.rdflags &= ~RDF_UNDERWATER;
	}

	if (contents & (CONTENTS_SOLID | CONTENTS_LAVA))
	{
		SV_AddBlend(1.0, 0.3, 0.0, 0.6, ent->client->ps.blend);
	}
	else if (contents & CONTENTS_SLIME)
	{
		SV_AddBlend(0.0, 0.1, 0.05, 0.6, ent->client->ps.blend);
	}
	else if (contents & CONTENTS_WATER)
	{
		SV_AddBlend(0.5, 0.3, 0.2, 0.4, ent->client->ps.blend);
	}

	/* add for powerups */
	if (ent->client->quad_framenum > level.framenum)
	{
		remaining = ent->client->quad_framenum - level.framenum;

		if (remaining == 30) /* beginning to fade */
		{
			gi.sound(ent, CHAN_ITEM, gi.soundindex(
							"items/damage2.wav"), 1, ATTN_NORM, 0);
		}

		if ((remaining > 30) || (remaining & 4))
		{
			SV_AddBlend(0, 0, 1, 0.08, ent->client->ps.blend);
		}
	}
	else if (ent->client->invincible_framenum > level.framenum)
	{
		remaining = ent->client->invincible_framenum - level.framenum;

		if (remaining == 30) /* beginning to fade */
		{
			gi.sound(ent, CHAN_ITEM, gi.soundindex(
							"items/protect2.wav"), 1, ATTN_NORM, 0);
		}

		if ((remaining > 30) || (remaining & 4))
		{
			SV_AddBlend(1, 1, 0, 0.08, ent->client->ps.blend);
		}
	}
	else if (ent->client->enviro_framenum > level.framenum)
	{
		remaining = ent->client->enviro_framenum - level.framenum;

		if (remaining == 30) /* beginning to fade */
		{
			gi.sound(ent, CHAN_ITEM, gi.soundindex(
							"items/airout.wav"), 1, ATTN_NORM, 0);
		}

		if ((remaining > 30) || (remaining & 4))
		{
			SV_AddBlend(0, 1, 0, 0.08, ent->client->ps.blend);
		}
	}
	else if (ent->client->breather_framenum > level.framenum)
	{
		remaining = ent->client->breather_framenum - level.framenum;

		if (remaining == 30) /* beginning to fade */
		{
			gi.sound(ent, CHAN_ITEM, gi.soundindex(
							"items/airout.wav"), 1, ATTN_NORM, 0);
		}

		if ((remaining > 30) || (remaining & 4))
		{
			SV_AddBlend(0.4, 1, 0.4, 0.04, ent->client->ps.blend);
		}
	}

	/* Screen blend */
	if (ent->client->damage_alpha > 0)
	{
		ent->client->damage_alpha = min(ent->client->damage_alpha, DAMAGE_FLASH_MAX);
		SV_AddBlend(DAMAGE_FLASH_COLOR[0], DAMAGE_FLASH_COLOR[1], DAMAGE_FLASH_COLOR[2],
		            ent->client->damage_alpha, ent->client->ps.blend);
	}

	if (ent->client->bonus_alpha > 0)
	{
		SV_AddBlend(0.85, 0.7, 0.3, ent->client->bonus_alpha, ent->client->ps.blend);
	}

	/* Drop damage value */
	if (ent->health < CRITICAL_HEALTH_FLASH_AT)
		ent->client->damage_alpha -= CRITICAL_HEALTH_DAMAGE_FLASH_FADE_OUT;
	else
		ent->client->damage_alpha -= DAMAGE_FLASH_FADE_OUT;

	if (ent->client->damage_alpha < 0)
		ent->client->damage_alpha = 0;

	/* Drop bonus value */
	ent->client->bonus_alpha -= 0.1;

	if (ent->client->bonus_alpha < 0)
		ent->client->bonus_alpha = 0;
}

void
P_FallingDamage(edict_t *ent)
{
	float delta;
	int damage;
	vec3_t dir;

	if (!ent)
	{
		return;
	}

	if (ent->s.modelindex != 255)
	{
		return; /* not in the player model */
	}

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		return;
	}

	if ((ent->client->oldvelocity[2] < 0) &&
		(ent->velocity[2] > ent->client->oldvelocity[2]) && (!ent->groundentity))
	{
		delta = ent->client->oldvelocity[2];
	}
	else
	{
		if (!ent->groundentity)
		{
			return;
		}

		delta = ent->velocity[2] - ent->client->oldvelocity[2];
	}

	delta = delta * delta * 0.0001;

	/* never take falling damage if completely underwater */
	if (ent->waterlevel == 3)
	{
		return;
	}

	if (ent->waterlevel == 2)
	{
		delta *= 0.25;
	}

	if (ent->waterlevel == 1)
	{
		delta *= 0.5;
	}

	if (delta < 1)
	{
		return;
	}

	if (delta < 15)
	{
		ent->s.event = EV_FOOTSTEP;
		return;
	}

	ent->client->fall_value = delta * 0.5;

	if (ent->client->fall_value > 40)
	{
		ent->client->fall_value = 40;
	}

	ent->client->fall_time = level.time + FALL_TIME;

	if (delta > 30)
	{
		if (ent->health > 0)
		{
			if (delta >= 55)
			{
				ent->s.event = EV_FALLFAR;
			}
			else
			{
				ent->s.event = EV_FALL;
			}
		}

		/* ent->pain_debounce_time = level.time; */ /* baAlex, TODO: check how this interact with above code */
		damage = (delta - 30) / 2;

		if (damage < 1)
		{
			damage = 1;
		}

		VectorSet(dir, 0, 0, 1);

		if (!deathmatch->value || !((int)dmflags->value & DF_NO_FALLING))
		{
			T_Damage(ent, world, world, dir, ent->s.origin,
					vec3_origin, damage, 0, 0, MOD_FALLING);
		}
	}
	else
	{
		ent->s.event = EV_FALLSHORT;
		return;
	}
}

void
P_WorldEffects(edict_t *ent)
{
	qboolean breather;
	qboolean envirosuit;
	int waterlevel, old_waterlevel;

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->air_finished = level.time + 12; /* don't need air */
		return;
	}

	waterlevel = ent->waterlevel;
	old_waterlevel = ent->client->old_waterlevel;
	ent->client->old_waterlevel = waterlevel;

	breather = ent->client->breather_framenum > level.framenum;
	envirosuit = ent->client->enviro_framenum > level.framenum;

	/* if just entered a water volume, play a sound */
	if (!old_waterlevel && waterlevel)
	{
		PlayerNoise(ent, ent->s.origin, PNOISE_SELF);

		if (ent->watertype & CONTENTS_LAVA)
		{
			gi.sound(ent, CHAN_BODY,
					gi.soundindex("player/lava_in.wav"), 1, ATTN_NORM, 0);
		}
		else if (ent->watertype & CONTENTS_SLIME)
		{
			gi.sound(ent, CHAN_BODY,
					gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		}
		else if (ent->watertype & CONTENTS_WATER)
		{
			gi.sound(ent, CHAN_BODY,
					gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		}

		ent->flags |= FL_INWATER;

		/* clear damage_debounce, so the pain sound will play immediately */
		ent->damage_debounce_time = level.time - 1;
	}

	/* if just completely exited a water volume, play a sound */
	if (old_waterlevel && !waterlevel)
	{
		PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
		gi.sound(ent, CHAN_BODY, gi.soundindex(
						"player/watr_out.wav"), 1, ATTN_NORM, 0);
		ent->flags &= ~FL_INWATER;
	}

	/* check for head just going under moove^^water */
	if ((old_waterlevel != 3) && (waterlevel == 3))
	{
		gi.sound(ent, CHAN_BODY, gi.soundindex(
						"player/watr_un.wav"), 1, ATTN_NORM, 0);
	}

	/* check for head just coming out of water */
	if ((old_waterlevel == 3) && (waterlevel != 3))
	{
		if (ent->air_finished < level.time)
		{
			/* gasp for air */
			gi.sound(ent, CHAN_VOICE,
					gi.soundindex("player/gasp1.wav"), 1, ATTN_NORM, 0);
			PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
		}
		else if (ent->air_finished < level.time + 11)
		{
			/* just break surface */
			gi.sound(ent, CHAN_VOICE,
					gi.soundindex("player/gasp2.wav"), 1, ATTN_NORM, 0);
		}
	}

	/* check for drowning */
	if (waterlevel == 3)
	{
		/* breather or envirosuit give air */
		if (breather || envirosuit)
		{
			ent->air_finished = level.time + 10;

			if (((int)(ent->client->breather_framenum -
					   level.framenum) % 25) == 0)
			{
				if (!ent->client->breather_sound)
				{
					gi.sound(ent, CHAN_AUTO,
							gi.soundindex("player/u_breath1.wav"), 1, ATTN_NORM, 0);
				}
				else
				{
					gi.sound(ent, CHAN_AUTO,
							gi.soundindex("player/u_breath2.wav"), 1, ATTN_NORM, 0);
				}

				ent->client->breather_sound ^= 1;
				PlayerNoise(ent, ent->s.origin,
						PNOISE_SELF);
			}
		}

		/* if out of air, start drowning */
		if (ent->air_finished < level.time)
		{
			/* drown! */
			if ((ent->client->next_drown_time < level.time) &&
				(ent->health > 0))
			{
				ent->client->next_drown_time = level.time + 1;

				/* take more damage the longer underwater */
				ent->dmg += 2;

				if (ent->dmg > 15)
				{
					ent->dmg = 15;
				}

				/* play a gurp sound instead of a normal pain sound */
				if (ent->health <= ent->dmg)
				{
					gi.sound(ent, CHAN_VOICE,
							gi.soundindex("player/drown1.wav"), 1, ATTN_NORM, 0);
				}
				else if (randk() & 1)
				{
					gi.sound(ent, CHAN_VOICE,
							gi.soundindex("*gurp1.wav"), 1, ATTN_NORM, 0);
				}
				else
				{
					gi.sound(ent, CHAN_VOICE,
							gi.soundindex("*gurp2.wav"), 1, ATTN_NORM, 0);
				}

				/*ent->pain_debounce_time = level.time;*/ /* baAlex, TODO: check how this interact with above code */

				T_Damage(ent, world, world, vec3_origin,
						ent->s.origin, vec3_origin,
						ent->dmg, 0, DAMAGE_NO_ARMOR,
						MOD_WATER);
			}
		}
	}
	else
	{
		ent->air_finished = level.time + 12;
		ent->dmg = 2;
	}

	/* check for sizzle damage */
	if (waterlevel && (ent->watertype & (CONTENTS_LAVA | CONTENTS_SLIME)))
	{
		if (ent->watertype & CONTENTS_LAVA)
		{
			if ((ent->client->invincible_framenum < level.framenum) &&
				!(ent->flags & FL_GODMODE))
			{
				sPlayBurnSound(ent);
			}

			if (envirosuit) /* take 1/3 damage with envirosuit */
			{
				T_Damage(ent, world, world, vec3_origin,
						ent->s.origin, vec3_origin,
						1 * waterlevel, 0, 0, MOD_LAVA);
			}
			else
			{
				T_Damage(ent, world, world, vec3_origin,
						ent->s.origin, vec3_origin,
						3 * waterlevel, 0, 0, MOD_LAVA);
			}
		}

		if (ent->watertype & CONTENTS_SLIME)
		{
			if (!envirosuit)
			{
				/* no damage from slime with envirosuit */
				T_Damage(ent, world, world, vec3_origin,
						ent->s.origin, vec3_origin,
						1 * waterlevel, 0, 0, MOD_SLIME);
			}
		}
	}
}

void
G_SetClientEffects(edict_t *ent)
{
	int pa_type;
	int remaining;

	if (!ent)
	{
		return;
	}

	ent->s.effects = 0;
	ent->s.renderfx = RF_IR_VISIBLE;

	if ((ent->health <= 0) || level.intermissiontime)
	{
		return;
	}

	if (ent->powerarmor_time > level.time)
	{
		pa_type = PowerArmorType(ent);

		if (pa_type == POWER_ARMOR_SCREEN)
		{
			ent->s.effects |= EF_POWERSCREEN;
		}
		else if (pa_type == POWER_ARMOR_SHIELD)
		{
			ent->s.effects |= EF_COLOR_SHELL;
			ent->s.renderfx |= RF_SHELL_GREEN;
		}
	}

	if (ent->client->quad_framenum > level.framenum)
	{
		remaining = ent->client->quad_framenum - level.framenum;

		if ((remaining > 30) || (remaining & 4))
		{
			ent->s.effects |= EF_QUAD;
		}
	}

	if (ent->client->invincible_framenum > level.framenum)
	{
		remaining = ent->client->invincible_framenum - level.framenum;

		if ((remaining > 30) || (remaining & 4))
		{
			ent->s.effects |= EF_PENT;
		}
	}

	/* show cheaters */
	if (ent->flags & FL_GODMODE)
	{
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
	}
}

void
G_SetClientSound(edict_t *ent)
{
	char *weap;

	if (!ent)
	{
		return;
	}

	if (ent->client->pers.game_helpchanged != game.helpchanged)
	{
		ent->client->pers.game_helpchanged = game.helpchanged;
		ent->client->pers.helpchanged = 1;
	}

	/* help beep (no more than three times) */
	if (ent->client->pers.helpchanged &&
		(ent->client->pers.helpchanged <= 3) && !(level.framenum & 63))
	{
		ent->client->pers.helpchanged++;
		sPlayComputerSound(ent);
	}

	if (ent->client->pers.weapon)
	{
		weap = ent->client->pers.weapon->classname;
	}
	else
	{
		weap = "";
	}

	if (ent->waterlevel && (ent->watertype & (CONTENTS_LAVA | CONTENTS_SLIME)))
	{
		ent->s.sound = snd_fry;
	}
	else if (strcmp(weap, "weapon_railgun") == 0)
	{
		ent->s.sound = gi.soundindex("weapons/rg_hum.wav");
	}
	else if (strcmp(weap, "weapon_bfg") == 0)
	{
		ent->s.sound = gi.soundindex("weapons/bfg_hum.wav");
	}
	else if (ent->client->weapon_sound)
	{
		ent->s.sound = ent->client->weapon_sound;
	}
	else
	{
		ent->s.sound = 0;
	}
}

void
G_SetClientFrame(edict_t *ent, float xyspeed)
{
	gclient_t *client;
	qboolean duck, run;

	if (!ent)
	{
		return;
	}

	if (ent->s.modelindex != 255)
	{
		return; /* not in the player model */
	}

	client = ent->client;

	if (client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		duck = true;
	}
	else
	{
		duck = false;
	}

	if (xyspeed)
	{
		run = true;
	}
	else
	{
		run = false;
	}

	/* check for stand/duck and stop/go transitions */
	if ((duck != client->anim_duck) && (client->anim_priority < ANIM_DEATH))
	{
		goto newanim;
	}

	if ((run != client->anim_run) && (client->anim_priority == ANIM_BASIC))
	{
		goto newanim;
	}

	if (!ent->groundentity && (client->anim_priority <= ANIM_WAVE))
	{
		goto newanim;
	}

	if (client->anim_priority == ANIM_REVERSE)
	{
		if (ent->s.frame > client->anim_end)
		{
			ent->s.frame--;
			return;
		}
	}
	else if (ent->s.frame < client->anim_end)
	{
		/* continue an animation */
		ent->s.frame++;
		return;
	}

	if (client->anim_priority == ANIM_DEATH)
	{
		return; /* stay there */
	}

	if (client->anim_priority == ANIM_JUMP)
	{
		if (!ent->groundentity)
		{
			return; /* stay there */
		}

		ent->client->anim_priority = ANIM_WAVE;
		ent->s.frame = FRAME_jump3;
		ent->client->anim_end = FRAME_jump6;
		return;
	}

newanim:

	/* return to either a running or standing frame */
	client->anim_priority = ANIM_BASIC;
	client->anim_duck = duck;
	client->anim_run = run;

	if (!ent->groundentity)
	{
		client->anim_priority = ANIM_JUMP;

		if (ent->s.frame != FRAME_jump2)
		{
			ent->s.frame = FRAME_jump1;
		}

		client->anim_end = FRAME_jump2;
	}
	else if (run)
	{
		/* running */
		if (duck)
		{
			ent->s.frame = FRAME_crwalk1;
			client->anim_end = FRAME_crwalk6;
		}
		else
		{
			ent->s.frame = FRAME_run1;
			client->anim_end = FRAME_run6;
		}
	}
	else
	{
		/* standing */
		if (duck)
		{
			ent->s.frame = FRAME_crstnd01;
			client->anim_end = FRAME_crstnd19;
		}
		else
		{
			ent->s.frame = FRAME_stand01;
			client->anim_end = FRAME_stand40;
		}
	}
}

/*
 * Called for each player at the end of
 * the server frame and right after spawning
 */
void
ClientEndServerFrame(edict_t *ent)
{
	if (!ent)
		return;

	/* If the origin or velocity have changed since ClientThink(),
	   update the pmove values. This will happen when the client
	   is pushed by a bmodel or kicked by an explosion.
	   If it wasn't updated here, the view position would lag a frame
	   behind the body position when pushed -- "sinking into plats" */
	for (int i = 0; i < 3; i++)
	{
		ent->client->ps.pmove.origin[i] = ent->s.origin[i] * 8.0;
		ent->client->ps.pmove.velocity[i] = ent->velocity[i] * 8.0;
	}

	/* If the end of unit layout is displayed, don't give
	   the player any normal movement attributes */
	if (level.intermissiontime)
	{
		ent->client->ps.blend[3] = 0;
		ent->client->ps.fov = 90;
		G_SetStats(ent);
		return;
	}

	/* Keep view angles */
	vec3_t forward, right, up;
	AngleVectors(ent->client->v_angle, forward, right, up);

	/* Burn from lava, etc */
	P_WorldEffects(ent);

	/* Set model angles from view angles so other things in
	   the world can tell which direction you are looking */
	if (ent->client->v_angle[PITCH] > 180)
		ent->s.angles[PITCH] = (-360 + ent->client->v_angle[PITCH]) / 3;
	else
		ent->s.angles[PITCH] = ent->client->v_angle[PITCH] / 3;

	ent->s.angles[YAW] = ent->client->v_angle[YAW];
	ent->s.angles[ROLL] = 0;

	/* Detect hitting the floor */
	P_FallingDamage(ent);

	/* Translate damage into sounds and visuals */
	sDamageFeedback(ent);

	/* If dead, fix view angle */
	if (ent->deadflag)
	{
		ent->client->ps.viewangles[ROLL] = 40.f;
		ent->client->ps.viewangles[PITCH] = -15.f;
		ent->client->ps.viewangles[YAW] = ent->client->killer_yaw;
	}

	/* Add view height */
	VectorClear(ent->client->ps.viewoffset);
	ent->client->ps.viewoffset[2] += ent->viewheight;

	/* Walkcycle */
	float xyspeed;
	if (WALKCYCLE == true)
	{
		xyspeed = sqrtf(powf(ent->velocity[0], 2.0f) + powf(ent->velocity[1], 2.0f));

		if (xyspeed > 0.0f && ent->groundentity)
		{
			bool footstep = (g_footsteps->value > 0) ? true : false;

			if (xyspeed > WALKCYCLE_RUN_SPEED)
				ent->client->bobtime += WALKCYCLE_FREQUENCY[0];
			else
			{
				if (xyspeed > WALKCYCLE_WALK_SPEED)
					ent->client->bobtime += WALKCYCLE_FREQUENCY[1];
				else
					ent->client->bobtime += WALKCYCLE_FREQUENCY[2];

				/* We need to honor 'g_footsteps' cvar, that is
				   why such weird if/else arrange */
				if (g_footsteps->value == 1)
					footstep = false;
			}

			/* Reset walkcycle phase and trigger a footstep sound when happens */
			if (ent->client->bobtime > 0.0f && ent->client->bobtime > M_PI * 1.0f)
			{
				ent->client->bobtime -= M_PI * 4.0f;
				if (WALKCYCLE_FOOTSTEP_SOUND == true && footstep == true)
					ent->s.event = EV_FOOTSTEP;
			}
			else if(ent->client->bobtime < 0.0f && ent->client->bobtime > -M_PI * 2.0f)
			{
				ent->client->bobtime += M_PI * 2.0f;
				if (WALKCYCLE_FOOTSTEP_SOUND == true && footstep == true)
					ent->s.event = EV_FOOTSTEP;
			}
		}
		else
		{
			ent->client->bobtime = 0.0f;
		}
	}
	else
	{
		xyspeed = 0.0f;
	}

	/* Gun bob */
	if (GUN_BOB == true && WALKCYCLE == true)
	{
		const float waveform[3] = {
			sinf(ent->client->bobtime * 2.0f),
			sinf(ent->client->bobtime * 1.0f),
			fabsf(sinf(ent->client->bobtime * 2.0f))
		};

		if (xyspeed > WALKCYCLE_RUN_SPEED)
		{
			ent->client->ps.gunangles[PITCH] = waveform[GUN_BOB_PITCH_WAVE[0]] * GUN_BOB_PITCH[0];
			ent->client->ps.gunangles[YAW] = waveform[GUN_BOB_YAW_WAVE[0]] * GUN_BOB_YAW[0];
			ent->client->ps.gunangles[ROLL] = waveform[GUN_BOB_ROLL_WAVE[0]] * GUN_BOB_ROLL[0];
		}
		if (xyspeed > WALKCYCLE_WALK_SPEED)
		{
			ent->client->ps.gunangles[PITCH] = waveform[GUN_BOB_PITCH_WAVE[1]] * GUN_BOB_PITCH[1];
			ent->client->ps.gunangles[YAW] = waveform[GUN_BOB_YAW_WAVE[1]] * GUN_BOB_YAW[1];
			ent->client->ps.gunangles[ROLL] = waveform[GUN_BOB_ROLL_WAVE[1]] * GUN_BOB_ROLL[1];
		}
		else
		{
			ent->client->ps.gunangles[PITCH] = waveform[GUN_BOB_PITCH_WAVE[2]] * GUN_BOB_PITCH[2];
			ent->client->ps.gunangles[YAW] = waveform[GUN_BOB_YAW_WAVE[2]] * GUN_BOB_YAW[2];
			ent->client->ps.gunangles[ROLL] = waveform[GUN_BOB_ROLL_WAVE[2]] * GUN_BOB_ROLL[2];
		}
	}
	else
	{
		VectorClear(ent->client->ps.gunangles);
	}

	/* Gun offset inertia */
	{
		vec3_t inertia;
		VectorClear(ent->client->ps.gunoffset);

		if (GUN_OFFSET_INERTIA == true)
		{
			inertia[0] = DotProduct(forward, ent->velocity) * (-GUN_OFFSET_INERTIA_SCALE[0]);
			inertia[1] = DotProduct(right, ent->velocity)   * (-GUN_OFFSET_INERTIA_SCALE[1]);
			inertia[2] = DotProduct(up, ent->velocity)      * (-GUN_OFFSET_INERTIA_SCALE[2]);
		}
		else
			VectorClear(inertia);

		for (int i = 0; i < 3; i++)
		{
			/* gun_x / gun_y / gun_z are development tools */
			ent->client->ps.gunoffset[i] += forward[i] * (gun_y->value + inertia[0]);
			ent->client->ps.gunoffset[i] += right[i] * (gun_x->value + inertia[1]);
			ent->client->ps.gunoffset[i] += up[i] * (-gun_z->value + inertia[2]);
		}
	}

	/* Determine the full screen color blend
	   must be after viewoffset, so eye contents
	   can be accurately determined */
	SV_CalcBlend(ent);

	/* Chase cam stuff */
	if (ent->client->resp.spectator)
		G_SetSpectatorStats(ent);
	else
		G_SetStats(ent);

	/* More stuff */
	G_CheckChaseStats(ent);
	G_SetClientEffects(ent);
	G_SetClientSound(ent);
	G_SetClientFrame(ent, xyspeed);

	if (!(level.framenum & 31))
	{
		/* if the scoreboard is up, update it */
		if (ent->client->showscores)
		{
			DeathmatchScoreboardMessage(ent, ent->enemy);
			gi.unicast(ent, false);
		}

		/* if the help computer is up, update it */
		if (ent->client->showhelp)
		{
			ent->client->pers.helpchanged = 0;
			HelpComputerMessage(ent);
			gi.unicast(ent, false);
		}
	}

	/* If the inventory is up, update it */
	if (ent->client->showinventory)
	{
		InventoryMessage(ent);
		gi.unicast(ent, false);
	}

	/* Keep some things for next call */
	VectorCopy(ent->velocity, ent->client->oldvelocity);
	VectorCopy(ent->client->ps.viewangles, ent->client->oldviewangles);
}

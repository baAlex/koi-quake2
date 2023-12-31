/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2023 Alexander Brandt
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
 */

#ifndef KOI_H
#define KOI_H

#include <stdbool.h>


// Health regeneration (client.c)
// ==============================
static const bool HEALTH_REGENERATION = true;
static const float HEALTH_REGENERATION_DELAY = 8.0f;  // Four seconds before regeneration
static const float HEALTH_REGENERATION_SPEED = 0.25f; // Seconds between '+1'


// Health Hud (hud.c)
// ==================
static const bool HEALTH_HUD_EASING = true;
static const float HEALTH_HUD_EASING_FACTOR = 2.0f; // A simple 'pow(normalized health, factor)'


// Damage flash (view.c)
// =====================
static const float DAMAGE_FLASH_MIN = 0.2f; // Total, a limit to actually show something
static const float DAMAGE_FLASH_MAX = 0.9f; // Total, a limit to not opaque the screen

static const float DAMAGE_FLASH_COLOR[3] = {1.0f, 0.0f, 0.0f};
static const float DAMAGE_FLASH_FADE_OUT = 0.06f; // Quake 2 default value


// Critical health flash (view.c)
// ==============================
static const int CRITICAL_HEALTH_FLASH_AT = 50; // Zero to disable

static const float CRITICAL_HEALTH_FLASH_DELAY = 0.5f; // One second
static const float CRITICAL_HEALTH_FLASH = 0.5f;       // Not proportional to health, a constant value
                                                       // (too much visual noise otherwise)

static const float CRITICAL_HEALTH_DAMAGE_FLASH_FADE_OUT = DAMAGE_FLASH_FADE_OUT / 2.0f;


// Walkcycle (view.c)
// ==================
static const bool WALKCYCLE = true;

static const float WALKCYCLE_RUN_SPEED = 297.0f;  // Yamagi use 225, Carmack 210
static const float WALKCYCLE_WALK_SPEED = 100.0f; // This one is Carmack

static const float WALKCYCLE_FREQUENCY[3] = {1.0f, 0.75f, 0.60f}; // Running, walking, otherwise
static const bool WALKCYCLE_FOOTSTEP_SOUND = true;                // Depends on 'g_footsteps' cvar


// Gun bob (view.c)
// ================
static const bool GUN_BOB = true; // Requires WALKCYCLE

static const float GUN_BOB_SCALE = 0.8f; // Too much movements layer

static const float GUN_BOB_PITCH[3] = {2.0f  * GUN_BOB_SCALE, 1.5f  * GUN_BOB_SCALE, 1.0f  * GUN_BOB_SCALE}; // Running, walking, otherwise
static const float GUN_BOB_YAW[3] =   {1.5f  * GUN_BOB_SCALE, 1.5f  * GUN_BOB_SCALE, 1.5f  * GUN_BOB_SCALE};
static const float GUN_BOB_ROLL[3] =  {0.75f * GUN_BOB_SCALE, 0.75f * GUN_BOB_SCALE, 0.75f * GUN_BOB_SCALE};

static const int GUN_BOB_PITCH_WAVE[3] = {0, 0, 0}; // Waveforms, for running, walking, otherwhise
static const int GUN_BOB_YAW_WAVE[3] = {1, 1, 1};
static const int GUN_BOB_ROLL_WAVE[3] = {1, 1, 1};

// Waveforms are:
// - 0 = sin(phase * 2)
// - 1 = sin(phase)
// - 2 = abs(sin(phase * 2)) !!! Enters in phase with the others...
// ... and a '(fabsf(sinf(ent->client->bobtime * 2.0f)) - 0.5f) * 2.0f' don't help either


// Gun offset recoil (view.c)
// ==========================
static const bool GUN_OFFSET_RECOIL = true; // WALKCYCLE not required, but helps

static const float GUN_OFFSET_RECOIL_SCALE[3] = {0.4F, 0.3F, 0.2F}; // Running, walking, otherwise


// Gun angle inertia (view.c)
// ==========================
static const bool GUN_ANGLE_INERTIA = true;

static const float GUN_ANGLE_INERTIA_Q = 0.9f / (10.0f); // Game runs at 10hz
static const float GUN_ANGLE_INERTIA_CLAMP = 20.0f;      // To large and the weapon will
                                                         // feel floaty and slow (if angles
                                                         // doesn't overflow first when the
                                                         // engine converts things to 'short')

// The game running at fixed 10hz is... well, funny, a good thing
// tho is that we don't need to measure mouse movement to activate
// deactivate inertia, basically from our point of view the mouse
// itself is lowpass-filtered


// Gun offset inertia (view.c)
// ===========================
static const bool GUN_OFFSET_INERTIA = true;

static const float GUN_OFFSET_INERTIA_SCALE[] = {(1.0f / WALKCYCLE_RUN_SPEED) * 0.5f,    // Forward
                                                 (1.0f / WALKCYCLE_RUN_SPEED) * 1.0f,    // Side
                                                 (1.0f / WALKCYCLE_RUN_SPEED) * -0.75f}; // Up/down

#endif

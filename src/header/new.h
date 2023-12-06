

#include <stdbool.h>

#ifndef NEW_H
#define NEW_H

// Health regeneration (client.c)
static const bool HEALTH_REGENERATION = true;
static const float HEALTH_REGENERATION_DELAY = 4.0f;  // Four seconds before regeneration
static const float HEALTH_REGENERATION_SPEED = 0.25f; // Seconds between '+1'

// Health Hud (hud.c)
static const bool HEALTH_HUD_EASING = true;
static const float HEALTH_HUD_EASING_FACTOR = 2.0f; // A simple 'pow(normalized health, factor)'

#endif

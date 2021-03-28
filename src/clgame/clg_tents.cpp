// LICENSE HERE.

//
// clg_tents.c
//
//
// Handles the temporary entities. Explions, Beams, etc.
//
#include "clg_local.h"

//
// CVars.
//
static color_t  railcore_color;
static color_t  railspiral_color;

static cvar_t* cl_railtrail_type;
static cvar_t* cl_railtrail_time;
static cvar_t* cl_railcore_color;
static cvar_t* cl_railcore_width;
static cvar_t* cl_railspiral_color;
static cvar_t* cl_railspiral_radius;

//
// Handles to Sound Effects.
//
qhandle_t   cl_sfx_ric1;
qhandle_t   cl_sfx_ric2;
qhandle_t   cl_sfx_ric3;
qhandle_t   cl_sfx_lashit;
qhandle_t   cl_sfx_flare;
qhandle_t   cl_sfx_spark5;
qhandle_t   cl_sfx_spark6;
qhandle_t   cl_sfx_spark7;
qhandle_t   cl_sfx_railg;
qhandle_t   cl_sfx_rockexp;
qhandle_t   cl_sfx_grenexp;
qhandle_t   cl_sfx_watrexp;
qhandle_t   cl_sfx_footsteps[4];

qhandle_t   cl_sfx_lightning;
qhandle_t   cl_sfx_disrexp;

//
// Handles to Models.
//
qhandle_t   cl_mod_explode;
qhandle_t   cl_mod_smoke;
qhandle_t   cl_mod_flash;
qhandle_t   cl_mod_parasite_segment;
qhandle_t   cl_mod_grapple_cable;
qhandle_t   cl_mod_explo4;
qhandle_t   cl_mod_bfg_explo;
qhandle_t   cl_mod_powerscreen;
qhandle_t   cl_mod_laser;
qhandle_t   cl_mod_dmspot;
qhandle_t   cl_mod_explosions[4];

qhandle_t   cl_mod_lightning;
qhandle_t   cl_mod_heatbeam;
qhandle_t   cl_mod_explo4_big;

extern cvar_t* cvar_pt_particle_emissive;

static void cl_railcore_color_changed(cvar_t* self)
{
	//if (!SCR_ParseColor(self->string, &railcore_color)) {
	//	Com_WPrintf("Invalid value '%s' for '%s'\n", self->string, self->name);
	//	Cvar_Reset(self);
	//	railcore_color.u32 = U32_RED;
	//}
}

static void cl_railspiral_color_changed(cvar_t* self)
{
	//if (!SCR_ParseColor(self->string, &railspiral_color)) {
	//	Com_WPrintf("Invalid value '%s' for '%s'\n", self->string, self->name);
	//	Cvar_Reset(self);
	//	railspiral_color.u32 = U32_BLUE;
	//}
}

//
//=============================================================================
//
//	LASER MANAGEMENT
//
//=============================================================================
//

#define MAX_LASERS  32

typedef struct {
	vec3_t      start;
	vec3_t      end;
	int         color;
	color_t     rgba;
	int         width;
	int         lifetime, starttime;
} laser_t;

static laser_t  clg_lasers[MAX_LASERS];

static void CLG_ClearLasers(void)
{
	memset(clg_lasers, 0, sizeof(clg_lasers));
}

static laser_t* CLG_AllocLaser(void)
{
	laser_t* l;
	int i;

	for (i = 0, l = clg_lasers; i < MAX_LASERS; i++, l++) {
		if (cl->time - l->starttime >= l->lifetime) {
			memset(l, 0, sizeof(*l));
			l->starttime = cl->time;
			return l;
		}
	}

	return NULL;
}

static void CLG_AddLasers(void)
{
	laser_t* l;
	entity_t    ent;
	int         i;
	int         time;

	memset(&ent, 0, sizeof(ent));

	for (i = 0, l = clg_lasers; i < MAX_LASERS; i++, l++) {
		time = l->lifetime - (cl->time - l->starttime);
		if (time < 0) {
			continue;
		}

		if (l->color == -1) {
			float f = (float)time / (float)l->lifetime;

			ent.rgba.u8[0] = l->rgba.u8[0];
			ent.rgba.u8[1] = l->rgba.u8[1];
			ent.rgba.u8[2] = l->rgba.u8[2];
			ent.rgba.u8[3] = l->rgba.u8[3] * f;
			ent.alpha = f;
		}
		else {
			ent.alpha = 0.30f;
		}

		ent.skinnum = l->color;
		ent.flags = RF_TRANSLUCENT | RF_BEAM;
		Vec3_Copy_(l->start, ent.origin);
		Vec3_Copy_(l->end, ent.oldorigin);
		ent.frame = l->width;

		V_AddEntity(&ent);
	}
}

static void CLG_ParseLaser(int colors)
{
	laser_t* l;

	l = CLG_AllocLaser();
	if (!l)
		return;

	Vec3_Copy_(teParameters.pos1, l->start);
	Vec3_Copy_(teParameters.pos2, l->end);
	l->lifetime = 100;
	l->color = (colors >> ((rand() % 4) * 8)) & 0xff;
	l->width = 4;
}


//
//=============================================================================
//
//	EXPLOSION MANAGEMENT
//
//=============================================================================
//
explosion_t  clg_explosions[MAX_EXPLOSIONS];

static void CLG_ClearExplosions(void)
{
	memset(clg_explosions, 0, sizeof(clg_explosions));
}

static explosion_t* CLG_AllocExplosion(void)
{
	explosion_t* e, * oldest;
	int     i;
	int     time;

	for (i = 0, e = clg_explosions; i < MAX_EXPLOSIONS; i++, e++) {
		if (e->type == explosion_t::ex_free) { // CPP: Enum
			memset(e, 0, sizeof(*e));
			return e;
		}
	}
	// find the oldest explosion
	time = cl->time;
	oldest = clg_explosions;

	for (i = 0, e = clg_explosions; i < MAX_EXPLOSIONS; i++, e++) {
		if (e->start < time) {
			time = e->start;
			oldest = e;
		}
	}
	memset(oldest, 0, sizeof(*oldest));
	return oldest;
}

static explosion_t* CLG_PlainExplosion(qboolean big)
{
	explosion_t* ex;

	ex = CLG_AllocExplosion();
	Vec3_Copy_(teParameters.pos1, ex->ent.origin);
	ex->type = explosion_t::ex_poly; // CPP: Enum
	ex->ent.flags = RF_FULLBRIGHT;
	ex->start = cl->servertime - CL_FRAMETIME;
	ex->light = 350;
	Vec3_Set_(ex->lightcolor, 1.0, 0.5, 0.5);
	ex->ent.angles[1] = rand() % 360;

	int model_idx = rand() % (sizeof(cl_mod_explosions) / sizeof(*cl_mod_explosions));
	model_t* sprite_model = clgi.MOD_ForHandle(cl_mod_explosions[model_idx]);

	if (cl_explosion_sprites->integer && !big && sprite_model)
	{
		ex->ent.model = cl_mod_explosions[model_idx];
		ex->frames = sprite_model->numframes;
		ex->frametime = cl_explosion_frametime->integer;
	}
	else
	{
		ex->ent.model = big ? cl_mod_explo4_big : cl_mod_explo4;
		if (frand() < 0.5)
			ex->baseframe = 15;
		ex->frames = 15;
	}

	return ex;
}

/*
=================
CL_SmokeAndFlash
=================
*/
void CLG_SmokeAndFlash(vec3_t origin)
{
	explosion_t* ex;

	ex = CLG_AllocExplosion();
	Vec3_Copy_(origin, ex->ent.origin);
	ex->type = explosion_t::ex_misc; // CPP: Enum
	ex->frames = 4;
	ex->ent.flags = RF_TRANSLUCENT | RF_NOSHADOW;
	ex->start = cl->servertime - CL_FRAMETIME;
	ex->ent.model = cl_mod_smoke;

	ex = CLG_AllocExplosion();
	Vec3_Copy_(origin, ex->ent.origin);
	ex->type = explosion_t::ex_flash; // CPP: Enum
	ex->ent.flags = RF_FULLBRIGHT;
	ex->frames = 2;
	ex->start = cl->servertime - CL_FRAMETIME;
	ex->ent.model = cl_mod_flash;
}

#define LENGTH(a) ((sizeof (a)) / (sizeof(*(a))))

typedef struct light_curve_s {
	vec3_t color;
	float radius;
	float offset;
} light_curve_t;

static light_curve_t ex_poly_light[] = {
	{ { 0.4f,       0.2f,       0.02f     }, 12.5f, 20.00f },
	{ { 0.351563f,  0.175781f,  0.017578f }, 15.0f, 23.27f },
	{ { 0.30625f,   0.153125f,  0.015312f }, 20.0f, 24.95f },
	{ { 0.264062f,  0.132031f,  0.013203f }, 22.5f, 25.01f },
	{ { 0.225f,     0.1125f,    0.01125f  }, 25.0f, 27.53f },
	{ { 0.189063f,  0.094531f,  0.009453f }, 27.5f, 28.55f },
	{ { 0.15625f,   0.078125f,  0.007813f }, 30.0f, 30.80f },
	{ { 0.126563f,  0.063281f,  0.006328f }, 27.5f, 40.43f },
	{ { 0.1f,       0.05f,      0.005f    }, 25.0f, 49.02f },
	{ { 0.076563f,  0.038281f,  0.003828f }, 22.5f, 58.15f },
	{ { 0.05625f,   0.028125f,  0.002812f }, 20.0f, 61.03f },
	{ { 0.039063f,  0.019531f,  0.001953f }, 17.5f, 63.59f },
	{ { 0.025f,     0.0125f,    0.00125f  }, 15.0f, 66.47f },
	{ { 0.014063f,  0.007031f,  0.000703f }, 12.5f, 71.34f },
	{ { 0.f,        0.f,        0.f       }, 10.0f, 72.00f }
};

static light_curve_t ex_blaster_light[] = {
	{ { 0.04f,      0.02f,      0.0f      },  5.f, 15.00f },
	{ { 0.2f,       0.15f,      0.01f     }, 15.f, 15.00f },
	{ { 0.04f,      0.02f,      0.0f      },  5.f, 15.00f },
};

static void CLG_AddExplosionLight(explosion_t* ex, float phase)
{
	int curve_size;
	light_curve_t* curve;

	switch (ex->type)
	{
	case explosion_t::ex_poly: // CPP: Enum
		curve = ex_poly_light;
		curve_size = LENGTH(ex_poly_light);
		break;
	case explosion_t::ex_blaster: // CPP: Enum
		curve = ex_blaster_light;
		curve_size = LENGTH(ex_blaster_light);
		break;
	default:
		return;
	}

	float timeAlpha = ((float)(curve_size - 1)) * phase;
	int baseSample = (int)floorf(timeAlpha);
	baseSample = max(0, min(curve_size - 2, baseSample));

	float w1 = timeAlpha - (float)(baseSample);
	float w0 = 1.f - w1;

	light_curve_t* s0 = curve + baseSample;
	light_curve_t* s1 = curve + baseSample + 1;

	float offset = w0 * s0->offset + w1 * s1->offset;
	float radius = w0 * s0->radius + w1 * s1->radius;

	vec3_t origin;
	vec3_t up;
	AngleVectors(ex->ent.angles, NULL, NULL, up);
	Vec3_MA_(ex->ent.origin, offset, up, origin);

	vec3_t color;
	Vec3_Clear(color);
	Vec3_MA_(color, w0, s0->color, color);
	Vec3_MA_(color, w1, s1->color, color);

	V_AddLightEx(origin, 500.f, color[0], color[1], color[2], radius);
}

static void CLG_AddExplosions(void)
{
	entity_t* ent;
	int         i;
	explosion_t* ex;
	float       frac;
	int         f;

	memset(&ent, 0, sizeof(ent));

	for (i = 0, ex = clg_explosions; i < MAX_EXPLOSIONS; i++, ex++) {
		if (ex->type == explosion_t::ex_free) // CPP: Cast
			continue;
		float inv_frametime = ex->frametime ? 1.f / (float)ex->frametime : BASE_1_FRAMETIME;
		frac = (cl->time - ex->start) * inv_frametime;
		f = floor(frac);

		ent = &ex->ent;

		switch (ex->type) {
		case explosion_t::ex_mflash:
			if (f >= ex->frames - 1)
				ex->type = explosion_t::ex_free;
			break;
		case explosion_t::ex_misc:
		case explosion_t::ex_blaster:
		case explosion_t::ex_flare:
		case explosion_t::ex_light:
			if (f >= ex->frames - 1) {
				ex->type = explosion_t::ex_free;
				break;
			}
			ent->alpha = 1.0 - frac / (ex->frames - 1);
			break;
		case explosion_t::ex_flash:
			if (f >= 1) {
				ex->type = explosion_t::ex_free;
				break;
			}
			ent->alpha = 1.0;
			break;
		case explosion_t::ex_poly:
			if (f >= ex->frames - 1) {
				ex->type = explosion_t::ex_free;
				break;
			}

			ent->alpha = ((float)ex->frames - (float)f) / (float)ex->frames;
			ent->alpha = max(0.f, min(1.f, ent->alpha));
			ent->alpha = ent->alpha * ent->alpha * (3.f - 2.f * ent->alpha); // smoothstep

			if (f < 10) {
				ent->skinnum = (f >> 1);
				if (ent->skinnum < 0)
					ent->skinnum = 0;
			}
			else {
				ent->flags |= RF_TRANSLUCENT;
				if (f < 13)
					ent->skinnum = 5;
				else
					ent->skinnum = 6;
			}
			break;
		case explosion_t::ex_poly2:
			if (f >= ex->frames - 1) {
				ex->type = explosion_t::ex_free;
				break;
			}

			ent->alpha = (5.0 - (float)f) / 5.0;
			ent->skinnum = 0;
			ent->flags |= RF_TRANSLUCENT;
			break;
		default:
			break;
		}

		if (ex->type == explosion_t::ex_free) // CPP: Cast
			continue;

		if (vid_rtx->integer)
			CLG_AddExplosionLight(ex, frac / (ex->frames - 1));
		else
		{
			if (ex->light)
				V_AddLight(ent->origin, ex->light * ent->alpha,
					ex->lightcolor[0], ex->lightcolor[1], ex->lightcolor[2]);
		}

		if (ex->type != explosion_t::ex_light) {
			Vec3_Copy_(ent->origin, ent->oldorigin);

			if (f < 0)
				f = 0;
			ent->frame = ex->baseframe + f + 1;
			ent->oldframe = ex->baseframe + f;
			ent->backlerp = 1.0 - (frac - f);

			V_AddEntity(ent);
		}
	}
}

//
//=============================================================================
//
//	BEAM MANAGEMENT
//
//=============================================================================
//

#define MAX_BEAMS   32

typedef struct {
	int         entity;
	int         dest_entity;
	qhandle_t   model;
	int         endtime;
	vec3_t      offset;
	vec3_t      start, end;
} beam_t;

static beam_t   clg_beams[MAX_BEAMS];
static beam_t   clg_playerbeams[MAX_BEAMS];

static void CLG_ClearBeams(void)
{
	memset(clg_beams, 0, sizeof(clg_beams));
	memset(clg_playerbeams, 0, sizeof(clg_playerbeams));
}

static void CLG_ParseBeam(qhandle_t model)
{
	beam_t* b;
	int     i;

	// override any beam with the same source AND destination entities
	for (i = 0, b = clg_beams; i < MAX_BEAMS; i++, b++)
		if (b->entity == teParameters.entity1 && b->dest_entity == teParameters.entity2)
			goto override;

	// find a free beam
	for (i = 0, b = clg_beams; i < MAX_BEAMS; i++, b++) {
		if (!b->model || b->endtime < cl->time) {
			override :
				b->entity = teParameters.entity1;
			b->dest_entity = teParameters.entity2;
			b->model = model;
			b->endtime = cl->time + 200;
			Vec3_Copy_(teParameters.pos1, b->start);
			Vec3_Copy_(teParameters.pos2, b->end);
			Vec3_Copy_(teParameters.offset, b->offset);
			return;
		}
	}
}

static void CLG_ParsePlayerBeam(qhandle_t model)
{
	beam_t* b;
	int     i;

	// override any beam with the same entity
	for (i = 0, b = clg_playerbeams; i < MAX_BEAMS; i++, b++) {
		if (b->entity == teParameters.entity1) {
			b->entity = teParameters.entity1;
			b->model = model;
			b->endtime = cl->time + 200;
			Vec3_Copy_(teParameters.pos1, b->start);
			Vec3_Copy_(teParameters.pos2, b->end);
			Vec3_Copy_(teParameters.offset, b->offset);
			return;
		}
	}

	// find a free beam
	for (i = 0, b = clg_playerbeams; i < MAX_BEAMS; i++, b++) {
		if (!b->model || b->endtime < cl->time) {
			b->entity = teParameters.entity1;
			b->model = model;
			b->endtime = cl->time + 100;     // PMM - this needs to be 100 to prevent multiple heatbeams
			Vec3_Copy_(teParameters.pos1, b->start);
			Vec3_Copy_(teParameters.pos2, b->end);
			Vec3_Copy_(teParameters.offset, b->offset);
			return;
		}
	}

}

/*
=================
CL_AddBeams
=================
*/
static void CLG_AddBeams(void)
{
	int         i, j;
	beam_t* b;
	vec3_t      dist, org;
	float       d;
	entity_t    ent;
	vec3_t      angles;
	float       len, steps;
	float       model_length;

	// update beams
	for (i = 0, b = clg_beams; i < MAX_BEAMS; i++, b++) {
		if (!b->model || b->endtime < cl->time)
			continue;

		// if coming from the player, update the start position
		if (b->entity == cl->frame.clientNum + 1)
			Vec3_Add_(cl->playerEntityOrigin, b->offset, org);
		else
			Vec3_Add_(b->start, b->offset, org);

		// calculate pitch and yaw
		Vec3_Subtract_(b->end, org, dist);
		vectoangles2(dist, angles);

		// add new entities for the beams
		d = VectorNormalize(dist);
		if (b->model == cl_mod_lightning) {
			model_length = 35.0;
			d -= 20.0; // correction so it doesn't end in middle of tesla
		}
		else {
			model_length = 30.0;
		}
		steps = ceil(d / model_length);
		len = (d - model_length) / (steps - 1);

		memset(&ent, 0, sizeof(ent));
		ent.model = b->model;

		// PMM - special case for lightning model .. if the real length is shorter than the model,
		// flip it around & draw it from the end to the start.  This prevents the model from going
		// through the tesla mine (instead it goes through the target)
		if ((b->model == cl_mod_lightning) && (d <= model_length)) {
			Vec3_Copy_(b->end, ent.origin);
			ent.flags = RF_FULLBRIGHT;
			ent.angles[0] = angles[0];
			ent.angles[1] = angles[1];
			ent.angles[2] = rand() % 360;
			V_AddEntity(&ent);
			return;
		}

		while (d > 0) {
			Vec3_Copy_(org, ent.origin);
			if (b->model == cl_mod_lightning) {
				ent.flags = RF_FULLBRIGHT;
				ent.angles[0] = -angles[0];
				ent.angles[1] = angles[1] + 180.0;
				ent.angles[2] = rand() % 360;
			}
			else {
				ent.angles[0] = angles[0];
				ent.angles[1] = angles[1];
				ent.angles[2] = rand() % 360;
			}

			V_AddEntity(&ent);

			for (j = 0; j < 3; j++)
				org[j] += dist[j] * len;
			d -= model_length;
		}
	}
}

/*
=================
CL_AddPlayerBeams
Draw player locked beams. Currently only used by the plasma beam.
=================
*/
static void CLG_AddPlayerBeams(void)
{
	int         i, j;
	beam_t* b;
	vec3_t      dist, org;
	float       d;
	entity_t    ent;
	vec3_t      angles;
	float       len, steps;
	int         framenum;
	float       model_length;
	float       hand_multiplier;
	player_state_t* ps, * ops;

	if (info_hand->integer == 2)
		hand_multiplier = 0;
	else if (info_hand->integer == 1)
		hand_multiplier = -1;
	else
		hand_multiplier = 1;

	// update beams
	for (i = 0, b = clg_playerbeams; i < MAX_BEAMS; i++, b++) {
		if (!b->model || b->endtime < cl->time)
			continue;

		// if coming from the player, update the start position
		if (b->entity == cl->frame.clientNum + 1) {
			// set up gun position
			ps = CL_KEYPS;
			ops = CL_OLDKEYPS;

			for (j = 0; j < 3; j++)
				b->start[j] = cl->refdef.vieworg[j] + ops->gunoffset[j] +
				CL_KEYLERPFRAC * (ps->gunoffset[j] - ops->gunoffset[j]);

			Vec3_MA_(b->start, (hand_multiplier * b->offset[0]), cl->v_right, org);
			Vec3_MA_(org, b->offset[1], cl->v_forward, org);
			Vec3_MA_(org, b->offset[2], cl->v_up, org);
			if (info_hand->integer == 2)
				Vec3_MA_(org, -1, cl->v_up, org);

			// calculate pitch and yaw
			Vec3_Subtract_(b->end, org, dist);

			// FIXME: don't add offset twice?
			d = Vec3_Length(dist);
			Vec3_Scale_(cl->v_forward, d, dist);
			Vec3_MA_(dist, (hand_multiplier * b->offset[0]), cl->v_right, dist);
			Vec3_MA_(dist, b->offset[1], cl->v_forward, dist);
			Vec3_MA_(dist, b->offset[2], cl->v_up, dist);
			if (info_hand->integer == 2)
				Vec3_MA_(org, -1, cl->v_up, org);

			// FIXME: use cl.refdef.viewangles?
			vectoangles2(dist, angles);

			// if it's the heatbeam, draw the particle effect
			CLG_Heatbeam(org, dist);

			framenum = 1;
		}
		else {
			Vec3_Copy_(b->start, org);

			// calculate pitch and yaw
			Vec3_Subtract_(b->end, org, dist);
			vectoangles2(dist, angles);

			// if it's a non-origin offset, it's a player, so use the hardcoded player offset
			if (!Vec3_Compare(b->offset, vec3_origin)) {
				vec3_t  tmp, f, r, u;

				tmp[0] = angles[0];
				tmp[1] = angles[1] + 180.0;
				tmp[2] = 0;
				AngleVectors(tmp, f, r, u);

				Vec3_MA_(org, -b->offset[0] + 1, r, org);
				Vec3_MA_(org, -b->offset[1], f, org);
				Vec3_MA_(org, -b->offset[2] - 10, u, org);
			}
			else {
				// if it's a monster, do the particle effect
				CLG_MonsterPlasma_Shell(b->start);
			}

			framenum = 2;
		}

		// add new entities for the beams
		d = VectorNormalize(dist);
		model_length = 32.0;
		steps = ceil(d / model_length);
		len = (d - model_length) / (steps - 1);

		memset(&ent, 0, sizeof(ent));
		ent.model = b->model;
		ent.frame = framenum;
		ent.flags = RF_FULLBRIGHT;
		ent.angles[0] = -angles[0];
		ent.angles[1] = angles[1] + 180.0;
		ent.angles[2] = cl->time % 360;

		while (d > 0) {
			Vec3_Copy_(org, ent.origin);

			V_AddEntity(&ent);

			for (j = 0; j < 3; j++)
				org[j] += dist[j] * len;
			d -= model_length;
		}
	}
}

//
//=============================================================================
//
//	SUSTAIN MANAGEMENT
//
//=============================================================================
//

#define MAX_SUSTAINS    32

static cl_sustain_t     clg_sustains[MAX_SUSTAINS];

static void CLG_ClearSustains(void)
{
	memset(clg_sustains, 0, sizeof(clg_sustains));
}

static cl_sustain_t* CLG_AllocSustain(void)
{
	cl_sustain_t* s;
	int             i;

	for (i = 0, s = clg_sustains; i < MAX_SUSTAINS; i++, s++) {
		if (s->id == 0)
			return s;
	}

	return NULL;
}

static void CLG_ProcessSustain(void)
{
	cl_sustain_t* s;
	int             i;

	for (i = 0, s = clg_sustains; i < MAX_SUSTAINS; i++, s++) {
		if (s->id) {
			if ((s->endtime >= cl->time) && (cl->time >= s->nextthink))
				s->think(s);
			else if (s->endtime < cl->time)
				s->id = 0;
		}
	}
}

static void CLG_ParseSteam(void)
{
	cl_sustain_t* s;

	if (teParameters.entity1 == -1) {
		CLG_ParticleSteamEffect(teParameters.pos1, teParameters.dir, teParameters.color & 0xff, teParameters.count, teParameters.entity2);
		return;
	}

	s = CLG_AllocSustain();
	if (!s)
		return;

	s->id = teParameters.entity1;
	s->count = teParameters.count;
	Vec3_Copy_(teParameters.pos1, s->org);
	Vec3_Copy_(teParameters.dir, s->dir);
	s->color = teParameters.color & 0xff;
	s->magnitude = teParameters.entity2;
	s->endtime = cl->time + teParameters.time;
	s->think = CLG_ParticleSteamEffect2;
	s->thinkinterval = 100;
	s->nextthink = cl->time;
}

static void CLG_ParseWidow(void)
{
	cl_sustain_t* s;

	s = CLG_AllocSustain();
	if (!s)
		return;

	s->id = teParameters.entity1;
	Vec3_Copy_(teParameters.pos1, s->org);
	s->endtime = cl->time + 2100;
	s->think = CLG_Widowbeamout;
	s->thinkinterval = 1;
	s->nextthink = cl->time;
}

static void CLG_ParseNuke(void)
{
	cl_sustain_t* s;

	s = CLG_AllocSustain();
	if (!s)
		return;

	s->id = 21000;
	Vec3_Copy_(teParameters.pos1, s->org);
	s->endtime = cl->time + 1000;
	s->think = CLG_Nukeblast;
	s->thinkinterval = 1;
	s->nextthink = cl->time;
}

//
//=============================================================================
//
//	TEMP ENTITY MANAGEMENT
//
//=============================================================================
//

static void CLG_RailCore(void)
{
	laser_t* l;

	l = CLG_AllocLaser();
	if (!l)
		return;

	Vec3_Copy_(teParameters.pos1, l->start);
	Vec3_Copy_(teParameters.pos2, l->end);
	l->color = -1;
	l->lifetime = 1000 * cl_railtrail_time->value;
	l->width = cl_railcore_width->integer;
	l->rgba.u32 = railcore_color.u32;
}

static void CLG_RailSpiral(void)
{
	vec3_t      move;
	vec3_t      vec;
	float       len;
	int         j;
	cparticle_t* p;
	vec3_t      right, up;
	int         i;
	float       d, c, s;
	vec3_t      dir;

	Vec3_Copy_(teParameters.pos1, move);
	Vec3_Subtract_(teParameters.pos2, teParameters.pos1, vec);
	len = VectorNormalize(vec);

	MakeNormalVectors(vec, right, up);

	for (i = 0; i < len; i++) {
		p = CLG_AllocParticle();
		if (!p)
			return;

		p->time = cl->time;
		Vec3_Clear(p->accel);

		d = i * 0.1;
		c = cos(d);
		s = sin(d);

		Vec3_Scale_(right, c, dir);
		Vec3_MA_(dir, s, up, dir);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (cl_railtrail_time->value + frand() * 0.2);
		p->color = -1;
		p->rgba.u32 = railspiral_color.u32;
		p->brightness = cvar_pt_particle_emissive->value;
		for (j = 0; j < 3; j++) {
			p->org[j] = move[j] + dir[j] * cl_railspiral_radius->value;
			p->vel[j] = dir[j] * 6;
		}

		Vec3_Add_(move, vec, move);
	}
}

static void CLG_RailLights(color_t color)
{
	vec3_t fcolor;
	fcolor[0] = (float)color.u8[0] / 255.f;
	fcolor[1] = (float)color.u8[1] / 255.f;
	fcolor[2] = (float)color.u8[2] / 255.f;

	vec3_t      move;
	vec3_t      vec;
	float       len;

	Vec3_Copy_(teParameters.pos1, move);
	Vec3_Subtract_(teParameters.pos2, teParameters.pos1, vec);
	len = VectorNormalize(vec);

	float num_segments = ceilf(len / 100.f);
	float segment_size = len / num_segments;

	for (float segment = 0; segment < num_segments; segment++)
	{
		float offset = (segment + 0.25f) * segment_size;
		vec3_t pos;
		Vec3_MA_(move, offset, vec, pos);

		cdlight_t* dl = CLG_AllocDLight(0);
		Vec3_Scale_(fcolor, 0.25f, dl->color);
		Vec3_Copy_(pos, dl->origin);
		dl->radius = 400;
		dl->decay = 400;
		dl->die = cl->time + 1000;
		Vec3_Scale_(vec, segment_size * 0.5f, dl->velosity);
	}
}

extern uint32_t d_8to24table[256];
extern cvar_t* cvar_pt_beam_lights;

static void CLG_RailTrail(void)
{
	color_t rail_color;

	if (!cl_railtrail_type->integer)
	{
		rail_color.u32 = d_8to24table[0x74];

		CLG_OldRailTrail();
	}
	else
	{
		rail_color = railcore_color;

		CLG_RailCore();
		if (cl_railtrail_type->integer > 1) {
			CLG_RailSpiral();
		}
	}

	if (!cl_railtrail_type->integer || cvar_pt_beam_lights->value <= 0)
	{
		CLG_RailLights(rail_color);
	}
}

static void dirtoangles(vec3_t angles)
{
	angles[0] = acos(teParameters.dir[2]) / M_PI * 180;
	if (teParameters.dir[0])
		angles[1] = atan2(teParameters.dir[1], teParameters.dir[0]) / M_PI * 180;
	else if (teParameters.dir[1] > 0)
		angles[1] = 90;
	else if (teParameters.dir[1] < 0)
		angles[1] = 270;
	else
		angles[1] = 0;
}


//
//===============
// CLG_ParseTempEntity
// 
// Parses a temporary entity message and acts accordingly.
//===============
//
static const byte splash_color[] = { 0x00, 0xe0, 0xb0, 0x50, 0xd0, 0xe0, 0xe8 };

void CLG_ParseTempEntity(void)
{
	explosion_t* ex;
	int r;

	switch (teParameters.type) {
	case TE_BLOOD:          // bullet hitting flesh
		if (!(cl_disable_particles->integer & NOPART_BLOOD))
		{
			// CL_ParticleEffect(teParameters.pos1, teParameters.dir, 0xe8, 60);
			CLG_BloodParticleEffect(teParameters.pos1, teParameters.dir, 0xe8, 1000);
		}
		break;

	case TE_GUNSHOT:            // bullet hitting wall
		CLG_ParticleEffect(teParameters.pos1, teParameters.dir, 0, 40);
	case TE_SPARKS:
	case TE_BULLET_SPARKS:
		CLG_ParticleEffect(teParameters.pos1, teParameters.dir, 0xe0, 6);

		if (teParameters.type != TE_SPARKS) {
			CLG_SmokeAndFlash(teParameters.pos1);

			// impact sound
			r = rand() & 15;
			if (r == 1)
				clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_ric1, 1, ATTN_NORM, 0);
			else if (r == 2)
				clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_ric2, 1, ATTN_NORM, 0);
			else if (r == 3)
				clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_ric3, 1, ATTN_NORM, 0);
		}
		break;


	case TE_SCREEN_SPARKS:
	case TE_SHIELD_SPARKS:
		if (teParameters.type == TE_SCREEN_SPARKS)
			CLG_ParticleEffect(teParameters.pos1, teParameters.dir, 0xd0, 40);
		else
			CLG_ParticleEffect(teParameters.pos1, teParameters.dir, 0xb0, 40);
		//FIXME : replace or remove this sound
		clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_SHOTGUN:            // bullet hitting wall
		CLG_ParticleEffect(teParameters.pos1, teParameters.dir, 0, 20);
		CLG_SmokeAndFlash(teParameters.pos1);
		break;

	case TE_SPLASH:         // bullet hitting water
		if (teParameters.color < 0 || teParameters.color > 6)
			r = 0x00;
		else
			r = splash_color[teParameters.color];
		CLG_ParticleEffectWaterSplash(teParameters.pos1, teParameters.dir, r, teParameters.count);

		if (teParameters.color == SPLASH_SPARKS) {
			r = rand() & 3;
			if (r == 0)
				clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_spark5, 1, ATTN_STATIC, 0);
			else if (r == 1)
				clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_spark6, 1, ATTN_STATIC, 0);
			else
				clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_spark7, 1, ATTN_STATIC, 0);
		}
		break;

	case TE_LASER_SPARKS:
		CLG_ParticleEffect2(teParameters.pos1, teParameters.dir, teParameters.color, teParameters.count);
		break;

	case TE_BLUEHYPERBLASTER:
		CLG_BlasterParticles(teParameters.pos1, teParameters.dir);
		break;

	case TE_BLASTER:            // blaster hitting wall
	case TE_BLASTER2:           // green blaster hitting wall
	case TE_FLECHETTE:          // flechette
	case TE_FLARE:              // flare
		ex = CLG_AllocExplosion();
		Vec3_Copy_(teParameters.pos1, ex->ent.origin);
		dirtoangles(ex->ent.angles);
		ex->type = explosion_t::ex_blaster;
		ex->ent.flags = RF_FULLBRIGHT | RF_TRANSLUCENT;
		ex->ent.tent_type = teParameters.type;
		switch (teParameters.type) {
		case TE_BLASTER:
			CLG_BlasterParticles(teParameters.pos1, teParameters.dir);
			ex->lightcolor[0] = 1;
			ex->lightcolor[1] = 1;
			break;
		case TE_BLASTER2:
			CLG_BlasterParticles2(teParameters.pos1, teParameters.dir, 0xd0);
			ex->ent.skinnum = 1;
			ex->lightcolor[1] = 1;
			break;
		case TE_FLECHETTE:
			ex->type = explosion_t::ex_blaster;
			CLG_BlasterParticles2(teParameters.pos1, teParameters.dir, 0x6f);  // 75
			ex->ent.skinnum = 2;
			ex->lightcolor[0] = 0.19;
			ex->lightcolor[1] = 0.41;
			ex->lightcolor[2] = 0.75;
			break;
		case TE_FLARE:
			CLG_BlasterParticles2(teParameters.pos1, teParameters.dir, 0xd0);
			ex->lightcolor[0] = 1;
			ex->lightcolor[1] = 1;
			ex->type = explosion_t::ex_flare;
			break;
		}
		ex->start = cl->servertime - CL_FRAMETIME;
		ex->light = 150;
		ex->ent.model = cl_mod_explode;
		ex->frames = 4;

		if (teParameters.type != TE_FLARE)
		{
			clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		}
		else
		{
			// teParameters.count is set to 1 on the first tick of the flare, 0 afterwards
			if (teParameters.count != 0)
				clgi.S_StartSound(NULL, teParameters.entity1, 0, cl_sfx_flare, 0.5, ATTN_NORM, 0);
		}
		break;

	case TE_RAILTRAIL:          // railgun effect
		CLG_RailTrail();
		clgi.S_StartSound(teParameters.pos2, 0, 0, cl_sfx_railg, 1, ATTN_NORM, 0);
		break;

	case TE_GRENADE_EXPLOSION:
	case TE_GRENADE_EXPLOSION_WATER:
		ex = CLG_PlainExplosion(false);
		if (!cl_explosion_sprites->integer)
		{
			ex->frames = 19;
			ex->baseframe = 30;
		}
		if (cl_disable_explosions->integer & NOEXP_GRENADE)
			ex->type = explosion_t::ex_light;

		if (!(cl_disable_particles->integer & NOPART_GRENADE_EXPLOSION))
			CLG_ExplosionParticles(teParameters.pos1);

		if (teParameters.type == TE_GRENADE_EXPLOSION_WATER)
			clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0);
		else
			clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_grenexp, 1, ATTN_NORM, 0);
		break;

	case TE_EXPLOSION2:
		ex = CLG_PlainExplosion(false);
		if (!cl_explosion_sprites->integer)
		{
			ex->frames = 19;
			ex->baseframe = 30;
		}
		CLG_ExplosionParticles(teParameters.pos1);
		clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_grenexp, 1, ATTN_NORM, 0);
		break;

	case TE_PLASMA_EXPLOSION:
		CLG_PlainExplosion(false);
		CLG_ExplosionParticles(teParameters.pos1);
		clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		break;

	case TE_ROCKET_EXPLOSION:
	case TE_ROCKET_EXPLOSION_WATER:
		ex = CLG_PlainExplosion(false);
		if (cl_disable_explosions->integer & NOEXP_ROCKET)
			ex->type = explosion_t::ex_light;

		if (!(cl_disable_particles->integer & NOPART_ROCKET_EXPLOSION))
			CLG_ExplosionParticles(teParameters.pos1);

		if (teParameters.type == TE_ROCKET_EXPLOSION_WATER)
			clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0);
		else
			clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		break;

	case TE_EXPLOSION1:
		CLG_PlainExplosion(false);
		CLG_ExplosionParticles(teParameters.pos1);
		clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		break;

	case TE_EXPLOSION1_NP:
		CLG_PlainExplosion(false);
		clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		break;

	case TE_EXPLOSION1_BIG:
		ex = CLG_PlainExplosion(true);
		clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		break;

	case TE_BFG_EXPLOSION:
		ex = CLG_AllocExplosion();
		Vec3_Copy_(teParameters.pos1, ex->ent.origin);
		ex->type = explosion_t::ex_poly;
		ex->ent.flags = RF_FULLBRIGHT;
		ex->start = cl->servertime - CL_FRAMETIME;
		ex->light = 350;
		ex->lightcolor[0] = 0.0;
		ex->lightcolor[1] = 1.0;
		ex->lightcolor[2] = 0.0;
		ex->ent.model = cl_mod_bfg_explo;
		ex->ent.flags |= RF_TRANSLUCENT;
		ex->ent.alpha = 0.80;
		ex->frames = 4;
		break;

	case TE_BFG_BIGEXPLOSION:
		CLG_BFGExplosionParticles(teParameters.pos1);
		break;

	case TE_BFG_LASER:
		CLG_ParseLaser(0xd0d1d2d3);
		break;

	case TE_BUBBLETRAIL:
		CLG_BubbleTrail(teParameters.pos1, teParameters.pos2);
		break;

	case TE_PARASITE_ATTACK:
	case TE_MEDIC_CABLE_ATTACK:
		Vec3_Clear(teParameters.offset);
		teParameters.entity2 = 0;
		CLG_ParseBeam(cl_mod_parasite_segment);
		break;

	case TE_BOSSTPORT:          // boss teleporting to station
		CLG_BigTeleportParticles(teParameters.pos1);
		clgi.S_StartSound(teParameters.pos1, 0, 0, clgi.S_RegisterSound("misc/bigtele.wav"), 1, ATTN_NONE, 0);
		break;

	case TE_GRAPPLE_CABLE:
		teParameters.entity2 = 0;
		CLG_ParseBeam(cl_mod_grapple_cable);
		break;

	case TE_WELDING_SPARKS:
		CLG_ParticleEffect2(teParameters.pos1, teParameters.dir, teParameters.color, teParameters.count);

		ex = CLG_AllocExplosion();
		Vec3_Copy_(teParameters.pos1, ex->ent.origin);
		ex->type = explosion_t::ex_flash;
		// note to self
		// we need a better no draw flag
		ex->ent.flags = RF_BEAM;
		ex->start = cl->servertime - CL_FRAMETIME;
		ex->light = 100 + (rand() % 75);
		ex->lightcolor[0] = 1.0;
		ex->lightcolor[1] = 1.0;
		ex->lightcolor[2] = 0.3;
		ex->ent.model = cl_mod_flash;
		ex->frames = 2;
		break;

	case TE_GREENBLOOD:
		CLG_ParticleEffect2(teParameters.pos1, teParameters.dir, 0xdf, 30);
		break;

	case TE_TUNNEL_SPARKS:
		CLG_ParticleEffect3(teParameters.pos1, teParameters.dir, teParameters.color, teParameters.count);
		break;

	case TE_LIGHTNING:
		clgi.S_StartSound(NULL, teParameters.entity1, CHAN_WEAPON, cl_sfx_lightning, 1, ATTN_NORM, 0);
		Vec3_Clear(teParameters.offset);
		CLG_ParseBeam(cl_mod_lightning);
		break;

	case TE_DEBUGTRAIL:
		CLG_DebugTrail(teParameters.pos1, teParameters.pos2);
		break;

	case TE_PLAIN_EXPLOSION:
		CLG_PlainExplosion(false);
		break;

	case TE_FLASHLIGHT:
#if USE_DLIGHTS
		CLG_Flashlight(teParameters.entity1, teParameters.pos1);
#endif
		break;

	case TE_FORCEWALL:
		CLG_ForceWall(teParameters.pos1, teParameters.pos2, teParameters.color);
		break;

	case TE_HEATBEAM:
		Vec3_Set_(teParameters.offset, 2, 7, -3);
		CLG_ParsePlayerBeam(cl_mod_heatbeam);
		break;

	case TE_MONSTER_HEATBEAM:
		Vec3_Clear(teParameters.offset);
		CLG_ParsePlayerBeam(cl_mod_heatbeam);
		break;

	case TE_HEATBEAM_SPARKS:
		CLG_ParticleSteamEffect(teParameters.pos1, teParameters.dir, 0x8, 50, 60);
		clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_HEATBEAM_STEAM:
		CLG_ParticleSteamEffect(teParameters.pos1, teParameters.dir, 0xE0, 20, 60);
		clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_STEAM:
		CLG_ParseSteam();
		break;

	case TE_BUBBLETRAIL2:
		CLG_BubbleTrail2(teParameters.pos1, teParameters.pos2, 8);
		clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_MOREBLOOD:
		CLG_ParticleEffect(teParameters.pos1, teParameters.dir, 0xe8, 250);
		break;

	case TE_CHAINFIST_SMOKE:
		Vec3_Set_(teParameters.dir, 0, 0, 1);
		CLG_ParticleSmokeEffect(teParameters.pos1, teParameters.dir, 0, 20, 20);
		break;

	case TE_ELECTRIC_SPARKS:
		CLG_ParticleEffect(teParameters.pos1, teParameters.dir, 0x75, 40);
		//FIXME : replace or remove this sound
		clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_TRACKER_EXPLOSION:
#if USE_DLIGHTS
		CLG_ColorFlash(teParameters.pos1, 0, 150, -1, -1, -1);
#endif
		CLG_ColorExplosionParticles(teParameters.pos1, 0, 1);
		clgi.S_StartSound(teParameters.pos1, 0, 0, cl_sfx_disrexp, 1, ATTN_NORM, 0);
		break;

	case TE_TELEPORT_EFFECT:
	case TE_DBALL_GOAL:
		CLG_TeleportParticles(teParameters.pos1);
		break;

	case TE_WIDOWBEAMOUT:
		CLG_ParseWidow();
		break;

	case TE_NUKEBLAST:
		CLG_ParseNuke();
		break;

	case TE_WIDOWSPLASH:
		CLG_WidowSplash();
		break;

	default:
		Com_Error(ERR_DROP, "%s: bad type", __func__);
	}
}

//
//===============
// CLG_RegisterTEntModels
// 
// Registers all sounds used for temporary entities.
//===============
//
void CLG_RegisterTempEntitySounds(void)
{
	int     i;
	char    name[MAX_QPATH];

	// Register SFX sounds.
	cl_sfx_ric1 = clgi.S_RegisterSound("world/ric1.wav");
	cl_sfx_ric2 = clgi.S_RegisterSound("world/ric2.wav");
	cl_sfx_ric3 = clgi.S_RegisterSound("world/ric3.wav");
	cl_sfx_lashit = clgi.S_RegisterSound("weapons/lashit.wav");
	cl_sfx_flare = clgi.S_RegisterSound("weapons/flare.wav");
	cl_sfx_spark5 = clgi.S_RegisterSound("world/spark5.wav");
	cl_sfx_spark6 = clgi.S_RegisterSound("world/spark6.wav");
	cl_sfx_spark7 = clgi.S_RegisterSound("world/spark7.wav");
	cl_sfx_railg = clgi.S_RegisterSound("weapons/railgf1a.wav");
	cl_sfx_rockexp = clgi.S_RegisterSound("weapons/rocklx1a.wav");
	cl_sfx_grenexp = clgi.S_RegisterSound("weapons/grenlx1a.wav");
	cl_sfx_watrexp = clgi.S_RegisterSound("weapons/xpld_wat.wav");

	// Register Player sounds.
	clgi.S_RegisterSound("player/land1.wav");
	clgi.S_RegisterSound("player/fall2.wav");
	clgi.S_RegisterSound("player/fall1.wav");

	// Register Footstep sounds.
	for (i = 0; i < 4; i++) {
		Q_snprintf(name, sizeof(name), "player/step%i.wav", i + 1);
		cl_sfx_footsteps[i] = clgi.S_RegisterSound(name);
	}

	cl_sfx_lightning = clgi.S_RegisterSound("weapons/tesla.wav");
	cl_sfx_disrexp = clgi.S_RegisterSound("weapons/disrupthit.wav");
}

//
//===============
// CLG_RegisterTempEntityModels
// 
// Registers all models used for temporary entities.
//===============
//
void CLG_RegisterTempEntityModels(void)
{
	// Register FX models.
	cl_mod_explode = clgi.R_RegisterModel("models/objects/explode/tris.md2");
	cl_mod_smoke = clgi.R_RegisterModel("models/objects/smoke/tris.md2");
	cl_mod_flash = clgi.R_RegisterModel("models/objects/flash/tris.md2");
	cl_mod_parasite_segment = clgi.R_RegisterModel("models/monsters/parasite/segment/tris.md2");
	cl_mod_grapple_cable = clgi.R_RegisterModel("models/ctf/segment/tris.md2");
	cl_mod_explo4 = clgi.R_RegisterModel("models/objects/r_explode/tris.md2");
	cl_mod_explosions[0] = clgi.R_RegisterModel("sprites/rocket_0.sp2");
	cl_mod_explosions[1] = clgi.R_RegisterModel("sprites/rocket_1.sp2");
	cl_mod_explosions[2] = clgi.R_RegisterModel("sprites/rocket_5.sp2");
	cl_mod_explosions[3] = clgi.R_RegisterModel("sprites/rocket_6.sp2");
	cl_mod_bfg_explo = clgi.R_RegisterModel("sprites/s_bfg2.sp2");
	cl_mod_powerscreen = clgi.R_RegisterModel("models/items/armor/effect/tris.md2");
	cl_mod_laser = clgi.R_RegisterModel("models/objects/laser/tris.md2");
	cl_mod_dmspot = clgi.R_RegisterModel("models/objects/dmspot/tris.md2");

	cl_mod_lightning = clgi.R_RegisterModel("models/proj/lightning/tris.md2");
	cl_mod_heatbeam = clgi.R_RegisterModel("models/proj/beam/tris.md2");
	cl_mod_explo4_big = clgi.R_RegisterModel("models/objects/r_explode2/tris.md2");

	//
	// Configure certain models to be a vertical spriteParameters.
	//
	model_t* model = clgi.MOD_ForHandle(cl_mod_explode);
	if (model)
		model->sprite_vertical = true;

	model = clgi.MOD_ForHandle(cl_mod_smoke);
	if (model)
		model->sprite_vertical = true;

	model = clgi.MOD_ForHandle(cl_mod_flash);
	if (model)
		model->sprite_vertical = true;

	model = clgi.MOD_ForHandle(cl_mod_parasite_segment);
	if (model)
		model->sprite_vertical = true;

	model = clgi.MOD_ForHandle(cl_mod_grapple_cable);
	if (model)
		model->sprite_vertical = true;

	model = clgi.MOD_ForHandle(cl_mod_explo4);
	if (model)
		model->sprite_vertical = true;

	model = clgi.MOD_ForHandle(cl_mod_bfg_explo);
	if (model)
		model->sprite_vertical = true;

	model = clgi.MOD_ForHandle(cl_mod_powerscreen);
	if (model)
		model->sprite_vertical = true;

	model = clgi.MOD_ForHandle(cl_mod_laser);
	if (model)
		model->sprite_vertical = true;

	model = clgi.MOD_ForHandle(cl_mod_dmspot);
	if (model)
		model->sprite_vertical = true;

	model = clgi.MOD_ForHandle(cl_mod_lightning);
	if (model)
		model->sprite_vertical = true;

	model = clgi.MOD_ForHandle(cl_mod_heatbeam);
	if (model)
		model->sprite_vertical = true;

	model = clgi.MOD_ForHandle(cl_mod_explo4_big);
	if (model)
		model->sprite_vertical = true;
}

//
//===============
// CLG_AddTempEntities
// 
// Adds all temporal entities to the current frame scene.
//===============
//
void CLG_AddTempEntities(void)
{
	CLG_AddBeams();
	CLG_AddPlayerBeams();
	CLG_AddExplosions();
	CLG_ProcessSustain();
	CLG_AddLasers();
}

//
//===============
// CLG_ClearTempEntities
// 
// Clear the current temporary entities.
//===============
//
void CLG_ClearTempEntities(void)
{
	CLG_ClearBeams();
	CLG_ClearExplosions();
	CLG_ClearLasers();
	CLG_ClearSustains();
}

//
//===============
// CLG_InitTempEntities
// 
// Initialize temporary entity CVars.
//===============
//
void CLG_InitTempEntities(void)
{
	cl_railtrail_type = clgi.Cvar_Get("cl_railtrail_type", "0", 0);
	cl_railtrail_time = clgi.Cvar_Get("cl_railtrail_time", "1.0", 0);
	cl_railcore_color = clgi.Cvar_Get("cl_railcore_color", "red", 0);
	cl_railcore_color->changed = cl_railcore_color_changed;
	//cl_railcore_color->generator = Com_Color_g;
	cl_railcore_color_changed(cl_railcore_color);
	cl_railcore_width = clgi.Cvar_Get("cl_railcore_width", "2", 0);
	cl_railspiral_color = clgi.Cvar_Get("cl_railspiral_color", "blue", 0);
	cl_railspiral_color->changed = cl_railspiral_color_changed;
	//cl_railspiral_color->generator = Com_Color_g;
	cl_railspiral_color_changed(cl_railspiral_color);
	cl_railspiral_radius = clgi.Cvar_Get("cl_railspiral_radius", "3", 0);
	
	// 
	cvar_pt_beam_lights = clgi.Cvar_Get("pt_beam_lights", NULL, 0);
}
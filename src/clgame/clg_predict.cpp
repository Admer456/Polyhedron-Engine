// LICENSE HERE.

//
// clg_predict.c
//
//
// Movement prediction implementation for the client side.
//
#include "clg_local.h"

//
//===============
// CLG_CheckPredictionError
// 
// Checks for prediction errors.
//================
//
void CLG_CheckPredictionError(int frame, unsigned int cmd) {
    float delta[3];
    float len;

    // Compare what the server returned with what we had predicted it to be
    VectorSubtract(cl->frame.ps.pmove.origin, cl->predicted_origins[cmd & CMD_MASK], delta);

    len = std::fabsf(delta[0]) + std::fabsf(delta[1]) + std::fabsf(delta[2]);
    if (len < 0.125f  || len > 80.f) {
        VectorClear(cl->prediction_error);
        return;
    }

    // Don't predict steps against server returned data
    if (cl->predicted_step_frame <= cmd)
        cl->predicted_step_frame = cmd + 1;

    VectorCopy(cl->frame.ps.pmove.origin, cl->predicted_origins[cmd & CMD_MASK]);

    // Save for error interpolation
    VectorCopy(delta, cl->prediction_error);
}

//
//===============
// CLG_PredictAngles
// 
// Sets the predicted angles.
//================
//
void CLG_PredictAngles(void) {
    cl->predicted_angles[0] = cl->viewangles[0] + SHORT2ANGLE(cl->frame.ps.pmove.delta_angles[0]);
    cl->predicted_angles[1] = cl->viewangles[1] + SHORT2ANGLE(cl->frame.ps.pmove.delta_angles[1]);
    cl->predicted_angles[2] = cl->viewangles[2] + SHORT2ANGLE(cl->frame.ps.pmove.delta_angles[2]);
}

//
//===============
// CLG_ClipMoveToEntities
// 
// 
//================
//
static void CLG_ClipMoveToEntities(const vec3_t &start, const vec3_t &mins, const vec3_t &maxs, const vec3_t &end, trace_t* tr)
{
    int         i;
    trace_t     trace;
    mnode_t* headnode;
    centity_t* ent;
    mmodel_t* cmodel;

    for (i = 0; i < cl->numSolidEntities; i++) {
        ent = cl->solidEntities[i];

        if (ent->current.solid == PACKED_BSP) {
            // special value for bmodel
            cmodel = cl->model_clip[ent->current.modelindex];
            if (!cmodel)
                continue;
            headnode = cmodel->headnode;
        }
        else {
            headnode = clgi.CM_HeadnodeForBox(ent->mins, ent->maxs);
        }

        if (tr->allsolid)
            return;

        clgi.CM_TransformedBoxTrace(&trace, start, end,
            mins, maxs, headnode, CONTENTS_MASK_PLAYERSOLID,
            ent->current.origin, ent->current.angles);

        clgi.CM_ClipEntity(tr, &trace, (struct edict_s*)ent);
    }
}

/*
================
CL_PMTrace
================
*/
static trace_t q_gameabi CLG_Trace(const vec3_t &start, const vec3_t &mins, const vec3_t &maxs, const vec3_t &end)
{
    trace_t    t;

    // check against world
    clgi.CM_BoxTrace(&t, start, end, mins, maxs, cl->bsp->nodes, CONTENTS_MASK_PLAYERSOLID);
    if (t.fraction < 1.0)
        t.ent = (struct edict_s*)1;

    // check all other solid models
    CLG_ClipMoveToEntities(start, mins, maxs, end, &t);

    return t;
}

static int CLG_PointContents(const vec3_t &point)
{
    int         i;
    centity_t* ent;
    mmodel_t* cmodel;
    int         contents;

    contents = clgi.CM_PointContents(point, cl->bsp->nodes);

    for (i = 0; i < cl->numSolidEntities; i++) {
        ent = cl->solidEntities[i];

        if (ent->current.solid != PACKED_BSP) // special value for bmodel
            continue;

        cmodel = cl->model_clip[ent->current.modelindex];
        if (!cmodel)
            continue;

        contents |= clgi.CM_TransformedPointContents(
            point, cmodel->headnode,
            ent->current.origin,
            ent->current.angles);
    }

    return contents;
}

//
//================
// PM_UpdateClientSoundSpecialEffects
//
// Can be called by either the server or the client
//================
//
static void CLG_UpdateClientSoundSpecialEffects(pm_move_t* pm)
{
    static int underwater;

    // Ensure that cl != NULL, it'd be odd but hey..
    if (cl == NULL) {
        return;
    }

    if ((pm->waterLevel == 3) && !underwater) {
        underwater = 1;
        cl->snd_is_underwater = 1; // OAL: snd_is_underwater moved to client struct.
// TODO: DO!
#ifdef USE_OPENAL
        if (cl->snd_is_underwater_enabled)
            clgi.SFX_Underwater_Enable();
#endif
    }

    if ((pm->waterLevel < 3) && underwater) {
        underwater = 0;
        cl->snd_is_underwater = 0; // OAL: snd_is_underwater moved to client struct.

// TODO: DO!
#ifdef USE_OPENAL
        if (cl->snd_is_underwater_enabled)
            clgi.SFX_Underwater_Disable();
#endif
    }
}

/**
 * @brief Setup step interpolation.
 */
typedef struct {
    float height;
    uint32_t time;
    uint32_t timestamp;
    uint32_t interval;
    float delta_height;
} cl_entity_step_t;
void Cg_TraverseStep(cl_entity_step_t* step, uint32_t time, float height) {

    const uint32_t delta = time - step->timestamp;

    if (delta < step->interval) {
        const float lerp = (step->interval - delta) / (float)step->interval;
        step->height = step->height * (1.f - lerp) + height;
    }
    else {
        step->height = height;
        step->timestamp = time;
    }

    step->interval = 128.f * (fabsf(step->height) / PM_STEP_HEIGHT);
}

//
//===============
// CLG_PredictMovement
// 
// Predicts the actual client side movement.
//================
//
cl_entity_step_t stepx;
void CLG_PredictMovement(unsigned int ack, unsigned int current) {
    pm_move_t   pm = {};
    float       step, oldz;
    int         frame;

    // copy current state to pmove
    memset(&pm, 0, sizeof(pm));
    pm.Trace = CLG_Trace;
    pm.PointContents = CLG_PointContents;
    pm.state = cl->frame.ps.pmove;
#if USE_SMOOTH_DELTA_ANGLES
    VectorCopy(cl->delta_angles, pm.state.delta_angles);
#endif

    // run frames
    while (++ack <= current) {
        pm.cmd = cl->cmds[ack & CMD_MASK];
        PMove(&pm, &clg.pmoveParams);

        // Update player move client side audio effects.
        CLG_UpdateClientSoundSpecialEffects(&pm);

        // save for debug checking
        cl->predicted_origins[ack & CMD_MASK] = pm.state.origin;
    }

    // run pending cmd
    if (cl->cmd.msec) {
        pm.cmd = cl->cmd;
        pm.cmd.forwardmove = cl->localmove[0];
        pm.cmd.sidemove = cl->localmove[1];
        pm.cmd.upmove = cl->localmove[2];
        PMove(&pm, &clg.pmoveParams);
        frame = current;


        // save for debug checking
        cl->predicted_origins[(current + 1) & CMD_MASK] = pm.state.origin;
    }
    else {
        frame = current - 1;
    }

    // This only interpolates up step...
    //if (pm.state.type != PM_SPECTATOR && (pm.state.flags & PMF_ON_GROUND)) {
    //    oldz = cl->predicted_origins[cl->predicted_step_frame & CMD_MASK][2];
    //    step = pm.state.origin[2] - oldz;

    //    if (step > (63.0f / 8.0f) && step < (160.0f / 8.0f)) {
    //        cl->predicted_step = step;
    //        cl->predicted_step_time = clgi.GetRealTime();
    //        cl->predicted_step_frame = frame + 1;    // don't double step
    //    }
    //}

    if (pm.state.type != PM_SPECTATOR && (pm.state.flags & PMF_ON_GROUND)) {
        oldz = cl->predicted_origins[cl->predicted_step_frame & CMD_MASK][2];
        step = pm.state.origin[2] - oldz;

        if (step > (63.0f / 8.0f) && step < (160.0f / 8.0f)) {
            cl->predicted_step = step;
            Cg_TraverseStep(&stepx, clgi.GetRealTime(), step);
            cl->predicted_step_frame = frame + 1;    // don't double step
            stepx.time = clgi.GetRealTime();
        }
    }
    
    if (cl->predicted_step_frame < frame) {
        cl->predicted_step_frame = frame;
    }

    // copy results out for rendering
    cl->predicted_origin = pm.state.origin;
    cl->predicted_velocity = pm.state.velocity;
    cl->predicted_angles = pm.viewAngles;
}
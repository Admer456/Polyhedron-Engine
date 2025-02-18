/*
// LICENSE HERE.

//
// InfoPlayerStart.h
//
//
*/
#ifndef __SVGAME_ENTITIES_INFO_INFOPLAYERSTART_H__
#define __SVGAME_ENTITIES_INFO_INFOPLAYERSTART_H__

class SVGBaseEntity;

class InfoPlayerStart : public SVGBaseEntity {
public:
    // Constructor/Deconstructor.
    InfoPlayerStart(Entity* svEntity);
    virtual ~InfoPlayerStart();

    DefineMapClass( "info_player_start", InfoPlayerStart, SVGBaseEntity );

    // Interface functions. 
    void Precache();    // Precaches data.
    void Spawn();       // Spawns the entity.
    void PostSpawn();   // PostSpawning is for handling entity references, since they may not exist yet during a spawn period.
    void Think();       // General entity thinking routine.

    void SpawnKey(const std::string& key, const std::string& value)  override;

private:

};

#endif // __SVGAME_ENTITIES_INFO_INFOPLAYERSTART_H__
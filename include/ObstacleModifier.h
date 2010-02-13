/* bzflag
 * Copyright (c) 1993-2010 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef	BZF_OBSTACLE_MODIFIER_H
#define	BZF_OBSTACLE_MODIFIER_H

// the common header
#include "common.h"

// common headers
#include "BzMaterial.h"
#include "vectors.h"


class Obstacle;
class WorldText;
class GroupInstance;


class ObstacleModifier {
  public:
    typedef std::map<int, int>                  IntSwapMap;
    typedef std::map<std::string, std::string> TextSwapMap;

  public:
    ObstacleModifier();
    ObstacleModifier(const ObstacleModifier& obsMod,
		     const GroupInstance& grpinst);
    ~ObstacleModifier();
    void init();

    void execute(Obstacle* obstacle) const;
    void execute(WorldText* text) const;

    void getMaterialMap(const MaterialSet& matSet, MaterialMap& matMap) const;

  private:
    bool modifyTeam; // only for bases
    int team;
    bool modifyColor; // modify by tinting
    fvec4 tint;
    bool modifyPhysicsDriver; // only replaces valid physics drivers
    int phydrv;
    bool modifyMaterial; // swaps the whole thing
    const BzMaterial* material;

    MaterialMap matMap;
    IntSwapMap  phydrvMap;
    TextSwapMap textMap;
    TextSwapMap zoneMap;
    TextSwapMap weaponMap;

    unsigned char driveThrough;
    unsigned char shootThrough;
    bool          ricochet;
};


#endif // BZF_OBSTACLE_MODIFIER_H

// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8

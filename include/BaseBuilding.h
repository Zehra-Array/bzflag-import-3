/* bzflag
 * Copyright (c) 1993 - 2001 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named LICENSE that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* BaseBuilding:
 *	Encapsulates a base in the game environment
 */

#ifndef BZF_BASE_BUILDING_H
#define BZF_BASE_BUILDING_H

#include "Obstacle.h"
#include <vector>

class BaseBuilding : public Obstacle {
public:
	BaseBuilding(const float *pos, float rotation,
							const float *size, int _team);
	~BaseBuilding();

	BzfString			getType() const;
	BzfString			getClassName(); // const
	float				intersect(const Ray &) const;
	void				getNormal(const float *p, float *n) const;
	bool				isInside(const float *p, float radius) const;
	bool				isInside(const float *p, float angle,
							float halfWidth, float halfBreadth) const;
	bool				isCrossing(const float *p, float angle,
							float halfWidth, float halfBreadth,
							float *plane) const;
	bool				getHitNormal(const float *pos1, float azimuth1,
							const float *pos2, float azimuth2,
							float halfWidth, float halfBreadth,
							float *normal) const;
	void				getCorner(int index, float *pos) const;
	const int			getTeam() const;

private:
	static BzfString	typeName;
	int					team;
};

typedef std::vector<BaseBuilding> BaseBuildings;

#endif

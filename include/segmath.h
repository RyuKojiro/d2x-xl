/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _SEGMATH_H
#define _SEGMATH_H

#include "pstypes.h"
#include "fix.h"
#include "vecmat.h"
#include "segment.h"

#define DEFAULT_PLANE_DIST_TOLERANCE	250
#define PLANE_DIST_TOLERANCE				gameStates.render.xPlaneDistTolerance

// -----------------------------------------------------------------------------
//Tries to find a CSegment for a point, in the following way:
// 1. Check the given CSegment
// 2. Recursively trace through attached segments
// 3. Check all the segmentns
//Returns nSegment if found, or -1
int32_t FindSegByPos (const CFixVector& vPos, int32_t nSegment, int32_t bExhaustive, int32_t bSkyBox, fix xTolerance = 0, int32_t nThread = 0);
int32_t FindSegByPosExhaustive (const CFixVector& vPos, int32_t bSkyBox, int32_t nStartSeg = -1);
int16_t FindClosestSeg (CFixVector& vPos);

// -----------------------------------------------------------------------------
// Determine whether seg0 and seg1 are reachable using widFlag to go through walls.
// For example, set to WID_TRANSPARENT_FLAG to see if sound can get from one CSegment to the other.
// set to WID_PASSABLE_FLAG to see if a robot could fly from one to the other.
// Search up to a maximum depth of max_depth.
// Return the distance.
//create a matrix that describes the orientation of the given CSegment
void ExtractOrientFromSegment (CFixMatrix *m,CSegment *seg);

uint16_t SortVertsForNormal (uint16_t v0, uint16_t v1, uint16_t v2, uint16_t v3, uint16_t* vSorted);
void AddToVertexNormal (int32_t nVertex, CFixVector& vNormal);
void ComputeVertexNormals (void);
void ResetVertexNormals (void);

// -----------------------------------------------------------------------------

#endif



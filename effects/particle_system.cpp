
//particle.h
//simple particle system handler

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#ifdef __macosx__
# include <SDL/SDL.h>
#else
# include <SDL.h>
#endif

#include "pstypes.h"
#include "descent.h"
#include "error.h"
#include "u_mem.h"
#include "vecmat.h"
#include "hudmsgs.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_shader.h"
#include "ogl_fastrender.h"
#include "piggy.h"
#include "globvars.h"
#include "segmath.h"
#include "network.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "renderlib.h"
#include "rendermine.h"
#include "transprender.h"
#include "objsmoke.h"
#include "glare.h"
#include "particles.h"
#include "renderthreads.h"
#include "automap.h"

//------------------------------------------------------------------------------

int32_t CParticleSystem::Create (CFixVector *vPos, CFixVector *vDir, CFixMatrix *mOrient,
											int16_t nSegment, int32_t nMaxEmitters, int32_t nMaxParts,
											float fScale, /*int32_t nDensity, int32_t nPartsPerPos,*/ int32_t nLife, int32_t nSpeed, char nType,
											int32_t nObject, CFloatVector *pColor, int32_t bBlowUpParts, char nSide)
{
	int32_t		i;
	CFixVector	vEmittingFace [4];

if (nSide >= 0)
	SEGMENT (nSegment)->GetCornerVertices (nSide, vEmittingFace);
nMaxParts = MAX_PARTICLES (nMaxParts, gameOpts->render.particles.nDens [0]);
if (!m_emitters.Create (nMaxEmitters)) {
	return 0;
	}

CObject* pObj;

if (((m_nObject = nObject) < 0x70000000) && (pObj = OBJECT (m_nObject))) {
	m_nSignature = pObj->info.nSignature;
	m_nObjType = pObj->info.nType;
	m_nObjId = pObj->info.nId;
	}
m_nEmitters = 0;
m_nLife = nLife;
m_nSpeed = nSpeed;
m_nBirth = gameStates.app.nSDLTicks [0];
m_nMaxEmitters = nMaxEmitters;
for (i = 0; i < nMaxEmitters; i++)
	if (m_emitters [i].Create (vPos, vDir, mOrient, nSegment, nObject, nMaxParts, fScale, /*nDensity, nPartsPerPos,*/ 
										nLife, nSpeed, nType, pColor, gameStates.app.nSDLTicks [0], bBlowUpParts, (nSide < 0) ? NULL : vEmittingFace))
		m_nEmitters++;
	else {
		particleManager.Destroy (m_nId);
		return -1;
		}
m_nType = nType;
m_bValid = 1;
return 1;
}

//	-----------------------------------------------------------------------------

void CParticleSystem::Init (int32_t nId)
{
m_nId = nId;
m_nObject = -1;
m_nObjType = -1;
m_nObjId = -1;
m_nSignature = -1;
m_bValid = 0;
m_bDestroy = false;
}

//------------------------------------------------------------------------------

void CParticleSystem::Destroy (void)
{
m_bValid = 0;
m_bDestroy = false;
if (m_emitters.Buffer ()) {
	m_emitters.Destroy ();
	if ((m_nObject >= 0) && (m_nObject < 0x70000000))
		particleManager.SetObjectSystem (m_nObject, -1);
	m_nObject = -1;
	m_nObjType = -1;
	m_nObjId = -1;
	m_nSignature = -1;
	}
}

//------------------------------------------------------------------------------

int32_t CParticleSystem::Render (int32_t nThread)
{
if (m_bValid < 1)
	return 0;

	int32_t	h = 0;
	CParticleEmitter* pEmitter = m_emitters.Buffer ();

if (pEmitter) {
	if (!particleImageManager.Load (m_nType))
		return 0;
	if (m_nObject < 0x70000000) {
		CObject* pObj = OBJECT (m_nObject);
		if (!pObj)
			SetLife (0);
		else {
			if ((pObj == LOCALOBJECT) && !gameStates.render.bChaseCam && !gameStates.render.bFreeCam)
				return 0;
			if ((pObj->Type () == OBJ_NONE) ||
				 (pObj->Signature () != m_nSignature) ||
				 ((particleManager.GetObjectSystem (m_nObject, 0) < 0) && (particleManager.GetObjectSystem (m_nObject, 1) < 0)))
				SetLife (0);
			}
		}
	for (int32_t i = m_nEmitters; i; i--, pEmitter++)
		h += pEmitter->Render (nThread);
	}
#if DBG
if (!h)
	return 0;
#endif
return h;
}

//------------------------------------------------------------------------------

void CParticleSystem::SetPos (CFixVector *vPos, CFixMatrix *mOrient, int16_t nSegment)
{
if (m_bValid && m_emitters.Buffer ())
	for (int32_t i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetPos (vPos, mOrient, nSegment);
}

//------------------------------------------------------------------------------

void CParticleSystem::SetDensity (int32_t nMaxParts/*, int32_t nDensity*/)
{
if (m_bValid && m_emitters.Buffer ()) {
	nMaxParts = MAX_PARTICLES (nMaxParts, gameOpts->render.particles.nDens [0]);
	for (int32_t i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetDensity (nMaxParts/*, nDensity*/);
	}
}

//------------------------------------------------------------------------------

void CParticleSystem::SetScale (float fScale)
{
if (m_bValid && m_emitters.Buffer ())
	for (int32_t i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetScale (fScale);
}

//------------------------------------------------------------------------------

void CParticleSystem::SetLife (int32_t nLife)
{
if (m_bValid && m_emitters.Buffer () && (m_nLife != nLife)) {
	m_nLife = nLife;
	for (int32_t i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetLife (nLife);
	}
}

//------------------------------------------------------------------------------

void CParticleSystem::SetBlowUp (int32_t bBlowUpParts)
{
if (m_bValid && m_emitters.Buffer ()) {
	for (int32_t i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetBlowUp (bBlowUpParts);
	}
}

//------------------------------------------------------------------------------

void CParticleSystem::SetBrightness (int32_t nBrightness)
{
if (m_bValid && m_emitters.Buffer ())
	for (int32_t i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetBrightness (nBrightness);
}

//------------------------------------------------------------------------------

void CParticleSystem::SetFadeType (int32_t nFadeType)
{
if (m_bValid && m_emitters.Buffer ())
	for (int32_t i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetFadeType (nFadeType);
}

//------------------------------------------------------------------------------

void CParticleSystem::SetType (int32_t nType)
{
if (m_bValid && m_emitters.Buffer () && (m_nType != nType)) {
	m_nType = nType;
	for (int32_t i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetType (nType);
	}
}

//------------------------------------------------------------------------------

void CParticleSystem::SetSpeed (int32_t nSpeed)
{
if (m_bValid && m_emitters.Buffer () && (m_nSpeed != nSpeed)) {
	m_nSpeed = nSpeed;
	for (int32_t i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetSpeed (nSpeed);
	}
}

//------------------------------------------------------------------------------

void CParticleSystem::SetDir (CFixVector *vDir)
{
if (m_bValid && m_emitters.Buffer ())
	for (int32_t i = 0; i < m_nEmitters; i++)
		m_emitters [i].SetDir (vDir);
}

//------------------------------------------------------------------------------

int32_t CParticleSystem::RemoveEmitter (int32_t i)
{
if (m_bValid && m_emitters.Buffer () && (i < m_nEmitters)) {
	m_emitters [i].Destroy ();
	if (i < --m_nEmitters)
		m_emitters [i] = m_emitters [m_nEmitters];
	}
return m_nEmitters;
}

//------------------------------------------------------------------------------

int32_t CParticleSystem::Update (int32_t nThread)
{
if (m_bValid < 1)
	return 0;

	CParticleEmitter*				pEmitter;
	CArray<CParticleEmitter*>	emitters;
	int32_t							nEmitters = 0;

if ((m_nObject == 0x7fffffff) && (m_nType <= SMOKE_PARTICLES) &&
	 (gameStates.app.nSDLTicks [0] - m_nBirth > (MAX_SHRAPNEL_LIFE / I2X (1)) * 1000))
	SetLife (0);

if ((pEmitter = m_emitters.Buffer ()) && emitters.Create (m_nEmitters)) {
	CObject *pObj;
	bool bKill = (m_nObject < 0x70000000) && (!(pObj = OBJECT (m_nObject)) || (pObj->info.nSignature != m_nSignature) || (pObj->info.nType == OBJ_NONE));
	while (nEmitters < m_nEmitters) {
		if (!pEmitter)
			return 0;
		if (pEmitter->IsDead (gameStates.app.nSDLTicks [0])) {
			if (!RemoveEmitter (nEmitters)) {
				m_bDestroy = true;
				break;
				}
			}
		else {
			if (bKill)
				pEmitter->SetLife (0);
			pEmitter->Update (gameStates.app.nSDLTicks [0], nThread);
			nEmitters++, pEmitter++;
			}
		}
	}
return nEmitters;
}

//------------------------------------------------------------------------------
//eof

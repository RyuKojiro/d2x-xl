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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "descent.h"
#include "error.h"
#include "objrender.h"
#include "key.h"
#include "input.h"
#include "fireball.h"
#include "cockpit.h"
#include "text.h"
#include "scores.h"
#include "light.h"
#include "kconfig.h"
#include "rendermine.h"
#include "newdemo.h"
#include "objeffects.h"
#include "sphere.h"

int32_t ReturnFlagHome (CObject *pObj);
void InvalidateEscortGoal (void);
void MultiSendGotFlag (uint8_t);

const char *pszPowerup [MAX_POWERUP_TYPES] = {
	"extra life",
	"energy",
	"shield",
	"laser",
	"blue key",
	"red key",
	"gold key",
	"hoard orb",
	"monsterball",
	"",
	"concussion missile",
	"concussion missile (4)",
	"quad laser",
	"vulcan gun",
	"spreadfire gun",
	"plasma gun",
	"fusion gun",
	"prox mine",
	"homing missile",
	"homing missile (4)",
	"smart missile",
	"mega missile",
	"vulcan ammo",
	"cloak",
	"turbo",
	"invul",
	"",
	"megawow",
	"gauss cannon",
	"helix cannon",
	"phoenix cannon",
	"omega cannon",
	"super laser",
	"full map",
	"converter",
	"ammo rack",
	"after burner",
	"headlight",
	"flash missile",
	"flash missile (4)",
	"guided missile",
	"guided missile (4)",
	"smart mine",
	"mercury missile",
	"mercury missile (4)",
	"earth shaker",
	"blue flag",
	"red flag",
	"slow motion",
	"bullet time"
	};

#define	MAX_INV_ITEMS	((5 - gameStates.app.nDifficultyLevel) * ((pPlayer->flags & PLAYER_FLAGS_AMMO_RACK) ? 2 : 1))

int32_t powerupToDevice [MAX_POWERUP_TYPES];
char powerupToWeaponCount [MAX_POWERUP_TYPES];
char powerupClass [MAX_POWERUP_TYPES];
char powerupToObject [MAX_POWERUP_TYPES];
int16_t powerupToModel [MAX_POWERUP_TYPES];
int16_t weaponToModel [D2_MAX_WEAPON_TYPES];
uint8_t powerupType [MAX_POWERUP_TYPES];
uint8_t powerupFilter [MAX_POWERUP_TYPES];
void *pickupHandler [MAX_POWERUP_TYPES];

//------------------------------------------------------------------------------

static int32_t nDbgMinFrame = 0;

void UpdatePowerupClip (tAnimationInfo *pAnimInfo, tAnimationState *pClip, int32_t nObject)
{
if (pAnimInfo) {
	static fix	xPowerupTime = 0;
	int32_t		h, nFrames = SetupHiresVClip (pAnimInfo, pClip);
	fix			xTime, xFudge = (xPowerupTime * (nObject & 3)) >> 4;

	xPowerupTime += gameData.physicsData.xTime;
	xTime = pClip->xFrameTime - (fix) ((xPowerupTime + xFudge) / gameStates.gameplay.slowmo [0].fSpeed);
	if ((xTime < 0) && (pAnimInfo->xFrameTime > 0)) {
		h = (-xTime + pAnimInfo->xFrameTime - 1) / pAnimInfo->xFrameTime;
		xTime += h * pAnimInfo->xFrameTime;
		h %= nFrames;
		if ((nObject & 1) && (OBJECT (nObject)->info.nType != OBJ_EXPLOSION)) 
			pClip->nCurFrame -= h;
		else
			pClip->nCurFrame += h;
		if (pClip->nCurFrame < nDbgMinFrame)
			pClip->nCurFrame = nFrames - (-pClip->nCurFrame % nFrames);
		else 
			pClip->nCurFrame %= nFrames;
		}
#if DBG
	if (pClip->nCurFrame < nDbgMinFrame)
		pClip->nCurFrame = nDbgMinFrame;
#endif
	pClip->xFrameTime = xTime;
	xPowerupTime = 0;
	}
else {
	int32_t	h, nFrames;

	if (0 > (h = (gameStates.app.nSDLTicks [0] - pClip->xTotalTime))) {
		pClip->xTotalTime = gameStates.app.nSDLTicks [0];
		h = 0;
		}
	else if ((h = h / 80) && (nFrames = gameData.pigData.tex.addonBitmaps [-pClip->nClipIndex - 1].FrameCount ())) { //???
		pClip->xTotalTime += h * 80;
		if (gameStates.app.nSDLTicks [0] < pClip->xTotalTime)
			pClip->xTotalTime = gameStates.app.nSDLTicks [0];
		if (nObject & 1)
			pClip->nCurFrame += h;
		else
			pClip->nCurFrame -= h;
		if (pClip->nCurFrame < 0) {
			if (!(h = -pClip->nCurFrame % nFrames))
				h = 1;
			pClip->nCurFrame = nFrames - h;
			}
		else 
			pClip->nCurFrame %= nFrames;
		}
#if DBG
	if (pClip->nCurFrame < nDbgMinFrame)
		pClip->nCurFrame = nDbgMinFrame;
#endif
	}
}

//------------------------------------------------------------------------------

void UpdateFlagClips (void)
{
if (!gameStates.app.bDemoData) {
	UpdatePowerupClip (gameData.pigData.flags [0].pAnimInfo, &gameData.pigData.flags [0].animState, 0);
	UpdatePowerupClip (gameData.pigData.flags [1].pAnimInfo, &gameData.pigData.flags [1].animState, 0);
	}
}

//------------------------------------------------------------------------------
//process this powerup for this frame
void CObject::DoPowerupFrame (void)
{
	int32_t	i = OBJ_IDX (this);
//if (gameStates.app.tick40fps.bTick) 
if (info.renderType != RT_POLYOBJ) {
	tAnimationState	*pClip = &rType.animationInfo;
	tAnimationInfo		*pAnimInfo = ((pClip->nClipIndex < 0) || (pClip->nClipIndex >= MAX_ANIMATIONS_D2)) ? NULL : gameData.effectData.animations [0] + pClip->nClipIndex;
	UpdatePowerupClip (pAnimInfo, pClip, i);
	}
if (info.xLifeLeft <= 0) {
	CreateExplosion (this, info.nSegment, info.position.vPos, info.position.vPos, I2X (7) / 2, ANIM_POWERUP_DISAPPEARANCE);
	if (gameData.effectData.animations [0][ANIM_POWERUP_DISAPPEARANCE].nSound > -1)
		audio.CreateObjectSound (gameData.effectData.animations [0][ANIM_POWERUP_DISAPPEARANCE].nSound, SOUNDCLASS_GENERIC, i);
	}
}

//------------------------------------------------------------------------------

void DrawPowerup (CObject *pObj)
{
#if DBG
//return;
#endif
if (pObj->info.nType == OBJ_MONSTERBALL) {
	DrawMonsterball (pObj, 1.0f, 0.5f, 0.0f, 0.9f);
	RenderMslLockIndicator (pObj);
	}
else if ((pObj->rType.animationInfo.nClipIndex >= -MAX_ADDON_BITMAP_FILES) && (pObj->rType.animationInfo.nClipIndex < MAX_ANIMATIONS_D2)) {
	if ((pObj->info.nId < MAX_POWERUP_TYPES_D2) || ((pObj->info.nType == OBJ_EXPLOSION) && (pObj->info.nId < MAX_ANIMATIONS_D2))) {
			tBitmapIndex	*pFrame = gameData.effectData.animations [0][pObj->rType.animationInfo.nClipIndex].frames;
			int32_t			iFrame = pObj->rType.animationInfo.nCurFrame;
		DrawObjectBitmap (pObj, pFrame->index, pFrame [iFrame].index, iFrame, NULL, 0);
		}
	else {
		DrawObjectBitmap (pObj, pObj->rType.animationInfo.nClipIndex, pObj->rType.animationInfo.nClipIndex, pObj->rType.animationInfo.nCurFrame, NULL, 1);
		}
	}
#if DBG
else
	PrintLog (0, "invalid powerup clip index\n");
#endif
}

//------------------------------------------------------------------------------

void _CDECL_ PickupEffect (int32_t redAdd, int32_t greenAdd, int32_t blueAdd, int32_t score, const char *format, ...)
{
	char		text[120];
	va_list	args;

va_start (args, format);
vsprintf (text, format, args);
va_end (args);
#if 0
paletteManager.BumpEffect (redAdd, greenAdd, blueAdd);
#else
float h = 48.0f / float (Max (redAdd, Max (greenAdd, blueAdd)));
paletteManager.BumpEffect (int32_t (float (redAdd) * h), int32_t (float (greenAdd) * h), int32_t (float (blueAdd) * h));
#endif
HUDInitMessage (text);
//mprintf_gameData.objs.pwrUp.Info ();
cockpit->AddPointsToScore (score);
}

//------------------------------------------------------------------------------

//#if DBG
//	Give the megawow powerup!
void DoMegaWowPowerup (int32_t quantity)
{
	int32_t i;

PickupEffect (30, 0, 30, 1, "MEGA-WOWIE-ZOWIE!");
LOCALPLAYER.primaryWeaponFlags = 0xffff ^ HAS_FLAG (SUPER_LASER_INDEX);		//no super laser
LOCALPLAYER.secondaryWeaponFlags = 0xffff;
for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	LOCALPLAYER.primaryAmmo[i] = VULCAN_AMMO_MAX;
for (i = 0; i < 3; i++)
	LOCALPLAYER.secondaryAmmo[i] = quantity;
for (i = 3; i < MAX_SECONDARY_WEAPONS; i++)
	LOCALPLAYER.secondaryAmmo[i] = quantity/5;
if (gameData.demoData.nState == ND_STATE_RECORDING)
	NDRecordLaserLevel (LOCALPLAYER.LaserLevel (), MAX_LASER_LEVEL);
LOCALPLAYER.SetEnergy (I2X (200));
LOCALPLAYER.SetShield (LOCALPLAYER.MaxShield ());
LOCALPLAYER.flags |= PLAYER_FLAGS_QUAD_LASERS;
LOCALPLAYER.SetSuperLaser (MAX_SUPERLASER_LEVEL - MAX_LASER_LEVEL);
if (IsHoardGame)
	LOCALPLAYER.secondaryAmmo[PROXMINE_INDEX] = 12;
else if (IsEntropyGame)
	LOCALPLAYER.secondaryAmmo[PROXMINE_INDEX] = 15;
cockpit->UpdateLaserWeaponInfo ();
}
//#endif

//------------------------------------------------------------------------------

int32_t PickupEnergyBoost (CObject *pObj, int32_t nPlayer)
{
	CPlayerData	*pPlayer = gameData.multiplayer.players + nPlayer;

if (pPlayer->Energy () < pPlayer->MaxEnergy ()) {
	fix boost = I2X (3) * (DIFFICULTY_LEVEL_COUNT - gameStates.app.nDifficultyLevel + 1);
	if (gameStates.app.nDifficultyLevel == 0)
		boost += boost / 2;
	pPlayer->UpdateEnergy (boost);
	if (ISLOCALPLAYER (nPlayer))
		PickupEffect (15, 7, 0, ENERGY_SCORE, "%s %s %d", TXT_ENERGY, TXT_BOOSTED_TO, X2IR (pPlayer->energy));
	return 1;
	} 
else if (ISLOCALPLAYER (nPlayer))
	HUDInitMessage (TXT_MAXED_OUT, TXT_ENERGY);
return 0;
}

//------------------------------------------------------------------------------

int32_t PickupShieldBoost (CObject *pObj, int32_t nPlayer)
{
	CPlayerData	*pPlayer = gameData.multiplayer.players + nPlayer;

if (pPlayer->Shield () < pPlayer->MaxShield ()) {
	fix boost = I2X (3) * (DIFFICULTY_LEVEL_COUNT - gameStates.app.nDifficultyLevel + 1);
	if (gameStates.app.nDifficultyLevel == 0)
		boost += boost / 2;
	pPlayer->UpdateShield (boost);
	if (ISLOCALPLAYER (nPlayer)) {
		PickupEffect (0, 0, 15, SHIELD_SCORE, "%s %s %d", TXT_SHIELD, TXT_BOOSTED_TO, X2IR (pPlayer->Shield ()));
		NetworkFlushData (); // will send position, shield and weapon info
		}
	OBJECT (nPlayer)->ResetDamage ();
	return 1;
	}
else if (ISLOCALPLAYER (nPlayer)) {
	if (OBJECT (N_LOCALPLAYER)->ResetDamage ())
		return 1;
	else
		HUDInitMessage (TXT_MAXED_OUT, TXT_SHIELD);
	}
else
	OBJECT (nPlayer)->ResetDamage ();
return 0;
}

//	-----------------------------------------------------------------------------

int32_t PickupCloakingDevice (CObject *pObj, int32_t nPlayer)
{
	CPlayerData *pPlayer = gameData.multiplayer.players + nPlayer;

if (!gameOpts->gameplay.bInventory || (IsMultiGame && !IsCoopGame)) 
	return -ApplyCloak (1, nPlayer);
if (pPlayer->nCloaks < MAX_INV_ITEMS) {
	pPlayer->nCloaks++;
	PickupEffect (15, 0, 15, 0, "%s!", TXT_CLOAKING_DEVICE);
	return 1;
	}
if (ISLOCALPLAYER (nPlayer))
	HUDInitMessage ("%s", TXT_INVENTORY_FULL);
return 0;
}

//	-----------------------------------------------------------------------------

int32_t PickupInvulnerability (CObject *pObj, int32_t nPlayer)
{
	CPlayerData *pPlayer = gameData.multiplayer.players + nPlayer;

if (!gameOpts->gameplay.bInventory || (IsMultiGame && !IsCoopGame)) 
	return -ApplyInvul (1, nPlayer);
if (pPlayer->nInvuls < MAX_INV_ITEMS) {
	pPlayer->nInvuls++;
	PickupEffect (0, 7, 15, 0, "%s!", TXT_INVULNERABILITY);
	return 1;
	}
if (ISLOCALPLAYER (nPlayer))
	HUDInitMessage ("%s", TXT_INVENTORY_FULL);
return 0;
}

//------------------------------------------------------------------------------

int32_t PickupExtraLife (CObject *pObj, int32_t nPlayer)
{
PLAYER (nPlayer).lives++;
if (nPlayer == N_LOCALPLAYER)
	PickupEffect (0, 15, 7, 0, TXT_EXTRA_LIFE);
return 1;
}

//	-----------------------------------------------------------------------------

int32_t PickupHoardOrb (CObject *pObj, int32_t nPlayer)
{
	CPlayerData *pPlayer = gameData.multiplayer.players + nPlayer;

if (IsHoardGame) {
	if (pPlayer->secondaryAmmo [PROXMINE_INDEX] < 12) {
		if (ISLOCALPLAYER (nPlayer)) {
			MultiSendGotOrb (N_LOCALPLAYER);
			PickupEffect (7, 15, 7, 0, "Orb!!!", nPlayer);
			}
		pPlayer->secondaryAmmo [PROXMINE_INDEX]++;
		pPlayer->flags |= PLAYER_FLAGS_FLAG;
		return 1;
		}
	}
else if (IsEntropyGame) {
	if (pObj->info.nCreator != GetTeam (N_LOCALPLAYER) + 1) {
		if ((extraGameInfo [1].entropy.nVirusStability < 2) ||
			 ((extraGameInfo [1].entropy.nVirusStability < 3) && 
			 ((SEGMENT (pObj->info.nSegment)->m_owner != pObj->info.nCreator) ||
			 (SEGMENT (pObj->info.nSegment)->m_function != SEGMENT_FUNC_ROBOTMAKER))))
			pObj->Die ();	//make orb disappear if touched by opposing team CPlayerData
		}
	else if (!extraGameInfo [1].entropy.nMaxVirusCapacity ||
				(pPlayer->secondaryAmmo [PROXMINE_INDEX] < pPlayer->secondaryAmmo [SMARTMINE_INDEX])) {
		if (ISLOCALPLAYER (nPlayer)) {
			MultiSendGotOrb (N_LOCALPLAYER);
			PickupEffect (15, 0, 15, 0, "Virus!!!", nPlayer);
			}
		pPlayer->secondaryAmmo [PROXMINE_INDEX]++;
		pPlayer->flags |= PLAYER_FLAGS_FLAG;
		return 1;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

int32_t PickupEquipment (CObject *pObj, int32_t nEquipment, const char *pszHave, const char *pszGot, int32_t nPlayer)
{
	CPlayerData	*pPlayer = gameData.multiplayer.players + nPlayer;
	int32_t		id, bPickedUp = 0;

if (pPlayer->flags & nEquipment) {
	if (ISLOCALPLAYER (nPlayer))
		HUDInitMessage ("%s %s!", TXT_ALREADY_HAVE, pszHave);
	if (!IsMultiGame)
		bPickedUp = PickupEnergyBoost (pObj, nPlayer);
	} 
else {
	pPlayer->flags |= nEquipment;
	if (ISLOCALPLAYER (nPlayer)) {
		id = pObj->info.nId;
		if (id >= MAX_POWERUP_TYPES_D2)
			id = POW_AFTERBURNER;
		MultiSendPlaySound (gameData.objData.pwrUp.info [id].hitSound, I2X (1));
		audio.PlaySound ((int16_t) gameData.objData.pwrUp.info [id].hitSound);
		PickupEffect (15, 0, 15, 0, pszGot, nPlayer);
		}
	bPickedUp = -1;
	}
return bPickedUp;
}

//	-----------------------------------------------------------------------------

int32_t PickupHeadlight (CObject *pObj, int32_t nPlayer)
{
	CPlayerData *pPlayer = gameData.multiplayer.players + nPlayer;
	char		szTemp [50];

sprintf (szTemp, TXT_GOT_HEADLIGHT, (EGI_FLAG (headlight.bAvailable, 0, 0, 1) && gameOpts->gameplay.bHeadlightOnWhenPickedUp) ? TXT_ON : TXT_OFF);
HUDInitMessage (szTemp);
int32_t bPickedUp = PickupEquipment (pObj, PLAYER_FLAGS_HEADLIGHT, TXT_THE_HEADLIGHT, szTemp, nPlayer);
if (bPickedUp >= 0)
	return bPickedUp;
if (ISLOCALPLAYER (nPlayer)) {
	if (EGI_FLAG (headlight.bAvailable, 0, 0, 1)  && gameOpts->gameplay.bHeadlightOnWhenPickedUp)
		pPlayer->flags |= PLAYER_FLAGS_HEADLIGHT_ON;
	if IsMultiGame
		MultiSendFlags (N_LOCALPLAYER);
	}
return 1;
}

//	-----------------------------------------------------------------------------

int32_t PickupFullMap (CObject *pObj, int32_t nPlayer)
{
return PickupEquipment (pObj, PLAYER_FLAGS_FULLMAP, TXT_THE_FULLMAP, TXT_GOT_FULLMAP, nPlayer) ? 1 : 0;
}


//	-----------------------------------------------------------------------------

int32_t PickupConverter (CObject *pObj, int32_t nPlayer)
{
	char		szTemp [50];

sprintf (szTemp, TXT_GOT_CONVERTER, KeyToASCII (controls.GetKeyValue (56)));
HUDInitMessage (szTemp);
return PickupEquipment (pObj, PLAYER_FLAGS_CONVERTER, TXT_THE_CONVERTER, szTemp, nPlayer) != 0;
}

//	-----------------------------------------------------------------------------

int32_t PickupAmmoRack (CObject *pObj, int32_t nPlayer)
{
return (gameData.multiplayer.weaponStates [nPlayer].nShip != 0)
		 ? 0
		 : PickupEquipment (pObj, PLAYER_FLAGS_AMMO_RACK, TXT_THE_AMMORACK, TXT_GOT_AMMORACK, nPlayer) != 0;
}

//	-----------------------------------------------------------------------------

int32_t PickupAfterburner (CObject *pObj, int32_t nPlayer)
{
	int32_t bPickedUp = PickupEquipment (pObj, PLAYER_FLAGS_AFTERBURNER, TXT_THE_BURNER, TXT_GOT_BURNER, nPlayer);
	
if (bPickedUp >= 0)
	return bPickedUp;
gameData.physicsData.xAfterburnerCharge = I2X (1);
return 1;
}

//	-----------------------------------------------------------------------------

int32_t PickupSlowMotion (CObject *pObj, int32_t nPlayer)
{
return PickupEquipment (pObj, PLAYER_FLAGS_SLOWMOTION, TXT_THE_SLOWMOTION, TXT_GOT_SLOWMOTION, nPlayer) != 0;
}

//	-----------------------------------------------------------------------------

int32_t PickupBulletTime (CObject *pObj, int32_t nPlayer)
{
return PickupEquipment (pObj, PLAYER_FLAGS_BULLETTIME, TXT_THE_BULLETTIME, TXT_GOT_BULLETTIME, nPlayer) != 0;
}

//------------------------------------------------------------------------------

int32_t PickupKey (CObject *pObj, int32_t nKey, const char *pszKey, int32_t nPlayer)
{
//if (ISLOCALPLAYER (nPlayer)) 
	{
	CPlayerData	*pPlayer = gameData.multiplayer.players + nPlayer;

	if (pPlayer->flags & nKey)
		return 0;
	if (pObj) {
		MultiSendPlaySound (gameData.objData.pwrUp.info [pObj->info.nId].hitSound, I2X (1));
		audio.PlaySound ((int16_t) gameData.objData.pwrUp.info [pObj->info.nId].hitSound);
		}
	pPlayer->flags |= nKey;
	PickupEffect (15, 0, 0, ISLOCALPLAYER (nPlayer) ? KEY_SCORE : 0, "%s %s", pszKey, TXT_ACCESS_GRANTED);
	InvalidateEscortGoal ();
	return IsMultiGame == 0;
	}
return 0;
}

//------------------------------------------------------------------------------

int32_t PickupFlag (CObject *pObj, int32_t nThisTeam, int32_t nOtherTeam, const char *pszFlag, int32_t nPlayer)
{
if (ISLOCALPLAYER (nPlayer)) {
	CPlayerData	*pPlayer = gameData.multiplayer.players + nPlayer;
	if (gameData.appData.GameMode (GM_CAPTURE)) {
		if (GetTeam (N_LOCALPLAYER) == nOtherTeam) {
			PickupEffect (15, 0, 15, 0, nOtherTeam ? "RED FLAG!" : "BLUE FLAG!", nPlayer);
			pPlayer->flags |= PLAYER_FLAGS_FLAG;
			gameData.pigData.flags [nThisTeam].path.Reset (10, -1);
			MultiSendGotFlag (N_LOCALPLAYER);
			return 1;
			}
		if (GetTeam (N_LOCALPLAYER) == nThisTeam) {
			ReturnFlagHome (pObj);
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------

void UsePowerup (int32_t id)
{
	int32_t	bApply = id < 0;

if (bApply)
	id = -id;
if (id >= MAX_POWERUP_TYPES_D2)
	id = POW_AFTERBURNER;
if (gameData.objData.pwrUp.info [id].hitSound > -1) {
	if (!bApply && (gameOpts->gameplay.bInventory && (!IsMultiGame || IsCoopGame)) && ((id == POW_CLOAK) || (id == POW_INVUL)))
		id = POW_SHIELD_BOOST;
	if (IsMultiGame) // Added by Rob, take this out if it turns out to be not good for net games!
		MultiSendPlaySound (gameData.objData.pwrUp.info [id].hitSound, I2X (1));
	audio.PlaySound (int16_t (gameData.objData.pwrUp.info [id].hitSound));
	}
}

//------------------------------------------------------------------------------

int32_t ApplyInvul (int32_t bForce, int32_t nPlayer)
{
	CPlayerData *pPlayer = gameData.multiplayer.players + ((nPlayer < 0) ? N_LOCALPLAYER : nPlayer);
	int32_t bInventory = pPlayer->nInvuls && gameOpts->gameplay.bInventory && (!IsMultiGame || IsCoopGame);

if (!(bForce || bInventory))
	return 0;
if (pPlayer->flags & PLAYER_FLAGS_INVULNERABLE) {
	if (ISLOCALPLAYER (nPlayer))
		HUDInitMessage ("%s %s!", TXT_ALREADY_ARE, TXT_INVULNERABLE);
	return 0;
	}
if (gameOpts->gameplay.bInventory && (!IsMultiGame || IsCoopGame))
	pPlayer->nInvuls--;
if (ISLOCALPLAYER (nPlayer)) {
	pPlayer->invulnerableTime = gameData.timeData.xGame;
	pPlayer->flags |= PLAYER_FLAGS_INVULNERABLE;
	if (IsMultiGame)
		MultiSendInvul ();
	if (bInventory)
		PickupEffect (7, 14, 21, 0, "");
	else
		PickupEffect (7, 14, 21, INVULNERABILITY_SCORE, "%s!", TXT_INVULNERABILITY);
	gameData.multiplayer.spherePulse [N_LOCALPLAYER].Setup (0.02f, 0.5f);
	UsePowerup (-POW_INVUL);
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t ApplyCloak (int32_t bForce, int32_t nPlayer)
{
	CPlayerData *pPlayer = gameData.multiplayer.players + ((nPlayer < 0) ? N_LOCALPLAYER : nPlayer);
	int32_t bInventory = pPlayer->nCloaks && gameOpts->gameplay.bInventory && (!IsMultiGame || IsCoopGame);

if (!(bForce || bInventory))
	return 0;
if (pPlayer->flags & PLAYER_FLAGS_CLOAKED) {
	if (ISLOCALPLAYER (nPlayer))
		HUDInitMessage ("%s %s!", TXT_ALREADY_ARE, TXT_CLOAKED);
	return 0;
	}
if (gameOpts->gameplay.bInventory && (!IsMultiGame || IsCoopGame))
	pPlayer->nCloaks--;
if (ISLOCALPLAYER (nPlayer)) {
	pPlayer->cloakTime = gameData.timeData.xGame;	//	Not!changed by awareness events (like CPlayerData fires laser).
	pPlayer->flags |= PLAYER_FLAGS_CLOAKED;
	AIDoCloakStuff ();
	if (IsMultiGame)
		MultiSendCloak ();
	if (bInventory)
		PickupEffect (-10, -10, -10, 0, "");
	else
		PickupEffect (-10, -10, -10, CLOAK_SCORE, "%s!", TXT_CLOAKING_DEVICE);
	UsePowerup (-POW_CLOAK);
	}
return 1;
}

//------------------------------------------------------------------------------

static inline int32_t PowerupCount (int32_t nId)
{
if ((nId == POW_CONCUSSION_4) || 
	 (nId == POW_HOMINGMSL_4) || 
	 (nId == POW_FLASHMSL_4) || 
	 (nId == POW_GUIDEDMSL_4) || 
	 (nId == POW_MERCURYMSL_4) || 
	 (nId == POW_PROXMINE) || 
	 (nId == POW_SMARTMINE))
	return 4;
return 1;
}

//------------------------------------------------------------------------------

#if defined (_WIN32) && defined (RELEASE)
typedef int32_t (__fastcall * pPickupGun) (CObject *, int32_t, int32_t);
typedef int32_t (__fastcall * pPickupMissile) (CObject *, int32_t, int32_t, int32_t);
typedef int32_t (__fastcall * pPickupEquipment) (CObject *, int32_t);
typedef int32_t (__fastcall * pPickupKey) (CObject *, int32_t, const char *, int32_t);
typedef int32_t (__fastcall * pPickupFlag) (CObject *, int32_t, int32_t, const char *, int32_t);
#else
typedef int32_t (* pPickupGun) (CObject *, int32_t, int32_t);
typedef int32_t (* pPickupMissile) (CObject *, int32_t, int32_t, int32_t);
typedef int32_t (* pPickupEquipment) (CObject *, int32_t);
typedef int32_t (* pPickupKey) (CObject *, int32_t, const char *, int32_t);
typedef int32_t (* pPickupFlag) (CObject *, int32_t, int32_t, const char *, int32_t);
#endif


//	returns true if powerup consumed
int32_t DoPowerup (CObject *pObj, int32_t nPlayer)
{
if (OBSERVING)
	return 0;
if (gameStates.app.bGameSuspended & SUSP_POWERUPS)
	return 0;
if (pObj->Ignored (1, 1))
	return 0;

	CPlayerData*	pPlayer;
	int32_t			bPickedUp = 0;
	int32_t			bPickedUpAmmo = 0;		//for when hitting vulcan cannon gets vulcan ammo
	int32_t			bLocalPlayer;
	int32_t			nId, nType;

if (nPlayer < 0)
	nPlayer = N_LOCALPLAYER;
pPlayer = gameData.multiplayer.players + nPlayer;
if (SPECTATOR (OBJECT (pPlayer->nObject)))
	return 0;
bLocalPlayer = (nPlayer == N_LOCALPLAYER);
if (bLocalPlayer &&
	 (gameStates.app.bPlayerIsDead || 
	  (gameData.objData.pConsole->info.nType == OBJ_GHOST) || 
	  (pPlayer->Shield () < 0)))
	return 0;
if (pObj->cType.powerupInfo.xCreationTime > gameData.timeData.xGame)		//gametime wrapped!
	pObj->cType.powerupInfo.xCreationTime = 0;				//allow CPlayerData to pick up
if ((pObj->cType.powerupInfo.nFlags & PF_SPAT_BY_PLAYER) && 
	 (pObj->cType.powerupInfo.xCreationTime > 0) && 
	 (gameData.timeData.xGame < pObj->cType.powerupInfo.xCreationTime + I2X (2)))
	return 0;		//not enough time elapsed
gameData.hudData.bPlayerMessage = 0;	//	Prevent messages from going to HUD if -PlayerMessages switch is set
nId = pObj->info.nId;
if ((abs (nId) >= (int32_t) sizeofa (pickupHandler)) || !pickupHandler [nId]) // unknown/unhandled powerup type
	return 0;
nType = powerupType [nId];
if (nType == POWERUP_IS_GUN) {
	bPickedUp = ((pPickupGun) (pickupHandler [nId])) (pObj, powerupToDevice [nId], nPlayer);
	if (bPickedUp < 0) { // only true if hit a gatling gun
		bPickedUp = -bPickedUp - 1; // yields 0 if gun still has some ammo (-> keep gun), 1 otherwise (-> remove gun)
		if (!bPickedUp) { // if gun will stay, create ammo pickup effect instead
			bPickedUpAmmo = 1;
			nId = POW_VULCAN_AMMO;
			}
		}
	}
else if (nType == POWERUP_IS_MISSILE) {
	bPickedUp = ((pPickupMissile) (pickupHandler [nId])) (pObj, powerupToDevice [nId], PowerupCount (nId), nPlayer);
	}
else if (nType == POWERUP_IS_KEY) {
	int32_t nKey = nId - POW_KEY_BLUE;
	bPickedUp = ((pPickupKey) (pickupHandler [nId])) (pObj, PLAYER_FLAGS_BLUE_KEY << nKey, GAMETEXT (12 + nKey), nPlayer);
	}
else if (nType == POWERUP_IS_EQUIPMENT) {
	bPickedUp = ((pPickupEquipment) (pickupHandler [nId])) (pObj, nPlayer);
	}
else if (nType == POWERUP_IS_FLAG) {
	int32_t nFlag = nId - POW_BLUEFLAG;
	bPickedUp = ((pPickupFlag) (pickupHandler [nId])) (pObj, nFlag, !nFlag, GT (1077 + nFlag), nPlayer);
	}
else
	return 0;

//always say bPickedUp, until physics problem (getting stuck on unused powerup)
//is solved.  Note also the break statements above that are commented out
//!!	bPickedUp = 1;

if (bPickedUp || bPickedUpAmmo)
	UsePowerup (nId);
gameData.hudData.bPlayerMessage = 1;
return bPickedUp;
}

//------------------------------------------------------------------------------

int32_t SpawnPowerup (CObject *pSpitter, uint8_t nId, int32_t nCount)
{
if (gameStates.app.bGameSuspended & SUSP_POWERUPS)
	return 0;
if (!pSpitter)
	return 0;

	int32_t		i;
	int16_t		nObject;
	CFixVector	velSave;
	CObject		*pObj;

if (nCount <= 0)
	return 0;
velSave = pSpitter->mType.physInfo.velocity;
pSpitter->mType.physInfo.velocity.SetZero ();
for (i = nCount; i; i--) {
	nObject = SpitPowerup (pSpitter, nId);
	if (nObject >= 0) {
		pObj = OBJECT (nObject);
		MultiSendCreatePowerup (nId, pObj->info.nSegment, nObject, &pObj->info.position.vPos);
		}
	}
pSpitter->mType.physInfo.velocity = velSave;
return nCount;
}

//------------------------------------------------------------------------------

void SpawnLeftoverPowerups (int16_t nObject)
{
SpawnPowerup (gameData.multiplayer.leftoverPowerups [nObject].pSpitter, 
				  gameData.multiplayer.leftoverPowerups [nObject].nType,
				  gameData.multiplayer.leftoverPowerups [nObject].nCount);
memset (&gameData.multiplayer.leftoverPowerups [nObject], 0, 
		  sizeof (gameData.multiplayer.leftoverPowerups [0]));
}

//------------------------------------------------------------------------------

void CheckInventory (void)
{
	CPlayerData	*pPlayer = gameData.multiplayer.players + N_LOCALPLAYER;
	CObject		*pObj = OBJECT (pPlayer->nObject);

if (SpawnPowerup (pObj, POW_CLOAK, pPlayer->nCloaks - MAX_INV_ITEMS))
	pPlayer->nCloaks = MAX_INV_ITEMS;
if (SpawnPowerup (pObj, POW_INVUL, pPlayer->nInvuls - MAX_INV_ITEMS))
	pPlayer->nInvuls = MAX_INV_ITEMS;
}

//-----------------------------------------------------------------------------

void InitPowerupTables (void)
{
memset (powerupToDevice, 0xff, sizeof (powerupToDevice));

powerupToDevice [POW_LASER] = LASER_INDEX;
powerupToDevice [POW_VULCAN] = VULCAN_INDEX;
powerupToDevice [POW_SPREADFIRE] = SPREADFIRE_INDEX;
powerupToDevice [POW_PLASMA] = PLASMA_INDEX;
powerupToDevice [POW_FUSION] = FUSION_INDEX;
powerupToDevice [POW_GAUSS] = GAUSS_INDEX;
powerupToDevice [POW_HELIX] = HELIX_INDEX;
powerupToDevice [POW_PHOENIX] = PHOENIX_INDEX;
powerupToDevice [POW_OMEGA] = OMEGA_INDEX;
powerupToDevice [POW_SUPERLASER] = SUPER_LASER_INDEX;
powerupToDevice [POW_CONCUSSION_1] = 
powerupToDevice [POW_CONCUSSION_4] = CONCUSSION_INDEX;
powerupToDevice [POW_PROXMINE] = PROXMINE_INDEX;
powerupToDevice [POW_HOMINGMSL_1] = 
powerupToDevice [POW_HOMINGMSL_4] = HOMING_INDEX;
powerupToDevice [POW_SMARTMSL] = SMART_INDEX;
powerupToDevice [POW_MEGAMSL] = MEGA_INDEX;
powerupToDevice [POW_FLASHMSL_1] = 
powerupToDevice [POW_FLASHMSL_4] = FLASHMSL_INDEX;
powerupToDevice [POW_GUIDEDMSL_1] = 
powerupToDevice [POW_GUIDEDMSL_4] = GUIDED_INDEX;
powerupToDevice [POW_SMARTMINE] = SMARTMINE_INDEX;
powerupToDevice [POW_MERCURYMSL_1] = 
powerupToDevice [POW_MERCURYMSL_4] = MERCURY_INDEX;
powerupToDevice [POW_EARTHSHAKER] = EARTHSHAKER_INDEX;

powerupToDevice [POW_CLOAK] = PLAYER_FLAGS_CLOAKED;
powerupToDevice [POW_INVUL] = PLAYER_FLAGS_INVULNERABLE;
powerupToDevice [POW_KEY_BLUE] = PLAYER_FLAGS_BLUE_KEY;
powerupToDevice [POW_KEY_RED] = PLAYER_FLAGS_RED_KEY;
powerupToDevice [POW_KEY_GOLD] = PLAYER_FLAGS_GOLD_KEY;
powerupToDevice [POW_FULL_MAP] = PLAYER_FLAGS_FULLMAP;
powerupToDevice [POW_AMMORACK] = PLAYER_FLAGS_AMMO_RACK;
powerupToDevice [POW_CONVERTER] = PLAYER_FLAGS_CONVERTER;
powerupToDevice [POW_QUADLASER] = PLAYER_FLAGS_QUAD_LASERS;
powerupToDevice [POW_AFTERBURNER] = PLAYER_FLAGS_AFTERBURNER;
powerupToDevice [POW_HEADLIGHT] = PLAYER_FLAGS_HEADLIGHT;
powerupToDevice [POW_SLOWMOTION] = PLAYER_FLAGS_SLOWMOTION;
powerupToDevice [POW_BULLETTIME] = PLAYER_FLAGS_BULLETTIME;
powerupToDevice [POW_BLUEFLAG] = 
powerupToDevice [POW_REDFLAG] = PLAYER_FLAGS_FLAG;

powerupToDevice [POW_VULCAN_AMMO] = POW_VULCAN_AMMO;

memset (powerupToWeaponCount, 0, sizeof (powerupToWeaponCount));

powerupToWeaponCount [POW_LASER] = 
powerupToWeaponCount [POW_VULCAN] = 
powerupToWeaponCount [POW_SPREADFIRE] = 
powerupToWeaponCount [POW_PLASMA] = 
powerupToWeaponCount [POW_FUSION] = 
powerupToWeaponCount [POW_GAUSS] = 
powerupToWeaponCount [POW_HELIX] = 
powerupToWeaponCount [POW_PHOENIX] = 
powerupToWeaponCount [POW_OMEGA] = 
powerupToWeaponCount [POW_SUPERLASER] = 1;

powerupToWeaponCount [POW_CLOAK] = 
powerupToWeaponCount [POW_TURBO] = 
powerupToWeaponCount [POW_INVUL] = 
powerupToWeaponCount [POW_FULL_MAP] = 
powerupToWeaponCount [POW_CONVERTER] = 
powerupToWeaponCount [POW_AMMORACK] = 
powerupToWeaponCount [POW_AFTERBURNER] = 
powerupToWeaponCount [POW_HEADLIGHT] = 1;

powerupToWeaponCount [POW_CONCUSSION_1] = 
powerupToWeaponCount [POW_SMARTMSL] = 
powerupToWeaponCount [POW_MEGAMSL] = 
powerupToWeaponCount [POW_FLASHMSL_1] = 
powerupToWeaponCount [POW_GUIDEDMSL_1] = 
powerupToWeaponCount [POW_MERCURYMSL_1] = 
powerupToWeaponCount [POW_EARTHSHAKER] = 
powerupToWeaponCount [POW_HOMINGMSL_1] = 1;

powerupToWeaponCount [POW_CONCUSSION_4] = 
powerupToWeaponCount [POW_PROXMINE] = 
powerupToWeaponCount [POW_FLASHMSL_4] = 
powerupToWeaponCount [POW_GUIDEDMSL_4] = 
powerupToWeaponCount [POW_SMARTMINE] = 
powerupToWeaponCount [POW_MERCURYMSL_4] = 
powerupToWeaponCount [POW_HOMINGMSL_4] = 4;

powerupToWeaponCount [POW_VULCAN_AMMO] = 1;

memset (powerupClass, 0, sizeof (powerupClass));
powerupClass [POW_LASER] = 
powerupClass [POW_VULCAN] =
powerupClass [POW_SPREADFIRE] =
powerupClass [POW_PLASMA] =
powerupClass [POW_FUSION] =
powerupClass [POW_GAUSS] =
powerupClass [POW_HELIX] =
powerupClass [POW_PHOENIX] =
powerupClass [POW_OMEGA] =
powerupClass [POW_SUPERLASER] = 1;

powerupClass [POW_CONCUSSION_1] = 
powerupClass [POW_CONCUSSION_4] = 
powerupClass [POW_PROXMINE] =
powerupClass [POW_SMARTMSL] =
powerupClass [POW_MEGAMSL] =
powerupClass [POW_FLASHMSL_1] = 
powerupClass [POW_FLASHMSL_4] =
powerupClass [POW_GUIDEDMSL_1] =
powerupClass [POW_GUIDEDMSL_4] = 
powerupClass [POW_SMARTMINE] = 
powerupClass [POW_MERCURYMSL_1] = 
powerupClass [POW_MERCURYMSL_4] =
powerupClass [POW_EARTHSHAKER] =
powerupClass [POW_HOMINGMSL_1] = 
powerupClass [POW_HOMINGMSL_4] = 2;

powerupClass [POW_QUADLASER] = 
powerupClass [POW_CLOAK] = 
//powerupClass [POW_TURBO] = 
powerupClass [POW_INVUL] = 
powerupClass [POW_FULL_MAP] = 
powerupClass [POW_CONVERTER] = 
powerupClass [POW_AMMORACK] = 
powerupClass [POW_AFTERBURNER] = 
powerupClass [POW_HEADLIGHT] = 3;

powerupClass [POW_VULCAN_AMMO] = 4;

powerupClass [POW_BLUEFLAG] = 
powerupClass [POW_REDFLAG] = 5;

powerupClass [POW_KEY_BLUE] = 
powerupClass [POW_KEY_GOLD] = 
powerupClass [POW_KEY_RED] = 6;

memset (powerupToObject, 0xff, sizeof (powerupToObject));
powerupToObject [POW_LASER] = LASER_ID;
powerupToObject [POW_VULCAN] = VULCAN_ID;
powerupToObject [POW_SPREADFIRE] = SPREADFIRE_ID;
powerupToObject [POW_PLASMA] = PLASMA_ID;
powerupToObject [POW_FUSION] = FUSION_ID;
powerupToObject [POW_GAUSS] = GAUSS_ID;
powerupToObject [POW_HELIX] = HELIX_ID;
powerupToObject [POW_PHOENIX] = PHOENIX_ID;
powerupToObject [POW_OMEGA] = OMEGA_ID;
powerupToObject [POW_SUPERLASER] = SUPERLASER_ID;
powerupToObject [POW_CONCUSSION_1] = CONCUSSION_ID;
powerupToObject [POW_PROXMINE] = PROXMINE_ID;
powerupToObject [POW_SMARTMSL] = SMARTMSL_ID;
powerupToObject [POW_MEGAMSL] = MEGAMSL_ID;
powerupToObject [POW_FLASHMSL_1] = FLASHMSL_ID;
powerupToObject [POW_GUIDEDMSL_1] = GUIDEDMSL_ID;
powerupToObject [POW_SMARTMINE] = SMINEPACK_ID;
powerupToObject [POW_MERCURYMSL_1] = MERCURYMSL_ID;
powerupToObject [POW_EARTHSHAKER] = EARTHSHAKER_ID;
powerupToObject [POW_HOMINGMSL_1] = HOMINGMSL_ID;

powerupToObject [POW_CONCUSSION_4] = CONCUSSION_ID;
powerupToObject [POW_FLASHMSL_4] = FLASHMSL_ID;
powerupToObject [POW_HOMINGMSL_4] = HOMINGMSL_ID;
powerupToObject [POW_GUIDEDMSL_4] = GUIDEDMSL_ID;
powerupToObject [POW_MERCURYMSL_4] = MERCURYMSL_ID;

memset (powerupToModel, 0, sizeof (powerupToModel));
powerupToModel [POW_PROXMINE] = MAX_POLYGON_MODELS - 1;
powerupToModel [POW_SMARTMINE] = MAX_POLYGON_MODELS - 3;
powerupToModel [POW_CONCUSSION_4] = MAX_POLYGON_MODELS - 5;
powerupToModel [POW_HOMINGMSL_4] = MAX_POLYGON_MODELS - 6;
powerupToModel [POW_FLASHMSL_4] = MAX_POLYGON_MODELS - 7;
powerupToModel [POW_GUIDEDMSL_4] = MAX_POLYGON_MODELS - 8;
powerupToModel [POW_MERCURYMSL_4] = MAX_POLYGON_MODELS - 9;
powerupToModel [POW_LASER] = MAX_POLYGON_MODELS - 10;
powerupToModel [POW_VULCAN] = MAX_POLYGON_MODELS - 11;
powerupToModel [POW_SPREADFIRE] = MAX_POLYGON_MODELS - 12;
powerupToModel [POW_PLASMA] = MAX_POLYGON_MODELS - 13;
powerupToModel [POW_FUSION] = MAX_POLYGON_MODELS - 14;
powerupToModel [POW_SUPERLASER] = MAX_POLYGON_MODELS - 15;
powerupToModel [POW_GAUSS] = MAX_POLYGON_MODELS - 16;
powerupToModel [POW_HELIX] = MAX_POLYGON_MODELS - 17;
powerupToModel [POW_PHOENIX] = MAX_POLYGON_MODELS - 18;
powerupToModel [POW_OMEGA] = MAX_POLYGON_MODELS - 19;
powerupToModel [POW_QUADLASER] = MAX_POLYGON_MODELS - 20;
powerupToModel [POW_AFTERBURNER] = MAX_POLYGON_MODELS - 21;
powerupToModel [POW_HEADLIGHT] = MAX_POLYGON_MODELS - 22;
powerupToModel [POW_AMMORACK] = MAX_POLYGON_MODELS - 23;
powerupToModel [POW_CONVERTER] = MAX_POLYGON_MODELS - 24;
powerupToModel [POW_FULL_MAP] = MAX_POLYGON_MODELS - 25;
powerupToModel [POW_CLOAK] = MAX_POLYGON_MODELS - 26;
powerupToModel [POW_INVUL] = MAX_POLYGON_MODELS - 27;
powerupToModel [POW_EXTRA_LIFE] = MAX_POLYGON_MODELS - 28;
powerupToModel [POW_KEY_BLUE] = MAX_POLYGON_MODELS - 29;
powerupToModel [POW_KEY_RED] = MAX_POLYGON_MODELS - 30;
powerupToModel [POW_KEY_GOLD] = MAX_POLYGON_MODELS - 31;
powerupToModel [POW_VULCAN_AMMO] = MAX_POLYGON_MODELS - 32;
powerupToModel [POW_SLOWMOTION] = MAX_POLYGON_MODELS - 33;
powerupToModel [POW_BULLETTIME] = MAX_POLYGON_MODELS - 34;

memset (weaponToModel, 0, sizeof (weaponToModel));
weaponToModel [PROXMINE_ID] = MAX_POLYGON_MODELS - 2;
weaponToModel [SMARTMINE_ID] = MAX_POLYGON_MODELS - 4;
weaponToModel [ROBOT_SMARTMINE_ID] = MAX_POLYGON_MODELS - 4;

pickupHandler [POW_EXTRA_LIFE] = reinterpret_cast<void*> (PickupExtraLife);
pickupHandler [POW_ENERGY] = reinterpret_cast<void*> (PickupEnergyBoost);
pickupHandler [POW_SHIELD_BOOST] = reinterpret_cast<void*> (PickupShieldBoost);
pickupHandler [POW_CLOAK] = reinterpret_cast<void*> (PickupCloakingDevice);
pickupHandler [POW_TURBO] = NULL;
pickupHandler [POW_INVUL] = reinterpret_cast<void*> (PickupInvulnerability);
pickupHandler [POW_MEGAWOW] = NULL;

pickupHandler [POW_FULL_MAP] = reinterpret_cast<void*> (PickupFullMap);
pickupHandler [POW_CONVERTER] = reinterpret_cast<void*> (PickupConverter);
pickupHandler [POW_AMMORACK] = reinterpret_cast<void*> (PickupAmmoRack);
pickupHandler [POW_AFTERBURNER] = reinterpret_cast<void*> (PickupAfterburner);
pickupHandler [POW_HEADLIGHT] = reinterpret_cast<void*> (PickupHeadlight);
pickupHandler [POW_SLOWMOTION] = reinterpret_cast<void*> (PickupSlowMotion);
pickupHandler [POW_BULLETTIME] = reinterpret_cast<void*> (PickupBulletTime);
pickupHandler [POW_HOARD_ORB] = reinterpret_cast<void*> (PickupHoardOrb);
pickupHandler [POW_MONSTERBALL] = NULL;
pickupHandler [POW_VULCAN_AMMO] = reinterpret_cast<void*> (PickupVulcanAmmo);

pickupHandler [POW_BLUEFLAG] =
pickupHandler [POW_REDFLAG] = reinterpret_cast<void*> (PickupFlag);

pickupHandler [POW_KEY_BLUE] =
pickupHandler [POW_KEY_RED] =
pickupHandler [POW_KEY_GOLD] = reinterpret_cast<void*> (PickupKey);

pickupHandler [POW_LASER] = reinterpret_cast<void*> (PickupLaser);
pickupHandler [POW_SUPERLASER] = reinterpret_cast<void*> (PickupSuperLaser);
pickupHandler [POW_QUADLASER] = reinterpret_cast<void*> (PickupQuadLaser);
pickupHandler [POW_SPREADFIRE] = 
pickupHandler [POW_PLASMA] = 
pickupHandler [POW_FUSION] = 
pickupHandler [POW_HELIX] = 
pickupHandler [POW_PHOENIX] = 
pickupHandler [POW_OMEGA] = reinterpret_cast<void*> (PickupGun);
pickupHandler [POW_VULCAN] = 
pickupHandler [POW_GAUSS] = reinterpret_cast<void*> (PickupGatlingGun);

pickupHandler [POW_PROXMINE] =
pickupHandler [POW_SMARTMINE] =
pickupHandler [POW_CONCUSSION_1] =
pickupHandler [POW_CONCUSSION_4] =
pickupHandler [POW_HOMINGMSL_1] =
pickupHandler [POW_HOMINGMSL_4] =
pickupHandler [POW_SMARTMSL] =
pickupHandler [POW_MEGAMSL] =
pickupHandler [POW_FLASHMSL_1] =
pickupHandler [POW_FLASHMSL_4] =     
pickupHandler [POW_GUIDEDMSL_1] =
pickupHandler [POW_GUIDEDMSL_4] =
pickupHandler [POW_MERCURYMSL_1] =
pickupHandler [POW_MERCURYMSL_4] =
pickupHandler [POW_EARTHSHAKER] = reinterpret_cast<void*> (PickupSecondary);

powerupType [POW_TURBO] = 
powerupType [POW_MEGAWOW] = 
powerupType [POW_MONSTERBALL] = (uint8_t) POWERUP_IS_UNDEFINED;

powerupType [POW_EXTRA_LIFE] = 
powerupType [POW_ENERGY] = 
powerupType [POW_SHIELD_BOOST] = 
powerupType [POW_CLOAK] = 
powerupType [POW_HOARD_ORB] =      
powerupType [POW_INVUL] = 
powerupType [POW_FULL_MAP] = 
powerupType [POW_CONVERTER] = 
powerupType [POW_AMMORACK] = 
powerupType [POW_AFTERBURNER] = 
powerupType [POW_HEADLIGHT] = 
powerupType [POW_SLOWMOTION] =
powerupType [POW_BULLETTIME] =
powerupType [POW_VULCAN_AMMO] = (uint8_t) POWERUP_IS_EQUIPMENT;

powerupType [POW_BLUEFLAG] =
powerupType [POW_REDFLAG] = (uint8_t) POWERUP_IS_FLAG;

powerupType [POW_KEY_BLUE] =
powerupType [POW_KEY_RED] =
powerupType [POW_KEY_GOLD] = (uint8_t) POWERUP_IS_KEY;

powerupType [POW_LASER] = 
powerupType [POW_SPREADFIRE] = 
powerupType [POW_PLASMA] = 
powerupType [POW_FUSION] = 
powerupType [POW_SUPERLASER] = 
powerupType [POW_HELIX] = 
powerupType [POW_PHOENIX] = 
powerupType [POW_OMEGA] = 
powerupType [POW_VULCAN] = 
powerupType [POW_GAUSS] = 
powerupType [POW_QUADLASER] = (uint8_t) POWERUP_IS_GUN;

powerupType [POW_CONCUSSION_1] = 
powerupType [POW_CONCUSSION_4] = 
powerupType [POW_PROXMINE] = 
powerupType [POW_HOMINGMSL_1] = 
powerupType [POW_HOMINGMSL_4] = 
powerupType [POW_SMARTMSL] = 
powerupType [POW_MEGAMSL] = 
powerupType [POW_FLASHMSL_1] = 
powerupType [POW_FLASHMSL_4] =   
powerupType [POW_GUIDEDMSL_1] = 
powerupType [POW_GUIDEDMSL_4] = 
powerupType [POW_SMARTMINE] = 
powerupType [POW_MERCURYMSL_1] = 
powerupType [POW_MERCURYMSL_4] = 
powerupType [POW_EARTHSHAKER] = (uint8_t) POWERUP_IS_MISSILE;
}

//-----------------------------------------------------------------------------

#define ENABLE_FILTER(_type,_flag) if (_flag) powerupFilter [_type] = 1;

void SetupPowerupFilter (tNetGameInfo *pInfo)
{
if (!pInfo)
	pInfo = &netGameInfo.m_info;
memset (powerupFilter, 0, sizeof (powerupFilter));
ENABLE_FILTER (POW_INVUL, pInfo->DoInvulnerability);
ENABLE_FILTER (POW_CLOAK, pInfo->DoCloak);
ENABLE_FILTER (POW_KEY_BLUE, IsCoopGame);
ENABLE_FILTER (POW_KEY_RED, IsCoopGame);
ENABLE_FILTER (POW_KEY_GOLD, IsCoopGame);
ENABLE_FILTER (POW_AFTERBURNER, pInfo->DoAfterburner);
ENABLE_FILTER (POW_FUSION, pInfo->DoFusions);
ENABLE_FILTER (POW_PHOENIX, pInfo->DoPhoenix);
ENABLE_FILTER (POW_HELIX, pInfo->DoHelix);
ENABLE_FILTER (POW_MEGAMSL, pInfo->DoMegas);
ENABLE_FILTER (POW_SMARTMSL, pInfo->DoSmarts);
ENABLE_FILTER (POW_GAUSS, pInfo->DoGauss);
ENABLE_FILTER (POW_VULCAN, pInfo->DoVulcan);
ENABLE_FILTER (POW_PLASMA, pInfo->DoPlasma);
ENABLE_FILTER (POW_OMEGA, pInfo->DoOmega);
ENABLE_FILTER (POW_LASER, 1);
ENABLE_FILTER (POW_SUPERLASER, pInfo->DoSuperLaser);
ENABLE_FILTER (POW_MONSTERBALL, (gameData.appData.GameMode (GM_HOARD | GM_MONSTERBALL)));
ENABLE_FILTER (POW_PROXMINE, pInfo->DoProximity || (gameData.appData.GameMode (GM_HOARD | GM_ENTROPY)));
ENABLE_FILTER (POW_SMARTMINE, pInfo->DoSmartMine || IsEntropyGame);
ENABLE_FILTER (POW_VULCAN_AMMO, (pInfo->DoVulcan || pInfo->DoGauss));
ENABLE_FILTER (POW_SPREADFIRE, pInfo->DoSpread);
ENABLE_FILTER (POW_FLASHMSL_1, pInfo->DoFlash);
ENABLE_FILTER (POW_FLASHMSL_4, pInfo->DoFlash);
ENABLE_FILTER (POW_GUIDEDMSL_1, pInfo->DoGuided);
ENABLE_FILTER (POW_GUIDEDMSL_4, pInfo->DoGuided);
ENABLE_FILTER (POW_EARTHSHAKER, pInfo->DoEarthShaker);
ENABLE_FILTER (POW_MERCURYMSL_1, pInfo->DoMercury);
ENABLE_FILTER (POW_MERCURYMSL_4, pInfo->DoMercury);
ENABLE_FILTER (POW_CONVERTER, pInfo->DoConverter);
ENABLE_FILTER (POW_AMMORACK, pInfo->DoAmmoRack);
ENABLE_FILTER (POW_HEADLIGHT, pInfo->DoHeadlight && (!EGI_FLAG (bDarkness, 0, 0, 0) || EGI_FLAG (headlight.bAvailable, 0, 1, 0)));
ENABLE_FILTER (POW_LASER, pInfo->DoLaserUpgrade);
ENABLE_FILTER (POW_CONCUSSION_1, 1);
ENABLE_FILTER (POW_CONCUSSION_4, 1);
ENABLE_FILTER (POW_HOMINGMSL_1, pInfo->DoHoming);
ENABLE_FILTER (POW_HOMINGMSL_4, pInfo->DoHoming);
ENABLE_FILTER (POW_QUADLASER, pInfo->DoQuadLasers);
ENABLE_FILTER (POW_BLUEFLAG, (gameData.appData.GameMode (GM_CAPTURE)));
ENABLE_FILTER (POW_REDFLAG, (gameData.appData.GameMode (GM_CAPTURE)));
}

//-----------------------------------------------------------------------------

int32_t PowerupToDevice (int16_t nPowerup, int32_t *pnType)
{
*pnType = powerupClass [nPowerup];
return powerupToDevice [nPowerup];
}

//-----------------------------------------------------------------------------

char PowerupClass (int16_t nPowerup)
{
return powerupClass [nPowerup];
}

//-----------------------------------------------------------------------------

char PowerupToWeaponCount (int16_t nPowerup)
{
return powerupToWeaponCount [nPowerup];
}

//-----------------------------------------------------------------------------

char PowerupToObject (int16_t nPowerup)
{
return powerupToObject [nPowerup];
}

//-----------------------------------------------------------------------------

int16_t PowerupToModel (int16_t nPowerup)
{
return powerupToModel [nPowerup];
}

//-----------------------------------------------------------------------------

int16_t WeaponToModel (int16_t nWeapon)
{
return weaponToModel [nWeapon];
}

//-----------------------------------------------------------------------------
// powerup classes:
// 1 - guns
// 2 - missiles
// 3 - equipment
// 4 - vulcan ammo
// 5 - flags
// 6 - keys

int16_t PowerupsOnShips (int32_t nPowerup)
{
	CPlayerData	*pPlayer = gameData.multiplayer.players;
	int32_t		nClass, nVulcanAmmo = 0;
	int16_t		nPowerups = 0, nIndex = PowerupToDevice (nPowerup, &nClass);

if (!nClass || ((nClass < 3) && (nIndex < 0)))
	return 0;
for (int16_t i = 0; i < N_PLAYERS; i++, pPlayer++) {
	//if ((i == N_LOCALPLAYER) && (LOCALPLAYER.m_bExploded || gameStates.app.bPlayerIsDead))
	//	continue;
	if (pPlayer->Shield () < 0)
		continue; 
#if 0 //DBG
	if (!pPlayer->connected && (gameStates.app.nSDLTicks [0] - pPlayer->m_tDisconnect > 600))
#else
	if (!pPlayer->connected && (gameStates.app.nSDLTicks [0] - pPlayer->m_tDisconnect > TIMEOUT_KICK))
#endif
		continue; // wait up to three minutes for a player to reconnect before dropping him and allowing to respawn his stuff
	if (nClass == 5) {
		if ((PLAYER (i).flags & PLAYER_FLAGS_FLAG) && ((nPowerup == POW_REDFLAG) != (GetTeam (i) == TEAM_RED)))
			nPowerups++;
		}
	if (nClass == 4) 
		nVulcanAmmo += pPlayer->primaryAmmo [VULCAN_INDEX] + gameData.multiplayer.weaponStates [i].nAmmoUsed % VULCAN_CLIP_CAPACITY;
	else if (nClass == 3) {	// some device
		if (!(extraGameInfo [IsMultiGame].loadout.nDevice & nIndex))
			nPowerups += (pPlayer->flags & nIndex) != 0;
		}
	else if (nClass == 2) {	// missiles
		nPowerups += gameData.multiplayer.SecondaryAmmo (i, nIndex, 0);
		}
	else {	// guns
		if (!IsBuiltinWeapon (nIndex)) {
			if (nIndex == LASER_INDEX)
				nPowerups += pPlayer->LaserLevel (0);
			else if (nIndex == SUPER_LASER_INDEX)
				nPowerups += pPlayer->LaserLevel (1);
			else if (pPlayer->primaryWeaponFlags & (1 << nIndex)) {
				nPowerups++;
				if ((nIndex == FUSION_INDEX) && gameData.multiplayer.weaponStates [i].bTripleFusion)
					nPowerups++;
				}
			}
		}
	}
return (nClass == 4) ? (nVulcanAmmo + VULCAN_CLIP_CAPACITY - 1) / VULCAN_CLIP_CAPACITY : nPowerups;
} 

//------------------------------------------------------------------------------
/*
 * reads n tPowerupTypeInfo structs from a CFile
 */
extern int32_t ReadPowerupTypeInfos (tPowerupTypeInfo *pti, int32_t n, CFile& cf)
{
for (int32_t i = 0; i < n; i++) {
	pti [i].nClipIndex = cf.ReadInt ();
	pti [i].hitSound = cf.ReadInt ();
	pti [i].size = cf.ReadFix ();
	pti [i].light = cf.ReadFix ();
	}
return n;
}

//------------------------------------------------------------------------------
//eof

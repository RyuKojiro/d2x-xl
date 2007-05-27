/* $Id: joydefs.c,v 1.7 2003/10/08 19:18:46 btb Exp $ */
/*
 *
 * SDL joystick support
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif
#include <string.h>

#include "pstypes.h"
#include "inferno.h"
#include "menu.h"
#include "newmenu.h"
#include "config.h"
#include "text.h"
#include "key.h"
#include "kconfig.h"
#include "network.h"
#include "joy.h"
#include "joydefs.h"

//------------------------------------------------------------------------------


int	nJoyDeadzoneOpt, nRampOpt, nRampKeyOpt, nJoySensOpt, nMouseSensOpt, aboveCustOpt, 
		kbdCustOpt, nLinSensOpt, nMouseLookOpt, nSyncJoyAxesOpt, nSyncMouseAxesOpt,
		nUseMouseOpt, nUseJoyOpt, nUseHotKeysOpt, nJoyMouseOpt, nMouseDZoneOpt,
		nUseTrackIROpt, nTrackIRDZoneOpt, nTrackIRSensOpt, nTrackIRMoveOpt, nSyncTrackIRAxesOpt;

int nJoyDeadzones [] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 30, 40, 50};

char szAxis3D [3] = {'X', 'Y', 'R'};

char axis3DHotkeys [3] = {KEY_X, KEY_Y, KEY_R};

char szJoyAxis [UNIQUE_JOY_AXES] = {'X', 'Y', 'R', 'T', 'Z'};

char joyHotkeys [UNIQUE_JOY_AXES] = {KEY_X, KEY_Y, KEY_R, KEY_T, KEY_Z};

int joydefs_calibrateFlag = 0;

//------------------------------------------------------------------------------

void joydefs_calibrate()
{
	joydefs_calibrateFlag = 0;

	if (!gameStates.input.nJoysticks) {
		ExecMessageBox( NULL, NULL, 1, TXT_OK, TXT_NO_JOYSTICK );
		return;
	}
	//Actual calibration if necessary
}

//------------------------------------------------------------------------------

static char *szDZoneSizes [5];

void MouseConfigCallback (int nitems, tMenuItem * items, int *key, int citem)
{
	int h, i, v;
	int ocType = gameConfig.nControlType;
	tMenuItem * m;

SetControlType ();
if ((ocType != gameConfig.nControlType) && (gameConfig.nControlType == CONTROL_THRUSTMASTER_FCS)) {
	ExecMessageBox (TXT_IMPORTANT_NOTE, NULL, 1, TXT_OK, TXT_FCS);
	}
if (ocType != gameConfig.nControlType) {
	switch (gameConfig.nControlType) {
//		case	CONTROL_NONE:
		case	CONTROL_JOYSTICK:
		case	CONTROL_FLIGHTSTICK_PRO:
		case	CONTROL_THRUSTMASTER_FCS:
		case	CONTROL_GRAVIS_GAMEPAD:
//		case	CONTROL_MOUSE:
//		case	CONTROL_CYBERMAN:
			joydefs_calibrateFlag = 1;
		}
	KCSetControls(0);
	}

m = items + nUseMouseOpt;
v = m->value;
if (gameOpts->input.mouse.bUse != v) {
	if (gameStates.app.bNostalgia) {
		gameOpts->input.mouse.bUse = v;
		gameOpts->input.joystick.bUse = gameStates.input.nJoysticks && !v;
		m->redraw = 1;
		}
	else {
		m->rebuild = 1;
		*key = -2;
		}
	return;
	}

if (gameStates.app.bNostalgia)
	return;

if (gameOpts->input.mouse.bUse) {
	if (gameOpts->app.bExpertMode) {
		m = items + nSyncMouseAxesOpt;
		v = m->value;
		if (gameOpts->input.mouse.bSyncAxes != v) {
			gameOpts->input.mouse.bSyncAxes = v;
			if (gameOpts->input.mouse.bSyncAxes)
				for (i = 1; i < 3; i++)
					gameOpts->input.mouse.sensitivity [i] = gameOpts->input.mouse.sensitivity [0];
			m->rebuild = 1;
			*key = -2;
			return;
			}
		m = items + nJoyMouseOpt;
		v = m->value;
		if (gameOpts->input.mouse.bJoystick != v) {
			gameOpts->input.mouse.bJoystick = v;
			*key = -2;
			}
		}
	h = gameOpts->input.mouse.bSyncAxes ? 1 : 3;
	for (i = 0; i < h; i++)
		gameOpts->input.mouse.sensitivity [i] = items [nMouseSensOpt + i].value;
	for (i = h; i < 3; i++)
		gameOpts->input.mouse.sensitivity [i] = gameOpts->input.mouse.sensitivity [0];
	if (gameOpts->input.mouse.bJoystick && (nMouseDZoneOpt >= 0)) {
		m = items + nMouseDZoneOpt;
		v = m->value;
		if (gameOpts->input.mouse.nDeadzone != v) {
			gameOpts->input.mouse.nDeadzone = v;
			sprintf (m->text, TXT_MOUSE_DEADZONE, szDZoneSizes [v]);
			m->rebuild = 1;
			}
		}
	}
}

//------------------------------------------------------------------------------

void JoystickConfigCallback (int nitems, tMenuItem * items, int *key, int citem)
{
	int h, i, v;
	int ocType = gameConfig.nControlType;
	tMenuItem * m;

SetControlType ();
if ((ocType != gameConfig.nControlType) && (gameConfig.nControlType == CONTROL_THRUSTMASTER_FCS)) {
	ExecMessageBox (TXT_IMPORTANT_NOTE, NULL, 1, TXT_OK, TXT_FCS);
	}

if (ocType != gameConfig.nControlType) {
	switch (gameConfig.nControlType) {
//		case	CONTROL_NONE:
		case	CONTROL_JOYSTICK:
		case	CONTROL_FLIGHTSTICK_PRO:
		case	CONTROL_THRUSTMASTER_FCS:
		case	CONTROL_GRAVIS_GAMEPAD:
//		case	CONTROL_MOUSE:
//		case	CONTROL_CYBERMAN:
			joydefs_calibrateFlag = 1;
		}
	KCSetControls(0);
	}
m = items + nUseJoyOpt;
v = m->value;
if (gameOpts->input.joystick.bUse != v) {
	if (gameStates.app.bNostalgia) {
		gameOpts->input.joystick.bUse = v;
		gameOpts->input.mouse.bUse = !v;
		m->redraw = 1;
		}
	else {
		m->rebuild = 1;
		*key = -2;
		}
	return;
	}

if (gameStates.app.bNostalgia)
	return;


if (gameStates.input.nJoysticks && gameOpts->input.joystick.bUse) {
	if (gameOpts->app.bExpertMode) {
		m = items + nSyncJoyAxesOpt;
		v = m->value;
		if (gameOpts->input.joystick.bSyncAxes != v) {
			gameOpts->input.joystick.bSyncAxes = v;
			if (gameOpts->input.joystick.bSyncAxes)
				for (i = 1; i < UNIQUE_JOY_AXES; i++) {
					gameOpts->input.joystick.deadzones [i] = gameOpts->input.joystick.deadzones [0];
					gameOpts->input.joystick.sensitivity [i] = gameOpts->input.joystick.sensitivity [0];
					}
			m->rebuild = 1;
			*key = -2;
			return;
			}
		}
	h = gameOpts->input.joystick.bSyncAxes ? 1 : UNIQUE_JOY_AXES;
	for (i = 0; i < h; i++)
		gameOpts->input.joystick.sensitivity [i] = items [nJoySensOpt + i].value;
	for (i = h; i < UNIQUE_JOY_AXES; i++)
		gameOpts->input.joystick.sensitivity [i] = gameOpts->input.joystick.sensitivity [0];

	for (i = 0; i < h; i++) {
		m = items + nJoyDeadzoneOpt + i;
		v = nJoyDeadzones [m->value];
		if (gameOpts->input.joystick.deadzones [i] != v) {
			gameOpts->input.joystick.deadzones [i] = v;
			JoySetDeadzone (gameOpts->input.joystick.deadzones [i], i);
			if (gameOpts->input.joystick.bSyncAxes)
				sprintf (m->text, TXT_JOY_DEADZONE, gameOpts->input.joystick.deadzones [i]);
			else
				sprintf (m->text, TXT_JOY_DEADZONE_N, szJoyAxis [i], gameOpts->input.joystick.deadzones [i]);
			m->rebuild = 1;
			}
		}
	for (i = h; i < UNIQUE_JOY_AXES; i++)
		gameOpts->input.joystick.deadzones [i] = gameOpts->input.joystick.deadzones [0];
	}
}

//------------------------------------------------------------------------------

static char *szDZoneSizes [5];

void TrackIRConfigCallback (int nitems, tMenuItem * items, int *key, int citem)
{
	int h, i, v;
	tMenuItem * m;

m = items + nUseTrackIROpt;
v = m->value;
if (gameOpts->input.trackIR.bUse != v) {
	gameOpts->input.trackIR.bUse = v;
	*key = -2;
	return;
	}
if (gameOpts->input.trackIR.bUse) {
	for (i = 0; i < 3; i++) {
		if (gameOpts->app.bExpertMode) {
			m = items + nSyncTrackIRAxesOpt;
			v = m->value;
			if (gameOpts->input.trackIR.bSyncAxes != v) {
				gameOpts->input.trackIR.bSyncAxes = v;
				if (gameOpts->input.trackIR.bSyncAxes)
					for (i = 1; i < 3; i++)
						gameOpts->input.trackIR.sensitivity [i] = gameOpts->input.trackIR.sensitivity [0];
				m->rebuild = 1;
				*key = -2;
				return;
				}
			}
		m = items + nTrackIRMoveOpt + i;
		v = m->value;
		if (gameOpts->input.trackIR.bMove [i] != v) {
			gameOpts->input.trackIR.bMove [i] = v;
			for (h = i = 0; i < 3; i++)
				h += gameOpts->input.trackIR.bMove [i];
			if (h < 2) {
				*key = -2;
				return;
				}
			}
		}
	m = items + nTrackIRDZoneOpt;
	v = m->value;
	if (gameOpts->input.trackIR.nDeadzone != v) {
		gameOpts->input.trackIR.nDeadzone = v;
		sprintf (m->text, TXT_TRACKIR_DEADZONE, szDZoneSizes [v]);
		m->rebuild = 1;
		}
	h = gameOpts->input.trackIR.bSyncAxes ? 1 : 3;
	for (i = 0; i < h; i++)
		gameOpts->input.trackIR.sensitivity [i] = items [nTrackIRSensOpt + i].value;
	for (i = h; i < 3; i++)
		gameOpts->input.trackIR.sensitivity [i] = gameOpts->input.trackIR.sensitivity [0];
	}
}

//------------------------------------------------------------------------------

static char *szDZoneSizes [5];

void KeyboardConfigCallback (int nitems, tMenuItem * items, int *key, int citem)
{
	int			i, v;
	tMenuItem	*m;

if (!gameStates.app.bNostalgia) { 
	m = items + nUseHotKeysOpt;
	v = m->value;
	if (gameOpts->input.bUseHotKeys != v) {
		gameOpts->input.bUseHotKeys = v;
		*key = -2;
		return;
		}
	}

if (gameOpts->app.bExpertMode) {
	m = items + nRampOpt;
	v = m->value * 10;
	if (gameOpts->input.keyboard.nRamp != v) {
		if (!(gameOpts->input.keyboard.nRamp && v))
			*key = -2;
		gameOpts->input.keyboard.nRamp = v;
		sprintf(m->text, TXT_KBD_RAMP, gameOpts->input.keyboard.nRamp);
		m->rebuild = 1;
		}

	for (i = 0; i < 3; i++) {
		m = items + nRampKeyOpt + i;
		v = (m->value != 0);
		if (gameOpts->input.keyboard.bRamp [i] != v) {
			gameOpts->input.keyboard.bRamp [i] = v;
			m->redraw = 1;
			}
		}
	}
}

//------------------------------------------------------------------------------

int AddDeadzoneControl (tMenuItem *m, char *szText, char *szFmt, char *szHelp, ubyte nValue, char nKey, int *pnOpt)
{
sprintf (szText + 1, szFmt, szDZoneSizes [nValue]);
*szText = *(szFmt - 1);
ADD_SLIDER (*pnOpt, szText + 1, nValue, 0, 4, nKey, szHelp);
return (*pnOpt)++;
}

//------------------------------------------------------------------------------

int AddAxisControls (tMenuItem *m, char *szText, char *szFmtSyncd, char *szFmt, char *szLabel, char *szHelp, 
							int nControls, int *pnValues, int nValues, int *pnIntervals, 
							char nKeySyncd, char *pnKeys, int bSyncControls, int *pnOpt)
{
	int	h, i, j, v, opt = *pnOpt; 

j = bSyncControls ? 1 : nControls;
for (i = 0; i < j; i++) {
	if (pnIntervals) {
		for (v = nValues - 1; v >= 0; v--)
			if (pnValues [i] >= pnIntervals [v])
				break;
		}
	else
		v = pnValues [i];
	if (bSyncControls) {
		if (pnIntervals)
			sprintf (szText + i * 50 + 1, szFmtSyncd, pnValues [i]);
		else
			strcpy (szText + i * 50 + 1, szFmtSyncd);
		}
	else {
		if (pnIntervals)
			sprintf (szText + i * 50 + 1, szFmt, szLabel [i], pnValues [i]);
		else
			sprintf (szText + i * 50 + 1, szFmt, szLabel [i]);
		}
	szText [i * 50] = *(szFmtSyncd - 1);
	ADD_SLIDER (opt, szText + i * 50 + 1, v, 0, nValues - 1, bSyncControls ? nKeySyncd : pnKeys [i], szHelp);
	if (!i)
		h = opt;
	opt++;
	}
*pnOpt = opt;
return h;
}

//------------------------------------------------------------------------------

void MouseConfigMenu (void)
{
	tMenuItem m [20];
	int	i, j, opt = 0, choice = 0;
	int	nCustMouseOpt, nMouseTypeOpt;
	char	szMouseSens [3][50];
	char	szMouseDeadzone [50];

szDZoneSizes [0] = TXT_NONE;
szDZoneSizes [1] = TXT_SMALL;
szDZoneSizes [2] = TXT_MEDIUM;
szDZoneSizes [3] = TXT_LARGE;
szDZoneSizes [4] = TXT_VERY_LARGE;

do {
	do {
		memset (&m, 0, sizeof (m));
		opt = 0;

		nCustMouseOpt = nMouseTypeOpt = -1;
		SetControlType ();
		ADD_CHECK (opt, TXT_USE_MOUSE, gameOpts->input.mouse.bUse, KEY_M, HTX_CONF_USEMOUSE);
		nUseMouseOpt = opt++;
		nMouseDZoneOpt = -1;
		if (gameOpts->input.mouse.bUse || gameStates.app.bNostalgia) {
			if (gameOpts->input.mouse.bUse) {
				ADD_MENU (opt, TXT_CUST_MOUSE, KEY_O, HTX_CONF_CUSTMOUSE);
				nCustMouseOpt = opt++;
				}
			ADD_TEXT (opt, "", 0);
			opt++;
			if (gameStates.app.bNostalgia) {
				gameOpts->input.mouse.bSyncAxes = 1;
				}
			else {
				ADD_CHECK (opt, TXT_SYNC_MOUSE_AXES, gameOpts->input.mouse.bSyncAxes, KEY_A, HTX_CONF_SYNCMOUSE);
				nSyncMouseAxesOpt = opt++;
				}
			nMouseSensOpt = AddAxisControls (m, &szMouseSens [0][0], TXT_MOUSE_SENS, TXT_MOUSE_SENS_N, szAxis3D, HTX_CONF_MOUSESENS, 
														3, gameOpts->input.mouse.sensitivity, 16, NULL, KEY_O, axis3DHotkeys, gameOpts->input.mouse.bSyncAxes, &opt);
			if (gameOpts->input.mouse.bUse && !gameStates.app.bNostalgia) {
				ADD_TEXT (opt, "", 0);
				opt++;
				ADD_CHECK (opt, TXT_MOUSELOOK, extraGameInfo [0].bMouseLook, KEY_L, HTX_CONF_MOUSELOOK);
				nMouseLookOpt = opt++;
				ADD_CHECK (opt, TXT_JOYMOUSE, gameOpts->input.mouse.bJoystick, KEY_J, HTX_CONF_JOYMOUSE);
				nJoyMouseOpt = opt++;
				if (gameOpts->input.mouse.bJoystick && gameOpts->app.bExpertMode)
					nMouseDZoneOpt = AddDeadzoneControl (m, szMouseDeadzone, TXT_MOUSE_DEADZONE, HTX_MOUSE_DEADZONE, gameOpts->input.mouse.nDeadzone, KEY_U, &opt);
				ADD_TEXT (opt, "", 0);
				opt++;
				nMouseTypeOpt = opt;
				ADD_RADIO (opt, TXT_STD_MOUSE, 0, 0, 1, HTX_CONF_STDMOUSE);
				opt++;
				ADD_RADIO (opt, TXT_CYBERMAN, 0, 0, 1, HTX_CONF_CYBERMAN);
				opt++;
				m [nMouseTypeOpt + NMCLAMP (gameStates.input.nMouseType - CONTROL_MOUSE, 0, 1)].value = 1;
				}
			}
		i = ExecMenu1 (NULL, TXT_MOUSE_CONFIG, opt, m, MouseConfigCallback, &choice);
		gameOpts->input.mouse.bUse = m [nUseMouseOpt].value;
		if (gameOpts->input.mouse.bUse && !gameStates.app.bNostalgia) {
			if (nMouseTypeOpt < 0) {
				gameStates.input.nMouseType = CONTROL_MOUSE;
				}
			else {
				extraGameInfo [0].bMouseLook = m [nMouseLookOpt].value;
				for (j = 0; j < 2; j++) {
					if (m [nMouseTypeOpt + j].value) {
						gameStates.input.nMouseType = CONTROL_MOUSE + j;
						break;
						}
					}
				}
			}
		} while (i == -2);
		if (i == -1)
			return;
		gameConfig.nControlType = gameOpts->input.mouse.bUse ? CONTROL_MOUSE : gameConfig.nControlType;
		if (choice == nCustMouseOpt)
			KConfig (2, TXT_CFG_MOUSE);
	} while (i >= 0);
}

//------------------------------------------------------------------------------

void JoystickConfigMenu (void)
{
	tMenuItem m [20];
	int	h, i, j, opt = 0, choice = 0;
	int	nCustJoyOpt, nJoyTypeOpt;
	char	szJoySens [UNIQUE_JOY_AXES][50];
	char	szJoyDeadzone [UNIQUE_JOY_AXES][50];

do {
	do {
		memset (&m, 0, sizeof (m));
		opt = 0;

		nCustJoyOpt = nJoyTypeOpt = -1;
		SetControlType ();
		ADD_CHECK (opt, TXT_USE_JOY, gameOpts->input.joystick.bUse, KEY_J, HTX_CONF_USEJOY);
		nUseJoyOpt = opt++;
		if (gameOpts->input.joystick.bUse || gameStates.app.bNostalgia) {
			ADD_MENU (opt, TXT_CUST_JOY, KEY_C, HTX_CONF_CUSTJOY);
			nCustJoyOpt = opt++;
			if (gameStates.app.bNostalgia || !gameOpts->app.bExpertMode)
				gameOpts->input.joystick.bSyncAxes = 1;
			else {
				ADD_CHECK (opt, TXT_JOY_LINSCALE, gameOpts->input.joystick.bLinearSens, KEY_I, HTX_CONF_LINJOY);
				nLinSensOpt = opt++;
				ADD_CHECK (opt, TXT_SYNC_JOY_AXES, gameOpts->input.joystick.bSyncAxes, KEY_Y, HTX_CONF_SYNCJOY);
				nSyncJoyAxesOpt = opt++;
				}
			h = gameOpts->input.joystick.bSyncAxes ? 1 : UNIQUE_JOY_AXES;
			nJoySensOpt = AddAxisControls (m, &szJoySens [0][0], TXT_JOY_SENS, TXT_JOY_SENS_N, szJoyAxis, HTX_CONF_JOYSENS, 
												    UNIQUE_JOY_AXES, gameOpts->input.joystick.sensitivity, 16, NULL, KEY_S, joyHotkeys, gameOpts->input.joystick.bSyncAxes, &opt);
			if (!gameStates.app.bNostalgia) {
				nJoyDeadzoneOpt = AddAxisControls (m, &szJoyDeadzone [0][0], TXT_JOY_DEADZONE, TXT_JOY_DEADZONE_N, szJoyAxis, HTX_CONF_JOYDZONE, 
															  UNIQUE_JOY_AXES, gameOpts->input.joystick.deadzones, 16, nJoyDeadzones, KEY_S, joyHotkeys, gameOpts->input.joystick.bSyncAxes, &opt);
				ADD_TEXT (opt, "", 0);
				opt++;
				nJoyTypeOpt = opt;
				ADD_RADIO (opt, TXT_STD_JOY, 0, 0, 2, HTX_CONF_STDJOY);
				opt++;
				ADD_RADIO (opt, TXT_FSPRO_JOY, 0, 0, 2, HTX_CONF_FSPRO);
				opt++;
				ADD_RADIO (opt, TXT_FCS_JOY, 0, 0, 2, HTX_CONF_FCS);
				opt++;
				ADD_RADIO (opt, TXT_GRAVIS_JOY, 0, 0, 2, HTX_CONF_GRAVIS);
				opt++;
				m [nJoyTypeOpt + NMCLAMP (gameStates.input.nJoyType - CONTROL_JOYSTICK, 0, 3)].value = 1;
				}
			}
		i = ExecMenu1 (NULL, TXT_JOYSTICK_CONFIG, opt, m, JoystickConfigCallback, &choice);
		gameOpts->input.joystick.bUse = m [nUseJoyOpt].value;
		if (gameOpts->input.joystick.bUse && !gameStates.app.bNostalgia) {
			if (nJoyTypeOpt < 0)
				gameStates.input.nJoyType = CONTROL_JOYSTICK;
			else {
				if (gameOpts->app.bExpertMode) {
					gameOpts->input.joystick.bLinearSens = m [nLinSensOpt].value;
				for (j = 0; j < UNIQUE_JOY_AXES; j++)
					if (m [nJoyTypeOpt + j].value) {
						gameStates.input.nJoyType = CONTROL_JOYSTICK + j;
						break;
						}
					}
				}
			}
		} while (i == -2);
	if (i == -1)
		return;
	if (choice == nCustJoyOpt)
		KConfig (1, TXT_CFG_JOY);
	} while (i >= 0);
}

//------------------------------------------------------------------------------

void TrackIRConfigMenu (void)
{
	tMenuItem m [20];
	int	i, opt = 0, choice = 0;
	char	szTrackIRSens [3][50];
	char	szTrackIRDeadzone [50];

szDZoneSizes [0] = TXT_NONE;
szDZoneSizes [1] = TXT_SMALL;
szDZoneSizes [2] = TXT_MEDIUM;
szDZoneSizes [3] = TXT_LARGE;
szDZoneSizes [4] = TXT_VERY_LARGE;

do {
	memset (&m, 0, sizeof (m));
	opt = 0;
	ADD_CHECK (opt, TXT_USE_TRACKIR, gameOpts->input.trackIR.bUse, KEY_M, HTX_USE_TRACKIR);
	nUseTrackIROpt = opt++;
	if (gameOpts->input.trackIR.bUse) {
		nTrackIRMoveOpt = opt;
		for (i = 0; i < 3; i++) {
			ADD_CHECK (opt, GT (924 + i), gameOpts->input.trackIR.bMove [i], (i == 2) ? KEY_T : (i == 1) ? KEY_U : KEY_S, HT (281 + i));
			opt++;
			}
		if (gameOpts->input.trackIR.bMove [0] || gameOpts->input.trackIR.bMove [1] || gameOpts->input.trackIR.bMove [2]) {
			ADD_CHECK (opt, TXT_SYNC_TRACKIR_AXES, gameOpts->input.trackIR.bSyncAxes, KEY_K, HTX_SYNC_TRACKIR_AXES);
			nSyncTrackIRAxesOpt = opt++;
			nTrackIRSensOpt = AddAxisControls (m, &szTrackIRSens [0][0], TXT_TRACKIR_SENS, TXT_TRACKIR_SENS_N, szAxis3D, HTX_TRACKIR_SENS, 
														3, gameOpts->input.trackIR.sensitivity, 16, NULL, KEY_S, axis3DHotkeys, gameOpts->input.trackIR.bSyncAxes, &opt);
			nTrackIRDZoneOpt = AddDeadzoneControl (m, szTrackIRDeadzone, TXT_TRACKIR_DEADZONE, HTX_TRACKIR_DEADZONE, gameOpts->input.trackIR.nDeadzone, KEY_U, &opt);
			}
		}
	i = ExecMenu1 (NULL, TXT_TRACKIR_CONFIG, opt, m, TrackIRConfigCallback, &choice);
	if (gameStates.input.bHaveTrackIR)
		gameOpts->input.trackIR.bUse = m [nUseTrackIROpt].value;
	} while ((i >= 0) || (i == -2));
}

//------------------------------------------------------------------------------

void KeyboardConfigMenu (void)
{
	tMenuItem m [20];
	int	i, opt = 0, choice = 0;
	int	nCustKbdOpt, nCustHotKeysOpt;
	char	szKeyRampScale [50];

do {
	do {
		memset (&m, 0, sizeof (m));
		opt = 0;

		nCustHotKeysOpt = -1;
		SetControlType ();
		ADD_MENU (opt, TXT_CUST_JOY, KEY_K, HTX_CONF_CUSTKBD);
		nCustKbdOpt = opt++;
		if (gameStates.app.bNostalgia) {
			gameOpts->input.keyboard.nRamp = 100;
			gameOpts->input.bUseHotKeys = 0;
			}
		else {
			if (gameOpts->app.bExpertMode) {
				ADD_TEXT (opt, "", 0);
				opt++;
				sprintf (szKeyRampScale + 1, TXT_KBD_RAMP, gameOpts->input.keyboard.nRamp, HTX_CONF_KBDRAMP);
				*szKeyRampScale = *(TXT_KBD_RAMP - 1);
				ADD_SLIDER (opt, szKeyRampScale + 1, gameOpts->input.keyboard.nRamp / 10, 0, 10, KEY_R, HTX_CONF_RAMPSCALE);   
				nRampOpt = opt++;
				if (gameOpts->input.keyboard.nRamp > 0) {
					nRampKeyOpt = opt;
					ADD_CHECK (opt, TXT_RAMP_ACCEL, gameOpts->input.keyboard.bRamp [0], KEY_A, HTX_CONF_RAMPACC);
					opt++;
					ADD_CHECK (opt, TXT_RAMP_ROT, gameOpts->input.keyboard.bRamp [1], KEY_O, HTX_CONF_RAMPROT);
					opt++;
					ADD_CHECK (opt, TXT_RAMP_SLIDE, gameOpts->input.keyboard.bRamp [2], KEY_D, HTX_CONF_RAMPSLD);
					opt++;
					}
				}
			ADD_TEXT (opt, "", 0);
			opt++;
			ADD_CHECK (opt, TXT_USE_HOTKEYS, gameOpts->input.bUseHotKeys, KEY_H, HTX_CONF_HOTKEYS);
			nUseHotKeysOpt = opt++;
			if (gameOpts->input.bUseHotKeys) {
				ADD_MENU (opt, TXT_CUST_HOTKEYS, KEY_W, HTX_CONF_CUSTHOT);
				nCustHotKeysOpt = opt++;
				}
			}
			i = ExecMenu1 (NULL, TXT_KEYBOARD_CONFIG, opt, m, KeyboardConfigCallback, &choice);
		if (!gameStates.app.bNostalgia)
			gameOpts->input.bUseHotKeys = m [nUseHotKeysOpt].value;
		} while (i == -2);
	if (i == -1)
		return;
	if (choice == nCustKbdOpt)
		KConfig (0, TXT_CFG_KBD);
	else if (choice == nCustHotKeysOpt)
		KConfig (4, TXT_CFG_HOTKEYS);
	} while (i >= 0);
}

//------------------------------------------------------------------------------

void InputDeviceConfig (void)
{
	tMenuItem m [20];
	int	i, opt = 0, choice = 0;
	int	nMouseOpt, nJoystickOpt, nTrackIROpt, nKeyboardOpt, nFastPitchOpt;

do {
	memset (&m, 0, sizeof (m));
	opt = 0;

	SetControlType ();
	ADD_MENU (opt, TXT_KBDCFG_MENUCALL, KEY_K, HTX_KEYBOARD_CONFIG);
	nKeyboardOpt = opt++;
	ADD_MENU (opt, TXT_MOUSECFG_MENUCALL, KEY_M, HTX_MOUSE_CONFIG);
	nMouseOpt = opt++;
	if (gameStates.input.nJoysticks) {
		ADD_MENU (opt, TXT_JOYCFG_MENUCALL, KEY_J, HTX_JOYSTICK_CONFIG);
		nJoystickOpt = opt++;
		}
	else
		nJoystickOpt = -1;
	if (!gameStates.app.bNostalgia && gameStates.input.bHaveTrackIR) {
		ADD_MENU (opt, TXT_TRACKIRCFG_MENUCALL, KEY_I, HTX_TRACKIR_CONFIG);
		nTrackIROpt = opt++;
		}
	else
		nTrackIROpt = -1;
	ADD_TEXT (opt, "", 0);
	opt++;
	if (gameOpts->app.bExpertMode && (gameStates.app.bNostalgia < 3)) {
		ADD_CHECK (opt, TXT_FASTPITCH, (extraGameInfo [0].bFastPitch == 1) ? 1 : 0, KEY_T, HTX_CONF_FASTPITCH);
		nFastPitchOpt = opt++;
		extraGameInfo [0].bFastPitch = m [nFastPitchOpt].value ? 1 : 2;
		}
	else
		nFastPitchOpt = -1;
	i = ExecMenu1 (NULL, TXT_CONTROLS, opt, m, NULL, &choice);
	if (i == -1)
		return;
	if (nFastPitchOpt >= 0)
		extraGameInfo [0].bFastPitch = m [nFastPitchOpt].value ? 1 : 2;
	if (choice == nKeyboardOpt)
		KeyboardConfigMenu ();
	else if (choice == nMouseOpt)
		MouseConfigMenu ();
	else if ((nJoystickOpt >= 0) && (choice == nJoystickOpt))
		JoystickConfigMenu ();
	else if ((nTrackIROpt >= 0) && (choice == nTrackIROpt))
		TrackIRConfigMenu ();
	} while (i >= 0);
}

//------------------------------------------------------------------------------
// eof

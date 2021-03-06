#define _CRT_SECURE_NO_WARNINGS 
#define _CRT_NON_CONFORMING_SWPRINTFS

#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhook.h"
#include "dxhelper.h"
#include "resource.h"

#include "MMSystem.h"
#include <stdio.h>

#define SUPPRESSMCIERRORS FALSE
#define EMULATEJOY TRUE
#define INVERTJOYAXIS TRUE

//#include "logall.h" // comment when not debugging

BOOL IsWithinMCICall = FALSE;

typedef MMRESULT (WINAPI *timeGetDevCaps_Type)(LPTIMECAPS, UINT);
timeGetDevCaps_Type ptimeGetDevCaps = NULL;
MMRESULT WINAPI exttimeGetDevCaps(LPTIMECAPS, UINT);
typedef MCIDEVICEID (WINAPI *mciGetDeviceIDA_Type)(LPCTSTR);
mciGetDeviceIDA_Type pmciGetDeviceIDA = NULL;
MCIDEVICEID WINAPI extmciGetDeviceIDA(LPCTSTR);
typedef MCIDEVICEID (WINAPI *mciGetDeviceIDW_Type)(LPCWSTR);
mciGetDeviceIDW_Type pmciGetDeviceIDW = NULL;
MCIDEVICEID WINAPI extmciGetDeviceIDW(LPCWSTR);
typedef DWORD (WINAPI *joyGetNumDevs_Type)(void);
joyGetNumDevs_Type pjoyGetNumDevs = NULL;
DWORD WINAPI extjoyGetNumDevs(void);
typedef MMRESULT (WINAPI *joyGetDevCapsA_Type)(DWORD, LPJOYCAPS, UINT);
joyGetDevCapsA_Type pjoyGetDevCapsA = NULL;
MMRESULT WINAPI extjoyGetDevCapsA(DWORD, LPJOYCAPS, UINT);
typedef MMRESULT (WINAPI *joyGetPosEx_Type)(DWORD, LPJOYINFOEX);
joyGetPosEx_Type pjoyGetPosEx = NULL;
MMRESULT WINAPI extjoyGetPosEx(DWORD, LPJOYINFOEX);
typedef MMRESULT (WINAPI *joyGetPos_Type)(DWORD, LPJOYINFO);
joyGetPos_Type pjoyGetPos = NULL;
MMRESULT WINAPI extjoyGetPos(DWORD, LPJOYINFO);
typedef MMRESULT (WINAPI *auxGetNumDevs_Type)(void);
auxGetNumDevs_Type pauxGetNumDevs = NULL;
MMRESULT WINAPI extauxGetNumDevs(void);
typedef BOOL (WINAPI *mciGetErrorStringA_Type)(DWORD, LPCSTR, UINT);
mciGetErrorStringA_Type pmciGetErrorStringA;
BOOL WINAPI extmciGetErrorStringA(DWORD, LPCSTR, UINT);
typedef MMRESULT (WINAPI *mixerGetLineControlsA_Type)(HMIXEROBJ, LPMIXERLINECONTROLS, DWORD);
mixerGetLineControlsA_Type pmixerGetLineControlsA;
MMRESULT WINAPI extmixerGetLineControlsA(HMIXEROBJ, LPMIXERLINECONTROLS, DWORD);
typedef UINT (WINAPI *waveOutGetNumDevs_Type)(void);
waveOutGetNumDevs_Type pwaveOutGetNumDevs;
UINT WINAPI extwaveOutGetNumDevs(void);
typedef UINT (WINAPI *mixerGetNumDevs_Type)(void);
mixerGetNumDevs_Type pmixerGetNumDevs;
UINT WINAPI extmixerGetNumDevs(void);
typedef UINT (WINAPI *timeBeginPeriod_Type)(UINT);
timeBeginPeriod_Type ptimeBeginPeriod;
UINT WINAPI exttimeBeginPeriod(UINT);
typedef UINT (WINAPI *timeEndPeriod_Type)(UINT);
timeEndPeriod_Type ptimeEndPeriod;
UINT WINAPI exttimeEndPeriod(UINT);


static HookEntryEx_Type Hooks[]={
	{HOOK_IAT_CANDIDATE, 0, "mciSendCommandA", NULL, (FARPROC *)&pmciSendCommandA, (FARPROC)extmciSendCommandA},
	{HOOK_IAT_CANDIDATE, 0, "mciSendCommandW", NULL, (FARPROC *)&pmciSendCommandW, (FARPROC)extmciSendCommandW},
	{HOOK_HOT_CANDIDATE, 0, "mciGetDeviceIDA", NULL, (FARPROC *)&pmciGetDeviceIDA, (FARPROC)extmciGetDeviceIDA},
	{HOOK_HOT_CANDIDATE, 0, "mciGetDeviceIDW", NULL, (FARPROC *)&pmciGetDeviceIDW, (FARPROC)extmciGetDeviceIDW},
	//{HOOK_IAT_CANDIDATE, 0, "auxGetNumDevs", NULL, (FARPROC *)&pauxGetNumDevs, (FARPROC)extauxGetNumDevs},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type TimeHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "timeGetTime", NULL, (FARPROC *)&ptimeGetTime, (FARPROC)exttimeGetTime},
	{HOOK_HOT_CANDIDATE, 0, "timeKillEvent", NULL, (FARPROC *)&ptimeKillEvent, (FARPROC)exttimeKillEvent},
	{HOOK_HOT_CANDIDATE, 0, "timeSetEvent", NULL, (FARPROC *)&ptimeSetEvent, (FARPROC)exttimeSetEvent},
	{HOOK_HOT_CANDIDATE, 0, "timeGetDevCaps", NULL, (FARPROC *)&ptimeGetDevCaps, (FARPROC)exttimeGetDevCaps},
	{HOOK_HOT_CANDIDATE, 0, "timeBeginPeriod", NULL, (FARPROC *)&ptimeBeginPeriod, (FARPROC)exttimeBeginPeriod},
	{HOOK_HOT_CANDIDATE, 0, "timeEndPeriod", NULL, (FARPROC *)&ptimeEndPeriod, (FARPROC)exttimeEndPeriod},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type RemapHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "mciSendStringA", NULL, (FARPROC *)&pmciSendStringA, (FARPROC)extmciSendStringA},
	{HOOK_HOT_CANDIDATE, 0, "mciSendStringW", NULL, (FARPROC *)&pmciSendStringW, (FARPROC)extmciSendStringW},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type JoyHooks[]={
	{HOOK_HOT_CANDIDATE, 0, "joyGetNumDevs", NULL, (FARPROC *)&pjoyGetNumDevs, (FARPROC)extjoyGetNumDevs},
	{HOOK_HOT_CANDIDATE, 0, "joyGetDevCapsA", NULL, (FARPROC *)&pjoyGetDevCapsA, (FARPROC)extjoyGetDevCapsA},
	{HOOK_HOT_CANDIDATE, 0, "joyGetPosEx", NULL, (FARPROC *)&pjoyGetPosEx, (FARPROC)extjoyGetPosEx},
	{HOOK_HOT_CANDIDATE, 0, "joyGetPos", NULL, (FARPROC *)&pjoyGetPos, (FARPROC)extjoyGetPos},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

static HookEntryEx_Type DebugHooks[]={
	{HOOK_IAT_CANDIDATE, 0, "mciGetErrorStringA", NULL, (FARPROC *)&pmciGetErrorStringA, (FARPROC)extmciGetErrorStringA},
	{HOOK_IAT_CANDIDATE, 0, "mixerGetLineControlsA", NULL, (FARPROC *)&pmixerGetLineControlsA, (FARPROC)extmixerGetLineControlsA},
	{HOOK_IAT_CANDIDATE, 0, "waveOutGetNumDevs", NULL, (FARPROC *)&pwaveOutGetNumDevs, (FARPROC)extwaveOutGetNumDevs},
	{HOOK_IAT_CANDIDATE, 0, "auxGetNumDevs", NULL, (FARPROC *)&pauxGetNumDevs, (FARPROC)extauxGetNumDevs},
	{HOOK_IAT_CANDIDATE, 0, "mixerGetNumDevs", NULL, (FARPROC *)&pmixerGetNumDevs, (FARPROC)extmixerGetNumDevs},
	{HOOK_IAT_CANDIDATE, 0, 0, NULL, 0, 0} // terminator
};

void HookWinMM(HMODULE module, char *libname)
{
	HookLibraryEx(module, Hooks, libname);
	if(dxw.dwFlags2 & TIMESTRETCH) HookLibraryEx(module, TimeHooks, libname);
	if(dxw.dwFlags5 & REMAPMCI) HookLibraryEx(module, RemapHooks, libname);
	if(dxw.dwFlags6 & VIRTUALJOYSTICK) HookLibraryEx(module, JoyHooks, libname);
	if(IsDebug) HookLibraryEx(module, DebugHooks, libname);
}

FARPROC Remap_WinMM_ProcAddress(LPCSTR proc, HMODULE hModule)
{
	FARPROC addr;

	if (addr=RemapLibraryEx(proc, hModule, Hooks)) return addr;
	if(dxw.dwFlags2 & TIMESTRETCH)
		if (addr=RemapLibraryEx(proc, hModule, TimeHooks)) return addr;
	if(dxw.dwFlags5 & REMAPMCI)
		if (addr=RemapLibraryEx(proc, hModule, RemapHooks)) return addr;
	if(dxw.dwFlags6 & VIRTUALJOYSTICK)
		if (addr=RemapLibraryEx(proc, hModule, JoyHooks)) return addr;
	if(IsDebug)
		if (addr=RemapLibraryEx(proc, hModule, DebugHooks)) return addr;

	return NULL;
}

MMRESULT WINAPI exttimeGetDevCaps(LPTIMECAPS ptc, UINT cbtc)
{
	MMRESULT res;
	res = (*ptimeGetDevCaps)(ptc, cbtc);
	if(res) {
		OutTraceE("timeGetDevCaps ERROR: res=%x err=%d\n", res, GetLastError());
	}
	else {
		OutTraceDW("timeGetDevCaps: period min=%d max=%d\n", ptc->wPeriodMin, ptc->wPeriodMax);
	}
	return MMSYSERR_NOERROR;
}

DWORD WINAPI exttimeGetTime(void)
{
	DWORD ret;
	ret = dxw.GetTickCount();
	//OutTraceB("timeGetTime: time=%x\n", ret);
	return ret;
}

MMRESULT WINAPI exttimeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent)
{
	MMRESULT res;
	UINT NewDelay;
	OutTraceDW("timeSetEvent: Delay=%d Resolution=%d Event=%x\n", uDelay, uResolution, fuEvent);
	if(dxw.dwFlags4 & STRETCHTIMERS) NewDelay = dxw.StretchTime(uDelay);
	else NewDelay = uDelay;
	res=(*ptimeSetEvent)(NewDelay, uResolution, lpTimeProc, dwUser, fuEvent);
	if(res) dxw.PushTimer(res, uDelay, uResolution, lpTimeProc, dwUser, fuEvent);
	OutTraceDW("timeSetEvent: ret=%x\n", res);
	return res;
}

MMRESULT WINAPI exttimeKillEvent(UINT uTimerID)
{
	MMRESULT res;
	OutTraceDW("timeKillEvent: TimerID=%x\n", uTimerID);
	res=(*ptimeKillEvent)(uTimerID);
	if(res==TIMERR_NOERROR) dxw.PopTimer(uTimerID);
	OutTraceDW("timeKillEvent: ret=%x\n", res);
	return res;
}

MMRESULT WINAPI exttimeBeginPeriod(UINT uPeriod)
{
	MMRESULT res;
	OutTraceDW("timeBeginPeriod: period=%d\n", uPeriod);
	res=(*ptimeBeginPeriod)(uPeriod);
	OutTraceDW("timeBeginPeriod: ret=%x\n", res);
	return res;
}

MMRESULT WINAPI exttimeEndPeriod(UINT uPeriod)
{
	MMRESULT res;
	OutTraceDW("timeEndPeriod: period=%d\n", uPeriod);
	res=(*ptimeEndPeriod)(uPeriod);
	OutTraceDW("timeEndPeriod: ret=%x\n", res);
	return res;
}

/* MCI_DGV_PUT_FRAME

    The rectangle defined for MCI_DGV_RECT applies to the frame rectangle. 
	The frame rectangle specifies the portion of the frame buffer used as the destination of the video images obtained from the video rectangle. 
	The video should be scaled to fit within the frame buffer rectangle.
    The rectangle is specified in frame buffer coordinates. 
	The default rectangle is the full frame buffer. 
	Specifying this rectangle lets the device scale the image as it digitizes the data. 
	Devices that cannot scale the image reject this command with MCIERR_UNSUPPORTED_FUNCTION. 
	You can use the MCI_GETDEVCAPS_CAN_STRETCH flag with the MCI_GETDEVCAPS command to determine if a device scales the image. A device returns FALSE if it cannot scale the image.
*/

static char *sStatusItem(DWORD dwItem)
{
	char *s;
	switch(dwItem){
		case MCI_STATUS_LENGTH:				s = "LENGTH"; break;
		case MCI_STATUS_POSITION:			s = "POSITION"; break;
		case MCI_STATUS_NUMBER_OF_TRACKS:	s = "NUMBER_OF_TRACKS"; break;
		case MCI_STATUS_MODE:				s = "MODE"; break;
		case MCI_STATUS_MEDIA_PRESENT:		s = "MEDIA_PRESENT"; break;
		case MCI_STATUS_TIME_FORMAT:		s = "TIME_FORMAT"; break;
		case MCI_STATUS_READY:				s = "READY"; break;
		case MCI_STATUS_CURRENT_TRACK:		s = "CURRENT_TRACK"; break;
		default:							s = "???"; break;
	}
	return s;
}

static char *sDeviceType(DWORD dt)
{
	char *s;
	switch(dt){
		case MCI_ALL_DEVICE_ID: s="ALL_DEVICE_ID"; break;
		case MCI_DEVTYPE_VCR: s="VCR"; break;
		case MCI_DEVTYPE_VIDEODISC: s="VIDEODISC"; break;
		case MCI_DEVTYPE_OVERLAY: s="OVERLAY"; break;
		case MCI_DEVTYPE_CD_AUDIO: s="CD_AUDIO"; break;
		case MCI_DEVTYPE_DAT: s="DAT"; break;
		case MCI_DEVTYPE_SCANNER: s="SCANNER"; break;
		case MCI_DEVTYPE_ANIMATION: s="ANIMATION"; break;
		case MCI_DEVTYPE_DIGITAL_VIDEO: s="DIGITAL_VIDEO"; break;
		case MCI_DEVTYPE_OTHER: s="OTHER"; break;
		case MCI_DEVTYPE_WAVEFORM_AUDIO: s="WAVEFORM_AUDIO"; break;
		case MCI_DEVTYPE_SEQUENCER: s="SEQUENCER"; break;
		default: s="unknown"; break;
	}
	return s;
}

static char *sTimeFormat(DWORD tf)
{
	char *s;
	switch(tf){
		case MCI_FORMAT_MILLISECONDS: s="MILLISECONDS"; break;
		case MCI_FORMAT_HMS: s="HMS"; break;
		case MCI_FORMAT_MSF: s="MSF"; break;
		case MCI_FORMAT_FRAMES: s="FRAMES"; break;
		case MCI_FORMAT_SMPTE_24: s="SMPTE_24"; break;
		case MCI_FORMAT_SMPTE_25: s="SMPTE_25"; break;
		case MCI_FORMAT_SMPTE_30: s="SMPTE_30"; break;
		case MCI_FORMAT_SMPTE_30DROP: s="SMPTE_30DROP"; break;
		case MCI_FORMAT_BYTES: s="BYTES"; break;
		case MCI_FORMAT_SAMPLES: s="SAMPLES"; break;
		case MCI_FORMAT_TMSF: s="TMSF"; break;
		default: s="unknown"; break;
	}
	return s;
}

static void DumpMciMessage(char *label, BOOL isAnsi, UINT uMsg, DWORD_PTR fdwCommand, DWORD_PTR dwParam)
{
	char *api="mciSendCommand";
	switch(uMsg){
		case MCI_BREAK: 
			{
				LPMCI_BREAK_PARMS lpBreak = (LPMCI_BREAK_PARMS)dwParam;
				OutTrace("%s%s: MCI_BREAK cb=%x virtkey=%d hwndbreak=%x\n",
					api, label, lpBreak->dwCallback, lpBreak->nVirtKey, lpBreak->hwndBreak);
			}
			break;
		case MCI_INFO: 
			{
				LPMCI_INFO_PARMS lpInfo = (LPMCI_INFO_PARMS)dwParam;
				OutTrace("%s%s: MCI_INFO cb=%x retsize=%x\n",
					api, label, lpInfo->dwCallback, lpInfo->dwRetSize);
			}
			break;
		case MCI_PLAY: 
			{
				LPMCI_PLAY_PARMS lpPlay = (LPMCI_PLAY_PARMS)dwParam;
				OutTrace("%s%s: MCI_PLAY cb=%x from=%x to=%x\n",
					api, label, lpPlay->dwCallback, lpPlay->dwFrom, lpPlay->dwTo);
			}
			break;
		case MCI_GETDEVCAPS:
			{
				LPMCI_GETDEVCAPS_PARMS lpDevCaps = (LPMCI_GETDEVCAPS_PARMS)dwParam;
				OutTrace("%s%s: MCI_GETDEVCAPS cb=%x ret=%x item=%x\n",
					api, label, lpDevCaps->dwCallback, lpDevCaps->dwReturn, lpDevCaps->dwItem);
			}
			break;
		case MCI_OPEN: 
			{
				DWORD dwFlags = (DWORD)fdwCommand;
				// how to dump LPMCI_OPEN_PARMS strings without crash?
				if(isAnsi){
					LPMCI_OPEN_PARMSA lpOpen = (LPMCI_OPEN_PARMSA)dwParam;
					OutTrace("%s%s: MCI_OPEN %scb=%x devid=%x devtype=%s elementname=%s alias=%s\n",
						api, label, 
						(dwFlags & MCI_OPEN_SHAREABLE) ? "OPEN_SHAREABLE " : "",
						lpOpen->dwCallback, lpOpen->wDeviceID,
						"", //(dwFlags & MCI_OPEN_TYPE) ? lpOpen->lpstrDeviceType : "",
						(dwFlags & MCI_OPEN_ELEMENT) ? lpOpen->lpstrElementName : "",
						(dwFlags & MCI_OPEN_ALIAS) ? lpOpen->lpstrAlias : "");
				}
				else{
					LPMCI_OPEN_PARMSW lpOpen = (LPMCI_OPEN_PARMSW)dwParam;
					OutTrace("%s%s: MCI_OPEN cb=%x devid=%x devtype=%ls elementname=%ls alias=%ls\n",
						api, label, 
						(dwFlags & MCI_OPEN_SHAREABLE) ? "OPEN_SHAREABLE " : "",
						lpOpen->dwCallback, lpOpen->wDeviceID,
						L"", //(dwFlags & MCI_OPEN_TYPE) ? lpOpen->lpstrDeviceType : L"",
						(dwFlags & MCI_OPEN_ELEMENT) ? lpOpen->lpstrElementName : L"",
						(dwFlags & MCI_OPEN_ALIAS) ? lpOpen->lpstrAlias : L"");
				}
			}
			break;
		case MCI_STATUS:
			{
				LPMCI_STATUS_PARMS lpStatus = (LPMCI_STATUS_PARMS)dwParam;
				OutTrace("%s%s: MCI_STATUS cb=%x ret=%x item=%x(%s) track=%x\n",
					api, label, lpStatus->dwCallback, lpStatus->dwReturn, lpStatus->dwItem, sStatusItem(lpStatus->dwItem), lpStatus->dwTrack);
			}
			break;
		case MCI_SYSINFO:
			{
				LPMCI_SYSINFO_PARMS lpSysInfo = (LPMCI_SYSINFO_PARMS)dwParam;
				OutTrace("%s%s: MCI_SYSINFO cb=%x retsize=%x number=%x devtype=%x(%s)\n",
					api, label, lpSysInfo->dwCallback, lpSysInfo->dwRetSize, lpSysInfo->dwNumber, lpSysInfo->wDeviceType, sDeviceType(lpSysInfo->wDeviceType));
			}
			break;
		case MCI_SET:
			{
				LPMCI_SET_PARMS lpSetInfo = (LPMCI_SET_PARMS)dwParam;
				OutTrace("%s%s: MCI_SET cb=%x audio=%x timeformat=%x(%s)\n",
					api, label, lpSetInfo->dwCallback, lpSetInfo->dwAudio, lpSetInfo->dwTimeFormat, sTimeFormat(lpSetInfo->dwTimeFormat));
			}
			break;
		default:
			{
				LPMCI_GENERIC_PARMS lpGeneric = (LPMCI_GENERIC_PARMS)dwParam;
				if(lpGeneric)
					OutTrace("%s%s: %s cb=%x\n", api, label, ExplainMCICommands(uMsg), lpGeneric->dwCallback);
				else
					OutTrace("%s%s: %s params=(NULL)\n", api, label, ExplainMCICommands(uMsg));
			}
			break;
	}
}

MCIERROR WINAPI extmciSendCommand(BOOL isAnsi, mciSendCommand_Type pmciSendCommand, MCIDEVICEID IDDevice, UINT uMsg, DWORD_PTR fdwCommand, DWORD_PTR dwParam)
{
	RECT saverect;
	MCIERROR ret;
	LPMCI_ANIM_RECT_PARMS pr;
	LPMCI_OVLY_WINDOW_PARMSA pw;
	LPMCI_OPEN_PARMSA po;

	OutTraceDW("mciSendCommand%c: IDDevice=%x msg=%x(%s) Command=%x(%s)\n",
		isAnsi ? 'A' : 'W', 
		IDDevice, 
		uMsg, ExplainMCICommands(uMsg), 
		fdwCommand, ExplainMCIFlags(uMsg, fdwCommand));

	if(IsDebug) DumpMciMessage(">>", isAnsi, uMsg, fdwCommand, dwParam);

	if(dxw.dwFlags6 & BYPASSMCI){
		//MCI_OPEN_PARMS *op;
		MCI_STATUS_PARMS *sp;
		ret = 0;
		switch(uMsg){
			case MCI_OPEN:
				po = (MCI_OPEN_PARMSA *)dwParam;
				po->wDeviceID = 1;
				break;
			case MCI_STATUS:
				if(fdwCommand & MCI_STATUS_ITEM){
					// fix for Tie Fighter 95: when bypassing, let the caller know you have no CD tracks
					// otherwise you risk an almost endless loop going through the unassigned returned 
					// number of ghost tracks
					// fix for "Emperor of the Fading Suns": the MCI_STATUS_ITEM is set in .or. with
					// MCI_TRACK
					sp = (MCI_STATUS_PARMS *)dwParam;
					switch(fdwCommand){
						case MCI_TRACK:
							sp->dwReturn = 1;
							break;
						default:
							sp->dwTrack = 0;
							if(sp->dwItem == MCI_STATUS_CURRENT_TRACK) sp->dwTrack = 1;
							if(sp->dwItem == MCI_STATUS_NUMBER_OF_TRACKS) sp->dwTrack = 1;
							if(sp->dwItem == MCI_STATUS_LENGTH) sp->dwTrack = 200;
							if(sp->dwItem == MCI_STATUS_MEDIA_PRESENT) sp->dwTrack = 1;
							sp->dwReturn = 0;
							break;
					}
				}
				break;
			default:
				break;
		}
		if(IsDebug) DumpMciMessage("<<", isAnsi, uMsg, fdwCommand, dwParam);
		return ret;
	}

	if(dxw.IsFullScreen()){
		switch(uMsg){
			case MCI_WINDOW:
				pw = (MCI_OVLY_WINDOW_PARMSA *)dwParam;
				OutTraceB("mciSendCommand: hwnd=%x CmdShow=%x\n",
					pw->hWnd, pw->nCmdShow);
				if(dxw.IsRealDesktop(pw->hWnd)) {
					pw->hWnd = dxw.GethWnd();
					OutTraceB("mciSendCommand: REDIRECT hwnd=%x\n", pw->hWnd);
				}
				// attempt to stretch "Wizardry Chronicle" intro movie, but it doesn't work ...
				//if(1){
				//	fdwCommand &= ~MCI_OVLY_WINDOW_DISABLE_STRETCH;
				//	fdwCommand |= MCI_OVLY_WINDOW_ENABLE_STRETCH;
				//	fdwCommand |= MCI_ANIM_WINDOW_HWND;
				//	OutTraceB("mciSendCommand: STRETCH flags=%x hwnd=%x\n", fdwCommand, pw->hWnd);
				//}
				break;
			case MCI_PUT:
				RECT client;
				pr = (MCI_ANIM_RECT_PARMS *)dwParam;
				OutTraceB("mciSendCommand: rect=(%d,%d),(%d,%d)\n",
					pr->rc.left, pr->rc.top, pr->rc.right, pr->rc.bottom);
				(*pGetClientRect)(dxw.GethWnd(), &client);
				//fdwCommand |= MCI_ANIM_PUT_DESTINATION;
				fdwCommand |= MCI_ANIM_RECT;
				saverect=pr->rc;
				pr->rc.top = (pr->rc.top * client.bottom) / dxw.GetScreenHeight();
				pr->rc.bottom = (pr->rc.bottom * client.bottom) / dxw.GetScreenHeight();
				pr->rc.left = (pr->rc.left * client.right) / dxw.GetScreenWidth();
				pr->rc.right = (pr->rc.right * client.right) / dxw.GetScreenWidth();
				OutTraceB("mciSendCommand: fixed rect=(%d,%d),(%d,%d)\n",
					pr->rc.left, pr->rc.top, pr->rc.right, pr->rc.bottom);
				break;
			case MCI_PLAY:
				if(dxw.dwFlags6 & NOMOVIES) return 0; // ???
				break;
			case MCI_OPEN:
				if(dxw.dwFlags6 & NOMOVIES) return 275; // quite brutal, but working ....
				break;
			case MCI_STOP:
				if(dxw.dwFlags6 & NOMOVIES) return 0; // ???
				break;
			case MCI_CLOSE:
				if(dxw.dwFlags6 & NOMOVIES) return 0; // ???
				break;
		}
	}

#if 0
	LPMCI_GENERIC_PARMS lpGeneric = (LPMCI_GENERIC_PARMS)dwParam;
	if(HIWORD(lpGeneric->dwCallback) == NULL) {
		lpGeneric->dwCallback = MAKELONG(dxw.GethWnd(),0);
		OutTraceB("mciSendCommand: REDIRECT dwCallback=%x\n", dxw.GethWnd());
	}
#endif

	ret=(*pmciSendCommand)(IDDevice, uMsg, fdwCommand, dwParam);
	if(IsDebug) DumpMciMessage("<<", isAnsi, uMsg, fdwCommand, dwParam);

	if(ret == 0){
		switch(uMsg){
		case MCI_STATUS:
			MCI_STATUS_PARMS *p = (MCI_STATUS_PARMS *)dwParam;
			OutTraceDW("mciSendCommand: Item=%d Track=%d return=%x\n", p->dwItem, p->dwTrack, p->dwReturn);
			break;
		}
	}

	if(dxw.IsFullScreen() && uMsg==MCI_PUT) pr->rc=saverect;
	if (ret) OutTraceE("mciSendCommand: ERROR res=%d\n", ret);
	return ret;
}

MCIERROR WINAPI extmciSendCommandA(MCIDEVICEID IDDevice, UINT uMsg, DWORD_PTR fdwCommand, DWORD_PTR dwParam)
{
	return extmciSendCommand(TRUE, pmciSendCommandA, IDDevice, uMsg, fdwCommand, dwParam);
}

MCIERROR WINAPI extmciSendCommandW(MCIDEVICEID IDDevice, UINT uMsg, DWORD_PTR fdwCommand, DWORD_PTR dwParam)
{
	return extmciSendCommand(FALSE, pmciSendCommandW, IDDevice, uMsg, fdwCommand, dwParam);
}

MCIERROR WINAPI extmciSendStringA(LPCTSTR lpszCommand, LPTSTR lpszReturnString, UINT cchReturn, HANDLE hwndCallback)
{
	MCIERROR ret;
	if(IsWithinMCICall) return(*pmciSendStringA)(lpszCommand, lpszReturnString, cchReturn, hwndCallback); // just proxy ...
	OutTraceDW("mciSendStringA: Command=\"%s\" Return=%x Callback=%x\n", lpszCommand, cchReturn, hwndCallback);
	char sMovieNickName[81];
	char sTail[81];
	RECT rect;
	sTail[0]=0;
	if (sscanf(lpszCommand, "put %s destination at %ld %ld %ld %ld %s", 
		sMovieNickName, &(rect.left), &(rect.top), &(rect.right), &(rect.bottom), sTail)>=5){
		char NewCommand[256];
		// v2.03.19 height / width fix
		rect.right += rect.left; // convert width to position
		rect.bottom += rect.top; // convert height to position
		rect=dxw.MapClientRect(&rect);
		rect.right -= rect.left; // convert position to width
		rect.bottom -= rect.top; // convert position to height
		sprintf(NewCommand, "put %s destination at %d %d %d %d %s", sMovieNickName, rect.left, rect.top, rect.right, rect.bottom, sTail);
		lpszCommand=NewCommand;
		OutTraceDW("mciSendStringA: replaced Command=\"%s\"\n", lpszCommand);
	}
	IsWithinMCICall=TRUE;
	ret=(*pmciSendStringA)(lpszCommand, lpszReturnString, cchReturn, hwndCallback);
	IsWithinMCICall=FALSE;
	if(ret) OutTraceDW("mciSendStringA ERROR: ret=%x\n", ret);
	OutTraceDW("mciSendStringA: RetString=\"%s\"\n", lpszReturnString);
	return ret;
}

MCIERROR WINAPI extmciSendStringW(LPCWSTR lpszCommand, LPWSTR lpszReturnString, UINT cchReturn, HANDLE hwndCallback)
{
	MCIERROR ret;
	WCHAR *target=L"put movie destination at ";
	if(IsWithinMCICall) return(*pmciSendStringW)(lpszCommand, lpszReturnString, cchReturn, hwndCallback); // just proxy ...
	OutTraceDW("mciSendStringW: Command=\"%ls\" Return=%x Callback=%x\n", lpszCommand, cchReturn, hwndCallback);
	WCHAR sMovieNickName[81];
	RECT rect;
	if (swscanf(lpszCommand, L"put %ls destination at %ld %ld %ld %ld", 
		sMovieNickName, &(rect.left), &(rect.top), &(rect.right), &(rect.bottom))==5){
		WCHAR NewCommand[256];
		// v2.03.19 height / width fix
		rect.right += rect.left; // convert width to position
		rect.bottom += rect.top; // convert height to position
		rect=dxw.MapClientRect(&rect);
		rect.right -= rect.left; // convert position to width
		rect.bottom -= rect.top; // convert position to height
		swprintf(NewCommand, L"put %ls destination at %d %d %d %d", sMovieNickName, rect.left, rect.top, rect.right, rect.bottom);
		lpszCommand=NewCommand;
		OutTraceDW("mciSendStringW: replaced Command=\"%ls\"\n", lpszCommand);
	}
	IsWithinMCICall=TRUE;
	ret=(*pmciSendStringW)(lpszCommand, lpszReturnString, cchReturn, hwndCallback);
	IsWithinMCICall=FALSE;
	if(ret) OutTraceDW("mciSendStringW ERROR: ret=%x\n", ret);
	OutTraceDW("mciSendStringW: RetString=\"%ls\"\n", lpszReturnString);
	return ret;
}

MCIDEVICEID WINAPI extmciGetDeviceIDA(LPCTSTR lpszDevice)
{
	MCIDEVICEID ret;
	ret = (*pmciGetDeviceIDA)(lpszDevice);
	OutTraceDW("mciGetDeviceIDA: device=\"%s\" ret=%x\n", lpszDevice, ret);
	return ret;
}

MCIDEVICEID WINAPI extmciGetDeviceIDW(LPCWSTR lpszDevice)
{
	MCIDEVICEID ret;
	ret = (*pmciGetDeviceIDW)(lpszDevice);
	OutTraceDW("mciGetDeviceIDW: device=\"%ls\" ret=%x\n", lpszDevice, ret);
	return ret;
}

DWORD WINAPI extjoyGetNumDevs(void)
{
	OutTraceDW("joyGetNumDevs: emulate joystick ret=1\n");
	return 1;
}

#define XSPAN 128
#define YSPAN 128
static void ShowJoystick(LONG, LONG, DWORD);

MMRESULT WINAPI extjoyGetDevCapsA(DWORD uJoyID, LPJOYCAPS pjc, UINT cbjc)
{
	OutTraceDW("joyGetDevCaps: joyid=%d size=%d\n", uJoyID, cbjc);
	if((uJoyID != -1) && (uJoyID != 0)) {
		OutTraceDW("joyGetDevCaps: ERROR joyid=%d ret=MMSYSERR_NODRIVER\n", uJoyID, cbjc);
		return MMSYSERR_NODRIVER;
	}
	if(cbjc != sizeof(JOYCAPS)) {
		OutTraceDW("joyGetDevCaps: ERROR joyid=%d size=%d ret=MMSYSERR_INVALPARAM\n", uJoyID, cbjc);
		return MMSYSERR_INVALPARAM;
	}
	uJoyID = 0; // always first (unique) one ...

	// set Joystick capability structure
	memset(pjc, 0, sizeof(JOYCAPS));
	strncpy(pjc->szPname, "DxWnd Joystick Emulator", MAXPNAMELEN);
	pjc->wXmin = -XSPAN;
	pjc->wXmax = XSPAN;
	pjc->wYmin = -YSPAN;
	pjc->wYmax = YSPAN;
	pjc->wNumButtons = 2;
	pjc->wMaxButtons = 2;
	pjc->wPeriodMin = 60;
	pjc->wPeriodMax = 600;
	pjc->wCaps = 0;
	pjc->wMaxAxes = 2;
	pjc->wNumAxes = 2;

	return JOYERR_NOERROR;
}

BOOL JoyProcessMouseWheelMessage(WPARAM wParam, LPARAM lParam)
{
	int zDelta;
	DWORD dwSensivity = GetHookInfo()->VJoySensivity;
	DWORD dwJoyStatus =	GetHookInfo()->VJoyStatus;

	if(!(dwJoyStatus & VJMOUSEWHEEL)) return FALSE;
	
	if(!dwSensivity) dwSensivity=100;
	//fwKeys = GET_KEYSTATE_WPARAM(wParam);
	zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	if(zDelta >  4 * WHEEL_DELTA) zDelta =  4 * WHEEL_DELTA;
	if(zDelta < -4 * WHEEL_DELTA) zDelta = -4 * WHEEL_DELTA;
	if(zDelta > 0) dwSensivity = (dwSensivity * 110 *  zDelta) / (100 * WHEEL_DELTA);
	if(zDelta < 0) dwSensivity = (dwSensivity * 100 * -zDelta) / (110 * WHEEL_DELTA);
	if(dwSensivity < 32) dwSensivity = 32;
	if(dwSensivity > 250) dwSensivity = 250;
	GetHookInfo()->VJoySensivity = dwSensivity;
	return TRUE;
}

static MMRESULT GetJoy(char *apiname, DWORD uJoyID, LPJOYINFO lpj)
{
	OutTraceC("%s: joyid=%d\n", apiname, uJoyID);
	if(uJoyID != 0) return JOYERR_PARMS;
	LONG x, y, CenterX, CenterY;
	HWND hwnd;
	DWORD dwButtons;
	static BOOL bJoyLock = FALSE;
	static DWORD dwLastClick = 0;
	extern DXWNDSTATUS *pStatus;
	DWORD dwVJoyStatus;

	dwVJoyStatus = GetHookInfo()->VJoyStatus;
	if(!(dwVJoyStatus & VJOYENABLED)) {
		lpj->wXpos = 0;
		lpj->wYpos = 0;
		lpj->wZpos = 0;
		pStatus->joyposx = (short)0;
		pStatus->joyposy = (short)0;
		return JOYERR_NOERROR;
	}

	dwButtons = 0;
	if ((GetKeyState(VK_LBUTTON) < 0) || (dwVJoyStatus & B1AUTOFIRE)) dwButtons |= JOY_BUTTON1;
	if ((GetKeyState(VK_RBUTTON) < 0) || (dwVJoyStatus & B2AUTOFIRE)) dwButtons |= JOY_BUTTON2;
	if (GetKeyState(VK_MBUTTON) < 0) dwButtons |= JOY_BUTTON3;
	OutTraceB("%s: Virtual Joystick buttons=%x\n", apiname, dwButtons);

	if(dwButtons == JOY_BUTTON3){
		if(((*pGetTickCount)() - dwLastClick) > 200){
			bJoyLock = !bJoyLock;
			dwButtons &= ~JOY_BUTTON3;
			dwLastClick = (*pGetTickCount)();
		}
	}

	// default: centered position
	x=0;
	y=0;
	// get cursor position and map it to virtual joystick x,y axis
	if(hwnd=dxw.GethWnd()){
		POINT pt;
		RECT client;
		POINT upleft = {0,0};
		(*pGetClientRect)(hwnd, &client);
		(*pClientToScreen)(hwnd, &upleft);
		(*pGetCursorPos)(&pt);
		pt.x -= upleft.x;
		pt.y -= upleft.y;
		if(bJoyLock || !dxw.bActive){
			// when the joystick is "locked" (bJoyLock) or when the window lost focus
			// (dxw.bActive == FALSE) place the joystick in the central position
			OutTraceB("%s: CENTERED lock=%x active=%x\n", apiname, bJoyLock, dxw.bActive);
			x=0;
			y=0;
			pt.x = client.right >> 1;
			pt.y = client.bottom >> 1;
			dwButtons = JOY_BUTTON3;
		}
		else{
			OutTraceB("%s: ACTIVE mouse=(%d,%d)\n", apiname, pt.x, pt.y);
			CenterX = (client.right - client.left) >> 1;
			CenterY = (client.bottom - client.top) >> 1;

			if(dwVJoyStatus & VJMOUSEMAP){
				if(pt.x < client.left) pt.x = client.left;
				if(pt.x > client.right) pt.x = client.right;
				if(pt.y < client.top) pt.y = client.top;
				if(pt.y > client.bottom) pt.y = client.bottom;

				x = ((pt.x - CenterX) * XSPAN) / client.right;
				y = ((pt.y - CenterY) * YSPAN) / client.bottom;

				if(dwVJoyStatus & VJAUTOCENTER) {
					// autocenter: each time, moves 1/20 distance toward centered 0,0 position
					// 1/20 = 0.05, changing value changes the autocenter speed (must be >0.0 and <1.0)
					int x1, y1;
					x1 = (int)(pt.x + upleft.x - ((pt.x - CenterX) * 0.05));
					y1 = (int)(pt.y + upleft.y - ((pt.y - CenterY) * 0.05));
					if((x1 != pt.x+upleft.x) || (y1 != pt.y+upleft.y)) (*pSetCursorPos)(x1, y1);
				}
			}

			if(dwVJoyStatus & VJKEYBOARDMAP){
				if (GetKeyState(VK_LEFT) < 0)  x = -XSPAN/2;
				if (GetKeyState(VK_RIGHT) < 0) x = +XSPAN/2;
				if (GetKeyState(VK_UP) < 0)    y = -YSPAN/2;
				if (GetKeyState(VK_DOWN) < 0)  y = +YSPAN/2;
				if (GetKeyState(VK_SPACE) < 0)    dwButtons |= JOY_BUTTON1;
				if (GetKeyState(VK_LCONTROL) < 0) dwButtons |= JOY_BUTTON2;
			}

			if(dwVJoyStatus & INVERTXAXIS) x = -x;
			if(dwVJoyStatus & INVERTYAXIS) y = -y;
			//if(dwVJoyStatus & VJSENSIVITY){
			{
				DWORD dwSensivity = GetHookInfo()->VJoySensivity;
				if(dwSensivity){
					x = (x * (LONG)dwSensivity) / 100; 
					y = (y * (LONG)dwSensivity) / 100;
					if(x > +XSPAN) x = +XSPAN;
					if(x < -XSPAN) x = -XSPAN;
					if(y > +YSPAN) y = +YSPAN;
					if(y < -YSPAN) y = -YSPAN;
				}
			}
		}
		if (dwVJoyStatus & CROSSENABLED) {
			int jx, jy;
			jx  = CenterX + (x * client.right) / XSPAN;
			jy  = CenterY + (y * client.bottom) / YSPAN;
			ShowJoystick(jx, jy, dwButtons);
		}
	}
	lpj->wXpos = x;
	lpj->wYpos = y;
	lpj->wZpos = 0;
	lpj->wButtons = dwButtons;
	OutTraceC("%s: joyid=%d pos=(%d,%d)\n", apiname, uJoyID, lpj->wXpos, lpj->wYpos);
	pStatus->joyposx = (short)x;
	pStatus->joyposy = (short)y;
	return JOYERR_NOERROR;
}

MMRESULT WINAPI extjoyGetPosEx(DWORD uJoyID, LPJOYINFOEX pji)
{
	MMRESULT res;
	JOYINFO jinfo;
	res=GetJoy("joyGetPosEx", uJoyID, &jinfo);

	// set Joystick JOYINFOEX info structure
	memset(pji, 0, sizeof(JOYINFOEX));
	pji->dwSize = sizeof(JOYINFOEX);
	pji->dwFlags = 0;
	pji->dwXpos = jinfo.wXpos;
	pji->dwYpos = jinfo.wYpos;
	pji->dwButtons = jinfo.wButtons;
	pji->dwFlags = JOY_RETURNX|JOY_RETURNY|JOY_RETURNBUTTONS;

	return res;
}

MMRESULT WINAPI extjoyGetPos(DWORD uJoyID, LPJOYINFO pji)
{
	MMRESULT res;
	res=GetJoy("joyGetPos", uJoyID, pji);
	return res;
}

static void ShowJoystick(LONG x, LONG y, DWORD dwButtons)
{
	static BOOL JustOnce=FALSE;
	extern HMODULE hInst;
	BITMAP bm;
	HDC hClientDC;
	static HBITMAP g_hbmJoyCross;
	static HBITMAP g_hbmJoyFire1;
	static HBITMAP g_hbmJoyFire2;
	static HBITMAP g_hbmJoyFire3;
	static HBITMAP g_hbmJoyCenter;
	HBITMAP g_hbmJoy;
	RECT client;
	RECT win;
	POINT PrevViewPort;

	//return;

	// don't show when system cursor is visible
	CURSORINFO ci;
	ci.cbSize = sizeof(CURSORINFO);
	GetCursorInfo(&ci);
	if(ci.flags == CURSOR_SHOWING) return;

	hClientDC=(*pGDIGetDC)(dxw.GethWnd());
	(*pGetClientRect)(dxw.GethWnd(), &client);

	if(!JustOnce){
		g_hbmJoyCross = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CROSS));
		g_hbmJoyFire1 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FIRE1));
		g_hbmJoyFire2 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FIRE2));
		g_hbmJoyFire3 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FIRE3));
		g_hbmJoyCenter = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_JOYCENTER));
		JustOnce=TRUE;
	}

    HDC hdcMem = CreateCompatibleDC(hClientDC);
 	switch(dwButtons){
		case 0: g_hbmJoy = g_hbmJoyCross; break;
		case JOY_BUTTON1: g_hbmJoy = g_hbmJoyFire1; break;
		case JOY_BUTTON2: g_hbmJoy = g_hbmJoyFire2; break;
		case JOY_BUTTON1|JOY_BUTTON2: g_hbmJoy = g_hbmJoyFire3; break;
		case JOY_BUTTON3: g_hbmJoy = g_hbmJoyCenter; break;
		default: g_hbmJoy = NULL; break;
	}

	if(g_hbmJoy == NULL) return; // show nothing ...

    HBITMAP hbmOld = (HBITMAP)(*pSelectObject)(hdcMem, g_hbmJoy);
	GetObject(g_hbmJoy, sizeof(bm), &bm);

	(*pGetWindowRect)(dxw.GethWnd(), &win);

	(*pSetViewportOrgEx)(hClientDC, 0, 0, &PrevViewPort);
	int w, h;
	w=bm.bmWidth;
	h=bm.bmHeight; 
	(*pGDIBitBlt)(hClientDC, x-(w>>1), y-(h>>1), w, h, hdcMem, 0, 0, SRCPAINT);

	(*pSetViewportOrgEx)(hClientDC, PrevViewPort.x, PrevViewPort.y, NULL);
    (*pSelectObject)(hdcMem, hbmOld);
    DeleteDC(hdcMem);
}

MMRESULT WINAPI extauxGetNumDevs(void)
{
	UINT ret;
	ret = (*pauxGetNumDevs)();
	OutTrace("auxGetNumDevs: ret=%d\n", ret);
	return ret;
}

BOOL WINAPI extmciGetErrorStringA(DWORD fdwError, LPCSTR lpszErrorText, UINT cchErrorText)
{
	BOOL ret;
	ret = (*pmciGetErrorStringA)(fdwError, lpszErrorText, cchErrorText);
	OutTrace("mciGetErrorStringA: ret=%x err=%d text=(%d)\"%s\"\n", ret, fdwError, cchErrorText, lpszErrorText);
	return ret;
}

MMRESULT WINAPI extmixerGetLineControlsA(HMIXEROBJ hmxobj, LPMIXERLINECONTROLS pmxlc, DWORD fdwControls)
{
	MMRESULT ret;
	ret = (*pmixerGetLineControlsA)(hmxobj, pmxlc, fdwControls);
	OutTrace("mixerGetLineControlsA: ret=%x hmxobj=%x Controls=%x\n", ret, hmxobj, fdwControls);
	return ret;
}

UINT WINAPI extwaveOutGetNumDevs(void)
{
	UINT ret;
	ret = (*pwaveOutGetNumDevs)();
	OutTrace("waveOutGetNumDevs: ret=%d\n", ret);
	return ret;
}

UINT WINAPI extmixerGetNumDevs(void)
{
	UINT ret;
	ret = (*pmixerGetNumDevs)();
	OutTrace("mixerGetNumDevs: ret=%d\n", ret);
	return ret;
}

#include "drkMapleDevices.h"
#include "resource.h"

// Aight lets clean this up a bit, just a bit
#define DREAMCAST_INPUTS_NONE 0x8080000000000000 >> 16

u16 kcode[4] = { 0x0000,0x0000,0x0000,0x0000 };
u16 kcodeLastFrame[4] = { 0x0000,0x0000,0x0000,0x0000 };

// MultiThreaded Inputs for Digital Inputs Only (I.E: Arcade Stick)
u16 kcode_threaded[4] = { 0x0000,0x0000,0x0000,0x0000 };

s8 joyx[4] = { 0 }, joyy[4] = { 0 };
s8 joy2x[4] = { 0 }, joy2y[4] = { 0 };
u8 rt[4] = { 0 };
u8 lt[4] = { 0 };

SDL_GameController *joy[2];
int JoyToUse[2];
int deadzone[2];
int Peripheral[2];
bool RecordInput[2] = { false, false };
bool RecordInputHold[2] = { false, false }; // This is only here so it doesn't keep toggling every frame if you hold the button down for more than a frame

bool PlaybackInput[2] = { false, false };
bool PlaybackInputHold[2] = { false, false };

int PlaybackFrameCounter[2] = { 0,0 };

SDL_Event poll_event;
int FIXED_DEADZONE = 256;

enum ANALOG_INPUTS {
	ANALOG_UP = 0,
	ANALOG_DOWN = 1,
	ANALOG_LEFT = 2,
	ANALOG_RIGHT = 3,
	ANALOG_LT = 4,
	ANALOG_RT = 5,
};

enum NAOMI_KEYS
{
	NAOMI_SERVICE_KEY_1 = 1 << 0,
	NAOMI_TEST_KEY_1 = 1 << 1,
	NAOMI_SERVICE_KEY_2 = 1 << 2,
	NAOMI_TEST_KEY_2 = 1 << 3,
	NAOMI_START_KEY = 1 << 4,
	NAOMI_UP_KEY = 1 << 5,
	NAOMI_DOWN_KEY = 1 << 6,
	NAOMI_LEFT_KEY = 1 << 7,
	NAOMI_RIGHT_KEY = 1 << 8,
	NAOMI_BTN0_KEY = 1 << 9,
	NAOMI_BTN1_KEY = 1 << 10,
	NAOMI_BTN2_KEY = 1 << 11,
	NAOMI_BTN3_KEY = 1 << 12,
	NAOMI_BTN4_KEY = 1 << 13,
	NAOMI_BTN5_KEY = 1 << 14,
	NAOMI_COIN_KEY = 1 << 15,
	NAOMI_BC_03 = 0 << 0 | NAOMI_BTN0_KEY | NAOMI_BTN3_KEY,
	NAOMI_BC_14 = 0 << 0 | NAOMI_BTN1_KEY | NAOMI_BTN4_KEY,
	NAOMI_BC_25 = 0 << 0 | NAOMI_BTN2_KEY | NAOMI_BTN5_KEY,
	NAOMI_BC_012 = 0 << 0 | NAOMI_BTN0_KEY | NAOMI_BTN1_KEY | NAOMI_BTN2_KEY,
	NAOMI_BC_345 = 0 << 0 | NAOMI_BTN3_KEY | NAOMI_BTN4_KEY | NAOMI_BTN5_KEY,
	NAOMI_BC_01 = 0 << 0 | NAOMI_BTN0_KEY | NAOMI_BTN1_KEY,
	NAOMI_BC_12 = 0 << 0 | NAOMI_BTN1_KEY | NAOMI_BTN2_KEY,
	NAOMI_BC_34 = 0 << 0 | NAOMI_BTN3_KEY | NAOMI_BTN4_KEY,
	NAOMI_BC_45 = 0 << 0 | NAOMI_BTN4_KEY | NAOMI_BTN5_KEY,
	NAOMI_REC = 0 << 0, 
	NAOMI_PLAYBACK = 0 << 0
};

enum DREAMCAST_KEYS
{
	key_CONT_C = 1 << 0,
	key_CONT_B = 1 << 1,
	key_CONT_A = 1 << 2,
	key_CONT_START = 1 << 3,
	key_CONT_DPAD_UP = 1 << 4,
	key_CONT_DPAD_DOWN = 1 << 5,
	key_CONT_DPAD_LEFT = 1 << 6,
	key_CONT_DPAD_RIGHT = 1 << 7,
	key_CONT_Z = 1 << 8,
	key_CONT_Y = 1 << 9,
	key_CONT_X = 1 << 10,
	key_CONT_D = 1 << 11,
	key_CONT_DPAD2_UP = 1 << 12,
	key_CONT_DPAD2_DOWN = 1 << 13,
	key_CONT_DPAD2_LEFT = 1 << 14,
	key_CONT_DPAD2_RIGHT = 1 << 15,
	key_CONT_ANALOG_UP = 1 << 16,
	key_CONT_ANALOG_DOWN = 1 << 17,
	key_CONT_ANALOG_LEFT = 1 << 18,
	key_CONT_ANALOG_RIGHT = 1 << 19,
	key_CONT_LSLIDER = 1 << 20,
	key_CONT_RSLIDER = 1 << 21,
	key_CONT_BC_XA = 0 << 0 | key_CONT_X | key_CONT_A,
	key_CONT_BC_YB = 0 << 0 | key_CONT_Y | key_CONT_B,
	key_CONT_BC_CZ = 0 << 0 | key_CONT_C | key_CONT_Z,
	key_CONT_BC_ABC = 0 << 0 |  key_CONT_A | key_CONT_B | key_CONT_C,
	key_CONT_BC_XYZ = 0 << 0 | key_CONT_X | key_CONT_Y | key_CONT_Z,
	key_CONT_BC_AB = 0 << 0 | key_CONT_A | key_CONT_B,
	key_CONT_BC_BC = 0 << 0 | key_CONT_B | key_CONT_C,
	key_CONT_BC_XY = 0 << 0 | key_CONT_X | key_CONT_Y,
	key_CONT_BC_YZ = 0 << 0 | key_CONT_Y | key_CONT_Z,
	key_CONT_REC = 0 << 0,
	key_CONT_PLAYBACK = 0 << 0
};

struct _joypad_settings_entry
{
	wchar KC[8];
	u32 BIT;
	wchar* name;
};

_joypad_settings_entry joypad_settings[2][36];

#define D(x) x ,_T( #x)

#ifdef BUILD_NAOMI
_joypad_settings_entry joypad_settings_K[] =
{
	{ L"k115",D(NAOMI_SERVICE_KEY_1) }, // 0
	{ L"k116",D(NAOMI_TEST_KEY_1) }, // 1
	{ L"k115",D(NAOMI_SERVICE_KEY_2) }, // 2
	{ L"k116",D(NAOMI_TEST_KEY_2) }, // 3

	{ L"k53",D(NAOMI_START_KEY) }, // 4
	
	{ L"k87",D(NAOMI_UP_KEY) }, // 5
	{ L"k67",D(NAOMI_DOWN_KEY) }, // 6
	{ L"k83",D(NAOMI_LEFT_KEY) }, // 7
	{ L"k68",D(NAOMI_RIGHT_KEY) }, // 8

	{ L"k56",D(NAOMI_BTN0_KEY) }, // 9
	{ L"k57",D(NAOMI_BTN1_KEY) }, // 10
	{ L"k48",D(NAOMI_BTN2_KEY) }, // 11
	{ L"k85",D(NAOMI_BTN3_KEY) }, // 12
	{ L"k73",D(NAOMI_BTN4_KEY) }, // 13
	{ L"k79",D(NAOMI_BTN5_KEY) }, // 14
	{ L"k49",D(NAOMI_COIN_KEY) }, // 15

	{ L"k0",D(NAOMI_BC_03) }, // 16
	{ L"k0",D(NAOMI_BC_14) }, // 17
	{ L"k0",D(NAOMI_BC_25) }, // 18
	{ L"k0",D(NAOMI_BC_012) }, // 19
	{ L"k0",D(NAOMI_BC_345) }, // 20
	{ L"k0",D(NAOMI_BC_01) }, // 21
	{ L"k0",D(NAOMI_BC_12) }, // 22
	{ L"k0",D(NAOMI_BC_34) }, // 23
	{ L"k0",D(NAOMI_BC_45) }, // 24

	{ L"k38",D(NAOMI_REC) }, // 25
	{ L"k40",D(NAOMI_PLAYBACK) }, // 26
	{ 0,0,0 },

};

#else

_joypad_settings_entry joypad_settings_K[] =
{
	{L"k0",D(key_CONT_C)}, // 1
	{L"k73",D(key_CONT_B)}, // 2
	{L"k85",D(key_CONT_A)}, // 3
	{L"k53",D(key_CONT_START)}, // 4

	{L"k87",D(key_CONT_DPAD_UP)}, // 5
	{L"k83",D(key_CONT_DPAD_DOWN)}, // 6
	{L"k65",D(key_CONT_DPAD_LEFT)}, // 7
	{L"k68",D(key_CONT_DPAD_RIGHT)}, // 8

	{L"k0",D(key_CONT_Z)}, // 9
	{L"k57",D(key_CONT_Y)}, // 10
	{L"k56",D(key_CONT_X)}, // 11
	{L"k0",D(key_CONT_DPAD2_UP)}, // 12
	{L"k0",D(key_CONT_DPAD2_DOWN)}, // 13
	{L"k0",D(key_CONT_DPAD2_LEFT)}, // 14
	{L"k0",D(key_CONT_DPAD2_RIGHT)}, // 15

	{L"k87",D(key_CONT_ANALOG_UP)}, // 16
	{L"k83",D(key_CONT_ANALOG_DOWN)}, // 17
	{L"k65",D(key_CONT_ANALOG_LEFT)}, // 18
	{L"k68",D(key_CONT_ANALOG_RIGHT)}, // 19

	{L"k48",D(key_CONT_LSLIDER)}, // 20
	{L"k79",D(key_CONT_RSLIDER)}, // 21

	{ L"k0",D(key_CONT_BC_XA) }, // 22
	{ L"k0",D(key_CONT_BC_YB) }, // 23
	{ L"k0",D(key_CONT_BC_CZ) }, // 24

	{ L"k0",D(key_CONT_BC_ABC) }, // 25
	{ L"k0",D(key_CONT_BC_XYZ) }, // 26

	{ L"k0",D(key_CONT_BC_AB) }, // 27
	{ L"k0",D(key_CONT_BC_BC) }, // 28
	{ L"k0",D(key_CONT_BC_XY) }, // 29
	{ L"k0",D(key_CONT_BC_YZ) }, // 30
	{ L"k0",D(key_CONT_REC) }, // 31
	{ L"k0",D(key_CONT_PLAYBACK) }, // 32
	{0,0,0},
};

#endif
#undef D

u32 current_port = 0;
bool waiting_key = false;
u32 edited_key = 0;
u32 waiting_key_timer = 6 * 4;

#ifdef BUILD_NAOMI
// This is the name of the buttons in the button config dialog. they have to correspond to the buttons but IDC_ as a prefix
u32 kid_to_did[] =
{
	IDC_NAOMI_SERVICE_KEY_1,
	IDC_NAOMI_TEST_KEY_1,
	IDC_NAOMI_SERVICE_KEY_2,
	IDC_NAOMI_TEST_KEY_2,
	IDC_NAOMI_START_KEY,
	IDC_NAOMI_UP_KEY,
	IDC_NAOMI_DOWN_KEY,
	IDC_NAOMI_LEFT_KEY,
	IDC_NAOMI_RIGHT_KEY,
	IDC_NAOMI_BTN0_KEY,
	IDC_NAOMI_BTN1_KEY,
	IDC_NAOMI_BTN2_KEY,
	IDC_NAOMI_BTN3_KEY,
	IDC_NAOMI_BTN4_KEY,
	IDC_NAOMI_BTN5_KEY,
	IDC_NAOMI_COIN_KEY,

	IDC_NAOMI_BC_03,
	IDC_NAOMI_BC_14,
	IDC_NAOMI_BC_25,
	IDC_NAOMI_BC_012,
	IDC_NAOMI_BC_345,
	IDC_NAOMI_BC_01,
	IDC_NAOMI_BC_12,
	IDC_NAOMI_BC_34,
	IDC_NAOMI_BC_45,

	IDC_NAOMI_REC,
	IDC_NAOMI_PLAYBACK
};
#else
u32 kid_to_did[] =
{
	IDC_BUTTON1, // 1
	IDC_BUTTON2, // 2
	IDC_BUTTON3, // 3
	IDC_BUTTON4, // 4
	IDC_BUTTON5, // 5
	IDC_BUTTON6, // 6
	IDC_BUTTON7, // 7
	IDC_BUTTON8, // 8
	IDC_BUTTON9, // 9
	IDC_BUTTON10, // 10
	IDC_BUTTON11, // 11
	IDC_BUTTON12, // 12
	IDC_BUTTON13, // 13
	IDC_BUTTON14, // 14
	IDC_BUTTON15, // 15
	IDC_BUTTON16, // 16 UP ANALOG
	IDC_BUTTON17, // 17 DOWN ANALOG
	IDC_BUTTON18, // 18 LEFT ANALOG
	IDC_BUTTON19, // 19 RIGHT ANALOG
	IDC_BUTTON20, // 20 L
	IDC_BUTTON21, // 21 R

	IDC_BUTTON22, // 22 XA
	IDC_BUTTON23, // 23 YB
	IDC_BUTTON24, // 24 CZ
	IDC_BUTTON25, // 25 ABC
	IDC_BUTTON26, // 26 XYZ
	IDC_BUTTON27, // 26 AB
	IDC_BUTTON28, // 26 BC
	IDC_BUTTON29, // 26 XY
	IDC_BUTTON30, // 26 YZ
};
#endif

u8 kbs[256];
const u32 kbratio = 40;
void ENABLESHITFACE(HWND hWnd, u32 state)
{
	Static_Enable(hWnd, state);
	for (int kk = 0; joypad_settings_K[kk].name; kk++)
	{
		Static_Enable(GetDlgItem(hWnd, kid_to_did[kk]), state);
	}
}

int BEARKeyToInt(wchar _input[]) {
	wchar temp[4];
	for (int i = 1; i < 5; i++) { temp[i - 1] = _input[i]; }
	return _wtoi(temp);
}

void get_name(int VK, wchar* text)
{
	int scancode = MapVirtualKey(VK, 0);
	switch (VK) {
	case VK_INSERT:
	case VK_DELETE:
	case VK_HOME:
	case VK_END:
	case VK_NEXT:  // Page down
	case VK_PRIOR: // Page up
	case VK_LEFT:
	case VK_RIGHT:
	case VK_UP:
	case VK_DOWN:
		scancode |= 0x100; // Add extended bit
	}
	GetKeyNameText(scancode * 0x10000, text, 512);
}

void UpdateControllerNames(HWND hWnd, int current_port) {
	if (!SDL_GameControllerGetAttached(joy[current_port])) {
		Static_SetText(GetDlgItem(hWnd, IDC_CONT), L"None");
	}
	else {
		// ALL THIS SHIT JUST TO WRITE THE FOOKING CONTROLLER NAME
		const char *p = SDL_GameControllerName(joy[current_port]);
		const WCHAR *ControllerName; //LPCWSTR
		int size = MultiByteToWideChar(CP_ACP, 0, p, -1, NULL, 0);
		ControllerName = new WCHAR[128];
		MultiByteToWideChar(CP_ACP, 0, p, -1, (LPWSTR)ControllerName, size);

		Static_SetText(GetDlgItem(hWnd, IDC_CONT), ControllerName);
		delete[] ControllerName;
	}

}

bool axisIsNotInDeadzone(int _axis, int _port, wchar _dir = '+-')
{
	int deadzonetotal = (32767 * deadzone[_port]) / 100;

	if (_axis <= FIXED_DEADZONE && _axis >= -FIXED_DEADZONE) {
		return false;
	} // Fixed Idle Deadzone
	if (_dir == '+-') {
		if (_axis > deadzonetotal || _axis < -(deadzonetotal+1)) { 
			return true; 
		}
	}
	else {
		if ((_axis > deadzonetotal && _dir == '+') || (_axis < -(deadzonetotal+1) && _dir == '-')) { 
			return true; 
		}
	}


	return false;
}

void UpdateKeySelectionNames(HWND hWnd) {
	wchar temp[512];
	for (int i = 0; joypad_settings_K[i].name; i++) {
		if (kid_to_did[i] == 0) { continue; }
		if (joypad_settings[current_port][i].KC[0] == 'k') {
			if (BEARKeyToInt(joypad_settings[current_port][i].KC) == 0) {
				Button_SetText(GetDlgItem(hWnd, kid_to_did[i]), L"None");
			}
			else {
				get_name(BEARKeyToInt(joypad_settings[current_port][i].KC), temp);
				Button_SetText(GetDlgItem(hWnd, kid_to_did[i]), temp);
			}
		}
		else {
			Button_SetText(GetDlgItem(hWnd, kid_to_did[i]), joypad_settings[current_port][i].KC);
		}
	}
}

INT_PTR CALLBACK ConfigKeysDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		TCITEM tci;
		tci.mask = TCIF_TEXT | TCIF_IMAGE;
		tci.iImage = -1;
		tci.pszText = L"Player 1";
		TabCtrl_InsertItem(GetDlgItem(hWnd, IDC_PORTTAB), 0, &tci);
		tci.pszText = L"Player 2";
		TabCtrl_InsertItem(GetDlgItem(hWnd, IDC_PORTTAB), 1, &tci);

		TabCtrl_SetCurSel(GetDlgItem(hWnd, IDC_PORTTAB), current_port);

		SetTimer(hWnd, 0, 16, 0);
		Static_SetText(GetDlgItem(hWnd, IDC_STATUS), L"Click a button , then press the key you want to use for it.");

		UpdateControllerNames(hWnd, current_port);
		UpdateKeySelectionNames(hWnd);
	}
	return true;
	case WM_NOTIFY:
	{
		if (((LPNMHDR)lParam)->idFrom == IDC_PORTTAB &&
			((LPNMHDR)lParam)->code == TCN_SELCHANGE)
		{
			current_port = TabCtrl_GetCurSel(GetDlgItem(hWnd, IDC_PORTTAB));
			UpdateControllerNames(hWnd, current_port);
			UpdateKeySelectionNames(hWnd);
		}
		return true;
	}
	case WM_COMMAND:
		for (int i = 0; joypad_settings_K[i].name; i++)
		{
			if (kid_to_did[i] == LOWORD(wParam))
			{
				edited_key = i;
				GetKeyboardState(kbs);
				ENABLESHITFACE(hWnd, 0);
				waiting_key_timer = 6 * kbratio;
				waiting_key = true;
				return true;
			}
		}

		switch (LOWORD(wParam)) {
			break;
		case IDOK: {}
		case IDCANCEL:
			EndDialog(hWnd, 0);
			return true;
		default:
			break;
		}
		return false;

	case WM_TIMER:
	{
		wchar temp[512];
		// This means we're binding a key
		if (waiting_key)
		{
			// Keyboard Recognition
			wchar VK_down[8] = L"";
			wchar VK_Opposite[8] = L"";
			u8 temp_kbs[256];
			GetKeyboardState(temp_kbs);
			for (int i = 0; i < 256; i++) {
				if (temp_kbs[i] != kbs[i] && temp_kbs[i] != 0) {
					if (i == 27) {
						swprintf(VK_down, L"k%d", 0);
					}
					else {
						swprintf(VK_down, L"k%d", i);
					}

				}
			}

			// Joystick Recognition
			if (SDL_GameControllerGetAttached(joy[current_port])) { // Joystick is attached
				SDL_PumpEvents();
				while (SDL_PollEvent(&poll_event)) {

					if (SDL_GameControllerFromInstanceID(poll_event.cbutton.which) != joy[current_port]) { break; }

					switch (poll_event.type) {
					case SDL_CONTROLLERDEVICEREMOVED:
						UpdateControllerNames(hWnd, current_port);
						break;

					case SDL_CONTROLLERDEVICEADDED:
						UpdateControllerNames(hWnd, current_port);
						break;

					case SDL_CONTROLLERBUTTONDOWN:
						swprintf(VK_down, L"b%d", poll_event.cbutton.button);
						break;

					case SDL_CONTROLLERAXISMOTION:
						if (axisIsNotInDeadzone(poll_event.caxis.value, current_port))
						{
							if (poll_event.caxis.value > 0) {
								// Too lazy to come up with a clever way to do this so just gona do it manually
								if (wcscmp(joypad_settings_K[edited_key].name, L"key_CONT_ANALOG_UP") == 0) { // 16
									swprintf(VK_Opposite, L"a%d-", poll_event.caxis.axis);
									memcpy(joypad_settings[current_port][16].KC, VK_Opposite, sizeof(VK_Opposite));
								}
								if (wcscmp(joypad_settings_K[edited_key].name, L"key_CONT_ANALOG_DOWN") == 0) {
									swprintf(VK_Opposite, L"a%d-", poll_event.caxis.axis);
									memcpy(joypad_settings[current_port][15].KC, VK_Opposite, sizeof(VK_Opposite));
								}
								if (wcscmp(joypad_settings_K[edited_key].name, L"key_CONT_ANALOG_LEFT") == 0) {
									swprintf(VK_Opposite, L"a%d-", poll_event.caxis.axis);
									memcpy(joypad_settings[current_port][18].KC, VK_Opposite, sizeof(VK_Opposite));
								}
								if (wcscmp(joypad_settings_K[edited_key].name, L"key_CONT_ANALOG_RIGHT") == 0) {
									swprintf(VK_Opposite, L"a%d-", poll_event.caxis.axis);
									memcpy(joypad_settings[current_port][17].KC, VK_Opposite, sizeof(VK_Opposite));
								}
								swprintf(VK_down, L"a%d+", poll_event.caxis.axis);
								break;
							}
							else {
								if (wcscmp(joypad_settings_K[edited_key].name, L"key_CONT_ANALOG_UP") == 0) { // 16
									swprintf(VK_Opposite, L"a%d+", poll_event.caxis.axis);
									memcpy(joypad_settings[current_port][16].KC, VK_Opposite, sizeof(VK_Opposite));
								}
								if (wcscmp(joypad_settings_K[edited_key].name, L"key_CONT_ANALOG_DOWN") == 0) {
									swprintf(VK_Opposite, L"a%d+", poll_event.caxis.axis);
									memcpy(joypad_settings[current_port][15].KC, VK_Opposite, sizeof(VK_Opposite));
								}
								if (wcscmp(joypad_settings_K[edited_key].name, L"key_CONT_ANALOG_LEFT") == 0) {
									swprintf(VK_Opposite, L"a%d+", poll_event.caxis.axis);
									memcpy(joypad_settings[current_port][18].KC, VK_Opposite, sizeof(VK_Opposite));
								}
								if (wcscmp(joypad_settings_K[edited_key].name, L"key_CONT_ANALOG_RIGHT") == 0) {
									swprintf(VK_Opposite, L"a%d+", poll_event.caxis.axis);
									memcpy(joypad_settings[current_port][17].KC, VK_Opposite, sizeof(VK_Opposite));
								}
								swprintf(VK_down, L"a%d-", poll_event.caxis.axis);
								break;
							}
						}
					}
				}
			}

			if (wcscmp(VK_down, L"") != 0) {
				waiting_key = false;
				swprintf_s(temp, L"Updated Key Mapping,%s", VK_down);
				Static_SetText(GetDlgItem(hWnd, IDC_STATUS), temp);
				memcpy(joypad_settings[current_port][edited_key].KC, VK_down, sizeof(VK_down));
				SaveSettings();
				UpdateKeySelectionNames(hWnd);
				SDL_PumpEvents();
				SDL_FlushEvent(SDL_CONTROLLERAXISMOTION);
				SDL_FlushEvent(SDL_CONTROLLERBUTTONDOWN);
			}
		}

		if (waiting_key) {
			wchar temp[512];
			waiting_key_timer--;
			if (waiting_key_timer == 0) {
				Static_Enable(hWnd, 1);
				for (int kk = IDC_BUTTON1; kk < (IDC_BUTTON1 + 16); kk++) {
					Static_Enable(GetDlgItem(hWnd, kk), 1);
				}
				waiting_key = false;
				waiting_key_timer = 6;

				swprintf_s(temp, L"Timed out while waiting for new key", waiting_key_timer / kbratio);
				Static_SetText(GetDlgItem(hWnd, IDC_STATUS), temp);
			}
			else {
				swprintf_s(temp, L"Waiting for key ...%d\n", waiting_key_timer / kbratio);
				Static_SetText(GetDlgItem(hWnd, IDC_STATUS), temp);
			}
		}

		if (!waiting_key) {
			ENABLESHITFACE(hWnd, 1);
		}
		GetKeyboardState(kbs);
	}
	return true;

	case WM_CLOSE:
	case WM_DESTROY:
		KillTimer(hWnd, 0);
		EndDialog(hWnd, 0);
		return true;

	default: break;
	}

	return false;
}

// BEAR STUFF STARTS HERE
int FrameCount = 0;
int LastSpectatorFrameRecieved = 0; // This is the last frame you got as a spectator

wchar BEARPlay_Host[32] = L"127.0.0.1";
wchar BEARPlay_P1Name[256] = L"Player 1";
wchar BEARPlay_P2Name[256] = L"Player 2";
wchar BEARPlay_File[512] = L"0";
wchar BEARPlay_GameName[256] = L"0";
wchar BEARPlay_GameRom[256] = L"0";
wchar BEARPlay_Region[4] = L"JPN";

char BEARPlay_RecordingFileName[512];
wchar BEARPlay_SessionTimeStamp[128];

int BEARPlay_Hosting = 0;
int BEARPlay_Port = 27886;
int BEARPlay_Delay = 1;
int BEARPlay_Online = 0;
int BEARPlay_Record = 0;
int BEARPlay_Playback = 0;
int BEARPlay_AllowSpectators = 0;
int BEARPlay_Spectator = 0;

int BEARPlay_debug = 0;

bool RecordingRepay = false;
int LastRecordedFrame = 0;

TCPsocket BEARListener;
IPaddress BEARListener_ip;
IPaddress BEARSocket_ip;

//TCPsocket BEARSocket;
bool Connected = false;
bool Disconnected = false;
bool FastForwardDown = false;

#ifdef BUILD_NAOMI
std::map<int, u16> SelfInputs;
std::map<int, u16> OpponentInputs;
std::map<int, u16> RecordedInputs[2];

#else
std::map<int, u64> SelfInputs;
std::map<int, u64> OpponentInputs;
std::map<int, u64> RecordedInputs[2];

#endif

int BEARPlay_ConnectionCount = 0;
std::map<int, TCPsocket> BEARPlay_ActiveConnection;
std::map<int, int> SpectatorFrameCounter;

template<typename T>
void print_bits(const T& a)
{
	const char* beg = reinterpret_cast<const char*>(&a);
	const char* end = beg + sizeof(a);
	while (beg != end)
		std::cout << std::bitset<CHAR_BIT>(*beg++) << ' ';
	std::cout << '\n';

}

void BearConnect() // Aight, second try on this one
{
	Debug_WriteLine("++BEARPlay: Starting Bearplay\n");

	// Check if we shuold create a listener
	if (BEARPlay_Playback == 0) { // Not Replay

		if (SDLNet_Init() == -1) { Debug_WriteLine("Coulnd't start SDL_Net\n"); ErrorMessageBoxPopUp(L"Couldn't start SDL_Net"); exit(0); }

		// Check if we should join someone
		if (BEARPlay_Spectator == 1 || (BEARPlay_Hosting == 0 && BEARPlay_Online == 1)) { // We're a client
			Debug_WriteLine("++BEARPlay: Connecting...\n");

			while (!BEARPlay_ActiveConnection[BEARPlay_ConnectionCount]) {
				if (SDLNet_ResolveHost(&BEARSocket_ip, _bstr_t(BEARPlay_Host), BEARPlay_Port) == -1) {
					Debug_WriteLine("++BEARPlay: Retrying...\n"); SDL_Delay(100); continue;
				}

				BEARPlay_ActiveConnection[BEARPlay_ConnectionCount] = SDLNet_TCP_Open(&BEARSocket_ip);
				if (!BEARPlay_ActiveConnection[BEARPlay_ConnectionCount]) {
					Debug_WriteLine("++BEARPlay: Retrying...\n"); SDL_Delay(100); continue;
				}
			}

			if (BEARPlay_Spectator == 1) { // We're spectating
				std::thread clt(&BEARSpectatorReceiver); clt.detach();
			}
			else { // We're a normal client
				std::thread clt(&BEAReceiver); clt.detach();
			}
			Connected = true;
			BEARPlay_ConnectionCount++;
		}

		if (BEARPlay_Online == 0) {
			printf("++BEARPlay: Started in OFFLINE mode\n");
			Connected = true;
		}

		// Start up the listener if needs be.
		if (BEARPlay_AllowSpectators == 1 || (BEARPlay_Hosting == 1 && BEARPlay_Online == 1)) {
			if (BEARPlay_Spectator == 0) { // Don't start a listender if we're spectating

				if (BEARPlay_Hosting == 1) { Debug_WriteLine("++BEARPlay: Hosting...\n"); }
				else { Debug_WriteLine("++BEARPlay: Spectator Host Started...\n"); }

				if (SDLNet_ResolveHost(&BEARListener_ip, NULL, BEARPlay_Port) == -1) {
					Debug_WriteLine("++BEARPlay: Couldn't resolve host\n"); ErrorMessageBoxPopUp(L"Couldn't resolve host"); exit(0);
				}

				BEARListener = SDLNet_TCP_Open(&BEARListener_ip);
				if (!BEARListener) {
					Debug_WriteLine("++BEARPlay: Couldn't open port\n"); ErrorMessageBoxPopUp(L"Couldn't open port"); exit(0);
				}

				while (true) {

					BEARPlay_ActiveConnection[BEARPlay_ConnectionCount] = SDLNet_TCP_Accept(BEARListener);
					if (!BEARPlay_ActiveConnection[BEARPlay_ConnectionCount]) { SDL_Delay(100); continue; }

					if (Connected == false) { // We a host
						std::thread clt(&BEAReceiver); clt.detach();
						Debug_WriteLine("++BEARPlay: Client Connected\n");
						Connected = true;
						if (BEARPlay_AllowSpectators == 0) { break; }
					}
					else
					{
						SpectatorFrameCounter[BEARPlay_ConnectionCount] = 0;
						Debug_WriteLine("++BEARPlay: Spectator Connected: %d \n", BEARPlay_ConnectionCount);
						std::thread clt(&BEARSpectatorSender, BEARPlay_ConnectionCount); clt.detach();
					}
					BEARPlay_ConnectionCount++;
				}
			}
		}
	}
	else { // This is a replay, so just set connected to true
		Connected = true;
	}
}

void ErrorMessageBoxPopUp(wchar* msg)
{
	int msgboxID = MessageBox(
		NULL,
		msg,
		(wchar*)L"BEARPLAY",
		MB_ICONWARNING | MB_OK | MB_DEFBUTTON2
	);
}

void BEARSpectatorReceiver() {

	while (true)
	{
#ifdef BUILD_NAOMI
		unsigned char bytes[16];
		int result = SDLNet_TCP_Recv(BEARPlay_ActiveConnection[0], &bytes, 16); // Spectators only have 1 connection, the host.

#else
		unsigned char bytes[40];
		int result = SDLNet_TCP_Recv(BEARPlay_ActiveConnection[0], &bytes, 40); // Spectators only have 1 connection, the host.

#endif
		if (result <= 0) {
			ErrorMessageBoxPopUp(L"BEARPlay: Connection Lost(SRR)");
			Debug_WriteLine("++BEARPlay: Connection Lost(SRR)\n");
			exit(0);
			Disconnected = true;
			break;
		}

#ifdef BUILD_NAOMI
		int totalBytes = 8;
#else
		int totalBytes = 20;
#endif // BUILD_NAOMI


		for (int i = 0; i < 2; i++) {
			int FrameNumber = int(
				(unsigned char)(bytes[0 + (i * totalBytes)]) << 24 |
				(unsigned char)(bytes[1 + (i * totalBytes)]) << 16 |
				(unsigned char)(bytes[2 + (i * totalBytes)]) << 8 |
				(unsigned char)(bytes[3 + (i * totalBytes)]));

			//Debug_WriteLine("Recieved Frame: %d\n", FrameNumber);
#ifdef BUILD_NAOMI
			SelfInputs[FrameNumber] = 
				bytes[5 + (i * totalBytes)] |
				(bytes[4 + (i * totalBytes)] << 8);
			OpponentInputs[FrameNumber] = 
				bytes[7 + (i * totalBytes)] |
				(bytes[6 + (i * totalBytes)] << 8);
#else
			SelfInputs[FrameNumber] =
				(u64)bytes[4 + (i * totalBytes)] |
				((u64)bytes[5 + (i * totalBytes)] << 8) |
				((u64)bytes[6 + (i * totalBytes)] << 16) |
				((u64)bytes[7 + (i * totalBytes)] << 24) |
				((u64)bytes[8 + (i * totalBytes)] << 32) |
				((u64)bytes[9 + (i * totalBytes)] << 40) |
				((u64)bytes[10 + (i * totalBytes)] << 48) |
				((u64)bytes[11 + (i * totalBytes)] << 56);
			/**/
			OpponentInputs[FrameNumber] =
				(u64)bytes[12 + (i * totalBytes)] |
				((u64)bytes[13 + (i * totalBytes)] << 8) |
				((u64)bytes[14 + (i * totalBytes)] << 16) |
				((u64)bytes[15 + (i * totalBytes)] << 24) |
				((u64)bytes[16 + (i * totalBytes)] << 32) |
				((u64)bytes[17 + (i * totalBytes)] << 40) |
				((u64)bytes[18 + (i * totalBytes)] << 48) |
				((u64)bytes[19 + (i * totalBytes)] << 56);

#endif // BUILD_NAOMI
			//printf("Raw:");
			//print_bits(bytes);
			//printf("p1: ");
			//print_bits(SelfInputs[FrameNumber]);
			//printf("p2: ");
			//print_bits(OpponentInputs[FrameNumber]);
			LastSpectatorFrameRecieved = FrameNumber;
		}
	}
}

void BEARSpectatorDataSenderReciever(int SpectatorID)
{
	Debug_WriteLine("++BEARPlay: Spectator Reciever Thread Created: %d\n", SpectatorID);

	while (true)
	{
		unsigned char bytes[4];
		int result = SDLNet_TCP_Recv(BEARPlay_ActiveConnection[SpectatorID], &bytes, 4);
		if (result <= 0) {
			Debug_WriteLine("++BEARPlay: Couldn't send frame data to spectator: %d\n", SpectatorID);
			break;
		}

		int FrameNumber = int(
			(unsigned char)(bytes[0]) << 24 |
			(unsigned char)(bytes[1]) << 16 |
			(unsigned char)(bytes[2]) << 8 |
			(unsigned char)(bytes[3]));

		if (FrameNumber < 10) {
			SpectatorFrameCounter[SpectatorID] = 0;
		}
		else {
			SpectatorFrameCounter[SpectatorID] = FrameNumber - 10;
		}

		//Debug_WriteLine("Resending Frame Data Starting From: %x\n", SpectatorFrameCounter[SpectatorID]);
		//printf("Resending Frame Data Starting From: %d\n", FrameNumber-1);

	}
}

void BEARSpectatorSender(int SpectatorID) {

	Debug_WriteLine("++BEARPlay: Spectator Thread Created: %d\n", SpectatorID);
	// Just keep track of which frame you should send tot he spectator next
	// int SpectatorFrameToSend = 0;

	// Reciever, this is only ever used if the person loses a frame
	std::thread spec(&BEARSpectatorDataSenderReciever, SpectatorID);
	spec.detach();

	//int FramesToSend = 0;

	while (true) {

#ifdef BUILD_NAOMI
		int totalBytes = 8;
		unsigned char bytes[16]; // 0-3 Frame Number. 4-5 P1 6-7 P2

#else
		int totalBytes = 20;
		unsigned char bytes[40]; // 0-3 Frame Number. 4-9 P1 10-15 P2
#endif // BUILD_NAOMI

		while (true) { // + 1
			if (OpponentInputs.find(SpectatorFrameCounter[SpectatorID]) == OpponentInputs.end() ||
				SelfInputs.find(SpectatorFrameCounter[SpectatorID] + 1) == SelfInputs.end()) {
				SDL_Delay(16);
				continue;
			} else if (OpponentInputs.find(SpectatorFrameCounter[SpectatorID] + 1) == OpponentInputs.end() ||
				SelfInputs.find(SpectatorFrameCounter[SpectatorID] + 2) == SelfInputs.end()) {
				SDL_Delay(16);
				continue;
			}

			break;
		}

#ifdef BUILD_NAOMI
		for (int i = 0; i < 2; i++) {

			bytes[0 + (i * totalBytes)] = ((SpectatorFrameCounter[SpectatorID] + i) >> 24) & 0xFF;
			bytes[1 + (i * totalBytes)] = ((SpectatorFrameCounter[SpectatorID] + i) >> 16) & 0xFF;
			bytes[2 + (i * totalBytes)] = ((SpectatorFrameCounter[SpectatorID] + i) >> 8) & 0xFF;
			bytes[3 + (i * totalBytes)] = (SpectatorFrameCounter[SpectatorID] + i) & 0xFF;

			if (BEARPlay_Hosting == 1 || BEARPlay_Online == 0) { // 16 bits total 2 bytes
				bytes[4 + (i * totalBytes)] = (SelfInputs[SpectatorFrameCounter[SpectatorID] + i] >> 8) & 0xFF;
				bytes[5 + (i * totalBytes)] = SelfInputs[SpectatorFrameCounter[SpectatorID] + i] & 0xFF;

				bytes[6 + (i * totalBytes)] = (OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] >> 8) & 0xFF;
				bytes[7 + (i * totalBytes)] = OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] & 0xFF;
			}
			else {
				bytes[4 + (i * totalBytes)] = (OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] >> 8) & 0xFF;
				bytes[5 + (i * totalBytes)] = OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] & 0xFF;

				bytes[6 + (i * totalBytes)] = (SelfInputs[SpectatorFrameCounter[SpectatorID] + i] >> 8) & 0xFF;
				bytes[7 + (i * totalBytes)] = SelfInputs[SpectatorFrameCounter[SpectatorID] + i] & 0xFF;
			}

		}
#else
		for (int i = 0; i < 2; i++) {

			bytes[0 + (i * totalBytes)] = ((SpectatorFrameCounter[SpectatorID] + i) >> 24) & 0xFF;
			bytes[1 + (i * totalBytes)] = ((SpectatorFrameCounter[SpectatorID] + i) >> 16) & 0xFF;
			bytes[2 + (i * totalBytes)] = ((SpectatorFrameCounter[SpectatorID] + i) >> 8) & 0xFF;
			bytes[3 + (i * totalBytes)] = (SpectatorFrameCounter[SpectatorID] + i) & 0xFF;

			if (BEARPlay_Hosting == 1 || BEARPlay_Online == 0) { // 48 bits total 6 bytes
				bytes[4 + (i * totalBytes)] = (SelfInputs[SpectatorFrameCounter[SpectatorID] + i] >> 0) & 0xFF;
				bytes[5 + (i * totalBytes)] = (SelfInputs[SpectatorFrameCounter[SpectatorID] + i] >> 8) & 0xFF;
				bytes[6 + (i * totalBytes)] = (SelfInputs[SpectatorFrameCounter[SpectatorID] + i] >> 16) & 0xFF;
				bytes[7 + (i * totalBytes)] = (SelfInputs[SpectatorFrameCounter[SpectatorID] + i] >> 24) & 0xFF;
				bytes[8 + (i * totalBytes)] = (SelfInputs[SpectatorFrameCounter[SpectatorID] + i] >> 32) & 0xFF;
				bytes[9 + (i * totalBytes)] = (SelfInputs[SpectatorFrameCounter[SpectatorID] + i] >> 40) & 0xFF;
				bytes[10 + (i * totalBytes)] = (SelfInputs[SpectatorFrameCounter[SpectatorID] + i] >> 48) & 0xFF;
				bytes[11 + (i * totalBytes)] = (SelfInputs[SpectatorFrameCounter[SpectatorID] + i] >> 56) & 0xFF;

				bytes[12 + (i * totalBytes)] = (OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] >> 0) & 0xFF;
				bytes[13 + (i * totalBytes)] = (OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] >> 8) & 0xFF;
				bytes[14 + (i * totalBytes)] = (OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] >> 16) & 0xFF;
				bytes[15 + (i * totalBytes)] = (OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] >> 24) & 0xFF;
				bytes[16 + (i * totalBytes)] = (OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] >> 32) & 0xFF;
				bytes[17 + (i * totalBytes)] = (OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] >> 40) & 0xFF;
				bytes[18 + (i * totalBytes)] = (OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] >> 48) & 0xFF;
				bytes[19 + (i * totalBytes)] = (OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] >> 56) & 0xFF;
			}
			else {
				bytes[4 + (i * totalBytes)] = (OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] >> 0) & 0xFF;
				bytes[5 + (i * totalBytes)] = (OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] >> 8) & 0xFF;
				bytes[6 + (i * totalBytes)] = (OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] >> 16) & 0xFF;
				bytes[7 + (i * totalBytes)] = (OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] >> 24) & 0xFF;
				bytes[8 + (i * totalBytes)] = (OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] >> 32) & 0xFF;
				bytes[9 + (i * totalBytes)] = (OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] >> 40) & 0xFF;
				bytes[10 + (i * totalBytes)] = (OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] >> 48) & 0xFF;
				bytes[11 + (i * totalBytes)] = (OpponentInputs[SpectatorFrameCounter[SpectatorID] + i] >> 56) & 0xFF;

				bytes[12 + (i * totalBytes)] = (SelfInputs[SpectatorFrameCounter[SpectatorID] + i] >> 0) & 0xFF;
				bytes[13 + (i * totalBytes)] = (SelfInputs[SpectatorFrameCounter[SpectatorID] + i] >> 8) & 0xFF;
				bytes[14 + (i * totalBytes)] = (SelfInputs[SpectatorFrameCounter[SpectatorID] + i] >> 16) & 0xFF;
				bytes[15 + (i * totalBytes)] = (SelfInputs[SpectatorFrameCounter[SpectatorID] + i] >> 24) & 0xFF;
				bytes[16 + (i * totalBytes)] = (SelfInputs[SpectatorFrameCounter[SpectatorID] + i] >> 32) & 0xFF;
				bytes[17 + (i * totalBytes)] = (SelfInputs[SpectatorFrameCounter[SpectatorID] + i] >> 40) & 0xFF;
				bytes[18 + (i * totalBytes)] = (SelfInputs[SpectatorFrameCounter[SpectatorID] + i] >> 48) & 0xFF;
				bytes[19 + (i * totalBytes)] = (SelfInputs[SpectatorFrameCounter[SpectatorID] + i] >> 56) & 0xFF;
			}

			//printf("p1: ");
			//print_bits(SelfInputs[SpectatorFrameCounter[SpectatorID] + i]);
			//printf("p2: ");
			//print_bits(OpponentInputs[SpectatorFrameCounter[SpectatorID] + i]);
		}

#endif // BUILD_NAOMI

		//Debug_WriteLine("Sending Spectator %d Data: %d\n", SpectatorID, SpectatorFrameCounter[SpectatorID]);
		int result2 = SDLNet_TCP_Send(BEARPlay_ActiveConnection[SpectatorID], bytes, sizeof(bytes));
		if (result2 < sizeof(bytes)) { break; }

		SpectatorFrameCounter[SpectatorID] += 2;
		SDL_Delay(16); // wait 16ms before sending any more frames, i mean we don't really need to send them all the frames that quickly, so when spectator joins we don't get any lag spikes
	}

	Debug_WriteLine("Spectator disconnected. ID: %d\n", SpectatorID);
	SDLNet_TCP_Close(BEARPlay_ActiveConnection[SpectatorID]); // Cleanup
}

void BEAReceiver()
{
	// Getting Data From Client
	while (true) {
		/* read the buffer from client */

#ifdef BUILD_NAOMI
		unsigned char bytes[6];
		int result = SDLNet_TCP_Recv(BEARPlay_ActiveConnection[0], &bytes, 6);
#else
		unsigned char bytes[12];
		int result = SDLNet_TCP_Recv(BEARPlay_ActiveConnection[0], &bytes, 12);
#endif

		if (result <= 0) {
			ErrorMessageBoxPopUp(L"Connection Lost(BRR)");
			Debug_WriteLine("++BEARPlay: Connection Lost(BRR)\n");
			Disconnected = true;
			exit(0);
			break;
		}

		int FrameNumber = int((u32)bytes[0] | (u32)bytes[1] << 8 | (u32)bytes[2] << 16 | (u32)bytes[3] << 24);

#ifdef BUILD_NAOMI
		u16 inputs = bytes[5] |
			(bytes[4] << 8);
#else
		u64 inputs = (u64)bytes[4] |
			((u64)bytes[5] << 8) |
			((u64)bytes[6] << 16) |
			((u64)bytes[7] << 24) |
			((u64)bytes[8] << 32) |
			((u64)bytes[9] << 40) |
			((u64)bytes[10] << 48) |
			((u64)bytes[11] << 56);
#endif

		if (FrameNumber > FrameCount + (BEARPlay_Delay * 2) + 1) { printf("Possible Desync: %d/%d \n", FrameCount, FrameNumber); }

		if (BEARPlay_debug == 2)
		{
			Debug_WriteLine("++BEARPlay: Recieved Frame: %d \n", FrameNumber);
			//print_bits(inputs);
		}

		OpponentInputs[FrameNumber] = inputs;
	}

}

u64 combineDreamcastInputData(u16 MainInputData, u8 LT, u8 RT, u8 JoyX, u8 JoyY)
{
	u64 ReturnData = MainInputData |
		(u64)LT << 16 |
		(u64)RT << 24 |
		(u64)JoyX << 32 |
		(u64)JoyY << 40;

	//print_bits(ReturnData);
	return ReturnData;
}

void BEARnet_Naomi(u16 &Player1Kode, u16 &Player2Kode) 
{

	unsigned char bytes[6];
	u16 temp_inputs_self;
	memcpy(&temp_inputs_self, &Player1Kode, sizeof(Player1Kode));

	u32 FramePlusDelay = FrameCount + BEARPlay_Delay;
	bytes[0] = (u32)FramePlusDelay & 0xFF;
	bytes[1] = (u32)(FramePlusDelay >> 8) & 0xFF;
	bytes[2] = (u32)(FramePlusDelay >> 16) & 0xFF;
	bytes[3] = (u32)(FramePlusDelay >> 24) & 0xFF;
	bytes[4] = (temp_inputs_self >> 8) & 0xFF;
	bytes[5] = temp_inputs_self & 0xFF;
	SelfInputs[FramePlusDelay] = temp_inputs_self;

	int result = SDLNet_TCP_Send(BEARPlay_ActiveConnection[0], bytes, sizeof(bytes));

	if (result <= 0) {
		ErrorMessageBoxPopUp(L"Connection Lost(BNS)");
		Debug_WriteLine("++BEARPlay: Connection Lost(BNS)\n");
		Disconnected = true;
		exit(0);
	}

	int WaitingForFrameTime = 0;
	while (true) {
		// DO NOT USE .count that shit breaks easily
		if (OpponentInputs.find(FrameCount) == OpponentInputs.end()) {
			// Sometimes if we get them out of order, the value will exist but it'll be 0.
			if (Disconnected) { break; };
			if (FrameCount < BEARPlay_Delay) { break; }
			
			WaitingForFrameTime++;
			if (WaitingForFrameTime > 3000) {
				u32 FrameMinusDelay = FrameCount - BEARPlay_Delay - 1;
				Debug_WriteLine("++Waiting for frame data...\n");

				bytes[0] = (u32)FrameMinusDelay & 0xFF;
				bytes[1] = (u32)(FrameMinusDelay >> 8) & 0xFF;
				bytes[2] = (u32)(FrameMinusDelay >> 16) & 0xFF;
				bytes[3] = (u32)(FrameMinusDelay >> 24) & 0xFF;
				bytes[4] = (SelfInputs[FrameMinusDelay] >> 8) & 0xFF;
				bytes[5] = SelfInputs[FrameMinusDelay] & 0xFF;

				int result = SDLNet_TCP_Send(BEARPlay_ActiveConnection[0], bytes, sizeof(bytes));

				if (result < sizeof(bytes)) {
					ErrorMessageBoxPopUp(L"Connection Lost(BNS)");
					Debug_WriteLine("++BEARPlay: Connection Lost(BNS)\n");
					Disconnected = true;
					exit(0);
				}

				WaitingForFrameTime = 0;
			}
			SDL_Delay(1);
		}
		else { break; }
	}

	if (FrameCount < BEARPlay_Delay)
	{
		SelfInputs[FrameCount] = 0x0000;
		OpponentInputs[FrameCount] = 0x0000;
		Player1Kode = 0x0000;
		Player2Kode = 0x0000;

	} else {
		
		if (BEARPlay_Hosting == 1) {
			Player1Kode = SelfInputs[FrameCount];
			Player2Kode = OpponentInputs[FrameCount];
		}
		else {
			Player1Kode = OpponentInputs[FrameCount];
			Player2Kode = SelfInputs[FrameCount];
		}
	}
}


#ifdef BUILD_DREAMCAST
void BEARnet_Dreamcast() // Dreamcast
{
	unsigned char bytes[12];
	u64 temp_inputs_self = combineDreamcastInputData(kcode[0], lt[0], rt[0], GetBtFromSgn(joyx[0]), GetBtFromSgn(joyy[0]));
	u32 FramePlusDelay = FrameCount + BEARPlay_Delay;

	bytes[0] = (u32)FramePlusDelay & 0xFF;
	bytes[1] = (u32)(FramePlusDelay >> 8) & 0xFF;
	bytes[2] = (u32)(FramePlusDelay >> 16) & 0xFF;
	bytes[3] = (u32)(FramePlusDelay >> 24) & 0xFF;
	// u64
	bytes[4] = temp_inputs_self & 0xFF;
	bytes[5] = (temp_inputs_self >> 8) & 0xFF;
	bytes[6] = (temp_inputs_self >> 16) & 0xFF;
	bytes[7] = (temp_inputs_self >> 24) & 0xFF;
	bytes[8] = (temp_inputs_self >> 32) & 0xFF;
	bytes[9] = (temp_inputs_self >> 40) & 0xFF;
	bytes[10] = (temp_inputs_self >> 48) & 0xFF;
	bytes[11] = (temp_inputs_self >> 56) & 0xFF;
	SelfInputs[FramePlusDelay] = temp_inputs_self;

	int result = SDLNet_TCP_Send(BEARPlay_ActiveConnection[0], bytes, sizeof(bytes));

	if (result < sizeof(bytes)) {
		ErrorMessageBoxPopUp(L"Connection Lost(BNS)");
		Debug_WriteLine("++BEARPlay: Connection Lost(BNS)\n");
		exit(0);
		Disconnected = true;
	}

	int WaitingForFrameTime = 0;
	while (true) {
		// DO NOT USE .count that shit breaks easily
		if (OpponentInputs.find(FrameCount) == OpponentInputs.end()) { // Check if the current frame can be found
			if (Disconnected) { break; };
			if (FrameCount < BEARPlay_Delay) { break; }

			WaitingForFrameTime++;
			if (WaitingForFrameTime > 3000) {
				int FrameMinusDelay = FrameCount - BEARPlay_Delay - 1;
				Debug_WriteLine("++Waiting for frame data...\n");

				bytes[0] = (u32)FrameMinusDelay & 0xFF;
				bytes[1] = (u32)(FrameMinusDelay >> 8) & 0xFF;
				bytes[2] = (u32)(FrameMinusDelay >> 16) & 0xFF;
				bytes[3] = (u32)(FrameMinusDelay >> 24) & 0xFF;
				// u64
				bytes[4] = SelfInputs[FrameMinusDelay] & 0xFF;
				bytes[5] = (SelfInputs[FrameMinusDelay] >> 8) & 0xFF;
				bytes[6] = (SelfInputs[FrameMinusDelay] >> 16) & 0xFF;
				bytes[7] = (SelfInputs[FrameMinusDelay] >> 24) & 0xFF;
				bytes[8] = (SelfInputs[FrameMinusDelay] >> 32) & 0xFF;
				bytes[9] = (SelfInputs[FrameMinusDelay] >> 40) & 0xFF;
				bytes[10] = (SelfInputs[FrameMinusDelay] >> 48) & 0xFF;
				bytes[11] = (SelfInputs[FrameMinusDelay] >> 56) & 0xFF;

				int result = SDLNet_TCP_Send(BEARPlay_ActiveConnection[0], bytes, sizeof(bytes));

				if (result < sizeof(bytes)) {
					ErrorMessageBoxPopUp(L"Connection Lost(BNS)");
					Debug_WriteLine("++BEARPlay: Connection Lost(BNS)\n");
					exit(0);
					Disconnected = true;
				}

				WaitingForFrameTime = 0;
			}
			SDL_Delay(1);

		}
		else { break; }
	}

	if (FrameCount < BEARPlay_Delay) {
		SelfInputs[FrameCount] = DREAMCAST_INPUTS_NONE;
		OpponentInputs[FrameCount] = DREAMCAST_INPUTS_NONE;
	}
}

#endif // BUILD_DREAMCAST

void GenerateRecordingFileName()
{
	// Clear the FileName just in case.
	memset(BEARPlay_RecordingFileName, 0, sizeof(BEARPlay_RecordingFileName));

	// Create the first past of the name P1 vs P2
	char tmp1[256];
	sprintf(tmp1, "replays/%S vs %S %S \0", BEARPlay_P1Name, BEARPlay_P2Name, BEARPlay_GameName);

	time_t curr_time;
	tm * curr_tm;
	char tmp2[128];
	time(&curr_time);
	char cur_timestamp[128];
	curr_tm = localtime(&curr_time);
	strftime(tmp2, sizeof(tmp2), "(%m %d %I-%M%p)\0", curr_tm);
	strftime(cur_timestamp, sizeof(cur_timestamp), "%m %d %I-%M%p\0", curr_tm);
	mbstowcs(BEARPlay_SessionTimeStamp, cur_timestamp, sizeof(cur_timestamp));

	int i = 0, o = 0;
	while (tmp1[i] != '\0') { BEARPlay_RecordingFileName[o] = tmp1[i]; i++; o++; } i = 0;
	while (tmp2[i] != '\0') { BEARPlay_RecordingFileName[o] = tmp2[i]; i++; o++; } i = 0;
	char tmp3[12] = ".bearplay2";
	while (tmp3[i] != '\0') { BEARPlay_RecordingFileName[o] = tmp3[i]; i++; o++; }

	//Debug_WriteLine("Record File Name: %s\n\n\n", BEARPlay_RecordingFileName);
}

void hexdump(void *ptr, int buflen, char *dump_to) {
	char eeprom_dump[256] = { 0 };

	char* tmp_char = new char[2];

	unsigned char *buf = (unsigned char*)ptr;
	int i, j;
	for (i = 0; i < buflen; i += 16) {
		for (j = 0; j < 16; j++) {
			if (i + j < buflen) {
				sprintf(tmp_char, "%02x", buf[i + j]);
				//printf("%s", tmp_char);
				strcat(dump_to, tmp_char);
				//printf("%s", eeprom_dump);
			}
		}
		//printf("\n");
	}
	//printf("%s\n", dump_to);
}

void BEARRecord()
{
	RecordingRepay = true;
	GenerateRecordingFileName();
	SDL_Delay(500);

	// Recording Loop

	SDL_RWops* file = SDL_RWFromFile(BEARPlay_RecordingFileName, "r+b");
	if (file == NULL) {
		file = SDL_RWFromFile(BEARPlay_RecordingFileName, "w+b");
	}

	char _EEPROM[0x80] = { 0 };
	char __EEPROM[0x80] = { 0 };

	wchar eeprom_path[512];

	host.ConfigLoadStr(L"emu", L"gamefile", eeprom_path, L"");

	if (BEARPlay_Online == 1) {
		if (BEARPlay_Hosting == 1) {
			wcscat(eeprom_path, L".eeprom");
		}
		else {
			wcscat(eeprom_path, L".eeprom_client");
		}
	}
	else {
		wcscat(eeprom_path, L".eeprom");
	}
	
	FILE* eeprome_file = _wfopen(eeprom_path, L"rb");
	if (eeprome_file)
	{
		fread(_EEPROM, 1, 0x80, eeprome_file);
		fclose(eeprome_file);
		hexdump(_EEPROM, sizeof(_EEPROM), __EEPROM);
	}

	char* x = new char[1028];
	sprintf(x, "bearplay2|%S|%S|%S|%S|%S|%S|eeprom|%s|eeprom|:", BEARPlay_P1Name, BEARPlay_P2Name, BEARPlay_GameName, BEARPlay_GameRom, BEARPlay_Region, BEARPlay_SessionTimeStamp, __EEPROM);

	int MetaLength = SDL_strlen(x);
	SDL_RWwrite(file, x, 1, MetaLength);

	while (RecordingRepay == true) {

		// Record the last recorded frame
		if (LastRecordedFrame >= FrameCount) {
			SDL_Delay(15);
			continue;
		}

		SDL_RWseek(file, ((LastRecordedFrame) * 4) + MetaLength, RW_SEEK_SET);
		if (BEARPlay_Hosting == 1 || BEARPlay_Online == 0 || BEARPlay_Spectator == 1)
		{
			SDL_WriteBE16(file, SelfInputs[LastRecordedFrame]);
			SDL_WriteBE16(file, OpponentInputs[LastRecordedFrame]);
		}
		else
		{
			SDL_WriteBE16(file, OpponentInputs[LastRecordedFrame]);
			SDL_WriteBE16(file, SelfInputs[LastRecordedFrame]);
		}

		LastRecordedFrame++;
	}

	SDL_RWclose(file);
}

int MetaDataEnd = 0;
bool FoundMetaData = false;
void BEARPlayback(u16 &Player1Kode, u16 &Player2Kode)
{
	// Discord any Input Data and use the data from the file.
	SDL_RWops* file = SDL_RWFromFile(_bstr_t(BEARPlay_File), "r");

	if (file != NULL)
	{

		Sint64 length = SDL_RWseek(file, 0, RW_SEEK_END);
		Sint64 currentOffset2;

		int SeekPoint = 0;
		if (MetaDataEnd == 0)
		{

			while (!FoundMetaData)
			{
				currentOffset2 = SDL_RWseek(file, SeekPoint, RW_SEEK_SET);
				u8 IsKeyChar = (u8)SDL_ReadBE16(file);
				//printf("Checking: %d %x\n", SeekPoint, IsKeyChar);
				SDL_Delay(1);
				if (length < currentOffset2) {
					ErrorMessageBoxPopUp(L"Meta Data Missing");
					Disconnected = true;
					SDL_RWclose(file);
					
					return;
				}
				if (IsKeyChar == 0x3a) {
					FoundMetaData = true;
					MetaDataEnd = SeekPoint + 2;
				}
				else { SeekPoint++; }
			}
		}

		Sint64 currentOffset = SDL_RWseek(file, (FrameCount * 4) + MetaDataEnd, RW_SEEK_SET);
		if (length < currentOffset) {
			ErrorMessageBoxPopUp(L"End of Replay");
			Disconnected = true;
			SDL_RWclose(file);
			exit(0);
			return;
		}

		SelfInputs[FrameCount] = (u16)SDL_ReadBE16(file);
		OpponentInputs[FrameCount] = (u16)SDL_ReadBE16(file);
		Player1Kode = SelfInputs[FrameCount];
		Player2Kode = OpponentInputs[FrameCount];
		//printf("P1: %x - P2 %x\n", kcode[0], kcode[1]);
		SDL_RWclose(file);
	}
	else
	{
		ErrorMessageBoxPopUp(L"Error Reading Replay File");
		Disconnected = true;
		SDL_RWclose(file);
		
		return;
	}
}

void BEARSpectator_Dreamcast() { // Spectator Code Here, get the inputs from the self/opponent inputs if we have any
														 // Getting Spectator Data From Buffer
														 //printf("Gettin Spectator Data from Buffer");
	int WaitingTimer = 0;
	while (true) { // We didn't get data for this frame yet
		if (OpponentInputs.find(FrameCount) == OpponentInputs.end() || SelfInputs.find(FrameCount) == SelfInputs.end()) {
			if (Disconnected == true) {
				ErrorMessageBoxPopUp(L"End of spectator stream");
				Debug_WriteLine("BearPlay: End of Spectator stream, Exiting.");
				exit(0);
			}

			SDL_Delay(8);
			WaitingTimer += 8;

			if (WaitingTimer > 1000) {
				// Been waiting half a second for the frame data, ask the host to go back and give you frame data from this point on, since something fucked up.
				unsigned char bytes[4];
				bytes[0] = (FrameCount >> 24) & 0xFF;
				bytes[1] = (FrameCount >> 16) & 0xFF;
				bytes[2] = (FrameCount >> 8) & 0xFF;
				bytes[3] = FrameCount & 0xFF;

				Debug_WriteLine("Missing Frame: %d | Asking for new frame data.\n", FrameCount);

				int SendResult = SDLNet_TCP_Send(BEARPlay_ActiveConnection[0], bytes, sizeof(bytes));
				if (SendResult < sizeof(bytes)) {
					ErrorMessageBoxPopUp(L"Connection Lost(BSS)");
					Debug_WriteLine("BEARPlay: Connection Lost(BSS)");
					exit(0);
				}
				SDL_Delay(2000);
				WaitingTimer = 0;
			}

			continue;
		}
		break;
	}
}

void BEARSpectator_Naomi(u16 &Player1Kode, u16 &Player2Kode) { // Spectator Code Here, get the inputs from the self/opponent inputs if we have any
	// Getting Spectator Data From Buffer
	//printf("Gettin Spectator Data from Buffer");
	int WaitingTimer = 0;
	while (true) { // We didn't get data for this frame yet
		if (OpponentInputs.find(FrameCount) == OpponentInputs.end() || SelfInputs.find(FrameCount) == SelfInputs.end()) {
			if (Disconnected == true) {
				ErrorMessageBoxPopUp(L"End of spectator stream");
				Debug_WriteLine("BearPlay: End of Spectator stream, Exiting.");
				exit(0);
			}

			SDL_Delay(8);
			WaitingTimer += 8;

			if (WaitingTimer > 1000) {
				// Been waiting half a second for the frame data, ask the host to go back and give you frame data from this point on, since something fucked up.
				unsigned char bytes[4];
				bytes[0] = (FrameCount >> 24) & 0xFF;
				bytes[1] = (FrameCount >> 16) & 0xFF;
				bytes[2] = (FrameCount >> 8) & 0xFF;
				bytes[3] = FrameCount & 0xFF;

				Debug_WriteLine("Missing Frame: %d | Asking for new frame data.\n", FrameCount);

				int SendResult = SDLNet_TCP_Send(BEARPlay_ActiveConnection[0], bytes, sizeof(bytes));
				if (SendResult < sizeof(bytes)) {
					ErrorMessageBoxPopUp(L"Connection Lost(BSS)");
					Debug_WriteLine("BEARPlay: Connection Lost(BSS)");
					exit(0);
				}
				SDL_Delay(2000);
				WaitingTimer = 0;
			}

			continue;
		}
		break;
	}

	Player1Kode = SelfInputs[FrameCount];
	Player2Kode = OpponentInputs[FrameCount];
}

void Debug_WriteLine(const char *fmt, ...)
{

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	if (BEARPlay_debug > 0)
	{
		FILE * pFile;
		pFile = fopen("BEARlog.txt", "a");
		fprintf(pFile, fmt, args);
		fclose(pFile);
	}
}

s32 FASTCALL Load(emu_info* emu)
{
	Debug_WriteLine("++BEARPLAY By RossenX LOADED.\n");

	memcpy(&host, emu, sizeof(host));
	for (int set = 0; set < 2; set++) {
		memcpy(joypad_settings[set], joypad_settings_K, sizeof(joypad_settings_K));
	}
	
	random_dev.seed(1337);

	// Init SDL and Joystick
	if (SDL_Init(SDL_INIT_GAMECONTROLLER ) == -1) {
		Debug_WriteLine("Coulnd't start SDL\n");
		ErrorMessageBoxPopUp(L"Couldn't start SDL");
		exit(0);
	}
	else
	{
		printf("++SDL: Started with Event and Joystick\n");
		SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
	}

	LoadSettings();
		
	return rv_ok;
}

void FASTCALL  Unload()
{
	RecordingRepay = false;

}

HMODULE hModule;
HINSTANCE hInstance;
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	::hModule = hModule;
	hInstance = (HINSTANCE)hModule;
	return TRUE;
}

bool CheckBEARJammaKeyPressed(int _port, int _inputIndex) {

	// KeyBoard
	wchar temp[5];
	swprintf_s(temp, L"%s", &joypad_settings[_port][_inputIndex].KC[0]);
	char kk = temp[0];

	// Keyboard there is no letter before it so this is just a normal keyboard keycode this is to have backward compatability with BEAR's old configs.
	if (kk == 'k') { // Starts with a = Joystick Axis a0+
		if (GetKeyState(BEARKeyToInt(temp)) & 0x8000) { // The Code Has Been Clicked
			return true;
		} else { return false; }
	}

	// Axis
	if (kk == 'a') { // Starts with a = Joystick Axis a0+
		Sint16 jValue = SDL_GameControllerGetAxis(joy[_port], SDL_GameControllerAxis(_wtoi(&temp[1])));

		if (axisIsNotInDeadzone(jValue, _port, temp[2]))
		{
			if (temp[2] == '+' && jValue > 0) { return true; }
			else if (temp[2] == '-' && jValue < 0) { return true; }

		} else { return false; }

	}

	// Button
	if (kk == 'b') { // Starts with b = Joystick Button b1
		if (SDL_GameControllerGetButton(joy[_port], SDL_GameControllerButton(BEARKeyToInt(temp))) != 0) {
			return true;
		} else {  return false;  }
	}

	// Hat
	/*
	if (kk == 'h') { // Starts with h = Joystick Hat b1 THIS IS KINDA USELESS SINCE THE CONTROLLER INPTU NEVER RETURNS A TYPE OF HAT
		Uint8 hValue = SDL_JoystickGetHat(SDL_GameControllerGetJoystick(joy[_port]), 0);
		if (hValue & temp[2]) {
			return true;
		} else { return false; }
	}
	/**/

	return false;
}

s8 GetAnalogInput(int _port, int _axisIndex, ANALOG_INPUTS _input) {
	int aValue = 0;
	if (joypad_settings[_port][_axisIndex].KC[0] == 'a') {
		aValue = SDL_GameControllerGetAxis(joy[_port], SDL_GameControllerAxis(_wtoi(&joypad_settings[_port][_axisIndex].KC[1])));

		if (axisIsNotInDeadzone(aValue, _port)) {

			if (_input == ANALOG_RT || _input == ANALOG_LT) {

				// Separate the axis, into negative and possitive just in case someone bings the trigger to a thumbstick
				if (joypad_settings[_port][_axisIndex].KC[2] == '+' && aValue > 0) { aValue /= 128; return (s8)aValue; }

				if (joypad_settings[_port][_axisIndex].KC[2] == '-' && aValue < 0) {
					aValue = abs(aValue);
					aValue /= 128;
					if (aValue > 255) { aValue = 255; }
					return (s8)aValue;
				}
				return 0;
			}
			else {
				// Normal Axis
				aValue /= 256;
				if (aValue > 127) { aValue = 127; }
				if (aValue < -127) { aValue = -127; }
			}
		}
		else {
			return 0;
		}
	}
	else { // Digital to Analog
		aValue = 256; // Idle

		switch (_input)
		{
		
		case ANALOG_RT: case ANALOG_LT:
				if (CheckBEARJammaKeyPressed(_port, _axisIndex)) {
					aValue = 255;
				}
				else {
					aValue = 0;
				}
				break;

		case ANALOG_UP:

			if (CheckBEARJammaKeyPressed(_port, 15)) { // Up
				aValue -= 127;
			}
			
			if (CheckBEARJammaKeyPressed(_port, 16)) { // Down
				aValue += 127;
			}

			break;

		case ANALOG_LEFT:
			if (CheckBEARJammaKeyPressed(_port, 17)) { // Left
				aValue -= 127;
			}

			if (CheckBEARJammaKeyPressed(_port, 18)) { // Right
				aValue += 127;
			}

			break;
		}
		
	}

	

	return (s8)aValue;
}


// Double Check the Diagonals
void DoubleCheckDigonals(int port) {

#ifdef BUILD_DREAMCAST
	int UP = key_CONT_DPAD_UP;
	int DOWN = key_CONT_DPAD_DOWN;
	int LEFT = key_CONT_DPAD_LEFT;
	int RIGHT = key_CONT_DPAD_RIGHT;

#else
	int UP = NAOMI_UP_KEY;
	int DOWN = NAOMI_DOWN_KEY;
	int LEFT = NAOMI_LEFT_KEY;
	int RIGHT = NAOMI_RIGHT_KEY;

#endif

	// DOWN LEFT
	if ((kcode[port] & DOWN) && (kcode[port] & LEFT)) {
		if ((kcodeLastFrame[port] & DOWN) && (kcodeLastFrame[port] & RIGHT)) {
			kcode[port] &= ~LEFT;
			kcode_threaded[port] |= DOWN;
			if (BEARPlay_debug > 0 && BEARPlay_Online == 0) { printf("-LEFT\n");}
			
		} else if ((kcodeLastFrame[port]& UP) && (kcodeLastFrame[port]& LEFT)) {
			kcode[port] &= ~DOWN;
			kcode_threaded[port] |= LEFT;
			if (BEARPlay_debug > 0 && BEARPlay_Online == 0) { printf("-DOWN\n"); }

			
		}

	} // DOWN RIGHT
	else if ((kcode[port] & DOWN) && (kcode[port] & RIGHT)) {
		if ((kcodeLastFrame[port] & DOWN) && (kcodeLastFrame[port] & LEFT)) {
			kcode[port] &= ~RIGHT;
			kcode_threaded[port] |= DOWN;
			if (BEARPlay_debug > 0 && BEARPlay_Online == 0) { printf("-RIGHT\n"); }
			

		}else if ((kcodeLastFrame[port] & UP) && (kcodeLastFrame[port] & RIGHT)) {
			kcode[port] &= ~DOWN;
			kcode_threaded[port] |= RIGHT;
			if (BEARPlay_debug > 0 && BEARPlay_Online == 0) { printf("-DOWN\n"); }
			
		}

	} // UP LEFT
	else if ((kcode[port] & UP) && (kcode[port] & LEFT)) {

		if ((kcodeLastFrame[port] & UP) && (kcodeLastFrame[port] & RIGHT)) {
			kcode[port] &= ~LEFT;
			kcode_threaded[port] |= UP;
			if (BEARPlay_debug > 0 && BEARPlay_Online == 0) { printf("-LEFT\n"); }
			

		}else if ((kcodeLastFrame[port] & DOWN) && (kcodeLastFrame[port] & LEFT)) {
			kcode[port] &= ~UP;
			kcode_threaded[port] |= LEFT;
			if (BEARPlay_debug > 0 && BEARPlay_Online == 0) { printf("-UP\n"); }
			
		}

	} // UP RIGHT
	else if ((kcode[port] & UP) && (kcode[port] & RIGHT)) {

		if ((kcodeLastFrame[port] & UP) && (kcodeLastFrame[port] & LEFT)) {
			kcode[port] &= ~RIGHT;
			kcode_threaded[port] |= UP;
			if (BEARPlay_debug > 0 && BEARPlay_Online == 0) { printf("-RIGHT\n"); }
			

		}else if ((kcodeLastFrame[port] & DOWN) && (kcodeLastFrame[port] & RIGHT)) {
			kcode[port] &= ~UP;
			kcode_threaded[port] |= RIGHT;
			if (BEARPlay_debug > 0 && BEARPlay_Online == 0) { printf("-UP\n"); }
			

		}

	}

}


void DoSDLInputRoll()
{
	// Naomi ALL DIGITAL
	// 00000000 | 00000000
	//L D U S T2 S2 T1 S1 | C 6 5 4 3 2 1 R
	for (int port = 0; port < 2 - BEARPlay_Online; port++) {
		kcode[port] = 0x0000;
		u16 tmpThreadedKeys = 0x0000;
		memcpy(&tmpThreadedKeys, &kcode_threaded[port], sizeof(kcode_threaded[port]));
		kcode_threaded[port] = 0x0000;

		for (int i = 0; joypad_settings_K[i].name; i++) {

			if (CheckBEARJammaKeyPressed(port, i)) { kcode[port] |= joypad_settings[port][i].BIT; }
#ifdef BUILD_DREAMCAST
			if (wcscmp(joypad_settings_K[i].name, L"key_CONT_ANALOG_UP") == 0) {joyy[port] = GetAnalogInput(port, i, ANALOG_UP);}
			if (wcscmp(joypad_settings_K[i].name, L"key_CONT_ANALOG_LEFT") == 0) {joyx[port] = GetAnalogInput(port, i, ANALOG_LEFT);}
			if (wcscmp(joypad_settings_K[i].name, L"key_CONT_LSLIDER") == 0) {lt[port] = GetAnalogInput(port, i, ANALOG_LT);}
			if (wcscmp(joypad_settings_K[i].name, L"key_CONT_RSLIDER") == 0) {rt[port] = GetAnalogInput(port, i, ANALOG_RT);}
#endif
		}

#ifdef BUILD_DREAMCAST
		if (((kcode[port]|tmpThreadedKeys) & key_CONT_DPAD_LEFT) && ((kcode[port]|tmpThreadedKeys) & key_CONT_DPAD_RIGHT)) {
			tmpThreadedKeys &= ~key_CONT_DPAD_LEFT; tmpThreadedKeys &= ~key_CONT_DPAD_RIGHT;
		}
		if (((kcode[port]|tmpThreadedKeys) & key_CONT_DPAD_DOWN) && ((kcode[port]|tmpThreadedKeys) & key_CONT_DPAD_UP)) {
			tmpThreadedKeys &= ~key_CONT_DPAD_DOWN; tmpThreadedKeys &= ~key_CONT_DPAD_UP;
		}

		if ((kcode[port] & key_CONT_DPAD_LEFT) && (kcode[port] & key_CONT_DPAD_RIGHT)) {
			kcode[port] &= ~key_CONT_DPAD_LEFT; kcode[port] &= ~key_CONT_DPAD_RIGHT;
		}
		if ((kcode[port] & key_CONT_DPAD_DOWN) && (kcode[port] & key_CONT_DPAD_UP)) {
			kcode[port] &= ~key_CONT_DPAD_DOWN; kcode[port] &= ~key_CONT_DPAD_UP;
		}

#else
		if (((kcode[port] | tmpThreadedKeys) & NAOMI_LEFT_KEY) && ((kcode[port] | tmpThreadedKeys) & NAOMI_RIGHT_KEY)) {
			tmpThreadedKeys &= ~NAOMI_LEFT_KEY; tmpThreadedKeys &= ~NAOMI_RIGHT_KEY;
		}
		if (((kcode[port] | tmpThreadedKeys) & NAOMI_UP_KEY) && ((kcode[port] | tmpThreadedKeys) & NAOMI_DOWN_KEY)) {
			tmpThreadedKeys &= ~NAOMI_UP_KEY; tmpThreadedKeys &= ~NAOMI_DOWN_KEY;
		}

		// Check if LEFT and DOWN are both pressed on dpad, and cancel eachother out.
		if ((kcode[port] & NAOMI_LEFT_KEY) && (kcode[port] & NAOMI_RIGHT_KEY)) {
			kcode[port] &= ~NAOMI_LEFT_KEY; kcode[port] &= ~NAOMI_RIGHT_KEY;

		}
		if ((kcode[port] & NAOMI_UP_KEY) && (kcode[port] & NAOMI_DOWN_KEY)) {
			kcode[port] &= ~NAOMI_UP_KEY; kcode[port] &= ~NAOMI_DOWN_KEY;

		}

#endif
		kcode[port] |= tmpThreadedKeys;
		DoubleCheckDigonals(port);
		memcpy(&kcodeLastFrame[port], &kcode[port], sizeof(kcode[port]));
	}
	
}

// MultiThreaded Inputs
// Inproved Input Handling code to catch inputs that might be pressed for less than 16ms
// This Only applies to button DOWN which is why they are collected every frame and then combined with the actual input poll.
void SDL_InputThread() {

	//printf("Threaded Inputs Started\n");

	while (true)
	{
		SDL_Event sdl_event;
		while (SDL_WaitEvent(&sdl_event))
		{
			// Get which Controller ID this is, if both controllers are on the same ID then do it for both kcode
			// printf("Minframe Input Event\n");
			switch (sdl_event.type)
			{
			case SDL_CONTROLLERBUTTONDOWN:

				for (int _i = 0; _i < 2 - BEARPlay_Online; _i++) {
					if (joy[_i] != NULL) {

						u8 PlayerIndex = 0;
						if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(joy[_i])) == sdl_event.cbutton.which) {
							PlayerIndex = _i;
						}else {continue;}

						//printf("JoystickID: %d PlayerID: %d", sdl_event.cbutton.which, PlayerIndex);

						wchar temp[8];
						swprintf_s(temp, L"b%d", sdl_event.cbutton.button);

						for (int i = 0; joypad_settings_K[i].name; i++) {
							if (wcscmp(temp, joypad_settings[PlayerIndex][i].KC) == 0) {
								kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][i].BIT;

								#ifdef BUILD_NAOMI
								// Check if LEFT and DOWN are both pressed on dpad, and cancel eachother out.
								if (joypad_settings[PlayerIndex][i].BIT == (1 << 5) || joypad_settings[PlayerIndex][i].BIT == (1 << 6)) {
									if (CheckBEARJammaKeyPressed(PlayerIndex, 7)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][7].BIT; }
									if (CheckBEARJammaKeyPressed(PlayerIndex, 8)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][8].BIT; }
								}

								if (joypad_settings[PlayerIndex][i].BIT == (1 << 7) || joypad_settings[PlayerIndex][i].BIT == (1 << 8)) {
									if (CheckBEARJammaKeyPressed(PlayerIndex, 5)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][5].BIT; }
									if (CheckBEARJammaKeyPressed(PlayerIndex, 6)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][6].BIT; }
									
								}

								#else
									if (joypad_settings[PlayerIndex][i].BIT == (1 << 4) || joypad_settings[PlayerIndex][i].BIT == (1 << 5)) {
										if (CheckBEARJammaKeyPressed(PlayerIndex, 6)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][6].BIT; }
										if (CheckBEARJammaKeyPressed(PlayerIndex, 7)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][7].BIT; }
									}

									if (joypad_settings[PlayerIndex][i].BIT == (1 << 6) || joypad_settings[PlayerIndex][i].BIT == (1 << 7)) {
										if (CheckBEARJammaKeyPressed(PlayerIndex, 4)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][4].BIT; }
										if (CheckBEARJammaKeyPressed(PlayerIndex, 5)) { kcode_threaded[PlayerIndex] |=  joypad_settings[PlayerIndex][5].BIT; }
										
									}

								#endif

								break;
							}
						}
					}
				}
				break;
			case SDL_CONTROLLERBUTTONUP:
				for (int _i = 0; _i < 2 - BEARPlay_Online; _i++) {
					if (joy[_i] != NULL) {

						u8 PlayerIndex = 0;
						if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(joy[_i])) == sdl_event.cbutton.which) {
							PlayerIndex = _i;
						}
						else { continue; }

						//printf("JoystickID: %d PlayerID: %d", sdl_event.cbutton.which, PlayerIndex);

						wchar temp[8];
						swprintf_s(temp, L"b%d", sdl_event.cbutton.button);

						for (int i = 0; joypad_settings_K[i].name; i++) {
							if (wcscmp(temp, joypad_settings[PlayerIndex][i].KC) == 0) {
								//kcode_threaded[PlayerIndex] &= 0xFFFF - joypad_settings[PlayerIndex][i].BIT;

								#ifdef BUILD_NAOMI
								if (joypad_settings[PlayerIndex][i].BIT == (1 << 5) || joypad_settings[PlayerIndex][i].BIT == (1 << 6)) {
									if (CheckBEARJammaKeyPressed(PlayerIndex, 7)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][7].BIT; }
									if (CheckBEARJammaKeyPressed(PlayerIndex, 8)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][8].BIT; }
								}

								if (joypad_settings[PlayerIndex][i].BIT == (1 << 7) || joypad_settings[PlayerIndex][i].BIT == (1 << 8)) {
									if (CheckBEARJammaKeyPressed(PlayerIndex, 5)) { kcode_threaded[PlayerIndex] |=  joypad_settings[PlayerIndex][5].BIT; }
									if (CheckBEARJammaKeyPressed(PlayerIndex, 6)) { kcode_threaded[PlayerIndex] |=  joypad_settings[PlayerIndex][6].BIT; }

								}
								#else
								if (joypad_settings[PlayerIndex][i].BIT == (1 << 4) || joypad_settings[PlayerIndex][i].BIT == (1 << 5)) {
									if (CheckBEARJammaKeyPressed(PlayerIndex, 6)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][6].BIT; }
									if (CheckBEARJammaKeyPressed(PlayerIndex, 7)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][7].BIT; }
								}

								if (joypad_settings[PlayerIndex][i].BIT == (1 << 6) || joypad_settings[PlayerIndex][i].BIT == (1 << 7)) {
									if (CheckBEARJammaKeyPressed(PlayerIndex, 4)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][4].BIT; }
									if (CheckBEARJammaKeyPressed(PlayerIndex, 5)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][5].BIT; }

								}
								#endif

								break;
							}
						}
					}
				}
				break;
			case SDL_CONTROLLERAXISMOTION:
				for (int _i = 0; _i < 2 - BEARPlay_Online; _i++) {
					if (joy[_i] != NULL) {

						u8 PlayerIndex = 0;
						if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(joy[_i])) == sdl_event.caxis.which) {
							PlayerIndex = _i;
						}
						else { continue; }

						wchar temp[8];
						if (sdl_event.caxis.value > 0) {
							swprintf_s(temp, L"a%d+", sdl_event.caxis.axis);
						}else {swprintf_s(temp, L"a%d-", sdl_event.caxis.axis);}

						for (int i = 0; joypad_settings_K[i].name; i++) {
							if (wcscmp(temp, joypad_settings[PlayerIndex][i].KC) == 0) {
								if (axisIsNotInDeadzone(sdl_event.caxis.value, PlayerIndex)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][i].BIT; }
								

							#ifdef BUILD_NAOMI
								// Check if LEFT and DOWN are both pressed on dpad, and cancel eachother out.
								if (joypad_settings[PlayerIndex][i].BIT == (1 << 5) || joypad_settings[PlayerIndex][i].BIT == (1 << 6)) {
									if (CheckBEARJammaKeyPressed(PlayerIndex, 7)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][7].BIT; }
									if (CheckBEARJammaKeyPressed(PlayerIndex, 8)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][8].BIT; }									
								}

								if (joypad_settings[PlayerIndex][i].BIT == (1 << 7) || joypad_settings[PlayerIndex][i].BIT == (1 << 8)) {
									if (CheckBEARJammaKeyPressed(PlayerIndex, 5)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][5].BIT; }
									if (CheckBEARJammaKeyPressed(PlayerIndex, 6)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][6].BIT; }
								}
							#else
								if (joypad_settings[PlayerIndex][i].BIT == (1 << 4) || joypad_settings[PlayerIndex][i].BIT == (1 << 5)) {
									if (CheckBEARJammaKeyPressed(PlayerIndex, 6)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][6].BIT; }
									if (CheckBEARJammaKeyPressed(PlayerIndex, 7)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][7].BIT; }
								}

								if (joypad_settings[PlayerIndex][i].BIT == (1 << 6) || joypad_settings[PlayerIndex][i].BIT == (1 << 7)) {
									if (CheckBEARJammaKeyPressed(PlayerIndex, 4)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][4].BIT; }
									if (CheckBEARJammaKeyPressed(PlayerIndex, 5)) { kcode_threaded[PlayerIndex] |= joypad_settings[PlayerIndex][5].BIT; }
								}
#							endif

								break;
							}
						}
					}
				}
				break;
			case SDL_CONTROLLERDEVICEADDED:
				//ReloadControlSettings(0, 0, 0);
				break;
			case SDL_CONTROLLERDEVICEREMOVED:
				//ReloadControlSettings(0, 0, 0);
				break;
			}
		}
	}
}

//(D)      (D2)
//RLDUSABC UDRL?XYZ L        R        Y(LR)    X(UD)    Nothing  Nothing                                   32767
//11111111 11111111 00000000 00000000 00000010 00000010 00000000 00000000 - Min  - 2
//11111111 11111111 00000000 00000000 10000000 10000000 00000000 00000000 - Idle - 128 
//11111111 11111111 11111111 11111111 11111110 11111110 00000000 00000000 - Max  - 254

#ifdef BUILD_DREAMCAST
u32 FASTCALL ControllerDMA(void* device_instance, u32 Command, u32* buffer_in, u32 buffer_in_len, u32* buffer_out, u32& buffer_out_len)
{
	//printf("++BEARPlay: ControllerDMA");
	u8*buffer_out_b = (u8*)buffer_out;
	u32 port = ((maple_device_instance*)device_instance)->port >> 6;
	u64 InputsToUse = DREAMCAST_INPUTS_NONE;

	switch (Command)
	{
	case 1:
		//printf("++BEARPlay: Controller 1");
		w32(1 << 24); // 24
		if (Peripheral[port] == 1) {
			w32(0xFF070000); // C:0xFE060F00 A: 0xFF070000 FUNCTION ID
		}
		else {
			w32(0xFE060F00); // C:0xFE060F00 A: 0xFF070000 FUNCTION ID
		}

		w32(0);   // 0 F2
		w32(0);   // 0 F3
		w8(0xFF);    // 0 AREA CODE
		w8(0);    // 0 DIRECTION
		//w8((u8)"Arcade Stick");
		for (u32 i = 0; i < 30; i++) { if (testJoy_strName[i] != 0) { w8((u8)testJoy_strName[i]); } else { w8(0x20); } } // STR NAME
		for (u32 i = 0; i < 60; i++) { if (testJoy_strBrand_2[i] != 0) { w8((u8)testJoy_strBrand_2[i]); } else { w8(0x20); } } // STR NAME

		if (Peripheral[port] == 1) {
			w16(0x012C); // 0x01AE Standby
			w16(0x0190); // 0x01F4 MAMAX
		}
		else {
			w16(0x01AE); // 0x01AE Standby
			w16(0x01F4); // 0x01F4 MAMAX
		}

		return 5;

	case 9:
		//printf("++BEARPlay: Controller 9");
		w32(1 << 24);

		if (port == 0) {DoSDLInputRoll();}
		

		// Spectatore Controls
		if (BEARPlay_Spectator == 1) {

			if (CheckBEARJammaKeyPressed(0, 7)) {
				if (!FastForwardDown) {
					host.BroadcastEvent((u32)MT_All, NDE_AICA_TOGGLE_AUDIO_SYNC, 0, 0);
					FastForwardDown = true;
				}
			}
			else { FastForwardDown = false; }

		}

		if (BEARPlay_Spectator == 1) {
			BEARSpectator_Dreamcast();

			if (port == 0) {
				InputsToUse = SelfInputs[FrameCount];
			}
			else { 
				InputsToUse = OpponentInputs[FrameCount]; 
			}

		}
		else if (Disconnected == true)
		{
			ErrorMessageBoxPopUp(L"Connection Lost");
			exit(0);
		}
		else if (BEARPlay_Online == 1) {
				if (port == 0) { BEARnet_Dreamcast(); }
				if (port == 0 && BEARPlay_Hosting == 1) { memcpy(&InputsToUse, &SelfInputs[FrameCount], sizeof(SelfInputs[FrameCount])); }
				else if (port == 0 && BEARPlay_Hosting == 0) { memcpy(&InputsToUse, &OpponentInputs[FrameCount], sizeof(OpponentInputs[FrameCount])); }
				else if (port == 1 && BEARPlay_Hosting == 0) { memcpy(&InputsToUse, &SelfInputs[FrameCount], sizeof(SelfInputs[FrameCount])); }
				else if (port == 1 && BEARPlay_Hosting == 1) { memcpy(&InputsToUse, &OpponentInputs[FrameCount], sizeof(OpponentInputs[FrameCount])); }
			}
			else {
				if (port == 0) {

					// Temp inputs for the sake of desync so the spectator data does not get sent between these lines
					u64 tmpP1Inputs = combineDreamcastInputData(kcode[0], lt[0], rt[0], GetBtFromSgn(joyx[0]), GetBtFromSgn(joyy[0]));
					u64 tmpP2Inputs = combineDreamcastInputData(kcode[1], lt[1], rt[1], GetBtFromSgn(joyx[1]), GetBtFromSgn(joyy[1]));

					for (int i = 0; i < 2; ++i) { // Record/Playback

						if (CheckBEARJammaKeyPressed(i, 30)) {

							if (RecordInputHold[i] == false) {
								if (!RecordInput[i]) {
									RecordInput[i] = true;
									PlaybackInput[i] = false;
									host.BroadcastEvent((u32)MT_All, NDE_PVR_RECORD, &RecordInput, 1);
									host.BroadcastEvent((u32)MT_All, NDE_PVR_PLAYBACK, &PlaybackInput, 1);
									RecordedInputs[i].clear();
									RecordInputHold[i] = true;
									printf("Toggle Recording on.\n");
								}
								else {
									RecordInput[i] = false;
									host.BroadcastEvent((u32)MT_All, NDE_PVR_RECORD, &RecordInput, 1);
									RecordInputHold[i] = true;
									printf("Toggle Recording off.\n");
								}
							}

						}
						else { RecordInputHold[i] = false;  }

						if (CheckBEARJammaKeyPressed(i, 31)) {

							if (PlaybackInputHold[i] == false) {
								if (!PlaybackInput[i]) {
									PlaybackInput[i] = true;
									RecordInput[i] = false;
									host.BroadcastEvent((u32)MT_All, NDE_PVR_RECORD, &RecordInput, 1);
									host.BroadcastEvent((u32)MT_All, NDE_PVR_PLAYBACK, &PlaybackInput, 1);
									PlaybackInputHold[i] = true;
									printf("Toggle Playback on.\n");
									PlaybackFrameCounter[i] = 0;
								}
								else {
									PlaybackInput[i] = false;
									PlaybackInputHold[i] = true;
									host.BroadcastEvent((u32)MT_All, NDE_PVR_PLAYBACK, &PlaybackInput, 1);
									printf("Toggle Playback off.\n");
								}
							}

						}
						else { PlaybackInputHold[i] = false; }

						if (RecordInput[i]) {
							if (i == 0) {
								RecordedInputs[i][RecordedInputs[i].size()] = tmpP1Inputs;
								tmpP2Inputs = tmpP1Inputs;
								tmpP1Inputs = DREAMCAST_INPUTS_NONE;
							}
							else {
								RecordedInputs[i][RecordedInputs[i].size()] = tmpP2Inputs;
								tmpP1Inputs = tmpP2Inputs;
								tmpP2Inputs = DREAMCAST_INPUTS_NONE;
							}
							int RecordedCount = RecordedInputs[i].size();
							//host.BroadcastEvent((u32)MT_All, NDE_PVR_RECORDEDFRAMECOUNT, &RecordedCount, 0);
						}

						if (PlaybackInput[i]) {

							if (RecordedInputs[i].find(PlaybackFrameCounter[i]) == RecordedInputs[i].end()) { PlaybackFrameCounter[i] = 0; }
							if (i == 0) {
								tmpP2Inputs = RecordedInputs[i][PlaybackFrameCounter[i]];
							}
							else {
								tmpP1Inputs = RecordedInputs[i][PlaybackFrameCounter[i]];
							}
							PlaybackFrameCounter[i]++;
							//host.BroadcastEvent((u32)MT_All, NDE_PVR_PLAYBACKFRAMECOUNT, 0, PlaybackFrameCounter[i]);
						}
					}

					SelfInputs[FrameCount + BEARPlay_Delay] = tmpP1Inputs;
					OpponentInputs[FrameCount + BEARPlay_Delay] = tmpP2Inputs;

					InputsToUse = SelfInputs[FrameCount];
				}
				else {
					InputsToUse = OpponentInputs[FrameCount];
				}

			}

			if (InputsToUse == 0) {
				while (InputsToUse == 0)
				{
					if (FrameCount <= BEARPlay_Delay) { break; }
					if (port == 0 && BEARPlay_Hosting == 1) { memcpy(&InputsToUse, &SelfInputs[FrameCount], sizeof(SelfInputs[FrameCount])); }
					else if (port == 0 && BEARPlay_Hosting == 0) { memcpy(&InputsToUse, &OpponentInputs[FrameCount], sizeof(OpponentInputs[FrameCount])); }
					else if (port == 1 && BEARPlay_Hosting == 0) { memcpy(&InputsToUse, &SelfInputs[FrameCount], sizeof(SelfInputs[FrameCount])); }
					else if (port == 1 && BEARPlay_Hosting == 1) { memcpy(&InputsToUse, &OpponentInputs[FrameCount], sizeof(OpponentInputs[FrameCount])); }
					SDL_Delay(1);
				}
			}
			
			w16(~InputsToUse); // InputsToUse | 0xF901 Buttons that are always up on a DC Controller | Removed to enable C/Z for macros
			w8(InputsToUse >> 24); // R
			w8(InputsToUse >> 16); // L
			w8(InputsToUse >> 32); // Joy X
			w8(InputsToUse >> 40); // Joy Y
			w8(0); // ?joy2X
			w8(0); // ?joy2Y

			if (port == 1) { FrameCount++; }

			if (BEARPlay_debug > 0 && InputsToUse != DREAMCAST_INPUTS_NONE && BEARPlay_Online == 0) {
				
				if (port == 0) {printf("P1: ");}else {printf("P2: ");}

				if (InputsToUse&key_CONT_DPAD_UP) { printf("U"); }
				else { printf(" "); }
				if (InputsToUse&key_CONT_DPAD_DOWN) { printf("D"); }
				else { printf(" "); }
				if (InputsToUse&key_CONT_DPAD_LEFT) { printf("L"); }
				else { printf(" "); }
				if (InputsToUse&key_CONT_DPAD_RIGHT) { printf("R"); }
				else { printf(" "); }

				if (InputsToUse&key_CONT_A) { printf("A"); }
				else { printf(" "); }
				if (InputsToUse&key_CONT_B) { printf("B"); }
				else { printf(" "); }
				if (InputsToUse&key_CONT_X) { printf("X"); }
				else { printf(" "); }
				if (InputsToUse&key_CONT_Y) { printf("Y"); }
				else { printf(" "); }
				if (InputsToUse&key_CONT_Z) { printf("Z"); }
				else { printf(" "); }
				if (InputsToUse&key_CONT_C) { printf("C"); }
				else { printf(" "); }
				if (InputsToUse&key_CONT_START) { printf("S"); }
				else { printf(" "); }

				print_bits(InputsToUse);

			}

			return 8;

	default:
		return 7;
	}
}
#endif

void printState(u32 cmd, u32* buffer_in, u32 buffer_in_len)
{
	printf("Command : 0x%X", cmd);
	if (buffer_in_len > 0)
		printf(",Data : %d bytes\n", buffer_in_len);
	else
		printf("\n");
	buffer_in_len >>= 2;
	while (buffer_in_len-- > 0)
	{
		printf("%08X ", *buffer_in++);
		if (buffer_in_len == 0)
			printf("\n");
	}
}

// Naomi
// 00000000 | 00000000
//L D U S T2 S2 T1 S1 | C 6 5 4 3 2 1 R

#ifdef BUILD_NAOMI
u32 FASTCALL ControllerDMA_naomi(void* device_instance, u32 Command, u32* buffer_in, u32 buffer_in_len, u32* buffer_out, u32& buffer_out_len)
{
#define ret(x) { responce=(x); return; }

	// printf("ControllerDMA Called 0x%X;Command %d\n", ((maple_device_instance*)device_instance)->port, Command);

	// BEAR
	u8*buffer_out_b = (u8*)buffer_out;
	u8*buffer_in_b = (u8*)buffer_in;
	buffer_out_len = 0;
	u32 port = ((maple_device_instance*)device_instance)->port >> 6;
	switch (Command)
	{
	case 0x86:
	{
		u32 subcode = *(u8*)buffer_in;
		switch (subcode)
		{
		case 0x15:
		case 0x33:
		{
			// do SDL stuff here, to get the kcode to use
			DoSDLInputRoll();

			// Spectator Controls here
			if (BEARPlay_Spectator == 1) {

				if (CheckBEARJammaKeyPressed(0, 8)) {
					if (!FastForwardDown) {
						host.BroadcastEvent((u32)MT_All, NDE_AICA_TOGGLE_AUDIO_SYNC, 0, 0);
						FastForwardDown = true;
					}
				}
				else { FastForwardDown = false; }

			}

			buffer_out[0] = 0xFFFFFFFF;
			buffer_out[1] = 0xFFFFFFFF;

			u16 keycode = kcode[0];
			u16 keycode2 = kcode[1];

			//print_bits(keycode);

			if (BEARPlay_Playback == 1) {
				BEARPlayback(keycode, keycode2);
			}
			else if (BEARPlay_Spectator == 1) {
				BEARSpectator_Naomi(keycode, keycode2);
			}
			else if (Disconnected) {
				ErrorMessageBoxPopUp(L"Connection Lost");
				exit(0);
			}
			else if (BEARPlay_Online == 1) {
				BEARnet_Naomi(keycode, keycode2);
			}
			else { // Playing Offline

				for (int i = 0; i < 2; ++i) { // Record/Playback

					if (CheckBEARJammaKeyPressed(i,25)) {

						if (RecordInputHold[i] == false) {
							if (!RecordInput[i]) {
								RecordInput[i] = true;
								PlaybackInput[i] = false;
								host.BroadcastEvent((u32)MT_All, NDE_PVR_RECORD, &RecordInput, 1);
								host.BroadcastEvent((u32)MT_All, NDE_PVR_PLAYBACK, &PlaybackInput, 1);
								RecordedInputs[i].clear();
								RecordInputHold[i] = true;
								printf("Toggle Recording on.\n");
							}
							else {
								RecordInput[i] = false;
								RecordInputHold[i] = true;
								host.BroadcastEvent((u32)MT_All, NDE_PVR_RECORD, &RecordInput, 1);
								printf("Toggle Recording off.\n");
							}
						}

					}else {RecordInputHold[i] = false;}

					if (CheckBEARJammaKeyPressed(i, 26)) {

						if (PlaybackInputHold[i] == false) {
							if (!PlaybackInput[i]) {
								PlaybackInput[i] = true;
								RecordInput[i] = false;
								host.BroadcastEvent((u32)MT_All, NDE_PVR_RECORD, &RecordInput, 1);
								host.BroadcastEvent((u32)MT_All, NDE_PVR_PLAYBACK, &PlaybackInput, 1);
								PlaybackInputHold[i] = true;
								printf("Toggle Playback on.\n");
								PlaybackFrameCounter[i] = 0;
							}
							else {
								PlaybackInput[i] = false;
								PlaybackInputHold[i] = true;
								host.BroadcastEvent((u32)MT_All, NDE_PVR_PLAYBACK, &PlaybackInput, 1);
								printf("Toggle Playback off.\n");
							}
						}

					}
					else { PlaybackInputHold[i] = false; }

					if (RecordInput[i]) {
						if (i == 0) {
							RecordedInputs[i][RecordedInputs[i].size()] = keycode;
							keycode2 = keycode;
							keycode = 0x00000000;
						}
						else {
							RecordedInputs[i][RecordedInputs[i].size()] = keycode2;
							keycode = keycode2;
							keycode2 = 0x00000000;
						}
						
					}

					if (PlaybackInput[i]) {

						if (RecordedInputs[i].find(PlaybackFrameCounter[i]) == RecordedInputs[i].end()) { PlaybackFrameCounter[i] = 0; }
						if (i == 0) {
							keycode2 = RecordedInputs[i][PlaybackFrameCounter[i]];
						}else {
							keycode = RecordedInputs[i][PlaybackFrameCounter[i]];
						}
						PlaybackFrameCounter[i]++;

					}
				}

				if (BEARPlay_Delay > 0) // Just to make double sure no delay code is executed offline if delay is 0
				{
					// Delay Simulation
					if (FrameCount > BEARPlay_Delay) {
						SelfInputs[FrameCount + BEARPlay_Delay] = keycode; // Store for replay/spectators
						OpponentInputs[FrameCount + BEARPlay_Delay] = keycode2; // Store for replay/spectators
					}else{
						SelfInputs[FrameCount] = 0x0000; // Store for replay/spectators
						OpponentInputs[FrameCount] = 0x0000; // Store for replay/spectators
					}

					keycode = ~SelfInputs[FrameCount];
					keycode2 = ~OpponentInputs[FrameCount];

					
				}else{
					SelfInputs[FrameCount] = keycode; // Store for replay/spectators
					OpponentInputs[FrameCount] = keycode2; // Store for replay/spectators

				}
			}
			
			//Doublecheck Inputs
			if (BEARPlay_Hosting == 1 || BEARPlay_Spectator == 1 || BEARPlay_Online == 0) {
				if (keycode != SelfInputs[FrameCount]) {
					while (keycode != SelfInputs[FrameCount]) { 
						keycode = SelfInputs[FrameCount]; 
						SDL_Delay(1); 
					}
				}
				if (keycode2 != OpponentInputs[FrameCount]) {
					while (keycode2 != OpponentInputs[FrameCount]) {
						keycode2 = OpponentInputs[FrameCount]; 
						SDL_Delay(1); 
					}
				}
			}
			else {
				if (keycode != OpponentInputs[FrameCount]) {
					while (keycode != OpponentInputs[FrameCount]) { 
						keycode = OpponentInputs[FrameCount]; 
						SDL_Delay(1); 
					}
				}
				if (keycode2 != SelfInputs[FrameCount]) {
					while (keycode2 != SelfInputs[FrameCount]) { 
						keycode2 = SelfInputs[FrameCount]; 
						SDL_Delay(1); 
					}
				}
			}

			if (BEARPlay_debug > 0 && BEARPlay_Online == 0) {

				if (keycode != 0x0000) {
					printf("P1: ");
					if (keycode&NAOMI_UP_KEY) { printf("U"); }
					else { printf(" "); }
					if (keycode&NAOMI_DOWN_KEY) { printf("D"); }
					else { printf(" "); }
					if (keycode&NAOMI_LEFT_KEY) { printf("L"); }
					else { printf(" "); }
					if (keycode&NAOMI_RIGHT_KEY) { printf("R"); }
					else { printf(" "); }

					if (keycode&NAOMI_BTN0_KEY) { printf("1"); }
					else { printf(" "); }
					if (keycode&NAOMI_BTN1_KEY) { printf("2"); }
					else { printf(" "); }
					if (keycode&NAOMI_BTN2_KEY) { printf("3"); }
					else { printf(" "); }
					if (keycode&NAOMI_BTN3_KEY) { printf("4"); }
					else { printf(" "); }
					if (keycode&NAOMI_BTN4_KEY) { printf("5"); }
					else { printf(" "); }
					if (keycode&NAOMI_BTN5_KEY) { printf("6"); }
					else { printf(" "); }
					if (keycode&NAOMI_COIN_KEY) { printf("C"); }
					else { printf(" "); }
					if (keycode&NAOMI_START_KEY) { printf("S"); }
					else { printf(" "); }
					printf("\n");
				}

				if (keycode2 != 0x0000 && BEARPlay_Online == 0) {
					printf("P2: ");
					if (keycode2&NAOMI_UP_KEY) { printf("U"); }
					else { printf(" "); }
					if (keycode2&NAOMI_DOWN_KEY) { printf("D"); }
					else { printf(" "); }
					if (keycode2&NAOMI_LEFT_KEY) { printf("L"); }
					else { printf(" "); }
					if (keycode2&NAOMI_RIGHT_KEY) { printf("R"); }
					else { printf(" "); }

					if (keycode2&NAOMI_BTN0_KEY) { printf("1"); }
					else { printf(" "); }
					if (keycode2&NAOMI_BTN1_KEY) { printf("2"); }
					else { printf(" "); }
					if (keycode2&NAOMI_BTN2_KEY) { printf("3"); }
					else { printf(" "); }
					if (keycode2&NAOMI_BTN3_KEY) { printf("4"); }
					else { printf(" "); }
					if (keycode2&NAOMI_BTN4_KEY) { printf("5"); }
					else { printf(" "); }
					if (keycode2&NAOMI_BTN5_KEY) { printf("6"); }
					else { printf(" "); }
					if (keycode2&NAOMI_COIN_KEY) { printf("C"); }
					else { printf(" "); }
					if (keycode2&NAOMI_START_KEY) { printf("S"); }
					else { printf(" "); }
					printf("\n");
				}

			}

			FrameCount++; // Aight we did all the shit, lets get ready for next frame

			if (keycode&NAOMI_SERVICE_KEY_2)		//Service
				buffer_out[0] &= ~(1 << 0x1b);

			if (keycode&NAOMI_TEST_KEY_2)		//Test
				buffer_out[0] &= ~(1 << 0x1a);

			if (State.Mode == 0 && subcode != 0x33)	//Get Caps
			{
				buffer_out_b[0x11 + 1] = 0x8E;	//Valid data check
				buffer_out_b[0x11 + 2] = 0x01;
				buffer_out_b[0x11 + 3] = 0x00;
				buffer_out_b[0x11 + 4] = 0xFF;
				buffer_out_b[0x11 + 5] = 0xE0;
				buffer_out_b[0x11 + 8] = 0x01;

				switch (State.Cmd)
				{
					//Reset, in : 2 bytes, out : 0
				case 0xF0:
					break;

					//Find nodes?
					//In addressing Slave address, in : 2 bytes, out : 1
				case 0xF1:
				{
					buffer_out_len = 4 * 4;
				}
				break;

				//Speed Change, in : 2 bytes, out : 0
				case 0xF2:
					break;

					//Name
					//"In the I / O ID" "Reading each slave ID data"
					//"NAMCO LTD.; I / O PCB-1000; ver1.0; for domestic only, no analog input"
					//in : 1 byte, out : max 102
				case 0x10:
				{
					static char ID1[102] = "nullDC Team; I/O Plugin-1; ver0.2; for nullDC or other emus";
					buffer_out_b[0x8 + 0x10] = (BYTE)strlen(ID1) + 3;
					for (int i = 0; ID1[i] != 0; ++i)
					{
						buffer_out_b[0x8 + 0x13 + i] = ID1[i];
					}
				}
				break;

				//CMD Version
				//REV in command|Format command to read the (revision)|One|Two 
				//in : 1 byte, out : 2 bytes
				case 0x11:
				{
					buffer_out_b[0x8 + 0x13] = 0x13;
				}
				break;

				//JVS Version
				//In JV REV|JAMMA VIDEO standard reading (revision)|One|Two 
				//in : 1 byte, out : 2 bytes
				case 0x12:
				{
					buffer_out_b[0x8 + 0x13] = 0x30;
				}
				break;

				//COM Version
				//VER in the communication system|Read a communication system compliant version of |One|Two
				//in : 1 byte, out : 2 bytes
				case 0x13:
				{
					buffer_out_b[0x8 + 0x13] = 0x10;
				}
				break;

				//Features
				//Check in feature |Each features a slave to read |One |6 to
				//in : 1 byte, out : 6 + (?)
				case 0x14:
				{
					unsigned char *FeatPtr = buffer_out_b + 0x8 + 0x13;
					buffer_out_b[0x8 + 0x9 + 0x3] = 0x0;
					buffer_out_b[0x8 + 0x9 + 0x9] = 0x1;
#define ADDFEAT(Feature,Count1,Count2,Count3)	*FeatPtr++=Feature; *FeatPtr++=Count1; *FeatPtr++=Count2; *FeatPtr++=Count3;
					ADDFEAT(1, 2, 12, 0);	//Feat 1=Digital Inputs.  2 Players. 10 bits
					ADDFEAT(2, 2, 0, 0);	//Feat 2=Coin inputs. 2 Inputs
					ADDFEAT(3, 2, 0, 0);	//Feat 3=Analog. 2 Chans
					ADDFEAT(0, 0, 0, 0);	//End of list
				}
				break;

				default:
					//printf("unknown CAP %X\n",State.Cmd);
					return 0;
				}
				buffer_out_len = 4 * 4;
			}
			else if (State.Mode == 1 || State.Mode == 2 || subcode == 0x33)	//Get Data
			{
				unsigned char glbl = 0x00;
				unsigned char p1_1 = 0x00;
				unsigned char p1_2 = 0x00;
				unsigned char p2_1 = 0x00;
				unsigned char p2_2 = 0x00;
				static unsigned char LastKey[256];
				static unsigned short coin1 = 0x0000;
				static unsigned short coin2 = 0x0000;
				unsigned char Key[256];
				GetKeyboardState(Key);

				if (keycode&NAOMI_SERVICE_KEY_1)			//Service ?
					glbl |= 0x80;
				if (keycode&NAOMI_TEST_KEY_1)			//Test
					p1_1 |= 0x40;
				if (keycode&NAOMI_START_KEY)			//start ?
					p1_1 |= 0x80;
				if (keycode&NAOMI_UP_KEY)			//up
					p1_1 |= 0x20;
				if (keycode&NAOMI_DOWN_KEY)		//down
					p1_1 |= 0x10;
				if (keycode&NAOMI_LEFT_KEY)		//left
					p1_1 |= 0x08;
				if (keycode&NAOMI_RIGHT_KEY)		//right
					p1_1 |= 0x04;
				if (keycode&NAOMI_BTN0_KEY)			//btn1
					p1_1 |= 0x02;
				if (keycode&NAOMI_BTN1_KEY)			//btn2
					p1_1 |= 0x01;
				if (keycode&NAOMI_BTN2_KEY)			//btn3
					p1_2 |= 0x80;
				if (keycode&NAOMI_BTN3_KEY)			//btn4
					p1_2 |= 0x40;
				if (keycode&NAOMI_BTN4_KEY)			//btn5
					p1_2 |= 0x20;
				if (keycode&NAOMI_BTN5_KEY)			//btn6
					p1_2 |= 0x10;

				if (keycode2&NAOMI_TEST_KEY_1)			//Test
					p2_1 |= 0x40;
				if (keycode2&NAOMI_START_KEY)			//start ?
					p2_1 |= 0x80;
				if (keycode2&NAOMI_UP_KEY)			//up
					p2_1 |= 0x20;
				if (keycode2&NAOMI_DOWN_KEY)		//down
					p2_1 |= 0x10;
				if (keycode2&NAOMI_LEFT_KEY)		//left
					p2_1 |= 0x08;
				if (keycode2&NAOMI_RIGHT_KEY)		//right
					p2_1 |= 0x04;
				if (keycode2&NAOMI_BTN0_KEY)			//btn1
					p2_1 |= 0x02;
				if (keycode2&NAOMI_BTN1_KEY)			//btn2
					p2_1 |= 0x01;
				if (keycode2&NAOMI_BTN2_KEY)			//btn3
					p2_2 |= 0x80;
				if (keycode2&NAOMI_BTN3_KEY)			//btn4
					p2_2 |= 0x40;
				if (keycode2&NAOMI_BTN4_KEY)			//btn5
					p2_2 |= 0x20;
				if (keycode2&NAOMI_BTN5_KEY)			//btn6
					p2_2 |= 0x10;

				static bool old_coin = false;
				static bool old_coin2 = false;

				if ((old_coin == false) && (keycode&NAOMI_COIN_KEY))
					coin1++;
				old_coin = (keycode&NAOMI_COIN_KEY) ? true : false;

				if ((old_coin2 == false) && (keycode2&NAOMI_COIN_KEY))
					coin2++;
				old_coin2 = (keycode2&NAOMI_COIN_KEY) ? true : false;

				buffer_out_b[0x11 + 0] = 0x00;
				buffer_out_b[0x11 + 1] = 0x8E;	//Valid data check
				buffer_out_b[0x11 + 2] = 0x01;
				buffer_out_b[0x11 + 3] = 0x00;
				buffer_out_b[0x11 + 4] = 0xFF;
				buffer_out_b[0x11 + 5] = 0xE0;
				buffer_out_b[0x11 + 8] = 0x01;

				//memset(OutData+8+0x11,0x00,0x100);

				buffer_out_b[8 + 0x12 + 0] = 1;
				buffer_out_b[8 + 0x12 + 1] = glbl;
				buffer_out_b[8 + 0x12 + 2] = p1_1;
				buffer_out_b[8 + 0x12 + 3] = p1_2;
				buffer_out_b[8 + 0x12 + 4] = p2_1;
				buffer_out_b[8 + 0x12 + 5] = p2_2;
				buffer_out_b[8 + 0x12 + 6] = 1;
				buffer_out_b[8 + 0x12 + 7] = coin1 >> 8;
				buffer_out_b[8 + 0x12 + 8] = coin1 & 0xff;
				buffer_out_b[8 + 0x12 + 9] = coin2 >> 8;
				buffer_out_b[8 + 0x12 + 10] = coin2 & 0xff;
				buffer_out_b[8 + 0x12 + 11] = 1;
				buffer_out_b[8 + 0x12 + 12] = 0x00;
				buffer_out_b[8 + 0x12 + 13] = 0x00;
				buffer_out_b[8 + 0x12 + 14] = 0x00;
				buffer_out_b[8 + 0x12 + 15] = 0x00;
				buffer_out_b[8 + 0x12 + 16] = 0x00;
				buffer_out_b[8 + 0x12 + 17] = 0x00;
				buffer_out_b[8 + 0x12 + 18] = 0x00;
				buffer_out_b[8 + 0x12 + 19] = 0x00;
				buffer_out_b[8 + 0x12 + 20] = 0x00;

				memcpy(LastKey, Key, sizeof(Key));

				if (State.Mode == 1)
				{
					buffer_out_b[0x11 + 0x7] = 19;
					buffer_out_b[0x11 + 0x4] = 19 + 5;
				}
				else
				{
					buffer_out_b[0x11 + 0x7] = 17;
					buffer_out_b[0x11 + 0x4] = 17 - 1;
				}

				//OutLen=8+0x11+16;
				buffer_out_len = 8 + 0x12 + 20;
			}
			/*ID.Keys=0xFFFFFFFF;
			if(GetKeyState(VK_F1)&0x8000)		//Service
			ID.Keys&=~(1<<0x1b);
			if(GetKeyState(VK_F2)&0x8000)		//Test
			ID.Keys&=~(1<<0x1a);
			memcpy(OutData,&ID,sizeof(ID));
			OutData[0x12]=0x8E;
			OutLen=sizeof(ID);
			*/
		}
		return 8;

		case 0x17:	//Select Subdevice
		{
			State.Mode = 0;
			State.Cmd = buffer_in_b[8];
			State.Node = buffer_in_b[9];
			buffer_out_len = 0;
		}
		return (7);

		case 0x27:	//Transfer request
		{
			State.Mode = 1;
			State.Cmd = buffer_in_b[8];
			State.Node = buffer_in_b[9];
			buffer_out_len = 0;
		}
		return (7);
		case 0x21:		//Transfer request with repeat
		{
			State.Mode = 2;
			State.Cmd = buffer_in_b[8];
			State.Node = buffer_in_b[9];
			buffer_out_len = 0;
		}
		return (7);

		case 0x0B:	//EEPROM write
		{
			int address = buffer_in_b[1];
			int size = buffer_in_b[2];
			//printf("EEprom write %08X %08X\n",address,size);
			//printState(Command,buffer_in,buffer_in_len);
			memcpy(EEPROM + address, buffer_in_b + 4, size);

			wchar eeprom_file[512];
			host.ConfigLoadStr(L"emu", L"gamefile", eeprom_file, L"");

			if (BEARPlay_Online == 1) {
				if (BEARPlay_Hosting == 1) {
					wcscat(eeprom_file, L".eeprom");
				}
				else {
					wcscat(eeprom_file, L".eeprom_client");
				}
			}
			else {
				if (BEARPlay_Playback == 1) {
					wcscat(eeprom_file, L".eeprom_client");
				}
				else {
					wcscat(eeprom_file, L".eeprom");
				}

			}

			//wcscat(eeprom_file, L".eeprom");

			FILE* f = _wfopen(eeprom_file, L"wb");
			if (f) {
				fwrite(EEPROM, 1, 0x80, f);
				fclose(f);
				//wprintf(L"SAVED EEPROM to %s\n",eeprom_file);
			}
		}
		return (7);
		case 0x3:	//EEPROM read
		{
			if (!EEPROM_loaded)
			{
				EEPROM_loaded = true;
				wchar eeprom_file[512];
				host.ConfigLoadStr(L"emu", L"gamefile", eeprom_file, L"");

				if (BEARPlay_Online == 1) {
					if (BEARPlay_Hosting == 1) {
						wcscat(eeprom_file, L".eeprom");
					}
					else {
						wcscat(eeprom_file, L".eeprom_client");
					}
				}
				else {
					if (BEARPlay_Playback == 1) {
						wcscat(eeprom_file, L".eeprom_client");
					}
					else {
						wcscat(eeprom_file, L".eeprom");
					}
					
				}

				//wcscat(eeprom_file, L".eeprom");

				FILE* f = _wfopen(eeprom_file, L"rb");
				if (f) {
					fread(EEPROM, 1, 0x80, f);
					fclose(f);
					//wprintf(L"LOADED EEPROM from %s\n", EEPROM);
				}
			}
			//printf("EEprom READ ?\n");
			int address = buffer_in_b[1];
			//printState(Command,buffer_in,buffer_in_len);
			memcpy(buffer_out, EEPROM + address, 0x80);
			buffer_out_len = 0x80;
		}
		return 8;
		//IF I return all FF, then board runs in low res
		case 0x31:
		{
			buffer_out[0] = 0xffffffff;
			buffer_out[1] = 0xffffffff;
		}
		return (8);

		//case 0x3:
		//	break;

		//case 0x1:
		//	break;
		default:
			return 7;
		}

		return 8;//MAPLE_RESPONSE_DATATRF
	}
	break;

	case 0x82:
	{
		const char *ID = "315-6149    COPYRIGHT SEGA E\x83\x00\x20\x05NTERPRISES CO,LTD.  ";
		memset(buffer_out_b, 0x20, 256);
		memcpy(buffer_out_b, ID, 0x38 - 4);
		buffer_out_len = 256;
		return (0x83);
	}

	case 1:
	{
		w32(1 << 24);
		w32(0xfe060f00);
		w32(0);
		w32(0);
		w8(0xFF);
		w8(0);
		for (u32 i = 0; i < 30; i++)
		{
			if (testJoy_strName[i] != 0)
			{
				w8((u8)testJoy_strName[i]);
			}
			else
			{
				w8(0x20);
			}
		}
		for (u32 i = 0; i < 60; i++)
		{
			if (testJoy_strBrand_2[i] != 0)
			{
				w8((u8)testJoy_strBrand_2[i]);
			}
			else
			{
				w8(0x20);
			}
		}
		w16(0xAE01);
		w16(0xF401);
	}
	return 5;

	case 9:
	{
		w32(1 << 24);
		w16(kcode[port] | 0xF901);
		w8(rt[port]);
		w8(lt[port]);
		w8(GetBtFromSgn(joyx[port]));
		w8(GetBtFromSgn(joyy[port]));
		w8(0x80);
		w8(0x80);
	}
	return 8;

	default:
		//printf("unknown MAPLE Frame\n");
		//printState(Command, buffer_in, buffer_in_len);
		break;
	}
	return 0;
}

#endif

void EXPORT_CALL config_keys(u32 id, void* w, void* p)
{
	maple_device_instance* mdd = (maple_device_instance*)p;
	current_port = mdd->port >> 6;
#ifdef BUILD_NAOMI
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), (HWND)w, ConfigKeysDlgProc);
#else
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_ConfigKeys), (HWND)w, ConfigKeysDlgProc);
#endif
}

s32 FASTCALL CreateMain(maple_device_instance* inst, u32 id, u32 flags, u32 rootmenu)
{
	inst->data = inst;
	wchar temp[512];
	if (id <= 1)
	{
		u32 ckid = host.AddMenuItem(rootmenu, -1, L"Config Controls", config_keys, 0);
		u32 asd = host.AddMenuItem(rootmenu, -1, L"Reload Controls", ReloadControlSettings, 0);
		MenuItem mi;
		mi.PUser = inst;
		host.SetMenuItem(ckid, &mi, MIM_PUser);
	}
	if (id == 0)
	{
#ifdef BUILD_NAOMI
		inst->dma = ControllerDMA_naomi;
#else
		inst->dma = ControllerDMA;
#endif
		swprintf_s(temp, L"Controller[winhook] : 0x%02X", inst->port);
	}
	//else if (id == 1)
	//{
		//inst->dma = ControllerDMA_Remote;
		//swprintf_s(temp, L"Controller[winhook, remote] : 0x%02X", inst->port);
	//}

	host.AddMenuItem(rootmenu, -1, temp, 0, 0);
	return rv_ok;
	//printf("Created instance of device %s on port 0x%x\n",dev->name,port);
}

s32 FASTCALL CreateSub(maple_subdevice_instance* inst, u32 id, u32 flags, u32 rootmenu)
{
	wchar wtemp[512];
	u32 mitem = host.AddMenuItem(rootmenu, -1, L"Show VMU", vmu_showwindow, g_ShowVMU);

	inst->data = malloc(sizeof(VMU_info));

	// If we're Offline use our Offline VMU
	if (BEARPlay_Online == 1) {
		if (BEARPlay_Hosting == 1) {
			sprintf(((VMU_info*)inst->data)->file, "vmu_data_host.bin", inst->port);
			swprintf_s(wtemp, L"VMU :vmu_data_host.bin", inst->port);
			host.AddMenuItem(rootmenu, -1, wtemp, 0, 0);
		}
		else {
			sprintf(((VMU_info*)inst->data)->file, "vmu_data_client.bin", inst->port);
			swprintf_s(wtemp, L"VMU :vmu_data_client.bin", inst->port);
			host.AddMenuItem(rootmenu, -1, wtemp, 0, 0);
		}
	}
	else
	{
		sprintf(((VMU_info*)inst->data)->file, "vmu_data_host.bin", inst->port);
		swprintf_s(wtemp, L"VMU :vmu_data_host.bin", inst->port);
		host.AddMenuItem(rootmenu, -1, wtemp, 0, 0);
	}

	((VMU_info*)inst->data)->lcd.handle = 0;
	FILE* f = fopen(((VMU_info*)inst->data)->file, "rb");

	if (!f)
		f = fopen("data\\vmu_default.bin", "rb");
	if (f)
	{
		fread(((VMU_info*)inst->data)->data, 1, 128 * 1024, f);
		fclose(f);
	}

	inst->dma = VmuDMA;
	VMU_info* dev = (VMU_info*)inst->data;

	dev->lcd.handle = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_LCD), 0, VMULCDProc, (LPARAM)dev);

	MenuItem mi;
	mi.PUser = dev->lcd.handle;
	host.SetMenuItem(mitem, &mi, MIM_PUser);

	if (vmu_bmi.bmiHeader.biSize == 0)
	{
		memset(&vmu_bmi, 0, sizeof(BITMAPINFO));
		BITMAPINFO& bmi = vmu_bmi;

		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biBitCount = 16;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biHeight = 32;
		bmi.bmiHeader.biWidth = 48;
		bmi.bmiHeader.biPlanes = 1;
		//memset(&bmi.bmiColors[0],0x00,4);
		//memset(&bmi.bmiColors[1],0xFF,4);
	}

	memset(dev->lcd.bitmap, 0xff, 32 * 48 * 2);

	RECT rc = { 0,0,48 * 3,32 * 3 };
	AdjustWindowRectEx(&rc, GetWindowLong(dev->lcd.handle, GWL_STYLE), FALSE, GetWindowLong(dev->lcd.handle, GWL_EXSTYLE));
	static int lastPosX = 0;
	static int lastPosY = 0;
	SetWindowPos(dev->lcd.handle, NULL, 32 + lastPosX * 192, 32 + lastPosY * 128, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE);
	lastPosX++;
	if (lastPosX > 2)
	{
		lastPosY++;
		lastPosX = 0;
	}
	if (lastPosY > 4)
		lastPosY = 0;
	wchar windowtext[512];
	swprintf_s(windowtext, L"nullDC VMU %c%d", 'A' + (inst->port >> 6), (int)(log10f((float)(inst->port & 31)) / log10f(2.0f)));
	SetWindowText(dev->lcd.handle, windowtext);
	EnableWindow(dev->lcd.handle, TRUE);
	return rv_ok;
}


void drawRectangle(HWND hwnd, const int x, const int y, int RECT_WIDTH, int RECT_HEIGHT)
{
	// obtain a handle to the device context
	HDC hdc = GetDC(hwnd);

	// RECT_WIDTH and RECT_HEIGHT are elsewhere defined
	// draw rectangle
	Rectangle(hdc, x - RECT_WIDTH / 2, y - RECT_HEIGHT / 2, x + RECT_WIDTH / 2, y + RECT_HEIGHT / 2);

	// release the DC
	ReleaseDC(hwnd, hdc);
}

bool BearConnectStarted = false;
s32 FASTCALL Init(void* data, u32 id, maple_init_params* params)
{

	if (BearConnectStarted == false)
	{
		std::thread rcd(&BearConnect);
		rcd.detach();
		BearConnectStarted = true;
	}

	while (Connected == false) { SDL_Delay(1); }

#ifdef BUILD_NAOMI
	if (BEARPlay_Record == 1 && BEARPlay_Playback == 0) {
		std::thread rcd(&BEARRecord); rcd.detach();
	}
#endif

	// MultiThreaded Inputs
	std::thread sdl_input_thread(&SDL_InputThread); sdl_input_thread.detach();
	//SDL_InputThread();
	
	//printf("++BEARPlay: Init Finished");
	return rv_ok;
}

void FASTCALL Term(void* data, u32 id) {}

void FASTCALL Destroy(void* data, u32 id) {
	if (id == 2) {
		VMU_info* dev = (VMU_info*)data;
		if (dev->lcd.handle) {
			DestroyWindow(dev->lcd.handle);
		}
		free(data);
	}
}

void EXPORT_CALL dcGetInterface(plugin_interface* info)
{
#define km info->maple
#define c info->common

	info->InterfaceVersion = PLUGIN_I_F_VERSION;
	c.InterfaceVersion = MAPLE_PLUGIN_I_F_VERSION;

	c.Load = Load;
	c.Unload = Unload;
	c.Type = Plugin_Maple;

	wcscpy(c.Name, L"nullDC Maple Devices [" _T(__DATE__) L"]");

	km.CreateMain = CreateMain;
	km.CreateSub = CreateSub;
	km.Init = Init;
	km.Term = Term;
	km.Destroy = Destroy;
	u32 mdi = 0;

#ifdef BUILD_NAOMI
	MMD(L"BEAR JAMMA Controller[WinHook] (" _T(__DATE__) L")", 0);
#else

	//0
	MMD(L"BEAR nullDC Controller [WinHook,Online] (" _T(__DATE__) L")", MDTF_Sub0 | MDTF_Sub1);

	//1
	//MMD(L"BEAR nullDC Controller [WinHook, REMOTE] (" _T(__DATE__) L")", MDTF_Hotplug | MDTF_Sub0 | MDTF_Sub1);

	//2
	MSD(L"BEAR nullDC VMU (" _T(__DATE__) L")", 0);
	//3
	//MMD(L"BEAR nullDC Keyboard [WinHook] (" _T(__DATE__) L")", MDTF_Hotplug);

	//4
	//MMD(L"BEAR nullDC Controller [no input] (" _T(__DATE__) L")", MDTF_Hotplug | MDTF_Sub0 | MDTF_Sub1);

	//5
	//MMD(L"BEAR nullDC Mouse [WinHook] (" _T(__DATE__) L")", MDTF_Hotplug);
#endif

	MDLE();
}

void EXPORT_CALL ReloadControlSettings(u32 id, void* w, void* p)
{
	host.ConfigExists(L"BEARJamma", L"Whateves");
	SDL_GameControllerAddMappingsFromFile("bearcontrollerdb.txt");

	for (int port = 0; port < 2; port++) {
		for (int i = 0; joypad_settings_K[i].name; i++) {
			wchar temp[512];
			swprintf_s(temp, L"BPort%c_%s", port + 'A', &joypad_settings_K[i].name[4]);
			//printf("Loading key: %ls | %d\n", temp,port);
			host.ConfigLoadStr(L"BEARJamma", temp, joypad_settings[port][i].KC, joypad_settings_K[i].KC);
		}

		wchar temp[512];
		swprintf_s(temp, L"BPort%c_%s", port + 'A', L"Joystick");
		JoyToUse[port] = host.ConfigLoadInt(L"BEARJamma", temp, -1);

		swprintf_s(temp, L"BPort%c_%s", port + 'A', L"Deadzone");
		deadzone[port] = host.ConfigLoadInt(L"BEARJamma", temp, 0);

		swprintf_s(temp, L"BPort%c_%s", port + 'A', L"Peripheral");
		Peripheral[port] = host.ConfigLoadInt(L"BEARJamma", temp, 0);

	}

	for (int _i = 0; _i < 2 - BEARPlay_Online; _i++) {
		if (JoyToUse[_i] >= 0) {
			if (joy[_i] != NULL) {
				SDL_GameControllerClose(joy[_i]);
				joy[_i] = NULL;
			}

			if (JoyToUse[_i] != -1) {
				joy[_i] = SDL_GameControllerOpen(JoyToUse[_i]);
				if (joy[_i] == NULL) {
					printf("++BEARPlay: Port: %d using Keyboard.\n", _i + 1);
				}
				else {
					printf("++BEARPlay: Port: %d using Controller: %s\n", _i + 1, SDL_GameControllerName(joy[_i]));
				}
			}
			else {
				if (joy[_i] != NULL) {
					SDL_GameControllerClose(joy[_i]);
					joy[_i] = NULL;
				}
				printf("++BEARPlay: Port: %d using Keyboard.\n", _i + 1);
				printf("Joy: %d\n", joy[_i]);
			}
			

		}
		else {
			if (joy[_i] != NULL) {
				SDL_GameControllerClose(joy[_i]);
				joy[_i] = NULL;
			}
			printf("++BEARPlay: Port: %d using Keyboard.\n", _i + 1);
		}
	}
}

void LoadSettings()
{
	ReloadControlSettings(0,0,0);
	// Check if stuff exists in the configs
	if (!host.ConfigExists(L"BEARPlay", L"Online")) { host.ConfigSaveInt(L"BEARPlay", L"Online", BEARPlay_Online); }
	if (!host.ConfigExists(L"BEARPlay", L"Host")) { host.ConfigSaveStr(L"BEARPlay", L"Host", BEARPlay_Host); }
	if (!host.ConfigExists(L"BEARPlay", L"Hosting")) { host.ConfigSaveInt(L"BEARPlay", L"Hosting", BEARPlay_Hosting); }
	if (!host.ConfigExists(L"BEARPlay", L"Port")) { host.ConfigSaveInt(L"BEARPlay", L"Port", BEARPlay_Port); }
	if (!host.ConfigExists(L"BEARPlay", L"Delay")) { host.ConfigSaveInt(L"BEARPlay", L"Delay", BEARPlay_Delay); }
	if (!host.ConfigExists(L"BEARPlay", L"Record")) { host.ConfigSaveInt(L"BEARPlay", L"Record", BEARPlay_Record); }
	if (!host.ConfigExists(L"BEARPlay", L"Playback")) { host.ConfigSaveInt(L"BEARPlay", L"Playback", BEARPlay_Playback); }
	if (!host.ConfigExists(L"BEARPlay", L"File")) { host.ConfigSaveStr(L"BEARPlay", L"File", BEARPlay_File); }
	if (!host.ConfigExists(L"BEARPlay", L"AllowSpectators")) { host.ConfigSaveInt(L"BEARPlay", L"AllowSpectators", BEARPlay_AllowSpectators); }
	if (!host.ConfigExists(L"BEARPlay", L"Spectator")) { host.ConfigSaveInt(L"BEARPlay", L"Spectator", BEARPlay_Spectator); }
	if (!host.ConfigExists(L"BEARPlay", L"P1Name")) { host.ConfigSaveStr(L"BEARPlay", L"P1Name", BEARPlay_P1Name); }
	if (!host.ConfigExists(L"BEARPlay", L"P2Name")) { host.ConfigSaveStr(L"BEARPlay", L"P2Name", BEARPlay_P2Name); }
	if (!host.ConfigExists(L"BEARPlay", L"GameName")) { host.ConfigSaveStr(L"BEARPlay", L"GameName", BEARPlay_GameName); }
	if (!host.ConfigExists(L"BEARPlay", L"GameRom")) { host.ConfigSaveStr(L"BEARPlay", L"GameRom", BEARPlay_GameRom); }
	if (!host.ConfigExists(L"BEARPlay", L"Region")) { host.ConfigSaveStr(L"BEARPlay", L"Region", BEARPlay_Region); }

	BEARPlay_Online = host.ConfigLoadInt(L"BEARPlay", L"Online", 0);
	host.ConfigLoadStr(L"BEARPlay", L"Host", BEARPlay_Host, L"127.0.0.1");
	BEARPlay_Hosting = host.ConfigLoadInt(L"BEARPlay", L"Hosting", 0);
	BEARPlay_Port = host.ConfigLoadInt(L"BEARPlay", L"Port", 27886);
	BEARPlay_Delay = host.ConfigLoadInt(L"BEARPlay", L"Delay", 1);

	BEARPlay_Record = host.ConfigLoadInt(L"BEARPlay", L"Record", 0);
	BEARPlay_Playback = host.ConfigLoadInt(L"BEARPlay", L"Playback", 0);
	host.ConfigLoadStr(L"BEARPlay", L"File", BEARPlay_File, L"0");

	BEARPlay_AllowSpectators = host.ConfigLoadInt(L"BEARPlay", L"AllowSpectators", 0);
	BEARPlay_Spectator = host.ConfigLoadInt(L"BEARPlay", L"Spectator", 0);

	host.ConfigLoadStr(L"BEARPlay", L"P1Name", BEARPlay_P1Name, L"0");
	host.ConfigLoadStr(L"BEARPlay", L"P2Name", BEARPlay_P2Name, L"0");

	host.ConfigLoadStr(L"BEARPlay", L"GameName", BEARPlay_GameName, L"0");
	host.ConfigLoadStr(L"BEARPlay", L"GameRom", BEARPlay_GameRom, L"0");

	host.ConfigLoadStr(L"BEARPlay", L"Region", BEARPlay_Region, L"JPN");

	BEARPlay_debug = host.ConfigLoadInt(L"BEARPlay", L"Debug", 0);

	// DREAMCAST STUFF
	g_ShowVMU = host.ConfigLoadInt(L"drkMaple", L"ShowVMU", 1);
}

void SaveSettings()
{
	// BEAR
	for (int port = 0; port < 2; port++)
	{
		for (int i = 0; joypad_settings_K[i].name; i++)
		{
			wchar temp[512];
			swprintf_s(temp, L"BPort%c_%s", port + 'A', &joypad_settings_K[i].name[4]);
			host.ConfigSaveStr(L"BEARJamma", temp, joypad_settings[port][i].KC);
		}
	}

	host.ConfigSaveInt(L"BEARPlay", L"Debug", BEARPlay_debug);
	host.ConfigSaveInt(L"drkMaple", L"ShowVMU", g_ShowVMU);
}

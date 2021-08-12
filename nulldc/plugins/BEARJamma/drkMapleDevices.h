#pragma once

// This is all the boring shit i don't want to look at constantly

#include "SDL\include\SDL.h"
#include "SDL\include\SDL_net.h"
#include "nullDC\plugins\plugin_header.h"
#include <memory.h>
#include <math.h>
#include <random>
#include <SdkDdkver.h>
#include <windowsx.h>
#include <winsock2.h>
#include <windows.h>
#include <WS2tcpip.h>
#include <time.h>
#include <iostream>
#include <cstdlib>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <cstdio>
#include <stdlib.h>
#include <tchar.h>
#include <string.h>
#include <string>
#include <commctrl.h>
#include "resource.h"
#include <comdef.h>
#include <conio.h>
#include <thread>
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <iostream>
#include <fstream>
#include <bitset>
#include <Shlwapi.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define _WIN32_WINNT _WIN32_WINNT_VISTA

// Functions
void LoadSettings();
void SaveSettings();
void DoSDLInputRoll();
void Debug_WriteLine(const char *fmt, ...);
void BEAReceiver();
void ErrorMessageBoxPopUp(wchar* msg);
void BEARSpectatorReceiver();
void BEARSpectatorSender(int SpectatorID);
void BEARSpectatorDataSenderReciever(int SpectatorID);
void EXPORT_CALL ReloadControlSettings(u32 id, void* w, void* p);
void drawRectangle(HWND hwnd, const int x, const int y, int RECT_WIDTH, int RECT_HEIGHT);

// NullDC Shit

emu_info host; // NullDC
static std::mt19937 random_dev;

typedef INT_PTR CALLBACK dlgp(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Maple Shit

char testJoy_strName[64] = "Dreamcast Controller\0";
char testJoy_strName_nul[64] = "Null Dreamcast Controler\0";
char testJoy_strName_net[64] = "Net Dreamcast Controler\0";
char testJoy_strName_vmu[64] = "Visual Memory\0";
char testJoy_strName_kbd[64] = "Emulated Dreamcast Keyboard\0";
char testJoy_strName_mouse[64] = "Emulated Dreamcast Mouse\0";
char testJoy_strName_dreameye_1[64] = "Dreamcast Camera Flash  Devic\0";
char testJoy_strName_dreameye_2[64] = "Dreamcast Camera Flash LDevic\0";
char testJoy_strName_mic[64] = "MicDevice for Dreameye\0";
char testJoy_strBrand[69] = "Faked by drkIIRaziel && ZeZu - Netplay by RossenX , made for nullDC\0";
char testJoy_strBrand_2[64] = "Produced By or Under License From SEGA ENTERPRISES,LTD.\0";

char EEPROM[0x100];
bool EEPROM_loaded = false;
int g_ShowVMU;

#define SWAP32(val) ((u32) ( \
	(((u32) (val) & (u32) 0x000000ffU) << 24) | \
	(((u32) (val) & (u32) 0x0000ff00U) <<  8) | \
	(((u32) (val) & (u32) 0x00ff0000U) >>  8) | \
	(((u32) (val) & (u32) 0xff000000U) >> 24)))

#define MMD(name,flags) \
	wcscpy(km.devices[mdi].Name,name);	\
	km.devices[mdi].Type=MDT_Main;	\
	km.devices[mdi].Flags= flags;	\
	mdi++;

#define MSD(name,flags)	\
	wcscpy(km.devices[mdi].Name,name);	\
	km.devices[mdi].Type=MDT_Sub;	\
	km.devices[mdi].Flags= flags;	\
	mdi++;

#define MDLE() km.devices[mdi].Type=MDT_EndOfList;
#define __T(x) L##x
#define _T(x) __T(x)

#define sk(num,key)kb_map[key]=0x##num;
#define w32(data) *(u32*)buffer_out_b=(data);buffer_out_b+=4;buffer_out_len+=4
#define w16(data) *(u16*)buffer_out_b=(data);buffer_out_b+=2;buffer_out_len+=2
#define w8(data) *(u8*)buffer_out_b=(data);buffer_out_b+=1;buffer_out_len+=1

#define dbgbreak {while(1) __noop;}
#define verify(x) if((x)==false){ printf("Verify Failed  : " #x "\n in %s -> %s : %d \n",__FUNCTION__,__FILE__,__LINE__); dbgbreak;}
#pragma pack(1)

enum MapleFunctionID
{
	MFID_0_Input = 0x01000000,		//DC Controller, Lightgun buttons, arcade stick .. stuff like that
	MFID_1_Storage = 0x02000000,		//VMU , VMS
	MFID_2_LCD = 0x04000000,		//VMU
	MFID_3_Clock = 0x08000000,		//VMU
	MFID_4_Mic = 0x10000000,		//DC Mic (, dreameye too ?)
	MFID_5_ARGun = 0x20000000,		//Artificial Retina gun ? seems like this one was never developed or smth -- i only remember of lightguns
	MFID_6_Keyboard = 0x40000000,		//DC Keyboard
	MFID_7_LightGun = 0x80000000,		//DC Lightgun
	MFID_8_Vibration = 0x00010000,		//Puru Puru
	MFID_9_Mouse = 0x00020000,		//DC Mouse
	MFID_10_StorageExt = 0x00040000,		//Storage ? propably never used
	MFID_11_Camera = 0x00080000,		//DreamEye
};
enum MapleDeviceCommand
{
	MDC_DeviceRequest = 0x01,			//7 words.Note : Initialises device
	MDC_AllStatusReq = 0x02,			//7 words + device depedant ( seems to be 8 words)
	MDC_DeviceReset = 0x03,			//0 words
	MDC_DeviceKill = 0x04,			//0 words
	MDC_DeviceStatus = 0x05,			//Same as MDC_DeviceRequest ?
	MDC_DeviceAllStatus = 0x06,			//Same as MDC_AllStatusReq ?
	//various Functions
	MDCF_GetCondition = 0x09,				//FT
	MDCF_GetMediaInfo = 0x0A,				//FT,PT,3 pad
	MDCF_BlockRead = 0x0B,				//FT,PT,Phase,Block #
	MDCF_BlockWrite = 0x0C,				//FT,PT,Phase,Block #,data ...
	MDCF_GetLastError = 0x0D,				//FT,PT,Phase,Block #
	MDCF_SetCondition = 0x0E,				//FT,data ...
	MDCF_MICControl = 0x0F,				//FT,MIC data ...
	MDCF_ARGunControl = 0x10,				//FT,AR-Gun data ...
};
enum MapleDeviceRV
{
	MDRS_DeviseStatus = 0x05,			//28 words
	MDRS_DeviseStatusAll = 0x06,		//28 words + device depedant data
	MDRS_DeviceReply = 0x07,			//0 words
	MDRS_DataTransfer = 0x08,			//FT,depends on the command

	MDRE_UnknownFunction = 0xFE,		//0 words
	MDRE_UnknownCmd = 0xFD,			//0 words
	MDRE_TransminAgain = 0xFC,		//0 words
	MDRE_FileError = 0xFB,			//1 word, bitfield
	MDRE_LCDError = 0xFA,				//1 word, bitfield
	MDRE_ARGunError = 0xF9,			//1 word, bitfield
};

struct VMU_info
{
	u8 data[256 * 1024];
	char file[512];
	struct {
		HWND handle;
		BYTE data[192];
		WORD bitmap[48 * 32];
		//		BITMAPINFO bmi;
		//		bool visible;
	} lcd;
};
BITMAPINFO vmu_bmi;

struct _NaomiState
{
	BYTE Cmd;
	BYTE Mode;
	BYTE Node;
};
_NaomiState State;

typedef struct {
	u16 total_size;
	u16 partition_number;
	u16 system_area_block;
	u16 fat_area_block;
	u16 number_fat_areas_block;
	u16 file_info_block;
	u16 number_info_blocks;
	u8 volume_icon;
	u8 reserved1;
	u16 save_area_block;
	u16 number_of_save_blocks;
	u16 reserverd0;
}
maple_getvmuinfo_t;

u8 GetBtFromSgn(s8 val) { return val + 128; }
//VMU
INT_PTR CALLBACK VMULCDProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	VMU_info* dev = (VMU_info*)(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	switch (msg)
	{
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		break;
	case WM_INITDIALOG:
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
	}
	return TRUE;
	case WM_ERASEBKGND:
		return TRUE;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		int x = 0;//(ps.rcPaint.left)/3;
		int y = 0;//(ps.rcPaint.top)/3;
		int wx = 48;//(ps.rcPaint.right+2)/3-x;
		int wy = 32;//(ps.rcPaint.bottom+2)/3-y;


		SetStretchBltMode(hdc, BLACKONWHITE);

		StretchDIBits(hdc, x * 3, y * 3, wx * 3, wy * 3,
			x, y, wx, wy,
			dev->lcd.bitmap, &vmu_bmi, DIB_RGB_COLORS, SRCCOPY);

		EndPaint(hWnd, &ps);
	}
	break;
	//case WM_ACTIVATE:
	//	printf("VMU GOT FOCUS !\n");
	//	break;
	}
	return FALSE;
}

u32 FASTCALL VmuDMA(void* device_instance, u32 Command, u32* buffer_in, u32 buffer_in_len, u32* buffer_out, u32& buffer_out_len)
{
	VMU_info* dev = (VMU_info*)(device_instance);
	u8*buffer_out_b = (u8*)buffer_out;
	switch (Command)
	{
	case MDC_DeviceRequest:
	{
		w32(MFID_1_Storage | MFID_2_LCD | MFID_3_Clock);
		w32(0x403f7e7e);
		w32(0x00100500);
		w32(0x00410f00);
		w8(0xFF);
		w8(0);
		for (u32 i = 0; i < 30; i++) {
			if (testJoy_strName_vmu[i]) {
				w8((u8)testJoy_strName_vmu[i]);
			}
			else {
				w8(' ');
			}
		}
		for (u32 i = 0; i < 60; i++) {
			if (testJoy_strBrand_2[i]) {
				w8((u8)testJoy_strBrand_2[i]);
			}
			else {
				w8(' ');
			}
		}
		w16(0x007c);
		w16(0x0082);
		return MDRS_DeviseStatus;
	}
	case MDCF_GetMediaInfo:
	{
		if (buffer_in[0] & MFID_1_Storage)
		{
			buffer_out[0] = MFID_1_Storage;//is that ok ?
			maple_getvmuinfo_t* vmui = (maple_getvmuinfo_t*)(&buffer_out[1]);
			//ZeroMemory(pMediaInfo,sizeof(TMAPLE_MEDIAINFO));
			memset(vmui, 0, sizeof(maple_getvmuinfo_t));
			vmui->total_size = 0xFF;//0x7FFF;//0xFF
			vmui->system_area_block = 0xFF;//0x7FFF;//0xff
			vmui->fat_area_block = 0xfe;//0x7F00;	//0xfe
			vmui->number_fat_areas_block = 1;//256;//1
			vmui->volume_icon = 0x0;//0
			vmui->save_area_block = 0xc8;//?
			vmui->number_of_save_blocks = 0x1f;
			//pMediaInfo->volume_icon = 0x0;
			vmui->file_info_block = 0xfd;//0x7E00;//0xfd
			vmui->number_info_blocks = 0xd;//0x100;//0xd
			vmui->reserverd0 = 0x0000;
			buffer_out_len = 4 + (sizeof(maple_getvmuinfo_t));
			return MDRS_DataTransfer;//data transfer
		}
		else if (buffer_in[0] & MFID_2_LCD)
		{
			if (buffer_in[1] != 0)
			{
				//printf("VMU: MDCF_GetMediaInfo -> bad input |%08X|, returning MDRE_UnknownCmd\n",buffer_in[0]);
				return MDRE_UnknownCmd;
			}
			else
			{
				w32(MFID_2_LCD);

				w8(47);				//X dots -1
				w8(31);				//Y dots -1
				w8(((1) << 4) | (0));		//1 Color, 0 contrast levels
				w8(0);					//Padding
				return MDRS_DataTransfer;
			}
		}
		else
		{
			//printf("VMU: MDCF_GetMediaInfo -> Bad function used |%08X|, returning -2\n",buffer_in[0]);
			return MDRE_UnknownFunction;//bad function
		}

	case MDCF_BlockRead:
		if (buffer_in[0] & MFID_1_Storage)
		{
			//VMU_info* dev=(VMU_info*)((*device_instance).data);

			buffer_out[0] = MFID_1_Storage;
			u32 Block = (SWAP32(buffer_in[1])) & 0xffff;
			buffer_out[1] = buffer_in[1];
			if (Block > 255)
			{
				//printf("Block read : %d\n",Block);
				//printf("BLOCK READ ERROR\n");
				Block &= 255;
			}
			memcpy(&buffer_out[2], (dev->data) + Block * 512, 512);
			buffer_out_len = (512 + 8);
			return MDRS_DataTransfer;//data transfer
		}
		else if (buffer_in[0] & MFID_2_LCD)
		{
			buffer_out[0] = MFID_2_LCD;
			buffer_out[1] = buffer_in[1];
			memcpy(&buffer_out[2], (dev->lcd.data), 192);
			buffer_out_len = (192 + 8);
			return MDRS_DataTransfer;//data transfer
		}
		else if (buffer_in[0] & MFID_3_Clock)
		{
			if (buffer_in[1] != 0 || buffer_in_len != 8)
			{
				//printf("VMU: Block read: MFID_3_Clock : invalid params \n");
				return MDRE_TransminAgain;		//invalid params
			}
			else
			{
				buffer_out[0] = MFID_3_Clock;
				buffer_out_len = 12;

				time_t now;
				time(&now);
				tm* timenow = localtime(&now);
				u8* timebuf = (u8*)&buffer_in[1];	//YY M D H M S DotY

				timebuf[0] = (timenow->tm_year + 1900) % 256;
				timebuf[1] = (timenow->tm_year + 1900) / 256;

				timebuf[2] = timenow->tm_mon + 1;
				timebuf[3] = timenow->tm_mday;

				timebuf[4] = timenow->tm_hour;
				timebuf[5] = timenow->tm_min;
				timebuf[6] = timenow->tm_sec;
				timebuf[7] = 0;

				//printf("VMU: CLOCK Read-> datetime is %04d/%02d/%02d ~ %02d:%02d:%02d!\n",timebuf[0]+timebuf[1]*256,timebuf[2],timebuf[3],timebuf[4],timebuf[5],timebuf[6]);

				return MDRS_DataTransfer;//transfer reply ...
			}
		}
		else
		{
			//printf("VMU: cmd MDCF_BlockRead -> Bad function used, returning -2\n");
			return MDRE_UnknownFunction;//bad function
		}
		break;
	}

	case MDCF_BlockWrite:
	{
		if (buffer_in[0] & MFID_1_Storage)
		{
			u32 Block = (SWAP32(buffer_in[1])) & 0xffff;
			u32 Phase = ((SWAP32(buffer_in[1])) >> 16) & 0xff;
			//printf("Block wirte : %d:%d , %d bytes\n",Block,Phase,(buffer_in_len-8));
			memcpy(&dev->data[Block * 512 + Phase*(512 / 4)], &buffer_in[2], (buffer_in_len - 8));
			buffer_out_len = 0;
			FILE* f = fopen(dev->file, "wb");
			if (f)
			{
				fwrite(dev->data, 1, 128 * 1024, f);
				fclose(f);
			}
			else
			{
				//printf("Failed to open %s for saving vmu data\n",dev->file);
				//return MDRE_FileError; -> this also has to return an error bitfield, will do so later on ...
			}
			return MDRS_DeviceReply;//just ko
		}
		else if (buffer_in[0] & MFID_2_LCD)
		{
			memcpy(dev->lcd.data, &buffer_in[2], 192);
			//Update lcd window
			if (g_ShowVMU)
			{
				ShowWindow(dev->lcd.handle, SHOW_OPENNOACTIVATE);
			}
			//if(LCDBitmap)
			{
				const WORD white = 0xffff;
				const WORD black = 0x0000;

				for (int y = 0; y < 32; ++y)
				{
					WORD *dst = dev->lcd.bitmap + y * 48;
					BYTE *src = dev->lcd.data + 6 * y + 5;
					for (int x = 0; x < 48 / 8; ++x)
					{
						BYTE val = *src;
						for (int m = 0; m < 8; ++m)
						{
							if (val&(1 << (m)))
								*dst++ = black;
							else
								*dst++ = white;
						}
						--src;
					}
				}
				InvalidateRect(dev->lcd.handle, NULL, FALSE);
			}

			return  MDRS_DeviceReply;//just ko
		}
		else if (buffer_in[0] & MFID_3_Clock)
		{
			if (buffer_in[1] != 0 || buffer_in_len != 16)
				return MDRE_TransminAgain;	//invalid params ...
			else
			{
				u8* timebuf = (u8*)&buffer_in[2];	//YY M D H M S DotY
													//printf("VMU: CLOCK Write-> datetime is %04d/%02d/%02d ~ %02d:%02d:%02d! Nothing set tho ...\n",timebuf[0]+timebuf[1]*256,timebuf[2],timebuf[3],timebuf[4],timebuf[5],timebuf[6]);
				return  MDRS_DeviceReply;//ok !
			}
		}
		else
		{
			//printf("VMU: cmd MDCF_BlockWrite -> Bad function used, returning MDRE_UnknownFunction\n");
			return  MDRE_UnknownFunction;//bad function
		}
		break;
	}

	case MDCF_GetLastError:
		return MDRS_DeviceReply;//just ko
		break;

	case MDCF_SetCondition:
	{
		if (buffer_in[0] & MFID_3_Clock)
		{
			if (buffer_in[1])
			{
				u8* beepbuf = (u8*)&buffer_in[1];
				//printf("BEEP : %d %d | %d %d\n",beepbuf[0],beepbuf[1],beepbuf[2],beepbuf[3]);
			}
			return  MDRS_DeviceReply;//just ko
		}
		else
		{
			//printf("VMU: cmd MDCF_SetCondition -> Bad function used, returning MDRE_UnknownFunction\n");
			return MDRE_UnknownFunction;//bad function
		}
		break;
	}

	default:
		//printf("unknown MAPLE COMMAND %d\n",Command);
		return MDRE_UnknownCmd;
	}

}

void EXPORT_CALL vmu_showwindow(u32 id, void* w, void* p)
{
	if (g_ShowVMU) {
		ShowWindow((HWND)p, SW_HIDE);
		g_ShowVMU = 0;
	}
	else {
		ShowWindow((HWND)p, SHOW_OPENNOACTIVATE);
		g_ShowVMU = 1;
	}

	host.SetMenuItemStyle(id, g_ShowVMU ? MIS_Checked : 0, MIS_Checked);

	SaveSettings();
}

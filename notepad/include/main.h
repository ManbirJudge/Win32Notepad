#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include "framework.h"
#include "resource.h"

#include "nlohmann/json.hpp"

using json = nlohmann::json;

#define WS_CHILD_DEF WS_VISIBLE | WS_CHILD | WS_TABSTOP

#define WIDTH 1000
#define HEIGHT 600

const std::string SETTINGS_FILE_PATH = "settings.json";

typedef struct SettingsStruct
{
	LOGFONTW font;
	int zoom;
	bool showStatusBar;
} Settings;

const int sizeP1 = 120;
const int sizeP3 = 84;
const int sizeP4 = 210;
const int sizeP5 = 180;

int dpi;

HINSTANCE hInst;

HWND hwndMain;
HMENU hMenu;

HWND hwndEdit;
HWND hwndStatus;

HWND hwndFindDlg;

HFONT hfDef;
HFONT hfEdit;

bool fileOpen = false;
bool changesMade = false;
bool shouldReCalcFinds = true;

std::wstring curFilePath;

bool findDlgOpen = false;
std::vector<std::pair<size_t, size_t>> finds;
int nextFindI = -1;
bool selectedFind = false;

bool showStatusBar = true;
bool wordWrap = false;

int zoom = 100;  // %

void createDebugConsole();

std::wstring getWndTxt(HWND);
std::wstring getDlgItemTxt(HWND, int);

LRESULT CALLBACK winProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK findDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK replaceDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK gotoDlgProc(HWND, UINT, WPARAM, LPARAM);

BOOL CALLBACK childEnumProc(HWND, LPARAM);

void createMenus(HWND);
void createCtrls(HWND);
void createStatusBar(HWND);

inline Settings getDefSettings()
{
    LOGFONTW lfDef{ 0 };

    lfDef.lfHeight = -16;
    lfDef.lfWidth = 0;
    lfDef.lfEscapement = 0;
    lfDef.lfOrientation = 0;
    lfDef.lfWeight = FW_NORMAL;
    lfDef.lfItalic = FALSE;
    lfDef.lfUnderline = FALSE;
    lfDef.lfStrikeOut = FALSE;
    lfDef.lfCharSet = DEFAULT_CHARSET;
    lfDef.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lfDef.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lfDef.lfQuality = DEFAULT_QUALITY;
    lfDef.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;

    wcscpy_s(lfDef.lfFaceName, L"Courier New");

    Settings defSettings{
        lfDef,
        100,
        true
    };

    return defSettings;
};
void loadSettings();
void saveSettings();

void openFile();
bool createNewFile();
void saveFile();
void printFile();
bool canCloseFile();

void openFindDlg();
void openReplaceDlg();

void calcFinds();
void calcNextFindI();
void selNextFind();
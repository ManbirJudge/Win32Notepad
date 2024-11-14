#include "main.h"

void createDebugConsole()
{
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    std::wcout << L"[DEBUG] Debug console window allocated." << std::endl;
}

// ---
std::wstring getWndTxt(HWND hWnd)
{
    int len = GetWindowTextLengthW(hWnd);
    std::wstring txt(len, '\0');

    GetWindowTextW(hWnd, &txt[0], len + 1);

    return txt;
}

std::wstring getDlgItemTxt(HWND hDlg, int nIDDlgItem)
{
    HWND hWnd = GetDlgItem(hDlg, nIDDlgItem);
    return getWndTxt(hWnd);
}

// ---
int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow
) {
    hInst = hInstance;

    // debugging
    createDebugConsole();

    // initalizing common controls
    INITCOMMONCONTROLSEX icex{ 0 };

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_USEREX_CLASSES | ICC_STANDARD_CLASSES | ICC_DATE_CLASSES | ICC_HOTKEY_CLASS | ICC_INTERNET_CLASSES;

    InitCommonControlsEx(&icex);

    LoadLibraryW(L"Msftedit.dll");

    // default font
    NONCLIENTMETRICS ncm;

    ZeroMemory(&ncm, sizeof(NONCLIENTMETRICS));
    ncm.cbSize = sizeof(NONCLIENTMETRICS);

    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, FALSE);
    hfDef = CreateFontIndirect(&ncm.lfMessageFont);

    // getting some system info
    HDC hdc = GetDC(NULL);
    dpi = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(NULL, hdc);

    std::cout << "[_INFO] DPI: " << dpi << std::endl;

    // registering window class
    WNDCLASSEXW wcex{ 0 };

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = winProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
    wcex.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH) COLOR_WINDOW;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"MainWindow";
    wcex.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

    if (!RegisterClassExW(&wcex)) return -1;

    // creating windowE
    hwndMain = CreateWindowW(
        L"MainWindow", 
        L"Notepad", 
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, 0,
        CW_USEDEFAULT,  0, 
        NULL, NULL, hInst, NULL
    );
    // TODO: EnumChildWindows(hwndMain, childEnumProc, (LPARAM) &hfDef);

    ShowWindow(hwndMain, nCmdShow);

    HICON hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
    SendMessage(hwndMain, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    SendMessage(hwndMain, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    
    // keyboard shortcuts (aka accelerators)
    ACCEL accelerators[] = {
        { FCONTROL | FVIRTKEY, 'O', ID_FILE_OPEN },
        { FCONTROL | FVIRTKEY, 'S', ID_FILE_SAVE },
        { FCONTROL | FSHIFT | FVIRTKEY, 'S', ID_FILE_SAVEAS },
        { FCONTROL | FVIRTKEY, 'P', ID_FILE_PRINT },
        { FCONTROL | FVIRTKEY, 'Z', ID_EDIT_UNDO },
        { FCONTROL | FVIRTKEY, 'F', ID_EDIT_FIND },
        { FCONTROL | FVIRTKEY, 'H', ID_EDIT_REPLACE },
        { FVIRTKEY, 0x74, ID_EDIT_DATETIME },
    };

    HACCEL hAccelTable = CreateAcceleratorTable(accelerators, 8);

    // message loop
    MSG msg{ 0 };

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(hwndMain, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // exit
    DeleteObject(hfDef);
    DeleteObject(hfEdit);

    return 0;
}

// ---
LRESULT CALLBACK winProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);

        loadSettings();

        createMenus(hWnd);
        createCtrls(hWnd);
        createStatusBar(hWnd);
        
        saveSettings();

        break;
    }

    case WM_SIZE:
    {
        SendMessage(hwndStatus, WM_SIZE, NULL, NULL);

        RECT rcWnd;
        RECT rcStatus;

        GetClientRect(hWnd, &rcWnd);
        GetClientRect(hwndStatus, &rcStatus);

        LONG height;
        if (showStatusBar) height = rcWnd.bottom - rcStatus.bottom;
        else height = rcWnd.bottom;

        SetWindowPos(hwndEdit, NULL, 0, 0, rcWnd.right, height, SWP_NOACTIVATE);

        HLOCAL hloc = LocalAlloc(LHND, sizeof(int) * 5);
        PINT paParts = (PINT) LocalLock(hloc);

        int right = 0;

        paParts[0] = right += sizeP1;
        paParts[1] = right += rcWnd.right - (sizeP1 + sizeP3 + sizeP4 + sizeP5);
        paParts[2] = right += sizeP3;
        paParts[3] = right += sizeP4;
        paParts[4] = (int)rcWnd.right;

        SendMessage(hwndStatus, SB_SETPARTS, (WPARAM)5, (LPARAM)paParts);

        LocalUnlock(hloc);
        LocalFree(hloc);

        break;
    }

    case WM_COMMAND:
    {
        bool handled = false;

        switch (LOWORD(wp))
        {
        case ID_FILE_OPEN:
            openFile();

            handled = true;
            break;

        case ID_FILE_SAVE:
            saveFile();

            handled = true;
            break;

        case ID_FILE_PRINT:
            printFile();

            handled = true;
            break;

        case ID_FILE_EXIT:
            SendMessage(hwndMain, WM_CLOSE, NULL, NULL);

            handled = true;
            break;

        case ID_EDIT_UNDO:
            SendMessage(hwndEdit, EM_UNDO, NULL, NULL);

            handled = true;
            break;

        case ID_EDIT_CUT:
            SendMessage(hwndEdit, WM_CUT, NULL, NULL);

            handled = true;
            break;

        case ID_EDIT_COPY:
            SendMessage(hwndEdit, WM_COPY, NULL, NULL);

            handled = true;
            break;

        case ID_EDIT_PASTE:
            SendMessage(hwndEdit, WM_PASTE, NULL, NULL);

            handled = true;
            break;

        case ID_EDIT_FIND:
            openFindDlg();

            handled = true;
            break;

        case ID_EDIT_REPLACE:
            openReplaceDlg();

            handled = true;
            break;

        case ID_EDIT_SELECTALL:
            SendMessage(hwndEdit, EM_SETSEL, (WPARAM)0, (LPARAM)-1);

            handled = true;
            break;

        case ID_EDIT_DATETIME:
        {
            SYSTEMTIME curDt;
            GetLocalTime(&curDt);

            std::wstringstream ss;
            ss << std::setw(2) << std::setfill(L'0') << curDt.wHour << ":" << std::setw(2) << std::setfill(L'0') << curDt.wMinute << " " << std::setw(2) << std::setfill(L'0') << curDt.wDay << "-" << std::setw(2) << std::setfill(L'0') << curDt.wMonth << "-" << std::setw(2) << std::setfill(L'0') << curDt.wYear;

            std::wstring curDtStr = ss.str();

            SendMessage(hwndEdit, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)curDtStr.c_str());
        }

        case ID_EDIT_GOOGLEIT:
        {
            DWORD start{ 0 };
            DWORD end{ 0 };

            SendMessage(hwndEdit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

            if (!(start == end))
            {
                int selSize = start - end;
                selSize = abs(selSize) + 1;

                std::wstring allTxt(max(end, start), '\0');
                GetWindowText(hwndEdit, &allTxt[0], max(end, start) + 1);
                std::wstring selTxt = allTxt.substr(min(start, end), selSize);

                ShellExecute(NULL, L"open", (L"http://www.google.com/search?q=" + selTxt).c_str(), NULL, NULL, SW_SHOWNORMAL);
            }

            handled = true;
            break;
        }

        case ID_VIEW_STATUSBAR:
        {
            showStatusBar = !showStatusBar;

            if (showStatusBar) {
                ShowWindow(hwndStatus, SW_SHOW);
                CheckMenuItem(hMenu, ID_VIEW_STATUSBAR, MF_CHECKED);
            }
            else {
                ShowWindow(hwndStatus, SW_HIDE);
                CheckMenuItem(hMenu, ID_VIEW_STATUSBAR, MF_UNCHECKED);
            }

            SendMessageW(hwndMain, WM_SIZE, NULL, NULL);
            saveSettings();

            handled = true;
            break;
        }

        case ID_PREFERENCES_CHANGEFONT:
        {
            LOGFONT chosenFontData{ 0 };

            GetObject(hfEdit, sizeof(LOGFONT), &chosenFontData);

            CHOOSEFONTW chooseFontData{ 0 };

            chooseFontData.lStructSize = sizeof(CHOOSEFONT);
            chooseFontData.hwndOwner = hwndMain;
            chooseFontData.hDC = NULL;
            chooseFontData.lpLogFont = &chosenFontData;
            chooseFontData.Flags = CF_APPLY | CF_INACTIVEFONTS | CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL;  // CF_EFFECTS
            chooseFontData.rgbColors = NULL;
            chooseFontData.lCustData = NULL;
            chooseFontData.lpfnHook = NULL;
            chooseFontData.lpTemplateName = NULL;
            chooseFontData.hInstance = hInst;
            chooseFontData.lpszStyle = NULL;
            chooseFontData.nFontType = NULL;
            chooseFontData.nSizeMin = NULL;
            chooseFontData.nSizeMax = NULL;
            
            BOOL res = ChooseFontW(&chooseFontData);

            if (res)
            {
                hfEdit = CreateFontIndirect(&chosenFontData);
                SendMessage(hwndEdit, WM_SETFONT, (WPARAM) hfEdit, TRUE);

                saveSettings();
            }

            handled = true;
            break;
        }
        }

        switch (HIWORD(wp))  // we should check you sent it, but we only have 1 control so no worries
        {
        case EN_CHANGE:
        {
            if (!changesMade) changesMade = true;
            if (!shouldReCalcFinds) shouldReCalcFinds = true;

            int nChar = (int)GetWindowTextLength(hwndEdit);

            SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM)(std::to_wstring(nChar) + L" characters").c_str());
        }
        }

        if (handled) return 0;
        else return DefWindowProc(hWnd, msg, wp, lp);
    }

    case WM_CLOSE:
        if (canCloseFile()) DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        saveSettings();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wp, lp);
    }

    return 0;
}

BOOL CALLBACK findDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case IDC_FIND_NXT:
        {
            calcNextFindI();
            selNextFind();

            return TRUE;
        }

        case IDC_FIND_REPLACE:
        {
            calcNextFindI();
            selNextFind();

            if (nextFindI == -1) return TRUE;

            std::wstring replaceWith = getDlgItemTxt(hDlg, IDC_FIND_REPLACEWITH);
            SendMessageW(hwndEdit, EM_REPLACESEL, (WPARAM) TRUE, (LPARAM) replaceWith.c_str());

            shouldReCalcFinds = true;

            calcFinds();
            selNextFind();

            return TRUE;
        }

        case IDC_FIND_MATCHCASE:
            shouldReCalcFinds = true;
            return TRUE;

        case IDC_FIND_CANCEL:
            EndDialog(hDlg, wp);
            return TRUE;
        }

        switch (HIWORD(wp))
        {
        case EN_CHANGE:
            if ((HWND)lp == GetDlgItem(hDlg, IDC_FIND_TXT)) shouldReCalcFinds = true;
            return TRUE;
        }

        return FALSE;

    case WM_CLOSE:
        EndDialog(hDlg, wp);
        findDlgOpen = false;

        return TRUE;
    }

    return FALSE;
}

BOOL CALLBACK childEnumProc(HWND hWnd, LPARAM lp)
{
    HFONT hfDef = *(HFONT*)lp;
    SendMessage(hWnd, WM_SETFONT, (WPARAM)hfDef, MAKELPARAM(TRUE, 0));
    return TRUE;
}

// ---
void createMenus(HWND hWnd) {
    hMenu = LoadMenuW(hInst, MAKEINTRESOURCE(IDR_MAINMENU));
    SetMenu(hWnd, hMenu);

    CheckMenuItem(hMenu, ID_VIEW_STATUSBAR, showStatusBar ? MF_CHECKED : MF_UNCHECKED);
}

void createCtrls(HWND hWnd) {
    hwndEdit = CreateWindowEx(
        ES_EX_ZOOMABLE, 
        MSFTEDIT_CLASS,
        L"",
        WS_CHILD_DEF | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_EX_ZOOMABLE,
        0, 0, 
        WIDTH, HEIGHT,
        hWnd,
        NULL, 
        hInst, 
        NULL
    );

    SendMessage(hwndEdit, WM_SETFONT, (WPARAM)hfEdit, TRUE);
    SendMessage(hwndEdit, EM_SETEVENTMASK, NULL, (LPARAM)ENM_CHANGE);
}

void createStatusBar(HWND hWnd)
{
    hwndStatus = CreateWindowEx(
        NULL,
        STATUSCLASSNAME,
        L"Open a file to get started ...",
        SBARS_SIZEGRIP |
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0,
        hWnd,
        NULL,
        hInst,
        NULL
    );

    // ---
    RECT rcWnd;
    GetClientRect(hWnd, &rcWnd);

    HLOCAL hloc = LocalAlloc(LHND, sizeof(int) * 5);
    PINT paParts = (PINT)LocalLock(hloc);

    int right = 0;

    paParts[0] = right += sizeP1;
    paParts[1] = right += rcWnd.right - (sizeP1 + sizeP3 + sizeP4 + sizeP5);
    paParts[2] = right += sizeP3;
    paParts[3] = right += sizeP4;
    paParts[4] = (int)rcWnd.right;

    SendMessage(hwndStatus, SB_SETPARTS, (WPARAM)5, (LPARAM)paParts);

    LocalUnlock(hloc);
    LocalFree(hloc);

    SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)L"Ln 1, Col 1");
    SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM)L"0 characters");
    SendMessage(hwndStatus, SB_SETTEXT, 2, (LPARAM)(std::to_wstring(zoom) + L"%").c_str());
    SendMessage(hwndStatus, SB_SETTEXT, 3, (LPARAM)L"Windows (CLRF)");
    SendMessage(hwndStatus, SB_SETTEXT, 4, (LPARAM)L"UTF-8");

    if (!showStatusBar)
    {
        ShowWindow(hwndStatus, SW_HIDE);
        SendMessageW(hwndMain, WM_SIZE, NULL, NULL);
    }
}

// ---
void loadSettings()
{
    Settings settings;

    if (!PathFileExistsA(SETTINGS_FILE_PATH.c_str()))
        settings = getDefSettings();
    else
    {
        std::ifstream file(SETTINGS_FILE_PATH);

        if (!file.is_open())
            settings = getDefSettings();

        try {
            json settingsJson;
            file >> settingsJson;

            LOGFONT font { 0 };

            font.lfHeight = settingsJson.at("font").value("lfHeight", -16);
            font.lfWidth = settingsJson.at("font").value("lfWidth", 0);
            font.lfEscapement = settingsJson.at("font").value("lfEscapement", 0);
            font.lfOrientation = settingsJson.at("font").value("lfOrientation", 0);
            font.lfWeight = settingsJson.at("font").value("lfWeight", FW_NORMAL);
            font.lfItalic = settingsJson.at("font").value("lfItalic", false) ? TRUE : FALSE;
            font.lfUnderline = settingsJson.at("font").value("lfUnderline", false) ? TRUE : FALSE;
            font.lfStrikeOut = settingsJson.at("font").value("lfStrikeOut", false) ? TRUE : FALSE;
            font.lfCharSet = settingsJson.at("font").value("lfCharSet", DEFAULT_CHARSET);
            font.lfOutPrecision = settingsJson.at("font").value("lfOutPrecision", OUT_DEFAULT_PRECIS);
            font.lfClipPrecision = settingsJson.at("font").value("lfClipPrecision", CLIP_DEFAULT_PRECIS);
            font.lfQuality = settingsJson.at("font").value("lfQuality", DEFAULT_QUALITY);
            font.lfPitchAndFamily = settingsJson.at("font").value("lfPitchAndFamily", DEFAULT_PITCH | FF_SWISS);

            std::string _ = std::string(settingsJson.at("font").value("lfFaceName", "Segoe UI"));
            std::wstring __(_.begin(), _.end());
            wcsncpy_s(font.lfFaceName, __.c_str(), LF_FACESIZE - 1);

            settings = Settings{
                font,
                settingsJson.value("zoom", 100),
                settingsJson.value("show-status-bar", true)
            };
        }
        catch (const json::parse_error) {
            std::cout << "[ERROR] Error while parsin settings.json." << std::endl;
            settings = getDefSettings();
        }
    }

    hfEdit = CreateFontIndirectW(&settings.font);
    zoom = settings.zoom;
    showStatusBar = settings.showStatusBar;
}

void saveSettings()
{
    json settingsJson;

    LOGFONT font{ 0 };
    GetObject(hfEdit, sizeof(LOGFONT), &font);

    std::wstring _(font.lfFaceName);
    std::string __(_.begin(), _.end());

    settingsJson["font"] = {
        {"lfHeight", font.lfHeight},
        {"lfWidth", font.lfWidth},
        {"lfEscapement", font.lfEscapement},
        {"lfOrientation", font.lfOrientation},
        {"lfWeight", font.lfWeight},
        {"lfItalic", static_cast<bool>(font.lfItalic)},
        {"lfUnderline", static_cast<bool>(font.lfUnderline)},
        {"lfStrikeOut", static_cast<bool>(font.lfStrikeOut)},
        {"lfCharSet", font.lfCharSet},
        {"lfOutPrecision", font.lfOutPrecision},
        {"lfClipPrecision", font.lfClipPrecision},
        {"lfQuality", font.lfQuality},
        {"lfPitchAndFamily", font.lfPitchAndFamily},
        {"lfFaceName", __}
    };
    settingsJson["zoom"] = zoom;
    settingsJson["show-status-bar"] = showStatusBar;

    std::ofstream file(SETTINGS_FILE_PATH);
    if (file.is_open()) {
        file << settingsJson.dump(4);
        file.close();
    }
    else std::cout << "[ERROR] Could open settings.json for saving settings." << std::endl;
}

// ---
void openFile()
{
    if (!canCloseFile()) return;

    // getting file path
    wchar_t filePath[MAX_PATH] = L"\0";
    OPENFILENAME ofn;

    ZeroMemory(&ofn, sizeof(OPENFILENAME));

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwndMain;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"All files\0*.*\0Text Files\0*.txt\0";
    ofn.nFilterIndex = 1;

    GetOpenFileName(&ofn);

    if (ofn.lpstrFile[0] == '\0')
        return;

    // reading content | TODO: maybe use a win32 specific method
    std::wifstream in(ofn.lpstrFile, std::ios::binary);

    if (!in)
    {
        MessageBox(hwndMain, L"Failed to open file!", L"Error", MB_ICONERROR);
        return;
    }

    std::wstring content;

    in.seekg(0, std::ios::end);
    content.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&content[0], content.size());

    in.close();

    // updating variables
    SetWindowText(hwndEdit, content.c_str());

    curFilePath = std::wstring(ofn.lpstrFile);
    fileOpen = true;
    changesMade = false;

    SetWindowText(hwndMain, (L"Notepad | " + curFilePath).c_str());
}

bool createNewFile()
{
    wchar_t filePath[MAX_PATH] = L"\0";
    OPENFILENAME ofn;

    ZeroMemory(&ofn, sizeof(OPENFILENAME));

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwndMain;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"All files\0*.*\0Text Files\0*.txt\0";
    ofn.nFilterIndex = 1;

    GetSaveFileName(&ofn);

    if (ofn.lpstrFile[0] == '\0')
        return false;

    curFilePath = std::wstring(ofn.lpstrFile);
    fileOpen = true;

    SetWindowText(hwndMain, (L"Notepad | " + curFilePath).c_str());

    return true;
}

void saveFile()
{
    if (fileOpen == false)
        if (!createNewFile()) return;

    HANDLE hFile = CreateFile(curFilePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        MessageBox(hwndMain, L"Failed to open file!", L"Error", MB_ICONERROR);

    int cTxtLength = GetWindowTextLength(hwndEdit);

    std::string buff(cTxtLength, '\0');

    GetWindowTextA(hwndEdit, &buff[0], cTxtLength + 1);

    DWORD bytesWritten;
    if (!WriteFile(hFile, &buff[0], cTxtLength, &bytesWritten, NULL)) {
        CloseHandle(hFile);
        MessageBox(hwndMain, L"Failed to write to file!", L"Error", MB_ICONERROR);
    }

    std::wcout << L"[DEBUG] Bytes written: " << bytesWritten << std::endl;

    CloseHandle(hFile);

    std::cout << "[_INFO] File saved." << std::endl;

    changesMade = false;
}

void printFile()
{
    PRINTDLGW printDlg = { sizeof(printDlg) };

    printDlg.hwndOwner = hwndMain;
    printDlg.hDevMode = NULL;
    printDlg.hDevNames = NULL;
    printDlg.hDC = NULL;
    printDlg.Flags = PD_ALLPAGES | PD_NONETWORKBUTTON | PD_NOSELECTION | PD_COLLATE | PD_RETURNDC;
    printDlg.nMinPage = 1;
    printDlg.nMaxPage = 20;
    printDlg.nCopies = 1;
    printDlg.hInstance = hInst;
    printDlg.lCustData = NULL;
    printDlg.lpfnPrintHook = NULL;
    printDlg.lpfnSetupHook = NULL;
    printDlg.lpPrintTemplateName = NULL;
    printDlg.lpSetupTemplateName = NULL;
    printDlg.hPrintTemplate = NULL;
    printDlg.hSetupTemplate = NULL;

    int res = PrintDlgW(&printDlg);

    if (!res)
    {
        std::cout << "[DEBUG] User cancelled printing." << std::endl;
        return;
    }

    HDC hdcPrinter = printDlg.hDC;

    DOCINFO docInfo = { sizeof(docInfo) };

    if (!StartDoc(hdcPrinter, &docInfo))
    {
        std::cout << "[ERROR] Couldn't start document." << std::endl;
        return;
    }

    // from now on, I have no idea how the code works

    int cxPhysOffset = GetDeviceCaps(hdcPrinter, PHYSICALOFFSETX);
    int cyPhysOffset = GetDeviceCaps(hdcPrinter, PHYSICALOFFSETY);

    int cxPhys = GetDeviceCaps(hdcPrinter, PHYSICALWIDTH);
    int cyPhys = GetDeviceCaps(hdcPrinter, PHYSICALHEIGHT);

    SendMessage(hwndEdit, EM_SETTARGETDEVICE, (WPARAM) hdcPrinter, cxPhys / 2);

    FORMATRANGE fr;

    fr.hdc = hdcPrinter;
    fr.hdcTarget = hdcPrinter;

    fr.rcPage.top = 0;
    fr.rcPage.left = 0;
    fr.rcPage.right = MulDiv(cxPhys, 1440, GetDeviceCaps(hdcPrinter, LOGPIXELSX));
    fr.rcPage.bottom = MulDiv(cyPhys, 1440, GetDeviceCaps(hdcPrinter, LOGPIXELSY));

    fr.rc.left = cxPhysOffset;
    fr.rc.right = cxPhysOffset + cxPhys;
    fr.rc.top = cyPhysOffset;
    fr.rc.bottom = cyPhysOffset + cyPhys;

    SendMessage(hwndEdit, EM_SETSEL, 0, (LPARAM) -1);
    SendMessage(hwndEdit, EM_EXGETSEL, 0, (LPARAM) &fr.chrg);

    BOOL success = TRUE;

    while (fr.chrg.cpMin < fr.chrg.cpMax && success)
    {
        success = StartPage(hdcPrinter) > 0;

        if (!success) break;

        int cpMin = (int) SendMessageW(hwndEdit, EM_FORMATRANGE, TRUE, (LPARAM)&fr);

        if (cpMin <= fr.chrg.cpMin)
        {
            success = FALSE;
            break;
        }

        fr.chrg.cpMin = cpMin;
        success = EndPage(hdcPrinter) > 0;
    }

    SendMessage(hwndEdit, EM_FORMATRANGE, FALSE, 0);

    if (success) EndDoc(hdcPrinter);
    else AbortDoc(hdcPrinter);

    if (success) MessageBoxW(hwndMain, L"File printed succesfully!", L"Success", MB_ICONINFORMATION);
    else MessageBoxW(hwndMain, L"Error while printing file!", L"Error", MB_ICONERROR);
}

bool canCloseFile()
{
    if (changesMade)
    {
        int res = MessageBox(hwndMain, L"You have made changes to your current file. Do you want to save it?", L"Wait ...", MB_YESNOCANCEL | MB_ICONQUESTION);

        switch (res)
        {
        case IDYES:
            saveFile();
            return true;

        case IDNO:
            // Do nothing
            return true;

        case IDCANCEL:
            return false;
        }
    }

    return true;
}

// ---
void openFindDlg()
{
    if (findDlgOpen) return;

    hwndFindDlg = CreateDialogW(
        hInst,
        MAKEINTRESOURCE(IDD_FIND),
        hwndMain,
        (DLGPROC) findDlgProc
    );
    ShowWindow(hwndFindDlg, SW_SHOW);

    findDlgOpen = true;
}

void openReplaceDlg()
{
    if (findDlgOpen) return;

    hwndFindDlg = CreateDialogW(
        hInst,
        MAKEINTRESOURCE(IDD_FIND),
        hwndMain,
        (DLGPROC) findDlgProc
    );
    ShowWindow(hwndFindDlg, SW_SHOW);

    findDlgOpen = true;
}

// ---
void calcFinds()
{
    if (!shouldReCalcFinds) return;

    std::wstring txt = getWndTxt(hwndEdit);
    std::wstring toFind = getDlgItemTxt(hwndFindDlg, IDC_FIND_TXT);

    if (!IsDlgButtonChecked(hwndFindDlg, IDC_FIND_MATCHCASE))
    {
        std::transform(txt.begin(), txt.end(), txt.begin(), ::tolower);
        std::transform(toFind.begin(), toFind.end(), toFind.begin(), ::tolower);
    }

    finds.clear();

    size_t pos = txt.find(toFind);
    while (pos != std::wstring::npos) {
        finds.push_back({ pos, pos + toFind.length() });
        pos = txt.find(toFind, pos + 1);
    }

    shouldReCalcFinds = false;
}

void calcNextFindI()
{
    calcFinds();

    if (finds.size() == 0)
    {
        nextFindI = -1;
        return;
    }

    DWORD selStart{};
    DWORD selEnd{};

    SendMessageW(hwndEdit, EM_GETSEL, (WPARAM)&selStart, (LPARAM)&selEnd);

    nextFindI = -1;
    for (int i = 0; i < finds.size(); i++)
    {
        if (finds[i].first > selStart)
        {
            nextFindI = i;
            break;
        }
    }

    std::cout << "Selection: " << selStart << ", " << selEnd << std::endl;

    if (nextFindI == -1 && IsDlgButtonChecked(hwndFindDlg, IDC_FIND_WRAPAROUND))
        nextFindI = 0;

    std::cout << "Find (" << nextFindI << "): " << finds[nextFindI].first << ", " << finds[nextFindI].second << std::endl;
}

void selNextFind()
{
    if (nextFindI == -1)
    {
        MessageBoxW(hwndFindDlg, L"No more occurences in this file.", L"Information", MB_ICONINFORMATION);
        return;
    }

    SendMessageW(hwndEdit, EM_SETSEL, (WPARAM)finds[nextFindI].first, (LPARAM)finds[nextFindI].second);
}
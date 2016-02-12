#pragma once

//---------------------------------------------------------------------------//
//
// TestWnd.hpp
//  テスト用 UI
//   Copyright (C) 2016 tapetums
//
//---------------------------------------------------------------------------//

#include <windows.h>

#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include "include/UWnd.hpp"
#include "include/CtrlWnd.hpp"
#include "include/Font.hpp"

#include "TTBBridgePlugin.hpp"

//---------------------------------------------------------------------------//
// クラス
//---------------------------------------------------------------------------//

class TestWnd : public tapetums::UWnd
{
    using super = UWnd;

private:
    tapetums::Font     font;
    tapetums::BtnWnd   btn_load;
    tapetums::BtnWnd   btn_free;
    tapetums::BtnWnd   btn_exec;
    tapetums::LabelWnd label;
    tapetums::EditWnd  input_path;
    tapetums::EditWnd  input_cmd;

    struct CTRL { enum : INT16 { LOAD, FREE, EXEC, LABEL, PATH, CMD, }; };

    DWORD        threadId { 0 };
    HANDLE       evt_done { nullptr };
    PLUGIN_INFO* info     { nullptr };

    TTBBridgePlugin plugin;

public:
    TestWnd();

public:
    LRESULT WndProc(HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp)
    {
        if ( uMsg == WM_SIZE )    { OnSize(LOWORD(lp), HIWORD(lp)); return 0; }
        if ( uMsg == WM_PAINT )   { OnPaint(hwnd); return 0; }
        if ( uMsg == WM_COMMAND ) { OnCommand(hwnd, LOWORD(wp)); return 0; }
        return super::WndProc(hwnd, uMsg, wp, lp);
    }

private:
    void OnPaint(HWND hwnd);
    void OnSize(INT32 cx, INT32 cy);
    void OnCommand(HWND hwnd, INT16 wID);
};

//---------------------------------------------------------------------------//

TestWnd::TestWnd()
{
    const auto hwnd = Create
    (
        TEXT("TTBMiniHost"), WS_CAPTION | WS_SYSMENU, 0, nullptr, nullptr
    );
    Resize(320, 24 * 7);
    ToCenter();
    Show();

    font.create(18, TEXT("Meiryo UI"),FW_REGULAR);
     
    btn_load.Create(BS_PUSHBUTTON, hwnd, CTRL::LOAD);
    btn_load.SetFont(font);
    btn_load.SetText(TEXT("Load"));

    btn_free.Create(BS_PUSHBUTTON, hwnd, CTRL::FREE);
    btn_free.SetFont(font);
    btn_free.SetText(TEXT("Free"));

    btn_exec.Create(BS_PUSHBUTTON, hwnd, CTRL::EXEC);
    btn_exec.SetFont(font);
    btn_exec.SetText(TEXT("Execute"));

    label.Create(ES_CENTER, hwnd, CTRL::LABEL);
    label.SetFont(font);
    label.SetText(TEXT("Not loaded"));

    input_path.Create(ES_CENTER | ES_AUTOHSCROLL, hwnd, CTRL::PATH);
    input_path.SetFont(font);
    input_path.SetText(TEXT(""));

    input_cmd.Create(ES_CENTER, hwnd, CTRL::CMD);
    input_cmd.SetFont(font);
    input_cmd.SetText(TEXT("0"));

    ::SetFocus(input_path);
}

//---------------------------------------------------------------------------//

void TestWnd::OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    const auto hdc = ::BeginPaint(hwnd, &ps);

    ::FillRect(hdc, &ps.rcPaint, GetStockBrush(LTGRAY_BRUSH));

    ::EndPaint(hwnd, &ps);
}

//---------------------------------------------------------------------------//

void TestWnd::OnSize(INT32 cx, INT32)
{
    input_path.Bounds(8, 24, cx - 16, 24);
    btn_load.  Bounds(8, 4 + 24 * 2, (cx - 16) / 2, 24);
    btn_free.  Bounds(cx - 8 - (cx - 16) / 2, 4 + 24 * 2, (cx - 16) / 2, 24);

    label.Bounds(8, 8 + 24 * 3, cx - 16, 24);

    input_cmd.Bounds(8, 4 + 24 * 5, (cx - 16) / 2, 24);
    btn_exec. Bounds(cx - 8 - (cx - 16) / 2, 4 + 24 * 5, (cx - 16) / 2, 24);
}

//---------------------------------------------------------------------------//

void TestWnd::OnCommand(HWND hwnd, INT16 wID)
{
    switch ( wID )
    {
        case CTRL::LOAD:
        {
            wchar_t buf[MAX_PATH];
            input_path.GetText(buf, MAX_PATH);
            ::OutputDebugStringW(L"path:  ");
            ::OutputDebugStringW(buf);
            ::OutputDebugStringW(L"\n");

            if ( plugin.Load(buf) )
            {
                label.SetText(TEXT("Loaded"));
            }
            break;
        }
        case CTRL::FREE:
        {
            if ( plugin.Free() )
            {
                label.SetText(TEXT("Not loaded"));
            }
            break;
        }
        case CTRL::EXEC:
        {
            wchar_t buf[4];
            input_cmd.GetText(buf, 4);
            ::OutputDebugStringW(L"CmdID\n  ");
            ::OutputDebugStringW(buf);
            ::OutputDebugStringW(L"\n");

            const auto CmdID = ::StrToInt(buf);
            plugin.Execute(CmdID, hwnd);

            break;
        }
        default: break;
    }
}

//---------------------------------------------------------------------------//

// TestWnd.hpp.cpp
//---------------------------------------------------------------------------//
//
// WinMain.cpp
//  アプリケーション エントリポイント
//   Copyright (C) 2016 tapetums
//
//---------------------------------------------------------------------------//

#include <windows.h>
#include <tchar.h>

#include "include/Application.hpp"
#include "TestWnd.hpp"

//---------------------------------------------------------------------------//

// メモリリーク検出
#if defined(_DEBUG) || (DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

//---------------------------------------------------------------------------//
// アプリケーション エントリポイント
//---------------------------------------------------------------------------//

INT32 WINAPI _tWinMain
(
    HINSTANCE, HINSTANCE, LPTSTR, INT32
)
{
    // メモリリーク検出
  #if defined(_DEBUG) || (DEBUG)
    ::_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
  #endif

    // メッセージループ
    TestWnd wnd;
    return tapetums::Application::Run();
}

//---------------------------------------------------------------------------//

// WinMain.cpp
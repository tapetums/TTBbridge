//---------------------------------------------------------------------------//
//
// WinMain.cpp
//  アプリケーション エントリポイント
//
//---------------------------------------------------------------------------//

#include <array>
#include <vector>

#include <windows.h>
#include <tchar.h>

#include "TTBasePluginAdapter.hpp"

//---------------------------------------------------------------------------//

// メモリリーク検出
#if defined(_DEBUG) || defined(DEBUG)
  #define _CRTDBG_MAP_ALLOC
  #include <crtdbg.h>
  #define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

//---------------------------------------------------------------------------//
// アプリケーション エントリポイント
//---------------------------------------------------------------------------//

INT32 APIENTRY _tWinMain
(
    HINSTANCE, HINSTANCE, LPTSTR, INT32
)
{
    // メモリリーク検出
  #if defined(_DEBUG) || (DEBUG)
    ::_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
  #endif

    // コマンドラインオプションの取得
    int32_t argc;
    const auto argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

    // データをやりとりするための 共有ファイル名を受け取る
    TTBasePluginAdapter adapter;
    adapter.ctor(argv[0]);

    // メッセージループ
    MSG msg { };
    while ( ::GetMessage(&msg, nullptr, 0, 0) > 0 )
    {
        if ( msg.message == WM_QUIT ) { break; }

        // 親プロセスからの イベントを処理する
        adapter();
    }

    return static_cast<INT32>(msg.wParam);
}

//---------------------------------------------------------------------------//

// WinMain.cpp
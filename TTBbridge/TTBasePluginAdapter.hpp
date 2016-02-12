#pragma once

//---------------------------------------------------------------------------//
//
// TTBasePluginAdapter.hpp
//  TTBase プラグインを プロセスを超えて使用するためのアダプタ
//   Copyright (C) 2014-2016 tapetums
//
//---------------------------------------------------------------------------//

#include <array>
#include <vector>

#include <windows.h>

#include "TTBase/Plugin.hpp"
#include "include/File.hpp"
#include "include/GenerateUUIDString.hpp"
#include "../BridgeData.hpp"
#include "Utility.hpp"

//---------------------------------------------------------------------------//
// ユーティリティ関数
//---------------------------------------------------------------------------//

static void ShowLastError
(
    LPCTSTR window_title
)
{
    LPTSTR lpMsgBuf{ nullptr };
    ::FormatMessage
    (
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        ::GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0,
        nullptr
    );
    ::MessageBox(nullptr, lpMsgBuf, window_title, MB_OK);
    ::LocalFree(lpMsgBuf);
}

//---------------------------------------------------------------------------//
// クラス
//---------------------------------------------------------------------------//

class TTBasePluginAdapter
{
    using File = tapetums::File;

private:
    BridgeData data;
    File       shrmem;
    File       info_data;
    HANDLE     evt_done { nullptr };

    wchar_t path[MAX_PATH];
    HMODULE handle { nullptr };

    TTBEVENT_INITPLUGININFO TTBEvent_InitPluginInfo { nullptr };
    TTBEVENT_FREEPLUGININFO TTBEvent_FreePluginInfo { nullptr };
    TTBEVENT_INIT           TTBEvent_Init           { nullptr };
    TTBEVENT_UNLOAD         TTBEvent_Unload         { nullptr };
    TTBEVENT_EXECUTE        TTBEvent_Execute        { nullptr };
    TTBEVENT_WINDOWSHOOK    TTBEvent_WindowsHook    { nullptr };

public:
    TTBasePluginAdapter() = default;
    ~TTBasePluginAdapter() { dtor(); }

public:
    // 共有メモリを開く
    bool ctor(const wchar_t* const filename)
    {
        ::OutputDebugStringW(L"Hello\n");

        if ( ! shrmem.Open(filename, File::ACCESS::WRITE) )
        {
            return false;
        }
        shrmem.Read(&data);

        evt_done = ::OpenEventW(EVENT_ALL_ACCESS, FALSE, data.done);
        ::SetEvent(evt_done);

        return true;
    }

    // 共有メモリを閉じる
    void dtor()
    {
        free();

        ::CloseHandle(evt_done);
        shrmem.Close();

        ::OutputDebugStringW(L"Bye\n");
    }

    // イベントハンドラ
    void operator ()()
    {
        shrmem.Seek(0);
        shrmem.Read(&data);
        ::OutputDebugStringW(L"Received:\n  ");
        ::OutputDebugStringW(data.filename);
        ::OutputDebugStringW(L"\n");

        File plugin_data;
        if ( ! plugin_data.Open(data.filename, File::ACCESS::WRITE) )
        {
            ::OutputDebugStringW(L"  データの受け取りに失敗\n");
            return;
        }

        PluginMsg msg;
        plugin_data.Read(&msg, sizeof(msg));
        ::OutputDebugStringW(L"Message:\n  ");
        ::OutputDebugStringW(PluginMsgTxt[(uint8_t)msg]);
        ::OutputDebugStringW(L"\n");

        bool result { true };
        switch ( msg )
        {
            case PluginMsg::Load:     result = load(plugin_data);    break;
            case PluginMsg::Free:     free();                        break;
            case PluginMsg::InitInfo: init_plugin_info(plugin_data); break;
            case PluginMsg::FreeInfo: free_plugin_info();            break;
            case PluginMsg::Init:     result = init(plugin_data);    break;
            case PluginMsg::Unload:   unload();                      break;
            case PluginMsg::Execute:  result = execute(plugin_data); break;
            case PluginMsg::Hook:     hook(plugin_data);             break;
            default: break;
        }

        msg = result ? PluginMsg::OK : PluginMsg::NG;
        plugin_data.Seek(0);
        plugin_data.Write(msg);

        ::SetEvent(evt_done);
    }

    // プラグインの読み込み
    bool load(File& plugin_data)
    {
        ::OutputDebugStringW(L"load()\n");

        std::array<wchar_t, MAX_PATH> buf;
        plugin_data.Read(buf.data(), sizeof(wchar_t) * buf.size());
        ::OutputDebugStringW(L"  ");
        ::OutputDebugStringW(buf.data());
        ::OutputDebugStringW(L"\n");

        if ( handle )
        {
            ::OutputDebugStringW(L"  読込済み\n");
            return true;
        }

        handle = ::LoadLibraryExW(buf.data(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if ( nullptr == handle )
        {
            ::OutputDebugStringW(L" 読込失敗 \n");
            ShowLastError("A");
            return false;
        }

        // DLLのフルパスを取得
        ::GetModuleFileNameW(handle, path, MAX_PATH);

        // 関数ポインタの取得
        TTBEvent_InitPluginInfo = (TTBEVENT_INITPLUGININFO)::GetProcAddress(handle, "TTBEvent_InitPluginInfo");
        TTBEvent_FreePluginInfo = (TTBEVENT_FREEPLUGININFO)::GetProcAddress(handle, "TTBEvent_FreePluginInfo");
        TTBEvent_Init           = (TTBEVENT_INIT)          ::GetProcAddress(handle, "TTBEvent_Init");
        TTBEvent_Unload         = (TTBEVENT_UNLOAD)        ::GetProcAddress(handle, "TTBEvent_Unload");
        TTBEvent_Execute        = (TTBEVENT_EXECUTE)       ::GetProcAddress(handle, "TTBEvent_Execute");
        TTBEvent_WindowsHook    = (TTBEVENT_WINDOWSHOOK)   ::GetProcAddress(handle, "TTBEvent_WindowsHook");

        // 必須APIを実装しているか
        if ( nullptr == TTBEvent_InitPluginInfo || nullptr == TTBEvent_FreePluginInfo )
        {
            ::OutputDebugStringW(L"  有効なプラグインではありません\n");
            free();
            return false;
        }

        ::OutputDebugStringW(L"  OK\n");
        return true;
    }

    // プラグインの解放
    void free()
    {
        unload();

        ::OutputDebugStringW(L"free()\n");
        ::OutputDebugStringW(L"  ");
        ::OutputDebugStringW(path);
        ::OutputDebugStringW(L"\n");

        TTBEvent_InitPluginInfo = nullptr;
        TTBEvent_FreePluginInfo = nullptr;
        TTBEvent_Init           = nullptr;
        TTBEvent_Unload         = nullptr;
        TTBEvent_Execute        = nullptr;
        TTBEvent_WindowsHook    = nullptr;

        if ( nullptr == handle )
        {
            ::OutputDebugStringW(L"  解放済み\n");
           return;
        }

        ::FreeLibrary(handle);
        handle = nullptr;

        ::OutputDebugStringW(L"  OK\n");
    };

    // プラグイン情報の初期化
    void init_plugin_info(File& plugin_data)
    {
        if ( nullptr == TTBEvent_InitPluginInfo ) { return; }
        if ( nullptr == TTBEvent_FreePluginInfo ) { return; }

        ::OutputDebugStringW(L"init_plugin_info()\n");

        using namespace tapetums;

        std::array<wchar_t, MAX_PATH> filename;
        plugin_data.Read(filename.data(), sizeof(wchar_t) * filename.size());

        char PluginFilename[MAX_PATH];
        toMBCS(filename.data(), PluginFilename, MAX_PATH);
        ::OutputDebugStringA("  ");
        ::OutputDebugStringA(PluginFilename);
        ::OutputDebugStringA("\n");

        // データの準備
        std::array<wchar_t, BridgeData::namelen> info_data_name;
        GenerateUUIDStringW(info_data_name.data(), sizeof(wchar_t) * info_data_name.size());

        auto tmp = TTBEvent_InitPluginInfo(PluginFilename);
        const auto info = MarshallPluginInfo(tmp);
        TTBEvent_FreePluginInfo(tmp);

        const auto serialized = SerializePluginInfo(info);
        FreePluginInfo(info);

        info_data.Close();
        info_data.Map(serialized.size(), info_data_name.data(), File::ACCESS::WRITE);
        info_data.Write(serialized.size());
        info_data.Write(serialized.data(), serialized.size());

        // 共有ファイルの中で plugin_data の ファイル名が入っていた場所に
        // info_data の ファイル名を上書き
        ::StringCchCopyW(data.filename, data.namelen, info_data.name());
        shrmem.Seek(0);
        shrmem.Write(data);

        ::OutputDebugStringW(L"  OK\n");
    }

    // プラグイン情報の解放
    void free_plugin_info()
    {
        ::OutputDebugStringW(L"free_plugin_info()\n");

        info_data.Close();

        ::OutputDebugStringW(L"  OK\n");
    }

    // プラグインの初期化
    bool init(File& plugin_data)
    {
        ::OutputDebugStringW(L"init()\n");

        if ( nullptr == TTBEvent_Init ) { return true; }

        using namespace tapetums;

        std::array<wchar_t, MAX_PATH> filename;
        plugin_data.Read(filename.data(), sizeof(wchar_t) * filename.size());

        DWORD_PTR hPlugin;
        plugin_data.Read(&hPlugin);

        char PluginFilename[MAX_PATH];
        toMBCS(filename.data(), PluginFilename, MAX_PATH);
        ::OutputDebugStringA("  ");
        ::OutputDebugStringA(PluginFilename);
        ::OutputDebugStringA("\n");

        const auto result = TTBEvent_Init(PluginFilename, hPlugin) ? true : false;
        ::OutputDebugStringW(result ? L"  OK\n" : L"  NG\n");

        return result;
    }

    // プラグインの解放
    void unload()
    {
        ::OutputDebugStringW(L"unload()\n");

        if ( nullptr == TTBEvent_Unload ) { return; }

        TTBEvent_Unload();

        ::OutputDebugStringW(L"  OK\n");
    }

    // コマンドの実行
    bool execute(File& plugin_data)
    {
        ::OutputDebugStringW(L"execute()\n");

        if ( nullptr == TTBEvent_Execute ) { return true; }

        INT32 CmdID;
        plugin_data.Read(&CmdID);

        HWND hwnd;
        plugin_data.Read(&hwnd);

        wchar_t buf[16];
        ::StringCchPrintfW(buf, 16, L"%i", CmdID);
        ::OutputDebugStringW(L"  CmdID: ");
        ::OutputDebugStringW(buf);
        ::OutputDebugStringW(L"\n");

        const auto result = TTBEvent_Execute(CmdID, hwnd) ? true : false;

        ::OutputDebugStringW(result ? L"  OK\n" : L"  NG\n");
        return result;
    }

    // フックの開始
    void hook(File& plugin_data)
    {
        ::OutputDebugStringW(L"hook()\n");

        if ( nullptr == TTBEvent_WindowsHook ) { return; }

        std::array<wchar_t, MAX_PATH> filename;
        plugin_data.Read(filename.data(), sizeof(wchar_t) * filename.size());

        DWORD Msg;
        plugin_data.Read(&Msg);

        WPARAM wp;
        plugin_data.Read(&wp);

        LPARAM lp;
        plugin_data.Read(&lp);

        TTBEvent_WindowsHook(Msg, wp, lp);

        ::OutputDebugStringW(L"  OK\n");
    }
};

//---------------------------------------------------------------------------//

// TTBasePluginAdapter.hpp
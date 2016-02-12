#pragma once

//---------------------------------------------------------------------------//
//
// TTBBridgePlugin.hpp
//  プロセス間通信を使って別プロセスのプラグインを操作する
//   Copyright (C) 2016 tapetums
//
//---------------------------------------------------------------------------//

#include <array>
#include <vector>

#include <windows.h>
#include <strsafe.h>

#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib") // PathRemoveFileSpec, PathRelativePathTo

#include "TTBase/Plugin.hpp"
#include "include/File.hpp"
#include "include/GenerateUUIDString.hpp"
#include "../BridgeData.hpp"
#include "Utility.hpp"

//---------------------------------------------------------------------------//
// クラス
//---------------------------------------------------------------------------//

class TTBBridgePlugin
{
    using File = tapetums::File;

private:
    DWORD  threadId { 0 };
    HANDLE evt_done { nullptr };

    File shrmem;

    bool         loaded { false };
    PLUGIN_INFO* info   { nullptr };

public:
    TTBBridgePlugin();
    ~TTBBridgePlugin();

public:
    bool Load    (LPCWSTR path);
    bool Free    ();
    bool InitInfo(LPCWSTR PluginFilename);
    bool FreeInfo();
    bool Init    (LPCWSTR PluginFilename, DWORD_PTR hPlugin);
    bool Unload  ();
    bool Execute (INT32 CmdID, HWND hwnd);
    bool Hook    (UINT Msg, WPARAM wParam, LPARAM lParam);
};

//---------------------------------------------------------------------------//

TTBBridgePlugin::TTBBridgePlugin()
{
    using namespace tapetums;


    // データの準備
    BridgeData data;
    GenerateUUIDStringW(data.filename, data.namelen);
    GenerateUUIDStringW(data.done,     data.namelen);
    evt_done = ::CreateEventW(nullptr, FALSE, FALSE, data.done);

    if ( ! shrmem.Map(sizeof(data), data.filename, File::ACCESS::WRITE) )
    {
        return;
    }
    shrmem.Write(data);

    // 子プロセスのパスを合成
    std::array<wchar_t, MAX_PATH> path;
    ::GetModuleFileNameW(nullptr, path.data(), (DWORD)path.size());
    ::PathRemoveFileSpecW(path.data());
    ::StringCchCatW(path.data(), path.size(), LR"(\TTBBridge.exe)");

    // 子プロセスの生成
    std::array<wchar_t, MAX_PATH> args;
    ::StringCchCopyW(args.data(), args.size(), shrmem.name());

    STARTUPINFO si;
    ::GetStartupInfoW(&si);

    PROCESS_INFORMATION pi { };
    const auto result = ::CreateProcessW
    (
        path.data(), args.data(),
        nullptr, nullptr, FALSE,
        NORMAL_PRIORITY_CLASS, nullptr, nullptr,
        &si, &pi
    );
    if ( ! result )
    {
        ::OutputDebugStringW(L"子プロセスを起動できません\n");
        return;
    }
    threadId = pi.dwThreadId;

    // 接続テスト
    DWORD ret;
    ret = ::WaitForSingleObject(evt_done, 5'000);
    if ( ret != WAIT_OBJECT_0 )
    {
        ::MessageBoxW(nullptr, L"子プロセスを開始できません\n",  L"TTBase", MB_OK);
        ::TerminateProcess(pi.hProcess, 0);
        ::Sleep(100);
        return;
    }

    ::OutputDebugStringW(L"  OK\n");
    return;
}

//---------------------------------------------------------------------------//

TTBBridgePlugin::~TTBBridgePlugin()
{
    if ( ! loaded )
    {
        return; // ムーブデストラクタでは余計な処理をしない
    }

    Unload();
    FreeInfo();
    Free();

    // 子プロセスの終了
    ::PostThreadMessage(threadId, WM_QUIT, 0, 0);
    ::OutputDebugStringW(L"Bye\n");
}

//---------------------------------------------------------------------------//

bool TTBBridgePlugin::Load(LPCTSTR path)
{
    ::OutputDebugStringW(L"Load\n");
    ::OutputDebugStringW(L"  ");
    ::OutputDebugString(path);
    ::OutputDebugStringW(L"\n");

    if ( loaded )
    {
        ::OutputDebugStringW(L"  読込済み\n");
        return false;
    }

    if ( ! shrmem.is_mapped() ) { return false; }

    // データの準備
    using namespace tapetums;

    std::array<wchar_t, BridgeData::namelen> uuid;
    GenerateUUIDStringW(uuid.data(), sizeof(wchar_t) * uuid.size());

    File plugin_data;
    const auto result = plugin_data.Map
    (
        sizeof(PluginMsg) + MAX_PATH * sizeof(wchar_t),
        uuid.data(), File::ACCESS::WRITE
    );
    if ( result )
    {
        PluginMsg msg = PluginMsg::Load;
        plugin_data.Write(&msg, sizeof(msg));
    }

    // データの書き込み
    std::array<wchar_t, MAX_PATH> buf;
    ::StringCchCopyW(buf.data(), buf.size(), path);
    plugin_data.Write(buf.data(), buf.size() * sizeof(wchar_t));

    // データを送信
    BridgeData data;
    ::StringCchCopyW(data.filename, data.namelen, plugin_data.name());
    shrmem.Seek(0);
    shrmem.Write(data);
    ::PostThreadMessage(threadId, WM_COMMAND, 0, 0);

    // 受信完了待ち
    const auto ret = ::WaitForSingleObject(evt_done, 3'000);
    if ( ret != WAIT_OBJECT_0 )
    {
        ::OutputDebugStringW(L"  子プロセスにデータを送れませんでした\n");
        return false;
    }

    PluginMsg msg;
    plugin_data.Seek(0);
    plugin_data.Read(&msg);
    ::OutputDebugStringW(L"  ");
    ::OutputDebugStringW(PluginMsgTxt[(uint8_t)msg]);
    ::OutputDebugStringW(L"\n");

    if ( msg != PluginMsg::OK )
    {
        return false;
    }

    // 読み込み済みのフラグをオン
    loaded = true;

    // 相対パスの生成
    std::array<wchar_t, MAX_PATH> exe_path;
    std::array<wchar_t, MAX_PATH> rel_path;
    ::GetModuleFileNameW
    (
        ::GetModuleHandle(nullptr), exe_path.data(), (DWORD)exe_path.size()
    );
    ::PathRelativePathToW
    (
        rel_path.data(),
        exe_path.data(), FILE_ATTRIBUTE_ARCHIVE,
        path,            FILE_ATTRIBUTE_ARCHIVE
    );

    return InitInfo(rel_path.data() + 2);
}

//---------------------------------------------------------------------------//

bool TTBBridgePlugin::Free()
{
    ::OutputDebugStringW(L"Free\n");

    if ( ! loaded )
    {
        ::OutputDebugStringW(L"  解放済み\n");
        return false;
    }

    if ( ! shrmem.is_mapped() ) { return false; }

    using namespace tapetums;

    // データの準備
    std::array<wchar_t, BridgeData::namelen> uuid;
    GenerateUUIDStringW(uuid.data(), sizeof(wchar_t) * uuid.size());

    File plugin_data;
    const auto result = plugin_data.Map
    (
        sizeof(PluginMsg) + 0,
        uuid.data(), File::ACCESS::WRITE
    );
    if ( result )
    {
        PluginMsg msg = PluginMsg::Free;
        plugin_data.Write(&msg, sizeof(msg));
    }

    // データを送信
    BridgeData data;
    ::StringCchCopyW(data.filename, data.namelen, plugin_data.name());
    shrmem.Seek(0);
    shrmem.Write(data);
    ::PostThreadMessage(threadId, WM_COMMAND, 0, 0);

    // 受信完了待ち
    const auto ret = ::WaitForSingleObject(evt_done, 3'000);
    if ( ret != WAIT_OBJECT_0 )
    {
        ::OutputDebugStringW(L"  子プロセスにデータを送れませんでした\n");
        return false;
    }

    // 読み込み済みのフラグをオフ
    loaded = false;

    ::OutputDebugStringW(L"  OK\n");
    return true;
}

//---------------------------------------------------------------------------//

bool TTBBridgePlugin::InitInfo(LPCWSTR PluginFilename)
{
    ::OutputDebugStringW(L"InitInfo\n");

    if ( ! loaded )
    {
        ::OutputDebugStringW(L"  未読込\n");
        return false;
    }

    if ( ! shrmem.is_mapped() ) { return false; }

    using namespace tapetums;

    // データの準備
    std::array<wchar_t, BridgeData::namelen> uuid;
    GenerateUUIDStringW(uuid.data(), sizeof(wchar_t) * uuid.size());

    File plugin_data;
    const auto result = plugin_data.Map
    (
        sizeof(PluginMsg) + MAX_PATH * sizeof(wchar_t),
        uuid.data(), File::ACCESS::WRITE
    );
    if ( result )
    {
        PluginMsg msg = PluginMsg::InitInfo;
        plugin_data.Write(&msg, sizeof(msg));
    }

    // データの書き込み
    std::array<wchar_t, MAX_PATH> buf;
    ::StringCchCopyW(buf.data(), buf.size(), PluginFilename);
    plugin_data.Write(buf.data(), buf.size() * sizeof(wchar_t));

    // データを送信
    BridgeData data;
    ::StringCchCopyW(data.filename, data.namelen, plugin_data.name());
    shrmem.Seek(0);
    shrmem.Write(data);
    ::PostThreadMessage(threadId, WM_COMMAND, 0, 0);

    // 受信完了待ち
    const auto ret = ::WaitForSingleObject(evt_done, 3'000);
    if ( ret != WAIT_OBJECT_0 )
    {
        ::OutputDebugStringW(L"  子プロセスにデータを送れませんでした\n");
        return false;
    }

    // データの読み取り
    shrmem.Seek(0);
    shrmem.Read(&data);

    File info_data;
    if ( ! info_data.Open(data.filename, File::ACCESS::WRITE) )
    {
        ::OutputDebugStringW(L"  共有ファイルが開けません\n");
        return false;
    }

    // プラグイン情報の取得
    uint32_t size; // 32-bit の size_t は 32-bit 幅!!!!
    info_data.Seek(0);
    info_data.Read(&size);

    std::vector<uint8_t> serialized;
    serialized.resize(size);
    info_data.Read(serialized.data(), serialized.size());

    // プラグイン情報のマーシャリング
    FreePluginInfo(info);
    info = DeserializePluginInfo(serialized);
    ::OutputDebugStringW(L"  OK\n");

    if ( info->PluginType != ptAlwaysLoad )
    {
        Free(); return true;
    }
    else
    {
        return Init(PluginFilename, 0);
    }
}

//---------------------------------------------------------------------------//

bool TTBBridgePlugin::FreeInfo()
{
    // プラグイン情報の解放
    if ( info )
    {
        FreePluginInfo(info);
        info = nullptr;
    }

    ::OutputDebugStringW(L"FreeInfo\n");

    if ( ! loaded )
    {
        ::OutputDebugStringW(L"  解放済み\n");
        return false;
    }

    if ( ! shrmem.is_mapped() ) { return false; }

    using namespace tapetums;

    // データの準備
    std::array<wchar_t, BridgeData::namelen> uuid;
    GenerateUUIDStringW(uuid.data(), sizeof(wchar_t) * uuid.size());

    File plugin_data;
    const auto result = plugin_data.Map
    (
        sizeof(PluginMsg) + 0,
        uuid.data(), File::ACCESS::WRITE
    );
    if ( result )
    {
        PluginMsg msg = PluginMsg::FreeInfo;
        plugin_data.Write(&msg, sizeof(msg));
    }

    // データを送信
    BridgeData data;
    ::StringCchCopyW(data.filename, data.namelen, plugin_data.name());
    shrmem.Seek(0);
    shrmem.Write(data);
    ::PostThreadMessage(threadId, WM_COMMAND, 0, 0);

    // 受信完了待ち
    const auto ret = ::WaitForSingleObject(evt_done, 3'000);
    if ( ret != WAIT_OBJECT_0 )
    {
        ::OutputDebugStringW(L"  子プロセスにデータを送れませんでした\n");
        return false;
    }

    ::OutputDebugStringW(L"  OK\n");
    return true;
}

//---------------------------------------------------------------------------//

bool TTBBridgePlugin::Init(LPCWSTR PluginFilename, DWORD_PTR hPlugin)
{
    ::OutputDebugStringW(L"Init\n");

    if ( ! loaded )
    {
        ::OutputDebugStringW(L"  未読込\n");
        return false;
    }

    if ( ! shrmem.is_mapped() ) { return false; }

    using namespace tapetums;

    // データの準備
    std::array<wchar_t, BridgeData::namelen> uuid;
    GenerateUUIDStringW(uuid.data(), sizeof(wchar_t) * uuid.size());

    File plugin_data;
    const auto result = plugin_data.Map
    (
        sizeof(PluginMsg) + MAX_PATH * sizeof(wchar_t) + sizeof(DWORD_PTR),
        uuid.data(), File::ACCESS::WRITE
    );
    if ( result )
    {
        PluginMsg msg = PluginMsg::Init;
        plugin_data.Write(&msg, sizeof(msg));
    }

    // データの書き込み
    std::array<wchar_t, MAX_PATH> buf;
    ::StringCchCopyW(buf.data(), buf.size(), PluginFilename);

    plugin_data.Write(buf.data(), sizeof(wchar_t) * buf.size());
    plugin_data.Write(hPlugin);

    // データを送信
    BridgeData data;
    ::StringCchCopyW(data.filename, data.namelen, plugin_data.name());
    shrmem.Seek(0);
    shrmem.Write(data);
    ::PostThreadMessage(threadId, WM_COMMAND, 0, 0);

    // 受信完了待ち
    const auto ret = ::WaitForSingleObject(evt_done, 3'000);
    if ( ret != WAIT_OBJECT_0 )
    {
        ::OutputDebugStringW(L"  子プロセスにデータを送れませんでした\n");
        return false;
    }

    PluginMsg msg;
    plugin_data.Seek(0);
    plugin_data.Read(&msg);

    ::OutputDebugStringW(L"  ");
    ::OutputDebugStringW(PluginMsgTxt[(uint8_t)msg]);
    ::OutputDebugStringW(L"\n");

    return true;
}

//---------------------------------------------------------------------------//

bool TTBBridgePlugin::Unload()
{
    ::OutputDebugStringW(L"Unload\n");

    if ( ! loaded )
    {
        ::OutputDebugStringW(L"  未読込\n");
        return false;
    }

    if ( ! shrmem.is_mapped() ) { return false; }

    using namespace tapetums;

    // データの準備
    std::array<wchar_t, BridgeData::namelen> uuid;
    GenerateUUIDStringW(uuid.data(), sizeof(wchar_t) * uuid.size());

    File plugin_data;
    const auto result = plugin_data.Map
    (
        sizeof(PluginMsg) + 0,
        uuid.data(), File::ACCESS::WRITE
    );
    if ( result )
    {
        PluginMsg msg = PluginMsg::Unload;
        plugin_data.Write(&msg, sizeof(msg));
    }

    // データを送信
    BridgeData data;
    ::StringCchCopyW(data.filename, data.namelen, plugin_data.name());
    shrmem.Seek(0);
    shrmem.Write(data);
    ::PostThreadMessage(threadId, WM_COMMAND, 0, 0);

    // 受信完了待ち
    const auto ret = ::WaitForSingleObject(evt_done, 3'000);
    if ( ret != WAIT_OBJECT_0 )
    {
        ::OutputDebugStringW(L"  子プロセスにデータを送れませんでした\n");
        return false;
    }

    ::OutputDebugStringW(L"  ");
    ::OutputDebugStringW(L"OK\n");

    return true;
}

//---------------------------------------------------------------------------//

bool TTBBridgePlugin::Execute(INT32 CmdID, HWND hwnd)
{
    ::OutputDebugStringW(L"Execute\n");

    if ( ! loaded )
    {
        ::OutputDebugStringW(L"  未読込\n");
        return false;
    }

    if ( ! shrmem.is_mapped() ) { return false; }

    using namespace tapetums;

    // データの準備
    std::array<wchar_t, BridgeData::namelen> uuid;
    GenerateUUIDStringW(uuid.data(), sizeof(wchar_t) * uuid.size());

    File plugin_data;
    const auto result = plugin_data.Map
    (
        sizeof(PluginMsg) + sizeof(INT32) + sizeof(DWORD_PTR),
        uuid.data(), File::ACCESS::WRITE
    );
    if ( result )
    {
        PluginMsg msg = PluginMsg::Execute;
        plugin_data.Write(&msg, sizeof(msg));
    }

    // データの書き込み
    plugin_data.Write(CmdID);
    plugin_data.Write(hwnd);

    // データを送信
    BridgeData data;
    ::StringCchCopyW(data.filename, data.namelen, plugin_data.name());
    shrmem.Seek(0);
    shrmem.Write(data);
    ::PostThreadMessage(threadId, WM_COMMAND, 0, 0);

    // 受信完了待ち
    const auto ret = ::WaitForSingleObject(evt_done, 3'000);
    if ( ret != WAIT_OBJECT_0 )
    {
        ::OutputDebugStringW(L"  子プロセスにデータを送れませんでした\n");
        return false;
    }

    PluginMsg msg;
    plugin_data.Seek(0);
    plugin_data.Read(&msg);

    ::OutputDebugStringW(L"  ");
    ::OutputDebugStringW(PluginMsgTxt[(uint8_t)msg]);
    ::OutputDebugStringW(L"\n");

    return true;
}

//---------------------------------------------------------------------------//

bool TTBBridgePlugin::Hook(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    ::OutputDebugStringW(L"Hook\n");

    if ( ! loaded )
    {
        ::OutputDebugStringW(L"  未読込\n");
        return false;
    }

    if ( ! shrmem.is_mapped() ) { return false; }

    using namespace tapetums;

    // データの準備
    std::array<wchar_t, BridgeData::namelen> uuid;
    GenerateUUIDStringW(uuid.data(), sizeof(wchar_t) * uuid.size());

    File plugin_data;
    const auto result = plugin_data.Map
    (
        sizeof(PluginMsg) + sizeof(DWORD) + sizeof(WPARAM) + sizeof(LPARAM),
        uuid.data(), File::ACCESS::WRITE
    );
    if ( result )
    {
        PluginMsg msg = PluginMsg::Hook;
        plugin_data.Write(&msg, sizeof(msg));
    }

    // データの書き込み
    plugin_data.Write(Msg);
    plugin_data.Write(wParam);
    plugin_data.Write(lParam);

    // データを送信
    BridgeData data;
    ::StringCchCopyW(data.filename, data.namelen, plugin_data.name());
    shrmem.Seek(0);
    shrmem.Write(data);
    ::PostThreadMessage(threadId, WM_COMMAND, 0, 0);

    // 受信完了待ち
    const auto ret = ::WaitForSingleObject(evt_done, 3'000);
    if ( ret != WAIT_OBJECT_0 )
    {
        ::OutputDebugStringW(L"  子プロセスにデータを送れませんでした\n");
        return false;
    }

    ::OutputDebugStringW(L"  ");
    ::OutputDebugStringW(L"OK\n");

    return true;
}

//---------------------------------------------------------------------------//

// TTBBridgePlugin.hpp
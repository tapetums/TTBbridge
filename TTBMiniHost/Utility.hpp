#pragma once

//---------------------------------------------------------------------------//
//
// Utility.hpp
//  ユーティリティルーチン
//   Copyright (C) 2014-2016 tapetums
//
//---------------------------------------------------------------------------//

#include <array>
#include <vector>

#include <windows.h>
#include <strsafe.h>

#include "TTBase/Plugin.hpp"
#include "include/File.hpp"
#include "include/GenerateUUIDString.hpp"
#include "../BridgeData.hpp"

//---------------------------------------------------------------------------//
// ユーティリティルーチン
//---------------------------------------------------------------------------//

// 文字列の格納領域を確保し、文字列をコピーして返す
LPTSTR CopyString(LPCTSTR Src)
{
    const auto len = 1 + ::lstrlen(Src);

    auto Dst = new TCHAR[len];
    if ( Dst != nullptr )
    {
        ::StringCchCopy(Dst, len, Src);
    }

    return Dst;
}

//---------------------------------------------------------------------------//

// 文字列を削除する
void DeleteString(LPCTSTR Str)
{
    if ( Str != nullptr )
    {
        delete[] Str;
    }
}

//---------------------------------------------------------------------------//

// プラグイン情報構造体をディープコピーして返す
PLUGIN_INFO* CopyPluginInfo(PLUGIN_INFO* Src)
{
    if ( Src == nullptr ) { return nullptr; }

    const auto info = new PLUGIN_INFO;
    if ( info == nullptr )
    {
        return nullptr;
    }

    // プラグイン情報のコピー
    *info = *Src;

    info->Name     = CopyString(Src->Name);
    info->Filename = CopyString(Src->Filename);
    info->Commands = (Src->CommandCount == 0) ?
                     nullptr : new PLUGIN_COMMAND_INFO[Src->CommandCount];

    // コマンド情報のコピー
    if ( info->Commands != nullptr && Src->Commands != nullptr )
    {
        for ( size_t i = 0; i < Src->CommandCount; ++i )
        {
            info->Commands[i] = Src->Commands[i];

            info->Commands[i].Name    = CopyString(Src->Commands[i].Name);
            info->Commands[i].Caption = CopyString(Src->Commands[i].Caption);
        }
    }

    return info;
}

//---------------------------------------------------------------------------//

// プラグイン側で作成されたプラグイン情報構造体を破棄する
void FreePluginInfo(PLUGIN_INFO* PluginInfo)
{
    if ( PluginInfo == nullptr ) { return; }

    // コマンド情報構造体配列の破棄
    if ( PluginInfo->Commands != nullptr )
    {
        for ( size_t i = 0; i < PluginInfo->CommandCount; ++i )
        {
            const auto pCI = &PluginInfo->Commands[i];

            DeleteString(pCI->Name);
            DeleteString(pCI->Caption);
        }
        delete[] PluginInfo->Commands;
    }

    DeleteString(PluginInfo->Filename);
    DeleteString(PluginInfo->Name);

    delete PluginInfo;
}

//---------------------------------------------------------------------------//

// デシリアライズ ヘルパー関数
template<typename T>
void desirialize(T* t, const std::vector<uint8_t>& data, size_t* p)
{
    const auto size = sizeof(T);

    ::memcpy(t, data.data() + *p, size);
    *p += size;

    wchar_t buf[16];
    ::StringCchPrintfW(buf, 16, L"%x", *t);
    ::OutputDebugStringW(buf);
    ::OutputDebugStringW(L"\n");
}

// デシリアライズ ヘルパー関数 (文字列版)
template<>
void desirialize<LPWSTR>(LPWSTR* dst, const std::vector<uint8_t>& data, size_t* p)
{
    const auto src = LPWSTR(data.data() + *p);
    const auto size = (1 + lstrlenW(src)) * sizeof(wchar_t);

    *dst = new wchar_t[size];
    ::memcpy(*dst, src, size);
    *p += size;

    ::OutputDebugStringW(*dst);
    ::OutputDebugStringW(L"\n");
}

// プロセス境界を超えるため データをデシリアライズする
PLUGIN_INFO_W* DeserializePluginInfo(const std::vector<uint8_t>& data)
{
    auto info = new PLUGIN_INFO_W;

    size_t p = 0;
    desirialize(&info->NeedVersion,  data, &p);
    desirialize(&(info->Name),       data, &p);
    desirialize(&(info->Filename),   data, &p);
    desirialize(&info->PluginType,   data, &p);
    desirialize(&info->VersionMS,    data, &p);
    desirialize(&info->VersionLS,    data, &p);
    desirialize(&info->CommandCount, data, &p);

    const auto count = info->CommandCount;
    info->Commands = new PLUGIN_COMMAND_INFO_W[count];
    for ( DWORD idx = 0; idx < count; ++idx )
    {
        auto& cmd = info->Commands[idx];

        desirialize(&(cmd.Name),        data, &p);
        desirialize(&(cmd.Caption),     data, &p);
        desirialize(&cmd.CommandID,     data, &p);
        desirialize(&cmd.Attr,          data, &p);
        desirialize(&cmd.ResID,         data, &p);
        desirialize(&cmd.DispMenu,      data, &p);
        desirialize(&cmd.TimerInterval, data, &p);
        desirialize(&cmd.TimerCounter,  data, &p);
    }

    desirialize(&info->LoadTime, data, &p);

    return info;
}

//---------------------------------------------------------------------------//

// Utility.hpp
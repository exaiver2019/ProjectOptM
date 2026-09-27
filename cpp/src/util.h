// Small helpers shared across Project OptM.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>

namespace util {

std::string  Narrow(const std::wstring& w);            // UTF-16 -> UTF-8
std::wstring Widen(const std::string& s);              // UTF-8 -> UTF-16
std::string  Trim(const std::string& s);
std::string  Lower(std::string s);
std::vector<std::string> Split(const std::string& s, char sep);   // trimmed, empties dropped
std::string  Join(const std::vector<std::string>& v, const char* sep);
bool         Contains(const std::vector<std::string>& v, const std::string& s);   // case-insensitive
std::string  StripExe(std::string name);                // "Game.exe" -> "Game"
std::vector<std::string> NameList(const std::string& s);   // "a.exe, b" -> {a, b}: split on commas, .exe dropped
bool         ReadFile(const std::wstring& path, std::string& out);
bool         WriteFile(const std::wstring& path, const std::string& data);
bool         AppendFile(const std::wstring& path, const std::string& data);
int64_t      FileTime(const std::wstring& path);        // last write time, 0 if missing
std::wstring AppDataRoot();                             // %APPDATA%
std::wstring LocalAppDataRoot();                        // %LOCALAPPDATA%
std::wstring AppDataDir();                              // %APPDATA%\ProjectOptM
std::wstring SelfPath();                                // this exe
std::wstring EnvVar(const wchar_t* name);
std::string  RegString(HKEY root, const wchar_t* key, const wchar_t* value);
bool         RegDword(HKEY root, const wchar_t* key, const wchar_t* value, DWORD& out);
bool         SetRegDword(HKEY root, const wchar_t* key, const wchar_t* value, DWORD data);
int          AppxInstalled(const wchar_t* packagePrefix);   // Store app for this user: 1 yes, 0 no, -1 couldn't tell
std::string  FormatHours(double minutes);              // "52.9h" / "40m"
std::string  FormatDuration(double minutes);           // "1h 5m" / "12m"
std::string  NowStamp(const char* fmt);                // strftime of the local time
void         OpenAsUser(const std::wstring& target);    // opens a file/link/folder through Explorer (not as admin)

}  // namespace util

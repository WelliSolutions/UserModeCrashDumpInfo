#pragma once
#include <algorithm> 
#include <cctype>
#include <string>

inline void ltrim(std::wstring& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](std::wstring::value_type ch) {
		return !std::isspace(ch);
		}));
}

inline void rtrim(std::wstring& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](std::wstring::value_type ch) {
		return !std::isspace(ch);
		}).base(), s.end());
}

inline std::wstring trim(std::wstring s) {
	ltrim(s);
	rtrim(s);
	return s;
}
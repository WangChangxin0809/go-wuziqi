/*
 * Rapfi 2018 single-file port, with rule adapter for the contest in 要求.txt.
 * Original engine Copyright (c) 2018 Haobin Duan, MIT License.
 * Upstream: https://github.com/dhbloo/Rapfi-gomocup
 */
#include <algorithm>
#include <cstring>
#include <cstdio>
#define sscanf_s std::sscanf
using WORD = unsigned short;
inline int GetStdHandle(int) { return 0; }
inline void SetConsoleTextAttribute(int, WORD) {}
constexpr int STD_OUTPUT_HANDLE = 0;
constexpr int BACKGROUND_INTENSITY = 0;
constexpr int FOREGROUND_RED = 0;
constexpr int FOREGROUND_GREEN = 0;
using std::reverse;

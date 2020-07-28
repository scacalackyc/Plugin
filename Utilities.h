/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2020 Justin Shannon
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
*/

#ifndef Utilities_h
#define Utilities_h

#include <ctime>
#include <string>
#include <memory>
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>
#include <algorithm>
#include <vector>

#include "Constants.h"
#include "XPLMPlugin.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"

template<typename ... Args>
inline std::string string_format(const std::string& format, Args ... args)
{
	size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1;
	std::unique_ptr<char[]> buf(new char[size]);
	snprintf(buf.get(), size, format.c_str(), args ...);
	return std::string(buf.get(), buf.get() + size - 1);
}

template<class TContainer>
inline bool begins_with(const TContainer& input, const TContainer& match)
{
	return input.size() >= match.size() && std::equal(match.cbegin(), match.cend(), input.cbegin());
}

inline char* strScpy(char* dest, const char* src, size_t size)
{
	strncpy(dest, src, size);
	dest[size - 1] = 0;
	return dest;
}

inline std::string strAtMost(const std::string s, size_t m) {
	return s.length() <= m ? s :
		s.substr(0, m - 3) + "...";
}

#define STRCPY_ATMOST(dest,src) strncpy_s(dest,sizeof(dest),strAtMost(src,sizeof(dest)-1).c_str(),sizeof(dest)-1)

inline const auto str_tolower = [](std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
	return s;
};
inline const auto str_toupper = [](std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return toupper(c); });
	return s;
};

template < class ContainerT >
inline void tokenize(const std::string& str, ContainerT& tokens, const std::string& delimiters = " ", bool trimEmpty = false)
{
	std::string::size_type pos, lastPos = 0, length = str.length();

	using value_type = typename ContainerT::value_type;
	using size_type = typename ContainerT::size_type;

	while (lastPos < length + 1)
	{
		pos = str.find_first_of(delimiters, lastPos);
		if (pos == std::string::npos)
		{
			pos = length;
		}

		if (pos != lastPos || !trimEmpty)
			tokens.push_back(value_type(str.data() + lastPos,
				(size_type)pos - lastPos));

		lastPos = pos + 1;
	}
}

inline void join(const std::vector<std::string>& v, char c, std::string& s)
{
	s.clear();
	auto it = v.begin();
	++it;
	++it;
	for (auto end = v.end(); it != end; ++it) {
		s += *it;
		if (it != v.end() - 1) {
			s += c;
		}
	}
}

inline std::string GetXPlanePath()
{
	char buffer[2048];
	XPLMGetSystemPath(buffer);
	return buffer;
}

inline std::string GetPluginPath()
{
	XPLMPluginID myId = XPLMGetMyID();
	char buffer[2048];
	XPLMGetPluginInfo(myId, nullptr, buffer, nullptr, nullptr);
	char* path = XPLMExtractFileAndPath(buffer);
	return std::string(buffer, 0, path - buffer) + "/../";
}

inline std::string RemoveSystemPath(std::string path)
{
	if (begins_with<std::string>(path, GetXPlanePath()))
	{
		path.erase(0, GetXPlanePath().length());
	}
	return path;
}

inline int CountFilesInPath(const std::string& path)
{
	char buffer[2048];
	int fileCount = 0;
	XPLMGetDirectoryContents(path.c_str(), 0, buffer, sizeof(buffer), nullptr, 0, &fileCount, nullptr);
	return fileCount;
}

inline uint64_t TimeSinceEpochSeconds()
{
	using namespace std::chrono;
	return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

inline std::string UtcTimestamp()
{
	auto tp = std::chrono::system_clock::now();
	std::time_t current_time = std::chrono::system_clock::to_time_t(tp);
	std::tm* timeInfo = std::gmtime(&current_time);
	char buffer[128];
	return std::string(buffer, buffer + strftime(buffer, sizeof(buffer), "%H:%M:%S", timeInfo));
}

inline float GetNetworkTime()
{
	static XPLMDataRef drNetworkTime = nullptr;
	if (!drNetworkTime)
	{
		drNetworkTime = XPLMFindDataRef("sim/network/misc/network_time_sec");
	}
	return XPLMGetDataf(drNetworkTime);
}

enum logLevel
{
	logDEBUG,
	logINFO,
	logWARN,
	logERROR,
	logFATAL
};

inline const char* LOG_LEVEL[] = { "[DEBUG]","[INFO]","[WARN]","[ERROR]","[FATAL]" };

inline const char* Logger(logLevel level, const char* msg, va_list args)
{
	static char buf[2048];

	float secs = GetNetworkTime();
	const unsigned hours = unsigned(secs / 3600.0f);
	secs -= hours * 3600.0f;
	const unsigned mins = unsigned(secs / 60.0f);
	secs -= mins * 60.0f;

	snprintf(buf, sizeof(buf), "%u:%02u:%06.3f %s %s ", hours, mins, secs, PLUGIN_NAME, LOG_LEVEL[level]);
	if (args)
	{
		vsnprintf(&buf[strlen(buf)], sizeof(buf) - strlen(buf) - 1, msg, args);
	}
	size_t length = strlen(buf);
	if (buf[length - 1] != '\n')
	{
		buf[length] = '\n';
		buf[length + 1] = 0;
	}
	return buf;
}

inline void Log(logLevel level, const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	XPLMDebugString(Logger(level, msg, args));
	va_end(args);
}

#define LOG(level,...) {Log(level, __VA_ARGS__);}
#define LOG_DEBUG(...){Log(logDEBUG, __VA_ARGS__);}
#define LOG_INFO(...){Log(logINFO, __VA_ARGS__);}
#define LOG_ERROR(...){Log(logERROR, __VA_ARGS__);}
#define LOG_FATAL(...){Log(logFATAL, __VA_ARGS__);}

#endif // !Utilities_h
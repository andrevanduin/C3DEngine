
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
	#define C3D_PLATFORM_WIN 1
#elif defined(__linux__) || defined(__gnu_linux__)
	#define C3D_PLATFORM_LINUX 1
#endif

#if C3D_PLATFORM_WIN == 1
	#include <Windows.h>
#elif C3D_PLATFORM_LINUX == 1
	#include <time.h>
#endif

#include <cstdio>

int main(int argc, char** argv)
{
#if C3D_PLATFORM_WINDOWS == 1
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);

	printf("%d%02d%02dT%02d%02d%02d", localTime.wDay, localTime.wMonth, localTime.wYear, localTime.wHour, localTime.wMinute, localTime.wSecond);
#elif C3D_PLATFORM_LINUX == 1
	time_t t = time(NULL);
	tm* localTime = localtime(&t);

	printf("%d%02d%02dT%02d%02d%02d", localTime->tm_mday, localTime->tm_mon + 1, localTime->tm_year + 1900, localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
#endif
	return 0;
}
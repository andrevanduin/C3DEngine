
#include <Windows.h>
#include <cstdio>


int main(int argc, char** argv)
{
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);

	printf("%d%02d%02dT%02d%02d%02d", localTime.wDay, localTime.wMonth, localTime.wYear, localTime.wHour, localTime.wMinute, localTime.wSecond);
	return 0;
}

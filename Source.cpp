#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
using namespace std;

#include <Windows.h>

#define _USE_MATH_DEFINES
#include <math.h>

int nScreenWidth = 120;
int nScreenHeight = 40;

float fPlayerX = 8.0f;
float fPlayerY = 8.0f;
float fPlayerA = 0.0f;

int nMapHeight = 16;
int nMapWidth = 16;

float fFOV = M_PI / 4.0;
float fDepth = 16.0f;

int main() {
	//create screen buffer
	wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	wstring map;

	map += L"################";
	map += L"#--------------#";
	map += L"#--------------#";
	map += L"#--------------#";
	map += L"#---#####------#";
	map += L"#-------#------#";
	map += L"#-------#------#";
	map += L"#-------#------#";
	map += L"#--------------#";
	map += L"#--------------#";
	map += L"#--------------#";
	map += L"#--------------#";
	map += L"#--------------#";
	map += L"#--------------#";
	map += L"#--------------#";
	map += L"################";

	auto tp1 = chrono::system_clock::now();
	auto tp2 = chrono::system_clock::now();

	while (1) {
		tp2 = chrono::system_clock::now();
		chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1= tp2;
		float deltaTime = elapsedTime.count();

		//플레이어 조작
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
			fPlayerA -= 1 * deltaTime;
		if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
			fPlayerA += 1 * deltaTime;
		if (GetAsyncKeyState((unsigned short)'W') & 0x8000) {
			fPlayerX += sinf(fPlayerA) * 5.0f* deltaTime;
			fPlayerY += cosf(fPlayerA) * 5.0f * deltaTime;
			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') {
				fPlayerX -= sinf(fPlayerA) * 5.0f * deltaTime;
				fPlayerY -= cosf(fPlayerA) * 5.0f * deltaTime;
			}
		}
		if (GetAsyncKeyState((unsigned short)'S') & 0x8000) {
			fPlayerX -= sinf(fPlayerA) * 5.0f * deltaTime;
			fPlayerY -= cosf(fPlayerA) * 5.0f * deltaTime;
			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') {
				fPlayerX += sinf(fPlayerA) * 5.0f * deltaTime;
				fPlayerY += cosf(fPlayerA) * 5.0f * deltaTime;
			}
		}


		for (int x = 0; x < nScreenWidth; x++) {
			//각 기둥마다, 발사된 ray의 각을 월드좌표계 상으로 계산함
			float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;

			float fDistanceToWall = 0;
			bool bHitWall = false;
			bool bBoundary = false;

			float fEyeX = sinf(fRayAngle); //각 Ray의 단위벡터
			float fEyeY = cosf(fRayAngle);

			//ray의 길이를 .1f씩 늘려가면서 벽에 맞았는지 확인
			while (!bHitWall && fDistanceToWall < fDepth) {
				fDistanceToWall += .1f;

				int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

				//ray가 경계를 벗어났는지 확인
				if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight) {
					bHitWall = true;
					fDistanceToWall = fDepth;
				}
				else {
					//만약 맵의 [현재 ray의 x좌표,y좌표]인덱스가 벽('#')이라면
					if (map.c_str()[nTestY * nMapWidth + nTestX] == '#') {
						bHitWall = true;
						 
						vector<pair<float,float>> p;//거리, 점곱

						for (int tx = 0; tx < 2; tx++) {
							for (int ty = 0; ty < 2; ty++) {
								float vy = (float)nTestY + ty - fPlayerY;
								float vx = (float)nTestX + tx - fPlayerX;
								float d = sqrt(vx * vx + vy * vy);
								float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
								p.push_back(make_pair(d, dot));
							}
						}
						sort(p.begin(), p.end(), [](const pair<float, float>& left, const pair<float, float>& right) {return left.first < right.first; });

						float fBound = .01;
						if (acos(p.at(0).second) < fBound) bBoundary = true;
						if (acos(p.at(1).second) < fBound) bBoundary = true;						
					}
				}
			}

			//바닥과 기둥과의 플레이어의 거리를 계산한다
			int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
			int nFloor = nScreenHeight - nCeiling;

			short nShade = ' ';

			if (fDistanceToWall <= fDepth / 4.0f) nShade = 0x2588;//fully shaded
			else if (fDistanceToWall < fDepth / 2.0f) nShade = 0x2593;
			else if (fDistanceToWall <= fDepth / 2.0f) nShade = 0x2592;
			else if (fDistanceToWall < fDepth) nShade = 0x2591;
			else nShade = ' ';//너무 멂

			if (bBoundary) nShade = 'I';

			for (int y = 0; y < nScreenHeight; y++) {
				if (y <= nCeiling)
					screen[y * nScreenWidth + x] = ' ';
				else if (y > nCeiling && y <= nFloor)
					screen[y * nScreenWidth + x] = nShade;
				else
				{
					float b  = 1 - (((float)y - nScreenHeight / 2) / ((float)nScreenHeight / 2));
					if (b < .25) nShade = '#';
					else if (b < .5) nShade = 'x';
					else if (b < .75) nShade = '-';
					else if (b < .9) nShade = '.';
					else nShade = ' ';
					screen[y * nScreenWidth + x] = nShade;
				}
			}
		}				
		screen[nScreenWidth * nScreenHeight - 1] = '\0';
		WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
	}
	return 0;
}

#include <stdio.h>
#include <easyx.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <Windows.h>
#include <conio.h>
#include <string.h>
#pragma comment(lib,"Winmm.lib") 

#define ScreenWidth 400
#define ScreenHeight 800

#define PlaneSize 50          
#define EnemyNum 8           
#define EnemySpeed 1.0        

#define BulletNum 10          

typedef struct pos {
	int x;
	int y;
} POS; // 2D point structure

typedef struct Plane {
	POS planePos;
	POS planeBullets[BulletNum];
	int bulletLen;                // Number of bullets active
	int bulletSpeed;
} PLANE;

PLANE myPlane, enemyPlanes[EnemyNum];
int enemyPlaneLen;
static time_t startTime, endTime;
IMAGE img[3];
int score = 0;
DWORD lastShootTime = 0;

void initGame();
void drawGame();
void updateGame();
void initEnemyPlane();
void destroyEnemy();
int Intersect(POS c1, POS c2, int radius);
void destroyBullet();

int main() {

	loadimage(&img[0], L"img/Background.png", ScreenWidth, ScreenHeight);
	loadimage(&img[1], L"img/Enemy.png", PlaneSize, PlaneSize);
	loadimage(&img[2], L"img/Player.png", PlaneSize, PlaneSize);

	initGame();

	while (TRUE) {
		drawGame();
		updateGame();
		Sleep(30); // Limit update rate
	}

	return 0;
}

void initGame() {
	// Create graphics window and reset game
	initgraph(ScreenWidth, ScreenHeight);
	score = 0;

	srand((unsigned)time(NULL)); //Random generator for enemy

	myPlane.bulletLen = 0;
	myPlane.bulletSpeed = 3;
	myPlane.planePos = { (ScreenWidth / 2) - (PlaneSize / 2), ScreenHeight - PlaneSize };

	enemyPlaneLen = 0;

	startTime = time(NULL);
}

void drawGame() {
	BeginBatchDraw();


	putimage(0, 0, &img[0]);
	putimage(myPlane.planePos.x - (PlaneSize / 3), myPlane.planePos.y - (PlaneSize / 3), &img[2], SRCAND);

	// Draw each enemy plane currently active
	for (int i = 0; i < enemyPlaneLen; i++) {
		putimage(enemyPlanes[i].planePos.x - (PlaneSize / 3), enemyPlanes[i].planePos.y - (PlaneSize / 3), &img[1], SRCAND);
	}

	// Draw player bullets as circles
	setfillcolor(RGB(255, 0, 0));
	for (int i = 0; i < myPlane.bulletLen; i++) {
		fillcircle(myPlane.planeBullets[i].x, myPlane.planeBullets[i].y, PlaneSize / 4);
	}


	// Draw the score text
	RECT rect = { 0, PlaneSize, ScreenWidth, ScreenHeight };
	wchar_t str[30] = { 0 };
	swprintf_s(str, L"Score: %d", score);
	setbkmode(TRANSPARENT);
	settextcolor(RGB(0, 255, 255));
	settextstyle(20, 15, L"TIMES NEW ROMAN");
	drawtext(str, &rect, DT_TOP | DT_CENTER);

	EndBatchDraw();
}

void updateGame() {
	// Key binds for player movement

	if (GetAsyncKeyState('W') & 0x8000) myPlane.planePos.y -= 4;
	if (GetAsyncKeyState('S') & 0x8000) myPlane.planePos.y += 4;
	if (GetAsyncKeyState('A') & 0x8000) myPlane.planePos.x -= 4;
	if (GetAsyncKeyState('D') & 0x8000) myPlane.planePos.x += 4;

	//shoot bullet
	if (GetAsyncKeyState(VK_SPACE) & 0x8000) {

		DWORD now = GetTickCount();

		// 300ms down
		if (now - lastShootTime > 300) {

			if (myPlane.bulletLen < BulletNum) {

				PlaySound(
					L"img/Bullet.wav",
					NULL,
					SND_FILENAME | SND_ASYNC | SND_NOWAIT
				);

				myPlane.planeBullets[myPlane.bulletLen] =
					myPlane.planePos;

				myPlane.bulletLen++;

				// update last shoot time
				lastShootTime = now;
			}
		}
	}

	// keep player inside screen
	if (myPlane.planePos.x < 0) myPlane.planePos.x = 0;
	if (myPlane.planePos.x > ScreenWidth - PlaneSize) myPlane.planePos.x = ScreenWidth - PlaneSize;
	if (myPlane.planePos.y < 0) myPlane.planePos.y = 0;
	if (myPlane.planePos.y > ScreenHeight - PlaneSize) myPlane.planePos.y = ScreenHeight - PlaneSize;

	for (int i = 0; i < enemyPlaneLen; i++) {
		enemyPlanes[i].planePos.y += 2;
	}

	for (int i = 0; i < myPlane.bulletLen; i++) {
		myPlane.planeBullets[i].y -= myPlane.bulletSpeed;
	}

	initEnemyPlane();
	destroyEnemy();
	destroyBullet();
}

void initEnemyPlane() {
	endTime = time(NULL);
	double elapsedTime = difftime(endTime, startTime);
	if (elapsedTime >= EnemySpeed) {
		if (enemyPlaneLen < EnemyNum) {
			int x = (rand() % (ScreenWidth - 2 * PlaneSize)) + PlaneSize;
			int y = -PlaneSize;

			enemyPlanes[enemyPlaneLen].planePos = { x, y };
			enemyPlaneLen++;
		}
		startTime = endTime;
	}
}

void destroyEnemy() {

	for (int i = 0; i < enemyPlaneLen; i++) {
		if (Intersect(myPlane.planePos, enemyPlanes[i].planePos, PlaneSize * 2 / 3)) {
			if (IDYES == MessageBox(GetHWnd(), L"Game Over! Want to restart?", L"Game Over", MB_ICONEXCLAMATION | MB_YESNO)) {
				initGame();
			}
			else {
				exit(0);
			}
		}
		else if (enemyPlanes[i].planePos.y > ScreenHeight) {
			for (int j = i; j < enemyPlaneLen; j++) {
				enemyPlanes[j] = enemyPlanes[j + 1];
			}
			enemyPlaneLen--;
			i--;
		}

	}
}

void destroyBullet() {
	for (int i = 0; i < myPlane.bulletLen; i++) {
		for (int j = 0; j < enemyPlaneLen; j++) {
			if (Intersect(myPlane.planeBullets[i], enemyPlanes[j].planePos, PlaneSize / 4 + PlaneSize / 2)) {
				for (int x = i; x < myPlane.bulletLen; x++) {
					myPlane.planeBullets[x] = myPlane.planeBullets[x + 1];
				}
				for (int x = j; x < enemyPlaneLen; x++) {
					enemyPlanes[x] = enemyPlanes[x + 1];
				}
				enemyPlaneLen--;
				myPlane.bulletLen--;
				j--;
				score += 500;
			}
		}
		if (myPlane.planeBullets[i].y < 0) {
			for (int x = i; x < myPlane.bulletLen; x++) {
				myPlane.planeBullets[x] = myPlane.planeBullets[x + 1];
			}
			myPlane.bulletLen--;
			i--;
		}
	}
}

int Intersect(POS c1, POS c2, int radius) {
	return abs(c1.x - c2.x) <= radius && abs(c1.y - c2.y) <= radius;
}
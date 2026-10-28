#include "game.h"


// 개미(7 x 3)를 정해진 구간에서 좌우로 왕복시킨다 [그림 11]
void UpdateAnts(Game* g) {

	int i;
	int tempX;
	for (i = 0;i < MAX_ANTS;i++) {//개미 마릿수 만큼

		if (g->ants[i].alive == 0) {  //죽은 개미는 건더 뜀
			continue;
		}

		if (g->ants[i].moveTick > 0) { // 4프레임 마다 움직여서 0보다 크면 패쓰
			g->ants[i].moveTick--;
			continue;
		}

		g->ants[i].moveTick = ANT_TERM;
		tempX = g->ants[i].x + g->ants[i].dir * ANT_STEP;

		// 앞쪽 끝 세로줄 (오른쪽으로 가면 몸 오른쪽 끝, 왼쪽으로 가면 왼쪽 끝)
		int front = (g->ants[i].dir > 0) ? tempX + ANT_W - 1 : tempX;
		int frontBlocked = 0;
		for (int y = g->ants[i].y - (ANT_H - 1); y <= g->ants[i].y; y++) {
			if (IsSolid(&g->map, front, y) == 1) {
				frontBlocked = 1;
			}
		}

		if (tempX + ANT_W - 1 > g->ants[i].right || tempX < g->ants[i].left || frontBlocked == 1) { // 구간 끝이거나 앞이 막히면 돌아선다
			g->ants[i].dir = -g->ants[i].dir;
		}
		else {
			g->ants[i].x = tempX;
		}
	}
}

// i 번 버튼이 차지하는 칸을 맵에 막힌 칸('B')으로 표시한다
// 아랫줄(y)은 항상 버튼, 윗줄(y-1)은 안 눌렸을 때만 버튼이고 눌리면 빈칸
void SetButtonTiles(Game* g, int i) {
	Button* b = &g->buttons[i];

	if (b->y < 1 || b->y >= MAP_H || b->x < 0 || b->x + BUTTON_W > MAP_W) {
		return;
	}
	for (int x = b->x; x < b->x + BUTTON_W; x++) {
		g->map.tile[b->y][x] = 'B';
		g->map.tile[b->y - 1][x] = (b->pressed == 0) ? 'B' : ' ';
	}
}

// 버튼(2 x 2)은 밟을 수 있는 막힌 칸이다 [그림 7]
// 위에 올라서는 순간 눌려서 2 x 1 로 납작해지고(플레이어는 한 칸 내려앉음), 연결된 선풍기를 켜거나 끈다
// 플레이어가 버튼에서 내려가면 다시 2 x 2 로 올라온다
void UpdateButtons(Game* g) {
	for (int i = 0;i < MAX_BUTTONS;i++) {
		Button* b = &g->buttons[i];

		if (g->map.tile[b->y][b->x] != 'B') {
			continue; // 맵에 없는 버튼 자리
		}

		// 플레이어 몸(가로 3칸)과 버튼(가로 2칸)이 가로로 겹치는가
		int overlapX = (g->p.x + PLAYER_HALF >= b->x) && (g->p.x - PLAYER_HALF <= b->x + BUTTON_W - 1);

		if (b->pressed == 0) {
			// 안 눌린 버튼 윗면(y-1) 바로 위에 서면 눌린다
			if (overlapX && g->p.onGround && g->p.y == b->y - BUTTON_H) {
				b->pressed = 1;
				g->fans[b->fanId].on = !g->fans[b->fanId].on; // 선풍기 ON <-> OFF
				SetButtonTiles(g, i);
			}
		}
		else {
			// 옆으로 내려갔거나, 원래 버튼 높이보다 위로 뛰어올랐으면 다시 올라온다
			if (!overlapX || g->p.y < b->y - BUTTON_H) {
				b->pressed = 0;
				SetButtonTiles(g, i);
			}
		}
	}
}

// i 번 선풍기가 차지하는 5 x 3 칸을 맵에 막힌 칸('F')으로 표시한다 (위에 설 수 있는 물체)
void SetFanTiles(Game* g, int i) {
	Fan* f = &g->fans[i];

	for (int y = f->y - (FAN_H - 1); y <= f->y; y++) {
		for (int x = f->x - FAN_W / 2; x <= f->x + FAN_W / 2; x++) {
			if (x >= 0 && x < MAP_W && y >= 0 && y < MAP_H) {
				g->map.tile[y][x] = 'F';
			}
		}
	}
}

// 켜진 선풍기의 바람 칸을 계산해 wind[y][x] 에 기록한다 [그림 8]
// 바람은 선풍기 윗면 바로 위 줄부터 위로 range 칸, 가로 WIND_W(3) 칸. 칸마다 발판에 막히면 그 위로는 없다
void UpdateFans(Game* g) {
	Fan fan;
	int i, y;
	memset(g->wind, 0, sizeof(g->wind)); //바람 칸 전부 0

	for (i = 0;i < MAX_FANS;i++) {
		fan = g->fans[i];
		if (fan.on == 0) {
			continue;
		}

		int top = fan.y - FAN_H;   // 선풍기 윗면 바로 위 줄 (바람이 시작되는 줄)

		for (int x = fan.x - WIND_W / 2; x <= fan.x + WIND_W / 2; x++) { // 바람 기둥 가로 3칸
			for (y = 0;y < fan.range;y++) { // 바람이 도달하는 거리
				int wy = top - y;

				if (x < 0 || x >= MAP_W || wy < 0 || IsSolid(&g->map, x, wy) == 1) { // 맵 밖이거나 막혀 있으면 그 위로는 바람 없음
					break;
				}
				g->wind[wy][x] = 5;
			}
		}
	}
}

void ApplyFanWind(Game* g) {
	int onWind = 0;

	// 발 3칸 중 하나라도 바람 칸에 있으면 바람을 탄다
	for (int x = g->p.x - PLAYER_HALF; x <= g->p.x + PLAYER_HALF; x++) {
		if (x >= 0 && x < MAP_W && g->p.y >= 0 && g->p.y < MAP_H && g->wind[g->p.y][x] == 5) {
			onWind = 1;
		}
	}

	if (onWind == 1) {
		g->p.vy -= FAN_POWER;
		if (g->p.vy < -MAX_RISE) {
			g->p.vy = -MAX_RISE;
		}
	}
}

int SpawnDrop(Game* g) {
	int i;

	for (i = 0;i < MAX_DROPS;i++) {
		if (g->drops[i].active == 0) { //낙하물이 만들어질 자리가 비워져있으면
			g->drops[i].x = RandRange(DROP_W / 2 + 1, MAP_W - 2 - DROP_W / 2);// 맵 x좌표에 랜덤으로 생성 (5칸 몸이 양쪽 벽 안에 들어가게)
			g->drops[i].y = DROP_H - 1;// 맨 아랫줄 y. 돌 전체(3줄)가 맵 맨 위에서 시작
			g->drops[i].prevY = g->drops[i].y; //
			g->drops[i].state = DROP_SHAKE;//상태 저장
			g->drops[i].tick = SHAKE_TICK;// 틱 초기화
			g->drops[i].active = 1;
			g->drops[i].shakeX = 0;

			return i;
		}

	}
	return -1;
}

void UpdateDrops(Game* g) {
	int i;

	if (g->frame % SPAWN_TERM == 0) { // 프레임 주기가 되고
		if (SPAWN_RATE > rand() % 100) {//생성 확률이 넘었으면
			SpawnDrop(g);			// 생성물 생성
		}
	}

	for (i = 0;i < MAX_DROPS;i++) {

		if (g->drops[i].active == 0) {// 자리 사용중이 다음 장애물 검사
			continue;
		}
		else {
			g->drops[i].prevY = g->drops[i].y;
			if (g->drops[i].state == DROP_SHAKE) {
				g->drops[i].tick--;
				if (g->drops[i].tick % 2 == 1) { // 프레임 수가 홀수면
					g->drops[i].shakeX = 1; // 흔들림 1저장
				}
				else {
					g->drops[i].shakeX = -1;// 짝수면 흔들림 -1저장
				}

				if (g->drops[i].tick == 0) { //프레임 수가 0이면
					g->drops[i].state = DROP_FALL; // 상태 떨어짐
					g->drops[i].shakeX = 0;
				}
			}
			else if (g->drops[i].state == DROP_FALL) {// 상태가 떨어짐일때
				int blocked = 0;
				for (int x = g->drops[i].x - DROP_W / 2; x <= g->drops[i].x + DROP_W / 2; x++) { // 돌 바로 아래 5칸 검사
					if (IsSolid(&g->map, x, g->drops[i].y + 1) == 1) {
						blocked = 1;
					}
				}
				if (blocked == 0) { // 낙하물 이동 가능이면
					g->drops[i].y++; // 이동
				}
				else { // 이동 불가능이면
					g->drops[i].state = DROP_LAND; //상태 착지로 저장
					g->drops[i].tick = LAND_TICK;
				}
			}
			else if (g->drops[i].state == DROP_LAND) {// 상태가 착지일때
				g->drops[i].tick--;//틱 감소
				if (g->drops[i].tick == 0) { //틱 0되면
					g->drops[i].active = 0; // 장애물 지우기
				}
			}
		}
	}
}

// ms 동안 기다리되 CUT_SLICE_MS(10ms) 씩 나눠 자며 키를 확인한다
// 키가 눌렸으면 그 키를 읽어서 버리고 1 을 반환한다 (버리지 않으면 다음 화면이나 게임이 그 키를 받아 버린다)
// Sleep(10) 은 실제로 약 15ms 씩 쉬어서 횟수로 세면 시간이 1.5배로 늘어난다. 그래서 GetTickCount 로 실제 흐른 시간을 잰다
int WaitCut(int ms) {
	DWORD start = GetTickCount();

	while (GetTickCount() - start < (DWORD)ms) {

		Sleep(CUT_SLICE_MS);
		if (_kbhit()) {
			int key = _getch();
			if (key == 0 || key == 224) {   // 방향키 등은 두 번 읽어야 한다
				_getch();
			}
			return 1;
		}
	}
	return 0;
}

/////  컷만화용 그림 (SCREEN_W x SCREEN_H 화면 기준) //////

// 큰 사람 (k = 3, 원래 크기) : 가로 11칸 x 세로 13줄
static const char* BIG_MAN[] = {
	"   _____   ",
	"  (     )  ",
	" (  o o  ) ",
	" (   >   ) ",
	"  ( --- )  ",
	"   (___)   ",
	"     |     ",
	"<----+---->",
	"     |     ",
	"     |     ",
	"    | |    ",
	"    | |    ",
	"   _| |_   ",
};

// 중간 사람 (k = 2, 작아지는 중 / 커지는 중) : 가로 7칸 x 세로 7줄
static const char* MID_MAN[] = {
	"  ___  ",
	" (o o) ",
	"  (_)  ",
	"<--+-->",
	"   |   ",
	"  | |  ",
	" _| |_ ",
};

// 작은 사람 (k = 1, 게임 속 모습과 같음) : 가로 3칸 x 세로 3줄
static const char* SMALL_MAN[] = {
	"(o)",
	"<|>",
	"| |",
};

// 쓰러진 작은 사람 (게임 오버) : 가로 6칸 x 세로 1줄
static const char* SMALL_MAN_DOWN[] = {
	"(x)=<|",
};

// 바닥에 놓인 약병 : 가로 9칸 x 세로 7줄
static const char* BOTTLE[] = {
	"  _____  ",
	" |_____| ",
	"  |   |  ",
	" (     ) ",
	"|  ???  |",
	"|       |",
	"|_______|",
};

// 입에 대고 마시는 약병 (2컷) : 가로 7칸 x 세로 4줄
static const char* BOTTLE_DRINK[] = {
	" ___   ",
	"|___|==",
	"|???|  ",
	"|___|  ",
};

#define CUT_FRAME_TOP    1                     // 그림 테두리 윗줄
#define CUT_TALK_BOTTOM  (SCREEN_H - 2)        // 대사 상자 아랫줄 (화면 맨 아래 바로 위)
#define CUT_TALK_TOP     (CUT_TALK_BOTTOM - 5) // 대사 상자 윗줄
#define CUT_FRAME_BOTTOM (CUT_TALK_TOP - 1)    // 그림 테두리 아랫줄
#define CUT_FLOOR_Y      (CUT_FRAME_BOTTOM - 2)// 방바닥 줄. 사람과 약병은 이 줄 바로 위에 선다
#define CUT_TALK_LINE1   (CUT_TALK_TOP + 2)    // 대사 첫째 줄
#define CUT_TALK_LINE2   (CUT_TALK_TOP + 3)    // 대사 둘째 줄
#define CUT_TALK_LINE3   (CUT_TALK_TOP + 4)    // 대사 셋째 줄 (다시 하기 안내)

// 버퍼의 (x, y) 부터 문자열을 쓴다. 화면 밖으로 나가는 글자는 버린다
static void PutText(char buf[][SCREEN_W + 1], int x, int y, const char* s) {
	if (y < 0 || y >= SCREEN_H) {
		return;
	}
	for (int i = 0; s[i] != '\0'; i++) {
		if (x + i >= 0 && x + i < SCREEN_W) {
			buf[y][x + i] = s[i];
		}
	}
}

// 문자열을 y 줄 가운데에 쓴다
static void PutCenter(char buf[][SCREEN_W + 1], int y, const char* s) {
	PutText(buf, (SCREEN_W - (int)strlen(s)) / 2, y, s);
}

// 여러 줄짜리 그림을 그린다. bottomY 는 그림 맨 아랫줄이 놓일 y
static void PutArt(char buf[][SCREEN_W + 1], int x, int bottomY, const char* art[], int lines) {
	for (int i = 0; i < lines; i++) {
		PutText(buf, x, bottomY - (lines - 1) + i, art[i]);
	}
}

// (x1, y1) ~ (x2, y2) 에 테두리 상자를 그린다
static void PutBox(char buf[][SCREEN_W + 1], int x1, int y1, int x2, int y2) {
	for (int x = x1; x <= x2; x++) {
		buf[y1][x] = '-';
		buf[y2][x] = '-';
	}
	for (int y = y1; y <= y2; y++) {
		buf[y][x1] = '|';
		buf[y][x2] = '|';
	}
	buf[y1][x1] = '+';
	buf[y1][x2] = '+';
	buf[y2][x1] = '+';
	buf[y2][x2] = '+';
}

// 크기 k(1~3)에 맞는 사람을 x 위치에 바닥 위로 세운다
static void PutMan(char buf[][SCREEN_W + 1], int x, int k) {
	if (k >= 3) {
		PutArt(buf, x, CUT_FLOOR_Y - 1, BIG_MAN, 13);
	}
	else if (k == 2) {
		PutArt(buf, x + 2, CUT_FLOOR_Y - 1, MID_MAN, 7);
	}
	else {
		PutArt(buf, x + 4, CUT_FLOOR_Y - 1, SMALL_MAN, 3);
	}
}

// 컷 번호
//  1 ~ 4 : 인트로 (약을 마시고 작아진다)
//  5 ~ 8 : 클리어 엔딩 (약을 다시 마시고 커진다)
//  9     : 게임 오버
#define CUT_ENDING_FIRST 5
#define CUT_ENDING_LAST  8
#define CUT_GAMEOVER     9

// 컷만화 한 컷(테두리 + 그림 + 대사)을 화면 버퍼에 그린다. 출력은 하지 않는다
// k 는 사람 크기(1~3). 0 이면 컷마다 정해진 크기를 쓴다
static void BuildCut(char buf[][SCREEN_W + 1], int cut, int k) {
	char title[40];
	const int manX = 38;              // 사람이 서는 x 위치

	ClearBuffer(buf);

	// 맨 윗줄 : 몇 번째 컷인지 + 건너뛰기 안내
	if (cut < CUT_ENDING_FIRST) {
		snprintf(title, sizeof(title), "[ 인트로 %d / 4 ]", cut);
		PutText(buf, SCREEN_W - 32, 0, "아무 키나 누르면 건너뜁니다");
	}
	else if (cut <= CUT_ENDING_LAST) {
		snprintf(title, sizeof(title), "[ 엔딩 %d / 4 ]", cut - CUT_ENDING_FIRST + 1);
		if (cut < CUT_ENDING_LAST) {
			PutText(buf, SCREEN_W - 32, 0, "아무 키나 누르면 건너뜁니다");
		}
	}
	else {
		snprintf(title, sizeof(title), "[ GAME OVER ]");
	}
	PutText(buf, 2, 0, title);

	// 그림 테두리와 방바닥
	PutBox(buf, 0, CUT_FRAME_TOP, SCREEN_W - 1, CUT_FRAME_BOTTOM);
	for (int x = 1; x < SCREEN_W - 1; x++) {
		buf[CUT_FLOOR_Y][x] = '_';
	}

	// 대사 상자
	PutBox(buf, 0, CUT_TALK_TOP, SCREEN_W - 1, CUT_TALK_BOTTOM);

	switch (cut) {

	/////  인트로 //////

	case 1: // 큰 사람이 바닥의 약병을 발견
		PutMan(buf, manX, 3);
		PutArt(buf, manX + 30, CUT_FLOOR_Y - 1, BOTTLE, 7);
		PutText(buf, manX + 14, CUT_FLOOR_Y - 12, "?");
		PutCenter(buf, CUT_TALK_LINE1, "이 약은 뭐지?");
		break;

	case 2: // 약병을 입에 대고 마심
		PutMan(buf, manX, 3);
		PutArt(buf, manX + 11, CUT_FLOOR_Y - 10, BOTTLE_DRINK, 4);
		PutCenter(buf, CUT_TALK_LINE1, "한번 마셔볼까?");
		break;

	case 3: // 몸이 작아지는 중
		if (k < 1 || k > 3) {
			k = 2;
		}
		PutMan(buf, manX, k);
		PutText(buf, manX - 6, CUT_FLOOR_Y - 8, "~ ~ ~");
		PutText(buf, manX + 13, CUT_FLOOR_Y - 8, "~ ~ ~");
		PutText(buf, manX - 4, CUT_FLOOR_Y - 4, "* *");
		PutText(buf, manX + 13, CUT_FLOOR_Y - 4, "* *");
		PutCenter(buf, CUT_TALK_LINE1, "어...? 몸이 작아지고 있어!");
		break;

	case 4: // 아주 작아짐. 멀리 책상 위에 약병(목표)이 보인다
		PutMan(buf, manX, 1);
		for (int x = 80; x <= 114; x++) {
			buf[CUT_FLOOR_Y - 12][x] = '=';                 // 책상 판
		}
		for (int y = CUT_FLOOR_Y - 11; y < CUT_FLOOR_Y; y++) {
			buf[y][82] = '#';                // 책상 다리
			buf[y][112] = '#';
		}
		PutText(buf, 106, CUT_FLOOR_Y - 13, "*");           // 책상 위 약병
		PutText(buf, 96, CUT_FLOOR_Y - 15, "저기 약병이...!");
		PutCenter(buf, CUT_TALK_LINE1, "너무 작아졌잖아!");
		PutCenter(buf, CUT_TALK_LINE2, "원래대로 돌아갈 방법을 찾아야 해...");
		break;

	/////  클리어 엔딩 (인트로의 반대 : 작은 사람 -> 큰 사람) //////

	case 5: // 작은 사람이 약병 앞에 섬
		PutMan(buf, manX, 1);
		PutArt(buf, manX + 12, CUT_FLOOR_Y - 1, BOTTLE, 7);
		PutText(buf, manX + 6, CUT_FLOOR_Y - 5, "?");
		PutCenter(buf, CUT_TALK_LINE1, "이 약을 마시면 돌아갈 수 있을까?");
		break;

	case 6: // 커지는 중 (k = 1 -> 2 -> 3 으로 다시 그리며 자라는 모습)
		if (k < 1 || k > 3) {
			k = 2;
		}
		PutMan(buf, manX, k);
		PutText(buf, manX - 6, CUT_FLOOR_Y - 8, "^ ^ ^");
		PutText(buf, manX + 13, CUT_FLOOR_Y - 8, "^ ^ ^");
		PutText(buf, manX - 4, CUT_FLOOR_Y - 4, "* *");
		PutText(buf, manX + 13, CUT_FLOOR_Y - 4, "* *");
		PutCenter(buf, CUT_TALK_LINE1, "점점 커지고 있어!");
		break;

	case 7: // 거의 원래 크기
		PutMan(buf, manX, 3);
		PutText(buf, manX - 6, CUT_FLOOR_Y - 10, "* *");
		PutText(buf, manX + 14, CUT_FLOOR_Y - 10, "* *");
		PutCenter(buf, CUT_TALK_LINE1, "거의 원래대로 돌아왔어!");
		break;

	case 8: // 원래대로! (시간·다시 하기 안내는 ShowEnding 이 덧붙인다)
		PutMan(buf, manX, 3);
		PutText(buf, manX - 8, CUT_FLOOR_Y - 14, "*  .  *");
		PutText(buf, manX + 13, CUT_FLOOR_Y - 14, "*  .  *");
		PutCenter(buf, CUT_FLOOR_Y - 18, "G A M E   C L E A R");
		PutCenter(buf, CUT_TALK_LINE1, "드디어 원래대로 돌아왔다!");
		break;

	/////  게임 오버 //////

	case CUT_GAMEOVER: // 작아진 채 쓰러짐
		PutArt(buf, manX + 2, CUT_FLOOR_Y - 1, SMALL_MAN_DOWN, 1);
		PutCenter(buf, CUT_FLOOR_Y - 14, "G A M E   O V E R");
		PutCenter(buf, CUT_TALK_LINE1, "으앗... 작은 몸으로는 버티지 못했어...");
		break;

	default:
		break;
	}
}

// 컷만화 한 컷을 120 x 36 화면 전체에 그린다
// k 는 사람 크기(1~3). 0 이면 컷마다 정해진 크기를 쓴다
void DrawCut(int cut, int k) {
	char buf[SCREEN_H][SCREEN_W + 1]; // 화면 버퍼 (Render 와 같은 방식)

	BuildCut(buf, cut, k);
	FlushBuffer(buf); // 완성된 컷을 한 번에 출력
}

void ShowIntro() {
	int cut;
	int k = 0;

	for (cut = 1;cut <= 4;cut++) {
		DrawCut(cut, k);
		if (WaitCut(2000) == 1) {
			break;
		}
	}
}

// 클리어면 커지는 엔딩 컷만화를, 게임 오버면 GAME OVER 화면을 보여 주고 다시 할지 묻는다 [그림 16]
// R 을 누르면 1, Q 를 누르면 0 을 반환한다. 게임 루프 밖이므로 키를 기다려도 된다
int ShowEnding(Game* g) {
	char buf[SCREEN_H][SCREEN_W + 1];
	char line[80];
	int key;
	int skipped = 0;

	if (g->state == ST_EXIT) {
		return 0;
	}

	// 게임 중에 눌러 둔 키가 남아 있으면 다시 하기 질문에 바로 답해 버리므로 비운다
	while (_kbhit()) {
		_getch();
	}

	if (g->state == ST_CLEAR) {
		// 엔딩 1컷 : 약병 앞
		DrawCut(5, 0);
		skipped = WaitCut(2000);

		// 엔딩 2컷 : 작은 -> 중간 -> 큰 사람으로 자란다 (인트로의 반대)
		for (int k = 1; k <= 3 && !skipped; k++) {
			DrawCut(6, k);
			skipped = WaitCut(700);
		}

		// 엔딩 3컷 : 거의 원래대로
		if (!skipped) {
			DrawCut(7, 0);
			skipped = WaitCut(2000);
		}

		// 엔딩 4컷 : 원래대로 + 걸린 시간 + 다시 하기 안내 (건너뛰어도 마지막 컷은 꼭 보여 준다)
		BuildCut(buf, 8, 0);
		snprintf(line, sizeof(line), "걸린 시간 : %lu 초", g->frame * FRAME_MS / 1000);
		PutCenter(buf, CUT_TALK_LINE2, line);
	}
	else {
		// 게임 오버
		BuildCut(buf, CUT_GAMEOVER, 0);
		snprintf(line, sizeof(line), "버틴 시간 : %lu 초", g->frame * FRAME_MS / 1000);
		PutCenter(buf, CUT_TALK_LINE2, line);
	}

	PutCenter(buf, CUT_TALK_LINE3, "다시 하기? ( R : 다시 / Q : 끝내기 )");
	FlushBuffer(buf);

	// R 또는 Q 가 눌릴 때까지 기다린다 (다른 키는 무시)
	while (1) {
		key = _getch();
		if (key == 0 || key == 224) {   // 방향키 등은 두 번째 값까지 읽고 무시
			_getch();
			continue;
		}
		if (key == 'r' || key == 'R') {
			return 1;
		}
		if (key == 'q' || key == 'Q') {
			return 0;
		}
	}
}

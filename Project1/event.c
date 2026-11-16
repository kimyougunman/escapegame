#include "game.h"


// 개미 좌우로 왔다갔다
void UpdateAnts(Game* g) {

	int i;
	int tempX;
	for (i = 0;i < MAX_ANTS;i++) {

		if (g->ants[i].alive == 0) {
			continue;
		}

		if (g->ants[i].moveTick > 0) { // 몇 프레임마다 한번씩만 움직임
			g->ants[i].moveTick--;
			continue;
		}

		g->ants[i].moveTick = ANT_TERM;
		tempX = g->ants[i].x + g->ants[i].dir * ANT_STEP;

		// 가는 방향 앞쪽 줄에 벽 있는지 확인
		int front = (g->ants[i].dir > 0) ? tempX + ANT_W - 1 : tempX;
		int frontBlocked = 0;
		for (int y = g->ants[i].y - (ANT_H - 1); y <= g->ants[i].y; y++) {
			if (IsSolid(&g->map, front, y) == 1) {
				frontBlocked = 1;
			}
		}

		if (tempX + ANT_W - 1 > g->ants[i].right || tempX < g->ants[i].left || frontBlocked == 1) { // 끝이면 방향 바꿈
			g->ants[i].dir = -g->ants[i].dir;
		}
		else {
			g->ants[i].x = tempX;
		}
	}
}

// 버튼 자리를 맵에 B로 표시
void SetButtonTiles(Game* g, int i) {
	Button* b = &g->buttons[i];

	if (b->y < 1 || b->y >= MAP_H || b->x < 0 || b->x + BUTTON_W > MAP_W) {
		return;
	}
	for (int x = b->x; x < b->x + BUTTON_W; x++) {
		g->map.tile[b->y][x] = 'B';
		g->map.tile[b->y - 1][x] = (b->pressed == 0) ? 'B' : ' '; // 눌리면 윗칸은 비움
	}
}

// 버튼 위에 올라가면 눌리고 선풍기 on/off
void UpdateButtons(Game* g) {
	for (int i = 0;i < MAX_BUTTONS;i++) {
		Button* b = &g->buttons[i];

		if (g->map.tile[b->y][b->x] != 'B') {
			continue;
		}

		// 플레이어랑 버튼이 가로로 겹치는지
		int overlapX = (g->p.x + PLAYER_HALF >= b->x) && (g->p.x - PLAYER_HALF <= b->x + BUTTON_W - 1);

		if (b->pressed == 0) {
			if (overlapX && g->p.onGround && g->p.y == b->y - BUTTON_H) {
				b->pressed = 1;
				g->fans[b->fanId].on = !g->fans[b->fanId].on;
				SetButtonTiles(g, i);
			}
		}
		else {
			// 버튼에서 내려가면 다시 올라옴
			if (!overlapX || g->p.y < b->y - BUTTON_H) {
				b->pressed = 0;
				SetButtonTiles(g, i);
			}
		}
	}
}

// 선풍기 자리를 맵에 F로 표시 (위에 올라설 수 있음)
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

// 켜진 선풍기 위쪽에 바람 칸 표시
void UpdateFans(Game* g) {
	Fan fan;
	int i, y;
	memset(g->wind, 0, sizeof(g->wind));

	for (i = 0;i < MAX_FANS;i++) {
		fan = g->fans[i];
		if (fan.on == 0) {
			continue;
		}

		int top = fan.y - FAN_H;   // 선풍기 바로 위

		for (int x = fan.x - WIND_W / 2; x <= fan.x + WIND_W / 2; x++) {
			for (y = 0;y < fan.range;y++) {
				int wy = top - y;

				if (x < 0 || x >= MAP_W || wy < 0 || IsSolid(&g->map, x, wy) == 1) { // 막히면 바람 끝
					break;
				}
				g->wind[wy][x] = 5;
			}
		}
	}
}

void ApplyFanWind(Game* g) {
	int onWind = 0;

	// 발이 바람 칸에 있으면 위로 뜸
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
		if (g->drops[i].active == 0) { // 빈 자리에 만들기
			g->drops[i].x = RandRange(DROP_W / 2 + 1, MAP_W - 2 - DROP_W / 2);
			g->drops[i].y = DROP_H - 1;
			g->drops[i].prevY = g->drops[i].y;
			g->drops[i].state = DROP_SHAKE;
			g->drops[i].tick = SHAKE_TICK;
			g->drops[i].active = 1;
			g->drops[i].shakeX = 0;

			return i;
		}

	}
	return -1;
}

void UpdateDrops(Game* g) {
	int i;

	if (g->frame % SPAWN_TERM == 0) {
		if (SPAWN_RATE > rand() % 100) { // 확률로 생성
			SpawnDrop(g);
		}
	}

	for (i = 0;i < MAX_DROPS;i++) {

		if (g->drops[i].active == 0) {
			continue;
		}
		else {
			g->drops[i].prevY = g->drops[i].y;
			if (g->drops[i].state == DROP_SHAKE) { // 떨어지기 전에 흔들림
				g->drops[i].tick--;
				if (g->drops[i].tick % 2 == 1) {
					g->drops[i].shakeX = 1;
				}
				else {
					g->drops[i].shakeX = -1;
				}

				if (g->drops[i].tick == 0) {
					g->drops[i].state = DROP_FALL;
					g->drops[i].shakeX = 0;
				}
			}
			else if (g->drops[i].state == DROP_FALL) {
				int blocked = 0;
				for (int x = g->drops[i].x - DROP_W / 2; x <= g->drops[i].x + DROP_W / 2; x++) { // 아래 막혔는지
					if (IsSolid(&g->map, x, g->drops[i].y + 1) == 1) {
						blocked = 1;
					}
				}
				if (blocked == 0) {
					g->drops[i].y++;
				}
				else {
					g->drops[i].state = DROP_LAND;
					g->drops[i].tick = LAND_TICK;
				}
			}
			else if (g->drops[i].state == DROP_LAND) { // 바닥에 잠깐 있다가 사라짐
				g->drops[i].tick--;
				if (g->drops[i].tick == 0) {
					g->drops[i].active = 0;
				}
			}
		}
	}
}

// ms 동안 기다림. 키 누르면 1 반환
// Sleep 횟수로 세면 시간이 틀어져서 GetTickCount로 잼
int WaitCut(int ms) {
	DWORD start = GetTickCount();

	while (GetTickCount() - start < (DWORD)ms) {

		Sleep(CUT_SLICE_MS);
		if (_kbhit()) {
			int key = _getch();
			if (key == 0 || key == 224) {   // 방향키는 두번 읽어야 함
				_getch();
			}
			return 1;
		}
	}
	return 0;
}

/////  컷 그림 //////

// 큰 사람
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

// 중간 사람
static const char* MID_MAN[] = {
	"  ___  ",
	" (o o) ",
	"  (_)  ",
	"<--+-->",
	"   |   ",
	"  | |  ",
	" _| |_ ",
};

// 작은 사람 (게임 캐릭터)
static const char* SMALL_MAN[] = {
	"(o)",
	"<|>",
	"| |",
};

// 쓰러진 모습
static const char* SMALL_MAN_DOWN[] = {
	"(x)=<|",
};

// 약병
static const char* BOTTLE[] = {
	"  _____  ",
	" |_____| ",
	"  |   |  ",
	" (     ) ",
	"|  ???  |",
	"|       |",
	"|_______|",
};

// 마시는 약병
static const char* BOTTLE_DRINK[] = {
	" ___   ",
	"|___|==",
	"|???|  ",
	"|___|  ",
};

#define CUT_FRAME_TOP    1
#define CUT_TALK_BOTTOM  (SCREEN_H - 2)
#define CUT_TALK_TOP     (CUT_TALK_BOTTOM - 5)
#define CUT_FRAME_BOTTOM (CUT_TALK_TOP - 1)
#define CUT_FLOOR_Y      (CUT_FRAME_BOTTOM - 2) // 바닥 줄
#define CUT_TALK_LINE1   (CUT_TALK_TOP + 2)     // 대사 줄
#define CUT_TALK_LINE2   (CUT_TALK_TOP + 3)
#define CUT_TALK_LINE3   (CUT_TALK_TOP + 4)

// 버퍼에 글자 쓰기
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

// 가운데 정렬
static void PutCenter(char buf[][SCREEN_W + 1], int y, const char* s) {
	PutText(buf, (SCREEN_W - (int)strlen(s)) / 2, y, s);
}

// 여러 줄 그림 그리기 (bottomY가 맨 아랫줄)
static void PutArt(char buf[][SCREEN_W + 1], int x, int bottomY, const char* art[], int lines) {
	for (int i = 0; i < lines; i++) {
		PutText(buf, x, bottomY - (lines - 1) + i, art[i]);
	}
}

// 사각형 테두리
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

// 크기(k)에 맞는 사람 그리기
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

// 1~4 인트로, 5~8 엔딩, 9 게임오버
#define CUT_ENDING_FIRST 5
#define CUT_ENDING_LAST  8
#define CUT_GAMEOVER     9

// 컷 하나를 버퍼에 그림
static void BuildCut(char buf[][SCREEN_W + 1], int cut, int k) {
	char title[40];
	const int manX = 38;

	ClearBuffer(buf);

	// 맨 윗줄
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

	// 테두리랑 바닥
	PutBox(buf, 0, CUT_FRAME_TOP, SCREEN_W - 1, CUT_FRAME_BOTTOM);
	for (int x = 1; x < SCREEN_W - 1; x++) {
		buf[CUT_FLOOR_Y][x] = '_';
	}

	// 대사 칸
	PutBox(buf, 0, CUT_TALK_TOP, SCREEN_W - 1, CUT_TALK_BOTTOM);

	switch (cut) {

	// 인트로

	case 1: // 약병 발견
		PutMan(buf, manX, 3);
		PutArt(buf, manX + 30, CUT_FLOOR_Y - 1, BOTTLE, 7);
		PutText(buf, manX + 14, CUT_FLOOR_Y - 12, "?");
		PutCenter(buf, CUT_TALK_LINE1, "이 약은 뭐지?");
		break;

	case 2: // 마심
		PutMan(buf, manX, 3);
		PutArt(buf, manX + 11, CUT_FLOOR_Y - 10, BOTTLE_DRINK, 4);
		PutCenter(buf, CUT_TALK_LINE1, "한번 마셔볼까?");
		break;

	case 3: // 작아지는 중
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

	case 4: // 작아짐, 책상 위에 약병
		PutMan(buf, manX, 1);
		for (int x = 80; x <= 114; x++) {
			buf[CUT_FLOOR_Y - 12][x] = '=';
		}
		for (int y = CUT_FLOOR_Y - 11; y < CUT_FLOOR_Y; y++) {
			buf[y][82] = '#';
			buf[y][112] = '#';
		}
		PutText(buf, 106, CUT_FLOOR_Y - 13, "*");
		PutText(buf, 96, CUT_FLOOR_Y - 15, "저기 약병이...!");
		PutCenter(buf, CUT_TALK_LINE1, "너무 작아졌잖아!");
		PutCenter(buf, CUT_TALK_LINE2, "원래대로 돌아갈 방법을 찾아야 해...");
		break;

	// 엔딩

	case 5: // 약병 앞
		PutMan(buf, manX, 1);
		PutArt(buf, manX + 12, CUT_FLOOR_Y - 1, BOTTLE, 7);
		PutText(buf, manX + 6, CUT_FLOOR_Y - 5, "?");
		PutCenter(buf, CUT_TALK_LINE1, "이 약을 마시면 돌아갈 수 있을까?");
		break;

	case 6: // 커지는 중
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

	case 7:
		PutMan(buf, manX, 3);
		PutText(buf, manX - 6, CUT_FLOOR_Y - 10, "* *");
		PutText(buf, manX + 14, CUT_FLOOR_Y - 10, "* *");
		PutCenter(buf, CUT_TALK_LINE1, "거의 원래대로 돌아왔어!");
		break;

	case 8: // 클리어
		PutMan(buf, manX, 3);
		PutText(buf, manX - 8, CUT_FLOOR_Y - 14, "*  .  *");
		PutText(buf, manX + 13, CUT_FLOOR_Y - 14, "*  .  *");
		PutCenter(buf, CUT_FLOOR_Y - 18, "G A M E   C L E A R");
		PutCenter(buf, CUT_TALK_LINE1, "드디어 원래대로 돌아왔다!");
		break;

	case CUT_GAMEOVER:
		PutArt(buf, manX + 2, CUT_FLOOR_Y - 1, SMALL_MAN_DOWN, 1);
		PutCenter(buf, CUT_FLOOR_Y - 14, "G A M E   O V E R");
		PutCenter(buf, CUT_TALK_LINE1, "으앗... 작은 몸으로는 버티지 못했어...");
		break;

	default:
		break;
	}
}

/////  컷 화면 색 //////

// 컷 화면 색칠 (벽지, 나무 테두리, 대사 칸)
static void ThemeCut(char buf[][SCREEN_W + 1], WORD attr[][SCREEN_W], int frameTop, int frameBottom, int floorY,
	int talkTop, int talkBottom, WORD belowFloor) {

	for (int y = 0; y < SCREEN_H; y++) {
		for (int x = 0; x < SCREEN_W; x++) {
			char c = buf[y][x];
			WORD a = ATTR(PAL_TEXT, PAL_WALL);
			int border = ((y == frameTop || y == frameBottom) && x >= 0 && x < SCREEN_W) ||
				((x == 0 || x == SCREEN_W - 1) && y >= frameTop && y <= frameBottom);
			int talk = (talkTop >= 0 && y >= talkTop && y <= talkBottom);
			int talkBorder = talk && (y == talkTop || y == talkBottom || x == 0 || x == SCREEN_W - 1);

			if (border || talkBorder) {                                   // 테두리
				buf[y][x] = ' ';
				a = ATTR(PAL_WOOD_D, PAL_WOOD);
			}
			else if (talk) {                                               // 대사 칸
				a = ATTR(PAL_TEXT, PAL_CREAM);
			}
			else if (y == floorY) {                                       // 바닥
				buf[y][x] = (x % 7 == 0) ? '|' : '_';
				a = ATTR(PAL_WOOD_D, PAL_WOOD);
			}
			else if (y > floorY && y < frameBottom) {
				a = (WORD)(PAL_TEXT | (belowFloor << 4));
			}
			else if (y > frameTop && y < floorY) {                        // 방 안
				if (c == '#') { buf[y][x] = ' '; a = ATTR(PAL_WOOD_D, PAL_WOOD); }
				else if (c == '=') { buf[y][x] = '_'; a = ATTR(PAL_WOOD_D, PAL_WOOD); }
				else if (c == '~' || c == '*' || c == '^') a = ATTR(PAL_BLUE, PAL_WALL);
				else if (c == '!' || c == '?') a = ATTR(PAL_RED, PAL_WALL);
				else if (c == ' ') {                                                    // 벽지 무늬
					if (y % 4 == 1 && x % 8 == 2) buf[y][x] = '.';
					else if (y % 4 == 3 && x % 8 == 6) buf[y][x] = '\'';
					if (buf[y][x] != ' ') a = ATTR(PAL_PATTERN, PAL_WALL);
				}
			}
			attr[y][x] = a;
		}
	}
}

// 출력한 화면에 색 입히기 (printf로는 색을 못 줘서 따로 칠함)
static void PaintCut(WORD attr[][SCREEN_W]) {
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	DWORD written;
	int left = 0, top = 0;

	if (GetConsoleScreenBufferInfo(h, &info)) {
		left = (info.dwSize.X > SCREEN_W) ? (info.dwSize.X - SCREEN_W) / 2 : 0;
		top = (info.dwSize.Y > SCREEN_H) ? (info.dwSize.Y - SCREEN_H) / 2 : 0;
	}
	for (int y = 0; y < SCREEN_H; y++) {
		COORD pos = { (SHORT)left, (SHORT)(top + y) };
		WriteConsoleOutputAttribute(h, attr[y], SCREEN_W, pos, &written);
	}
}

static void FlushCut(char buf[][SCREEN_W + 1]) {
	static WORD attr[SCREEN_H][SCREEN_W];

	ThemeCut(buf, attr, CUT_FRAME_TOP, CUT_FRAME_BOTTOM, CUT_FLOOR_Y, CUT_TALK_TOP, CUT_TALK_BOTTOM, PAL_WOOD_D);
	FlushBuffer(buf);
	PaintCut(attr);
}

void DrawCut(int cut, int k) {
	char buf[SCREEN_H][SCREEN_W + 1];

	BuildCut(buf, cut, k);
	FlushCut(buf);
}

/////  타이틀 화면 //////

// 제목 글자. @는 파랑(작아진), %는 빨강(나)
#define TITLE_SMALL_W  9
#define TITLE_SMALL_H  7
#define TITLE_BIG_W    16
#define TITLE_BIG_H    11

static const char* TITLE_JAK[TITLE_SMALL_H] = { "@@@@@  @ ", "  @    @ ", " @ @   @@", "@   @  @ ", "         ", "@@@@@@@@ ", "       @ " };
static const char* TITLE_AH[TITLE_SMALL_H]  = { "       @ ", " @@@   @ ", "@   @  @@", "@   @  @ ", " @@@   @ ", "       @ ", "       @ " };
static const char* TITLE_JIN[TITLE_SMALL_H] = { "@@@@@  @ ", "  @    @ ", " @ @   @ ", "@   @  @ ", "         ", "@        ", "@@@@@@@@ " };
static const char* TITLE_NA[TITLE_BIG_H] = {
	"           %%   ",
	"%%         %%   ",
	"%%         %%   ",
	"%%         %%%%%",
	"%%         %%%%%",
	"%%         %%   ",
	"%%         %%   ",
	"%%%%%%%%%  %%   ",
	"%%%%%%%%%  %%   ",
	"           %%   ",
	"           %%   ",
};

#define TITLE_Y        2
#define TITLE_BOTTOM   (TITLE_Y + TITLE_BIG_H - 1)
#define TITLE_X        ((SCREEN_W - (TITLE_SMALL_W * 3 + 2 * 2 + 5 + TITLE_BIG_W)) / 2)
#define TITLE_NA_X     (TITLE_X + TITLE_SMALL_W * 3 + 2 * 2 + 5)
#define TITLE_SUB_Y    (TITLE_BOTTOM + 2)
#define TITLE_FLOOR_Y  28
#define TITLE_DESK_Y   (TITLE_FLOOR_Y - 7)
#define TITLE_PRESS_Y  31
#define TITLE_KEYS_Y   33

// 타이틀 그리기. showPress가 0이면 PRESS ANY KEY 안 보임 (깜빡임용)
static void BuildTitle(char buf[][SCREEN_W + 1], int showPress) {
	ClearBuffer(buf);

	// 제목
	PutArt(buf, TITLE_X, TITLE_BOTTOM, TITLE_JAK, TITLE_SMALL_H);
	PutArt(buf, TITLE_X + TITLE_SMALL_W + 2, TITLE_BOTTOM, TITLE_AH, TITLE_SMALL_H);
	PutArt(buf, TITLE_X + (TITLE_SMALL_W + 2) * 2, TITLE_BOTTOM, TITLE_JIN, TITLE_SMALL_H);
	PutArt(buf, TITLE_NA_X, TITLE_BOTTOM, TITLE_NA, TITLE_BIG_H);
	PutCenter(buf, TITLE_SUB_Y, "~ 약을 마시고 작아진 나의 방 탈출 ~");

	// 책상이랑 약병
	for (int x = 1; x < SCREEN_W - 1; x++) {
		buf[TITLE_FLOOR_Y][x] = '_';
	}
	for (int x = 66; x <= 113; x++) {
		buf[TITLE_DESK_Y][x] = '=';
	}
	for (int y = TITLE_DESK_Y + 1; y < TITLE_FLOOR_Y; y++) {
		buf[y][68] = '#';
		buf[y][111] = '#';
	}
	PutArt(buf, 92, TITLE_DESK_Y - 1, BOTTLE, 7);
	PutText(buf, 89, TITLE_DESK_Y - 8, "*");
	PutText(buf, 103, TITLE_DESK_Y - 6, "*");
	PutText(buf, 88, TITLE_DESK_Y - 3, ".");
	PutArt(buf, 30, TITLE_FLOOR_Y - 1, SMALL_MAN, 3);
	PutText(buf, 34, TITLE_FLOOR_Y - 5, "!");
	PutText(buf, 37, TITLE_FLOOR_Y - 4, "저 약을 마시면...?");

	PutBox(buf, 0, 1, SCREEN_W - 1, SCREEN_H - 2);
	PutText(buf, 2, 0, "IT 프로젝트  |  방탈출 게임");

	if (showPress) {
		PutCenter(buf, TITLE_PRESS_Y, ">>   PRESS ANY KEY TO START   <<");
	}
	PutCenter(buf, TITLE_KEYS_Y, "<- ->  이동        SPACE  점프 (2단 점프)        Q  종료");
}

// 타이틀 화면. 아무 키나 누르면 시작
void ShowTitle(void) {
	char buf[SCREEN_H][SCREEN_W + 1];
	static WORD attr[SCREEN_H][SCREEN_W];

	while (_kbhit()) {   // 미리 눌린 키 버리기
		_getch();
	}

	for (int blink = 1; ; blink = !blink) {
		BuildTitle(buf, blink);

		ThemeCut(buf, attr, 1, SCREEN_H - 2, TITLE_FLOOR_Y, -1, -1, PAL_CREAM);

		// 제목 색
		for (int y = TITLE_Y; y <= TITLE_BOTTOM; y++) {
			for (int x = 1; x < SCREEN_W - 1; x++) {
				if (buf[y][x] == '@') { buf[y][x] = ' '; attr[y][x] = ATTR(PAL_TEXT, PAL_BLUE); }
				else if (buf[y][x] == '%') { buf[y][x] = ' '; attr[y][x] = ATTR(PAL_TEXT, PAL_RED); }
			}
		}
		for (int x = 1; x < SCREEN_W - 1; x++) {
			if (buf[TITLE_SUB_Y][x] != ' ' && buf[TITLE_SUB_Y][x] != '.' && buf[TITLE_SUB_Y][x] != '\'') attr[TITLE_SUB_Y][x] = ATTR(PAL_TEXT, PAL_WALL);
			attr[TITLE_PRESS_Y][x] = ATTR(PAL_RED, PAL_CREAM);
		}
		for (int y = TITLE_DESK_Y - 8; y < TITLE_DESK_Y; y++) {
			for (int x = 85; x <= 106; x++) {
				if (buf[y][x] != ' ' && buf[y][x] != '.' && buf[y][x] != '\'') {
					attr[y][x] = ATTR(PAL_BLUE, PAL_WALL); // 약병 색
				}
			}
		}
		FlushBuffer(buf);
		PaintCut(attr);

		if (WaitCut(500) == 1) {
			break;
		}
	}
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

// 엔딩 / 게임오버 화면. R이면 1, Q면 0
int ShowEnding(Game* g) {
	char buf[SCREEN_H][SCREEN_W + 1];
	char line[80];
	int key;
	int skipped = 0;

	if (g->state == ST_EXIT) {
		return 0;
	}

	// 게임 중에 눌린 키 버리기
	while (_kbhit()) {
		_getch();
	}

	if (g->state == ST_CLEAR) {
		DrawCut(5, 0);
		skipped = WaitCut(2000);

		// 점점 커짐
		for (int k = 1; k <= 3 && !skipped; k++) {
			DrawCut(6, k);
			skipped = WaitCut(700);
		}

		if (!skipped) {
			DrawCut(7, 0);
			skipped = WaitCut(2000);
		}

		// 마지막 컷은 건너뛰어도 보여줌
		BuildCut(buf, 8, 0);
		snprintf(line, sizeof(line), "걸린 시간 : %lu 초", g->frame * FRAME_MS / 1000);
		PutCenter(buf, CUT_TALK_LINE2, line);
	}
	else {
		BuildCut(buf, CUT_GAMEOVER, 0);
		if (g->frame > TIME_LIMIT_TICK) {    // 시간 초과
			snprintf(line, sizeof(line), "시간 초과! %d 초 안에 약병에 가지 못했어...", TIME_LIMIT_SEC);
		}
		else {
			snprintf(line, sizeof(line), "버틴 시간 : %lu 초", g->frame * FRAME_MS / 1000);
		}
		PutCenter(buf, CUT_TALK_LINE2, line);
	}

	PutCenter(buf, CUT_TALK_LINE3, "다시 하기? ( R : 다시 / Q : 끝내기 )");
	FlushCut(buf);

	// R이나 Q 누를 때까지 기다림
	while (1) {
		key = _getch();
		if (key == 0 || key == 224) {
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

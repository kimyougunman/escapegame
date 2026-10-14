#include"game.h"


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

		if (tempX > g->ants[i].right || tempX < g->ants[i].left || IsSolid(&g->map, tempX, g->ants[i].y) == 1) {
			g->ants[i].dir = -g->ants[i].dir;
		}
		else {
			g->ants[i].x = tempX;
		}
	}
}

void UpdateButtons(Game* g) {
	int i;
	Button button;
	Fan fan;

	for (i = 0;i < MAX_BUTTONS;i++) {
		button = g->buttons[i];
		fan = g->fans[i];

		if (g->p.x == button.x && g->p.y == button.y) { // B 는 통과 칸이라 발(p.y)이 버튼 칸에 들어간다
			if (button.pressed == 0) {
				if (fan.on == 0) { // 선풍기 꺼져있을때 밟았을 경우
					fan.on = 1;
				}
				else { // 선풍기 켜져있을때 밟았을 경우
					fan.on = 0;
				}
				button.pressed = 1;
			}
		}
		else {
			button.pressed = 0;
		}

		g->buttons[i] = button;
		g->fans[i] = fan;
	}
}

void UpdateFans(Game* g) {
	Fan fan;
	int i, y;
	memset(g->wind, 0, sizeof(g->wind)); //바람 칸 전부 0

	for (i = 0;i < MAX_FANS;i++) {
		fan = g->fans[i];
		if (fan.on == 0) {
			continue;
		}
		else {
			for (y = 1;y <= fan.range;y++) { // 바람이 도달하는 거리

				if (fan.x >= MAP_W || fan.x < 0 || fan.y - y < 0) {	 // 바람이 맵 안인지 확인
					break;
				}
				else {
					if (IsSolid(&g->map, fan.x, fan.y - y) == 1) { // 바람 생성칸 막혀있으면 break
						break;
					}
					else { // 안막혔으면 바람 저장
						g->wind[fan.y - y][fan.x] = 5;
					}
				}
			}

		}
	}
}

void ApplyFanWind(Game* g) {

	if (g->wind[g->p.y][g->p.x] == 5) {
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
			g->drops[i].x = rand() % (MAP_W - 2) + 1;// 맵 x좌표에 랜덤으로 생성
			g->drops[i].y = 1;// y좌표는 천장 아래로 고정
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
				if (IsSolid(&g->map, g->drops[i].x, g->drops[i].y + 1) != 1) { // 낙하물 이동 가능이면 
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
int WaitCut(int ms) {
	int i;
	for (i = 1;i <= ms / CUT_SLICE_MS;i++) {

		Sleep(10);
		if (_kbhit() == 1) {
			return 1;
		}
	}
	return 0;
}
/////  컷만화용 그림 (SCREEN_W x SCREEN_H = 120 x 30 화면 기준) //////

// 큰 사람 (k = 3, 약을 마시기 전) : 가로 11칸 x 세로 13줄
static const char* BIG_MAN[] = {
	"   _____   ",
	"  /     \\  ",
	" |  o o  | ",
	" |   >   | ",
	"  \\ --- /  ",
	"   \\___/   ",
	"     |     ",
	" ----+---- ",
	"/    |    \\",
	"     |     ",
	"    / \\    ",
	"   /   \\   ",
	"  /     \\  ",
};

// 중간 사람 (k = 2, 작아지는 중) : 가로 7칸 x 세로 7줄
static const char* MID_MAN[] = {
	"  ___  ",
	" (o o) ",
	"  \\_/  ",
	" --+-- ",
	"   |   ",
	"  / \\  ",
	" /   \\ ",
};

// 작은 사람 (k = 1, 게임 속 모습과 같음) : 머리 '0' + 몸 '^'
static const char* SMALL_MAN[] = {
	"0",
	"^",
};

// 바닥에 놓인 약병 : 가로 9칸 x 세로 7줄
static const char* BOTTLE[] = {
	"  _____  ",
	" |_____| ",
	"  |   |  ",
	" /     \\ ",
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

#define CUT_FRAME_TOP    1   // 그림 테두리 윗줄
#define CUT_FRAME_BOTTOM 22  // 그림 테두리 아랫줄
#define CUT_FLOOR_Y      20  // 방바닥 줄. 사람과 약병은 이 줄 바로 위에 선다
#define CUT_TALK_TOP     23  // 대사 상자 윗줄
#define CUT_TALK_BOTTOM  28  // 대사 상자 아랫줄

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
		PutArt(buf, x + 5, CUT_FLOOR_Y - 1, SMALL_MAN, 2);
	}
}

// 컷만화 한 컷(테두리 + 그림 + 대사)을 120 x 30 화면 전체에 그린다
// k 는 사람 크기(1~3). 0 이면 컷마다 정해진 크기를 쓴다
void DrawCut(int cut, int k) {
	char buf[SCREEN_H][SCREEN_W + 1]; // 화면 버퍼 (Render 와 같은 방식)
	char title[40];
	const int manX = 38;              // 사람이 서는 x 위치

	ClearBuffer(buf);

	// 맨 윗줄 : 몇 번째 컷인지 + 건너뛰기 안내
	snprintf(title, sizeof(title), "[ 인트로 %d / 4 ]", cut);
	PutText(buf, 2, 0, title);
	PutText(buf, SCREEN_W - 32, 0, "아무 키나 누르면 건너뜁니다");

	// 그림 테두리와 방바닥
	PutBox(buf, 0, CUT_FRAME_TOP, SCREEN_W - 1, CUT_FRAME_BOTTOM);
	for (int x = 1; x < SCREEN_W - 1; x++) {
		buf[CUT_FLOOR_Y][x] = '_';
	}

	// 대사 상자
	PutBox(buf, 0, CUT_TALK_TOP, SCREEN_W - 1, CUT_TALK_BOTTOM);

	switch (cut) {

	case 1: // 큰 사람이 바닥의 약병을 발견
		PutMan(buf, manX, 3);
		PutArt(buf, manX + 30, CUT_FLOOR_Y - 1, BOTTLE, 7);
		PutText(buf, manX + 14, 8, "?");
		PutCenter(buf, 25, "이 약은 뭐지?");
		break;

	case 2: // 약병을 입에 대고 마심
		PutMan(buf, manX, 3);
		PutArt(buf, manX + 11, 10, BOTTLE_DRINK, 4);
		PutCenter(buf, 25, "한번 마셔볼까?");
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
		PutCenter(buf, 25, "어...? 몸이 작아지고 있어!");
		break;

	case 4: // 아주 작아짐. 멀리 책상 위에 약병(목표)이 보인다
		PutMan(buf, manX, 1);
		for (int x = 80; x <= 114; x++) {
			buf[8][x] = '=';                 // 책상 판
		}
		for (int y = 9; y < CUT_FLOOR_Y; y++) {
			buf[y][82] = '#';                // 책상 다리
			buf[y][112] = '#';
		}
		PutText(buf, 106, 7, "*");           // 책상 위 약병
		PutText(buf, 96, 5, "저기 약병이...!");
		PutCenter(buf, 25, "너무 작아졌잖아!");
		PutCenter(buf, 26, "원래대로 돌아갈 방법을 찾아야 해...");
		break;

	default:
		break;
	}

	FlushBuffer(buf); // 완성된 컷을 한 번에 출력
}
void ShowIntro() {
	int cut;
	int k =0;

	for (cut = 1;cut <= 4;cut++) {
		DrawCut(cut, k);
		if (WaitCut(2000) == 1) {
			break;
		}
	}
}


int ShowEnding(Game* g) {      // 임시 
	return 0;
}
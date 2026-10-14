#include "game.h"


void CheckGoal(Game* g){
	
	if (g->p.x == g->map.goalX &&g->p.y == g->map.goalY && g->p.onGround) g->holdTick+=1;
	else g->holdTick = 0;
	if (g->holdTick>= DRINK_TICK){
		g->state = ST_CLEAR;
	}
}

// 120 x 30 화면이 모니터(작업 표시줄 제외)를 꽉 채우도록 콘솔 글꼴 크기를 자동으로 키운다
// 칸 수(120 x 30)는 그대로 두고 글자만 커진다. Windows Terminal 처럼 글꼴을 바꿀 수 없는 콘솔에서는 아무 일도 하지 않는다
static void FitConsoleFont(HANDLE h) {
	CONSOLE_FONT_INFOEX font;
	RECT work;                 // 작업 표시줄을 뺀 화면 영역
	int frameW, frameH;        // 창 테두리·제목 표시줄 두께
	int fontH;

	font.cbSize = sizeof(font);
	if (!GetCurrentConsoleFontEx(h, FALSE, &font)) {
		return;
	}
	if (!(font.FontFamily & TMPF_TRUETYPE)) {
		lstrcpynW(font.FaceName, L"굴림체", LF_FACESIZE); // 래스터 글꼴은 크기를 못 바꾸므로 한글 고정폭 글꼴로 바꾼다
	}
	font.FontWeight = FW_NORMAL;

	SystemParametersInfo(SPI_GETWORKAREA, 0, &work, 0);
	frameW = 2 * (GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER)) + GetSystemMetrics(SM_CXVSCROLL);
	frameH = 2 * (GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER)) + GetSystemMetrics(SM_CYCAPTION);

	// 화면 높이로 계산한 가장 큰 글꼴부터 한 단계씩 줄여 가며, 120 x 30 이 다 들어가는 크기를 찾는다
	for (fontH = (work.bottom - work.top) / SCREEN_H; fontH >= 8; fontH--) {
		font.dwFontSize.X = 0;            // 0 이면 높이에 맞는 너비를 자동으로 고른다
		font.dwFontSize.Y = (SHORT)fontH;
		if (!SetCurrentConsoleFontEx(h, FALSE, &font)) {
			return;
		}
		GetCurrentConsoleFontEx(h, FALSE, &font); // 실제로 적용된 글자 한 칸의 크기(픽셀)를 다시 읽는다

		COORD largest = GetLargestConsoleWindowSize(h);
		if (font.dwFontSize.X * SCREEN_W + frameW <= work.right - work.left &&
			font.dwFontSize.Y * SCREEN_H + frameH <= work.bottom - work.top &&
			largest.X >= SCREEN_W && largest.Y >= SCREEN_H) {
			return;
		}
	}
}

// 콘솔 창을 화면 가운데로 옮긴다
// 콘솔은 창 크기를 조금 늦게 반영하므로, 지금 창 크기를 읽지 않고 글꼴 크기 x 120 x 30 으로 창 크기를 계산한다
static void CenterConsoleWindow(HANDLE h) {
	HWND hwnd = GetConsoleWindow();
	CONSOLE_FONT_INFOEX font;
	RECT r, work;

	if (hwnd == NULL) {
		return;
	}
	font.cbSize = sizeof(font);
	if (!GetCurrentConsoleFontEx(h, FALSE, &font)) {
		return;
	}
	r.left = 0;
	r.top = 0;
	r.right = font.dwFontSize.X * SCREEN_W;   // 글자 영역 크기
	r.bottom = font.dwFontSize.Y * SCREEN_H;
	AdjustWindowRectEx(&r, (DWORD)GetWindowLong(hwnd, GWL_STYLE), FALSE, (DWORD)GetWindowLong(hwnd, GWL_EXSTYLE)); // 테두리·제목 표시줄 포함 크기
	SystemParametersInfo(SPI_GETWORKAREA, 0, &work, 0);

	int x = work.left + ((work.right - work.left) - (r.right - r.left)) / 2;
	int y = work.top + ((work.bottom - work.top) - (r.bottom - r.top)) / 2;
	SetWindowPos(hwnd, NULL, x > work.left ? x : work.left, y > work.top ? y : work.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

int InitGame(Game* g){

	memset(g, 0, sizeof(Game));
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);   // 콘솔 출력 손잡이
	FitConsoleFont(h);                            // 0 글꼴을 화면에 꽉 차는 크기로 (창 크기를 정하기 전에 해야 한다)
	SMALL_RECT win = { 0, 0, 1, 1 };
	SetConsoleWindowInfo(h, TRUE, &win);          //1 창을 먼저 아주 작게

	COORD size = { SCREEN_W, SCREEN_H };
	SetConsoleScreenBufferSize(h, size);          // 2 버퍼를 120x30으로

	win.Right = SCREEN_W - 1;
	win.Bottom = SCREEN_H - 1;
	SetConsoleWindowInfo(h, TRUE, &win);  // 3 창을 버퍼 크기에 맞춤
	CenterConsoleWindow(h);               // 4 커진 창을 화면 가운데로
	HideCursor(); // 깜빡이는 커서 숨기기 (CloseGame 에서 다시 보이게 함)
	int stage = 1;
	int flag;
	
	flag = LoadMap(&g->map,stage);
	srand((unsigned int)time(NULL));
	if (flag){
		// 플레이어 위치 초기화 
		g->p.y = 27;
		g->p.x = 3;
		// hp 값 초기화
		g->p.hp = PLAYER_MAX_HP;
		g->p.face = 1;
		g->state = ST_PLAY;

		// 개미 , 낙하물 , 선풍기 초기화 
		int Count_FANS =0;
		int Count_BUTTONS =0;
		for(int y =28; y >= 0; y--){
			for(int x =0 ; x <MAP_W; x++){
				if(g->map.tile[y][x]=='*'){
					g->map.goalX = x;
					g->map.goalY = y;
				}
				if(g->map.tile[y][x] == 'F' && Count_FANS  <MAX_FANS){
					Fan * f = &g->fans[Count_FANS];
					f->x =  x;
					f->y = y;
					f->on =0;
					f->power = FAN_POWER;
					f->range = 12;
					Count_FANS++;
				}
				else if(g->map.tile[y][x] == 'B' && Count_BUTTONS  < MAX_BUTTONS){
					Button *b = &g->buttons[Count_BUTTONS];
					b->x = x;
					b->y = y;
					b->fanId = Count_BUTTONS; 
					b->pressed = 0;
					Count_BUTTONS++;
				}
			}
		}
		// 개미 초기화 :
		int antInfo[][4] = { //  개미 한 마리 { x, y, left, right }
			{ 35, 27, 35,  60 },   // 1층 
			{ 30, 16, 30,  65 },   // 2층 
			{ 82,  6, 82, 100 },   // 제일 위층 
		};
		int antCount = sizeof(antInfo) / sizeof(antInfo[0]) ;

		for (int i = 0;   i< antCount  && (i <MAX_ANTS); i++) {
			Ant* a = &g->ants[i];

			a->x = antInfo[i][1-1];
			a->y = antInfo[i][2-1];
			a->left = antInfo[i][3-1];
			a->right = antInfo[i][4-1];


			a->dir = 1;
			a->moveTick = ANT_TERM;
			a->alive = 1;
		}
	}
	if (flag == 0) {
		printf("map.txt 를 열 수 없습니다.\n");
		_getch();
		return 0;
	}
	return 1;
}

void UpdateGame(Game* g){
	if (g->p.invTick > 0){
		g->p.invTick -=1;
	}
	UpdateButtons(g);
	UpdateFans(g);
	UpdatePlayer(g);
	UpdateDrops(g);
	UpdateAnts(g);
	CheckCollisions(g);

	if (g->p.hp <=0){
		g->state = ST_GAMEOVER;
	}
	else{
		CheckGoal(g);
	}
}

void RunGameLoop(Game *g){
	while((g->state) == ST_PLAY){
		DWORD start = GetTickCount(); // 프레임 재기		
		ProcessInput(g);
		if((g->state) == ST_EXIT){
			break;	
		}
		UpdateGame(g);
		Render(g);
		g->frame++;
		int elapsed = GetTickCount()-start; // 다시 프레임 재서 몇 프레임 걸렷는지 측정
		if( (FRAME_MS -elapsed) > 0){ // 50보다 적게 걸리면 
			Sleep(FRAME_MS -elapsed); // 그 값을 얼림 (실행시간+얼린시간 = 50 프레임이 되야 하므로)
		}			
	}
	
}

void CloseGame(Game *g){

	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO cursor;  // 커서 정보 구조체
	cursor.dwSize = 20;          // 커서 두께(1~100)
	cursor.bVisible = TRUE;      // 숨겨 둔 커서를 다시 보이게
	SetConsoleCursorInfo(h, &cursor);

	system("cls");
	printf("게임 종료\n");
}	


int main() {
	
	int firstRun = 1; // 인트로 판별 용 
	Game g;
	while(1){
	
		if (!InitGame(&g)) { 
			CloseGame(&g); return 0;
		}
		if (firstRun){
			ShowIntro(); // 인트로 표시 
			firstRun = 0;
		}
		g.state = ST_PLAY;
		RunGameLoop(&g);

		if((g.state) == ST_EXIT){ // 종료 시 
			break;
		}
		if(ShowEnding(&g) == 0){
			break;
		}
		
	}
	// 게임 종료 
	CloseGame(&g);
	return 0; 
	 
}	


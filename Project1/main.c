#include "game.h"
#include "bgm.h"


void CheckGoal(Game* g){

	if (abs(g->p.x - g->map.goalX) <= PLAYER_HALF && g->p.y == g->map.goalY && g->p.onGround) g->holdTick+=1; // 약병 앞에 서있으면
	else g->holdTick = 0;
	if (g->holdTick>= DRINK_TICK){
		g->state = ST_CLEAR;
	}
}

// 창 테두리 없애기 (on = 1) / 되돌리기 (on = 0)
static void SetConsoleBorderless(int on) {
	HWND hwnd = GetConsoleWindow();
	LONG style;

	if (hwnd == NULL) {
		return;
	}
	style = GetWindowLong(hwnd, GWL_STYLE);
	if (on) {
		style &= ~WS_OVERLAPPEDWINDOW;
	}
	else {
		style |= WS_OVERLAPPEDWINDOW;
	}
	SetWindowLong(hwnd, GWL_STYLE, style);
	SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

// 화면에 120 x 36이 꽉 차게 글꼴 크기 맞추기
static void FitConsoleFont(HANDLE h) {
	CONSOLE_FONT_INFOEX font;
	COORD largest;
	int fontH;

	font.cbSize = sizeof(font);
	if (!GetCurrentConsoleFontEx(h, FALSE, &font)) {
		return;
	}
	if (!(font.FontFamily & TMPF_TRUETYPE)) {
		lstrcpynW(font.FaceName, L"굴림체", LF_FACESIZE);
	}
	font.FontWeight = FW_NORMAL;

	// 큰 글꼴부터 줄여가면서 화면에 다 들어가는 크기 찾기
	for (fontH = GetSystemMetrics(SM_CYSCREEN) / SCREEN_H; fontH >= 8; fontH--) {
		font.dwFontSize.X = 0;
		font.dwFontSize.Y = (SHORT)fontH;
		if (!SetCurrentConsoleFontEx(h, FALSE, &font)) {
			return;
		}

		largest = GetLargestConsoleWindowSize(h);
		if (largest.X >= SCREEN_W && largest.Y >= SCREEN_H) {
			return;
		}
	}
}

// 창을 화면 가운데로
static void PlaceConsoleWindow(HANDLE h) {
	HWND hwnd = GetConsoleWindow();
	CONSOLE_FONT_INFOEX font;
	CONSOLE_SCREEN_BUFFER_INFO info;
	RECT r, work;
	int x, y;

	if (hwnd == NULL) {
		return;
	}
	font.cbSize = sizeof(font);
	if (!GetCurrentConsoleFontEx(h, FALSE, &font) || !GetConsoleScreenBufferInfo(h, &info)) {
		return;
	}
	r.left = 0;
	r.top = 0;
	r.right = font.dwFontSize.X * (info.srWindow.Right - info.srWindow.Left + 1);
	r.bottom = font.dwFontSize.Y * (info.srWindow.Bottom - info.srWindow.Top + 1);
	AdjustWindowRectEx(&r, (DWORD)GetWindowLong(hwnd, GWL_STYLE), FALSE, (DWORD)GetWindowLong(hwnd, GWL_EXSTYLE));

	SystemParametersInfo(SPI_GETWORKAREA, 0, &work, 0);  // 작업표시줄 빼고
	x = work.left + ((work.right - work.left) - (r.right - r.left)) / 2;
	y = work.top + ((work.bottom - work.top) - (r.bottom - r.top)) / 2;
	SetWindowPos(hwnd, HWND_TOP, x > 0 ? x : 0, y > 0 ? y : 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
}

// 콘솔 16색을 방 느낌 색으로 바꿈 (번호는 game.h)
static void SetGamePalette(HANDLE h) {
	CONSOLE_SCREEN_BUFFER_INFOEX info;

	info.cbSize = sizeof(info);
	if (!GetConsoleScreenBufferInfoEx(h, &info)) {
		return;
	}
	info.ColorTable[PAL_WALL]    = RGB(238, 229, 211);  // 벽지
	info.ColorTable[PAL_DECOR]   = RGB(150, 132, 112);
	info.ColorTable[PAL_BOOK_G]  = RGB(166, 200, 166);  // 책
	info.ColorTable[PAL_GLASS]   = RGB(196, 222, 240);  // 유리, 하늘
	info.ColorTable[PAL_BOOK_R]  = RGB(226, 160, 150);
	info.ColorTable[PAL_BOOK_P]  = RGB(190, 174, 220);
	info.ColorTable[PAL_WOOD_D]  = RGB(150, 108, 70);   // 진한 나무
	info.ColorTable[PAL_TEXT]    = RGB(70, 62, 56);     // 글자
	info.ColorTable[PAL_ANT]     = RGB(40, 36, 36);     // 개미
	info.ColorTable[PAL_PATTERN] = RGB(222, 207, 182);  // 벽지 무늬
	info.ColorTable[PAL_HUD]     = RGB(128, 104, 82);
	info.ColorTable[PAL_BLUE]    = RGB(92, 150, 205);   // 바람
	info.ColorTable[PAL_RED]     = RGB(205, 70, 70);    // 버튼
	info.ColorTable[PAL_WOOD]    = RGB(214, 178, 136);  // 나무
	info.ColorTable[PAL_SUN]     = RGB(250, 214, 110);
	info.ColorTable[PAL_CREAM]   = RGB(252, 250, 244);
	info.srWindow.Right++;   // 창이 한칸씩 줄어드는 거 보정
	info.srWindow.Bottom++;
	SetConsoleScreenBufferInfoEx(h, &info);
}

// 전체화면 (성공하면 1)
static int EnterFullscreen(HANDLE h) {
	DWORD mode = 0;
	COORD newSize;

	if (GetConsoleDisplayMode(&mode) && (mode & CONSOLE_FULLSCREEN)) {
		return 1;
	}
	return SetConsoleDisplayMode(h, CONSOLE_FULLSCREEN_MODE, &newSize) ? 1 : 0;
}

int InitGame(Game* g){

	memset(g, 0, sizeof(Game));
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	SetGamePalette(h);
	int fullscreen = EnterFullscreen(h);
	if (!fullscreen) {
		SetConsoleBorderless(1);   // 전체화면 안되면 테두리 없는 창
	}
	FitConsoleFont(h);

	// 콘솔 크기 정하기 (최소 120 x 36)
	COORD largest = GetLargestConsoleWindowSize(h);
	SHORT cols = (largest.X > SCREEN_W) ? largest.X : SCREEN_W;
	SHORT rows = (largest.Y > SCREEN_H) ? largest.Y : SCREEN_H;

	SMALL_RECT win = { 0, 0, 1, 1 };
	SetConsoleWindowInfo(h, TRUE, &win);   // 창 먼저 작게

	COORD size = { cols, rows };
	SetConsoleScreenBufferSize(h, size);

	win.Right = cols - 1;
	win.Bottom = rows - 1;
	SetConsoleWindowInfo(h, TRUE, &win);
	system("cls");
	ShowScrollBar(GetConsoleWindow(), SB_BOTH, FALSE); // 스크롤바 숨김
	if (!fullscreen) {
		PlaceConsoleWindow(h);
	}
	HideCursor();
	int stage = 1;
	int flag;

	flag = LoadMap(&g->map,stage);
	srand((unsigned int)time(NULL));
	if (flag){
		// 플레이어
		g->p.y = MAP_H - 2;
		g->p.x = 3;
		g->p.hp = PLAYER_MAX_HP;
		g->p.face = 1;
		g->state = ST_PLAY;

		// 맵에서 약병, 선풍기, 버튼 찾기
		int Count_FANS =0;
		int Count_BUTTONS =0;
		for(int y = MAP_H - 1; y >= 0; y--){
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
		// 다 찾은 다음에 칸 막기 (중간에 하면 같은 걸 또 찾음)
		for (int i = 0; i < Count_BUTTONS; i++) {
			SetButtonTiles(g, i);
		}
		for (int i = 0; i < Count_FANS; i++) {
			SetFanTiles(g, i);
		}
		// 개미 { x, y, 왼쪽 끝, 오른쪽 끝 }
		int antInfo[][4] = {
			{ 35, 33, 35,  60 },   // 1층
			{ 30, 22, 30,  65 },   // 2층
			{ 82, 12, 82, 100 },   // 맨 위
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
	if (g->frame >= TIME_LIMIT_TICK) {
		g->p.hp = 0;    // 시간 초과하면 바로 죽음
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
		DWORD start = GetTickCount();
		ProcessInput(g);
		if((g->state) == ST_EXIT){
			break;
		}
		UpdateGame(g);
		Render(g);
		g->frame++;
		int elapsed = GetTickCount()-start; // 걸린 시간
		if( (FRAME_MS -elapsed) > 0){
			Sleep(FRAME_MS -elapsed); // 50ms 맞추기
		}
	}

}

void CloseGame(Game *g){

	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO cursor;
	cursor.dwSize = 20;
	cursor.bVisible = TRUE;      // 커서 다시 보이게
	SetConsoleCursorInfo(h, &cursor);
	COORD normalSize;
	SetConsoleDisplayMode(h, CONSOLE_WINDOWED_MODE, &normalSize); // 전체화면 끄기
	SetConsoleBorderless(0);

	system("cls");
	printf("게임 종료\n");
}


// Windows Terminal에서는 글꼴/창 크기를 못 바꿔서 기본 콘솔(conhost)로 다시 실행
// 다시 실행했으면 1
static int RelaunchInClassicConsole(int argc, char* argv[]) {
	wchar_t conhost[MAX_PATH];
	wchar_t exePath[MAX_PATH];
	wchar_t cmd[MAX_PATH * 2 + 24];
	wchar_t className[64] = L"";
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	HWND hwnd = GetConsoleWindow();

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--classic") == 0) {
			return 0;   // 이미 다시 실행된 거면 그냥 진행
		}
	}
	if (hwnd != NULL) {
		GetClassNameW(hwnd, className, 64);
	}
	if (lstrcmpW(className, L"ConsoleWindowClass") == 0) {
		return 0;   // 원래 기본 콘솔이면 그냥 진행
	}
	if (IsDebuggerPresent()) {
		return 0;   // VS 디버깅 중이면 그냥 진행
	}
	if (GetSystemDirectoryW(conhost, MAX_PATH) == 0 || GetModuleFileNameW(NULL, exePath, MAX_PATH) == 0) {
		return 0;
	}
	lstrcatW(conhost, L"\\conhost.exe");

	// "conhost.exe" "게임.exe" --classic
	lstrcpyW(cmd, L"\"");
	lstrcatW(cmd, conhost);
	lstrcatW(cmd, L"\" \"");
	lstrcatW(cmd, exePath);
	lstrcatW(cmd, L"\" --classic");

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	if (!CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
		return 0;
	}
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return 1;
}

int main(int argc, char* argv[]) {

	int firstRun = 1; // 처음 실행인지
	Game g;

	if (RelaunchInClassicConsole(argc, argv)) {
		return 0;
	}
	while(1){

		if (!InitGame(&g)) {
			CloseGame(&g); return 0;
		}
		if (firstRun){
			BgmStart(); // 배경음악
			ShowTitle();
			ShowIntro();
			firstRun = 0;
		}
		g.state = ST_PLAY;
		RunGameLoop(&g);

		if((g.state) == ST_EXIT){
			break;
		}
		if(ShowEnding(&g) == 0){ // Q 누르면 끝
			break;
		}

	}
	// 게임 종료
	BgmStop();
	CloseGame(&g);
	return 0;

}


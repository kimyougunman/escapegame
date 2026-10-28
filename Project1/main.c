#include "game.h"


void CheckGoal(Game* g){
	
	if (abs(g->p.x - g->map.goalX) <= PLAYER_HALF && g->p.y == g->map.goalY && g->p.onGround) g->holdTick+=1; // 약병이 발 3칸 안에 있고 서 있으면
	else g->holdTick = 0;
	if (g->holdTick>= DRINK_TICK){
		g->state = ST_CLEAR;
	}
}

// 콘솔 창의 제목 표시줄·테두리를 없애거나(on = 1) 원래대로 되돌린다(on = 0)
// 테두리 없는 전체 화면용. Windows Terminal 에서는 효과가 없다
static void SetConsoleBorderless(int on) {
	HWND hwnd = GetConsoleWindow();
	LONG style;

	if (hwnd == NULL) {
		return;
	}
	style = GetWindowLong(hwnd, GWL_STYLE);
	if (on) {
		style &= ~WS_OVERLAPPEDWINDOW;  // 제목 표시줄, 테두리, 최소화·최대화·닫기 버튼 제거
	}
	else {
		style |= WS_OVERLAPPEDWINDOW;   // 보통 창 모양으로 되돌리기
	}
	SetWindowLong(hwnd, GWL_STYLE, style);
	SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED); // 바뀐 모양 적용
}

// 120 x 36 화면이 모니터 전체를 최대한 채우도록 콘솔 글꼴 크기를 자동으로 키운다
// 칸 수(SCREEN_W x SCREEN_H)는 그대로 두고 글자만 커진다. Windows Terminal 처럼 글꼴을 바꿀 수 없는 콘솔에서는 아무 일도 하지 않는다
static void FitConsoleFont(HANDLE h) {
	CONSOLE_FONT_INFOEX font;
	COORD largest;             // 지금 글꼴로 콘솔 창이 가질 수 있는 최대 칸 수
	int fontH;

	font.cbSize = sizeof(font);
	if (!GetCurrentConsoleFontEx(h, FALSE, &font)) {
		return;
	}
	if (!(font.FontFamily & TMPF_TRUETYPE)) {
		lstrcpynW(font.FaceName, L"굴림체", LF_FACESIZE); // 래스터 글꼴은 크기를 못 바꾸므로 한글 고정폭 글꼴로 바꾼다
	}
	font.FontWeight = FW_NORMAL;

	// 화면 높이로 계산한 가장 큰 글꼴부터 한 단계씩 줄여 가며, 120 x 36 이 화면에 다 들어가는 크기를 찾는다
	// "다 들어가는가" 는 콘솔이 직접 알려 주는 최대 칸 수로 판단한다 (Windows 배율 125% 등도 콘솔이 알아서 반영)
	for (fontH = GetSystemMetrics(SM_CYSCREEN) / SCREEN_H; fontH >= 8; fontH--) {
		font.dwFontSize.X = 0;            // 0 이면 높이에 맞는 너비를 자동으로 고른다
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

// 콘솔 창을 화면 가운데에 놓는다
// 콘솔은 창 크기를 조금 늦게 반영하므로, 지금 창 크기를 읽지 않고 글꼴 크기로 계산한다
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
	r.right = font.dwFontSize.X * (info.srWindow.Right - info.srWindow.Left + 1);   // 글자 영역 크기 (가로 칸 x 세로 줄)
	r.bottom = font.dwFontSize.Y * (info.srWindow.Bottom - info.srWindow.Top + 1);
	AdjustWindowRectEx(&r, (DWORD)GetWindowLong(hwnd, GWL_STYLE), FALSE, (DWORD)GetWindowLong(hwnd, GWL_EXSTYLE)); // 남은 테두리가 있으면 포함

	SystemParametersInfo(SPI_GETWORKAREA, 0, &work, 0);  // 작업 표시줄을 뺀 영역 (작업 표시줄에 가리지 않게)
	x = work.left + ((work.right - work.left) - (r.right - r.left)) / 2;
	y = work.top + ((work.bottom - work.top) - (r.bottom - r.top)) / 2;
	SetWindowPos(hwnd, HWND_TOP, x > 0 ? x : 0, y > 0 ? y : 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW); // 크기는 콘솔이 정한 그대로, 위치만 옮긴다
}

// 콘솔 색 표의 두 자리를 게임용 색으로 바꾼다 (기본 16색에는 갈색이 없고, 검은 바탕에서 완전한 검정 글자는 보이지 않는다)
//  6번(진한 노랑) -> 갈색 : 돌      8번(진회색) -> 검정에 가까운 회색 : 개미      1번(진한 파랑) -> 어두운 청회색 : 배경 장식
static void SetGamePalette(HANDLE h) {
	CONSOLE_SCREEN_BUFFER_INFOEX info;

	info.cbSize = sizeof(info);
	if (!GetConsoleScreenBufferInfoEx(h, &info)) {
		return;   // Windows Terminal 등 색 표를 바꿀 수 없는 콘솔이면 그대로
	}
	info.ColorTable[6] = RGB(150, 90, 40);   // 갈색
	info.ColorTable[8] = RGB(90, 90, 90);    // 검정에 가까운 진회색
	info.ColorTable[1] = RGB(70, 80, 110);   // 1번(진한 파랑) -> 어두운 청회색 : 배경 장식
	info.srWindow.Right++;                   // SetConsoleScreenBufferInfoEx 는 창을 한 칸씩 줄이는 버릇이 있어 보정한다
	info.srWindow.Bottom++;
	SetConsoleScreenBufferInfoEx(h, &info);
}

// 콘솔을 Alt+Enter 와 같은 전체 화면으로 바꾼다. 이미 전체 화면이거나 바꾸는 데 성공하면 1
// Windows Terminal 처럼 전체 화면 전환을 지원하지 않는 콘솔이면 0 (그때는 테두리 없는 창으로 대신한다)
static int EnterFullscreen(HANDLE h) {
	DWORD mode = 0;
	COORD newSize;

	if (GetConsoleDisplayMode(&mode) && (mode & CONSOLE_FULLSCREEN)) {
		return 1;   // 이미 전체 화면 (R 로 다시 시작할 때)
	}
	return SetConsoleDisplayMode(h, CONSOLE_FULLSCREEN_MODE, &newSize) ? 1 : 0;
}

int InitGame(Game* g){

	memset(g, 0, sizeof(Game));
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);   // 콘솔 출력 손잡이
	SetGamePalette(h);                            //   돌(갈색)·개미(검정)·배경 장식 색 준비
	int fullscreen = EnterFullscreen(h);          // 0 Alt+Enter 전체 화면으로 시작 (모든 화면의 기준)
	if (!fullscreen) {
		SetConsoleBorderless(1);                  //   전체 화면을 못 쓰는 콘솔이면 테두리 없는 창으로 대신
	}
	FitConsoleFont(h);                            //   글꼴을 120 x 36 이 화면에 꽉 차는 크기로 (창 크기를 정하기 전에 해야 한다)

	// 콘솔 크기 = 화면에 들어가는 최대 칸 수 (최소 120 x 36). 게임 화면은 그 가운데에 그린다 (FlushBuffer)
	COORD largest = GetLargestConsoleWindowSize(h);
	SHORT cols = (largest.X > SCREEN_W) ? largest.X : SCREEN_W;
	SHORT rows = (largest.Y > SCREEN_H) ? largest.Y : SCREEN_H;

	SMALL_RECT win = { 0, 0, 1, 1 };
	SetConsoleWindowInfo(h, TRUE, &win);          //1 창을 먼저 아주 작게

	COORD size = { cols, rows };
	SetConsoleScreenBufferSize(h, size);          // 2 버퍼를 cols x rows 로 (스크롤 없이 화면과 같은 크기)

	win.Right = cols - 1;
	win.Bottom = rows - 1;
	SetConsoleWindowInfo(h, TRUE, &win);  // 3 창을 버퍼 크기에 맞춤
	system("cls");                        //   남는 줄을 깨끗하게 (이전 출력이 남지 않도록)
	if (!fullscreen) {
		PlaceConsoleWindow(h);            // 4 (테두리 없는 창일 때만) 화면 가운데로
	}
	HideCursor(); // 깜빡이는 커서 숨기기 (CloseGame 에서 다시 보이게 함)
	int stage = 1;
	int flag;
	
	flag = LoadMap(&g->map,stage);
	srand((unsigned int)time(NULL));
	if (flag){
		// 플레이어 위치 초기화 
		g->p.y = MAP_H - 2;   // 맨 아래 바닥(#) 바로 위
		g->p.x = 3;
		// hp 값 초기화
		g->p.hp = PLAYER_MAX_HP;
		g->p.face = 1;
		g->state = ST_PLAY;

		// 개미 , 낙하물 , 선풍기 초기화 
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
		for (int i = 0; i < Count_BUTTONS; i++) {
			SetButtonTiles(g, i);   // 찾은 버튼 자리 2 x 2 를 막힌 칸으로 (스캔이 끝난 뒤에 해야 같은 버튼을 또 찾지 않는다)
		}
		for (int i = 0; i < Count_FANS; i++) {
			SetFanTiles(g, i);      // 찾은 선풍기 자리 5 x 3 을 막힌 칸으로 (스캔이 끝난 뒤에)
		}
		// 개미 초기화 :
		int antInfo[][4] = { //  개미 한 마리 { x, y, left, right }
			{ 35, 33, 35,  60 },   // 1층 (바닥 y=34 위)
			{ 30, 22, 30,  65 },   // 2층 (큰 발판 y=23 위)
			{ 82, 12, 82, 100 },   // 제일 위층 (꼭대기 발판 y=13 위) 
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
	if (g->frame > 0 && g->frame % HP_DRAIN_TICK == 0) {
		g->p.hp -= 1;   // 20초가 지날 때마다 HP 1 감소 (무적과 상관없이). 0 이 되면 아래에서 게임 오버
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
	COORD normalSize;
	SetConsoleDisplayMode(h, CONSOLE_WINDOWED_MODE, &normalSize); // 전체 화면 끝내기
	SetConsoleBorderless(0);     // (테두리 없는 창이었다면) 제목 표시줄·테두리 되돌리기

	system("cls");
	printf("게임 종료\n");
}	


// Windows Terminal(탭이 있는 새 터미널) 안에서 실행되면 글꼴·창 크기를 바꿀 수 없어 화면을 꽉 채우지 못한다
// 그래서 클래식 콘솔(conhost)로 자기 자신을 다시 실행한다. 다시 실행했으면 1, 그대로 진행하면 0 을 반환한다
// argc, argv : main 의 명령줄. 다시 실행된 게임에는 "--classic" 을 붙여 또 다시 실행하지 않게 한다
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
			return 0;   // 이미 다시 실행된 게임이면 그대로 진행 (무한 반복 방지)
		}
	}
	if (hwnd != NULL) {
		GetClassNameW(hwnd, className, 64);
	}
	if (lstrcmpW(className, L"ConsoleWindowClass") == 0) {
		return 0;   // 클래식 콘솔 창이면 그대로 진행 (Windows Terminal 안에서는 보이지 않는 가짜 창이라 이름이 다르다)
	}
	if (IsDebuggerPresent()) {
		return 0;   // Visual Studio 디버깅(F5) 중에는 다시 실행하면 디버거가 끊기므로 그대로 진행
	}
	if (GetSystemDirectoryW(conhost, MAX_PATH) == 0 || GetModuleFileNameW(NULL, exePath, MAX_PATH) == 0) {
		return 0;
	}
	lstrcatW(conhost, L"\\conhost.exe");

	// 명령줄 : "C:\Windows\System32\conhost.exe" "게임.exe 경로" --classic
	lstrcpyW(cmd, L"\"");
	lstrcatW(cmd, conhost);
	lstrcatW(cmd, L"\" \"");
	lstrcatW(cmd, exePath);
	lstrcatW(cmd, L"\" --classic");

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	if (!CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
		return 0;   // 실패하면 지금 창에서 그대로 진행
	}
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return 1;
}

int main(int argc, char* argv[]) {

	int firstRun = 1; // 인트로 판별 용
	Game g;

	if (RelaunchInClassicConsole(argc, argv)) {
		return 0;   // 새 클래식 콘솔 창에서 게임이 시작됐으므로 이 창은 끝낸다
	}
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


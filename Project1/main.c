#include "game.h"


void CheckGoal(Game* g){
	
	if (g->p.x == g->map.goalX &&g->p.y == g->map.goalY && g->p.onGround) g->holdTick+=1;
	else g->holdTick = 0;
	if (g->holdTick>= DRINK_TICK){
		g->state = ST_CLEAR;
	}
}

int InitGame(Game* g){

	memset(g, 0, sizeof(Game));
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);   // 콘솔 출력 손잡이
	SMALL_RECT win = { 0, 0, 1, 1 };
	SetConsoleWindowInfo(h, TRUE, &win);          //1 창을 먼저 아주 작게

	COORD size = { SCREEN_W, SCREEN_H };
	SetConsoleScreenBufferSize(h, size);          // 2 버퍼를 120x30으로

	win.Right = SCREEN_W - 1;
	win.Bottom = SCREEN_H - 1;
	SetConsoleWindowInfo(h, TRUE, &win);  // 3 창을 버퍼 크기에 맞춤
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
		if(ShowEnding(&g) ==0){
			break;
		}
		
	}
	// 게임 종료 
	CloseGame(&g);
	return 0; 
	 
}	


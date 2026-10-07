#ifndef GAME_H   // [수정] _GAME_H → GAME_H (예약 식별자 회피)
#define GAME_H

#define _CRT_SECURE_NO_WARNINGS // fopen 등 C4996 경고(에러) 끄기. 반드시 #include 보다 위에 있어야 한다

#include <stdio.h>
#include <conio.h>     // _kbhit(), _getch() 처럼 밑줄 버전 사용
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <Windows.h>
#include <ctype.h>
#include <string.h>

#if 1
#define MY_DEBUG   // [수정] __MY_DEBUG → MY_DEBUG (예약 식별자 회피). .c 의 #ifdef 도 같이 바꿀 것
#endif

/////  화면·플레이어 //////
#define SCREEN_W 120 // 콘솔 화면 크기 (가로 칸 수)
#define SCREEN_H 30 // 콘솔 화면 크기 (세로 줄 수)
#define MAP_W 120  // 맵 크기. 화면 0번 줄은 HUD, 1~29번 줄이 맵
#define MAP_H 29 // 맵 크기. 화면 0번 줄은 HUD, 1~29번 줄이 맵
#define PLAYER_H 2 // 작아진 플레이어의 세로 크기 (머리 1칸 + 몸 1칸)
#define PLAYER_MAX_HP 3 // 시작 체력
#define FRAME_MS 50 // 한 프레임 시간(ms). 약 20FPS
#define INV_TICK 20 // 피격 후 무적 시간(프레임). 약 1초

/////  이동·물리 //////
#define GRAVITY 1 // 매 프레임 세로 속도에 더해지는 중력 (바람보다 먼저 더한다)
#define MAX_FALL 2 // 최대 낙하 속도. 벽 뚫림 방지
#define JUMP_POWER (-3) // [수정] 괄호 추가. 점프할 때 세로 속도에 넣는 값 (위쪽이 음수)
#define MAX_RISE 2 // 바람·점프로 올라갈 수 있는 최대 속도 (vy 는 -2 보다 작아지지 않는다)
#define KNOCK_VY (-2) // [수정] 괄호 추가. 맞았을 때 튀어오르는 세로 속도

/////  이벤트 1 — 낙하물 //////
#define MAX_DROPS 8 // 동시에 존재할 수 있는 낙하물 수. 가득 차면 새로 만들지 않는다
#define SPAWN_TERM 30 // 낙하물 생성 검사 주기(프레임)
#define SPAWN_RATE 60 // 생성 확률(%)
#define SHAKE_TICK 12 // 떨어지기 전 흔들리는 경고 시간(프레임). 난이도의 핵심 값
#define LAND_TICK 6 // 착지한 뒤 사라질 때까지의 시간(프레임)
#define DROP_SPEED 1 // 한 프레임에 내려가는 칸 수

/////  이벤트 2·3 — 선풍기 / 개미 //////
#define MAX_FANS 2 //  선풍기
#define MAX_BUTTONS 2 //  버튼 개수
#define FAN_POWER 2 // 바람 세기. GRAVITY 보다 커야 떠오른다
#define MAX_ANTS 6 // 개미 최대 마리 수
#define ANT_TERM 4 // 개미가 몇 프레임마다 움직이는가
#define ANT_STEP 1 // 한 번에 몇 칸씩 움직이는가
/////  목표·연출 //////
#define DRINK_TICK 10 // 약병 위에서 클리어로 인정되기까지 머물러야 하는 프레임 수
#define CUT_SLICE_MS 10 // 컷만화 대기를 쪼개는 단위(ms). 이 간격마다 건너뛰기 키를 확인

/////  맵 타일 문자 //////////////////////////////////////////////////////////
//                                                                            /
// '#' 벽 / 바닥 막힘. 책상 다리, 바닥, 벽                                     / 
// '=' 발판 막힘. 책, 필통, 의자 등 올라설 수 있는 물건                         /
// 'F' 선풍기 막힘. 위에 올라서면 발 칸에 바람이 닿아 떠오른다                   /
// ' ' 빈칸 통과 가능                                                          /
// 'B' 버튼(블록) 통과 가능. 발이 이 칸에 들어오면 연결된 선풍기 ON/OFF          /
// '*' 약병(목표) 통과 가능. 여기서 바닥에 서서 DRINK_TICK 만큼 머물면 클리어     /
//  화면에만 그리는 문자 : '0' 머리 / '^' 몸 / 'o' 낙하물 / 'm' 개미 / '~' 바람  /
//                                                                            /
///////////////////////////////////////////////////////////////////////////////

typedef enum { //ST_PLAY 인 동안 게임 루프가 돈다. ST_EXIT(Q) 이면 엔딩 없이 종료한다
	ST_PLAY, // 시작  0 
	ST_CLEAR, // 클리어 1
 	ST_GAMEOVER, // 게임 오버 2
	ST_EXIT // 종료 3

}GameState;

typedef enum { // 낙하물 한 개의 상태. 흔들림 → 낙하 → 착지 순으로 바뀐다. 빈 자리는 active = 0 으로 구분
	DROP_SHAKE,
	DROP_FALL,
	DROP_LAND

}DropState;



typedef struct Player { // 플레이어 위치, 방향, 속도, 점프 상태 관리
	                                       //y-1 < - 머리 
	int x, y; // 플레이어 발좌표 머리는 y-1  y  <-   발
	int vy; // 세로 속도 (음수는 위로 양수는 아래로)
	int face; // 바라보는 방향 (-1왼쪽/+1 오른쪽) 낙하물에 맞았을때 밀리는 방향 계산시에 사용
	int onGround; // 바닥에 닿아 있으면 1. 점프 가능 여부, 클리어 판정에 사용
	int hp; // 남은 체력. 0 이 되면 게임 오버
	int invTick; // 남은 무적 프레임 수. 0 보다 크면 피격 판정 생략 + 깜빡임

}Player; // p 하기 


typedef struct Drop {
	int x, y; //낙하물 좌표
	int prevY; // 이번 프레임에 움직이기 전의 y. prevY ~ y 구간으로 충돌을 봐서 관통을 막는다
	DropState state;  // [수정] int → DropState. DROP_SHAKE(흔들림) / DROP_FALL(낙하) / DROP_LAND(착지)
	int tick; //현재 상태가 유지되는 남은 프레임 수
	int shakeX; //흔들림 보정값 (-1 또는 +1). 그릴 때 위치에만 반영
	int active; //이 자리를 사용 중이면 1
}Drop;

typedef struct Ant {
	int x, y; // 개미 좌표
	int dir; // 진행 방향 (-1 왼쪽 / +1 오른쪽)
	int left, right; // 왕복 순찰 구간의 양 끝 x 좌표
	int moveTick; // 다음 이동까지 남은 프레임 수
	int alive; // 살아 있으면 1
}Ant;



typedef struct Fan {
	int x, y; //선풍기 위치 (바닥 위에 놓이며 위쪽으로 바람을 분다)
	int on; // 켜져 있으면 1. 버튼으로 토글
	int power; // 바람 세기 = FAN_POWER(2). GRAVITY(1) 보다 커야 실제로 떠오른다
	int range; // 바람이 위로 닿는 최대 높이(칸). 발판에 막히면 그 전에서 멈춘다
}Fan;

typedef struct Button {
	int x, y; // 버튼 위치 (밟고 지나가는 칸)
	int fanId; // 이 버튼이 조작하는 선풍기 번호
	int pressed; // 지금 밟고 있으면 1. 밟고 있는 동안 매 프레임 토글되는 것을 막는다

}Button;

typedef struct Map {
	char tile[MAP_H][MAP_W + 1]; //맵 타일 문자 배열 (120글자 + '\0'). LoadMap 에서는 임시 버퍼로 읽고 개행 제거 후 복사할 것
	int goalX, goalY; //목표(책상 위 약병) 좌표

}Map;

typedef struct Game {
	Player p;
	Map map; //맵 타일과 목표 좌표
	Drop drops[MAX_DROPS]; // 낙하물 배열
	Ant ants[MAX_ANTS]; // 개미 배열
	Fan fans[MAX_FANS]; // 선풍기 배열
	Button buttons[MAX_BUTTONS]; // 버튼 배열
	int wind[MAP_H][MAP_W]; // 칸마다 걸린 상승 바람 세기. UpdateFans 가 매 프레임 다시 계산
	int inDx, inJump; // 이번 프레임의 입력 요청 (좌우 -1/0/+1, 점프 0/1)
	int hit; // 이번 프레임에 맞았으면 1. 피해를 프레임당 한 번만 주기 위한 표시
	int kdir; // 맞았을 때 밀려날 방향 (-1 / +1). 개미면 개미 반대쪽, 낙하물이면 - face
	int holdTick; // 약병 위에 머문 프레임 수 (클리어 판정용)
	GameState state; // ST_PLAY / ST_CLEAR / ST_GAMEOVER / ST_EXIT
	unsigned long frame; // 시작부터 지난 프레임 수. 생성 주기·애니메이션·걸린 시간 기준

}Game;


// 함수 선언 파트 
// [수정] int main(); 선언 삭제 — main 은 헤더에 선언하지 않는다. 아래 설명은 main 정의 위에 옮겨 적을 것
// main : 프로그램 진입점. 초기화 → (처음이면) 인트로 → 게임 루프 → 엔딩 → 정리 순으로 호출하고 재시작 여부를 판단한다. [그림 1]
int InitGame(Game* g); // 콘솔 설정, 맵 로드, 플레이어·낙하물·개미·선풍기·버튼을 시작 상태로 초기화한다 성공하면 1, 맵 파일을 열지 못하면 안내를 보여 준 뒤 0 을 반환한다. [그림 2]
int LoadMap(Map* m, int stage); // Map *m  - map.txt 를 한 줄씩 읽어 tile[][] 에 채운다. 성공하면 1, 파일을 열지 못하면 0 을 반환한다
void ShowIntro(void); // [수정] () → (void). 약을 마시고 작아지는 4컷 인트로 만화를 출력한다. 어느 컷에서든 키를 누르면 조작법 안내로 건너뛴다. [그림 3]
void DrawCut(int cut, int k); // 컷만화 한 컷(테두리 + 그림 + 대사)을 화면에 그린다. k 는 3컷·엔딩 2컷에서 사람의 크기(1~3), 그밖의 컷은 0 이다
int WaitCut(int ms); //ms 동안 대기하되 CUT_SLICE_MS(10ms) 씩 나눠 자며 키를 확인한다. 키가 눌렸으면 그 키를 읽어서 버리고 1 을 반환한다.단일 스레드에서 한 번에 오래 자면 그 동안 키를 받지 못하기 때문이다. 인트로·엔딩(게임 루프 밖)에서만 쓴다.

void RunGameLoop(Game* g); // 입력·갱신·출력을 한 프레임으로 묶어 state 가 ST_PLAY 인 동안 반복하고, 프레임 시간을 일정하게 유지한다. 스레드 없이 이 한 줄기로만 돈다. [그림 4]
void ProcessInput(Game* g); // 쌓여 있는 키를 기다리지 않고 모두 읽어 이동·점프 요청(inDx, inJump)에 기록한다. 실제 이동은 하지 않는다. [그림 5]

void UpdateGame(Game* g); // 한 프레임의 게임 내용을 갱신한다. 호출 순서를 바꾸지 않는다. [그림 6]
void UpdatePlayer(Game* g); // 좌우 이동 → 점프 → 중력 → 바람 → 속도 제한 → 세로 이동 순으로 플레이어의 한 프레임을 처리한다. [그림 9]
void MovePlayerX(Game* g, int dx); // 가로로 한 칸 이동을 시도하고 막혀 있으면 취소한다.
void TryJump(Player* p); // 바닥에 있을 때만 세로 속도에 JUMP_POWER 를 넣어 점프시킨다.
void ApplyGravity(Player* p); // 세로 속도에 중력을 더한다. 바람보다 먼저 호출한다
void ApplyFanWind(Game* g); // 플레이어의 발 칸에 바람이 있으면 세로 속도에서 바람 세기를 빼 위로 띄운다
void MovePlayerY(Game* g); // 세로 속도를 제한하고 |vy| 칸만큼 한 칸씩 이동하며 착지·천장 충돌을 판정한다.
int IsSolid(Map* m, int x, int y); // 해당 칸이 막혀 있는지 검사한다. 막혀 있으면 1, 지나갈 수 있으면 0 을 반환한다 플레이어·개미·낙하물·바람이 모두 이 함수를 사용하므로 가장 먼저 완성해야 한다.
void UpdateButtons(Game* g); // 버튼(블록)을 새로 밟은 순간에만 연결된 선풍기를 ON/OFF 토글한다. (이벤트 2) [그림 7]
void UpdateFans(Game* g); // 켜진 선풍기의 바람이 닿는 칸을 위쪽으로 계산해 wind[y][x] 에 기록한다. (이벤트 2) [그림 8]
void UpdateDrops(Game* g); // 낙하물을 흔들림 → 낙하 → 착지 → 소멸 순서로 갱신한다. (이벤트 1) [그림 10]
int SpawnDrop(Game* g); // 빈 자리를 찾아 랜덤 위치에 새 낙하물을 만든다. 성공하면 자리 번호, 빈 자리가 없으면 -1 을 반환한다.
void UpdateAnts(Game* g); // 개미를 정해진 구간에서 좌우로 왕복시킨다. (이벤트 3) [그림 11]
void CheckCollisions(Game* g); // 낙하물·개미와 플레이어가 겹쳤는지 모두 확인한 뒤, 맞았으면 한 번만 피해를 준다. [그림 12]
void DamagePlayer(Game* g); // HP 감소·무적 시간 설정·넉백을 한곳에서 처리한다. CheckCollisions 에서만 호출한다. [그림 13]
void CheckGoal(Game* g); // 책상 위 약병에 도달해 일정 시간 머물렀는지 판정한다. [그림 14]
void Render(Game* g); // 화면 버퍼에 한 프레임을 모두 그린 뒤 한 번에 출력한다. 출력 전용 스레드는 쓰지 않는다. [그림15]
void ClearBuffer(char buf[][SCREEN_W + 1]); // [수정] ClearBuffe → ClearBuffer 오타 수정. 화면 버퍼 전체를 공백으로 초기화한다
void FlushBuffer(char buf[][SCREEN_W + 1]); // 버퍼를 한 번에 출력한다. '\n' 없이 줄마다 WriteConsoleOutputCharacterA 로 써서 자동 줄바꿈·스크롤을 막을 것
void DrawMap(char buf[][SCREEN_W + 1], Game* g); // DrawMap : tile[y][x] 를 화면 (x, y + 1) 자리에 옮긴다. 0번 줄은 HUD 자리이다.
void DrawWind(char buf[][SCREEN_W + 1], Game* g); //  DrawWind : wind[y][x] 가 0 보다 큰 칸에 바람 문자를 그린다. 문자는 frame % 4 로 바꿔 움직이는 것처럼 보인다.
void DrawObjects(char buf[][SCREEN_W + 1], Game* g); //  DrawObjects : 낙하물(흔들림 보정 포함)과 개미를 그린다
void DrawPlayer(char buf[][SCREEN_W + 1], Game* g); // DrawPlayer : 머리(y-1)와 몸(y) 두 칸을 그린다.
void DrawHUD(char buf[][SCREEN_W + 1], Game* g); //  DrawHUD : 화면 맨 윗줄에 HP, 선풍기 ON/OFF, 경과 시간(frame ÷ 20 초)을 쓴다.
int ShowEnding(Game* g); // 클리어면 커지는 엔딩 컷만화를, 게임 오버면 GAME OVER 화면을 출력하고 다시 할지 묻는다 R 을 누르면 1, Q 를 누르면 0 을 반환한다. 게임 루프 밖이므로 키를 기다려도 된다. [그림 16]
void CloseGame(Game* g); // 커서를 되돌리고 화면을 정리한 뒤 프로그램을 끝낼 준비를 한다. [그림 17]
void gotoxy(int x, int y); // gotoxy 는 콘솔 커서를 지정 좌표로 옮기고
void HideCursor(void); // [수정] () → (void). HideCursor 는 커서를 숨기며,
int RandRange(int a, int b); // RandRange 는 a 이상 b 이하의 난수를 반환한다.

#endif // GAME_H

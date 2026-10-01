#include "game.h"

// 맵의 특정 좌표가 막혀 있는 칸인지 확인하는 함수(반환값은 0 = 이동 가능 / 1 = 이동 불가능)
int IsSolid(Map* m, int x, int y) {
    // 검사할 좌표가 맵 범위를 벗어나면 막힌 칸으로 처리
    if (!(x >= 0 && x < MAP_W && y >= 0 && y < MAP_H)) {
        return 1;
    }

    // 좌표값이 #(벽/바닥), =(발판), F(선풍기) 이라면 이동 불가능
    if (m->tile[y][x] == '#' ||
        m->tile[y][x] == '=' ||
        m->tile[y][x] == 'F') {
        return 1;
    }
    // 빈칸, 버튼, 약병 등은 통과 가능
    else {
        return 0;
    }
}


// map.txt 파일을 읽어 게임 맵을 불러오는 함수(반환값은 1 = 맵 불러오기 성공 / 0 = 실패)
int LoadMap(Map* map) {

    FILE* fp;               // 맵 파일을 가리키는 파일 포인터
    char line[MAP_W + 2];   // 파일에서 읽은 한 줄을 임시로 저장하는 배열(+2는 줄바꿈 문자(\n) 와 문자열 종료 문자(\0) 공간)

    fp = fopen("map.txt", "r"); // map.txt 파일을 읽기 모드("r")로 열기

    // 파일을 열지 못하면 실패 반환
    if (fp == NULL) {
        return 0;
    }

    for (int y = 0; y < MAP_H; y++) {

        // 파일에서 한 줄을 읽어 line 배열에 저장, 만약 읽을 줄이 부족하면 파일을 닫고 실패 반환
        if (fgets(line, sizeof(line), fp) == NULL) {
            fclose(fp);
            return 0;
        }

        // 읽어 온 한 줄의 문자를 맵 배열에 복사
        for (int x = 0; x < MAP_W; x++) {
            map->tile[y][x] = line[x];
            if (map->tile[y][x]=='*'){
                map->goalX = x;
                 map->goalY = y;
            }
        }
    }

    fclose(fp); // 맵을 모두 읽었으므로 파일 닫기

    return 1;
}


// 화면 출력용 버퍼를 공백으로 초기화하는 함수
void ClearBuffer(char buf[][SCREEN_W + 1]) {

    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            buf[y][x] = ' '; // 현재 칸을 공백 문자로 초기화
        }

        buf[y][SCREEN_W] = '\0';// 각 행의 마지막에 문자열 종료 문자 삽입
    }
}




// 완성된 화면 버퍼를 콘솔에 출력하는 함수
void FlushBuffer(char buf[][SCREEN_W + 1]) {

    gotoxy(0, 0); // 콘솔 커서를 화면 맨 왼쪽 위로 이동

    for (int y = 0; y < SCREEN_H; y++) {

        // 마지막 행일 경우
        if (y == SCREEN_H - 1) {
            buf[y][SCREEN_W - 1] = '\0'; // 마지막 칸에 문자열 종료 문자 삽입
        }

        printf("%s", buf[y]); // 현재 행의 문자열 출력

        // 마지막 행이 아니라면 다음 행으로 이동
        if (y < SCREEN_H - 1) {
            gotoxy(0, y + 1);
        }
    }
}


// 맵에 저장된 타일들을 화면 버퍼에 그리는 함수, buf에 맵을 복사
void DrawMap(char buf[][SCREEN_W + 1], Game* g) {

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            buf[y + 1][x] = g->map.tile[y][x]; // 맵의 위치에 있는 타일 문자를 버퍼에 복사(0번 줄은 HUD이므로 y+1 위치에 그림)
        }
    }
}


// 선풍기에서 발생하는 바람을 화면에 그리는 함수
void DrawWind(char buf[][SCREEN_W + 1], Game* g) {

    const char windAnimation[4] = { '|', '/', '|', '\\' }; // 바람 애니메이션에 사용할 문자 4개

    char windChar = windAnimation[g->frame % 4]; // 현재 프레임을 4로 나눈 나머지로 바람 모양 선택

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {

            // 현재 칸에 선풍기 바람이 존재하면
            if (g->wind[y][x] > 0) {
                buf[y + 1][x] = windChar; //바람 문자를 덮어 그리기
            }
        }
    }
}


// 낙하물과 개미를 화면 버퍼에 그리는 함수('o', 개미는 'm' 문자로 표현)
void DrawObjects(char buf[][SCREEN_W + 1], Game* g) {

    for (int i = 0; i < MAX_DROPS; i++) {

        Drop* d = &g->drops[i]; // 현재 확인 중인 낙하물의 주소 저장

        // 사용 중이지 않은 낙하물이면 그리지 않고 다음 낙하물 확인
        if (d->active == 0) {
            continue;
        }

        int drawX = d->x; // 낙하물의 실제 x좌표를 화면에 그릴 좌표로 저장

        // 낙하물이 떨어지기 전 흔들리는 상태
        if (d->state == DROP_SHAKE) {
            drawX += d->shakeX; // 흔들림 값(-1 또는 +1)을 적용하여 좌우 흔들림 표현
        }

        // 그릴 x좌표가 맵 밖이면 그리지 않음
        if (drawX < 0 || drawX >= MAP_W) {
            continue;
        }

        // 낙하물의 y좌표가 맵 밖이면 그리지 않음
        if (d->y < 0 || d->y >= MAP_H) {
            continue;
        }

        buf[d->y + 1][drawX] = 'o'; // 낙하물의 위치에 'o' 문자 그리기
    }

    for (int i = 0; i < MAX_ANTS; i++) {

        Ant* a = &g->ants[i]; // 현재 확인 중인 개미의 주소 저장

        // 살아 있지 않은 개미는 그리지 않고 건너뛰기
        if (a->alive == 0) {
            continue;
        }

        // 개미의 x좌표가 맵 밖이면 그리지 않음
        if (a->x < 0 || a->x >= MAP_W) {
            continue;
        }

        // 개미의 y좌표가 맵 밖이면 그리지 않음
        if (a->y < 0 || a->y >= MAP_H) {
            continue;
        }

        // 개미의 위치에 'm' 문자 그리기
        buf[a->y + 1][a->x] = 'm';
    }
}


// 플레이어를 화면 버퍼에 그리는 함수(플레이어는 머리 '0'과 몸 '^'으로 이루어진 세로 2칸)
void DrawPlayer(char buf[][SCREEN_W + 1], Game* g) {

    Player* p = &g->p; // 게임 구조체에 저장된 플레이어의 주소
    int x, y;           // 플레이어를 그릴 좌표

    // 무적 시간이 남아 있고, 현재 프레임이 깜빡임 구간이면 그리지 않고 함수 종료
    if (p->invTick > 0 && g->frame % 4 < 2) {
        return;
    }

    // 플레이어의 현재 위치 저장(x는 가로 위치, y는 몸의 위치)
    x = p->x;
    y = p->y;

    // 플레이어의 x좌표가 맵 밖이면 그리지 않음
    if (x < 0 || x >= MAP_W) {
        return;
    }

    // 플레이어 머리의 위치가 맵 범위 안이면
    if (y - 1 >= 0 && y - 1 < MAP_H) {
        buf[y][x] = '0'; // 머리는 몸보다 한 칸 위에 그림
    }

    // 플레이어 몸의 위치가 맵 범위 안이면
    if (y >= 0 && y < MAP_H) {

        // 몸은 플레이어의 발 좌표에 '^' 문자로 그림
        buf[y + 1][x] = '^';
    }
}


// 화면 맨 윗줄에 게임 상태(HUD)를 표시하는 함수(HP, 선풍기 ON/OFF 상태, 게임 경과 시간을 출력)
void DrawHUD(char buf[][SCREEN_W + 1], Game* g) {

    char hud[SCREEN_W + 1]; // HUD 문자열을 임시로 저장할 배열

    unsigned int seconds = g->frame / 20; // FRAME_MS가 50ms이므로 1초에 약 20프레임 실행(경과 시간을 초 단위로 계산)

    const char* fan1; // 첫 번째 선풍기의 ON/OFF 상태 문자열
    const char* fan2; // 두 번째 선풍기의 ON/OFF 상태 문자열

    // 첫 번째 선풍기가 켜져 있으면 ON, 아니면 OFF
    if (g->fans[0].on == 1) {
        fan1 = "ON";
    }
    else {
        fan1 = "OFF";
    }

    // 두 번째 선풍기가 켜져 있으면 ON, 아니면 OFF
    if (g->fans[1].on == 1) {
        fan2 = "ON";
    }
    else {
        fan2 = "OFF";
    }

    // 현재 게임 상태를 하나의 문자열로 만들어 hud에 저장
    snprintf(
        hud,
        sizeof(hud),
        "HP: %d/%d | FAN1: %s | FAN2: %s | TIME: %u sec",
        g->p.hp,      // 현재 플레이어 체력
        PLAYER_MAX_HP,  // 플레이어 최대 체력
        fan1,           // 첫 번째 선풍기 상태
        fan2,           // 두 번째 선풍기 상태
        seconds         // 게임 경과 시간(초)
    );

    memcpy(buf[0], hud, strlen(hud)); // 완성된 HUD 문자열을 화면 버퍼의 첫 번째 줄에 복사
}


// 한 프레임의 게임 화면을 완성하고 콘솔에 출력하는 함수(맵 → 바람 → 낙하물·개미 → 플레이어 → HUD 순서로 그림)
void Render(Game* g) {

    char buf[SCREEN_H][SCREEN_W + 1];// 실제 콘솔에 출력하기 전 그림을 저장할 임시 화면 버퍼(각 행에 문자열 종료 문자 공간(+1)을 추가)

    ClearBuffer(buf); // 이전 프레임의 그림이 남지 않도록 버퍼 전체를 공백으로 초기화

    DrawMap(buf, g); // 맵의 벽, 바닥, 발판, 선풍기 등의 기본 타일 그리기

    DrawWind(buf, g); // 켜져 있는 선풍기의 바람을 맵 위에 그리기

    DrawObjects(buf, g); // 낙하물과 개미를 바람 및 맵 위에 그리기

    DrawPlayer(buf, g); // 플레이어의 머리와 몸을 그리기

    DrawHUD(buf, g); // 화면 맨 윗줄에 HP, 선풍기 상태, 경과 시간 표시

    FlushBuffer(buf); // 완성된 화면 버퍼를 콘솔에 한 번에 출력
}


// 콘솔 커서를 (x, y) 위치로 옮기는 함수(FlushBuffer 가 줄마다 사용)
void gotoxy(int x, int y) {
    COORD pos = { (SHORT)x, (SHORT)y };  // 이동할 좌표

    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}


// 깜빡이는 콘솔 커서를 숨기는 함수(InitGame 에서 호출, CloseGame 에서 다시 보이게 함)
void HideCursor(void) {
    CONSOLE_CURSOR_INFO cursor;  // 커서 정보 구조체

    cursor.dwSize = 20;          // 커서 두께(1~100). 0 이면 설정이 실패한다
    cursor.bVisible = FALSE;     // FALSE = 숨김 / TRUE = 보임

    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor);
}

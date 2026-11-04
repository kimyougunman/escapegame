#include "game.h"

// 맵의 특정 좌표가 막혀 있는 칸인지 확인하는 함수(반환값은 0 = 이동 가능 / 1 = 이동 불가능)
int IsSolid(Map* m, int x, int y) {
    // 검사할 좌표가 맵 범위를 벗어나면 막힌 칸으로 처리
    if (!(x >= 0 && x < MAP_W && y >= 0 && y < MAP_H)) {
        return 1;
    }

    // 좌표값이 #(벽/바닥), =(발판), F(선풍기), B(버튼) 이라면 이동 불가능
    if (m->tile[y][x] == '#' ||
        m->tile[y][x] == '=' ||
        m->tile[y][x] == 'F' ||
        m->tile[y][x] == 'B') {
        return 1;
    }
    // 빈칸, 약병 등은 통과 가능
    else {
        return 0;
    }
}

void gotoxy(int x, int y) {

    COORD pos; // 콘솔 커서 위치 저장

    pos.X = (SHORT)x;
    pos.Y = (SHORT)y;

    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos); // 커서를 (x, y) 위치로 이동
}


void HideCursor(void) {

    CONSOLE_CURSOR_INFO cursorInfo; // 콘솔 커서 정보 저장

    cursorInfo.dwSize = 1;
    cursorInfo.bVisible = FALSE; // 커서를 보이지 않게 설정

    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo); // 설정 적용
}


int RandRange(int a, int b) {

    // a가 b보다 큰 값으로 들어오면 두 값을 서로 바꿔줌
    if (a > b) {
        int temp = a;
        a = b;
        b = temp;
    }

    return a + rand() % (b - a + 1); // a 이상 b 이하의 랜덤한 정수 반환
}


// map.txt 파일을 읽어 게임 맵을 불러오는 함수(반환값은 1 = 맵 불러오기 성공 / 0 = 실패)
int LoadMap(Map* m, int stage) {

    FILE* fp;               // 맵 파일을 가리키는 파일 포인터
    char line[MAP_W + 2];   // 파일에서 읽은 한 줄을 임시로 저장하는 배열(+2는 줄바꿈 문자(\n) 와 문자열 종료 문자(\0) 공간)

    fp = fopen("map.txt", "r"); // map.txt 파일을 읽기 모드("r")로 열기 (현재 작업 폴더 기준)

    // 못 열었으면 실행 파일(.exe)이 있는 폴더, 그 위 폴더들에서도 찾아본다
    // (exe 를 더블클릭하면 작업 폴더가 x64\Debug 가 되어 프로젝트 폴더의 map.txt 를 못 찾기 때문)
    if (fp == NULL) {
        wchar_t path[MAX_PATH];
        DWORD len = GetModuleFileNameW(NULL, path, MAX_PATH); // exe 의 전체 경로 (한글 폴더 때문에 W 버전)
        for (int up = 0; fp == NULL && len > 0 && up < 4; up++) {
            wchar_t* slash = wcsrchr(path, L'\\');            // 마지막 폴더 구분자
            if (slash == NULL) break;
            *slash = L'\0';                                    // 한 단계 위 폴더로
            wchar_t file[MAX_PATH];
            swprintf(file, MAX_PATH, L"%ls\\map.txt", path);
            fp = _wfopen(file, L"r");
            if (fp == NULL) {                                  // 바깥 솔루션(.slnx)으로 빌드한 경우 : 그 폴더의 Project1\map.txt
                swprintf(file, MAX_PATH, L"%ls\\Project1\\map.txt", path);
                fp = _wfopen(file, L"r");
            }
        }
    }

    // 그래도 파일을 열지 못하면 실패 반환
    if (fp == NULL) {
        return 0;
    }

    // 원하는 스테이지 전까지의 맵 줄을 건너뜀
    // stage가 1이면 0줄, 2이면 29줄, 3이면 58줄 건너뜀
    int skipLines = (stage - 1) * MAP_H;

    for (int i = 0; i < skipLines; i++) {

        if (fgets(line, sizeof(line), fp) == NULL) {
            fclose(fp);
            return 0;
        }
    }

    for (int y = 0; y < MAP_H; y++) {

        // 파일에서 한 줄을 읽어 line 배열에 저장, 만약 읽을 줄이 부족하면 파일을 닫고 실패 반환
        if (fgets(line, sizeof(line), fp) == NULL) {
            fclose(fp);
            return 0;
        }

        // 현재 줄의 실제 글자 수를 구함(\r, \n 제외)
        size_t len = strcspn(line, "\r\n");

        // 한 줄이 MAP_W 길이와 다르면 잘못된 맵 파일로 판단
        if (len != MAP_W) {
            fclose(fp);
            return 0;
        }

        // 읽어 온 한 줄의 문자를 맵 배열에 복사
        for (int x = 0; x < MAP_W; x++) {
            m->tile[y][x] = line[x];
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
// 전체 화면이라 콘솔 줄 수가 SCREEN_H 보다 많으면, 남는 줄의 절반만큼 내려서 세로 가운데에 출력한다
// 콘솔 화면 버퍼에서 게임 화면(SCREEN_W x SCREEN_H)이 놓일 왼쪽 위 위치를 구한다
// 콘솔이 게임 화면보다 크면(전체 화면, Alt+Enter 등) 가로·세로 가운데에 놓는다
static void GetScreenOrigin(CONSOLE_SCREEN_BUFFER_INFO* info, int* left, int* top) {
    *left = (info->dwSize.X > SCREEN_W) ? (info->dwSize.X - SCREEN_W) / 2 : 0;
    *top = (info->dwSize.Y > SCREEN_H) ? (info->dwSize.Y - SCREEN_H) / 2 : 0;
}

// 게임 화면 바깥(위·아래 남는 줄, 좌우 남는 칸)을 빈칸으로 지운다
// Alt+Enter 등으로 콘솔 크기가 바뀌면 게임 화면 위치가 옮겨지는데, 예전 위치에 찍혀 있던 글자가 남지 않게 한다
static void ClearOutsideScreen(HANDLE h, CONSOLE_SCREEN_BUFFER_INFO* info, int left, int top) {
    DWORD written;
    WORD gray = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    int width = info->dwSize.X;
    int rows = info->dwSize.Y;
    COORD pos;

    for (int y = 0; y < rows; y++) {
        pos.Y = (SHORT)y;
        if (y < top || y >= top + SCREEN_H) {              // 위·아래 남는 줄 : 한 줄 전체
            pos.X = 0;
            FillConsoleOutputCharacterA(h, ' ', width, pos, &written);
            FillConsoleOutputAttribute(h, gray, width, pos, &written);
        }
        else {
            if (left > 0) {                                // 왼쪽 남는 칸
                pos.X = 0;
                FillConsoleOutputCharacterA(h, ' ', left, pos, &written);
            }
            if (left + SCREEN_W < width) {                 // 오른쪽 남는 칸
                pos.X = (SHORT)(left + SCREEN_W);
                FillConsoleOutputCharacterA(h, ' ', width - (left + SCREEN_W), pos, &written);
            }
        }
    }
}

void FlushBuffer(char buf[][SCREEN_W + 1]) {

    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    int rows = SCREEN_H, width = SCREEN_W;   // 콘솔 화면 버퍼의 실제 줄 수·칸 수
    int left = 0, top = 0;                   // 게임 화면 (0, 0) 이 출력될 콘솔 위치

    if (GetConsoleScreenBufferInfo(h, &info)) {
        rows = info.dwSize.Y;
        width = info.dwSize.X;
        GetScreenOrigin(&info, &left, &top);
        ClearOutsideScreen(h, &info, left, top);   // 예전 위치의 흔적 지우기
    }

    for (int y = 0; y < SCREEN_H; y++) {

        gotoxy(left, top + y); // 출력할 줄의 맨 왼쪽으로 커서 이동

        // 콘솔의 맨 오른쪽 아래 칸이면 비운다 (그 칸까지 쓰면 커서가 넘어가며 화면 전체가 한 줄 올라간다)
        if (top + y == rows - 1 && left + SCREEN_W == width) {
            buf[y][SCREEN_W - 1] = '\0';
        }

        printf("%s", buf[y]); // 현재 행의 문자열 출력
    }

    fflush(stdout); // 출력 대기 중인 내용을 콘솔 화면에 출력
}


// 맵에 저장된 타일들을 화면 버퍼에 그리는 함수, buf에 맵을 복사
void DrawMap(char buf[][SCREEN_W + 1], Game* g) {

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            char c = g->map.tile[y][x];

            buf[y + 1][x] = (c == 'B' || c == 'F') ? ' ' : c; // 맵의 타일 문자를 버퍼에 복사(0번 줄은 HUD이므로 y+1 위치). 버튼·선풍기 칸은 아래에서 그림으로 그린다
        }
    }

    // 선풍기 그리기 : 5 x 3. 켜져 있으면 날개가 + / x 로 번갈아 돈다
    for (int i = 0; i < MAX_FANS; i++) {
        Fan* f = &g->fans[i];
        const char* blade = (f->on && (g->frame / 2) % 2 == 1) ? "(-x-)" : "(-+-)";
        const char* art[FAN_H] = { blade, "  |  ", "[___]" };

        if (f->y < FAN_H - 1 || f->y >= MAP_H || g->map.tile[f->y][f->x] != 'F') {
            continue; // 맵에 없는 선풍기 자리
        }
        for (int row = 0; row < FAN_H; row++) {
            for (int col = 0; col < FAN_W; col++) {
                int mx = f->x - FAN_W / 2 + col;
                int my = f->y - (FAN_H - 1) + row;

                if (mx >= 0 && mx < MAP_W && art[row][col] != ' ') {
                    buf[my + 1][mx] = art[row][col];
                }
            }
        }
    }

    // 버튼 그리기 : 안 눌림은 'ㄷ' 자를 엎은 2 x 2, 눌림은 납작한 2 x 1
    for (int i = 0; i < MAX_BUTTONS; i++) {
        Button* b = &g->buttons[i];

        if (b->y < 1 || b->y >= MAP_H || b->x < 0 || b->x + BUTTON_W > MAP_W || g->map.tile[b->y][b->x] != 'B') {
            continue; // 맵에 없는 버튼 자리
        }
        if (b->pressed == 0) {
            buf[b->y][b->x] = '_';         // 윗줄 (맵 y-1 → 화면 y)
            buf[b->y][b->x + 1] = '_';
            buf[b->y + 1][b->x] = '|';     // 아랫줄 (맵 y → 화면 y+1)
            buf[b->y + 1][b->x + 1] = '|';
        }
        else {
            buf[b->y + 1][b->x] = '_';     // 눌리면 아랫줄만 납작하게
            buf[b->y + 1][b->x + 1] = '_';
        }
    }
}


// 선풍기에서 발생하는 바람을 화면에 그리는 함수
void DrawWind(char buf[][SCREEN_W + 1], Game* g) {

    const char windAnimation[4] = { '|', '^', '|', ':' }; // 바람 애니메이션에 사용할 문자 4개 (역슬래시는 한글 글꼴에서 ₩ 로 보여서 쓰지 않는다)

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {

            // 현재 칸에 선풍기 바람이 존재하면
            if (g->wind[y][x] > 0) {
                // (y + x + frame) 으로 고르면 같은 무늬가 매 프레임 한 줄씩 위로 올라가 보인다. 가운데 줄은 엇갈리게
                buf[y + 1][x] = windAnimation[(y + x * 2 + g->frame) % 4];
            }
        }
    }
}


// 낙하물(돌 5 x 3)과 개미(7 x 3)를 화면 버퍼에 그리는 함수
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

        // 돌 5 x 3 그림 (맨 아랫줄 가운데가 drawX, d->y)
        const char* rock[DROP_H] = { ".---.", "(@@@)", "'---'" };

        for (int row = 0; row < DROP_H; row++) {
            int my = d->y - (DROP_H - 1) + row;
            for (int col = 0; col < DROP_W; col++) {
                int mx = drawX - DROP_W / 2 + col;
                if (mx >= 0 && mx < MAP_W && my >= 0 && my < MAP_H) { // 맵 밖 칸은 그리지 않음
                    buf[my + 1][mx] = rock[row][col];
                }
            }
        }
    }

    for (int i = 0; i < MAX_ANTS; i++) {

        Ant* a = &g->ants[i]; // 현재 확인 중인 개미의 주소 저장

        // 살아 있지 않은 개미는 그리지 않고 건너뛰기
        if (a->alive == 0) {
            continue;
        }

        // 개미 7 x 3 그림. 가는 방향을 바라본다 (왼쪽 아래 칸이 a->x, a->y)
        const char* right[ANT_H] = { "     ,,", "(@)o(O)", "  ^^^  " };
        const char* left[ANT_H] = { ",,     ", "(O)o(@)", "  ^^^  " };
        const char** art = (a->dir > 0) ? right : left;

        for (int row = 0; row < ANT_H; row++) {
            int my = a->y - (ANT_H - 1) + row;
            for (int col = 0; col < ANT_W; col++) {
                int mx = a->x + col;
                if (mx >= 0 && mx < MAP_W && my >= 0 && my < MAP_H && art[row][col] != ' ') { // 맵 밖 칸은 그리지 않음
                    buf[my + 1][mx] = art[row][col];
                }
            }
        }
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

    // 3 x 3 플레이어 그림. 맨 아랫줄이 발(p.y), 가운데 칸이 p.x
    const char* art[PLAYER_H] = {
        "(o)",   // 머리 (y - 2)
        "<|>",   // 몸   (y - 1)
        "| |",   // 발   (y)      ※ 역슬래시는 한글 글꼴에서 ₩ 로 보여서 쓰지 않는다
    };

    // 플레이어의 현재 위치 저장(x는 발 가운데, y는 발의 위치)
    x = p->x;
    y = p->y;

    for (int row = 0; row < PLAYER_H; row++) {
        int my = y - (PLAYER_H - 1) + row;         // 그릴 맵 y

        if (my < 0 || my >= MAP_H) {
            continue;                              // 맵 밖 줄은 그리지 않음
        }
        for (int col = 0; col < PLAYER_W; col++) {
            int mx = x - PLAYER_HALF + col;        // 그릴 맵 x

            if (mx >= 0 && mx < MAP_W && art[row][col] != ' ') {
                buf[my + 1][mx] = art[row][col];   // 0번 줄은 HUD 이므로 화면 y = 맵 y + 1
            }
        }
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
        "HP: %d/%d | FAN1: %s | FAN2: %s | TIME LEFT: %u sec",
        g->p.hp,      // 현재 플레이어 체력
        PLAYER_MAX_HP,  // 플레이어 최대 체력
        fan1,           // 첫 번째 선풍기 상태
        fan2,           // 두 번째 선풍기 상태
        (unsigned int)(seconds < TIME_LIMIT_SEC ? TIME_LIMIT_SEC - seconds : 0) // 남은 시간(초). 0 이 되면 즉사
    );

    memcpy(buf[0], hud, strlen(hud)); // 완성된 HUD 문자열을 화면 버퍼의 첫 번째 줄에 복사
}


// 한 프레임의 게임 화면을 완성하고 콘솔에 출력하는 함수(맵 → 바람 → 낙하물·개미 → 플레이어 → HUD 순서로 그림)
/////  색깔 //////
#define COLOR_DEFAULT (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)   // 기본 회색
#define COLOR_BUTTON  (FOREGROUND_RED | FOREGROUND_INTENSITY)                 // 버튼 : 빨간색
#define COLOR_WIND    (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY) // 바람 : 파란색(밝은 하늘빛이라 검은 바탕에서 잘 보인다)
#define COLOR_ROCK    (FOREGROUND_RED | FOREGROUND_GREEN)   // 돌 : 갈색 (InitGame 에서 이 색 자리를 갈색으로 바꿔 둔다)
#define COLOR_ANT     (FOREGROUND_INTENSITY)                // 개미 : 검정에 가까운 진회색 (InitGame 에서 바꿔 둔다)
#define COLOR_DECOR   (FOREGROUND_BLUE)                     // 배경 장식(창문·시계·액자) : 어두운 청회색 (InitGame 에서 바꿔 둔다)

// 화면 칸마다 색을 정한다. 기본은 회색, 버튼은 빨간색, 바람은 파란색, 돌은 갈색, 개미는 검정(진회색), 플레이어는 기본색
static void ColorScene(WORD attr[][SCREEN_W], Game* g) {

    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            attr[y][x] = COLOR_DEFAULT;
        }
    }

    // 배경 장식 : 벽·발판·선풍기·버튼·약병이 아닌 글자는 어두운 색으로 (게임 물체보다 눈에 덜 띄게)
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            char c = g->map.tile[y][x];
            if (c != ' ' && c != '#' && c != '=' && c != 'F' && c != 'B' && c != '*') {
                attr[y + 1][x] = COLOR_DECOR;
            }
        }
    }

    // 바람 (맵 y -> 화면 y + 1)
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if (g->wind[y][x] > 0) {
                attr[y + 1][x] = COLOR_WIND;
            }
        }
    }

    // 버튼 : 안 눌림은 2 x 2, 눌림은 아랫줄 2 x 1
    for (int i = 0; i < MAX_BUTTONS; i++) {
        Button* b = &g->buttons[i];

        if (b->y < 1 || b->y >= MAP_H || b->x < 0 || b->x + BUTTON_W > MAP_W || g->map.tile[b->y][b->x] != 'B') {
            continue;
        }
        for (int x = b->x; x < b->x + BUTTON_W; x++) {
            attr[b->y + 1][x] = COLOR_BUTTON;          // 아랫줄
            if (b->pressed == 0) {
                attr[b->y][x] = COLOR_BUTTON;          // 윗줄
            }
        }
    }

    // 돌 : 갈색 (5 x 3)
    for (int i = 0; i < MAX_DROPS; i++) {
        Drop* d = &g->drops[i];
        int cx = d->x + (d->state == DROP_SHAKE ? d->shakeX : 0);

        if (!d->active) continue;
        for (int y = d->y - (DROP_H - 1); y <= d->y; y++)
            for (int x = cx - DROP_W / 2; x <= cx + DROP_W / 2; x++)
                if (x >= 0 && x < MAP_W && y >= 0 && y < MAP_H) attr[y + 1][x] = COLOR_ROCK;
    }

    // 개미 : 검정(진회색) (7 x 3)
    for (int i = 0; i < MAX_ANTS; i++) {
        Ant* a = &g->ants[i];

        if (!a->alive) continue;
        for (int y = a->y - (ANT_H - 1); y <= a->y; y++)
            for (int x = a->x; x < a->x + ANT_W; x++)
                if (x >= 0 && x < MAP_W && y >= 0 && y < MAP_H) attr[y + 1][x] = COLOR_ANT;
    }

    // 플레이어는 원래 색으로
    for (int y = g->p.y - (PLAYER_H - 1); y <= g->p.y; y++) {
        for (int x = g->p.x - PLAYER_HALF; x <= g->p.x + PLAYER_HALF; x++) {
            if (x >= 0 && x < MAP_W && y >= 0 && y < MAP_H) attr[y + 1][x] = COLOR_DEFAULT;
        }
    }
}

// 글자 버퍼와 색 버퍼를 WriteConsoleOutput 으로 한 번에 출력한다 (printf 보다 빠르고 칸마다 색을 줄 수 있다)
// 게임 화면 전용. 한글이 있는 인트로 컷은 FlushBuffer 를 쓴다
static void FlushBufferColor(char buf[][SCREEN_W + 1], WORD attr[][SCREEN_W]) {

    static CHAR_INFO cells[SCREEN_H * SCREEN_W];   // 크기가 커서 static (스택 대신)
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    int left = 0, top = 0;
    COORD size = { SCREEN_W, SCREEN_H };
    COORD from = { 0, 0 };
    SMALL_RECT area;

    if (GetConsoleScreenBufferInfo(h, &info)) {
        GetScreenOrigin(&info, &left, &top);       // FlushBuffer 와 같은 가운데 정렬
        ClearOutsideScreen(h, &info, left, top);   // 예전 위치의 흔적 지우기
    }
    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            cells[y * SCREEN_W + x].Char.AsciiChar = buf[y][x] ? buf[y][x] : ' ';
            cells[y * SCREEN_W + x].Attributes = attr[y][x];
        }
    }
    area.Left = (SHORT)left;
    area.Top = (SHORT)top;
    area.Right = (SHORT)(left + SCREEN_W - 1);
    area.Bottom = (SHORT)(top + SCREEN_H - 1);
    WriteConsoleOutputA(h, cells, size, from, &area);
}

/////  방 꾸미기 (벽지·창문·시계·액자·가구·마루) //////
// 장식 위치는 숫자로 박지 않고 맵에서 찾는다. 그래서 map.txt 에서 장식을 옮겨도 색이 따라간다
// 움직이는 물체(돌·개미·플레이어·바람)가 장식 앞을 지나가면 물체는 그대로 두고 뒤 배경색만 이어지게 한다

typedef struct DecorRect {
    int x1, y1, x2, y2;   // 맵 좌표 (테두리 포함)
    int picture;          // 안에 '^' 가 있으면 1 (액자), 없으면 0 (창문)
} DecorRect;

#define MAX_DECOR 8

// 화면 칸 (x, 화면 y) 가 맵 글자 그대로인가 (물체가 덮어쓰지 않았는가)
static int Untouched(char buf[][SCREEN_W + 1], Game* g, int x, int my) {
    return buf[my + 1][x] == g->map.tile[my][x];
}

// 맵에서 '+' 모서리로 된 사각형 장식(창문·액자)을 찾는다. 찾은 개수를 반환한다
static int FindDecorRects(Map* m, DecorRect out[], int maxCount) {
    int n = 0;

    for (int y = 0; y < MAP_H - 1; y++) {
        for (int x = 0; x < MAP_W - 1; x++) {
            int inside = 0;

            if (m->tile[y][x] != '+' || m->tile[y][x + 1] != '-' || m->tile[y + 1][x] != '|') {
                continue;
            }
            for (int i = 0; i < n; i++) {   // 이미 찾은 사각형의 일부 (창문 가운데 십자 등)
                if (x >= out[i].x1 && x <= out[i].x2 && y >= out[i].y1 && y <= out[i].y2) inside = 1;
            }
            if (inside || n >= maxCount) {
                continue;
            }

            int x2 = x, y2 = y;
            for (int i = x + 1; i < MAP_W && (m->tile[y][i] == '-' || m->tile[y][i] == '+'); i++) {
                if (m->tile[y][i] == '+') x2 = i;          // 오른쪽 끝 모서리
            }
            for (int j = y + 1; j < MAP_H && (m->tile[j][x] == '|' || m->tile[j][x] == '+'); j++) {
                if (m->tile[j][x] == '+') y2 = j;          // 아래쪽 끝 모서리
            }
            if (x2 == x || y2 == y || m->tile[y2][x2] != '+') {
                continue;
            }

            out[n].x1 = x; out[n].y1 = y; out[n].x2 = x2; out[n].y2 = y2; out[n].picture = 0;
            for (int j = y; j <= y2; j++)
                for (int i = x; i <= x2; i++)
                    if (m->tile[j][i] == '^') out[n].picture = 1;
            n++;
        }
    }
    return n;
}

// 칸 (x, y) 가 찾은 사각형 장식 안인가
static int InDecorRect(DecorRect r[], int n, int x, int y) {
    for (int i = 0; i < n; i++) {
        if (x >= r[i].x1 && x <= r[i].x2 && y >= r[i].y1 && y <= r[i].y2) return 1;
    }
    return 0;
}

// 맵에서 시계("12" 가 있는 곳)를 찾아 줄마다 왼쪽·오른쪽 끝을 구한다. 시계가 없으면 0
static int FindClock(Map* m, int* top, int* bottom, int left[], int right[]) {
    int cx = -1, cy = -1;

    for (int y = 0; y < MAP_H && cx < 0; y++)
        for (int x = 0; x < MAP_W - 1; x++)
            if (m->tile[y][x] == '1' && m->tile[y][x + 1] == '2') { cx = x; cy = y; break; }
    if (cx < 0) {
        return 0;
    }

    *top = cy - 1;
    *bottom = cy;
    for (int y = cy + 1; y < MAP_H; y++)            // 아래쪽 '6' 을 찾고 그 아랫줄까지가 시계
        for (int x = cx - 2; x <= cx + 3; x++)
            if (x >= 0 && x < MAP_W && m->tile[y][x] == '6') *bottom = y + 1;
    if (*top < 0 || *bottom >= MAP_H) {
        return 0;
    }

    for (int y = *top; y <= *bottom; y++) {
        left[y] = -1; right[y] = -1;
        for (int x = cx - 16; x <= cx + 17; x++) {
            if (x < 1 || x >= MAP_W - 1) continue;
            char c = m->tile[y][x];
            if (c != ' ' && c != '#' && c != '=' && c != 'F' && c != 'B' && c != '*') {
                if (left[y] < 0) left[y] = x;
                right[y] = x;
            }
        }
    }
    return 1;
}

// 방 꾸미기 : ColorScene 이 정한 물체 색 위에 벽지·창문·시계·액자·가구·마루를 입힌다
static void DrawRoom(char buf[][SCREEN_W + 1], WORD attr[][SCREEN_W], Game* g) {
    Map* m = &g->map;
    DecorRect rects[MAX_DECOR];
    int nRect = FindDecorRects(m, rects, MAX_DECOR);
    int clockTop = 0, clockBottom = -1, clockL[MAP_H], clockR[MAP_H];
    int hasClock = FindClock(m, &clockTop, &clockBottom, clockL, clockR);

    // 1) 창문 : 흰 창틀 + 하늘색 유리
    for (int i = 0; i < nRect; i++) {
        DecorRect* r = &rects[i];
        if (r->picture) continue;
        for (int y = r->y1; y <= r->y2; y++) {
            for (int x = r->x1; x <= r->x2; x++) {
                char c = m->tile[y][x];
                int frame = (c == '|' || c == '+' || c == '-');
                if (Untouched(buf, g, x, y)) {
                    buf[y + 1][x] = frame ? ((c == '-') ? '=' : ' ') : c;
                    attr[y + 1][x] = frame ? ATTR(PAL_WOOD_D, PAL_CREAM) : ATTR(PAL_CREAM, PAL_GLASS);
                }
                else {
                    attr[y + 1][x] = (WORD)((attr[y + 1][x] & 0x0F) | ((frame ? PAL_CREAM : PAL_GLASS) << 4));
                }
            }
        }
    }

    // 2) 액자 : 원목 틀 + 크림색 여백 + 그림 (하늘·해·들판)
    for (int i = 0; i < nRect; i++) {
        DecorRect* r = &rects[i];
        int sunRow = -1, sunL = 0, sunR = 0, landTop = r->y2;
        if (!r->picture) continue;

        for (int y = r->y1 + 2; y <= r->y2 - 2; y++) {   // 해 : '(' 와 ')' 가 있는 줄 / 들판 : 처음 '^' 가 나오는 줄부터
            int l = -1, rr = -1;
            for (int x = r->x1 + 4; x <= r->x2 - 4; x++) {
                if (m->tile[y][x] == '(') l = x;
                if (m->tile[y][x] == ')') rr = x;
                if (m->tile[y][x] == '^' && y < landTop) landTop = y;
            }
            if (l >= 0 && rr > l && sunRow < 0) { sunRow = y; sunL = l; sunR = rr; }
        }

        for (int y = r->y1; y <= r->y2; y++) {
            for (int x = r->x1; x <= r->x2; x++) {
                WORD bg;
                int keepChar = 0;
                if (y == r->y1 || y == r->y2 || x == r->x1 || x == r->x2) bg = PAL_WOOD;                   // 바깥 틀
                else if (y == r->y1 + 1 || y == r->y2 - 1 || x <= r->x1 + 3 || x >= r->x2 - 3) bg = PAL_CREAM; // 여백
                else if (sunRow >= 0 && ((y == sunRow && x >= sunL && x <= sunR) ||
                         ((y == sunRow - 1 || y == sunRow + 1) && x > sunL && x < sunR))) bg = PAL_SUN;        // 동그란 해
                else if (y < landTop) bg = PAL_GLASS;                                                          // 하늘
                else { bg = PAL_BOOK_G; keepChar = (m->tile[y][x] == '^'); }                                    // 들판과 산

                if (Untouched(buf, g, x, y)) {
                    buf[y + 1][x] = keepChar ? '^' : ' ';
                    attr[y + 1][x] = ATTR(PAL_WOOD_D, bg);
                }
                else {
                    attr[y + 1][x] = (WORD)((attr[y + 1][x] & 0x0F) | (bg << 4));
                }
            }
        }
    }

    // 3) 벽시계 : 원목 테두리 + 크림색 시계판 (숫자는 진하게, 바늘은 진한 갈색)
    if (hasClock) {
        for (int y = clockTop; y <= clockBottom; y++) {
            if (clockL[y] < 0) continue;
            for (int x = clockL[y]; x <= clockR[y]; x++) {
                char c = m->tile[y][x];
                int rim = (y == clockTop || y == clockBottom || x == clockL[y] || x == clockR[y] ||
                           c == '.' || c == '\'' || (c == '-' && (y <= clockTop + 1 || y >= clockBottom - 1)));
                WORD bg = rim ? PAL_WOOD : PAL_CREAM;

                if (Untouched(buf, g, x, y)) {
                    if (rim) buf[y + 1][x] = ' ';
                    attr[y + 1][x] = ATTR((c >= '0' && c <= '9') ? PAL_TEXT : PAL_WOOD_D, bg);
                }
                else {
                    attr[y + 1][x] = (WORD)((attr[y + 1][x] & 0x0F) | (bg << 4));
                }
            }
        }
    }

    // 4) 발판 : 긴 것(30칸 이상)은 원목 판, 짧은 것은 파스텔 책 (빨강·연두·보라를 번갈아)
    {
        const WORD books[3] = { PAL_BOOK_R, PAL_BOOK_G, PAL_BOOK_P };
        int bookIdx = 0;
        for (int y = 0; y < MAP_H; y++) {
            int x = 0;
            while (x < MAP_W) {
                if (m->tile[y][x] != '=') { x++; continue; }
                int s = x;
                while (x < MAP_W && m->tile[y][x] == '=') x++;
                int longBoard = (x - s >= 30);
                WORD bg = longBoard ? PAL_WOOD : books[bookIdx++ % 3];
                for (int i = s; i < x; i++) {
                    buf[y + 1][i] = longBoard ? '_' : (((i - s) % 3 == 0) ? '|' : ' ');   // 나뭇결 / 책과 책 사이
                    attr[y + 1][i] = ATTR(PAL_WOOD_D, bg);
                }
            }
        }
    }

    // 5) 바닥(맵 맨 아랫줄)은 원목 마루, 양옆 벽은 원목 기둥
    for (int x = 0; x < MAP_W; x++) {
        if (m->tile[MAP_H - 1][x] == '#') {
            buf[MAP_H][x] = (x % 7 == 0) ? '|' : '_';
            attr[MAP_H][x] = ATTR(PAL_WOOD_D, PAL_WOOD);
        }
    }
    for (int y = 0; y < MAP_H - 1; y++) {
        for (int x = 0; x < MAP_W; x += MAP_W - 1) {
            if (m->tile[y][x] == '#') {
                buf[y + 1][x] = ' ';
                attr[y + 1][x] = ATTR(PAL_WOOD_D, PAL_WOOD);
            }
        }
    }

    // 6) 약병(목표)은 빨갛게 강조
    if (g->map.goalY >= 0 && g->map.goalY < MAP_H && Untouched(buf, g, g->map.goalX, g->map.goalY)) {
        attr[g->map.goalY + 1][g->map.goalX] = ATTR(PAL_RED, PAL_WALL);
    }

    // 7) 벽지 : 남은 빈 벽에 옅은 꽃무늬 (작은 꽃 * 와 점 . ' 을 엇갈리게)
    for (int y = 0; y < MAP_H - 1; y++) {
        int sy = y + 1;
        for (int x = 1; x < MAP_W - 1; x++) {
            char c = ' ';
            if (buf[sy][x] != ' ' || (attr[sy][x] >> 4) != PAL_WALL || InDecorRect(rects, nRect, x, y)) continue;
            if (hasClock && y >= clockTop && y <= clockBottom && clockL[y] >= 0 && x >= clockL[y] && x <= clockR[y]) continue;
            if (sy % 4 == 1 && x % 8 == 2) c = '*';
            else if (sy % 4 == 3 && x % 8 == 6) c = '.';
            else if (sy % 4 == 3 && x % 8 == 2) c = '\'';
            if (c != ' ') {
                buf[sy][x] = c;
                attr[sy][x] = ATTR(PAL_PATTERN, PAL_WALL);
            }
        }
    }

    // 8) HUD : 진한 원목 바 + 크림색 글씨
    for (int x = 0; x < SCREEN_W; x++) {
        attr[0][x] = ATTR(PAL_CREAM, PAL_HUD);
    }

    // 9) 남은 시간이 5초 이하 : 화면 테두리(HUD 줄·양옆 기둥·바닥 마루)가 빨갛게 깜빡인다 (1초에 4번)
    if (g->frame + TIME_WARN_TICK >= TIME_LIMIT_TICK && (g->frame / 5) % 2 == 0) {
        for (int x = 0; x < SCREEN_W; x++) {
            attr[0][x] = ATTR(PAL_CREAM, PAL_RED);              // HUD 줄
            attr[MAP_H][x] = ATTR(PAL_CREAM, PAL_RED);          // 바닥 마루
        }
        for (int y = 1; y < MAP_H; y++) {
            attr[y][0] = ATTR(PAL_CREAM, PAL_RED);              // 왼쪽 기둥
            attr[y][MAP_W - 1] = ATTR(PAL_CREAM, PAL_RED);      // 오른쪽 기둥
        }
    }
}

void Render(Game* g) {

    char buf[SCREEN_H][SCREEN_W + 1];// 실제 콘솔에 출력하기 전 그림을 저장할 임시 화면 버퍼(각 행에 문자열 종료 문자 공간(+1)을 추가)
    WORD attr[SCREEN_H][SCREEN_W];   // 칸마다의 색

    ClearBuffer(buf); // 이전 프레임의 그림이 남지 않도록 버퍼 전체를 공백으로 초기화

    DrawMap(buf, g); // 맵의 벽, 바닥, 발판, 선풍기 등의 기본 타일 그리기

    DrawWind(buf, g); // 켜져 있는 선풍기의 바람을 맵 위에 그리기

    DrawObjects(buf, g); // 낙하물과 개미를 바람 및 맵 위에 그리기

    DrawPlayer(buf, g); // 플레이어의 머리와 몸을 그리기

    DrawHUD(buf, g); // 화면 맨 윗줄에 HP, 선풍기 상태, 경과 시간 표시

    ColorScene(attr, g); // 칸마다 색 정하기 (버튼 빨강, 바람 파랑)

    DrawRoom(buf, attr, g); // 방 꾸미기 (벽지·창문·시계·액자·가구·마루)

    FlushBufferColor(buf, attr); // 완성된 화면을 글자와 색으로 콘솔에 한 번에 출력
}
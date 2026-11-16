#include "game.h"

// 화면 버퍼를 공백으로 초기화한다.
void ClearBuffer(char buf[][SCREEN_W + 1]) {
    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            buf[y][x] = ' ';
        }
        buf[y][SCREEN_W] = '\0';
    }
}

// 고정된 맵 위치는 별도 범위 검사 없이 출력하고, 움직이는 물체의 범위 검사는 유지한다.

// 맵의 특정 좌표가 막혀 있는 칸인지 확인하는 함수(반환값은 0 = 이동 가능 / 1 = 이동 불가능)
int IsSolid(Map* m, int x, int y) {
    if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) {
        return 1;
    }

    char c = m->tile[y][x];
    if (c == '#') return 1;
    if (c == '=') return 1;
    if (c == 'F') return 1;
    if (c == 'B') return 1;

    return 0;
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
    return rand() % (b - a + 1) + a;
}


// 맵 파일을 읽어 게임 맵에 저장하는 함수
int LoadMap(Map* m, int stage) {
    (void)stage; // 현재는 1개 맵(map.txt)만 사용한다.

    if (m == NULL) {
        return 0;
    }

    FILE* fp = fopen("map.txt", "r");
    char line[MAP_W + 3]; //줄바꿈 기호 2개랑 널 문자 고려하여 +3

    // exe를 더블클릭하면 map.txt를 못 찾아서 exe 폴더부터 위로 올라가며 찾음
    if (fp == NULL) {
        wchar_t path[MAX_PATH];
        wchar_t file[MAX_PATH];
        DWORD len = GetModuleFileNameW(NULL, path, MAX_PATH);

        for (int up = 0; up < 4 && fp == NULL && len > 0; up++) {
            wchar_t* slash = wcsrchr(path, L'\\');
            if (slash == NULL) break;
            *slash = L'\0';

            swprintf(file, MAX_PATH, L"%ls\\map.txt", path);
            fp = _wfopen(file, L"r");
            if (fp == NULL) {
                swprintf(file, MAX_PATH, L"%ls\\Project1\\map.txt", path);
                fp = _wfopen(file, L"r");
            }
        }
    }

    if (fp == NULL) {
        return 0;
    }

    for (int y = 0; y < MAP_H; y++) {

        if (fgets(line, sizeof(line), fp) == NULL) {
            fclose(fp);
            return 0;
        }

        int length = (int)strcspn(line, "\r\n");
        for (int x = 0; x < MAP_W; x++) {
            if (x < length) {
                m->tile[y][x] = line[x];
            }
            else {
                m->tile[y][x] = ' ';
            }
        }
        m->tile[y][MAP_W] = '\0';
    }

    fclose(fp);
    return 1;
}

// 콘솔 창 크기에 맞춰 게임 화면을 가운데 정렬할 시작 좌표를 계산하는 함수
static void GetScreenOrigin(CONSOLE_SCREEN_BUFFER_INFO* info, int* left, int* top) {
    *left = 0;
    *top = 0;

    // 콘솔 창 가로가 게임 화면보다 넓을 때 남는 여백의 절반만큼 오른쪽으로 이동
    if (info->dwSize.X > SCREEN_W) {
        *left = (info->dwSize.X - SCREEN_W) / 2;
    }

    // 콘솔 창 세로가 게임 화면보다 높을 때 남는 여백의 절반만큼 아래로 이동
    if (info->dwSize.Y > SCREEN_H) {
        *top = (info->dwSize.Y - SCREEN_H) / 2;
    }
}

// 게임 화면 바깥 부분 지우기 (Alt+Enter로 크기 바뀌면 예전 글자가 남아서)
static void ClearOutsideScreen(HANDLE h, CONSOLE_SCREEN_BUFFER_INFO* info, int left, int top) {
    DWORD written;
    WORD gray = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    int width = info->dwSize.X;
    int rows = info->dwSize.Y;
    COORD pos;

    for (int y = 0; y < rows; y++) {
        pos.Y = (SHORT)y;
        if (y < top || y >= top + SCREEN_H) {
            pos.X = 0;
            FillConsoleOutputCharacterA(h, ' ', width, pos, &written);
            FillConsoleOutputAttribute(h, gray, width, pos, &written);
        }
        else {
            if (left > 0) {
                pos.X = 0;
                FillConsoleOutputCharacterA(h, ' ', left, pos, &written);
            }
            if (left + SCREEN_W < width) {
                pos.X = (SHORT)(left + SCREEN_W);
                FillConsoleOutputCharacterA(h, ' ', width - left - SCREEN_W, pos, &written);
            }
        }
    }
}

// 완성된 화면 버퍼(buf)를 콘솔 창 중앙에 한 줄씩 출력하는 함수
void FlushBuffer(char buf[][SCREEN_W + 1]) {

    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    int rows = SCREEN_H, width = SCREEN_W;      // 콘솔 화면 버퍼의 실제 줄수, 칸수
    int left = 0, top = 0;                      // 게임 화면 (0, 0) 이 출력될 콘솔 시작 좌표

    // 콘솔 창 크기를 확인해 화면이 정중앙에 오도록 여백 계산
    if (GetConsoleScreenBufferInfo(h, &info)) {
        rows = info.dwSize.Y;
        width = info.dwSize.X;
        GetScreenOrigin(&info, &left, &top);
        ClearOutsideScreen(h, &info, left, top);
    }

    for (int y = 0; y < SCREEN_H; y++) {

        gotoxy(left, top + y); // 위쪽 여백(top)만큼 내려간 실제 콘솔 줄(top + y)의 왼쪽 끝으로 이동

        // 콘솔 맨 마지막 줄의 맨 오른쪽 끝 칸까지 꽉 채워 출력할 때 화면이 자동 스크롤되는 현상 방지
        if (top + y == rows - 1 && left + SCREEN_W == width) {
            buf[y][SCREEN_W - 1] = '\0';
        }

        printf("%s", buf[y]); // 현재 행의 문자열 출력
    }

    fflush(stdout); // 버퍼에 대기 중인 내용을 지연 없이 즉시 콘솔 화면에 반영
}


// 맵의 기본 지형을 화면 버퍼(buf)에 밑그림으로 복사하는 단계
void DrawMap(char buf[][SCREEN_W + 1], Game* g) {
    int y, x;

    // 기본 맵 복사 (0번 줄은 상단 HUD이므로, y + 1 위치부터 복사)
    for (y = 0; y < MAP_H; y++) {
        for (x = 0; x < MAP_W; x++) {
            char c = g->map.tile[y][x];

            // 해당 칸이 선풍기와 버튼이라면 우선 공백 처리 후 아래 단계에서 그림을 그림
            if (c == 'F' || c == 'B') {
                buf[y + 1][x] = ' ';
            }
            else {
                buf[y + 1][x] = c; // 벽('#'), 발판('=') 등 일반 타일은 그대로 복사
            }
        }
    }

    // 2. 선풍기 그리기
    for (int i = 0; i < MAX_FANS; i++) {
        Fan* fan = &g->fans[i];
        const char* blade = "(-+-)";

        // 선풍기가 켜지면 날개 모양 변경
        if (fan->on && (g->frame / 2) % 2 == 1) {
            blade = "(-x-)";
        }

        // 선풍기 그림 저장
        const char* fanArt[FAN_H] = { blade, "  |  ", "[___]" };

        // 선풍기 그림의 시작 위치 계산
        int startX = fan->x - FAN_W / 2;
        int startY = fan->y - FAN_H + 1;

        // 선풍기 그림 출력
        for (int row = 0; row < FAN_H; row++) {
            for (int col = 0; col < FAN_W; col++) {
                int mapX = startX + col;
                int mapY = startY + row;

                // 맵 범위 안에 있는 문자만 화면에 표시
                if (mapX >= 0 && mapX < MAP_W && mapY >= 0 && mapY < MAP_H &&
                    fanArt[row][col] != ' ') {
                    buf[mapY + 1][mapX] = fanArt[row][col];
                }
            }
        }
    }

    // 버튼 그리기
    for (int i = 0; i < MAX_BUTTONS; i++) {
        Button* button = &g->buttons[i];

        char top;
        char bottom;

        // 버튼 상태에 따라 모양 변경
        if (button->pressed) {
            top = ' ';
            bottom = '_';
        }
        else {
            top = '_';
            bottom = '|';
        }

        // 버튼 좌표가 맵 안에 있을 때만 그림
        if (button->x >= 0 && button->x + 1 < MAP_W &&
            button->y >= 1 && button->y < MAP_H) {
            buf[button->y][button->x] = top;
            buf[button->y][button->x + 1] = top;

            buf[button->y + 1][button->x] = bottom;
            buf[button->y + 1][button->x + 1] = bottom;
        }
    }
}

// 선풍기 바람 그리기
void DrawWind(char buf[][SCREEN_W + 1], Game * g) {
    char wind[4] = { '|', '^', '|', ':' };  // 바람 모양 저장

    // 맵 전체 확인
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {

            // 바람이 있는 위치 확인
            if (g->wind[y][x] > 0) {

                // 출력할 바람 문자 선택
                int n = (y + x * 2 + g->frame) % 4;

                // 화면 버퍼에 바람 표시
                buf[y + 1][x] = wind[n];
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
            drawX += d->shakeX;
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
        const char** art;

        if (a->dir > 0) {
            art = right;
        }
        else {
            art = left;
        }

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


// 플레이어 그리기
void DrawPlayer(char buf[][SCREEN_W + 1], Game* g) {
    Player* p = &g->p;  // 플레이어 정보 가져오기
    const char* art[PLAYER_H] = { "(o)", "<|>", "| |" }; // 플레이어 그림 저장

    // 피격 후 무적 시간에는 깜빡이게 표시
    if (p->invTick > 0 && g->frame % 4 < 2) {
        return;
    }

    // 세로 방향으로 반복
    for (int i = 0; i < PLAYER_H; i++) {

        // 가로 방향으로 반복
        for (int j = 0; j < PLAYER_W; j++) {

            // 플레이어 그림의 출력 위치 계산
            int x = p->x - PLAYER_HALF + j;
            int y = p->y - PLAYER_H + 1 + i;

            // 맵 범위 안에 있는지 확인
            if (x >= 0 && x < MAP_W && y >= 0 && y < MAP_H) {

                // 공백이 아닌 문자만 화면에 출력
                if (art[i][j] != ' ') {
                    buf[y + 1][x] = art[i][j];
                }
            }
        }
    }
}

// 화면 맨 윗줄에 게임 상태(HUD)를 표시하는 함수(HP, 선풍기 ON/OFF 상태, 게임 경과 시간을 출력)
void DrawHUD(char buf[][SCREEN_W + 1], Game* g) {
    char str[SCREEN_W + 1];
    const char* fan1 = "OFF";
    const char* fan2 = "OFF";
    unsigned int seconds = g->frame / 20;
    unsigned int time = 0;

    if (g->fans[0].on) fan1 = "ON";
    if (g->fans[1].on) fan2 = "ON";
    if (seconds < TIME_LIMIT_SEC) time = TIME_LIMIT_SEC - seconds;

    snprintf(str, sizeof(str),
        "HP: %d/%d | FAN1: %s | FAN2: %s | TIME LEFT: %u sec",
        g->p.hp, PLAYER_MAX_HP, fan1, fan2, (unsigned int)time);

    // 0번 줄에 상태 표시
    for (int i = 0; str[i] != '\0'; i++) {
        buf[0][i] = str[i];
    }
}


// 한 프레임의 게임 화면을 완성하고 콘솔에 출력하는 함수(맵 → 바람 → 낙하물·개미 → 플레이어 → HUD 순서로 그림)
/////  색깔 //////
#define COLOR_DEFAULT (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)   // 기본 회색
#define COLOR_BUTTON  (FOREGROUND_RED | FOREGROUND_INTENSITY)                 // 버튼 : 빨간색
#define COLOR_WIND    (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define COLOR_ROCK    (FOREGROUND_RED | FOREGROUND_GREEN)
#define COLOR_ANT     (FOREGROUND_INTENSITY)
#define COLOR_DECOR   (FOREGROUND_BLUE)

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

        if (b->x < 0 || b->x + BUTTON_W > MAP_W || b->y < 1 || b->y >= MAP_H) {
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
        int cx = d->x;
        if (d->state == DROP_SHAKE) cx += d->shakeX;

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

// 문자와 색상 정보를 콘솔에 출력하는 함수
static void FlushBufferColor(char buf[][SCREEN_W + 1], WORD attr[][SCREEN_W]) {

    CHAR_INFO cells[SCREEN_H * SCREEN_W];  // 출력할 문자와 색상 저장
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);  // 콘솔 핸들 가져오기
    CONSOLE_SCREEN_BUFFER_INFO info;
    int left = 0, top = 0;

    COORD size = { SCREEN_W, SCREEN_H };  // 화면 크기
    COORD from = { 0, 0 };               // 출력 시작 위치
    SMALL_RECT area;                     // 실제 출력 영역

    // 콘솔 크기를 확인하고 가운데 위치 계산
    if (GetConsoleScreenBufferInfo(h, &info)) {
        GetScreenOrigin(&info, &left, &top);
        ClearOutsideScreen(h, &info, left, top);
    }

    // 문자와 색상 정보를 합치기
    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            cells[y * SCREEN_W + x].Char.AsciiChar = buf[y][x];
            cells[y * SCREEN_W + x].Attributes = attr[y][x];
        }
    }

    // 화면 출력 영역 설정
    area.Left = (SHORT)left;
    area.Top = (SHORT)top;
    area.Right = (SHORT)(left + SCREEN_W - 1);
    area.Bottom = (SHORT)(top + SCREEN_H - 1);

    // 콘솔 화면에 최종 출력
    WriteConsoleOutputA(h, cells, size, from, &area);
}

/////  방 꾸미기 (벽지·창문·시계·액자·가구·마루) //////
// 장식 위치는 숫자로 박지 않고 맵에서 찾는다. 그래서 map.txt 에서 장식을 옮겨도 색이 따라간다
// 움직이는 물체(돌·개미·플레이어·바람)가 장식 앞을 지나가면 물체는 그대로 두고 뒤 배경색만 이어지게 한다

typedef struct DecorRect {
    int x1, y1, x2, y2;   // 맵 좌표 (테두리 포함)
    int picture;
} DecorRect;

#define MAX_DECOR 8

static int Untouched(char buf[][SCREEN_W + 1], Game* g, int x, int my) {
    if (x < 0 || x >= MAP_W || my < 0 || my >= MAP_H) {
        return 0;
    }
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

            for (int i = 0; i < n; i++) {
                if (x >= out[i].x1 && x <= out[i].x2 && y >= out[i].y1 && y <= out[i].y2) {
                    inside = 1;
                }
            }
            if (inside == 1 || n >= maxCount) continue;

            int x2 = x;
            int y2 = y;
            for (int i = x + 1; i < MAP_W; i++) {
                if (m->tile[y][i] != '-' && m->tile[y][i] != '+') break;
                if (m->tile[y][i] == '+') x2 = i;
            }
            for (int j = y + 1; j < MAP_H; j++) {
                if (m->tile[j][x] != '|' && m->tile[j][x] != '+') break;
                if (m->tile[j][x] == '+') y2 = j;
            }
            if (x2 == x || y2 == y || m->tile[y2][x2] != '+') continue;

            out[n].x1 = x;
            out[n].y1 = y;
            out[n].x2 = x2;
            out[n].y2 = y2;
            out[n].picture = 0;

            // ^가 들어있으면 창문이 아니라 액자
            for (int j = y; j <= y2; j++) {
                for (int i = x; i <= x2; i++) {
                    if (m->tile[j][i] == '^') out[n].picture = 1;
                }
            }
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
    int cx = -1;
    int cy = -1;

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W - 1; x++) {
            if (m->tile[y][x] == '1' && m->tile[y][x + 1] == '2') {
                cx = x;
                cy = y;
                break;
            }
        }
        if (cx >= 0) break;
    }
    if (cx == -1) return 0;

    *top = cy - 1;
    *bottom = cy;
    for (int y = cy + 1; y < MAP_H; y++) {
        for (int x = cx - 2; x <= cx + 3; x++) {
            if (x >= 0 && x < MAP_W && m->tile[y][x] == '6') {
                *bottom = y + 1;
            }
        }
    }
    if (*top < 0 || *bottom >= MAP_H) return 0;

    for (int y = *top; y <= *bottom; y++) {
        left[y] = -1;
        right[y] = -1;
        for (int x = cx - 16; x <= cx + 17; x++) {
            if (x < 1 || x >= MAP_W - 1) continue;
            char c = m->tile[y][x];
            if (c != ' ' && c != '#' && c != '=' && c != 'F' && c != 'B' && c != '*') {
                if (left[y] == -1) left[y] = x;
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

    // 창문 색
    for (int i = 0; i < nRect; i++) {
        DecorRect* r = &rects[i];
        if (r->picture) continue;
        for (int y = r->y1; y <= r->y2; y++) {
            for (int x = r->x1; x <= r->x2; x++) {
                char c = m->tile[y][x];
                int frame = (c == '|' || c == '+' || c == '-');
                if (Untouched(buf, g, x, y)) {
                    if (frame == 1) {
                        if (c == '-') buf[y + 1][x] = '=';
                        else buf[y + 1][x] = ' ';
                        attr[y + 1][x] = ATTR(PAL_WOOD_D, PAL_CREAM);
                    }
                    else {
                        buf[y + 1][x] = c;
                        attr[y + 1][x] = ATTR(PAL_CREAM, PAL_GLASS);
                    }
                }
                else {
                    WORD bg = PAL_GLASS;
                    if (frame) bg = PAL_CREAM;
                    attr[y + 1][x] = (WORD)((attr[y + 1][x] & 0x0F) | (bg << 4));
                }
            }
        }
    }

    // 액자
    for (int i = 0; i < nRect; i++) {
        DecorRect* r = &rects[i];
        int sunRow = -1, sunL = 0, sunR = 0, landTop = r->y2;
        if (!r->picture) continue;

        for (int y = r->y1 + 2; y <= r->y2 - 2; y++) {
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
                    if (keepChar == 1) buf[y + 1][x] = '^';
                    else buf[y + 1][x] = ' ';
                    attr[y + 1][x] = ATTR(PAL_WOOD_D, bg);
                }
                else {
                    attr[y + 1][x] = (WORD)((attr[y + 1][x] & 0x0F) | (bg << 4));
                }
            }
        }
    }

    // 시계
    if (hasClock) {
        for (int y = clockTop; y <= clockBottom; y++) {
            if (clockL[y] < 0) continue;
            for (int x = clockL[y]; x <= clockR[y]; x++) {
                char c = m->tile[y][x];
                int rim = (y == clockTop || y == clockBottom || x == clockL[y] || x == clockR[y] ||
                           c == '.' || c == '\'' || (c == '-' && (y <= clockTop + 1 || y >= clockBottom - 1)));
                WORD bg = PAL_CREAM;
                if (rim) bg = PAL_WOOD;

                if (Untouched(buf, g, x, y)) {
                    if (rim) buf[y + 1][x] = ' ';
                    if (c >= '0' && c <= '9') attr[y + 1][x] = ATTR(PAL_TEXT, bg);
                    else attr[y + 1][x] = ATTR(PAL_WOOD_D, bg);
                }
                else {
                    attr[y + 1][x] = (WORD)((attr[y + 1][x] & 0x0F) | (bg << 4));
                }
            }
        }
    }

    // 발판은 크기에 따라 다른 색
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
                WORD bg;
                if (longBoard) bg = PAL_WOOD;
                else {
                    bg = books[bookIdx % 3];
                    bookIdx++;
                }
                for (int i = s; i < x; i++) {
                    if (longBoard) buf[y + 1][i] = '_';
                    else if ((i - s) % 3 == 0) buf[y + 1][i] = '|';
                    else buf[y + 1][i] = ' ';
                    attr[y + 1][i] = ATTR(PAL_WOOD_D, bg);
                }
            }
        }
    }

    // 바닥, 벽
    for (int x = 0; x < MAP_W; x++) {
        if (m->tile[MAP_H - 1][x] == '#') {
            if (x % 7 == 0) buf[MAP_H][x] = '|';
            else buf[MAP_H][x] = '_';
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

    // 약병 색
    if (g->map.goalX >= 0 && g->map.goalX < MAP_W &&
        g->map.goalY >= 0 && g->map.goalY < MAP_H &&
        Untouched(buf, g, g->map.goalX, g->map.goalY)) {
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

    // 위에 표시하는 글씨
    for (int x = 0; x < SCREEN_W; x++) {
        attr[0][x] = ATTR(PAL_CREAM, PAL_HUD);
    }

    // 제한 시간이 얼마 남지 않으면 테두리를 빨간색으로 깜빡이게 함
    if (g->frame + TIME_WARN_TICK >= TIME_LIMIT_TICK &&
        (g->frame / 5) % 2 == 0) {

        // 화면 위쪽과 아래쪽 색상 변경
        for (int x = 0; x < SCREEN_W; x++) {
            attr[0][x] = ATTR(PAL_CREAM, PAL_RED);
            attr[MAP_H][x] = ATTR(PAL_CREAM, PAL_RED);
        }

        // 화면 왼쪽과 오른쪽 색상 변경
        for (int y = 1; y < MAP_H; y++) {
            attr[y][0] = ATTR(PAL_CREAM, PAL_RED);
            attr[y][MAP_W - 1] = ATTR(PAL_CREAM, PAL_RED);
        }
    }
}

void Render(Game* g) {
    char buf[SCREEN_H][SCREEN_W + 1];
    WORD attr[SCREEN_H][SCREEN_W];

    ClearBuffer(buf);
    DrawMap(buf, g);
    DrawWind(buf, g);
    DrawObjects(buf, g);
    DrawPlayer(buf, g);
    DrawHUD(buf, g);

    ColorScene(attr, g);
    DrawRoom(buf, attr, g);
    FlushBufferColor(buf, attr);
}

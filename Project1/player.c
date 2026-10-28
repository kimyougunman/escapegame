#include "game.h"

void ProcessInput(Game* g) {
	int key;
	g->inDx = 0;
	g->inJump = 0;

	if (_kbhit()) {
		key = _getch();
		if (key == 224) {
			key = _getch();
		}

		switch (key) {
		case 75: //왼쪽
			g->inDx = -1;
			break;
		case 77: //오른쪽
			g->inDx = 1;
			break;
		case 32: //스페이스바
			g->inJump = 1;
			break;
		case 81: //q
			g->state = ST_EXIT;
			break;
		default:
			break;
		}

	}

}

void ApplyGravity(Player* p) {
	p->vy = p->vy + GRAVITY;

}


// 점프 횟수가 MAX_JUMPS(2) 보다 적을 때만 점프한다. 땅에서 1번 + 공중에서 1번 = 2단 점프
void TryJump(Player* p) {
	if (p->jumpCount < MAX_JUMPS) {
		p->vy = JUMP_POWER;   // 떨어지는 중에도 같은 높이로 뛰도록 속도를 새로 넣는다
		p->jumpCount++;
		p->onGround = 0;
	}

}

/////  플레이어 몸(PLAYER_W x PLAYER_H = 3 x 3) 판정 //////
//
//   (o)   <- 머리  y - 2
//   <|>   <- 몸    y - 1
//   | |   <- 발    y        (p.x, p.y) = 발 가운데 칸
//
// 몸이 차지하는 칸 : 가로 x - PLAYER_HALF ~ x + PLAYER_HALF, 세로 y - (PLAYER_H - 1) ~ y

// 가로 cx - 1 ~ cx + 1 의 한 줄(row)에 막힌 칸이 있으면 1 (발 아래·머리 위 검사용)
static int IsRowBlocked(Map* m, int cx, int row) {
	for (int x = cx - PLAYER_HALF; x <= cx + PLAYER_HALF; x++) {
		if (IsSolid(m, x, row) == 1) {
			return 1;
		}
	}
	return 0;
}

// 발 가운데를 (cx, footY) 에 두었을 때 몸 3 x 3 중 막힌 칸과 겹치면 1
static int IsBodyBlocked(Map* m, int cx, int footY) {
	for (int y = footY - (PLAYER_H - 1); y <= footY; y++) {
		if (IsRowBlocked(m, cx, y) == 1) {
			return 1;
		}
	}
	return 0;
}

// 사각형 (ax1,ay1)-(ax2,ay2) 와 플레이어 몸 3 x 3 이 한 칸이라도 겹치면 1 (돌·개미 충돌용)
static int OverlapsBody(Player* p, int ax1, int ay1, int ax2, int ay2) {
	return ax1 <= p->x + PLAYER_HALF && ax2 >= p->x - PLAYER_HALF &&
		ay1 <= p->y && ay2 >= p->y - (PLAYER_H - 1);
}

// 가로로 한 칸 이동을 시도한다. 옮긴 자리에서 몸 3 x 3 이 막힌 칸과 겹치면 취소
void MovePlayerX(Game* g, int dx) {
	int nx;
	nx = g->p.x + dx;
	if (IsBodyBlocked(&g->map, nx, g->p.y) == 0) {
		g->p.x = nx;
		g->p.face = dx;
	}

}

void UpdatePlayer(Game *g) {
	if (g->inDx != 0) {
		MovePlayerX(g, g->inDx);
	}

	if (g->inJump == 1) {
		TryJump(&g->p);   // 몇 번째 점프인지는 TryJump 가 판단
	}


	ApplyGravity(&g->p);

	ApplyFanWind(g);

	//  ⑤ MovePlayerY() 로 속도를 -MAX_RISE ~ MAX_FALL 로 제한하고 한 칸씩 세로 이동하며 착지·천장을 판정한다.
	MovePlayerY(g);
}





void MovePlayerY(Game* g) {
	int steps;
	int i;

	g->p.onGround = 0; // 일단 공중으로 본다. 아래가 막혀서 멈추면 다시 1 (공중 무한 점프 방지)

	if (g->p.vy < -MAX_RISE) {
		g->p.vy = -MAX_RISE;
	}

	if (g->p.vy > MAX_FALL) {
		g->p.vy = MAX_FALL;
	}

	steps = abs(g->p.vy);

	for (i = 1;i <= steps;i++) {
		if (g->p.vy < 0) { //올라가는중 : 머리 바로 위 줄(y - PLAYER_H) 3칸 검사
			if (IsRowBlocked(&g->map, g->p.x, g->p.y - PLAYER_H) == 0) {
				g->p.y--;
			}
			else { //막혀있으면
				g->p.vy = 0;
				break;
			}

		}

		else if (g->p.vy > 0) { //내려가는 중 : 발 바로 아래 줄(y + 1) 3칸 검사
			if (IsRowBlocked(&g->map, g->p.x, g->p.y + 1) == 0) {
				g->p.y++;
			}
			else { //막혀있으면
				g->p.vy = 0;
				g->p.onGround = 1;
				g->p.jumpCount = 0; // 착지하면 점프 횟수 초기화
				break;
			}
		}

	}

}
void DamagePlayer(Game* g) {
	g->p.invTick = INV_TICK;

	g->p.hp -= 1;
	if (IsBodyBlocked(&g->map, g->p.x + g->kdir, g->p.y) == 0) { // 밀려날 자리에 몸이 들어가면 넉백
		g->p.x = g->p.x + g->kdir;
		g->p.vy = KNOCK_VY;
		g->p.onGround = 0;
	}

}

// 낙하물·개미가 플레이어 몸(3 x 3)과 겹쳤는지 모두 확인한 뒤, 맞았으면 한 번만 피해를 준다 [그림 12]
void CheckCollisions(Game* g) {
	int i;

	if (g->p.invTick > 0) { // ① 무적 중이면 판정을 건너뛴다
		return;
	}
	g->hit = 0;            // ② 초기화

	// ③ 떨어지는 돌(5 x 3) : 이번 프레임에 지나온 구간(prevY 때 윗줄 ~ 지금 아랫줄)이 몸과 겹치면 맞음 (빠르게 지나쳐도 놓치지 않게)
	for (i = 0;i < MAX_DROPS;i++) {
		Drop* d = &g->drops[i];

		if (d->active == 1 && d->state == DROP_FALL &&
			OverlapsBody(&g->p, d->x - DROP_W / 2, d->prevY - (DROP_H - 1), d->x + DROP_W / 2, d->y) == 1) {
			g->hit = 1;
			g->kdir = -g->p.face;     // 낙하물이면 바라보는 반대쪽으로
			d->active = 0;           // 맞은 낙하물은 사라진다
		}
	}

	// ④ 개미 : 그림은 7 x 3 이지만 히트박스는 5 x 2 (양 끝 1칸, 더듬이 줄 제외). 몸과 겹치면 맞고 개미 반대쪽으로 밀려난다
	for (i = 0;i < MAX_ANTS;i++) {
		Ant* a = &g->ants[i];

		if (a->alive == 1 && OverlapsBody(&g->p, a->x + ANT_HIT_INSET, a->y - (ANT_HIT_H - 1), a->x + ANT_W - 1 - ANT_HIT_INSET, a->y) == 1) {
			int antCenter = a->x + ANT_W / 2;   // 개미 가운데 x

			g->hit = 1;
			if (g->p.x > antCenter) {
				g->kdir = 1;
			}
			else if (g->p.x < antCenter) {
				g->kdir = -1;
			}
			else {
				g->kdir = -g->p.face;
			}
		}
	}

	// ⑤ 둘 다 맞아도 HP 는 1 만 줄어든다
	if (g->hit == 1) {
		DamagePlayer(g);
	}

}

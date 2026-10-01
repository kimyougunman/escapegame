#include "game.h"

void ProcessInput(Game *g) {
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
		case 113: //q
			g->state = ST_EXIT;
			break;
		default:
			break;
		}

	}

}

void ApplyGravity(Player *p) {
	p->vy = p->vy + GRAVITY;

}


void TryJump(Player *p) {
	if (p->onGround == 1) {
		p->vy = p->vy + JUMP_POWER;
	}
	else;

}

void MovePlayerX(Game* g, int dx) {
	int nx;
	nx = g->p.x + dx;
	if (IsSolid(&g->map, nx, g->p.y) == 0 && IsSolid(&g->map,nx,g->p.y-1)) {
		g->p.x = nx;
		g->p.face = dx;
	}

}

void UpdatePlayer(Game *g) {
	if (g->inDx != 0) {
		MovePlayerX(g,g->inDx);
	}

	if (g->inJump == 1 && g->p.onGround == 1) {
		TryJump(&g->p);
	}


	ApplyGravity(&g->p);

	ApplyFanWind(g);

//  ⑤ MovePlayerY() 로 속도를 -MAX_RISE ~ MAX_FALL 로 제한하고 한 칸씩 세로 이동하며 착지·천장을 판정한다.
	MovePlayerY(g);
}





void MovePlayerY(Game *g) {
	int steps;
	int i;
	if (g->p.vy > MAX_RISE) {
		g->p.vy = MAX_RISE;
	}

	if (g->p.vy > MAX_FALL) {
		g->p.vy = MAX_FALL;
	}
	
	steps = abs(g->p.vy);

	for (i = 1;i <= steps;i++) {
//IsSolid(Map *m, int x, int y)
		if (IsSolid(&g->map, g->p.x, g->p.y) == 0) {
			if (g->p.vy > 0) g->p.y++;

			else if (g->p.vy < 0) g->p.y--;
		}

		else if (IsSolid(&g->map, g->p.x, g->p.y) == 1) { //막혀있다면
			if (g->p.vy > 0) { //내려가는중이면
				g->p.onGround = 1;
			}
			else if (g->p.vy < 0) { //올라가는중이면
				g->p.vy = 0;

			}
		}

	}


}
void DamagePlayer(Game* g) {
	g->p.invTick = INV_TICK;

	g->p.hp -= 1;
	if (IsSolid(&g->map, g->p.x + g->kdir, g->p.y)==0) {//
		g->p.x = g->p.x + g->kdir;
		g->p.vy = -2;
		g->p.onGround = 0;
	}

}

void CheckCollisions(Game *g) {
	int i;
	int prevy, y;
	g->hit = 0;
	for (i = 0;i < MAX_DROPS;i++) {
		if (g->drops[i].active == 1) {
			if (g->drops[i].x == g->p.x) { //낙하물 x와 플레이어 x가 같을 경우
				prevy = g->drops[i].prevY;
				y = g->drops[i].y;
				while (prevy == y) {
					if (prevy == g->p.y || prevy == g->p.y - 1) {
						g->hit = 1;
						g->kdir = g->p.face * -1;
						DamagePlayer(g);
						break;
					}
					prevy++;


				}
			}
		}
	}

	for (i = 0;i < MAX_ANTS;i++) {
		if (g->ants[i].alive == 1) { //개미가 살아있으면
			if (g->ants[i].x == g->p.x && (g->ants[i].y == g->p.y || g->ants[i].y == g->p.y-1)) {
				g->hit = 1;
				g->kdir = g->p.face * -1;
				
			}
		}
	}




}
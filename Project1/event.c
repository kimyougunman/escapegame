#include "game.h"

void UpdateAnts(Game* g) {

	int i;
	int tempX;
	for (i = 0;i < MAX_ANTS;i++) {//개미 마릿수 만큼

		if (g->ants[i].alive  == 0) {  //죽은 개미는 건더 뜀
			continue;
		}

		if (g->ants[i].moveTick > 0) { // 4프레임 마다 움직여서 0보다 크면 패쓰
			g->ants[i].moveTick--;
			continue;
		}

		g->ants[i].moveTick = ANT_TERM;
		tempX = g->ants[i].x + g->ants[i].dir * ANT_STEP;

		if (tempX > g->ants[i].right || tempX < g->ants[i].left || IsSolid(&g -> map,tempX, g->ants[i].y) == 1) {
			g->ants[i].dir = -g->ants[i].dir;
		}
		else {
			g->ants[i].x = tempX;
		}
	}
}

void UpdateButtons(Game* g) {
	int i;
	Button button;
	Fan fan;

	for (i = 0;i < MAX_BUTTONS;i++) {
		button = g->buttons[i];
		fan = g->fans[i];

		if (g->p.x == button.x && g->p.y == button.y) {
			if (button.pressed == 0) {
				if (fan.on == 0) { // 선풍기 꺼져있을때 밟았을 경우
					fan.on = 1;
				}
				else { // 선풍기 켜져있을때 밟았을 경우
					fan.on = 0;
				}
				button.pressed = 1;
			}
		}
		else {
			button.pressed = 0;
		}

		g->buttons[i] = button;
		g->fans[i] = fan;
	}
}

void UpdateFans(Game* g) {
	Fan fan;
	int i,y;
	memset(g->wind, 0, sizeof(g->wind)); //바람 칸 전부 0

	for (i = 0;i < MAX_FANS;i++) {
		fan = g->fans[i];
		if (fan.on == 0) {
			continue;
		}
		else {
			for (y = 1;y <= fan.range;y++) { // 바람이 도달하는 거리

				if (fan.x >= MAP_W || fan.x < 0 || fan.y - y < 0) {	 // 바람이 맵 안인지 확인
					break;
				}
				else {
					if (IsSolid(&g -> map, fan.x, fan.y - y) == 1) { // 바람 생성칸 막혀있으면 break
						break;
					}
					else { // 안막혔으면 바람 저장
						g->wind[fan.y - y][fan.x] = 5;
					}
				}
			}

		}
	}
}

void ApplyFanWind(Game *g) {

	if (g->wind[g->p.y][g->p.x] == 5) {
		g->p.vy -= FAN_POWER;
		if (g->p.vy <-MAX_RISE) {
			g->p.vy = -MAX_RISE;
		}
	}
}

int SpawnDrop(Game* g) {
	int i;

	for (i = 0;i < MAX_DROPS;i++) {
		if (g->drops[i].active == 0) { //낙하물이 만들어질 자리가 비워져있으면
			g->drops[i].x = rand() % (MAP_W-2) + 1 ;// 맵 x좌표에 랜덤으로 생성
			g->drops[i].y = 1;// y좌표는 천장 아래로 고정
			g->drops[i].prevY = g->drops[i].y; //
			g->drops[i].state = DROP_SHAKE;//상태 저장
			g->drops[i].tick = SHAKE_TICK;// 틱 초기화
			g->drops[i].active = 1;
			g->drops[i].shakeX = 0;

			return i;
		}

	}
	return -1;
}

void UpdateDrops(Game* g) {
	int i;

	if (g->frame % SPAWN_TERM == 0) {
		if (SPAWN_RATE >rand()%100) {
			SpawnDrop(g);
		}
	}

	for (i = 0;i < MAX_DROPS;i++) {

		if (g->drops[i].active == 0) {
			continue;
		}
		else {
			g->drops[i].prevY = g->drops[i].y;
			if (g->drops[i].state == DROP_SHAKE) {
				g->drops[i].tick--;
				if (g->drops[i].tick % 2 == 1) {
					g->drops[i].shakeX = 1;
				}
				else {
					g->drops[i].shakeX = -1;
				}

				if (g->drops[i].tick == 0) {
					g->drops[i].state = DROP_FALL;
					g->drops[i].shakeX = 0;
				}
			}
			else if (g->drops[i].state == DROP_FALL) {
				if (IsSolid(&g->map, g->drops[i].x, g->drops[i].y + 1) != 1) {
					g->drops[i].y++;
				}
				else {
					g->drops[i].state = DROP_LAND;
					g->drops[i].tick = LAND_TICK;
				}
			}
			else if (g->drops[i].state == DROP_LAND) {
				g->drops[i].tick--;
				if (g->drops[i].tick == 0) {
					g->drops[i].active = 0;
				}
			}
		}
	}
}


void ShowIntro(void){
	
}

int ShowEnding(Game* g){

	return 0;
}
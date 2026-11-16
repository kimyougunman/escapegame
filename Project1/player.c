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


void TryJump(Player* p) {
   if (p->jumpCount < MAX_JUMPS) {
      p->vy = JUMP_POWER;   // 떨어지는 중에도 같은 높이로 뛰도록 속도를 새로 넣는다
      p->jumpCount++;
      p->onGround = 0;
   }

}

void MovePlayerX(Game* g, int dx) {
   int nx;
   nx = g->p.x + dx;
   if (IsSolid(&g->map, nx, g->p.y) == 0 && IsSolid(&g->map, nx, g->p.y - 1) == 0) {
      g->p.x = nx;
      g->p.face = dx;
   }

}

void UpdatePlayer(Game* g) {
   if (g->inDx != 0) {
      MovePlayerX(g, g->inDx);
   }

   if (g->inJump == 1) {
      TryJump(&g->p);
   }
   if (g->p.onGround == 1) {
      g->p.jumpCount = 0;
   }



   ApplyGravity(&g->p);

   ApplyFanWind(g);

   //  ⑤ MovePlayerY() 로 속도를 -MAX_RISE ~ MAX_FALL 로 제한하고 한 칸씩 세로 이동하며 착지·천장을 판정한다.
   MovePlayerY(g);
}





void MovePlayerY(Game* g) {
   int steps;
   int i;
   g->p.onGround = 0; // 일단 공중으로 보고 착지하면 다시 1 (발판에서 걸어 내려갈 때 무한 점프 방지)
   if (g->p.vy < -MAX_RISE) {
      g->p.vy = -MAX_RISE;
   }

   if (g->p.vy > MAX_FALL) {
      g->p.vy = MAX_FALL;
   }

   steps = abs(g->p.vy);

   for (i = 1;i <= steps;i++) {
      //IsSolid(Map *m, int x, int y)
      if (g->p.vy < 0) { //올라가는중
         if (IsSolid(&g->map, g->p.x, g->p.y - 3) == 0) {
            g->p.y--;
         }
         else { //막혀있으면
            g->p.vy = 0;
            break;
         }

      }

      else if (g->p.vy > 0) { //내려가는 중
         if (IsSolid(&g->map, g->p.x, g->p.y + 1) == 0) {
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

   if (g->p.invTick > 0) {
      return;
   }

   g->p.invTick = INV_TICK;

   g->p.hp -= 1;
   if (IsSolid(&g->map, g->p.x + g->kdir, g->p.y) == 0) {//
      g->p.x = g->p.x + g->kdir;
      g->p.vy = -2;
      g->p.onGround = 0;
   }

}

void CheckCollisions(Game* g) {
   int i;
   int prevy, y;
   g->hit = 0;
   for (i = 0;i < MAX_DROPS;i++) {
      if (g->drops[i].active == 1) {
         if (abs(g->drops[i].x - g->p.x) <= 3) {
            prevy = g->drops[i].prevY;
            y = g->drops[i].y;
            while (prevy <= y) {
               if (prevy == g->p.y || prevy == g->p.y - 1 || prevy == g->p.y-2) {
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
         if ((g->p.x + 1 >= g->ants[i].x + 1) &&
            (g->p.x - 1 <= g->ants[i].x + 5) &&
            (g->p.y >= g->ants[i].y - 1) &&
            (g->p.y - 2 <= g->ants[i].y))
         {
            g->hit = 1;
            g->kdir = g->p.face * -1;
            DamagePlayer(g);
         }
      }
   }

}

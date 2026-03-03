#include "sim.h"
#include "tiles.h"
#include "notify.h"
#include "sprite.h"
#include <stdlib.h>
#include <windows.h>

extern HWND hwndMain;
extern int addGameLog(const char *format, ...);
extern int addDebugLog(const char *format, ...);

short CrashX, CrashY;

extern int FireBomb();

int DropFireBombs() {
    int i;
    for (i = 0; i < 40; i++)
        FireBomb();
    return 0;
}

int MakeExplosion(int x, int y) {
    int dir, tx, ty;
    static const short xd[4] = {0, 1, 0, -1};
    static const short yd[4] = {-1, 0, 1, 0};

    if (!BOUNDS_CHECK(x, y)) return 0;

    Map[x][y] = FIRE + (rand() & 7) + ANIMBIT;

    for (dir = 0; dir < 4; dir++) {
        tx = x + xd[dir];
        ty = y + yd[dir];
        if (!BOUNDS_CHECK(tx, ty)) continue;
        if (Map[tx][ty] & ZONEBIT) continue;
        Map[tx][ty] = FIRE + (rand() & 7) + ANIMBIT;
    }
    return 0;
}

int DoMeltdown(int sx, int sy) {
    int x, y, z, t;

    MakeExplosion(sx - 1, sy - 1);
    MakeExplosion(sx - 1, sy + 2);
    MakeExplosion(sx + 2, sy - 1);
    MakeExplosion(sx + 2, sy + 2);

    for (x = sx - 1; x < sx + 3; x++) {
        for (y = sy - 1; y < sy + 3; y++) {
            if (BOUNDS_CHECK(x, y))
                Map[x][y] = FIRE + (rand() & 3) + ANIMBIT;
        }
    }

    for (z = 0; z < 200; z++) {
        x = sx - 20 + SimRandom(41);
        y = sy - 15 + SimRandom(31);
        if (!BOUNDS_CHECK(x, y)) continue;
        t = Map[x][y];
        if (t & ZONEBIT) continue;
        if ((t & BURNBIT) || (t == 0))
            Map[x][y] = RADTILE;
    }

    ShowNotificationAt(NOTIF_NUCLEAR_MELTDOWN, sx, sy);
    InvalidateRect(hwndMain, NULL, FALSE);
    return 0;
}

int MakeMonster() {
    int x, y, z, done;
    SimSprite *monster;
    extern short PolMaxX, PolMaxY;

    monster = GetSprite(SPRITE_MONSTER);
    if (monster) {
        monster->count = 1000;
        monster->dest_x = PolMaxX << 4;
        monster->dest_y = PolMaxY << 4;
        return 0;
    }

    done = 0;
    for (z = 0; z < 300; z++) {
        x = SimRandom(WORLD_X - 20) + 10;
        y = SimRandom(WORLD_Y - 10) + 5;
        if ((Map[x][y] & LOMASK) == RIVER) {
            done = 1;
            break;
        }
    }
    if (!done) {
        x = WORLD_X / 2;
        y = WORLD_Y / 2;
    }

    monster = NewSprite(SPRITE_MONSTER, x << 4, y << 4);
    if (monster) {
        monster->dest_x = PolMaxX << 4;
        monster->dest_y = PolMaxY << 4;
    }

    ShowNotification(NOTIF_MONSTER_SIGHTED);
    InvalidateRect(hwndMain, NULL, FALSE);
    return 0;
}

int MakeTornado() {
    int x, y;

    if (GetSprite(SPRITE_TORNADO))
        return 0;

    x = SimRandom(WORLD_X - 20) + 10;
    y = SimRandom(WORLD_Y - 10) + 5;

    NewSprite(SPRITE_TORNADO, (x << 4) + 8, (y << 4) + 8);

    ShowNotificationAt(NOTIF_TORNADO, x, y);
    InvalidateRect(hwndMain, NULL, FALSE);
    return 0;
}

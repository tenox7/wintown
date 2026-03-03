/* sprite.c - Transportation sprite system for WiNTown
 * Based on original WiNTown w_sprite.c
 * Implements trains, ships, ferries, airplanes, and helicopters
 */

#include "sprite.h"
#include "sim.h"
#include <stdlib.h>

/* External cheat flags */
extern int disastersDisabled;

/* Sprite globals */
static SimSprite GlobalSprites[MAX_SPRITES];
static int SpriteCount = 0;
static short CrashX, CrashY;
static int SpriteCycle = 0;
static int absDist;

/* Movement direction vectors for 4-directional movement (trains, buses) */
static short Dx[5] = {0, 4, 0, -4, 0};   /* East, South, West, North, Stop */
static short Dy[5] = {-4, 0, 4, 0, 0};

/* Boat direction vectors (matching original Micropolis) */
static short BDx[9] = {0,  0,  1,  1,  1,  0, -1, -1, -1};
static short BDy[9] = {0, -1, -1,  0,  1,  1,  1,  0, -1};
static short BPx[9] = {0,  0,  2,  2,  2,  0, -2, -2, -2};
static short BPy[9] = {0, -2, -2,  0,  2,  2,  2,  0, -2};

/* Helicopter movement vectors (matching original Micropolis) */
static short HDx[9] = {0,  0,  3,  5,  3,  0, -3, -5, -3};
static short HDy[9] = {0, -5, -3,  0,  3,  5,  3,  0, -3};

/* Airplane movement vectors (matching original Micropolis) */
static short CDx[12] = {0,  0,  6,  8,  6,  0, -6, -8, -6,  8,  8,  8};
static short CDy[12] = {0, -8, -6,  0,  6,  8,  6,  0, -6,  0,  0,  0};

/* Animation frames for trains: dir 0=E, 1=S, 2=W, 3=N, 4=stop */
static short TrainPic2[5] = {1, 2, 1, 2, 5};

/* Turn direction table for sprite navigation */
static short Dir2Tab[16] = {0, 1, 2, 3, 4, 7, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0};

/* Water navigation clear table */
static short BtClrTab[8] = {RIVER, CHANNEL, POWERBASE, POWERBASE+1, RAILBASE, RAILBASE+1, BRWH, BRWV};

/* Forward declarations */
static int GetDirection(int orgX, int orgY, int desX, int desY);
static short GetDir(int orgX, int orgY, int desX, int desY);
static int TurnTo(int orgDir, int desDir);
static short GetChar(int x, int y);
static int CanDriveOn(int tileValue);
static int CheckSpriteCollision(SimSprite *s1, SimSprite *s2);
static int SpriteNotInBounds(SimSprite *sprite);
static void ExplodeSprite(SimSprite *sprite);
static void MakeSound(int soundId, int x, int y);
static short TryOther(int x, int y, int dir, SimSprite *sprite);
static void CheckCollisions(SimSprite *sprite);
static int IsWater(short tile);
static void SpriteDestroy(int ox, int oy);
static void StartFire(int x, int y);

/* Initialize sprite system */
void InitSprites(void) {
    int i;
    
    for (i = 0; i < MAX_SPRITES; i++) {
        GlobalSprites[i].type = SPRITE_UNDEFINED;
        GlobalSprites[i].frame = 0;
        GlobalSprites[i].x = 0;
        GlobalSprites[i].y = 0;
        GlobalSprites[i].width = 0;
        GlobalSprites[i].height = 0;
        GlobalSprites[i].x_offset = 0;
        GlobalSprites[i].y_offset = 0;
        GlobalSprites[i].x_hot = 0;
        GlobalSprites[i].y_hot = 0;
        GlobalSprites[i].orig_x = 0;
        GlobalSprites[i].orig_y = 0;
        GlobalSprites[i].dest_x = 0;
        GlobalSprites[i].dest_y = 0;
        GlobalSprites[i].count = 0;
        GlobalSprites[i].sound_count = 0;
        GlobalSprites[i].dir = 0;
        GlobalSprites[i].new_dir = 0;
        GlobalSprites[i].step = 0;
        GlobalSprites[i].flag = 0;
        GlobalSprites[i].control = -1;
        GlobalSprites[i].turn = 0;
        GlobalSprites[i].accel = 0;
        GlobalSprites[i].speed = DEFAULT_SPRITE_SPEED;
    }
    
    SpriteCount = 0;
    SpriteCycle = 0;
}

/* Create a new sprite */
SimSprite* NewSprite(int type, int x, int y) {
    SimSprite *sprite;
    int i;
    
    /* Find available sprite slot */
    for (i = 0; i < MAX_SPRITES; i++) {
        if (GlobalSprites[i].type == SPRITE_UNDEFINED) {
            sprite = &GlobalSprites[i];
            break;
        }
    }
    
    if (i >= MAX_SPRITES) {
        return NULL; /* No available slots */
    }
    
    /* Initialize sprite */
    sprite->type = type;
    sprite->x = x;
    sprite->y = y;
    sprite->frame = 0;
    sprite->orig_x = x;
    sprite->orig_y = y;
    sprite->dest_x = x;
    sprite->dest_y = y;
    sprite->count = 0;
    sprite->sound_count = 0;
    sprite->dir = 0;
    sprite->new_dir = 0;
    sprite->step = 0;
    sprite->flag = 0;
    sprite->control = -1;
    sprite->turn = 0;
    sprite->accel = 0;
    sprite->speed = DEFAULT_SPRITE_SPEED;
    
    /* Set sprite-specific properties */
    switch (type) {
        case SPRITE_TRAIN:
            sprite->width = 32;
            sprite->height = 32;
            sprite->x_offset = 32;
            sprite->y_offset = -16;
            sprite->x_hot = 40;
            sprite->y_hot = -8;
            sprite->frame = 1;
            sprite->dir = 4;
            break;
            
        case SPRITE_SHIP:
            sprite->width = 48;
            sprite->height = 48;
            sprite->x_offset = 32;
            sprite->y_offset = -16;
            sprite->x_hot = 48;
            sprite->y_hot = 0;
            if (x < (4 << 4)) sprite->frame = 3;
            else if (x >= ((WORLD_X - 4) << 4)) sprite->frame = 7;
            else if (y < (4 << 4)) sprite->frame = 5;
            else if (y >= ((WORLD_Y - 4) << 4)) sprite->frame = 1;
            else sprite->frame = 3;
            sprite->new_dir = sprite->frame;
            sprite->dir = 10;
            sprite->count = 1;
            break;
            
        case SPRITE_AIRPLANE:
            sprite->width = 48;
            sprite->height = 48;
            sprite->x_offset = 24;
            sprite->y_offset = 0;
            sprite->x_hot = 48;
            sprite->y_hot = 16;
            if (x > ((WORLD_X - 20) << 4)) {
                sprite->x -= 100 + 48;
                sprite->dest_x = sprite->x - 200;
                sprite->frame = 7;
            } else {
                sprite->dest_x = sprite->x + 200;
                sprite->frame = 11;
            }
            sprite->dest_y = sprite->y;
            break;
            
        case SPRITE_HELICOPTER:
            sprite->width = 32;
            sprite->height = 32;
            sprite->x_offset = 32;
            sprite->y_offset = -16;
            sprite->x_hot = 40;
            sprite->y_hot = -8;
            sprite->frame = 5;
            sprite->count = 1500;
            sprite->dest_x = SimRandom((WORLD_X << 4) - 1);
            sprite->dest_y = SimRandom((WORLD_Y << 4) - 1);
            sprite->orig_x = x - 30;
            sprite->orig_y = y;
            break;
            
        case SPRITE_BUS:
            sprite->width = 32;
            sprite->height = 32;
            sprite->x_offset = 30;
            sprite->y_offset = -18;
            sprite->x_hot = 40;
            sprite->y_hot = -8;
            sprite->frame = 1;
            break;
            
        case SPRITE_POLICE:
            sprite->width = 32;
            sprite->height = 32;
            sprite->x_offset = 30;
            sprite->y_offset = -18;
            sprite->x_hot = 40;
            sprite->y_hot = -8;
            sprite->frame = 1;
            sprite->count = POLICE_DUTY_TIME; /* Police stay for 5 minutes */
            break;
            
        case SPRITE_MONSTER:
            sprite->width = 48;
            sprite->height = 48;
            sprite->x_offset = 24;
            sprite->y_offset = 0;
            sprite->x_hot = 40;
            sprite->y_hot = 16;
            if (x > ((WORLD_X << 4) / 2)) {
                if (y > ((WORLD_Y << 4) / 2)) sprite->frame = 10;
                else sprite->frame = 7;
            } else {
                if (y > ((WORLD_Y << 4) / 2)) sprite->frame = 1;
                else sprite->frame = 4;
            }
            sprite->count = 1000;
            sprite->orig_x = sprite->x;
            sprite->orig_y = sprite->y;
            break;
            
        case SPRITE_TORNADO:
            sprite->width = 48;
            sprite->height = 48;
            sprite->x_offset = 24;
            sprite->y_offset = 0;
            sprite->x_hot = 40;
            sprite->y_hot = 36;
            sprite->frame = 1;
            sprite->count = 200;
            break;
    }
    
    SpriteCount++;
    return sprite;
}

/* Remove sprite from system */
void DestroySprite(SimSprite *sprite) {
    if (sprite && sprite->type != SPRITE_UNDEFINED) {
        sprite->type = SPRITE_UNDEFINED;
        SpriteCount--;
    }
}

/* Update all sprites */
void MoveSprites(void) {
    int i;
    SimSprite *sprite;
    
    SpriteCycle++;
    if (SpriteCycle > 1023) {
        SpriteCycle = 0;
    }
    
    for (i = 0; i < MAX_SPRITES; i++) {
        sprite = &GlobalSprites[i];
        
        if (sprite->type == SPRITE_UNDEFINED) {
            continue;
        }
        
        /* Update sprite based on type */
        switch (sprite->type) {
            case SPRITE_TRAIN:
                DoTrainSprite(sprite);
                break;
                
            case SPRITE_SHIP:
                DoShipSprite(sprite);
                break;
                
            case SPRITE_AIRPLANE:
                DoAirplaneSprite(sprite);
                break;
                
            case SPRITE_HELICOPTER:
                DoCopterSprite(sprite);
                break;
                
            case SPRITE_BUS:
                DoBusSprite(sprite);
                break;
                
            case SPRITE_POLICE:
                DoPoliceSprite(sprite);
                break;
                
            case SPRITE_MONSTER:
                DoMonsterSprite(sprite);
                break;
                
            case SPRITE_TORNADO:
                DoTornadoSprite(sprite);
                break;
                
            case SPRITE_EXPLOSION:
                DoExplosion(sprite);
                break;
        }
        
        /* Check bounds */
        if (SpriteNotInBounds(sprite)) {
            if (sprite->type == SPRITE_AIRPLANE || sprite->type == SPRITE_HELICOPTER) {
                /* Aircraft can leave bounds temporarily */
                continue;
            }
            DestroySprite(sprite);
        }
    }
}

/* Train sprite behavior (based on original Micropolis) */
void DoTrainSprite(SimSprite *sprite) {
    static short Cx[4] = {   0,  16,   0, -16 };
    static short Cy[4] = { -16,   0,  16,   0 };
    short z, dir, dir2, c;

    if (sprite->frame == 3 || sprite->frame == 4)
        sprite->frame = TrainPic2[sprite->dir];

    sprite->x += Dx[sprite->dir];
    sprite->y += Dy[sprite->dir];

    if (!(SpriteCycle & 3)) {
        dir = SimRandom(4);
        for (z = dir; z < (dir + 4); z++) {
            dir2 = z & 3;
            if (sprite->dir != 4) {
                if (dir2 == ((sprite->dir + 2) & 3)) continue;
            }
            c = GetChar(sprite->x + Cx[dir2] + 48,
                        sprite->y + Cy[dir2]);
            if ((c >= RAILBASE && c <= LASTRAIL) ||
                c == RAILVPOWERH ||
                c == RAILHPOWERV) {
                if (sprite->dir != dir2 && sprite->dir != 4) {
                    if ((sprite->dir + dir2) == 3)
                        sprite->frame = 3;
                    else
                        sprite->frame = 4;
                } else {
                    sprite->frame = TrainPic2[dir2];
                }

                if (c == RAILBASE || c == (RAILBASE + 1))
                    sprite->frame = 5;
                sprite->dir = dir2;
                return;
            }
        }
        if (sprite->dir == 4) {
            sprite->frame = 0;
            return;
        }
        sprite->dir = 4;
    }
}

static int ShipTryOther(int tileVal, int oldDir, int newDir) {
    int z;
    z = oldDir + 4;
    if (z > 8) z -= 8;
    if (newDir != z) return 0;
    if (tileVal == POWERBASE || tileVal == POWERBASE + 1 ||
        tileVal == RAILBASE || tileVal == RAILBASE + 1)
        return 1;
    return 0;
}

void DoShipSprite(SimSprite *sprite) {
    int x, y, z, t, tem, pem;

    if (sprite->sound_count > 0) sprite->sound_count--;
    if (!sprite->sound_count) {
        if ((SimRandom(4)) == 1)
            MakeSound(0, sprite->x, sprite->y);
        sprite->sound_count = 200;
    }

    if (sprite->count > 0) sprite->count--;
    if (!sprite->count) {
        sprite->count = 9;
        if (sprite->frame != sprite->new_dir) {
            sprite->frame = TurnTo(sprite->frame, sprite->new_dir);
            return;
        }
        tem = SimRandom(8);
        for (pem = tem; pem < (tem + 8); pem++) {
            z = (pem & 7) + 1;
            if (z == sprite->dir) continue;
            x = ((sprite->x + 48 - 1) >> 4) + BDx[z];
            y = (sprite->y >> 4) + BDy[z];
            if (BOUNDS_CHECK(x, y)) {
                t = Map[x][y] & LOMASK;
                if (t == CHANNEL || t == BRWH || t == BRWV ||
                    ShipTryOther(t, sprite->dir, z)) {
                    sprite->new_dir = z;
                    sprite->frame = TurnTo(sprite->frame, sprite->new_dir);
                    sprite->dir = z + 4;
                    if (sprite->dir > 8) sprite->dir -= 8;
                    break;
                }
            }
        }
        if (pem == (tem + 8)) {
            sprite->dir = 10;
            sprite->new_dir = (SimRandom(8)) + 1;
        }
    } else {
        z = sprite->frame;
        if (z == sprite->new_dir) {
            sprite->x += BPx[z];
            sprite->y += BPy[z];
        }
    }

    if (SpriteNotInBounds(sprite)) {
        sprite->frame = 0;
        return;
    }

    {
        int ok = 0;
        t = GetChar(sprite->x + sprite->x_hot, sprite->y + sprite->y_hot);
        for (z = 0; z < 8; z++) {
            if (t == BtClrTab[z]) { ok = 1; break; }
        }
        if (!ok) {
            ExplodeSprite(sprite);
            SpriteDestroy(sprite->x + 48, sprite->y);
        }
    }
}

/* Airplane sprite behavior (based on original Micropolis) */
void DoAirplaneSprite(SimSprite *sprite) {
    int z, d;

    z = sprite->frame;

    if (!(SpriteCycle % 5)) {
        if (z > 8) {
            z--;
            if (z < 9) z = 3;
            sprite->frame = z;
        } else {
            d = GetDirection(sprite->x, sprite->y, sprite->dest_x, sprite->dest_y);
            z = TurnTo(z, d);
            sprite->frame = z;
        }
    }

    {
        int dx, dy;
        dx = sprite->dest_x - sprite->x;
        dy = sprite->dest_y - sprite->y;
        if (abs(dx) + abs(dy) < 50) {
            sprite->dest_x = SimRandom((WORLD_X * 16) + 100) - 50;
            sprite->dest_y = SimRandom((WORLD_Y * 16) + 100) - 50;
        }
    }

    if (!disastersDisabled) {
        int i, explode = 0;
        SimSprite *s;
        for (i = 0; i < MAX_SPRITES; i++) {
            s = &GlobalSprites[i];
            if (s->frame == 0 || s->type == SPRITE_UNDEFINED)
                continue;
            if ((s->type == SPRITE_HELICOPTER ||
                 (s != sprite && s->type == SPRITE_AIRPLANE)) &&
                CheckSpriteCollision(sprite, s)) {
                ExplodeSprite(s);
                explode = 1;
            }
        }
        if (explode) {
            ExplodeSprite(sprite);
            return;
        }
    }

    sprite->x += CDx[z];
    sprite->y += CDy[z];
    if (SpriteNotInBounds(sprite)) sprite->frame = 0;
}

/* Helicopter sprite behavior - matches original Micropolis w_sprite.c */
void DoCopterSprite(SimSprite *sprite) {
    short z, d, x, y;

    if (sprite->sound_count > 0) sprite->sound_count--;

    if (sprite->control < 0) {
        if (sprite->count > 0) sprite->count--;
        if (!sprite->count) {
            SimSprite *s = GetSprite(SPRITE_MONSTER);
            if (s != NULL) {
                sprite->dest_x = s->x;
                sprite->dest_y = s->y;
            } else {
                s = GetSprite(SPRITE_TORNADO);
                if (s != NULL) {
                    sprite->dest_x = s->x;
                    sprite->dest_y = s->y;
                } else {
                    sprite->dest_x = sprite->orig_x;
                    sprite->dest_y = sprite->orig_y;
                }
            }
        }
        if (!sprite->count) {
            GetDir(sprite->x, sprite->y, sprite->orig_x, sprite->orig_y);
            if (absDist < 30) {
                sprite->frame = 0;
                return;
            }
        }
    } else {
        GetDir(sprite->x, sprite->y, sprite->dest_x, sprite->dest_y);
        if (absDist < 16) {
            sprite->dest_x = sprite->orig_x;
            sprite->dest_y = sprite->orig_y;
            sprite->control = -1;
        }
    }

    if (!sprite->sound_count) {
        x = (sprite->x + 48) >> 5;
        y = sprite->y >> 5;
        if (x >= 0 && x < (WORLD_X >> 1) && y >= 0 && y < (WORLD_Y >> 1)) {
            if (TrfDensity[x][y] > 170 && (Rand16() & 7) == 0) {
                SendMesAt(-41, (x << 1) + 1, (y << 1) + 1);
                sprite->sound_count = 200;
            }
        }
    }

    z = sprite->frame;
    if (!(SpriteCycle & 3)) {
        d = GetDir(sprite->x, sprite->y, sprite->dest_x, sprite->dest_y);
        z = TurnTo(z, d);
        sprite->frame = z;
    }
    sprite->x += HDx[z];
    sprite->y += HDy[z];
}

/* Bus sprite behavior */
/* Bus sprite behavior - original physics with WinTown road-following */
void DoBusSprite(SimSprite *sprite) {
    static short BusDx[5] = {0, 1, 0, -1, 0};
    static short BusDy[5] = {-1, 0, 1, 0, 0};
    static short Dir2Frame[4] = {1, 2, 1, 2};
    short tile, dir;
    int tx, ty, z, speed;

    if (sprite->turn) {
        if (sprite->turn < 0) {
            if (sprite->dir & 1) sprite->frame = 4;
            else sprite->frame = 3;
            sprite->turn++;
            sprite->dir = (sprite->dir - 1) & 3;
        } else {
            if (sprite->dir & 1) sprite->frame = 3;
            else sprite->frame = 4;
            sprite->turn--;
            sprite->dir = (sprite->dir + 1) & 3;
        }
        sprite->x += BusDx[sprite->dir];
        sprite->y += BusDy[sprite->dir];
        return;
    }

    if (sprite->frame == 3 || sprite->frame == 4)
        sprite->frame = Dir2Frame[sprite->dir];

    tx = (sprite->x + sprite->x_hot) >> 5;
    ty = (sprite->y + sprite->y_hot) >> 5;
    if (tx >= 0 && tx < (WORLD_X >> 1) && ty >= 0 && ty < (WORLD_Y >> 1)) {
        z = TrfDensity[tx][ty] >> 6;
        if (z > 1) z--;
    } else z = 0;

    switch (z) {
    case 0: speed = 4; break;
    case 1: speed = 2; break;
    default: speed = 1; break;
    }

    sprite->x += BusDx[sprite->dir] * speed;
    sprite->y += BusDy[sprite->dir] * speed;

    tile = GetChar(sprite->x + sprite->x_hot, sprite->y + sprite->y_hot);
    if (CanDriveOn(tile) <= 0) {
        sprite->x -= BusDx[sprite->dir] * speed;
        sprite->y -= BusDy[sprite->dir] * speed;
        dir = TryOther(sprite->x + sprite->x_hot, sprite->y + sprite->y_hot, sprite->dir, sprite);
        if (dir != sprite->dir) {
            sprite->dir = dir;
            sprite->frame = Dir2Frame[dir];
        } else {
            DestroySprite(sprite);
            return;
        }
    }

    if (!disastersDisabled) {
        int i, explode = 0;
        SimSprite *s;
        for (i = 0; i < MAX_SPRITES; i++) {
            s = &GlobalSprites[i];
            if (sprite == s || s->frame == 0) continue;
            if ((s->type == SPRITE_BUS ||
                 (s->type == SPRITE_TRAIN && s->frame != 5)) &&
                CheckSpriteCollision(sprite, s)) {
                ExplodeSprite(s);
                explode = 1;
            }
        }
        if (explode) ExplodeSprite(sprite);
    }
}

/* Police sprite behavior */
void DoPoliceSprite(SimSprite *sprite) {
    int dx, dy, z;
    
    /* Police car stays stationary at congestion point */
    if (sprite->count > 0) {
        sprite->count--;
        
        /* Flash police lights by alternating frame */
        if ((SpriteCycle & 7) == 0) {
            sprite->frame = (sprite->frame == 1) ? 2 : 1;
        }
        
        /* Check traffic density periodically */
        if ((sprite->count % 50) == 0) {
            dx = sprite->x >> 5;
            dy = sprite->y >> 5;
            
            if (dx >= 0 && dx < WORLD_X/2 && dy >= 0 && dy < WORLD_Y/2) {
                z = TrfDensity[dx][dy];
                if (z > POLICE_TRAFFIC_THRESHOLD && SimRandom(POLICE_REPORT_CHANCE) == 0) {
                    /* Report heavy traffic */
                    MakeSound(SOUND_HEAVY_TRAFFIC, sprite->x, sprite->y);
                }
            }
        }
    } else {
        /* Police car duty time expired - remove sprite */
        DestroySprite(sprite);
    }
}

/* Explosion sprite behavior */
void DoExplosion(SimSprite *sprite) {
    int x, y;

    if (!(SpriteCycle & 1)) {
        if (sprite->frame == 1) {
            MakeSound(SOUND_EXPLOSION_HIGH, sprite->x, sprite->y);
            x = (sprite->x >> 4) + 3;
            y = sprite->y >> 4;
            SendMesAt(32, x, y);
        }
        sprite->frame++;
    }

    if (sprite->frame > 6) {
        sprite->frame = 0;
        StartFire(sprite->x + 48 - 8, sprite->y + 16);
        StartFire(sprite->x + 48 - 24, sprite->y);
        StartFire(sprite->x + 48 + 8, sprite->y);
        StartFire(sprite->x + 48 - 24, sprite->y + 32);
        StartFire(sprite->x + 48 + 8, sprite->y + 32);
        return;
    }
}

static void StartFire(int x, int y) {
    short z, t;
    x >>= 4;
    y >>= 4;
    if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y) return;
    z = Map[x][y];
    t = z & LOMASK;
    if (!(z & BURNBIT) && t != 0) return;
    if (z & ZONEBIT) return;
    Map[x][y] = FIRE + (SimRandom(4)) + ANIMBIT;
}

static int checkWet(int t) {
    if ((t == POWERBASE) || (t == POWERBASE + 1) ||
        (t == RAILBASE) || (t == RAILBASE + 1) ||
        (t == BRWH) || (t == BRWV))
        return 1;
    return 0;
}

static void OFireZone(int x, int y, int ch) {
    int tx, ty, xymax;

    ch &= LOMASK;
    if (ch < PORTBASE)
        xymax = 2;
    else if (ch == AIRPORT)
        xymax = 5;
    else
        xymax = 4;

    for (tx = -1; tx < xymax; tx++) {
        for (ty = -1; ty < xymax; ty++) {
            int xx = x + tx;
            int yy = y + ty;
            if (BOUNDS_CHECK(xx, yy)) {
                if ((Map[xx][yy] & LOMASK) >= ROADBASE)
                    Map[xx][yy] |= BULLBIT;
            }
        }
    }
}

static void SpriteDestroy(int ox, int oy) {
    short t, z;
    int x, y;

    x = ox >> 4;
    y = oy >> 4;
    if (!BOUNDS_CHECK(x, y))
        return;
    z = Map[x][y];
    t = z & LOMASK;
    if (t < TREEBASE)
        return;
    if (!(z & BURNBIT)) {
        if (t >= ROADBASE && t <= LASTROAD)
            Map[x][y] = RIVER;
        return;
    }
    if (z & ZONEBIT) {
        OFireZone(x, y, z);
        if (t > RZB)
            MakeExplosion(x, y);
    }
    if (checkWet(t))
        Map[x][y] = RIVER;
    else
        Map[x][y] = TINYEXP | BULLBIT | ANIMBIT;
}

/* Helper function to determine if tile is water */
static int IsWater(short tile) {
    int i;
    
    if (tile == RIVER || tile == CHANNEL) {
        return 1;
    }
    
    for (i = 0; i < 8; i++) {
        if (tile == BtClrTab[i]) {
            return 1;
        }
    }
    
    return 0;
}

/* Check if bus can drive on tile - matches original Micropolis
 * Returns: 1=smooth road, -1=bumpy/dirt, 0=can't drive */
static int CanDriveOn(int tileValue) {
    tileValue &= LOMASK;
    if ((tileValue >= ROADBASE && tileValue <= LASTROAD &&
         tileValue != BRWH && tileValue != BRWV) ||
        tileValue == HRAILROAD || tileValue == VRAILROAD)
        return 1;
    if (tileValue == DIRT || tileValue == TREEBASE)
        return -1;
    return 0;
}

/* Try to find alternate direction */
static short TryOther(int x, int y, int dir, SimSprite *sprite) {
    short tile;
    int i, start_dir;
    
    start_dir = (dir + 1) & 3;
    
    for (i = start_dir; i != dir; i = (i + 1) & 3) {
        tile = GetChar(x + Dx[i], y + Dy[i]);
        
        if (sprite->type == SPRITE_TRAIN) {
            if (tile >= RAILBASE && tile <= LASTRAIL) {
                return i;
            }
        } else if (sprite->type == SPRITE_BUS) {
            if (CanDriveOn(tile)) {
                return i;
            }
        }
    }
    
    return dir; /* No alternate found */
}

/* Get direction from origin to destination (matching original Micropolis GetDir) */
static int GetDirection(int orgX, int orgY, int desX, int desY) {
    static short Gdtab[13] = { 0, 3, 2, 1, 3, 4, 5, 7, 6, 5, 7, 8, 1 };
    int dispX, dispY, z;

    dispX = desX - orgX;
    dispY = desY - orgY;

    if (dispX < 0) {
        z = (dispY < 0) ? 11 : 8;
    } else {
        z = (dispY < 0) ? 2 : 5;
    }

    if (dispX < 0) dispX = -dispX;
    if (dispY < 0) dispY = -dispY;

    if ((dispX << 1) < dispY) z++;
    else if ((dispY << 1) < dispX) z--;

    if (z < 0 || z > 12) z = 0;

    return Gdtab[z];
}

static short GetDir(int orgX, int orgY, int desX, int desY) {
    static short Gdtab[13] = { 0, 3, 2, 1, 3, 4, 5, 7, 6, 5, 7, 8, 1 };
    int dispX, dispY, z;

    dispX = desX - orgX;
    dispY = desY - orgY;
    if (dispX < 0) {
        if (dispY < 0) z = 11;
        else z = 8;
    } else {
        if (dispY < 0) z = 2;
        else z = 5;
    }
    if (dispX < 0) dispX = -dispX;
    if (dispY < 0) dispY = -dispY;
    absDist = dispX + dispY;
    if ((dispX << 1) < dispY) z++;
    else if ((dispY << 1) < dispX) z--;
    if (z < 0 || z > 12) z = 0;
    return Gdtab[z];
}

/* Turn toward desired direction (1 step at a time, shortest path) */
static int TurnTo(int p, int d) {
    if (p == d) return p;
    if (p < d) {
        if ((d - p) < 4) p++;
        else p--;
    } else {
        if ((p - d) < 4) p--;
        else p++;
    }
    if (p > 8) p = 1;
    if (p < 1) p = 8;
    return p;
}

/* Get tile at coordinates */
static short GetChar(int x, int y) {
    int mapX, mapY;
    
    mapX = x >> 4; /* Divide by 16 */
    mapY = y >> 4;
    
    if (mapX < 0 || mapX >= WORLD_X || mapY < 0 || mapY >= WORLD_Y) {
        return -1;
    }
    
    return Map[mapX][mapY] & LOMASK;
}

static int CheckSpriteCollision(SimSprite *s1, SimSprite *s2) {
    int dx, dy;
    if (s1->frame == 0 || s2->frame == 0)
        return 0;
    dx = (s1->x + s1->x_hot) - (s2->x + s2->x_hot);
    if (dx < 0) dx = -dx;
    dy = (s1->y + s1->y_hot) - (s2->y + s2->y_hot);
    if (dy < 0) dy = -dy;
    return ((dx + dy) < 30);
}

static int SpriteNotInBounds(SimSprite *sprite) {
    int x = sprite->x + sprite->x_hot;
    int y = sprite->y + sprite->y_hot;
    return (x < 0 || y < 0 || x >= (WORLD_X << 4) || y >= (WORLD_Y << 4));
}

/* Create explosion at sprite location */
static void ExplodeSprite(SimSprite *sprite) {
    SimSprite *exp;
    
    CrashX = (sprite->x + sprite->x_hot) >> 4;
    CrashY = (sprite->y + sprite->y_hot) >> 4;
    
    exp = NewSprite(SPRITE_EXPLOSION, sprite->x, sprite->y);
    if (exp) {
        exp->width = 48;
        exp->height = 48;
        exp->x_offset = 24;
        exp->y_offset = 0;
        exp->x_hot = 40;
        exp->y_hot = 16;
        exp->frame = 1;
    }
    
    DestroySprite(sprite);
}

static void CheckCollisions(SimSprite *sprite) {
    int i, explode;
    SimSprite *other;

    if (disastersDisabled) return;

    explode = 0;
    for (i = 0; i < MAX_SPRITES; i++) {
        other = &GlobalSprites[i];
        if (other->type == SPRITE_UNDEFINED || other == sprite)
            continue;
        if (!CheckSpriteCollision(sprite, other))
            continue;

        switch (sprite->type) {
        case SPRITE_AIRPLANE:
            if (other->type == SPRITE_HELICOPTER ||
                (other->type == SPRITE_AIRPLANE && sprite != other)) {
                ExplodeSprite(other);
                explode = 1;
            }
            break;
        case SPRITE_HELICOPTER:
            if (other->type == SPRITE_AIRPLANE) {
                ExplodeSprite(other);
                explode = 1;
            }
            break;
        }
    }
    if (explode)
        ExplodeSprite(sprite);
}

void GenerateTrain(int x, int y) {
    if (TotalPop > 20 &&
        GetSprite(SPRITE_TRAIN) == NULL &&
        !SimRandom(25)) {
        NewSprite(SPRITE_TRAIN, (x << 4) + TRA_GROOVE_X, (y << 4) + TRA_GROOVE_Y);
    }
}

static void MakeShipHere(int x, int y) {
    NewSprite(SPRITE_SHIP, (x << 4) - (48 - 1), y << 4);
}

void GenerateShip(void) {
    int x, y;

    if (!(SimRandom(3) == 0))
        return;
    if (SimRandom(4) == 0) {
        for (x = 4; x < WORLD_X - 2; x++)
            if ((Map[x][0] & LOMASK) == CHANNEL) {
                MakeShipHere(x, 0);
                return;
            }
    }
    if (SimRandom(4) == 0) {
        for (y = 1; y < WORLD_Y - 2; y++)
            if ((Map[0][y] & LOMASK) == CHANNEL) {
                MakeShipHere(0, y);
                return;
            }
    }
    if (SimRandom(4) == 0) {
        for (x = 4; x < WORLD_X - 2; x++)
            if ((Map[x][WORLD_Y - 1] & LOMASK) == CHANNEL) {
                MakeShipHere(x, WORLD_Y - 1);
                return;
            }
    }
    if (SimRandom(4) == 0) {
        for (y = 1; y < WORLD_Y - 2; y++)
            if ((Map[WORLD_X - 1][y] & LOMASK) == CHANNEL) {
                MakeShipHere(WORLD_X - 1, y);
                return;
            }
    }
}

void GeneratePlane(int x, int y) {
    if (GetSprite(SPRITE_AIRPLANE) != NULL) return;
    NewSprite(SPRITE_AIRPLANE, (x << 4) + 48, (y << 4) + 12);
}

void GenerateCopter(int x, int y) {
    if (GetSprite(SPRITE_HELICOPTER) != NULL) return;
    NewSprite(SPRITE_HELICOPTER, x << 4, (y << 4) + 30);
}

/* Placeholder sound function - to be implemented with Windows audio */
static void MakeSound(int soundId, int x, int y) {
    /* TODO: Implement Windows NT sound system */
    /* For now, just log to debug file */
    /* No MessageBox allowed per guidelines */
}

/* Get sprite count for debugging */
int GetSpriteCount(void) {
    return SpriteCount;
}

/* Get sprite by index for rendering */
SimSprite* GetSpriteByIndex(int index) {
    if (index >= 0 && index < MAX_SPRITES) {
        if (GlobalSprites[index].type != SPRITE_UNDEFINED) {
            return &GlobalSprites[index];
        }
    }
    return NULL;
}

/* Find first active sprite of a given type */
SimSprite* GetSprite(int type) {
    int i;
    for (i = 0; i < MAX_SPRITES; i++) {
        if (GlobalSprites[i].type == type && GlobalSprites[i].frame != 0)
            return &GlobalSprites[i];
    }
    return NULL;
}

/* Monster (Godzilla) sprite behavior */
/* Monster sprite behavior - matches original Micropolis w_sprite.c */
void DoMonsterSprite(SimSprite *sprite) {
    static short Gx[5] = {2, 2, -2, -2, 0};
    static short Gy[5] = {-2, 2, 2, -2, 0};
    static short ND1[4] = {0, 1, 2, 3};
    static short ND2[4] = {1, 2, 3, 0};
    static short nn1[4] = {2, 5, 8, 11};
    static short nn2[4] = {11, 2, 5, 8};
    short d, z, c;
    int i;
    SimSprite *s;

    if (sprite->sound_count > 0) sprite->sound_count--;

    if (sprite->control < 0) {
        if (sprite->control == -2) {
            d = (sprite->frame - 1) / 3;
            z = (sprite->frame - 1) % 3;
            if (z == 2) sprite->step = 0;
            if (z == 0) sprite->step = 1;
            if (sprite->step) z++;
            else z--;
            c = GetDir(sprite->x, sprite->y, sprite->dest_x, sprite->dest_y);
            if (absDist < 18) {
                sprite->control = -1;
                sprite->count = 1000;
                sprite->flag = 1;
                sprite->dest_x = sprite->orig_x;
                sprite->dest_y = sprite->orig_y;
            } else {
                c = (c - 1) / 2;
                if (((c != d) && (!Rand(5))) || (!Rand(20))) {
                    if (((c - d) & 3) == 1 || ((c - d) & 3) == 3) d = c;
                    else { if (Rand16() & 1) d++; else d--; d &= 3; }
                } else {
                    if (!Rand(20)) { if (Rand16() & 1) d++; else d--; d &= 3; }
                }
            }
        } else {
            d = (sprite->frame - 1) / 3;
            if (d < 4) {
                z = (sprite->frame - 1) % 3;
                if (z == 2) sprite->step = 0;
                if (z == 0) sprite->step = 1;
                if (sprite->step) z++;
                else z--;
                GetDir(sprite->x, sprite->y, sprite->dest_x, sprite->dest_y);
                if (absDist < 60) {
                    if (sprite->flag == 0) {
                        sprite->flag = 1;
                        sprite->dest_x = sprite->orig_x;
                        sprite->dest_y = sprite->orig_y;
                    } else {
                        sprite->frame = 0;
                        return;
                    }
                }
                c = GetDir(sprite->x, sprite->y, sprite->dest_x, sprite->dest_y);
                c = (c - 1) / 2;
                if ((c != d) && (!Rand(10))) {
                    if (Rand16() & 1) z = ND1[d];
                    else z = ND2[d];
                    d = 4;
                    if (!sprite->sound_count)
                        sprite->sound_count = 50 + Rand(100);
                }
            } else {
                d = 4;
                c = sprite->frame;
                z = (c - 13) & 3;
                if (!(Rand16() & 3)) {
                    if (Rand16() & 1) z = nn1[z];
                    else z = nn2[z];
                    d = (z - 1) / 3;
                    z = (z - 1) % 3;
                }
            }
        }
    } else {
        d = sprite->control;
        z = (sprite->frame - 1) % 3;
        if (z == 2) sprite->step = 0;
        if (z == 0) sprite->step = 1;
        if (sprite->step) z++;
        else z--;
    }

    z = (((d * 3) + z) + 1);
    if (z > 16) z = 16;
    sprite->frame = z;

    sprite->x += Gx[d];
    sprite->y += Gy[d];

    if (sprite->count > 0) sprite->count--;
    c = GetChar(sprite->x + sprite->x_hot, sprite->y + sprite->y_hot);
    if ((c == -1) ||
        ((c == RIVER) && (sprite->count != 0) && (sprite->control == -1))) {
        sprite->frame = 0;
    }

    for (i = 0; i < MAX_SPRITES; i++) {
        s = &GlobalSprites[i];
        if (s->frame == 0) continue;
        if ((s->type == SPRITE_AIRPLANE || s->type == SPRITE_HELICOPTER ||
             s->type == SPRITE_SHIP || s->type == SPRITE_TRAIN) &&
            CheckSpriteCollision(sprite, s))
            ExplodeSprite(s);
    }

    SpriteDestroy(sprite->x + 48, sprite->y + 16);
}

/* Tornado sprite behavior - matches original Micropolis w_sprite.c */
void DoTornadoSprite(SimSprite *sprite) {
    static short TDx[9] = {2, 3, 2, 0, -2, -3};
    static short TDy[9] = {-2, 0, 2, 3, 2, 0};
    short z;
    int i;
    SimSprite *s;

    z = sprite->frame;
    if (z == 2) {
        if (sprite->flag) z = 3;
        else z = 1;
    } else {
        if (z == 1) sprite->flag = 1;
        else sprite->flag = 0;
        z = 2;
    }

    if (sprite->count > 0) sprite->count--;

    sprite->frame = z;

    for (i = 0; i < MAX_SPRITES; i++) {
        s = &GlobalSprites[i];
        if (s->frame == 0) continue;
        if ((s->type == SPRITE_AIRPLANE || s->type == SPRITE_HELICOPTER ||
             s->type == SPRITE_SHIP || s->type == SPRITE_TRAIN) &&
            CheckSpriteCollision(sprite, s))
            ExplodeSprite(s);
    }

    z = Rand(5);
    sprite->x += TDx[z];
    sprite->y += TDy[z];

    if (SpriteNotInBounds(sprite))
        sprite->frame = 0;

    if ((sprite->count != 0) && (!Rand(500)))
        sprite->frame = 0;

    SpriteDestroy(sprite->x + 48, sprite->y + 40);
}

/* Helper function to get traffic density at sprite location */
static int GetTrafficDensityAtSprite(SimSprite *sprite) {
    int dx, dy;
    
    dx = sprite->x >> 5;  /* Divide by 32 for traffic map coordinates */
    dy = sprite->y >> 5;
    
    if (dx >= 0 && dx < WORLD_X/2 && dy >= 0 && dy < WORLD_Y/2) {
        return TrfDensity[dx][dy];
    }
    return 0;
}

/* Helper function for common sprite bounds checking */
static int ValidateSpriteMovement(SimSprite *sprite, int newX, int newY) {
    /* Check if new position is within reasonable bounds */
    if (newX < -sprite->width || newX >= (WORLD_X << 4) + sprite->width ||
        newY < -sprite->height || newY >= (WORLD_Y << 4) + sprite->height) {
        return 0;  /* Out of bounds */
    }
    return 1;  /* Valid movement */
}

/* Helper function for common sprite generation pattern */
static int ShouldGenerateSprite(int maxSprites, int randomChance) {
    if (SpriteCount >= maxSprites) {
        return 0;
    }
    
    if (SimRandom(randomChance) != 0) {
        return 0;
    }
    
    return 1;
}

/* Unified sprite movement function */
void MoveSprite(SimSprite *sprite, int movementType) {
    int newX, newY;
    
    /* Calculate new position based on movement type */
    switch (movementType) {
        case MOVEMENT_TYPE_GROUND:
            /* Uses Dx/Dy arrays (trains, buses) */
            newX = sprite->x + Dx[sprite->dir];
            newY = sprite->y + Dy[sprite->dir];
            break;
            
        case MOVEMENT_TYPE_HELICOPTER:
            newX = sprite->x + HDx[sprite->dir];
            newY = sprite->y + HDy[sprite->dir];
            break;
            
        case MOVEMENT_TYPE_BOAT:
            /* Uses BPx/BPy arrays */
            newX = sprite->x + BPx[sprite->dir];
            newY = sprite->y + BPy[sprite->dir];
            break;
            
        case MOVEMENT_TYPE_AIRPLANE:
            /* Uses CDx/CDy arrays */
            newX = sprite->x + CDx[sprite->dir];
            newY = sprite->y + CDy[sprite->dir];
            break;
            
        default:
            /* Invalid movement type - no movement */
            return;
    }
    
    /* Validate movement bounds */
    if (ValidateSpriteMovement(sprite, newX, newY)) {
        sprite->x = newX;
        sprite->y = newY;
    }
}
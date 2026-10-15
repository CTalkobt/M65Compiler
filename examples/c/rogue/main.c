/*
 * rogue — a mini rogue-like for the MEGA65
 *
 * Find the amulet on depth 8 (or die trying). Each level is a new random
 * dungeon in the style of the original Rogue: a 3x3 grid of rooms (some of
 * them just a corridor crossing), joined by corridors along a random
 * spanning tree plus a few extra links, so every room can be reached.
 *
 *   - a room lights up when you enter it; in a corridor you see the
 *     squares next to you; what you have seen stays on the map
 *   - monsters (rats to dragons, by depth) sleep until they see you, then
 *     chase you; walk into a monster to fight it
 *   - potions heal, gold counts for the score, weapons and armour add to
 *     your attack and defence; experience raises your level and hit points
 *   - no diagonal steps through doorways (as in Rogue)
 *
 * Keys (<input.h>):
 *   cursor keys or h j k l, y u b n diagonally    move / attack
 *   .  or SPACE  wait      >  go down the stairs
 *   R  new game (after the end)     RUN/STOP  quit
 * A joystick in port 2 moves too (fire: down the stairs).
 *
 * The screen is our own buffer (the VIC-IV screen and colour RAM pointers
 * moved to it, BASIC's screen comes back at the end); the map uses the
 * screen's width (80 columns, or 40). Entities are structs with single
 * inheritance (a Monster and an Item are a Thing with a position) and a
 * method. Interrupts are off while the game runs.
 *
 * Build with -DROGUE_TEST for the test: levels of every depth from fixed
 * seeds are checked (everything reachable, things on the floor, no
 * overlaps), then scripted moves, a fight and pick-ups; results at $C0C0.
 */
#include <mega65.h>
#include <dma.h>
#include <input.h>

#define REG(a) (*(volatile unsigned char *)(a))

/* ------------------------------------------------------------------------ */
/* Random numbers: 16-bit xorshift                                           */

static unsigned int rs = 0x2545;
static unsigned int rnd16(void) {
    rs ^= rs << 7;
    rs ^= rs >> 9;
    rs ^= rs << 8;
    return rs;
}
/* 0 .. n-1 */
static unsigned char rnd_n(unsigned char n) {
    return (unsigned char)(((unsigned long)(rnd16() & 0xFF) * n) >> 8);
}

/* ------------------------------------------------------------------------ */
/* The map                                                                  */

#define MAP_W 80
#define MAP_H 21
#define WIN_DEPTH 8

enum { T_ROCK, T_FLOOR, T_CORR, T_DOOR, T_STAIRS,
       T_HWALL, T_VWALL, T_TL, T_TR, T_BL, T_BR };
#define PASSABLE(t) ((t) >= T_FLOOR && (t) <= T_STAIRS)

static unsigned char map[MAP_H][MAP_W];
static unsigned char seen[MAP_H][MAP_W];     /* explored */
static unsigned char vis[MAP_H][MAP_W];      /* in view this turn */
static unsigned char map_w;                  /* 80, or 40 on a 40-column screen */

/* A room: its interior x1..x2, y1..y2 (walls around it); a "gone" room is
 * just a corridor crossing at (x1, y1) */
struct room {
    unsigned char x1, y1, x2, y2, gone;
};
static struct room rooms[9];

/* ------------------------------------------------------------------------ */
/* Things: monsters and items                                               */

/* A Monster and an Item are a Thing (their position first): either can
 * be passed where a struct Thing * is expected.
 * (A method's result type is a typedef: cc45 does not take "unsigned char"
 * there; methods are called on array elements or through pointers.) */
typedef unsigned char flag;
struct Thing {
    unsigned char x, y;
};
struct Monster : Thing {
    unsigned char type;
    signed int hp;
    unsigned char awake;
    flag alive(void) { return this->hp > 0; }
};
struct Item : Thing {
    unsigned char kind;
    unsigned int value;
};

static flag thing_at(struct Thing *t, unsigned char x, unsigned char y) {
    return t->x == x && t->y == y;
}

struct mtype {
    char letter;
    const char *name;
    unsigned char hp, dmg, xp, colour, depth;
};
#define N_MTYPES 9
static const struct mtype mtypes[N_MTYPES] = {
    { 'r', "rat",     3,  2,  1, 9,  1 },
    { 'k', "kobold",  5,  3,  2, 13, 1 },
    { 'b', "bat",     4,  2,  2, 4,  2 },
    { 's', "snake",   7,  4,  3, 5,  2 },
    { 'g', "goblin",  9,  4,  5, 8,  3 },
    { 'o', "orc",    13,  5,  8, 10, 4 },
    { 'z', "zombie", 17,  5, 10, 15, 5 },
    { 't', "troll",  24,  8, 18, 13, 6 },
    { 'd', "dragon", 36, 10, 35, 2,  7 },
};

enum { I_NONE, I_POTION, I_GOLD, I_WEAPON, I_ARMOUR, I_AMULET };

#define MAXMON 12
#define MAXITEM 10
static struct Monster mons[MAXMON];
static unsigned char nmon;
static struct Item items[MAXITEM];
static unsigned char nitem;

struct player {
    unsigned char x, y;
    signed int hp, maxhp;
    unsigned char weapon, armour, lvl;
    unsigned int gold, xp;
};
static struct player pl;
static unsigned char depth;
static unsigned char stairs_x, stairs_y;
static unsigned int turns;
static unsigned char start_room;               /* the player's room: no monsters at first */
enum { PLAYING, DEAD, WON };
static unsigned char state;
static const char *killer;

/* ------------------------------------------------------------------------ */
/* Messages (the top line)                                                  */

static char msg[96];
static unsigned char msg_len;

static void say(const char *s) {
    /* words are separated by a space, punctuation follows directly */
    if (msg_len && msg_len < 94 && *s != '.' && *s != '!' && *s != ')') msg[msg_len++] = ' ';
    while (*s && msg_len < 95) msg[msg_len++] = *s++;
    msg[msg_len] = 0;
}
static void say_num(unsigned int v) {
    char b[6];
    unsigned char i = 0;
    if (msg_len && msg_len < 94 && msg[msg_len - 1] != '+') msg[msg_len++] = ' ';
    do { b[i++] = '0' + v % 10; v /= 10; } while (v);
    while (i && msg_len < 95) msg[msg_len++] = b[--i];
    msg[msg_len] = 0;
}
/* "the orc " + verb */
static void say_mon(unsigned char type, const char *verb) {
    say("the");
    say(mtypes[type].name);
    say(verb);
}

/* ------------------------------------------------------------------------ */
/* Level generation                                                         */

static void set_tile(unsigned char x, unsigned char y, unsigned char t) {
    map[y][x] = t;
}
/* a corridor square: rock becomes corridor, a wall a door */
static void dig(unsigned char x, unsigned char y) {
    unsigned char t = map[y][x];
    if (t == T_ROCK) map[y][x] = T_CORR;
    else if (t >= T_HWALL) map[y][x] = T_DOOR;
}
static void dig_h(unsigned char y, unsigned char xa, unsigned char xb) {
    unsigned char t;
    if (xa > xb) { t = xa; xa = xb; xb = t; }
    for (; xa <= xb; xa++) dig(xa, y);
}
static void dig_v(unsigned char x, unsigned char ya, unsigned char yb) {
    unsigned char t;
    if (ya > yb) { t = ya; ya = yb; yb = t; }
    for (; ya <= yb; ya++) dig(x, ya);
}

static void make_room(unsigned char i) {
    struct room *r = &rooms[i];
    unsigned char cw = map_w / 3, ox = (i % 3) * cw, oy = (i / 3) * 7;
    unsigned char w, h, x, y;
    if (r->gone) {
        /* a corridor crossing */
        r->x1 = r->x2 = ox + 1 + rnd_n(cw - 3);
        r->y1 = r->y2 = oy + 1 + rnd_n(4);
        set_tile(r->x1, r->y1, T_CORR);
        return;
    }
    /* walls within columns ox..ox+cw-2 and rows oy..oy+5: the cells' last
     * column and row stay free, so corridors always have room to turn */
    w = 2 + rnd_n(cw - 4);
    h = 2 + rnd_n(3);
    x = ox + rnd_n(cw - 1 - (w + 2) + 1);
    y = oy + rnd_n(6 - (h + 2) + 1);
    r->x1 = x + 1; r->x2 = x + w;
    r->y1 = y + 1; r->y2 = y + h;
    for (y = r->y1 - 1; y <= r->y2 + 1; y++)
        for (x = r->x1 - 1; x <= r->x2 + 1; x++) {
            unsigned char t = T_FLOOR;
            unsigned char top = y == r->y1 - 1, bot = y == r->y2 + 1;
            unsigned char lft = x == r->x1 - 1, rgt = x == r->x2 + 1;
            if (top && lft) t = T_TL;
            else if (top && rgt) t = T_TR;
            else if (bot && lft) t = T_BL;
            else if (bot && rgt) t = T_BR;
            else if (top || bot) t = T_HWALL;
            else if (lft || rgt) t = T_VWALL;
            set_tile(x, y, t);
        }
}

/* a corridor between room a and the room b to its right (horiz) or below */
static void connect(unsigned char a, unsigned char b, unsigned char horiz) {
    struct room *ra = &rooms[a], *rb = &rooms[b];
    unsigned char px, py, qx, qy, m;
    if (horiz) {
        py = ra->y1 + rnd_n(ra->y2 - ra->y1 + 1);
        px = ra->gone ? ra->x1 : ra->x2 + 1;
        qy = rb->y1 + rnd_n(rb->y2 - rb->y1 + 1);
        qx = rb->gone ? rb->x1 : rb->x1 - 1;
        m = px + 1 + rnd_n(qx - px - 1);
        dig_h(py, px, m);
        dig_v(m, py, qy);
        dig_h(qy, m, qx);
    } else {
        px = ra->x1 + rnd_n(ra->x2 - ra->x1 + 1);
        py = ra->gone ? ra->y1 : ra->y2 + 1;
        qx = rb->x1 + rnd_n(rb->x2 - rb->x1 + 1);
        qy = rb->gone ? rb->y1 : rb->y1 - 1;
        m = py + 1 + rnd_n(qy - py - 1);
        dig_v(px, py, m);
        dig_h(m, px, qx);
        dig_v(qx, m, qy);
    }
}

/* is anything (player, stairs, monster, item) at x, y? */
static unsigned char occupied(unsigned char x, unsigned char y) {
    unsigned char i;
    if (pl.x == x && pl.y == y) return 1;
    if (stairs_x == x && stairs_y == y) return 1;
    for (i = 0; i < nmon; i++) if (mons[i].alive() && thing_at(&mons[i], x, y)) return 1;
    for (i = 0; i < nitem; i++) if (items[i].kind != I_NONE && thing_at(&items[i], x, y)) return 1;
    return 0;
}
/* a free floor square in a real room (not the room 'avoid') */
static void free_spot(unsigned char avoid, unsigned char *px, unsigned char *py) {
    unsigned char r, x, y, tries = 0;
    for (;;) {
        r = rnd_n(9);
        if (rooms[r].gone || (r == avoid && tries < 50)) { tries++; continue; }
        x = rooms[r].x1 + rnd_n(rooms[r].x2 - rooms[r].x1 + 1);
        y = rooms[r].y1 + rnd_n(rooms[r].y2 - rooms[r].y1 + 1);
        if (map[y][x] == T_FLOOR && !occupied(x, y)) { *px = x; *py = y; return; }
        tries++;
    }
}

static void add_monster(void) {
    struct Monster *m;
    unsigned char t, n = 0;
    if (nmon == MAXMON) return;
    while (n < N_MTYPES && mtypes[n].depth <= depth) n++;
    /* the deeper kinds are likelier: one of the last four allowed */
    t = n > 4 ? n - 4 + rnd_n(4) : rnd_n(n);
    m = &mons[nmon];
    m->type = t;
    m->hp = mtypes[t].hp;
    m->awake = rnd_n(4) == 0;
    m->x = m->y = 255;
    free_spot(start_room, &m->x, &m->y);
    nmon++;
}
static void add_item(unsigned char kind) {
    struct Item *it;
    if (nitem == MAXITEM) return;
    it = &items[nitem];
    it->kind = kind;
    it->value = kind == I_GOLD ? 5 + rnd_n(16) * depth : 0;
    it->x = it->y = 255;
    free_spot(255, &it->x, &it->y);
    nitem++;
}

static void gen_level(void) {
    unsigned char i, k, gone = 0, inset = 1, start, edges;
    unsigned char conn[9];             /* bit d: linked to the next room right (1) / below (2) */
    for (k = 0; k < MAP_H; k++)
        for (i = 0; i < MAP_W; i++) { map[k][i] = T_ROCK; seen[k][i] = 0; }
    /* up to two rooms are only corridor crossings */
    for (i = 0; i < 9; i++) {
        rooms[i].gone = gone < 2 && rnd_n(6) == 0;
        if (rooms[i].gone) gone++;
        make_room(i);
        conn[i] = 0;
    }
    /* a random spanning tree over the 3x3 grid, then two extra links */
    start = rnd_n(9);
    {
        unsigned int in = 1 << start;
        while (inset < 9) {
            unsigned char a = rnd_n(9), dir = rnd_n(4), b;
            signed char ax = a % 3, ay = a / 3;
            if (!(in & (1 << a))) continue;
            if (dir == 0 && ax < 2) b = a + 1;
            else if (dir == 1 && ax > 0) b = a - 1;
            else if (dir == 2 && ay < 2) b = a + 3;
            else if (dir == 3 && ay > 0) b = a - 3;
            else continue;
            if (in & (1 << b)) continue;
            if (b == a + 1) conn[a] |= 1;
            else if (b == a - 1) conn[b] |= 1;
            else if (b == a + 3) conn[a] |= 2;
            else conn[b] |= 2;
            in |= 1 << b;
            inset++;
        }
    }
    for (edges = 0; edges < 2; edges++) {
        unsigned char a = rnd_n(9);
        if (a % 3 < 2 && rnd_n(2)) conn[a] |= 1;
        else if (a / 3 < 2) conn[a] |= 2;
    }
    for (i = 0; i < 9; i++) {
        if (conn[i] & 1) connect(i, i + 1, 1);
        if (conn[i] & 2) connect(i, i + 3, 0);
    }
    /* the player, the stairs (not in the player's room), monsters, items */
    nmon = nitem = 0;
    pl.x = pl.y = 255;
    stairs_x = stairs_y = 255;
    do { start = rnd_n(9); } while (rooms[start].gone);
    start_room = start;
    pl.x = rooms[start].x1 + rnd_n(rooms[start].x2 - rooms[start].x1 + 1);
    pl.y = rooms[start].y1 + rnd_n(rooms[start].y2 - rooms[start].y1 + 1);
    if (depth < WIN_DEPTH) {
        free_spot(start, &stairs_x, &stairs_y);
        map[stairs_y][stairs_x] = T_STAIRS;
    }
    k = 3 + depth / 2 + rnd_n(3);
    for (i = 0; i < k; i++) add_monster();
    k = 3 + rnd_n(3);
    for (i = 0; i < k; i++) {
        unsigned char r = rnd_n(10);
        add_item(r < 4 ? I_GOLD : r < 7 ? I_POTION : r < 9 ? I_WEAPON : I_ARMOUR);
    }
    if (depth == WIN_DEPTH) add_item(I_AMULET);
}

/* ------------------------------------------------------------------------ */
/* What the player sees                                                     */

/* the room whose walls contain x, y (255: none) */
static unsigned char room_at(unsigned char x, unsigned char y) {
    unsigned char i;
    for (i = 0; i < 9; i++) {
        struct room *r = &rooms[i];
        if (!r->gone && x + 1 >= r->x1 && x <= r->x2 + 1 && y + 1 >= r->y1 && y <= r->y2 + 1) return i;
    }
    return 255;
}

static void update_view(void) {
    unsigned char x, y, r;
    signed char dx, dy;
    for (y = 0; y < MAP_H; y++)
        for (x = 0; x < map_w; x++) vis[y][x] = 0;
    /* the squares around the player */
    for (dy = -1; dy <= 1; dy++)
        for (dx = -1; dx <= 1; dx++) {
            x = pl.x + dx; y = pl.y + dy;
            if (x < map_w && y < MAP_H) vis[y][x] = 1;
        }
    /* a lit room: all of it (also from its doorway) */
    r = room_at(pl.x, pl.y);
    if (r != 255) {
        struct room *rm = &rooms[r];
        for (y = rm->y1 - 1; y <= rm->y2 + 1; y++)
            for (x = rm->x1 - 1; x <= rm->x2 + 1; x++) vis[y][x] = 1;
    }
    for (y = 0; y < MAP_H; y++)
        for (x = 0; x < map_w; x++)
            if (vis[y][x]) seen[y][x] = 1;
}

/* ------------------------------------------------------------------------ */
/* Moving and fighting                                                      */

static signed char sgn(signed int v) { return v > 0 ? 1 : (v < 0 ? -1 : 0); }

static unsigned char mon_at(unsigned char x, unsigned char y) {
    unsigned char i;
    for (i = 0; i < nmon; i++) if (mons[i].alive() && thing_at(&mons[i], x, y)) return i;
    return 255;
}

/* a step from x, y by dx, dy: no diagonal steps into or out of doorways */
static unsigned char can_step(unsigned char x, unsigned char y, signed char dx, signed char dy) {
    unsigned char nx = x + dx, ny = y + dy;
    if (nx >= map_w || ny >= MAP_H || !PASSABLE(map[ny][nx])) return 0;
    if (dx && dy && (map[y][x] == T_DOOR || map[ny][nx] == T_DOOR)) return 0;
    return 1;
}

static unsigned char attack_power(void) { return 3 + pl.weapon + pl.lvl / 2; }

static void gain_xp(unsigned char xp) {
    pl.xp += xp;
    while (pl.xp >= (unsigned int)pl.lvl * pl.lvl * 10) {
        pl.lvl++;
        pl.maxhp += 4 + rnd_n(4);
        pl.hp += 4;
        say("welcome to level");
        say_num(pl.lvl);
        say("!");
    }
}

static void player_attacks(unsigned char i) {
    struct Monster *m = &mons[i];
    m->awake = 1;
    if (rnd_n(4) == 0) { say("you miss"); say_mon(m->type, "."); return; }
    m->hp -= 1 + rnd_n(attack_power());
    if (m->hp <= 0) {
        say("you kill"); say_mon(m->type, ".");
        gain_xp(mtypes[m->type].xp);
    } else {
        say("you hit"); say_mon(m->type, ".");
    }
}

static void pick_up(void) {
    unsigned char i;
    for (i = 0; i < nitem; i++) {
        struct Item *it = &items[i];
        if (it->kind == I_NONE || !thing_at(it, pl.x, pl.y)) continue;
        switch (it->kind) {
            case I_GOLD:
                pl.gold += it->value;
                say("you find"); say_num(it->value); say("gold.");
                break;
            case I_POTION:
                pl.hp += 8 + rnd_n(8);
                if (pl.hp > pl.maxhp) pl.hp = pl.maxhp;
                say("you drink a potion and feel better.");
                break;
            case I_WEAPON:
                pl.weapon++;
                say("you find a better sword (+"); say_num(pl.weapon); say(").");
                break;
            case I_ARMOUR:
                pl.armour++;
                say("you put on better armour (+"); say_num(pl.armour); say(").");
                break;
            case I_AMULET:
                say("you have the amulet of yendor!");
                state = WON;
                break;
        }
        it->kind = I_NONE;
    }
}

/* the player's move dx, dy (0, 0: wait); 1 if it took a turn */
static unsigned char player_move(signed char dx, signed char dy) {
    unsigned char nx = pl.x + dx, ny = pl.y + dy, m;
    if (!dx && !dy) return 1;
    m = mon_at(nx, ny);
    if (m != 255) { player_attacks(m); return 1; }
    if (!can_step(pl.x, pl.y, dx, dy)) return 0;
    pl.x = nx; pl.y = ny;
    pick_up();
    if (map[ny][nx] == T_STAIRS) say("there are stairs down here (>).");
    return 1;
}

static void monsters_act(void) {
    unsigned char i;
    for (i = 0; i < nmon; i++) {
        struct Monster *m = &mons[i];
        signed char dx, dy;
        if (m->hp <= 0 || state != PLAYING) continue;
        if (vis[m->y][m->x]) m->awake = 1;
        if (!m->awake) continue;
        dx = sgn((signed int)pl.x - m->x);
        dy = sgn((signed int)pl.y - m->y);
        if (m->type == 2 && rnd_n(2)) {           /* bats flutter */
            dx = (signed char)rnd_n(3) - 1;
            dy = (signed char)rnd_n(3) - 1;
        }
        if (m->x + dx == pl.x && m->y + dy == pl.y && can_step(m->x, m->y, dx, dy)) {
            /* next to the player: attack */
            if (rnd_n(20) >= 6 + pl.armour) {
                pl.hp -= 1 + rnd_n(mtypes[m->type].dmg);
                say_mon(m->type, "hits you.");
                if (pl.hp <= 0) { state = DEAD; killer = mtypes[m->type].name; }
            } else {
                say_mon(m->type, "misses.");
            }
            continue;
        }
        /* towards the player: straight, else along one axis */
        if (!(dx || dy)) continue;
        if (can_step(m->x, m->y, dx, dy) && mon_at(m->x + dx, m->y + dy) == 255 &&
            !(m->x + dx == pl.x && m->y + dy == pl.y)) {
            m->x += dx; m->y += dy;
        } else if (dx && can_step(m->x, m->y, dx, 0) && mon_at(m->x + dx, m->y) == 255 &&
                   !(m->x + dx == pl.x && m->y == pl.y)) {
            m->x += dx;
        } else if (dy && can_step(m->x, m->y, 0, dy) && mon_at(m->x, m->y + dy) == 255 &&
                   !(m->x == pl.x && m->y + dy == pl.y)) {
            m->y += dy;
        }
    }
}

static void new_level(void) {
    gen_level();
    update_view();
}

static void new_game(void) {
    pl.hp = pl.maxhp = 14;
    pl.weapon = pl.armour = 0;
    pl.lvl = 1;
    pl.gold = pl.xp = 0;
    depth = 1;
    turns = 0;
    state = PLAYING;
    msg_len = 0; msg[0] = 0;
    new_level();
    say("welcome to the dungeon. find the amulet on depth");
    say_num(WIN_DEPTH);
    say(".");
}

/* one command; 1 if it took a turn */
enum { CMD_NONE, CMD_N, CMD_S, CMD_E, CMD_W, CMD_NE, CMD_NW, CMD_SE, CMD_SW, CMD_WAIT, CMD_DOWN, CMD_RESTART, CMD_QUIT };
static const signed char cmd_dx[9] = { 0, 0, 0, 1, -1, 1, -1, 1, -1 };
static const signed char cmd_dy[9] = { 0, -1, 1, 0, 0, -1, -1, 1, 1 };

static unsigned char do_command(unsigned char c) {
    unsigned char took = 0;
    if (c >= CMD_N && c <= CMD_SW) took = player_move(cmd_dx[c], cmd_dy[c]);
    else if (c == CMD_WAIT) took = 1;
    else if (c == CMD_DOWN) {
        if (pl.x == stairs_x && pl.y == stairs_y) {
            depth++;
            new_level();
            say("you go down to depth"); say_num(depth); say(".");
            return 0;
        }
        say("there are no stairs here.");
    }
    if (took && state == PLAYING) {
        turns++;
        update_view();
        monsters_act();
        if ((turns & 7) == 0 && pl.hp < pl.maxhp) pl.hp++;
    }
    return took;
}

static unsigned int score(void) {
    return pl.gold + depth * 50 + pl.xp * 2 + (state == WON ? 1000 : 0);
}

#ifndef ROGUE_TEST
/* ------------------------------------------------------------------------ */
/* The screen: our own buffer, colour RAM from offset $0800                 */

#define COLOUR_LIN 0xFF80800UL
static unsigned char scr[2000];
static unsigned char colbuf[2000];
static unsigned char cols;
static unsigned int stride;

enum { C_BLACK, C_WHITE, C_RED, C_CYAN, C_PURPLE, C_GREEN, C_BLUE, C_YELLOW, C_ORANGE,
       C_BROWN, C_LRED, C_DGREY, C_GREY, C_LGREEN, C_LBLUE, C_LGREY };

static unsigned char scode(unsigned char c) {
    if (c >= 0x40 && c < 0x60) return c - 0x40;
    if (c >= 0x60 && c < 0x80) return c - 0x20;
    if (c >= 0xC0 && c < 0xE0) return c - 0x80;
    return c;
}
static void putc_at(unsigned char x, unsigned char y, unsigned char code, unsigned char colour) {
    unsigned int o = y * stride + x;
    scr[o] = code;
    colbuf[o] = colour;
}
static void put(unsigned char x, unsigned char y, const char *s, unsigned char colour) {
    while (*s && x < cols) putc_at(x++, y, scode((unsigned char)*s++), colour);
}
static void clear_row(unsigned char y) {
    unsigned char x;
    for (x = 0; x < cols; x++) putc_at(x, y, ' ', C_WHITE);
}
static unsigned char putnum(unsigned char x, unsigned char y, unsigned int v, unsigned char colour) {
    char b[6];
    unsigned char i = 0;
    do { b[i++] = '0' + v % 10; v /= 10; } while (v);
    while (i) putc_at(x++, y, b[--i], colour);
    return x;
}

/* screen codes of the map: rock, floor, corridor, door, stairs, walls */
static const unsigned char tile_code[11] = { ' ', '.', '#', '+', '>', 0x40, 0x5D, 0x70, 0x6E, 0x6D, 0x7D };
static const unsigned char tile_lit[11] = { C_BLACK, C_LGREY, C_GREY, C_ORANGE, C_YELLOW,
                                            C_LBLUE, C_LBLUE, C_LBLUE, C_LBLUE, C_LBLUE, C_LBLUE };
static const unsigned char tile_dim[11] = { C_BLACK, C_DGREY, C_DGREY, C_BROWN, C_BROWN,
                                            C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_BLUE, C_BLUE };
static const unsigned char item_code[6] = { ' ', '!', '$', ')', 0x1B, '"' };
static const unsigned char item_col[6] = { 0, C_PURPLE, C_YELLOW, C_CYAN, C_CYAN, C_LRED };

static void draw(void) {
    unsigned char x, y, i;
    /* the map (rows 1-21) */
    for (y = 0; y < MAP_H; y++)
        for (x = 0; x < map_w; x++) {
            unsigned char t = map[y][x];
            if (!seen[y][x]) putc_at(x, y + 1, ' ', C_BLACK);
            else putc_at(x, y + 1, tile_code[t], vis[y][x] ? tile_lit[t] : tile_dim[t]);
        }
    for (i = 0; i < nitem; i++) {
        struct Item *it = &items[i];
        if (it->kind != I_NONE && seen[it->y][it->x])
            putc_at(it->x, it->y + 1, item_code[it->kind], item_col[it->kind]);
    }
    for (i = 0; i < nmon; i++) {
        struct Monster *m = &mons[i];
        if (m->alive() && vis[m->y][m->x])
            putc_at(m->x, m->y + 1, scode((unsigned char)mtypes[m->type].letter), mtypes[m->type].colour);
    }
    putc_at(pl.x, pl.y + 1, 0x00, C_WHITE);           /* @ */
    /* the message line */
    clear_row(0);
    put(0, 0, msg, C_WHITE);
    /* the status lines */
    clear_row(23);
    x = 0;
    put(x, 23, "depth", C_LGREY); x = putnum(6, 23, depth, C_WHITE) + 2;
    put(x, 23, "hp", C_LGREY); x = putnum(x + 3, 23, pl.hp > 0 ? pl.hp : 0, pl.hp * 3 < pl.maxhp ? C_LRED : C_LGREEN);
    put(x, 23, "/", C_LGREY); x = putnum(x + 1, 23, pl.maxhp, C_WHITE) + 2;
    put(x, 23, "atk", C_LGREY); x = putnum(x + 4, 23, attack_power(), C_WHITE) + 2;
    put(x, 23, "def", C_LGREY); x = putnum(x + 4, 23, pl.armour, C_WHITE) + 2;
    put(x, 23, "gold", C_LGREY); x = putnum(x + 5, 23, pl.gold, C_YELLOW) + 2;
    put(x, 23, "lvl", C_LGREY); x = putnum(x + 4, 23, pl.lvl, C_WHITE) + 2;
    if (x + 8 < cols) { put(x, 23, "xp", C_LGREY); putnum(x + 3, 23, pl.xp, C_WHITE); }
    clear_row(24);
    if (state == PLAYING)
        put(0, 24, cols >= 80 ? "crsr/hjkl yubn move  . wait  > down  run/stop quit"
                              : "hjkl yubn move > down", C_LBLUE);
    else
        put(0, 24, "r new game   run/stop quit", C_LBLUE);
    if (state != PLAYING) {
        unsigned char bx = map_w / 2 - 16;
        for (y = 8; y < 14; y++)
            for (x = 0; x < 32; x++) putc_at(bx + x, y, ' ', C_WHITE);
        if (state == DEAD) {
            put(bx + 2, 9, "you were killed by a", C_LRED);
            put(bx + 23, 9, killer, C_LRED);
        } else {
            put(bx + 2, 9, "you escaped with the amulet!", C_YELLOW);
        }
        put(bx + 2, 10, "on depth", C_WHITE);
        putnum(bx + 11, 10, depth, C_WHITE);
        put(bx + 2, 11, "score", C_WHITE);
        putnum(bx + 8, 11, score(), C_YELLOW);
        put(bx + 2, 12, "press r to play again", C_LBLUE);
    }
    dma_copy_far(COLOUR_LIN, (unsigned long)(unsigned int)colbuf, 25 * stride);
}

static unsigned char saved[8];

static void video_on(void) {
    saved[0] = REG(0xD060); saved[1] = REG(0xD061); saved[2] = REG(0xD062); saved[3] = REG(0xD063);
    saved[4] = REG(0xD064); saved[5] = REG(0xD065);
    saved[6] = REG(0xD020); saved[7] = REG(0xD021);
    cols = REG(0xD05E);
    stride = REG(0xD058) | (REG(0xD059) << 8);
    if (cols < 40) cols = 40;
    if (cols > 80) cols = 80;
    if (stride > 80 || stride < cols) stride = cols;
    map_w = cols >= 80 ? 80 : 40;
    __dma_fill(scr, 2000, ' ');
    __dma_fill(colbuf, 2000, C_WHITE);
    REG(0xD060) = (unsigned char)(unsigned int)scr;
    REG(0xD061) = (unsigned char)((unsigned int)scr >> 8);
    REG(0xD062) = 0;
    REG(0xD063) = REG(0xD063) & 0xF0;
    REG(0xD064) = 0x00;
    REG(0xD065) = 0x08;
    REG(0xD020) = 0;
    REG(0xD021) = 0;
}
static void video_off(void) {
    REG(0xD060) = saved[0]; REG(0xD061) = saved[1]; REG(0xD062) = saved[2]; REG(0xD063) = saved[3];
    REG(0xD064) = saved[4]; REG(0xD065) = saved[5];
    REG(0xD020) = saved[6]; REG(0xD021) = saved[7];
}

static void wait_frame(void) {
    while (REG(0xD012) == 0) ;
    while (REG(0xD012) != 0 || (REG(0xD011) & 0x80)) ;
}

/* the next command: a key from the typing queue or the joystick in port 2 */
static unsigned char joy_last, joy_wait;
static unsigned char get_command(void) {
    unsigned char c, j;
    for (;;) {
        wait_frame();
        rnd16();                          /* the time between keys stirs the dice */
        if ((c = input_key()) != 0) {
            switch (c) {
                case CH_CURSOR_UP: case 'k': case 'K': return CMD_N;      /* cursor up */
                case CH_CURSOR_DOWN: case 'j': case 'J': return CMD_S;      /* cursor down */
                case CH_CURSOR_RIGHT: case 'l': case 'L': return CMD_E;      /* cursor right */
                case CH_CURSOR_LEFT: case 'h': case 'H': return CMD_W;      /* cursor left */
                case 'u': case 'U': return CMD_NE;
                case 'y': case 'Y': return CMD_NW;
                case 'n': case 'N': return CMD_SE;
                case 'b': case 'B': return CMD_SW;
                case '.': case ' ': return CMD_WAIT;
                case '>': return CMD_DOWN;
                case 'r': case 'R': return CMD_RESTART;
                case CH_STOP: return CMD_QUIT;                         /* RUN/STOP */
            }
            continue;
        }
        /* joystick 2 */
        j = input_joy(2);
        if (!j) { joy_last = 0; continue; }
        if (j == joy_last && joy_wait) { joy_wait--; continue; }
        joy_wait = j == joy_last ? 6 : 14;     /* held: repeat */
        joy_last = j;
        if (j & JOY_FIRE) return CMD_DOWN;
        if ((j & 5) == 5) return CMD_NW;
        if ((j & 9) == 9) return CMD_NE;
        if ((j & 6) == 6) return CMD_SW;
        if ((j & 10) == 10) return CMD_SE;
        if (j & 1) return CMD_N;
        if (j & 2) return CMD_S;
        if (j & 4) return CMD_W;
        if (j & 8) return CMD_E;
    }
}

int main(void) {
    unsigned char c;
    __asm__("sei");
    rs ^= REG(0xD012) | (REG(0xDC04) << 8);
    if (rs == 0) rs = 1;
    video_on();
    input_flush();
    new_game();
    for (;;) {
        draw();
        c = get_command();
        if (c == CMD_QUIT) break;
        if (state != PLAYING) {
            if (c == CMD_RESTART) new_game();
            continue;
        }
        msg_len = 0; msg[0] = 0;
        do_command(c);
    }
    video_off();
    input_flush();
    __asm__("cli");
    return 0;
}

#else
/* ------------------------------------------------------------------------ */
/* The test: results at $C0C0 (01 = pass)                                   */
/*   [0] every passable square reachable from the player (flood fill)       */
/*   [1] the player on the floor, the stairs on their square (the amulet on */
/*       the last depth instead)                                            */
/*   [2] monsters and items on the floor, none on another, the player or    */
/*       the stairs                                                         */
/*   [3] scripted moves: not into a wall, onto the floor; a fight to the    */
/*       end (the monster dies, experience gained)                          */
/*   [4] pick-ups: a potion heals, gold counts                              */
/*   [5] 3000 random commands over the levels: the player and the monsters */
/*       always on passable squares, never two on one                      */
/*   [6] $AA sentinel                                                       */

#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned char fill_x[MAP_W * 2], fill_y[MAP_W * 2];

/* passable squares not reached from the player */
static unsigned int unreached(void) {
    unsigned int n = 0, head = 0, tail = 0;
    unsigned char x, y, k;
    static const signed char ddx[4] = { 1, -1, 0, 0 }, ddy[4] = { 0, 0, 1, -1 };
    for (y = 0; y < MAP_H; y++)
        for (x = 0; x < MAP_W; x++) vis[y][x] = 0;
    vis[pl.y][pl.x] = 1;
    fill_x[0] = pl.x; fill_y[0] = pl.y; tail = 1;
    while (head != tail) {
        x = fill_x[head]; y = fill_y[head];
        head = head + 1 == MAP_W * 2 ? 0 : head + 1;
        for (k = 0; k < 4; k++) {
            unsigned char nx = x + ddx[k], ny = y + ddy[k];
            if (nx >= map_w || ny >= MAP_H || vis[ny][nx] || !PASSABLE(map[ny][nx])) continue;
            vis[ny][nx] = 1;
            fill_x[tail] = nx; fill_y[tail] = ny;
            tail = tail + 1 == MAP_W * 2 ? 0 : tail + 1;
        }
    }
    for (y = 0; y < MAP_H; y++)
        for (x = 0; x < map_w; x++)
            if (PASSABLE(map[y][x]) && !vis[y][x]) n++;
    return n;
}

int main(void) {
    unsigned char ok0 = 1, ok1 = 1, ok2 = 1, ok3 = 1, ok4 = 1, s, i, k, w;
    map_w = MAP_W;
    for (s = 0; s < 16; s++) {
        rs = 0x1357 + s * 2741;
        depth = 1 + (s & 7);
        gen_level();
        if (unreached()) ok0 = 0;
        if (map[pl.y][pl.x] != T_FLOOR) ok1 = 0;
        if (depth < WIN_DEPTH) {
            if (map[stairs_y][stairs_x] != T_STAIRS || (stairs_x == pl.x && stairs_y == pl.y)) ok1 = 0;
        } else {
            for (k = 0, i = 0; i < nitem; i++) if (items[i].kind == I_AMULET) k = 1;
            if (!k) ok1 = 0;
        }
        for (i = 0; i < nmon; i++) {
            struct Monster *m = &mons[i];
            if (map[m->y][m->x] != T_FLOOR || thing_at(m, pl.x, pl.y) || thing_at(m, stairs_x, stairs_y)) ok2 = 0;
            for (k = 0; k < nmon; k++) if (k != i && thing_at(&mons[k], m->x, m->y)) ok2 = 0;
            for (k = 0; k < nitem; k++) if (thing_at(&items[k], m->x, m->y)) ok2 = 0;
        }
        for (i = 0; i < nitem; i++) {
            struct Item *it = &items[i];
            if (map[it->y][it->x] != T_FLOOR || thing_at(it, pl.x, pl.y) || thing_at(it, stairs_x, stairs_y)) ok2 = 0;
            for (k = 0; k < nitem; k++) if (k != i && thing_at(&items[k], it->x, it->y)) ok2 = 0;
        }
    }
    OUT[0] = ok0; OUT[1] = ok1; OUT[2] = ok2;

    /* scripted moves in a room: its top-left corner square */
    rs = 0x4242; depth = 1;
    new_game();
    for (i = 0; i < 9 && (rooms[i].gone || rooms[i].x2 - rooms[i].x1 < 2); i++) ;
    nmon = 0; nitem = 0;
    pl.x = rooms[i].x1; pl.y = rooms[i].y1;
    if (do_command(CMD_N) || pl.y != rooms[i].y1) ok3 = 0;       /* the wall */
    if (do_command(CMD_W) || pl.x != rooms[i].x1) ok3 = 0;
    if (!do_command(CMD_E) || pl.x != rooms[i].x1 + 1) ok3 = 0;  /* the floor */
    if (!do_command(CMD_W) || pl.x != rooms[i].x1) ok3 = 0;
    /* a fight: a kobold to the east, awake */
    mons[0].x = pl.x + 1; mons[0].y = pl.y; mons[0].type = 1; mons[0].hp = 20; mons[0].awake = 1;
    nmon = 1;
    pl.hp = pl.maxhp = 500;
    k = pl.xp;
    for (w = 0; w < 200 && mons[0].hp > 0; w++) {
        do_command(CMD_E);
        if (mons[0].hp > 0 && pl.x != rooms[i].x1) ok3 = 0;    /* it is in the way */
    }
    if (mons[0].hp > 0 || pl.xp == k || pl.hp >= 500) ok3 = 0;   /* dead, xp, it hit back */
    if (state != PLAYING) ok3 = 0;
    OUT[3] = ok3;

    /* pick-ups */
    nmon = 0;
    items[0].x = pl.x + 1; items[0].y = pl.y; items[0].kind = I_POTION;
    items[1].x = pl.x;     items[1].y = pl.y + 1; items[1].kind = I_GOLD; items[1].value = 77;
    nitem = 2;
    pl.hp = 1; pl.maxhp = 30; k = pl.gold;
    do_command(CMD_E);
    if (pl.hp <= 1 || items[0].kind != I_NONE) ok4 = 0;
    do_command(CMD_W);
    if (rooms[i].y2 > rooms[i].y1) {
        do_command(CMD_S);
        if (pl.gold != k + 77 || items[1].kind != I_NONE) ok4 = 0;
    }
    OUT[4] = ok4;

    /* a random walk through the game (going down whenever on the stairs) */
    {
        unsigned char ok5 = 1, c;
        unsigned int n;
        rs = 0x7777;
        new_game();
        for (n = 0; n < 3000; n++) {
            if (state != PLAYING) new_game();
            if (n % 300 == 299 && depth < WIN_DEPTH && mon_at(stairs_x, stairs_y) == 255) {
                pl.x = stairs_x; pl.y = stairs_y;    /* on to the next depth */
            }
            if (pl.x == stairs_x && pl.y == stairs_y) c = CMD_DOWN;
            else if (rnd_n(2)) {
                /* towards the stairs */
                signed char dx = sgn((signed int)stairs_x - pl.x), dy = sgn((signed int)stairs_y - pl.y);
                for (c = CMD_N; c <= CMD_SW && !(cmd_dx[c] == dx && cmd_dy[c] == dy); c++) ;
                if (c > CMD_SW) c = CMD_WAIT;
            } else c = CMD_N + rnd_n(9);
            pl.hp = pl.maxhp;                      /* keep it going */
            msg_len = 0;
            do_command(c);
            if (!PASSABLE(map[pl.y][pl.x])) ok5 = 0;
            for (i = 0; i < nmon; i++) {
                struct Monster *m = &mons[i];
                if (!m->alive()) continue;
                if (!PASSABLE(map[m->y][m->x]) || thing_at(m, pl.x, pl.y)) ok5 = 0;
                for (k = i + 1; k < nmon; k++) if (mons[k].alive() && thing_at(&mons[k], m->x, m->y)) ok5 = 0;
            }
        }
        OUT[5] = ok5;
        OUT[7] = depth;                            /* how deep the walk got (8) */
    }
    OUT[6] = 0xAA;
    return 0;
}
#endif

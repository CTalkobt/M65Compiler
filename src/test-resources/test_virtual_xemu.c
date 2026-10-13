/* test_virtual_xemu.c — virtual dispatch through the vtable
 *
 * Every object of a struct with virtual methods has its __vt pointer set
 * to its struct's vtable: locals (after their initializer), globals and
 * statics (in the initial image), array elements and nested members.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] local base and derived objects
 *   [1] inherited (not overridden) method, derived without own overrides,
 *       sizeof of derived structs (one __vt, the parent's members included)
 *   [2] global objects (with and without an initializer)
 *   [3] static local, local with an initializer
 *   [4] array elements
 *   [5] nested member
 *   [6] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

struct Animal {
    int legs;
    virtual int sound() { return 1; }
    virtual int count() { return this->legs; }
};
struct Dog : Animal {
    virtual int sound() { return 2; }
};
struct Puppy : Dog {
    int age;
};
struct Kennel {
    int id;
    struct Dog resident;
};

static int snd(struct Animal *a) { return a->sound(); }
static int cnt(struct Animal *a) { return a->count(); }

struct Dog gd;
struct Dog gdi = {4};
struct Animal ga;

int main(void) {
    struct Animal a;
    struct Dog d;
    struct Puppy p;
    static struct Dog sd;
    struct Dog di = {3};
    struct Dog arr[3];
    struct Kennel k;
    unsigned char i, ok;

    a.legs = 2;
    d.legs = 4;
    p.legs = 5;
    OUT[0] = snd(&a) == 1 && snd((struct Animal *)&d) == 2;
    OUT[1] = cnt((struct Animal *)&d) == 4 && snd((struct Animal *)&p) == 2 && cnt((struct Animal *)&p) == 5 &&
             sizeof(struct Animal) == 4 && sizeof(struct Dog) == 4 && sizeof(struct Puppy) == 6;
    OUT[2] = snd((struct Animal *)&gd) == 2 && snd((struct Animal *)&gdi) == 2 && snd(&ga) == 1;
    OUT[3] = snd((struct Animal *)&sd) == 2 && snd((struct Animal *)&di) == 2;
    ok = 1;
    for (i = 0; i < 3; i++) {
        arr[i].legs = i;
        if (snd((struct Animal *)&arr[i]) != 2 || cnt((struct Animal *)&arr[i]) != i) ok = 0;
    }
    OUT[4] = ok;
    k.resident.legs = 7;
    OUT[5] = snd((struct Animal *)&k.resident) == 2 && cnt((struct Animal *)&k.resident) == 7;
    OUT[6] = 0xAA;
    return 0;
}

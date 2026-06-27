typedef int my_int;
typedef char* string;
typedef struct {
    int x;
    int y;
} Point;

typedef my_int alias_int;

int main() {
    my_int a = 10;
    alias_int b = 20;
    string s = "hello";
    Point p;
    p.x = a;
    p.y = b;

    typedef int local_int;
    local_int c = 30;

    // Typedef pointer in for loop (declared before loop per C89)
    typedef struct { int id; int val; } tsCmds;
    tsCmds cmds[3];
    cmds[0].id = 1; cmds[0].val = 10;
    cmds[1].id = 2; cmds[1].val = 20;
    cmds[2].id = 3; cmds[2].val = 30;

    int sum = 0;
    tsCmds *pCmd;
    int i;
    for (i = 0; i < 3; i++) {
        pCmd = &cmds[i];
        sum += pCmd->val;
    }
    // sum should be 60
    c = sum - 60;

    if (a + b + c + 30 == 60) {
        return 0; // PASS
    }
    return 1; // FAIL
}

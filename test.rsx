struct Location {
    int x = 0;
    int y = 0;
    int z = 0;
};

Location loc;
Location old_loc;

int flag = 1;

loc.x = -/data get entity @s Pos[0] 1;
loc.y = -/data get entity @s Pos[1] 1;
loc.z = -/data get entity @s Pos[2] 1;

while (flag) {
    old_loc.x = loc.x;
    old_loc.y = loc.y;
    old_loc.z = loc.z;
    loc.x = -/data get entity @s Pos[0] 1;
    loc.y = -/data get entity @s Pos[1] 1;
    loc.z = -/data get entity @s Pos[2] 1;
    if ((old_loc.x != loc.x) || (old_loc.y != loc.y) || (old_loc.z != loc.z)) {
        -/say 你移动了!!!!!!;
        flag = 0;
    }
    wait 5;
}
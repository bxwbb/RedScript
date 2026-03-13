-/say 游戏程序开始运行;
struct Location {
    int x = 0;
    int y = 0;
    int z = 0;
};
-/say 位置结构体创建完成;

Location loc;
Location old_loc;
-/say 位置结构体实例化完成;

int flag = 1;
-/say 循环变量创建完成;

loc.x = -/data get entity @s Pos[0] 1;
loc.y = -/data get entity @s Pos[1] 1;
loc.z = -/data get entity @s Pos[2] 1;
-/say 获取位置完成;

int target_x = loc.x + 20;
int count = 0;
int move_type = 1;
-/say 移动参数创建完成;

while (flag) {
    old_loc.x = loc.x;
    old_loc.y = loc.y;
    old_loc.z = loc.z;
    loc.x = -/data get entity @s Pos[0] 1;
    loc.y = -/data get entity @s Pos[1] 1;
    loc.z = -/data get entity @s Pos[2] 1;
    if (((old_loc.x != loc.x) || (old_loc.y != loc.y) || (old_loc.z != loc.z)) && (move_type == 0)) {
        -/say 你动了，游戏失败;
        flag = 0;
    }
    if (loc.x >= target_x) {
        -/say 你赢了;
        flag = 0;
    }
    if (count >= 10) {
        move_type = !move_type;
        count = 0;
    }
    if (count == 0) {
        if (move_type) {
            -/say 你现在可以移动;
        } else {
            -/say 你现在不可以移动;
        }
    }
    count++;
    wait 5;
}
-/say 程序循环结束;
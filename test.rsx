struct Location {
   int x = 0;
   int y = 5;   // 默认值为0
   int z = 5;
};

Location player_location;  // 创建结构体实例(无参)
$/say 玩家的坐标对象$(player_location); // 打印结构体实例的指针
$/say 玩家的z坐标为$(player_location.z);  // 打印结构体实例的属性

struct Player {
    Location location;   // 结构体嵌套
    int time = 0;
};

Player player;
player.time = 100;   // 结构体属性赋值
player.location.x = 10;
$/say 玩家$(player.time)秒内的位置是$(player.location.x)，$(player.location.y)，$(player.location.z);
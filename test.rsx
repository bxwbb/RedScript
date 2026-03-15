struct Location {
    int x = 0;
    int y = 0;
    int z = 0;
};

Location loc0;
Location loc1;

loc1.x = 10;
loc1.y = 20;
loc1.z = 30;

-/say 交换前:;
$/say X: $(loc0.x),Y: $(loc0.y),Z: $(loc0.z);
$/say X: $(loc1.x),Y: $(loc1.y),Z: $(loc1.z);

loc0 = loc1;

-/say 交换后:;
$/say X: $(loc0.x),Y: $(loc0.y),Z: $(loc0.z);
$/say X: $(loc1.x),Y: $(loc1.y),Z: $(loc1.z);
# RedScript

RedScript 是一种可以编译成 Minecraft 数据包的脚本语言，它可以让编写 Minecraft 函数变得更加简单和高效。

## 提示
这个项目目前还处于开发阶段，所以我没有进行任何优化，可能有些生成的命令很多余，但是结果是正确的。
在将来，我将会进行优化，使生成的命令更精简。
这个编译器不太稳定，如果遇到BUG可以反馈，感谢反馈。

## ✨ 特性

- **语法**：易于学习的语法，支持变量声明、循环、条件判断等
- **直接编译**：一键编译生成 Minecraft 数据包
- **宏命令支持**：在命令中嵌入变量，实现动态命令生成
- **自动环境配置**：自动生成数据包所需的目录结构和配置文件
- **版本支持**：支持 Minecraft 1.21+ (pack_format 71)

## 🚀 快速开始

### 使用方法

#### 编译 RedScript 文件

main.exe 你的脚本.rsx

编译器会自动：
1. 创建以文件名命名的数据包文件夹
2. 生成 `pack.mcmeta` 配置文件
3. 创建必要的目录结构
4. 生成可能会使用到的工具函数
5. 将 `.rsx` 文件编译成 `.mcfunction` 文件

#### 查看版本

main.exe --version
main.exe --v

## 📖 语言语法

### 变量声明

使用 `let` 关键字声明变量：

```RedScript
let x = 0;
let off_y = -5;
```

### 算术运算

支持基本的算术运算符：

```RedScript
let a = 5 + 3; // 加法 
let b = 10 - 4; // 减法 
let c = 6 * 7; // 乘法 
let d = 20 / 4; // 除法 
off_y = off_y - 1; // 赋值
```

### 自增自减

```RedScript
x++; // 自增 1 
y--; // 自减 1
```

### 循环

#### for 循环

```RedScript
for (let i = 0; i < 10; i++) {
    // 循环体 
}

for (语句;条件表达式;递增表达式) {
    // 循环体 
}
```

**示例：生成红石方块平台**

```RedScript
let off_y = -5;
for (let x = -5; x <= 5; x++) {
    for (let z = -5; z <= 5; z++) {
        $/fill ~$(x) ~$(off_y) ~$(z) ~$(x) ~$(off_y) ~$(z) redstone_block;
    } off_y = off_y;
}
```

**示例：生成二次函数曲线**

```RedScript
for (let x = -5; x <= 5; x++) {
    $/fill ~$(x) ~$(x * x) ~-5 ~$(x) ~$(x * x) ~-5 redstone_block;
}
```

#### while 循环

```RedScript
while (条件表达式) {
    // 循环体 
}
```

### 条件判断

```RedScript
if (score >= 10) {
    // 代码块 
} elif (score >= 5) {
    // 代码块 
} else {
    // 代码块 
}
```
由于目前只有整型，所以现在0为假，非0为真。

### 比较运算符

| 运算符  | 说明   | 示例            |
|------|------|---------------|
| `==` | 等于   | `if (a == b)` |
| `!=` | 不等于  | `if (a != b)` |
| `<`  | 小于   | `if (a < b)`  |
| `>`  | 大于   | `if (a > b)`  |
| `<=` | 小于等于 | `if (a <= b)` |
| `>=` | 大于等于 | `if (a >= b)` |

### 逻辑运算符

| 运算符    | 说明 | 示例                        |
|--------|----|---------------------------|
| `&&`   | 与  | `if (a > 0 && b < 10)`    |
| `\|\|` | 或  | `if (a == 0 \|\| b == 0)` |
| `!`    | 非  | `if (!condition)`         |

### Minecraft 命令

#### 普通命令

使用 `-/` 前缀执行原生 Minecraft 命令：

```RedScript
-/give @p diamond 1;
 -/tp @a 0 100 0;
```

#### 宏命令

使用 `$/` 前缀和 `$(变量)` 语法在命令中使用变量：

```RedScript
let x = 5; 
let y = 64; 
$/fill ~$(x) ~$(y) ~0 ~$(x) ~10 ~0 stone;
```

宏会在编译时展开为实际的 Minecraft 函数宏语法。

### 作用域

使用花括号定义作用域：

```RedScript
{
    let local_var = 10; 
    // local_var 只在这个作用域内有效 
}
```

### outline 作用域

使用 `outline` 关键字定义独立的作用域（会生成独立的函数文件）：

```RedScript
outline { 
    // 这个代码块会被编译成独立的 .mcfunction 文件
    let temp = 5;
    -/say Hello from outline!;
}
```

### 程序退出

```RedScript
exit(0); // 退出程序
```

### wait 等待(警告：不稳定)
```RedScript
wait 10; // 等待10 tick
```

### 断言
```RedScript
assert 10 == 9; // 不成立时触发断言，在游戏中显示消息并退出程序
```

### null赋值
```RedScript
null = a++; // 执行表达式带来的副作用并抛弃其返回值
a++;  // 若直接写表达式，则编译器会自动加上null =
```

### struct结构体
```RedScript
struct Location {
   int x = 0;
   int y = 0;  // int类型必须有默认值
   int z = 5;
};

Location player_location;  // 创建结构体实例(无参)
$/say 玩家的坐标对象$(player_location); // 打印结构体实例的指针
$/say 玩家的z坐标为$(player_location.z);  // 打印结构体实例的属性

struct Player {
   int time;
   Location location;   // 结构体嵌套
};

Player player;
player.time = 100;   // 结构体属性赋值
player.location.x = 10;
$/say 玩家$(player.time)秒内的位置是$(player.location.x)，$(player.location.y)，$(player.location.z);
```

## 💡 完整示例

### 示例 1：生成平台

```RedScript
for (let x = -5; x <= 5; x++) {
    for (let z = -5; z <= 5; z++) {
        $/setblock ~$(x) ~ ~$(z) stone; 
    } 
}
```

### 示例 2：复杂计算

```RedScript
let base_y = 64; let height = 10;
for (let i = 0; i < height; i++) {
    let current_y = base_y + i;
    $/setblock ~0 ~$(current_y) ~0 gold_block;
}
```
### 示例 4：斐波那契数列
```RedScript
let a = 0; let b = 1;
for (let i = 0; i < 10; i++) {
    $/tellraw @a "Fibonacci: $(a)";
    let temp = a + b;
    a = b;
    b = temp; 
}
```

### 示例 5：移动检测器
```RedScript
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
```

## 🔧 技术细节

### 编译器架构

RedScript 编译器包含三个主要阶段：

1. **词法分析** (`tokenization.h`)
   - 将源代码转换为 token 流
   - 支持单行注释 `//` 和多行注释 `/* */`
   - 识别关键字、标识符、字面量、运算符等

2. **语法分析** (`parser.h`)
   - 解析 token 流生成抽象语法树 (AST)
   - 使用递归下降分析法
   - 支持表达式优先级

3. **代码生成** (`generation.h`)
   - 遍历 AST 生成 Minecraft 函数代码
   - 管理变量存储和作用域
   - 优化命令输出

### 内存管理

使用自定义的 Arena 分配器 (`arena.h`) 进行高效的内存管理：
- 一次性分配大块内存
- 避免频繁的 malloc/free 调用
- 编译完成后统一释放

### 栈虚拟机

RedScript 使用基于 NBT 存储的栈来管理变量和表达式求值：

- **变量存储**: `storage minecraft:__<project>.__stack` 数组
- **临时寄存器**: `rax`, `rbx`, `r` 等 scoreboard 虚拟对象
- **运算方式**: `scoreboard players operation rax __proj += rbx __proj`
- **NBT 转换**: `execute store result storage minecraft:__proj rax int 1 run scoreboard players get rax __proj`

### 作用域管理

- 使用 `begin_scope()` 和 `end_scope()` 管理变量生命周期
- 离开作用域时自动清理栈上的局部变量
- 支持嵌套作用域

## 📋 构建要求

- **CMake**: 4.1+
- **C++ 标准**: C++23
- **编译器**: GCC / Clang / MSVC (支持 C++23)
- **操作系统**: Windows / Linux / macOS

## 🛠️ 开发指南
### 添加新特性

1. **新关键字**: 在 `tokenization.h` 中添加词法规则
2. **新语法**: 在 `parser.h` 中添加解析逻辑
3. **代码生成**: 在 `generation.h` 中添加对应的生成代码

### 调试技巧

- 编译后的 `.mcfunction` 文件带有注释，便于调试
- 使用 `print_error()` 函数报告错误
- 检查生成的数据包结构是否正确

## 📊 当前版本

**v0.1.0**

### 已实现功能
- ✅ 变量声明和赋值
- ✅ 算术和逻辑运算
- ✅ 条件语句 (if/elif/else)
- ✅ 循环语句 (for/while)
- ✅ 自增自减运算符
- ✅ Minecraft 命令宏
- ✅ 作用域管理
- ✅ outline 独立作用域

### 计划功能
- ⏳ 函数定义
- ⏳ 数组支持
- ⏳ 字符串操作
- ⏳ 更多内置命令

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

### 贡献方式
1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

## 📄 许可证

本项目采用开源许可证。

## 🙏 致谢

感谢 Minecraft 社区和所有贡献者。

https://www.youtube.com/watch?v=vcSijrRsrY0
感谢这位youtober的灵感和基础框架支持，虽然你已经两年没发视频了，请快点回归。

---

**Happy Coding!** 🎮



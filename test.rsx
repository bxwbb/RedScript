let a = 0; let b = 1;
for (let i = 0; i < 10; i++) {
    $/tellraw @a "Fibonacci: $(a)";
    let temp = a + b;
    a = b;
    b = temp;
}
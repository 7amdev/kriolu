klasi Forma {}
mimoria circulo = Forma();
circulo.raio = 5;
imprimi circulo.raio; 

mimoria a = 10;
mimoria b;
b = a + circulo.raio * 2;
// b = a + circulo.raio = 10; // expect error: Invalid placement...
imprimi b; 

// Test: Assignment
//
// mimoria a;
// mimoria b;
// mimoria c;
// a = b = c = 10 + 10 * 2;
// a = b = c = 2 ^ 3 ^ 2;
// a = 10 * b = 2 + 4; // expect Error: Invalid placement...
// imprimi a;
// imprimi b;
// imprimi c;

// Test: Rigth Associativity
//
// mimoria a = 2 ^ 3 ^ 2;
// imprimi a;

// Test: Left and Rigth Associativity
// 
// mimoria a = 10 + 2 * 2 ^ 3 ^ 2;
// imprimi a;

// Test: Arithmetic Precedence
//
// mimoria e1 = (1 + 2) * 3; 
// mimoria e2 = 2 + 3 - 4 * 5 / 6 ^ 7 * 8 + 9 - 10;
// mimoria e3 = (1 + 2) * 3 - 4 / 5; 
// mimoria e4 = 1 * 2 ^ 2;
// mimoria e5 = (-2) ^ 2;
// mimoria e6 = 2^2;
// mimoria e7 = (-1 * 2) ^ 2;
// mimoria e8 = 1 ^ 2 ^ 3 ^ 4;
// imprimi e1;
// imprimi e2;
// imprimi e3;
// imprimi e4;
// imprimi e5;
// imprimi e6;
// imprimi e7;
// imprimi e8;
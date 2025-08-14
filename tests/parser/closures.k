// Test::1
//
// mimoria imprimi_1; 
// mimoria imprimi_2;
// funson kria_closure() {
//   pa (mimoria i = 1; i <= 2; i = i + 1) {
//     funson imprimi_x()  {
//       imprimi i;
//     } 
// 
//     si (i == 1) imprimi_1 = imprimi_x;
//     si (i == 2) imprimi_2 = imprimi_x;
//   }
// }
// 
// kria_closure();
// imprimi_1();
// imprimi_2();

// Test::Object using a message-based pattern and closure
// 
funson Vetor2(x, y) {
  funson objetu(mensagem) {
    funson soma(v2_other) {
      divolvi Vetor2( x + v2_other("x"), y + v2_other("y") );
    }

    si (mensagem == "x")    divolvi x;
    si (mensagem == "y")    divolvi y;
    si (mensagem == "soma") divolvi soma;

    imprimi "Eru: opson ka ta existe!"; 
  }

  divolvi objetu;
}

mimoria a = Vetor2(1, 2);
mimoria b = Vetor2(3, 4);
mimoria c = a("soma")(b);

imprimi a("x");
imprimi a("y");

imprimi c("x");
imprimi c("y");

// Test::2
//
// mimoria x = "global";
// funson Vetor() {
//   mimoria x = 10;
//   mimoria y = 5;
// 
//   funson imprimi_x() {
//     imprimi x;
//   }
// 
//   divolvi imprimi_x;
// }
//
// mimoria print_x = Vetor();
// print_x();

// Test::3
//
// mimoria x = "global";
// funson exterior() {
//  mimoria x = "exterior";
//  funson interno() {
//    imprimi x;
//  }
//  interno();
// }
// exterior();

// Test::4
// 
// funson outer() {
//   mimoria a = 1;
//   mimoria b = 2;
//   funson middle() {
//     mimoria c = 3;
//     mimoria d = 4;
//     funson inner() {
//       imprimi a + c + b + d;
//     }
//     inner();
//   }
//   middle();
// }
// outer();

// Test::5
//
// mimoria ret; 
// {
//     mimoria a = 1;
//     mimoria b = 2;
//     funson sum() {
//         imprimi a + b;
//     }
//     ret = sum;
// }
// ret();

// Test: Linked-List vm->openValues
// Expect: vm->openValues -> v_a: 3 -> v_a: 2 -> v_a: 1 -> NULL
// 'v_a' means value_address
// 
// mimoria c_h;
// {
//   mimoria a = 1;
//   funson f() {
//     imprimi a;
//   }
//   mimoria b = 2;
//   funson g() {
//     imprimi b;
//   }
//   mimoria c = 3;
//   mimoria d = 4;
//   funson h() {
//     imprimi d + b;
//   }
//   mimoria e = 5;
//   funson j() {
//     imprimi c + e;
//   }
// 
//   c_h = h; 
// }
// 
// c_h();

// Test::6
//
// mimoria j;
// {
//   mimoria a = 1;
//   funson f() {
//     imprimi a;
//   }
// 
//   mimoria b = 2;
//   mimoria c = 3;
//   funson g() {
//     imprimi c;
//   }
// 
//   mimoria d = 4;
//   funson h() {
//     imprimi b + d;
//   }
//   
//   j = h;
// }
// 
// j();
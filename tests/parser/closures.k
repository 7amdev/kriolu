mimoria imprimi_1; 
mimoria imprimi_2;
funson kria_closure() {
  pa (mimoria i = 1; i <= 2; i = i + 1) {
    funson imprimi_x()  {
      imprimi i;
    } 

    si (i == 1) imprimi_1 = imprimi_x;
    si (i == 2) imprimi_2 = imprimi_x;
  }
}

kria_closure();
imprimi_1();
imprimi_2();

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

// Test: 1
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

// Test: 2
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

// Test: 3
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
//     imprimi c + d;
//   }
// 
//   c_h = h; 
// }
// 
// c_h();

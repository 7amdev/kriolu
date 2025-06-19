funson a() { b(); }
funson b() { c(); }
funson c() {
  c("too", "many");
}

a();

// funson sum(a, b, c) {
//     divolvi a + b + c;
// }
// imprimi 4 + sum(1, 2, 3);
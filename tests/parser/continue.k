pa (mimoria i = 0; i < 2; i = i + 1) {
  imprimi "one";
  pa (mimoria j = 0; j < 2; j = j + 1) {
    imprimi "two";
    salta;
    imprimi "two.one";
  }
  salta;
  imprimi "one.one";
}

// pa (mimoria i = 0; i < 2; i = i + 1) {
//    imprimi "one";
//    si (i == 1) sai;
//    imprimi "two";
//    salta;
//    imprimi "three";
// }
// mimoria x = "global";
// funson exterior() {
//  mimoria x = "exterior";
//  funson interno() {
//    imprimi x;
//  }
//  interno();
// }
// exterior();

funson outer() {
  mimoria a = 1;
  mimoria b = 2;
  funson middle() {
    mimoria c = 3;
    mimoria d = 4;
    funson inner() {
      imprimi a + c + b + d;
    }
    inner();
  }
  middle();
}
outer();
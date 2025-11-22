funson vetor_2(x, y) {
  $break
  mimoria d = 5;

  funson objetu(msg) {
    funson soma(other) { 
      divolvi other("x") + x;
    }

    si (msg == "x") divolvi x;
    si (msg == "x") divolvi y;

    imprimi "[ERROR] opson ka ezisti";
  }

  divolvi objetu;
}

klasi Forma {
  $break
  muda_posison(x, dx) {
    imprimi x + dx;
    $break
  }
}

klasi Circulo < Forma $break {
  konstrutor(x, y, raiu) {
    keli.x = x;
    keli.y = y;
    keli.raiu = raiu;
  }
  altera_posison(dx) {
    Forma.muda_posison(keli.x, dx);
  }
}

klasi Kuadrilateru < Forma {
  konstrutor(x, y, largura, altura) {
    keli.x       = x;
    keli.y       = y;
    keli.largura = largura;
    keli.altura  = altura;
  }
  altera_posison(dx) {
    Forma.muda_posison(keli.x, dx);
  }
  area() {
    divolvi keli.largura * keli.altura;
  }
}

mimoria retangulo = Kuadrilateru{5, 10, 40, 50};
mimoria circ      = Circulo{5, 15, 8};

retangulo.altera_posison(8);
circ.altera_posison(10);
imprimi circ.x;
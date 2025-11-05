mimoria nomi = "Alfredo";
klasi Forma {
    muda_posison(x, dx) {
        imprimi x + dx;
    }
}

klasi Circulo < Forma {
    konstrutor(x) {
        keli.x = x;
    }
    altera_posison(dx) {
        riba.muda_posison(keli.x, dx);
    }
}

mimoria circ = Circulo(5);
circ.altera_posison(10);
imprimi circ.x;

// klasi Forma {
//     konstrutor(x) {
//         keli.x = x;
//     }
//     muda_posison(px) {
//         keli.x = px;
//     }
// }
// 
// klasi Circulo < Forma {
//     konstrutor(x) {
//         riba.muda_posison(x); // doesn't make much sense
//     }
// }
// 
// mimoria F = Forma(69);
// mimoria circ = Circulo(10);
// imprimi F.x;
// imprimi circ.x;

// klasi Forma {
//     konstrutor(px, py) {
//         keli.x = px;
//         keli.y = py;
//     }
//     muda_posison(x, y) {
//         keli.x = keli.x + x;
//         keli.y = keli.y + y;
//     }
// }
// 
// klasi Circulo < Forma {
//     konstrutor(x, y, raio) {
//         keli.x    = x;
//         keli.y    = y;
//         keli.raio = raio;
//     }
//     area() {
//         divolvi 3.14 * keli.raio * keli.raio;
//     }
//     altera_posison(dx, dy) {
//         riba.muda_posison(dx, dy);
//     }
// }
// 
// mimoria f = Forma(10, 15);
// mimoria c = Circulo(5, 8, 25);
// imprimi c.area();
// c.altera_posison(5, 2);
// imprimi "Posison: ";
// imprimi c.x;
// imprimi c.y;
// 
// klasi kuadradu < Forma {
//     konstrutor(x, y, largura, altura) {
//         keli.x = x;
//         keli.y = y;
//         keli.largura = largura;
//         keli.altura = altura;
//     }
//     area() {
//         divolvi keli.largura * keli.altura;
//     }
// }
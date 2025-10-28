
klasi Animal {
    bua() {}
    anda(distance, speed) {
        imprimi distance * speed;
    }
    imprimi_velovidade() {
        imprimi "velocidade: " + keli.velocidade;
    }
}

mimoria gatu = Animal();
gatu.velocidade = "10";
gatu.nomi = "Mimi";
imprimi gatu.nomi;
gatu.anda(10, 4);
gatu.imprimi_velovidade();

// funson Forma_2D() {
//     klasi Forma {}
//     mimoria circulo = Forma();
//     circulo.raio = 5;
//     imprimi circulo.raio; 
// }
// Forma_2D();

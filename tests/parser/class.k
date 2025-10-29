klasi Animal {
    konstrutor(velocidade, distancia) {
        keli.vel = velocidade;
        keli.dis = distancia;
    }
    bua() {}
    anda(distance, speed) {
        imprimi distance * speed;
    }
    dan_velocidade() {
        imprimi "vel = " + keli.vel;
    }
}

mimoria gatu = Animal("69", 10);
mimoria gatu_velocidade = gatu.dan_velocidade;
gatu.nomi = "Mimi";
gatu.dan_velocidade();
gatu.anda(10, 4);
gatu.vel = "10";
gatu_velocidade();
imprimi gatu.nomi;

// funson Forma_2D() {
//     klasi Forma {}
//     mimoria circulo = Forma();
//     circulo.raio = 5;
//     imprimi circulo.raio; 
// }
// Forma_2D();

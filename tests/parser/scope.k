pa (mimoria i = 0; i < 2; i = i + 1) {
    mimoria a = 0;
    mimoria b = 1;
    imprimi i;
    pa (mimoria j = 0; j < 2; j = j + 1) {
        mimoria c = 2; 
        mimoria d = 3;
        sai;
    }
    mimoria c = 2; 
    sai;
}
// TODO: pa (mimoria i = 0; i < 5; i = i + 1) mimoria a = 2;
{
    si (verdadi) {
        si (falsu) { sai; }
    }
    imprimi "Ola";
    
    pa (mimoria i = 0; i < 2; i = i + 1) {
        if (falsu) { sai; }
        pa (mimoria j = 0; j < 2; j = j + 1) {
            imprimi "loop 2";
            sai;
        }
        imprimi "end loop 1"
    }
    
    {
        si (verdadi) { sai; }
        imprimi "Hi";
    }

    imprimi "world!";
}
imprimi "Hello world!";
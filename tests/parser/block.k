{
    pa (mimoria i = 0; i < 2; i = i + 1) {
        si (i == 1) { sai; }
        pa (mimoria j = 0; j < 2; j = j + 1) {
            imprimi "loop 2";
            sai;
        }
        imprimi "end loop 1";
    }
    imprimi "root";
    
    {
        si (verdadi) { sai; }
        imprimi "Hi";
    }

    imprimi "world!";
}
imprimi "Hello world!";
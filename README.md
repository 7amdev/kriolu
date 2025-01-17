# Kriolu

Kriolu is an interpreter written in C with syntax in the unofficial language of Cape Verde Island, Kriolu. To facilitate learning and familiarity, the syntax is simple and has almost direct relationships with other popular languages such as C, Java, Javascript, C#, etc.

## Todo

- [ ] Fix typos and grammar errors.
- [ ] code snipet with syntax highlited [link](https://pragmaticpineapple.com/adding-custom-html-and-css-to-github-readme/)

## Build Project

TBD

## Kriolu Programming Language

### Data Types

- **Booleans.** You can't code without logic and you can't logic without Boolean.

  ```
  verdadi
  falsu;
  ```

- **Numbers.**
  ```
  9876;
  98.76;
  ```
- **Strings.**
  ```
  "Abo mi";
  "";
  ```
- **Null.** A `nulo` represents "no value". In other popular languages such as Java and C, It's called `null`.
  ```
  nulo;
  ```

### Variables

To store and keep track of all the data in our program, we use variables to create names to which data can be assigned.

```
mimoria nome;
imprimi nome; // nil

mimoria cidade = "Praia";
imprimi cidade; // Praia
```

### Expressions

With ways to create and store data in our program, expressions are actions we can perform on them leaving as a side effect a new value.

Expressions are actions we can perform on data, which produces a value as a result.

> _Arithmetic_

kriolu supports basic arithmetic operations such as:

Binary operations

```
// Numbers
1 + 1;    // = 2 Addition
9 - 1;    // = 8 Subtraction
4 * 1;    // = 4 Multiplication
5 / 1;    // = 5 Division

// Strings
"Oi " + "mundo.";    // "Oi mundo."
```

Unary operation on number

```
-9;    // Negation
```

> _Comparison_

Comparing 2(two) values yields a logical value `verdadi` or `falsu`;

```
// Magnitude

19 < 45;    // 19 minor ki 45 -> verdadi
19 <= 45;   // 19 minor ki ou igual a 45 -> verdadi
5 > 7;      // 5 maior ki 7 -> falsu
5 >= 7;     // 5 maior ki ou igual a 7 -> falsu

// Equality and Inequality

1 == 2;                // falsu
"seti" == 7;           // falsu
7 == 7;                // verdadi
"seti" == "seti";      // verdadi
"cidade" =/= "velha";  // verdadi
7 =/= 7                // falsu
2 =/= 3                // verdadi
```

_Logical operators_

```
// ka

ka verdadi;         // falsu
ka falsu;           // verdadi

// e

verdadi e falsu;    // falsu
verdadi e verdadi;  // verdadi

// ou

falsu ou falsu;     // falsu
verdai ou falsu;    // verdadi

```

_Precedence and grouping_

You can change the operators precedence and associativity by using `()` to group particular operations in a expression.

```
mimoria media = (19 + 45) / 2;
```

### Statements

Statements are commands to be performed when the program is run. By definition, it doesn't produce and value, only an effect.

```
imprimi "Oi, mundo!";
"expression";
{
    imprimi "Santiago";
    imprimi "Fogo";
    imprimi "Mindelo";
    imprimi "Brava";
}
```

### Control Flow

Control flow gives you the ability to change program's execution order which is sequential and top to bottom.

Given a condition, an `if` statement executes only the portion of code that pass the condition.

```
mimoria ano = 1945;
si (ano == 1945) {
    imprimi "sim";
} sinau {
    imprimi "nao";
}

si (ano == 1900)
{
    imprimi "Ano 1900.";
} sinau si (ano == 1950) {
    imprimi "Ano 1950.";
} sinau si (ano == 1945) {
    imprimi "Ano 1945.";
} sinau {
    imprimi "ka sabi.";
}
```

A `timenti` and `di` loop executes som code more than once.

```
mimoria a = 1;
timenti (a < 10) {
    imprimi a;
    a = a + 1;
}

di (mimoria i = 1; i < 10; i = i + 1) {
    imprimi i;
}

di (5 ti 10) {
    imprimi i;
}
```

### Functions

```
// Declaration
funson produto(a, b) {
    imprimi a * b;
}

// Call
produto(2, 3);    // 6
```

> _Closures_

```
// example 1
funson soma(a, b) {
    divolvi a + b;
}

funson identidade(a) {
    divolvi a;
}

imprimi identidade(soma)(2, 3);    // prints 6

// example 2
funson mensagem() {
    mimoria texto = "5 di julho di 1945.";

    fn imprimiTexto() {
        imprimi texto;
    }

    divolvi imprimiTexto;
}

mimoria texto = mensagem();
texto();    // "5 di julho di 1945."
```

### Classes

_Class declaration_

```
klasi Quadrilateru {
    area() {
        divolvi keli.base * keli.altura;
    }

    perimetru() {
        divolvi 2 * (keli.base + keli.altura);
    }
}
```

_Classes in kriolu are first class_

```
mimoria temp = Quadrilateru;
qualquerFunson(Quadrilateru);
```

_Instantiation_

```
mimoria quadrado = Quadrilateru();
imprimi quadrado;    // "Quadrilateru instance"

quadrado.base = 10;
quadrado.altura = 10;

imprimi quadrado.area();    // 100
```

_Initialization_

Ensures the object is in a valid state upon creation. By declaring a method named `init()`, it will be executed when the objet is constructed.

```
klasi Quadrilateru {
    init(x, y, base, altura, cor) {
        keli.x = x;
        keli.y = y;
        keli.base = base;
        keli.altura = altura;
        keli.cor = cor;
    }

    area() {
        divolvi keli.base * keli.altura;
    }

    perimetru() {
        divolvi 2 * (keli.base + keli.altura);
    }
}

mimoria retangulo = new Quadrilateru(0, 0, 10, 20, "burmedjo");
mimoria quadradu = new Quadrilateru(0, 0, 10, 10, "berdi");

imprimi retangulo.area();   // 200
imprimi quadradu.area();    // 100

```

_Inheritance_

```
klasi Animal {
    init(nomi) {
        keli.nomi = nomi;
    }

    cume() {
        imprimi "N'ta cume."
    }

    durmi() {
        imprimi "N'ta durmi."
    }
}

klasi Catxor : Animal  {
    init(nomi) {
        super.init(nomi);
    }

    ladra() {
        imprimi "N'ta ladra.";
    }
}

mimoria labrador = Catxor("bobi");
labrador.ladra();


```

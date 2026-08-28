#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define x_min -2.0
#define x_max 1.0
#define y_max 1.5
#define y_min -1.5

typedef struct{
    float x;
    float y;
}C;

C* converter_pixel_para_complexo(int Px, int Py, int largura, int altura, C * c ){

    //parte real
    float x = x_min + (Px / (float)largura) * (x_max - x_min);

    //parte imaginária
    //Lembrar que a parte imaginária vem com aquele i, então eu não posso simplesmente somar com a parte real
    float y = y_max - (Py/(float)altura) * (y_max - y_min);

    c->x = x;
    c->y = y;


    return c;
}

void calcular_normal(int Px, int Py, int largura, int altura, int max_interacoes, C * c){

    double z_real = 0.0;
    double z_imaginario = 0.0;

    C * ponto_c = converter_pixel_para_complexo(Px, Py, largura, altura, c);

    double c_real = c->x;
    double c_imaginario = c->y;

    double z_novo[2];
    z_novo[0] = 0.0;
    z_novo[1] = 0.0;

    //Fórmula para eu aplicar no loop: z = z^2 + c, onde c vai ser o retorno da função converter
    for(int i =  0; i < max_interacoes; i++){

        //Parte Real
        z_novo[0] = (z_real * z_real) + c_real - (z_imaginario * z_imaginario);

        //Parte Imaginária
        z_novo[1] = (2 * z_real * z_imaginario) + c_imaginario;

        z_real = z_novo[0];
        z_imaginario = z_novo[1];

    }

    printf("Número complexo formado ao final: c = %f + %fi\n",z_novo[0],z_novo[1]);
}

int main(void){ 
    C * c = (C*)malloc(sizeof(C));
    int Px = 0;
    int Py = 0;
    int largura = 800;
    int altura = 600;
    int max_interacoes = 1;

    calcular_normal(Px, Py, largura, altura, max_interacoes, c);

    return 0;
}


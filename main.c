#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#define x_min -2.0
#define x_max 1.0
#define y_max 1.5
#define y_min -1.5


typedef struct{
    float x;
    float y;
}C;

typedef struct{
    int inicio;
    int fim;
    int max_interacoes;
    int largura;
    int altura;
    int * interacoes;
    int * intensidades;

    int *proxima_linha;
    pthread_mutex_t * fila_mutex;

}argumentos_threads;

C* converter_pixel_para_complexo(int Px, int Py, int largura, int altura, C * c ){

    //parte real
    float x = x_min + (Px / (float)(largura)) * (x_max - x_min);

    //parte imaginária
    //Lembrar que a parte imaginária vem com aquele i, então eu não posso simplesmente somar com a parte real
    float y = y_max - (Py/(float)(altura)) * (y_max - y_min);

    c->x = x;
    c->y = y;


    return c;
}


int calcular_normal(int Px, int Py, int largura, int altura, int max_interacoes, C * c){
    
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
        
        if((z_real * z_real) + (z_imaginario * z_imaginario) > 4.0){
            return i + 1;
        }
        
    }
    
    return max_interacoes;
}

int normalizar(int interacoes, int max_interacoes){
    
    return (interacoes/(double)max_interacoes) * 255;
    
}

void * calcular_normal_pthread(void * arg){

    argumentos_threads * dados = (argumentos_threads *) arg;

    C c_pthread;

    for(int py = dados->inicio; py < dados->fim; py++){

        for(int px = 0; px < dados->largura; px++){

            int indice_no_vetor = (py * dados->largura) + px;
            dados->interacoes[indice_no_vetor] = calcular_normal(px, py, dados->largura, dados->altura, dados->max_interacoes, &c_pthread);
            dados->intensidades[indice_no_vetor] = normalizar(dados->interacoes[indice_no_vetor], dados->max_interacoes);

        }
    }

    return NULL;
}


void * calcular_normal_pthread2(void * arg){

    argumentos_threads * dados = (argumentos_threads *) arg;

    C c_pthread2;

    while(1){

        pthread_mutex_lock(dados->fila_mutex);

        int linha_atual = *dados->proxima_linha;
        *dados->proxima_linha = *dados->proxima_linha + 1;

        pthread_mutex_unlock(dados->fila_mutex);

        if(linha_atual >= dados->altura){
            break;
        }


        for(int px = 0; px < dados->largura; px++){

            int indice_no_vetor = (linha_atual * dados->largura) + px;
            dados->interacoes[indice_no_vetor] = calcular_normal(px, linha_atual, dados->largura, dados->altura, dados->max_interacoes, &c_pthread2);
            dados->intensidades[indice_no_vetor] = normalizar(dados->interacoes[indice_no_vetor], dados->max_interacoes);

        }

    }

    return NULL;
}

int main(int argc, char *argv[]){ 

    FILE * arquivo_erros = fopen("erros.txt","w");
    if(arquivo_erros == NULL){
        return -1;
    }

    if (argc < 5) {
        fprintf(arquivo_erros, "Erro: faltam argumentos!\n");
        fclose(arquivo_erros);
        return 1;
    }

    if(argc > 5){
        fprintf(arquivo_erros, "Erro: Muitos argumentos!\n");
        fclose(arquivo_erros);
        return -1;
    }

    C * c_serial = (C*)malloc(sizeof(C));
    if(c_serial == NULL){
        fprintf(arquivo_erros,"Erro: falha na alocação de memória");
        fclose(arquivo_erros);
        return 1;
    }

    int largura = atoi(argv[1]);
    int altura = atoi(argv[2]);
    int max_interacoes = atoi(argv[3]);
    int numero_threads = atoi(argv[4]);

    if(largura <= 0 || altura <= 0 || max_interacoes <= 0 || numero_threads <= 0){
        fprintf(arquivo_erros,"Apenas números maiores ou iguais a 0 são permitidos");
        fclose(arquivo_erros);
        return -1;
    }

    long nucleos = sysconf(_SC_NPROCESSORS_ONLN);
    int limite_threads;

    if(nucleos > 0){
        limite_threads = (int)(nucleos * 10);
    } 
    //Caso s função não consiga determinar a quantidade de núcleos
    else {
        limite_threads = 256;
    }

    if(numero_threads > limite_threads){
        fprintf(arquivo_erros, "Erro: número de threads excede o limite razoável (%d) para esta máquina\n", limite_threads);
        fclose(arquivo_erros);
        return -1;
    }

    int *interacoes = (int *) malloc((largura * altura) * sizeof(int));
    if(interacoes == NULL){
        fprintf(arquivo_erros,"Erro: Não foi possível alocar memória");
        fclose(arquivo_erros);
        return -1;
    }

    int *intensidades = (int *) malloc((largura * altura) * sizeof(int));

    if(intensidades == NULL){
        fprintf(arquivo_erros,"Erro: Não foi possível alocar memória");
        fclose(arquivo_erros);
        return -1;
    }

    struct timespec tempo_inicio, tempo_fim;
    clock_gettime(CLOCK_MONOTONIC, &tempo_inicio);

    for(int py = 0; py < altura; py++){
        for(int px = 0; px < largura; px++){
            int indice_no_vetor = (py * largura) + px;
            interacoes[indice_no_vetor] = calcular_normal(px, py, largura, altura, max_interacoes, c_serial);
            intensidades[indice_no_vetor] = normalizar(interacoes[indice_no_vetor], max_interacoes);
            
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &tempo_fim);

    double tempo_decorrido = (tempo_fim.tv_sec - tempo_inicio.tv_sec) + 
                            (tempo_fim.tv_nsec - tempo_inicio.tv_nsec) / 1e9;

    FILE * arquivo_time = fopen("times.txt","a");
    if(arquivo_time == NULL){
        fprintf(arquivo_erros,"Erro: Não foi possível abrir o arquivo de registro de tempo");
        fclose(arquivo_erros);
        return -1;
    }

    fprintf(arquivo_time,"Serial: %fs\n",tempo_decorrido);

    fclose(arquivo_time);
    
    FILE * arquivo = fopen("mandelbrot_dmcv_serial.pgm","w");

    if(arquivo == NULL){
        fprintf(arquivo_erros,"Erro: falha ao abrir o arquivo serial");
        fclose(arquivo_erros);
        return -1;
    }

    for(int py = 0; py < altura; py ++){
        for(int px = 0; px < largura; px++){
            int indice = (py * largura) + px;
            fprintf(arquivo,"%d ",intensidades[indice]);
        }
        fprintf(arquivo,"\n");
    }
    fclose(arquivo);

    //Open MP
    double inicio = omp_get_wtime();
    #pragma omp parallel for num_threads(numero_threads) schedule(dynamic)
    for(int py = 0; py < altura; py++){
        for(int px = 0; px < largura; px++){
            C c_thread;
            int indice_no_vetor = (py * largura) + px;
            interacoes[indice_no_vetor] = calcular_normal(px, py, largura, altura, max_interacoes, &c_thread);
            intensidades[indice_no_vetor] = normalizar(interacoes[indice_no_vetor], max_interacoes);
            
        }
    }
    double fim = omp_get_wtime();

    FILE * arquivo_tempo = fopen("times.txt","a");

    if(arquivo_tempo == NULL){
        fprintf(arquivo_erros,"Erro: falha ao abrir o arquivo de registro de tempo");
        fclose(arquivo_erros);
        return -1;
    }

    fprintf(arquivo_tempo, "OpenMp: %fs\n",(fim - inicio));
    fclose(arquivo_tempo);

    FILE * arquivo_openMP = fopen("mandelbrot_dmcv_openmp.pgm","w");
    
    if(arquivo_openMP == NULL){
        fprintf(arquivo_erros,"Erro: falha ao abrir o arquivo OpenMP");
        fclose(arquivo_erros);
        return -1;
    }

    for(int py = 0; py < altura; py ++){
        for(int px = 0; px < largura; px++){
            int indice = (py * largura) + px;
            fprintf(arquivo_openMP,"%d ",intensidades[indice]);
        }
        fprintf(arquivo_openMP,"\n");
    }
    fclose(arquivo_openMP);

    //P_thread

    pthread_t threads[numero_threads];
    argumentos_threads dados[numero_threads];


    for(int i = 0; i < numero_threads; i++){
        dados[i].max_interacoes = max_interacoes;
        dados[i].largura = largura;
        dados[i].altura = altura;

        dados[i].interacoes = interacoes;

        dados[i].intensidades = intensidades;

        if(i == 0){
            dados[i].inicio = 0;
            dados[i].fim = altura/numero_threads;

        }
        //Para caso divisão não seja exata, sempre garanto que vai até o fim
        else if(i == numero_threads - 1){
            dados[i].inicio = dados[i - 1].fim;
            dados[i].fim = altura;
        }
        else{
            dados[i].inicio = dados[i - 1].fim;
            dados[i].fim = dados[i].inicio + (altura/numero_threads);

        }
    }

    clock_gettime(CLOCK_MONOTONIC, &tempo_inicio);


    int status_pthread;

    for(int i = 0; i < numero_threads; i++){
        status_pthread = pthread_create(&threads[i],NULL,calcular_normal_pthread,&dados[i]);

        if(status_pthread != 0){
            fprintf(arquivo_erros,"Erro: Não foi possível criar a thread");
            fclose(arquivo_erros);
            return -1;
        }
    }

    for(int i = 0; i < numero_threads; i++){
        pthread_join(threads[i],NULL);
    }

    FILE * arquivo_p_thread = fopen("mandelbrot_dmcv_pthreads1.pgm","w");
    if(arquivo_p_thread == NULL){
        fprintf(arquivo_erros,"Erro: falha ao abrir arquivo do pthread");
        fclose(arquivo_erros);
        return -1;
    }


    for(int py = 0; py < altura; py++){
        for(int px = 0; px < largura; px++){

                int indice = (py * largura) + px;
                fprintf(arquivo_p_thread,"%d ",intensidades[indice]);

            }
            fprintf(arquivo_p_thread,"\n");
        }
    

    fclose(arquivo_p_thread);

    clock_gettime(CLOCK_MONOTONIC, &tempo_fim);

    tempo_decorrido = (tempo_fim.tv_sec - tempo_inicio.tv_sec) + 
                            (tempo_fim.tv_nsec - tempo_inicio.tv_nsec) / 1e9;

    FILE * arquivo_time_pthread = fopen("times.txt","a");
    if(arquivo_time_pthread == NULL){
        fprintf(arquivo_erros,"Erro: falha ao abrir o arquivo de tempo");
        fclose(arquivo_erros);
        return -1;
    }

    fprintf(arquivo_time_pthread,"Pthread1: %fs\n",tempo_decorrido);

    fclose(arquivo_time_pthread);

    //P_thread2


    int proxima_linha = 0;
    pthread_mutex_t fila_mutex;
    pthread_mutex_init(&fila_mutex, NULL);


    for(int i = 0; i < numero_threads; i++){
        dados[i].max_interacoes = max_interacoes;
        dados[i].largura = largura;
        dados[i].altura = altura;

        dados[i].interacoes = interacoes;

        dados[i].intensidades = intensidades;

        dados[i].proxima_linha = &proxima_linha;

        dados[i].fila_mutex = &fila_mutex;
    }

    clock_gettime(CLOCK_MONOTONIC, &tempo_inicio);


    int status_pthread2;

    for(int i = 0; i < numero_threads; i++){
        status_pthread2 = pthread_create(&threads[i],NULL,calcular_normal_pthread2,&dados[i]);

        if(status_pthread2 != 0){
            fprintf(arquivo_erros,"Erro: Não foi possível criar a thread");
            fclose(arquivo_erros);
            return -1;
        }
    }

    for(int i = 0; i < numero_threads; i++){
        pthread_join(threads[i],NULL);
    }

    pthread_mutex_destroy(&fila_mutex);

    FILE * arquivo_p_thread2 = fopen("mandelbrot_dmcv_pthreads2.pgm","w");
    if(arquivo_p_thread2 == NULL){
        fprintf(arquivo_erros,"Erro: falha ao abrir arquivo do pthread");
        fclose(arquivo_erros);
        return -1;
    }


    for(int py = 0; py < altura; py++){
        for(int px = 0; px < largura; px++){

                int indice = (py * largura) + px;
                fprintf(arquivo_p_thread2,"%d ",intensidades[indice]);

            }
            fprintf(arquivo_p_thread2,"\n");
        }
    

    fclose(arquivo_p_thread2);

    clock_gettime(CLOCK_MONOTONIC, &tempo_fim);

    tempo_decorrido = (tempo_fim.tv_sec - tempo_inicio.tv_sec) + 
                            (tempo_fim.tv_nsec - tempo_inicio.tv_nsec) / 1e9;

    FILE * arquivo_time_pthread2 = fopen("times.txt","a");
    if(arquivo_time_pthread2 == NULL){
        fprintf(arquivo_erros,"Erro: falha ao abrir o arquivo de tempo");
        fclose(arquivo_erros);
        return -1;
    }

    fprintf(arquivo_time_pthread2,"Pthread2: %fs\n",tempo_decorrido);

    fclose(arquivo_time_pthread2);


    fclose(arquivo_erros);

    free(c_serial);
    free(interacoes);
    free(intensidades);

    return 0;
}
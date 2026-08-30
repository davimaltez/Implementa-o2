# Mandelbrot — Serial, OpenMP e Pthreads

Implementação do conjunto de Mandelbrot em C, com quatro versões diferentes de cálculo: serial, OpenMP e duas estratégias distintas de paralelização com Pthreads. O projeto foi desenvolvido como parte da disciplina de Infraestrutura de Software.

## Descrição

O programa gera uma imagem do conjunto de Mandelbrot, representando a região real `[-2,0; 1,0]` e imaginária `[-1,5; 1,5]` do plano complexo. Para cada pixel, calcula o número de iterações necessárias até o ponto correspondente "escapar" (módulo maior que 2) ou atingir o número máximo de iterações definido pelo usuário. Esse valor é normalizado para uma escala de intensidade de 0 a 255 e gravado em um arquivo de imagem.

As quatro implementações — serial, OpenMP, Pthreads com divisão em blocos fixos (estratégia 1) e Pthreads com fila de trabalho dinâmica protegida por mutex (estratégia 2) — geram, para os mesmos parâmetros de entrada, imagens idênticas entre si. O que muda entre elas é apenas o tempo de execução, registrado separadamente para cada uma.

## Compilação

O projeto usa um `Makefile` para automatizar a compilação:

```bash
make
```

Isso gera o executável `mandelbrot`.

Para remover os arquivos gerados pela compilação:

```bash
make clean
```

### Dependências

- Compilador `gcc` com suporte a:
  - OpenMP (`-fopenmp`)
  - Pthreads (`-pthread`)
- Biblioteca matemática padrão (`-lm`)
- Sistema Linux, Unix ou macOS (uso de `<pthread.h>`, `<unistd.h>` e `<time.h>` POSIX)

## Uso

```bash
./mandelbrot [largura] [altura] [max_iteracoes] [num_threads]
```

| Parâmetro | Descrição |
|---|---|
| `largura` | Largura da imagem em pixels (inteiro positivo) |
| `altura` | Altura da imagem em pixels (inteiro positivo) |
| `max_iteracoes` | Número máximo de iterações por ponto (inteiro positivo) |
| `num_threads` | Número de threads usadas nas implementações paralelas (inteiro positivo, limitado a um valor razoável de acordo com os núcleos da máquina) |

### Exemplo

```bash
./mandelbrot 1920 1080 1000 8
```

## Arquivos gerados

Após a execução, o programa produz:

| Arquivo | Conteúdo |
|---|---|
| `mandelbrot_dmcv_serial.pgm` | Imagem gerada pela implementação serial |
| `mandelbrot_dmcv_openmp.pgm` | Imagem gerada pela implementação com OpenMP |
| `mandelbrot_dmcv_pthreads1.pgm` | Imagem gerada pela implementação com Pthreads (blocos fixos) |
| `mandelbrot_dmcv_pthreads2.pgm` | Imagem gerada pela implementação com Pthreads (fila de trabalho com mutex) |
| `times.txt` | Tempo de execução de cada uma das quatro implementações |
| `erros.txt` | Mensagens de erro, quando aplicável |

Os arquivos `.pgm` contêm apenas os valores de intensidade de cada pixel, um valor por pixel, separados por espaço, com uma linha por linha da imagem — sem cabeçalho de formato.

O programa não imprime nada na saída padrão (stdout) durante a execução normal; qualquer mensagem de erro é escrita em `erros.txt`.

## Estratégias de paralelização

- **OpenMP**: paraleliza o loop externo (linhas da imagem) com `#pragma omp parallel for`, usando `schedule(dynamic)` — testes mostraram tempo menor que `schedule(static)`, já que o custo de cálculo varia entre pixels dentro e fora do conjunto.
- **Pthreads — Estratégia 1 (blocos fixos)**: cada thread recebe, antes da execução, um intervalo contíguo e fixo de linhas para processar. A última thread absorve eventuais linhas restantes quando a altura não é múltipla do número de threads.
- **Pthreads — Estratégia 2 (fila de trabalho dinâmica)**: as threads compartilham um contador de "próxima linha disponível", protegido por mutex. Cada thread, ao ficar livre, pega a próxima linha da fila, o que tende a balancear melhor a carga entre threads quando o custo de cálculo é desigual entre elas.

## Tratamento de erros

O programa valida:

- Número incorreto de argumentos
- Valores inválidos (não numéricos, negativos ou zero) para largura, altura, máximo de iterações e número de threads
- Número de threads acima de um limite considerado razoável para a máquina (baseado no número de núcleos, obtido via `sysconf`)
- Falha na alocação de memória
- Falha na criação de arquivos de saída
- Falha na criação de threads

Todas as mensagens de erro são registradas em `erros.txt`.

## Estrutura de arquivos

- `main.c`: código-fonte completo (structs, funções de cálculo, as quatro implementações e escrita dos arquivos de saída)
- `Makefile`: automação de build e limpeza
- `main.o`: arquivo objeto gerado na compilação
- `mandelbrot`: executável final

## Autor

Davi Maltez Costa Vasconcelos
//Projeto SO - exercicio 3
//----- Grupo 97 -----
//Joana Teodoro, 86440
//Taíssa Ribeiro, 86514

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "matrix2d.h"


//Estrutura de uma trabalhadora
typedef struct { 
  int id;
  int N;
  int trab;
  int iter;
  double maxD;
} args_mythread_t;

DoubleMatrix2D *matrix, *matrix_aux, *result;

//Constituicao da barreira
pthread_mutex_t    mutex;
pthread_cond_t     wait_for_threads;

//Variaveis adicionadas
int count; //contador que decrementa sempre que uma thread chega à barreira
int next; //bit que permite que todas as threads possam avançar para a proxima iteraçao
int terminal_condition; //bit que permite que todas as threads sejam terminadas (1 para terminar, 0 para continuar)
double max_dif = 0; //variavel que contem o valor da diferença maior entre valores antigos e atualizados da matriz


/*--------------------------------------------------------------------
| Function: simul
  Novos argumentos: trab, id
---------------------------------------------------------------------*/

void *simul(void*a) {
  
  args_mythread_t *args = (args_mythread_t *) a;

  int id = args->id;
  int N = args->N;
  int iter = args->iter;
  int trab = args->trab;
  double maxD = args->maxD;

  DoubleMatrix2D *m, *aux,*tmp;
  int i,j, w;
  double value, prev_value, local_dif = 0;
  int k = N/trab;

  int local_next; //bit que permite que a thread possa avançar para a proxima iteraçao
  m = matrix;
  aux = matrix_aux;

  for (i = 0; i < iter; i++) {
  	for (j = k*id + 1; j < k*(id + 1) + 1; j++){
      for (w = 1; w < N + 1; w++) {
        prev_value  = dm2dGetEntry(m, j, w);
        value = ( dm2dGetEntry(m, j-1, w) + dm2dGetEntry(m, j+1, w) +
        dm2dGetEntry(m, j, w-1) + dm2dGetEntry(m, j, w+1) ) / 4.0;
        dm2dSetEntry(aux, j, w, value);
        if(value-prev_value > local_dif) local_dif =  value-prev_value;
      }
    }  

    tmp = aux;
    aux = m;
    m = tmp;

    if(pthread_mutex_lock(&mutex)!=0){
      fprintf(stderr, "\nErro ao bloquear mutex\n");
      return NULL;
    }

    if(local_dif > max_dif) max_dif =  local_dif;

    local_dif = 0;
    terminal_condition = 0;
    count--;
    local_next = 1 - local_next; //atualizacao do bit

    if(count == 0){ //threads chegam todas à barreira e acordam
      count = trab;
      next = local_next;
      if(max_dif < maxD) terminal_condition = 1; 
      max_dif = 0;
      pthread_cond_broadcast(&wait_for_threads);
    }

    else{
      while(local_next != next){ //enquanto nao chegarem todas, as que ja executaram adormecem
        if(pthread_cond_wait(&wait_for_threads, &mutex) != 0) {
          fprintf(stderr, "\nErro ao esperar pela variável de condição\n");
          return NULL;
        }
      }
    }

    if(pthread_mutex_unlock(&mutex)!=0){
      fprintf(stderr, "\nErro ao desbloquear mutex\n");
      return NULL;
    }

    if(terminal_condition){
      matrix = m;
      matrix_aux = aux;
      pthread_exit((void*)-1);
      return NULL;
    }

  }
  matrix = m;
  matrix_aux = aux;
  return NULL;
}


/*--------------------------------------------------------------------
| Function: parse_integer_or_exit
---------------------------------------------------------------------*/

int parse_integer_or_exit(char const *str, char const *name){
  int value;
 
  if(sscanf(str, "%d", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(1);
  }
  return value;
}

/*--------------------------------------------------------------------
| Function: parse_double_or_exit
---------------------------------------------------------------------*/

double parse_double_or_exit(char const *str, char const *name){
  double value;

  if(sscanf(str, "%lf", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(1);
  }
  return value;
}


/*--------------------------------------------------------------------
| Function: main
---------------------------------------------------------------------*/

int main (int argc, char** argv) {

  args_mythread_t *slave_args;
  pthread_t       *slaves;
  int              i;

  if(argc != 9) {
    fprintf(stderr, "\nNumero invalido de argumentos.\n");
    fprintf(stderr, "Uso: heatSim N tEsq tSup tDir tInf iteracoes trab csz\n\n");
    return 1;
  }

  /* argv[0] = program name */
  int N = parse_integer_or_exit(argv[1], "N");
  double tEsq = parse_double_or_exit(argv[2], "tEsq");
  double tSup = parse_double_or_exit(argv[3], "tSup");
  double tDir = parse_double_or_exit(argv[4], "tDir");
  double tInf = parse_double_or_exit(argv[5], "tInf");
  int iteracoes = parse_integer_or_exit(argv[6], "iteracoes");
  int trab = parse_integer_or_exit(argv[7], "trab");
  double maxD = parse_double_or_exit(argv[8], "maxD");


  fprintf(stderr, "\nArgumentos:\n"
	" N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iteracoes=%d trab=%d maxD=%f\n",
	N, tEsq, tSup, tDir, tInf, iteracoes, trab, maxD);

  if(N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iteracoes < 1 || trab <=0 || N%trab !=0 || maxD<0) {
    fprintf(stderr, "\nErro: Argumentos invalidos.\n"
	" Lembrar que N >= 1, temperaturas >= 0, iteracoes >= 1, trab>=1 e tem de ser multiplo de N, e maxD>=0\n\n");
    return 1;
  }

  count = trab;

  slave_args = (args_mythread_t*)malloc(trab*sizeof(args_mythread_t));
  slaves     = (pthread_t*)malloc(trab*sizeof(pthread_t));

  matrix = dm2dNew(N+2, N+2);
  matrix_aux = dm2dNew(N+2, N+2);

  if (matrix == NULL || matrix_aux == NULL || slave_args == NULL || slaves == NULL) {
  	fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para as matrizes ou para as trabalhadoras\n\n");
    return -1;
  }

  if(pthread_mutex_init(&mutex, NULL) != 0) {
    fprintf(stderr, "\nErro ao inicializar mutex\n");
    return -1;
   }

  if(pthread_cond_init(&wait_for_threads, NULL) != 0) {
    fprintf(stderr, "\nErro ao inicializar variável de condição\n");
    return -1;
   }


  for(i=0; i<N+2; i++)
    dm2dSetLineTo(matrix, i, 0);

  dm2dSetLineTo (matrix, 0, tSup);
  dm2dSetLineTo (matrix, N+1, tInf);
  dm2dSetColumnTo (matrix, 0, tEsq);
  dm2dSetColumnTo (matrix, N+1, tDir);

  dm2dCopy (matrix_aux, matrix);


   //Criação de threads e envio, por parte da tarefa mestre, linha a linha da informação 
   //com a qual as threads terão de trabalhar
   for (i=0; i<trab; i++) {
   	slave_args[i].id = i;
    slave_args[i].N = N;
    slave_args[i].trab = trab;
    slave_args[i].iter = iteracoes;
    slave_args[i].maxD = maxD;
   	printf("Creating thread %d\n", i);
    pthread_create(&slaves[i], NULL, simul, &slave_args[i]);
   }

   for (i=0; i<trab; i++) {
    if (pthread_join(slaves[i], NULL)) {
      fprintf(stderr, "\nErro ao esperar por um escravo.\n");    
      return -1;
    }
   }

  dm2dPrint(matrix);

  free(slaves);
  free(slave_args);

  if(pthread_mutex_destroy(&mutex) != 0) {
    fprintf(stderr, "\nErro ao destruir mutex\n");
    exit(1);
   }
    
   if(pthread_cond_destroy(&wait_for_threads) != 0) {
    fprintf(stderr, "\nErro ao destruir variável de condição\n");
    exit(1);
   }

  dm2dFree(matrix);
  dm2dFree(matrix_aux);

  return 0;
}

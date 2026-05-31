#ifndef TRANSACAO_H
#define TRANSACAO_H

/* =========================================================
 *  transacao.h — Definições centrais do sistema
 * =========================================================*/

#define _GNU_SOURCE
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Limiar de valor para alerta de risco (em R$) */
#define LIMIAR_RISCO    200.0f
/* Hora inicial do período noturno (inclusive) */
#define HORA_NOITE_INI  23
/* Hora final do período noturno (inclusive) */
#define HORA_NOITE_FIM   5

/* Caminho do arquivo binário indexado */
#define ARQUIVO_BIN "transacoes.dat"
/* Caminho do relatório final */
#define ARQUIVO_REL "relatorio_consolidado.txt"

/* -------------------------------------------------------
 *  Struct principal — representa uma transação válida
 * -------------------------------------------------------*/
typedef struct {
    unsigned long id;       /* Transaction ID (numérico extraído) */
    char  data[11];         /* Data  DD/MM/AAAA                   */
    char  hora[9];          /* Hora  HH:MM:SS  (hora + :00:00)    */
    char  bandeira[20];     /* Type of Card                       */
    char  categoria[30];    /* Merchant Group                     */
    float valor;            /* Amount (R$)                        */
    int   status;           /* 0=Pendente  1=Aprovada  2=Rejeitada*/
} Transacao;

/* -------------------------------------------------------
 *  Nó da Lista Encadeada de transações suspeitas
 * -------------------------------------------------------*/
typedef struct NoLista {
    Transacao       dado;
    struct NoLista *prox;
} NoLista;

/* -------------------------------------------------------
 *  Fila FIFO — autorização de crédito
 * -------------------------------------------------------*/
typedef struct NoFila {
    Transacao      dado;
    struct NoFila *prox;
} NoFila;

typedef struct {
    NoFila *inicio;
    NoFila *fim;
    int     tamanho;
} Fila;

/* -------------------------------------------------------
 *  Pilha LIFO — auditoria de rejeições
 * -------------------------------------------------------*/
typedef struct NoPilha {
    Transacao       dado;
    struct NoPilha *abaixo;
} NoPilha;

typedef struct {
    NoPilha *topo;
    int      tamanho;
} Pilha;

/* -------------------------------------------------------
 *  Protótipos dos módulos
 * -------------------------------------------------------*/

/* modulo_csv.c */
int  csv_para_binario(const char *csv_path, const char *bin_path,
                      int *lidas, int *validas, int *descartadas);

/* modulo_ordenacao.c */
Transacao *carregar_binario(const char *bin_path, int *total);
void       ordenar_por_valor(Transacao *v, int n);
void       ordenar_por_data_hora(Transacao *v, int n);

/* modulo_estruturas.c */
void   fila_inicializar(Fila *f);
int    fila_enfileirar(Fila *f, Transacao t);
int    fila_desenfileirar(Fila *f, Transacao *dest);
void   fila_liberar(Fila *f);

void   pilha_inicializar(Pilha *p);
int    pilha_empilhar(Pilha *p, Transacao t);
int    pilha_desempilhar(Pilha *p, Transacao *dest);  /* undo() */
void   pilha_liberar(Pilha *p);

NoLista *lista_inserir(NoLista *cab, Transacao t);
void     lista_liberar(NoLista *cab);
int      lista_contar(NoLista *cab);

/* modulo_relatorio.c */
void gerar_relatorio(
    const char  *caminho,
    double       tempo_val,
    double       tempo_data,
    Transacao   *vetor,
    int          total,
    const Fila  *fila,
    const Pilha *pilha,
    NoLista     *lista_suspeitas);

#endif /* TRANSACAO_H */

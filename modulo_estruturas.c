/* =========================================================
 *  modulo_estruturas.c — Fila, Pilha e Lista Encadeada
 *
 *  Todas as alocações são dinâmicas (malloc/free).
 *  Cada estrutura é totalmente independente.
 * =========================================================*/

#include "transacao.h"

/* =======================================================
 *  FILA (FIFO) — Autorização de Crédito
 *
 *  Critério de entrada: Fraud == 0 (sem fraude detectada)
 *  Processamento: estritamente na ordem de chegada
 * =======================================================*/

void fila_inicializar(Fila *f) {
    f->inicio   = NULL;
    f->fim      = NULL;
    f->tamanho  = 0;
}

/* Enfileira uma transação no final da fila.
 * Retorna 1 em sucesso, 0 em falha de alocação. */
int fila_enfileirar(Fila *f, Transacao t) {
    NoFila *novo = (NoFila *)malloc(sizeof(NoFila));
    if (!novo) return 0;

    novo->dado = t;
    novo->prox = NULL;

    if (f->fim == NULL) {
        /* Fila vazia: início e fim apontam para o mesmo nó */
        f->inicio = novo;
        f->fim    = novo;
    } else {
        f->fim->prox = novo;
        f->fim       = novo;
    }
    f->tamanho++;
    return 1;
}

/* Retira o primeiro elemento da fila e copia em *dest.
 * Retorna 1 em sucesso, 0 se fila vazia. */
int fila_desenfileirar(Fila *f, Transacao *dest) {
    if (f->inicio == NULL) return 0;

    NoFila *removido = f->inicio;
    *dest = removido->dado;
    f->inicio = removido->prox;

    if (f->inicio == NULL) f->fim = NULL; /* Fila ficou vazia */

    free(removido);
    f->tamanho--;
    return 1;
}

/* Libera todos os nós remanescentes */
void fila_liberar(Fila *f) {
    Transacao t;
    while (fila_desenfileirar(f, &t));
}

/* =======================================================
 *  PILHA (LIFO) — Auditoria de Rejeições
 *
 *  Critério de entrada: status == 2 (Rejeitada)
 *  Função undo(): desempilha a última transação rejeitada
 * =======================================================*/

void pilha_inicializar(Pilha *p) {
    p->topo     = NULL;
    p->tamanho  = 0;
}

/* Empilha uma transação no topo.
 * Retorna 1 em sucesso, 0 em falha de alocação. */
int pilha_empilhar(Pilha *p, Transacao t) {
    NoPilha *novo = (NoPilha *)malloc(sizeof(NoPilha));
    if (!novo) return 0;

    novo->dado   = t;
    novo->abaixo = p->topo;
    p->topo      = novo;
    p->tamanho++;
    return 1;
}

/* undo() — retira o topo e copia em *dest para revisão.
 * Retorna 1 em sucesso, 0 se pilha vazia. */
int pilha_desempilhar(Pilha *p, Transacao *dest) {
    if (p->topo == NULL) return 0;

    NoPilha *removido = p->topo;
    *dest = removido->dado;
    p->topo = removido->abaixo;

    free(removido);
    p->tamanho--;
    return 1;
}

/* Libera todos os nós remanescentes */
void pilha_liberar(Pilha *p) {
    Transacao t;
    while (pilha_desempilhar(p, &t));
}

/* =======================================================
 *  LISTA ENCADEADA — Alertas de Risco
 *
 *  Critério: valor > LIMIAR_RISCO
 *         E  hora em [23, 24] OU [0, 5]
 *
 *  Inserção sempre no final (ordem de chegada preservada).
 *  Alocação estritamente dinâmica.
 * =======================================================*/

/* Insere ao final; retorna nova cabeça da lista.
 * (Se cab == NULL, cria a lista.) */
NoLista *lista_inserir(NoLista *cab, Transacao t) {
    NoLista *novo = (NoLista *)malloc(sizeof(NoLista));
    if (!novo) return cab; /* falha silenciosa — retorna cabeça original */

    novo->dado = t;
    novo->prox = NULL;

    if (cab == NULL) return novo;

    /* Percorre até o final */
    NoLista *cur = cab;
    while (cur->prox) cur = cur->prox;
    cur->prox = novo;
    return cab;
}

/* Conta os nós da lista */
int lista_contar(NoLista *cab) {
    int c = 0;
    while (cab) { c++; cab = cab->prox; }
    return c;
}

/* Libera todos os nós da lista */
void lista_liberar(NoLista *cab) {
    while (cab) {
        NoLista *prox = cab->prox;
        free(cab);
        cab = prox;
    }
}

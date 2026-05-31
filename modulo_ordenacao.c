/* =========================================================
 *  modulo_ordenacao.c — Carga do binário e ordenação
 *
 *  Algoritmo escolhido: Merge Sort (O(n log n) garantido)
 *
 *  Justificativa técnica (consta no relatório):
 *    - O dataset possui ~100 mil registros sem ordem definida.
 *    - Quick Sort tem pior caso O(n²) em vetores já ordenados
 *      ou com muitas chaves repetidas — risco real neste dataset
 *      onde valores (R$) se repetem muito (ex: R$17 aparece 2153×).
 *    - Heap Sort também é O(n log n) garantido, mas tem pior
 *      constante de cache que Merge Sort em vetores grandes.
 *    - Merge Sort: estável, O(n log n) em todos os casos,
 *      ideal para ordenação cronológica (chave composta).
 * =========================================================*/

#include "transacao.h"

/* -------------------------------------------------------
 *  Carrega todos os registros do binário em um vetor
 *  alocado dinamicamente.
 *  Retorna ponteiro para o vetor; *total = número de itens.
 *  Retorna NULL em caso de erro.
 * -------------------------------------------------------*/
Transacao *carregar_binario(const char *bin_path, int *total) {
    *total = 0;

    FILE *f = fopen(bin_path, "rb");
    if (!f) { perror("Erro ao abrir binário"); return NULL; }

    /* Descobrir quantos registros existem */
    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    rewind(f);

    if (tam % (long)sizeof(Transacao) != 0) {
        fprintf(stderr, "Aviso: tamanho do binário não é múltiplo da struct.\n");
    }

    *total = (int)(tam / (long)sizeof(Transacao));
    if (*total == 0) { fclose(f); return NULL; }

    Transacao *v = (Transacao *)malloc((size_t)(*total) * sizeof(Transacao));
    if (!v) { perror("malloc falhou"); fclose(f); return NULL; }

    int lidos = (int)fread(v, sizeof(Transacao), (size_t)*total, f);
    if (lidos != *total) {
        fprintf(stderr, "Aviso: esperados %d registros, lidos %d.\n",
                *total, lidos);
        *total = lidos;
    }

    fclose(f);
    return v;
}

/* =======================================================
 *  MERGE SORT genérico com função de comparação
 * =======================================================*/

/* Auxiliar: mescla dois sub-vetores */
static void mesclar(Transacao *v, int esq, int meio, int dir,
                    int (*cmp)(const Transacao *, const Transacao *)) {
    int n1 = meio - esq + 1;
    int n2 = dir - meio;

    Transacao *L = (Transacao *)malloc((size_t)n1 * sizeof(Transacao));
    Transacao *R = (Transacao *)malloc((size_t)n2 * sizeof(Transacao));
    if (!L || !R) { free(L); free(R); return; }

    memcpy(L, v + esq,      (size_t)n1 * sizeof(Transacao));
    memcpy(R, v + meio + 1, (size_t)n2 * sizeof(Transacao));

    int i = 0, j = 0, k = esq;
    while (i < n1 && j < n2) {
        if (cmp(&L[i], &R[j]) <= 0) v[k++] = L[i++];
        else                         v[k++] = R[j++];
    }
    while (i < n1) v[k++] = L[i++];
    while (j < n2) v[k++] = R[j++];

    free(L); free(R);
}

static void merge_sort(Transacao *v, int esq, int dir,
                       int (*cmp)(const Transacao *, const Transacao *)) {
    if (esq >= dir) return;
    int meio = esq + (dir - esq) / 2;
    merge_sort(v, esq,    meio, cmp);
    merge_sort(v, meio+1, dir,  cmp);
    mesclar(v, esq, meio, dir, cmp);
}

/* -------------------------------------------------------
 *  Critério 1 — Por Valor (Ordem Decrescente)
 *  Retorna negativo se a > b (para que Merge Sort coloque
 *  maior valor primeiro).
 * -------------------------------------------------------*/
static int cmp_valor_desc(const Transacao *a, const Transacao *b) {
    if (a->valor > b->valor) return -1;
    if (a->valor < b->valor) return  1;
    return 0;
}

void ordenar_por_valor(Transacao *v, int n) {
    if (n > 1) merge_sort(v, 0, n - 1, cmp_valor_desc);
}

/* -------------------------------------------------------
 *  Critério 2 — Por Cronologia (Data + Hora)
 *
 *  Estratégia de comparação:
 *    - Converter data "DD/MM/AAAA" → inteiro AAAAMMDD
 *    - Converter hora "HH:MM:SS"   → inteiro HHMMSS
 *    - Comparar data primeiro; em empate, comparar hora
 * -------------------------------------------------------*/
static int data_para_int(const char *data) {
    /* Formato: DD/MM/AAAA */
    int dd = 0, mm = 0, aaaa = 0;
    sscanf(data, "%d/%d/%d", &dd, &mm, &aaaa);
    return aaaa * 10000 + mm * 100 + dd;
}

static int hora_para_int(const char *hora) {
    /* Formato: HH:MM:SS */
    int hh = 0, mi = 0, ss = 0;
    sscanf(hora, "%d:%d:%d", &hh, &mi, &ss);
    return hh * 10000 + mi * 100 + ss;
}

static int cmp_data_hora_asc(const Transacao *a, const Transacao *b) {
    int da = data_para_int(a->data);
    int db = data_para_int(b->data);
    if (da != db) return da - db;
    return hora_para_int(a->hora) - hora_para_int(b->hora);
}

void ordenar_por_data_hora(Transacao *v, int n) {
    if (n > 1) merge_sort(v, 0, n - 1, cmp_data_hora_asc);
}

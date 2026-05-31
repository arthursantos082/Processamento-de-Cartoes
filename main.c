/* =========================================================
 *  main.c — Motor de Processamento Central
 *
 *  Fluxo completo:
 *    1. CSV → Binário (leitura única + higienização)
 *    2. Carrega binário em vetor
 *    3. Ordena por valor (Merge Sort, decrescente) com timer
 *    4. Ordena por data/hora (Merge Sort, crescente) com timer
 *    5. Preenche Fila, Pilha e Lista de acordo com regras
 *    6. Gera relatorio_consolidado.txt
 *    7. Libera toda a memória alocada
 * =========================================================*/

#include "transacao.h"

/* -------------------------------------------------------
 *  Auxiliar: verifica se a hora da transação está no
 *  período noturno de risco [23h-24h] ou [00h-05h].
 * -------------------------------------------------------*/
static int hora_eh_noturna(const char *hora) {
    int hh = 0;
    sscanf(hora, "%d", &hh);
    return (hh >= HORA_NOITE_INI || hh <= HORA_NOITE_FIM);
}

/* -------------------------------------------------------
 *  Auxiliar: classifica status para fila e pilha
 *    status 0 (Pendente)  → vai para a Fila
 *    status 2 (Rejeitada) → vai para a Pilha
 *  O critério de rejeição simulado: 1 em cada 20 transações
 *  pendentes é marcada como rejeitada por estouro de limite.
 *  (Em produção viria de regra de negócio real.)
 * -------------------------------------------------------*/
static int simular_status(int idx, int status_original) {
    if (status_original == 1) return 1; /* Fraude → Aprovada no sistema */
    /* Simula rejeição para ~5% das transações pendentes */
    if (idx % 20 == 0) return 2; /* Rejeitada */
    return 0;                    /* Pendente → Fila */
}

int main(int argc, char *argv[]) {

    /* Permite passar caminho do CSV como argumento */
    const char *csv_path = (argc > 1) ? argv[1] : "CreditCardData.csv";

    printf("==============================================\n");
    printf(" SISTEMA DE PROCESSAMENTO DE TRANSACOES\n");
    printf("==============================================\n\n");

    /* --------------------------------------------------
     *  ETAPA 1 — Converter CSV para binário
     * --------------------------------------------------*/
    printf("[1/5] Lendo CSV e gravando binario...\n");

    int lidas = 0, validas = 0, descartadas = 0;
    if (csv_para_binario(csv_path, ARQUIVO_BIN,
                         &lidas, &validas, &descartadas) != 0) {
        fprintf(stderr, "Falha na etapa de conversao CSV->Binario.\n");
        return 1;
    }

    printf("      Linhas lidas:      %d\n", lidas);
    printf("      Registros validos: %d\n", validas);
    printf("      Descartados:       %d\n\n", descartadas);

    /* --------------------------------------------------
     *  ETAPA 2 — Carregar binário em vetor
     * --------------------------------------------------*/
    printf("[2/5] Carregando binario em vetor...\n");

    int total = 0;
    Transacao *vetor = carregar_binario(ARQUIVO_BIN, &total);
    if (!vetor) {
        fprintf(stderr, "Falha ao carregar binario.\n");
        return 1;
    }
    printf("      %d transacoes carregadas.\n\n", total);

    /* --------------------------------------------------
     *  ETAPA 3 — Ordenação por valor (decrescente)
     * --------------------------------------------------*/
    printf("[3/5] Ordenando por valor (decrescente)...\n");

    clock_t t0 = clock();
    ordenar_por_valor(vetor, total);
    clock_t t1 = clock();
    double tempo_val = (double)(t1 - t0) / CLOCKS_PER_SEC;

    printf("      Concluido em %.4f segundos.\n", tempo_val);
    printf("      Maior: R$ %.2f  |  Menor: R$ %.2f\n\n",
           vetor[0].valor, vetor[total-1].valor);

    /* --------------------------------------------------
     *  ETAPA 4 — Ordenação por data/hora (crescente)
     *  (fazemos uma cópia para não perder a ordem por valor)
     * --------------------------------------------------*/
    printf("[4/5] Ordenando copia por data/hora (crescente)...\n");

    Transacao *vetor_data = (Transacao *)malloc((size_t)total * sizeof(Transacao));
    if (!vetor_data) { perror("malloc vetor_data"); free(vetor); return 1; }
    memcpy(vetor_data, vetor, (size_t)total * sizeof(Transacao));

    clock_t t2 = clock();
    ordenar_por_data_hora(vetor_data, total);
    clock_t t3 = clock();
    double tempo_data = (double)(t3 - t2) / CLOCKS_PER_SEC;

    printf("      Concluido em %.4f segundos.\n\n", tempo_data);

    /* --------------------------------------------------
     *  ETAPA 5 — Preencher Fila, Pilha e Lista
     *
     *  Percorremos o vetor ordenado por valor (decrescente)
     *  para alimentar as estruturas lineares.
     * --------------------------------------------------*/
    printf("[5/5] Preenchendo estruturas lineares...\n");

    Fila    fila;
    Pilha   pilha;
    NoLista *lista_suspeitas = NULL;

    fila_inicializar(&fila);
    pilha_inicializar(&pilha);

    for (int i = 0; i < total; i++) {
        Transacao t = vetor[i];

        /* Determina status operacional */
        t.status = simular_status(i, t.status);

        /* --- Fila: Pendentes (sem fraude original) --------- */
        if (t.status == 0) {
            fila_enfileirar(&fila, t);
        }

        /* --- Pilha: Rejeitadas ----------------------------- */
        if (t.status == 2) {
            pilha_empilhar(&pilha, t);
        }

        /* --- Lista: Alerta de Risco ------------------------ */
        if (t.valor > LIMIAR_RISCO && hora_eh_noturna(t.hora)) {
            lista_suspeitas = lista_inserir(lista_suspeitas, t);
        }
    }

    printf("      Fila:   %d transacoes\n", fila.tamanho);
    printf("      Pilha:  %d transacoes\n", pilha.tamanho);
    printf("      Lista:  %d transacoes suspeitas\n\n",
           lista_contar(lista_suspeitas));

    /* --------------------------------------------------
     *  Demo undo(): exibe a última transação rejeitada
     * --------------------------------------------------*/
    if (pilha.tamanho > 0) {
        Transacao revisao;
        pilha_desempilhar(&pilha, &revisao);
        printf("[UNDO] Ultima rejeitada retirada para revisao:\n");
        printf("       ID: %lu | Valor: R$ %.2f | Data: %s %s\n\n",
               revisao.id, revisao.valor, revisao.data, revisao.hora);
        /* Recolocamos na pilha para o relatório ficar correto */
        pilha_empilhar(&pilha, revisao);
    }

    /* --------------------------------------------------
     *  ETAPA 6 — Gerar relatório
     *  Passamos o vetor já ordenado por valor
     * --------------------------------------------------*/
    gerar_relatorio(ARQUIVO_REL,
                    tempo_val, tempo_data,
                    vetor, total,
                    &fila, &pilha,
                    lista_suspeitas);

    /* --------------------------------------------------
     *  ETAPA 7 — Liberar toda memória alocada
     * --------------------------------------------------*/
    printf("Liberando memoria...\n");
    fila_liberar(&fila);
    pilha_liberar(&pilha);
    lista_liberar(lista_suspeitas);
    free(vetor);
    free(vetor_data);

    printf("Memoria liberada. Sistema encerrado com sucesso.\n");
    return 0;
}

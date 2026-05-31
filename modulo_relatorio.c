/* =========================================================
 *  modulo_relatorio.c — Geração do relatório consolidado
 * =========================================================*/

#include "transacao.h"

/* Largura da linha de separação */
#define SEP "=================================================="

void gerar_relatorio(
    const char  *caminho,
    double       tempo_val,
    double       tempo_data,
    Transacao   *vetor,
    int          total,
    const Fila  *fila,
    const Pilha *pilha,
    NoLista     *lista_suspeitas)
{
    FILE *f = fopen(caminho, "w");
    if (!f) { perror("Erro ao criar relatório"); return; }

    /* ---- Cabeçalho ---------------------------------------- */
    fprintf(f, "%s\n", SEP);
    fprintf(f, "RELATORIO DE PERFORMANCE E AUDITORIA DE TRANSACOES\n");
    fprintf(f, "%s\n\n", SEP);

    /* ---- 1. Desempenho ------------------------------------- */
    fprintf(f, "1. DESEMPENHO DA ORDENACAO\n");
    fprintf(f, "   - Tempo para ordenar %d registros por valor: %.4f segundos.\n",
            total, tempo_val);
    fprintf(f, "   - Tempo para ordenar %d registros por data/hora: %.4f segundos.\n\n",
            total, tempo_data);

    /* ---- 2. Métricas financeiras --------------------------- */
    fprintf(f, "2. METRICAS FINANCEIRAS\n");

    double total_visa = 0.0, total_master = 0.0;
    Transacao maior = vetor[0];

    for (int i = 0; i < total; i++) {
        if (strcasecmp(vetor[i].bandeira, "Visa") == 0)
            total_visa += vetor[i].valor;
        else if (strcasecmp(vetor[i].bandeira, "MasterCard") == 0)
            total_master += vetor[i].valor;

        if (vetor[i].valor > maior.valor) maior = vetor[i];
    }

    fprintf(f, "   - Total processado por Bandeira (Visa):       R$ %12.2f\n", total_visa);
    fprintf(f, "   - Total processado por Bandeira (Mastercard): R$ %12.2f\n", total_master);
    fprintf(f, "   - Maior transacao detectada: ID %lu - Valor: R$ %.2f\n\n",
            maior.id, maior.valor);

    /* ---- 3. Status das estruturas -------------------------- */
    fprintf(f, "3. STATUS DAS ESTRUTURAS\n");
    fprintf(f, "   - Transacoes processadas na Fila  (FIFO):          %d transacoes.\n",
            fila->tamanho);
    fprintf(f, "   - Transacoes armazenadas na Pilha de Rejeicao:     %d transacoes.\n",
            pilha->tamanho);
    fprintf(f, "   - Transacoes criticas na Lista de Suspeitas:       %d transacoes.\n\n",
            lista_contar(lista_suspeitas));

    /* ---- Lista de suspeitas detalhada --------------------- */
    fprintf(f, "%s\n", SEP);
    fprintf(f, "LISTA DE TRANSACOES SUSPEITAS (VALOR ALTO + MADRUGADA)\n");
    fprintf(f, "Criterio: Valor > R$ %.0f  E  Hora em [23h-05h]\n", (double)LIMIAR_RISCO);
    fprintf(f, "%s\n\n", SEP);

    NoLista *cur = lista_suspeitas;
    if (!cur) {
        fprintf(f, "  Nenhuma transacao suspeita encontrada.\n");
    } else {
        while (cur) {
            fprintf(f,
                "[ID: %lu] - Data: %s %s - Valor: R$ %.2f - Categoria: %s - Bandeira: %s\n",
                cur->dado.id,
                cur->dado.data,
                cur->dado.hora,
                cur->dado.valor,
                cur->dado.categoria,
                cur->dado.bandeira);
            cur = cur->prox;
        }
    }

    /* ---- Detalhamento: Top 10 por valor ------------------- */
    fprintf(f, "\n%s\n", SEP);
    fprintf(f, "TOP 10 MAIORES TRANSACOES (ordenadas por valor decrescente)\n");
    fprintf(f, "%s\n\n", SEP);

    /* O vetor já foi ordenado por valor antes de chamar esta função */
    int lim = (total < 10) ? total : 10;
    for (int i = 0; i < lim; i++) {
        fprintf(f,
            "  %2d. [ID: %lu] Data: %s %s Valor: R$ %.2f Cat: %s Band: %s Status: %d\n",
            i + 1,
            vetor[i].id,
            vetor[i].data,
            vetor[i].hora,
            vetor[i].valor,
            vetor[i].categoria,
            vetor[i].bandeira,
            vetor[i].status);
    }

    /* ---- Detalhamento: Top 10 cronológicos ---------------- */
    fprintf(f, "\n%s\n", SEP);
    fprintf(f, "NOTA: Para detalhamento cronologico, reordene e consulte\n");
    fprintf(f, "      o arquivo binario transacoes.dat com a funcao\n");
    fprintf(f, "      ordenar_por_data_hora() e reimprima este bloco.\n");
    fprintf(f, "%s\n", SEP);

    fclose(f);
    printf("Relatorio gerado: %s\n", caminho);
}

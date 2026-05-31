/* =========================================================
 *  modulo_csv.c — Leitura do CSV e gravação do binário
 *
 *  Responsabilidades:
 *    - Abrir o arquivo .csv mestre UMA única vez
 *    - Higienizar cada linha (data cleaning)
 *    - Converter e gravar cada registro válido em .dat
 *
 *  Regras de higienização implementadas:
 *    1. Linha em branco → descartada
 *    2. Número de campos diferente de 16 → descartada
 *    3. Campo Amount sem prefixo "R$ " ou valor não-numérico → descartada
 *    4. Campo Fraud ausente ou não-numérico → descartada
 *    5. Transaction ID sem '#' → descartada
 *    6. Time fora de [0,24] → descartada
 * =========================================================*/

#include "transacao.h"

/* Tamanho máximo de uma linha CSV */
#define MAX_LINHA 512
/* Número esperado de colunas */
#define NUM_COLUNAS 16

/* ---------------------------------------------------------
 *  Auxiliar: remove espaços/tabs/\r nas extremidades
 * ---------------------------------------------------------*/
static void trim(char *s) {
    if (!s) return;
    /* trim à direita */
    int len = (int)strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' ||
                        s[len-1] == '\r' || s[len-1] == '\n')) {
        s[--len] = '\0';
    }
    /* trim à esquerda */
    int ini = 0;
    while (s[ini] == ' ' || s[ini] == '\t') ini++;
    if (ini > 0) memmove(s, s + ini, strlen(s + ini) + 1);
}

/* ---------------------------------------------------------
 *  Auxiliar: converte "14-Oct-20" → "14/10/2020"
 *  Retorna 0 se falhar.
 * ---------------------------------------------------------*/
static int converter_data(const char *src, char *dest) {
    /* Formato esperado: DD-Mon-YY */
    char tmp[32];
    strncpy(tmp, src, 31); tmp[31] = '\0';
    trim(tmp);

    char dd[3], mon[4], yy[3];
    if (sscanf(tmp, "%2[^-]-%3[^-]-%2s", dd, mon, yy) != 3) return 0;

    /* Mapa de meses abreviados em inglês */
    const char *meses[] = {
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };
    int m = 0, i;
    for (i = 0; i < 12; i++) {
        if (strcasecmp(mon, meses[i]) == 0) { m = i + 1; break; }
    }
    if (m == 0) return 0;

    int ano = atoi(yy);
    /* Heurística: anos 00-99 → 2000-2099 */
    ano += 2000;

    snprintf(dest, 11, "%02d/%02d/%04d", atoi(dd), m, ano);
    return 1;
}

/* ---------------------------------------------------------
 *  Auxiliar: converte hora inteira (0-24) → "HH:00:00"
 * ---------------------------------------------------------*/
static int converter_hora(const char *src, char *dest) {
    char tmp[16];
    strncpy(tmp, src, 15); tmp[15] = '\0';
    trim(tmp);

    char *endp;
    long h = strtol(tmp, &endp, 10);
    if (*endp != '\0' || h < 0 || h > 24) return 0;

    snprintf(dest, 9, "%02ld:00:00", h);
    return 1;
}

/* ---------------------------------------------------------
 *  Auxiliar: converte "R$ 288" → 288.0f
 *  Retorna 0 se falhar.
 * ---------------------------------------------------------*/
static int converter_valor(const char *src, float *dest) {
    char tmp[32];
    strncpy(tmp, src, 31); tmp[31] = '\0';
    trim(tmp);

    /* Aceita prefixo "R$ " com ou sem espaço */
    const char *p = tmp;
    if (strncmp(p, "R$", 2) == 0) p += 2;
    while (*p == ' ') p++;

    char *endp;
    double v = strtod(p, &endp);
    if (endp == p || *endp != '\0') return 0;
    if (v < 0.0) return 0;

    *dest = (float)v;
    return 1;
}

/* ---------------------------------------------------------
 *  Auxiliar: extrai número de "Transaction ID"
 *  Formato: "#XXXX XXX" — retira '#' e espaços
 * ---------------------------------------------------------*/
static unsigned long extrair_id(const char *src) {
    char tmp[32];
    strncpy(tmp, src, 31); tmp[31] = '\0';
    trim(tmp);

    char *p = tmp;
    if (*p == '#') p++;
    /* Remove espaço interno */
    char limpo[32]; int j = 0;
    while (*p) { if (*p != ' ') limpo[j++] = *p; p++; }
    limpo[j] = '\0';

    return (unsigned long)strtoul(limpo, NULL, 10);
}

/* ---------------------------------------------------------
 *  Auxiliar: determina status com base no campo Fraud
 *    Fraud=0 → Pendente (0)   Fraud=1 → Aprovada (1)
 *  (Nota: o projeto usa Fraud apenas para decisão de fila;
 *   o status pode ser refinado em módulos posteriores)
 * ---------------------------------------------------------*/
static int determinar_status(int fraud) {
    return (fraud == 0) ? 0 : 1;
}

/* ---------------------------------------------------------
 *  Função principal do módulo
 *
 *  Parâmetros de saída (ponteiros):
 *    lidas      — total de linhas lidas (excluindo cabeçalho)
 *    validas    — linhas que geraram um registro binário
 *    descartadas — linhas rejeitadas pela higienização
 *
 *  Retorna 0 em sucesso, -1 em erro de arquivo.
 * ---------------------------------------------------------*/
int csv_para_binario(const char *csv_path, const char *bin_path,
                     int *lidas, int *validas, int *descartadas) {

    *lidas = *validas = *descartadas = 0;

    FILE *fcsv = fopen(csv_path, "r");
    if (!fcsv) { perror("Erro ao abrir CSV"); return -1; }

    FILE *fbin = fopen(bin_path, "wb");
    if (!fbin) { perror("Erro ao criar binário"); fclose(fcsv); return -1; }

    char linha[MAX_LINHA];

    /* Pular cabeçalho */
    if (!fgets(linha, MAX_LINHA, fcsv)) {
        fclose(fcsv); fclose(fbin); return -1;
    }

    while (fgets(linha, MAX_LINHA, fcsv)) {

        /* --- Linha em branco -------------------------------- */
        char copia[MAX_LINHA];
        memcpy(copia, linha, MAX_LINHA - 1); copia[MAX_LINHA-1] = '\0';
        trim(copia);
        if (strlen(copia) == 0) { (*lidas)++; (*descartadas)++; continue; }

        (*lidas)++;

        /* --- Tokenizar por ';' ------------------------------ */
        char *campos[NUM_COLUNAS + 1];
        int nc = 0;
        char *tok = strtok(linha, ";");
        while (tok && nc <= NUM_COLUNAS) {
            campos[nc++] = tok;
            tok = strtok(NULL, ";");
        }
        if (nc != NUM_COLUNAS) { (*descartadas)++; continue; }

        /* Trim em todos os campos */
        for (int i = 0; i < nc; i++) trim(campos[i]);

        /* --- Validações individuais ------------------------- */

        /* Transaction ID */
        if (campos[0][0] != '#') { (*descartadas)++; continue; }
        unsigned long id = extrair_id(campos[0]);

        /* Date */
        char data[11];
        if (!converter_data(campos[1], data)) { (*descartadas)++; continue; }

        /* Time */
        char hora[9];
        if (!converter_hora(campos[3], hora)) { (*descartadas)++; continue; }

        /* Type of Card */
        if (strlen(campos[4]) == 0) { (*descartadas)++; continue; }

        /* Amount */
        float valor;
        if (!converter_valor(campos[6], &valor)) { (*descartadas)++; continue; }

        /* Merchant Group — pode ser vazio; usamos "Desconhecida" */
        const char *cat = (strlen(campos[8]) > 0) ? campos[8] : "Desconhecida";

        /* Fraud */
        char *endp;
        long fraud = strtol(campos[15], &endp, 10);
        if (*endp != '\0' || (fraud != 0 && fraud != 1)) {
            (*descartadas)++;
            continue;
        }

        /* --- Montar struct e gravar no binário -------------- */
        Transacao t;
        memset(&t, 0, sizeof(Transacao));

        t.id     = id;
        t.valor  = valor;
        t.status = determinar_status((int)fraud);

        strncpy(t.data,      data,    10);  t.data[10]     = '\0';
        strncpy(t.hora,      hora,     8);  t.hora[8]      = '\0';
        strncpy(t.bandeira,  campos[4], 19); t.bandeira[19] = '\0';
        strncpy(t.categoria, cat,       29); t.categoria[29]= '\0';

        fwrite(&t, sizeof(Transacao), 1, fbin);
        (*validas)++;
    }

    fclose(fcsv);
    fclose(fbin);
    return 0;
}

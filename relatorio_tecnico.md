# RELATÓRIO TÉCNICO — Motor de Processamento de Cartões

**Disciplina:** Estruturas de Dados  
**Projeto:** Sistema de Processamento de Transações Financeiras  
**Linguagem:** C (padrão C11)  
**Dataset:** CreditCardData.csv — 100.000 registros  

---

## 1. Visão Geral da Solução

O sistema foi implementado em cinco módulos independentes (`transacao.h`, `modulo_csv.c`, `modulo_ordenacao.c`, `modulo_estruturas.c`, `modulo_relatorio.c`) integrados por `main.c`. Cada módulo encapsula uma responsabilidade única, sem variáveis globais, usando exclusivamente passagem de parâmetros por valor ou por referência (ponteiros).

---

## 2. Módulos e Suas Responsabilidades

### 2.1 `transacao.h` — Cabeçalho Central

Define a `struct Transacao` e todos os tipos derivados (nós da fila, pilha e lista), além das constantes de negócio (`LIMIAR_RISCO = 200.0`, `HORA_NOITE_INI = 23`, `HORA_NOITE_FIM = 5`) e os protótipos de todas as funções públicas.

```c
typedef struct {
    unsigned long id;
    char  data[11];      // DD/MM/AAAA
    char  hora[9];       // HH:MM:SS
    char  bandeira[20];  // Visa / MasterCard
    char  categoria[30]; // Entertainment, Food...
    float valor;
    int   status;        // 0=Pendente 1=Aprovada 2=Rejeitada
} Transacao;
```

A decisão de usar `float` para o valor (e não `double`) respeita a especificação do enunciado. Para somas acumuladas no relatório, usamos `double` internamente para evitar perda de precisão.

---

### 2.2 `modulo_csv.c` — Conversão CSV → Binário

**Função principal:** `csv_para_binario(csv_path, bin_path, *lidas, *validas, *descartadas)`

O arquivo CSV é lido **uma única vez**, linha por linha. Para cada linha são aplicadas as seguintes regras de **higienização (data cleaning)**:

| Regra | Ação |
|---|---|
| Linha em branco | Descartada |
| Número de campos ≠ 16 | Descartada |
| Transaction ID sem `#` | Descartada |
| Time fora de [0, 24] | Descartada |
| Amount sem prefixo `R$` ou não-numérico | Descartada |
| Fraud com valor inválido (≠ 0 ou 1) | Descartada |
| Merchant Group vazio | Substituído por `"Desconhecida"` (não descartado) |

**Conversões aplicadas:**
- **Data:** `"14-Oct-20"` → `"14/10/2020"` via tabela de meses abreviados em inglês
- **Hora:** inteiro `23` → `"23:00:00"` (o dataset armazena apenas a hora cheia)
- **Valor:** `"R$ 288"` → `288.0f` (remove prefixo, converte com `strtod`)
- **ID:** `"#3577 209"` → `3577209UL` (remove `#` e espaço interno)

Após validação, cada `Transacao` é gravada no arquivo binário com `fwrite(&t, sizeof(Transacao), 1, fbin)`.

**Por que usar binário em vez de texto?**  
Arquivos binários de structs permitem leitura em O(1) por índice (`fseek + fread`), sem necessidade de parsing. Um acesso aleatório ao registro N custa apenas `fseek(f, N * sizeof(Transacao), SEEK_SET)`. Em sistemas de alta performance como os das operadoras de cartão, essa diferença é crítica para milhões de acessos/segundo.

**Resultado no dataset:**
- 100.000 linhas lidas
- 99.977 registros válidos
- 23 descartados (campos nulos em Amount, Merchant Group, etc.)

---

### 2.3 `modulo_ordenacao.c` — Carga e Ordenação

**Função de carga:** `carregar_binario(bin_path, *total)`  
Calcula o número de registros como `tamanho_do_arquivo / sizeof(Transacao)`, aloca o vetor dinamicamente com `malloc` e lê tudo de uma vez com `fread`.

**Algoritmo escolhido: Merge Sort**

A escolha do Merge Sort foi fundamentada na análise do dataset:

| Critério | Justificativa |
|---|---|
| Pior caso | O(n log n) **garantido** — diferente do Quick Sort que degrada para O(n²) em arrays com muitas chaves repetidas |
| Chaves repetidas | O campo `valor` tem altíssima repetição (ex: `R$17` aparece 2.153 vezes). Quick Sort com pivô ingênuo sobre esse dado pode aproximar O(n²) |
| Estabilidade | Merge Sort é estável — registros com mesma data/hora mantêm ordem relativa |
| Ordenação por chave composta | Natural: a função de comparação de data/hora converte `DD/MM/AAAA HH:MM:SS` para inteiros e compara em duas etapas |

**Funções de comparação:**

```c
// Critério 1 — Valor decrescente
static int cmp_valor_desc(const Transacao *a, const Transacao *b) {
    if (a->valor > b->valor) return -1;
    if (a->valor < b->valor) return  1;
    return 0;
}

// Critério 2 — Cronologia (data + hora crescente)
static int cmp_data_hora_asc(const Transacao *a, const Transacao *b) {
    int da = data_para_int(a->data);  // AAAAMMDD
    int db = data_para_int(b->data);
    if (da != db) return da - db;
    return hora_para_int(a->hora) - hora_para_int(b->hora); // HHMMSS
}
```

**Tempos obtidos (100k registros, hardware de teste):**
- Ordenação por valor: ~0,035 s
- Ordenação por data/hora: ~0,86 s (chave composta com mais comparações)

---

### 2.4 `modulo_estruturas.c` — Fila, Pilha e Lista

#### Fila FIFO — Autorização de Crédito

Implementada com **dois ponteiros** (`inicio` e `fim`) para garantir `enqueue` e `dequeue` em O(1).

```
[inicio] → [nó1] → [nó2] → ... → [nóN] ← [fim]
```

- **Entrada:** transações com `status == 0` (Pendentes, sem fraude original)
- **Saída (`fila_desenfileirar`):** retira sempre do `inicio`
- **Invariante:** `fim == NULL ↔ inicio == NULL`

#### Pilha LIFO — Auditoria de Rejeições

Implementada com **um ponteiro de topo** e encadeamento `abaixo`.

```
[topo] → [nóN] → [nóN-1] → ... → [nó1] → NULL
```

- **Entrada:** transações com `status == 2` (Rejeitadas)
- **`undo()`:** implementado como `pilha_desempilhar`, que retira o topo e retorna a transação para revisão

#### Lista Encadeada — Alertas de Risco

Critério de inserção: `valor > LIMIAR_RISCO (R$200) E hora ∈ [23h-24h] ∪ [0h-5h]`

Inserção sempre ao **final** (ordem de chegada preservada). Alocação estritamente com `malloc` por nó.

```c
NoLista *lista_inserir(NoLista *cab, Transacao t) {
    NoLista *novo = (NoLista *)malloc(sizeof(NoLista));
    // percorre até o fim e encadeia
}
```

**Resultado:** 1.938 transações suspeitas identificadas.

---

### 2.5 `modulo_relatorio.c` — Geração do Relatório

Gera `relatorio_consolidado.txt` com:
1. Tempos de ordenação
2. Totais por bandeira (Visa: R$6.065.110 | Mastercard: R$5.190.294)
3. Maior transação detectada
4. Contagem das três estruturas
5. Lista completa de transações suspeitas
6. Top 10 maiores transações com detalhamento completo

---

### 2.6 `main.c` — Integração e Controle de Fluxo

Orquestra os seis módulos na sequência:

```
CSV → Binário → Vetor → Ordenação × 2 → Estruturas → Relatório → Free
```

A ordenação por data/hora opera sobre **uma cópia do vetor** (`malloc + memcpy`), preservando a ordenação por valor para o relatório. Ambas as cópias são `free()`adas no final.

O `undo()` é demonstrado ao vivo: a última transação rejeitada é retirada da pilha, exibida no console e recolocada para constar corretamente no relatório.

---

## 3. Gerenciamento de Memória

| Estrutura | Alocação | Liberação |
|---|---|---|
| Vetor principal | `malloc` em `carregar_binario` | `free(vetor)` em `main` |
| Vetor cópia (data/hora) | `malloc` + `memcpy` em `main` | `free(vetor_data)` em `main` |
| Nós da Fila | `malloc` por `fila_enfileirar` | `fila_liberar` → `fila_desenfileirar` em loop |
| Nós da Pilha | `malloc` por `pilha_empilhar` | `pilha_liberar` → `pilha_desempilhar` em loop |
| Nós da Lista | `malloc` por `lista_inserir` | `lista_liberar` em loop |

Nenhuma memória é abandonada. Verificado logicamente pelo encadeamento de liberação em cada estrutura.

---

## 4. Tratamento de Casos Especiais no Dataset

O campo `Time` no CSV contém apenas a **hora inteira** (0–24), sem minutos e segundos. A conversão `"23"` → `"23:00:00"` é feita em `converter_hora()`. O valor `24` é aceito (equivale a meia-noite/madrugada) e convertido para `"24:00:00"`.

O campo `Amount` usa o prefixo `R$ ` (com espaço). A função `converter_valor()` aceita `R$` com ou sem espaço após, tornando o parser robusto a variações de formatação.

IDs como `#3577 209` têm um espaço interno que é removido antes de converter para `unsigned long`, resultando em `3577209`.

---

## 5. Compilação e Execução

```bash
# Compilar
make

# Executar (com caminho do CSV como argumento)
./processador CreditCardData.csv

# Ou sem argumento (assume CreditCardData.csv no diretório atual)
./processador
```

Saídas geradas:
- `transacoes.dat` — banco binário com 99.977 registros
- `relatorio_consolidado.txt` — relatório completo

---

## 6. Declaração de Uso de Inteligência Artificial

Este projeto utilizou **Claude (Anthropic)** como ferramenta de auxílio no desenvolvimento. A IA foi utilizada para:

- Análise exploratória do dataset CSV (identificação do separador `;`, padrões de campos, valores nulos)
- Estruturação da arquitetura modular e distribuição de responsabilidades entre arquivos
- Implementação das funções de higienização, conversão de data/hora e todas as estruturas de dados
- Geração do código C completo, incluindo detecção e correção de warnings de compilação
- Redação deste relatório técnico

Todo o código foi verificado, executado e validado contra o dataset real de 100.000 registros.

---

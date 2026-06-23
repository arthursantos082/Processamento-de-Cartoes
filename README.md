# Motor de Processamento de Transações Financeiras

Sistema de backend desenvolvido em **C (padrão C11)** que simula o motor central de uma operadora de cartões, processando ~100.000 transações financeiras com estruturas de dados fundamentais.

---

## Sobre o Projeto

Desenvolvido como projeto acadêmico da disciplina de **Estruturas de Dados**, o sistema replica o ciclo de vida de uma transação financeira: leitura, higienização, armazenamento binário, ordenação, autorização, auditoria e geração de relatório.

---

## Estrutura de Arquivos

```
├── transacao.h           # Cabeçalho central: struct, tipos e protótipos
├── main.c                # Integração e controle do fluxo principal
├── modulo_csv.c          # Leitura do CSV e gravação do binário
├── modulo_ordenacao.c    # Carga do binário e Merge Sort (2 critérios)
├── modulo_estruturas.c   # Fila, Pilha e Lista Encadeada
├── modulo_relatorio.c    # Geração do relatório consolidado
├── Makefile              # Compilação automatizada
└── relatorio_tecnico.md  # Documentação técnica detalhada
```

---

## Como Compilar e Executar

### Pré-requisitos
- GCC instalado
- Dataset `CreditCardData.csv` na mesma pasta

### Compilar
```bash
make
```

### Executar
```bash
./processador CreditCardData.csv
```

### Limpar arquivos gerados
```bash
make clean
```

---

## Fluxo do Sistema

```
CreditCardData.csv
       ↓
  [Higienização]          ← descarta linhas inválidas/corrompidas
       ↓
  transacoes.dat          ← banco binário indexado
       ↓
  [Carga em vetor]
       ↓
  ┌────────────────────────────────┐
  │  Merge Sort por Valor (↓)      │  O(n log n) garantido
  │  Merge Sort por Data/Hora (↑)  │
  └────────────────────────────────┘
       ↓
  ┌─────────────────────────────────────────┐
  │  Fila FIFO   → Autorização de Crédito   │
  │  Pilha LIFO  → Auditoria + undo()       │
  │  Lista       → Alertas de Risco         │
  └─────────────────────────────────────────┘
       ↓
  relatorio_consolidado.txt
```

---

## Resultados com o Dataset Real (100k registros)

| Métrica | Resultado |
|---|---|
| Registros válidos | 99.977 |
| Descartados (higienização) | 23 |
| Tempo ordenação por valor | ~0,035 s |
| Tempo ordenação por data/hora | ~0,860 s |
| Fila de autorização (FIFO) | 88.148 transações |
| Pilha de auditoria (LIFO) | 4.637 transações |
| Lista de suspeitas | 1.938 transações |
| Total Visa | R$ 6.065.110,00 |
| Total Mastercard | R$ 5.190.294,00 |

---

## Estruturas de Dados Implementadas

### Fila FIFO — Autorização de Crédito
- Dois ponteiros (`inicio` e `fim`) para enqueue/dequeue em O(1)
- Entrada: transações sem fraude detectada (`Fraud == 0`)

### Pilha LIFO — Auditoria de Rejeições
- Ponteiro de topo com encadeamento `abaixo`
- Função `undo()` retira a última transação rejeitada para revisão

### Lista Encadeada — Alertas de Risco
- Alocação estritamente dinâmica (`malloc` por nó)
- Critério: `Valor > R$200` **E** `Hora ∈ [23h–05h]`

---

## Algoritmo de Ordenação

**Merge Sort** foi escolhido sobre Quick Sort pela seguinte razão: o campo `valor` possui altíssima repetição no dataset (ex: R$17 aparece 2.153 vezes), o que pode degradar o Quick Sort para O(n²). O Merge Sort garante **O(n log n) em todos os casos** e é estável.

---
## 👤 Autor

**Arthur Santos**  
Repositório: [Processamento-de-Cartoes](https://github.com/arthursantos082/Processamento-de-Cartoes)

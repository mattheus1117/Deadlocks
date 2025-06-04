# Simulador de Controle de Concorrência com Wound-Wait e Wait-For Graph

Este projeto em C++ simula um sistema de controle de concorrência com múltiplas transações concorrentes acessando recursos compartilhados. Ele implementa o protocolo **Wound-Wait** para evitar deadlocks e um **Wait-For Graph** para visualizar dependências entre transações.

---

## Funcionamento Geral

- Simula múltiplas **transações** acessando dois recursos: `X` e `Y`.
- Cada recurso possui:
  - Lock exclusivo
  - Fila de espera
  - Controle via `mutex` e `condition_variable`

---

## Protocolo Wound-Wait

- Cada transação recebe um **timestamp** ao iniciar.
- Ao tentar obter um lock:
  - Se a transação for **mais velha** que quem detém o lock, ela **fere (wound)** a mais nova (abortando-a).
  - Se for **mais nova**, ela **espera** até o recurso ser liberado.
- Transações abortadas liberam seus locks e **reiniciam** automaticamente.

---

## Fluxo de Execução

1. Transações tentam adquirir locks em `X` e `Y`.
2. Sistema aplica a lógica do **Wound-Wait** em caso de conflito.
3. Locks são adquiridos ou transações entram em espera.
4. Transações notificam próximas na fila ao liberar locks.
5. Transações realizam **commit** ao final.
6. Um **Wait-For Graph** é exibido mostrando dependências criadas.

---

## Recursos Usados

- `std::thread` para simular concorrência.
- `std::mutex` e `std::condition_variable` para controle de acesso aos recursos.
- `unordered_map`, `unordered_set`, `queue` para estruturas de dados auxiliares.
- `DFS` para detecção de ciclos no grafo de espera (deadlocks).

---

## Execução

Compile e execute o programa com:

g++ -std=c++17 main.cpp -o simulador -lpthread

./simulador
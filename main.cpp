#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <cstdlib>
#include <ctime>

using namespace std;

struct Recurso {
    string item_id;
    bool valor_lock = false;
    int transacao = -1;
    queue<int> fila;
    mutex mtx;
    condition_variable cv;
};

Recurso X {"X", false, -1, {}, {}, {}};
Recurso Y {"Y", false, -1, {}, {}, {}};

unordered_map<int, unordered_set<int>> wait_for_graph;
mutex mtx_wait_for_graph;

unordered_map<int, int> timestamps;
int global_timestamp = 0;
mutex mtx_timestamp;

bool dfs(int atual, unordered_set<int>& visitados, unordered_set<int>& pilha) {
    visitados.insert(atual);
    pilha.insert(atual);

    for (int vizinho : wait_for_graph[atual]) {
        if (pilha.count(vizinho)) return true;
        if (!visitados.count(vizinho)) {
            if (dfs(vizinho, visitados, pilha)) return true;
        }
    }

    pilha.erase(atual);
    return false;
}

void print_wait_for_graph() {
    cout << "\nGrafo de Espera (Wait-For Graph):\n";
    for (auto& [tid, deps] : wait_for_graph) {
        cout << "T" << tid << " -> ";
        for (int d : deps) cout << "T" << d << " ";
        cout << endl;
    }
}

void randon() {
    int tempo = (rand() % 1001) + 1000;
    this_thread::sleep_for(chrono::milliseconds(tempo));
}

bool lock_item(Recurso &r, int transacao_id, bool &abortada) {
    unique_lock<mutex> lock(r.mtx);

    int ts_req;
    {
        lock_guard<mutex> ts_lock(mtx_timestamp);
        ts_req = timestamps[transacao_id];
    }

    while (r.valor_lock) {
        int ts_dono;
        {
            lock_guard<mutex> ts_lock(mtx_timestamp);
            ts_dono = timestamps[r.transacao];
        }

        if (ts_req < ts_dono) {
            cout << "Transacao " << transacao_id << " (TS=" << ts_req << ") feriu T" << r.transacao << " (TS=" << ts_dono << ") em " << r.item_id << endl;

            // Libera lock do mais novo
            r.valor_lock = false;
            r.transacao = -1;
            r.cv.notify_all();  // Acorda quem estiver esperando
            break;  // Reavalia para pegar o lock
        } else {
            cout << "Transacao " << transacao_id << " (TS=" << ts_req << ") esperando por T" << r.transacao << " (TS=" << ts_dono << ") em " << r.item_id << endl;

            wait_for_graph[transacao_id].insert(r.transacao);
            r.fila.push(transacao_id);

            r.cv.wait(lock, [&]{ return !r.valor_lock && r.fila.front() == transacao_id; });

            if (r.fila.front() == transacao_id) r.fila.pop();
            wait_for_graph.erase(transacao_id);
        }
    }

    if (abortada) return false;

    r.valor_lock = true;
    r.transacao = transacao_id;
    wait_for_graph.erase(transacao_id);
    cout << "Transacao " << transacao_id << " obteve lock em " << r.item_id << endl;

    return true;
}

void unlock_item(Recurso &r, int transacao_id) {
    unique_lock<mutex> lock(r.mtx);
    if (r.transacao == transacao_id) {
        r.valor_lock = false;
        r.transacao = -1;

        for (auto it = wait_for_graph.begin(); it != wait_for_graph.end(); ) {
            it->second.erase(transacao_id);
            if (it->second.empty()) {
                it = wait_for_graph.erase(it);
            } else {
                ++it;
            }
        }

        cout << "Transacao " << transacao_id << " liberou lock em " << r.item_id << endl;
        r.cv.notify_all();
    }
}

void transacao_dinamica(int id) {
    {
        lock_guard<mutex> lock(mtx_timestamp);
        timestamps[id] = global_timestamp++;
    }

    bool abortada = false;
    srand(time(nullptr) + id);

    if (rand() % 2 == 0) {
        if (!lock_item(X, id, abortada)) return;
        randon();
        if (!lock_item(Y, id, abortada)) {
            unlock_item(X, id);
            cout << "Transacao " << id << " foi abortada.\n";
            return;
        }

        randon();
        unlock_item(Y, id);
        randon();
        unlock_item(X, id);
    } else {
        if (!lock_item(Y, id, abortada)) return;
        randon();
        if (!lock_item(X, id, abortada)) {
            unlock_item(Y, id);
            cout << "Transacao " << id << " foi abortada.\n";
            return;
        }

        randon();
        unlock_item(X, id);
        randon();
        unlock_item(Y, id);
    }

    randon();
    cout << "Transacao " << id << " fez commit." << endl;
}

int main() {
    srand(time(nullptr));

    const int N = 4;
    vector<thread> threads;

    for (int i = 0; i < N; ++i) {
        threads.emplace_back(transacao_dinamica, i + 1);
    }

    for (auto &t : threads) {
        t.join();
    }

    print_wait_for_graph();

    return 0;
}
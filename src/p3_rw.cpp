

#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <ctime>

// -------------------- Configuración del "mapa" --------------------
constexpr int NBUCKET = 1024;          // Potencia de 2 para usar máscara
struct Node { int k, v; Node* next; };

struct Map {
    Node* b[NBUCKET]{};                // buckets
    pthread_rwlock_t rw = PTHREAD_RWLOCK_INITIALIZER; // RW lock
};

static inline int hashk(int k) {
    // Hash simple (no criptográfico), usa máscara con NBUCKET-1
    return (k * 1103515245u + 12345u) & (NBUCKET - 1);
}

// GET protegido con lock de lectura
int map_get(Map* m, int k) {
    pthread_rwlock_rdlock(&m->rw);
    int h = hashk(k);
    Node* cur = m->b[h];
    int ans = -1;
    while (cur) {
        if (cur->k == k) { ans = cur->v; break; }
        cur = cur->next;
    }
    pthread_rwlock_unlock(&m->rw);
    return ans;
}

// PUT protegido con lock de escritura
void map_put(Map* m, int k, int v) {
    pthread_rwlock_wrlock(&m->rw);
    int h = hashk(k);
    Node* cur = m->b[h];
    while (cur) {
        if (cur->k == k) { cur->v = v; pthread_rwlock_unlock(&m->rw); return; }
        cur = cur->next;
    }
    Node* n = new Node{k, v, m->b[h]};
    m->b[h] = n;
    pthread_rwlock_unlock(&m->rw);
}

// -------------------- Carga de trabajo --------------------
struct Args {
    Map* m;
    int iters;
    int read_pct; // 0..100
};

void* worker(void* p) {
    auto* a = static_cast<Args*>(p);

    // Semilla diferente por hilo: usa el ID del hilo + tiempo actual
    uintptr_t tid = (uintptr_t)pthread_self();
    unsigned seed = static_cast<unsigned>(tid ^ static_cast<uintptr_t>(time(nullptr)));

    for (int i = 0; i < a->iters; ++i) {
        int k = rand_r(&seed) & 4095;             // clave en [0..4095]
        int r = rand_r(&seed) % 100;              // decide lectura/escritura
        if (r < a->read_pct) (void)map_get(a->m, k);
        else                 map_put(a->m, k, k + 1);
    }
    return nullptr;
}

// -------------------- Programa principal --------------------
int main(int argc, char** argv) {
    int T    = (argc > 1) ? std::atoi(argv[1]) : 8;        // hilos
    int READ = (argc > 2) ? std::atoi(argv[2]) : 90;       // % lecturas
    int it   = (argc > 3) ? std::atoi(argv[3]) : 200000;   // iters por hilo

    Map m;

    // Precarga de claves para tener hits en lectura y evitar inserts al principio
    for (int k = 0; k < 4096; ++k) map_put(&m, k, 0);

    std::vector<pthread_t> th(T);
    Args a{&m, it, READ};

    // (Opcional) Temporización sencilla
    auto now_ms = []{
        struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
        return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
    };
    long long t0 = now_ms();

    for (int i = 0; i < T; ++i) pthread_create(&th[i], nullptr, worker, &a);
    for (int i = 0; i < T; ++i) pthread_join(th[i], nullptr);

    long long t1 = now_ms();
    std::printf("[P3-rwlock] T=%d READ=%d%% iters/hilo=%d  tiempo_ms=%lld\n",
                T, READ, it, (t1 - t0));


    return 0;
}

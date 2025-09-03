#include <pthread.h>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

constexpr int NBUCKET = 1024;

struct Node { int k,v; Node* next; };
struct Map {
    Node* b[NBUCKET]{};
    pthread_rwlock_t rw = PTHREAD_RWLOCK_INITIALIZER; // cambiar por mutex para comparar
};

int  hashk(int k){ return (k*1103515245u + 12345u) & (NBUCKET-1); }

int map_get(Map* m, int k){
    pthread_rwlock_rdlock(&m->rw);
    int h = hashk(k);
    Node* cur = m->b[h];
    int ans = 0, found = 0;
    while (cur){ if (cur->k==k){ ans=cur->v; found=1; break; } cur=cur->next; }
    pthread_rwlock_unlock(&m->rw);
    return found? ans : -1;
}

void map_put(Map* m, int k, int v){
    pthread_rwlock_wrlock(&m->rw);
    int h = hashk(k);
    Node* cur = m->b[h];
    while (cur){ if (cur->k==k){ cur->v=v; pthread_rwlock_unlock(&m->rw); return; } cur=cur->next; }
    Node* n = new Node{k,v,m->b[h]};
    m->b[h] = n;
    pthread_rwlock_unlock(&m->rw);
}

// workload: mezcla de lecturas/escrituras
struct Args { Map* m; int iters; int read_pct; };
void* worker(void* p){
    auto* a = (Args*)p;
    unsigned seed = (unsigned)p;
    for (int i=0;i<a->iters;i++){
        int k = rand_r(&seed) & 4095;
        int r = rand_r(&seed) % 100;
        if (r < a->read_pct) (void)map_get(a->m, k);
        else                 map_put(a->m, k, k+1);
    }
    return nullptr;
}

int main(int argc, char** argv){
    int T = (argc>1)? std::atoi(argv[1]) : 8;
    int READ = (argc>2)? std::atoi(argv[2]) : 90; // 90/70/50
    int it = (argc>3)? std::atoi(argv[3]) : 200000;

    Map m;
    // pre-cargar algunas llaves
    for (int k=0;k<4096;k++) map_put(&m,k,0);

    std::vector<pthread_t> th(T);
    Args a{&m, it, READ};
    for (int i=0;i<T;i++) pthread_create(&th[i], nullptr, worker, &a);
    for (int i=0;i<T;i++) pthread_join(th[i], nullptr);

    std::puts("[rwlock] terminado (cambia a mutex para comparar)");
    return 0;
}

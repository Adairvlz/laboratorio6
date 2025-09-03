#include <pthread.h>
#include <vector>
#include <cstdio>
#include <cstdlib>

struct Args {
    long iters;
    long* global;
    pthread_mutex_t* mtx;
};

// Versión insegura (race)
void* worker_naive(void* p){
    auto* a = (Args*)p;
    for (long i = 0; i < a->iters; ++i) (*a->global)++; // RACE intencional
    return nullptr;
}

// Versión segura (mutex)
void* worker_mutex(void* p){
    auto* a = (Args*)p;
    for (long i = 0; i < a->iters; ++i) {
        pthread_mutex_lock(a->mtx);
        (*a->global)++;
        pthread_mutex_unlock(a->mtx);
    }
    return nullptr;
}

// Versión sharded (cada hilo acumula local)
void* worker_sharded(void* p){
    auto* a = (Args*)p;
    long local = 0;
    for (long i = 0; i < a->iters; ++i) local++;
    // reduce (sumar al global): proteger este paso corto
    pthread_mutex_lock(a->mtx);
    (*a->global) += local;
    pthread_mutex_unlock(a->mtx);
    return nullptr;
}

int main(int argc, char** argv){
    int  T  = (argc>1)? std::atoi(argv[1]) : 4;
    long it = (argc>2)? std::atol (argv[2]) : 1000000;

    auto run = [&](void*(*fn)(void*), const char* tag){
        long global = 0;
        pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
        std::vector<pthread_t> th(T);
        Args a{it, &global, &mtx};
        for (int i=0;i<T;++i) pthread_create(&th[i], nullptr, fn, &a);
        for (int i=0;i<T;++i) pthread_join(th[i], nullptr);
        std::printf("[%s] total=%ld (esperado=%ld)\n", tag, global, (long)T*it);
    };

    run(worker_naive,   "NAIVE");   // resultado no determinista (race)
    run(worker_mutex,   "MUTEX");   // correcto
    run(worker_sharded, "SHARDED"); // correcto (y suele más rápido)
    return 0;
}

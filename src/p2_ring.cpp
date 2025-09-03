#include <pthread.h>
#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <vector>
#include <unistd.h>

constexpr std::size_t Q = 1024;

struct Ring {
    int buf[Q];
    std::size_t head=0, tail=0, count=0;
    pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t  not_full  = PTHREAD_COND_INITIALIZER;
    pthread_cond_t  not_empty = PTHREAD_COND_INITIALIZER;
    bool stop=false;
};

void ring_push(Ring* r, int v){
    pthread_mutex_lock(&r->m);
    while (r->count==Q && !r->stop) pthread_cond_wait(&r->not_full, &r->m);
    if (!r->stop) {
        r->buf[r->head] = v;
        r->head = (r->head + 1) % Q;
        r->count++;
        pthread_cond_signal(&r->not_empty);
    }
    pthread_mutex_unlock(&r->m);
}

bool ring_pop(Ring* r, int* out){
    pthread_mutex_lock(&r->m);
    while (r->count==0 && !r->stop) pthread_cond_wait(&r->not_empty, &r->m);
    if (r->count==0 && r->stop) { pthread_mutex_unlock(&r->m); return false; }
    *out = r->buf[r->tail];
    r->tail = (r->tail + 1) % Q;
    r->count--;
    pthread_cond_signal(&r->not_full);
    pthread_mutex_unlock(&r->m);
    return true;
}

void* producer(void* p){
    Ring* r = (Ring*)p;
    for (int i=1; i<=5000; ++i) ring_push(r, i);
    pthread_mutex_lock(&r->m);
    r->stop = true;
    pthread_cond_broadcast(&r->not_empty);
    pthread_mutex_unlock(&r->m);
    return nullptr;
}

void* consumer(void* p){
    Ring* r = (Ring*)p;
    int x;
    long cnt=0;
    while (ring_pop(r, &x)) { cnt++; /* procesar x */ }
    std::printf("[consumidor] items=%ld\n", cnt);
    return nullptr;
}

int main(){
    Ring r;
    pthread_t prod, cons;
    pthread_create(&prod, nullptr, producer, &r);
    pthread_create(&cons, nullptr, consumer, &r);
    pthread_join(prod, nullptr);
    pthread_join(cons, nullptr);
    return 0;
}

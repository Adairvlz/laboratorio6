#include <pthread.h>
#include <cstdio>

constexpr int TICKS = 1000;
static pthread_barrier_t barrier;
static pthread_once_t once_flag = PTHREAD_ONCE_INIT;

static void init_shared(){
    // abrir log, reservar buffers, etc.
    std::puts("[init] corrida inicializada");
}

void* stage(void* p){
    long id = (long)p;            // 1=genera, 2=filtra, 3=reduce
    pthread_once(&once_flag, init_shared);
    for (int t=0; t<TICKS; ++t){
        // trabajo simulado por etapa:
        // if(id==1) generar(); else if(id==2) filtrar(); else reducir();
        pthread_barrier_wait(&barrier); // sincronizar tick
    }
    return nullptr;
}

int main(){
    pthread_t h1,h2,h3;
    pthread_barrier_init(&barrier, NULL, 3);
    pthread_create(&h1,0,stage,(void*)1);
    pthread_create(&h2,0,stage,(void*)2);
    pthread_create(&h3,0,stage,(void*)3);
    pthread_join(h1,0); pthread_join(h2,0); pthread_join(h3,0);
    pthread_barrier_destroy(&barrier);
    return 0;
}

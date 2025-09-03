#include <pthread.h>
#include <unistd.h>
#include <cstdio>

// Caso con deadlock (orden opuesto)
pthread_mutex_t A=PTHREAD_MUTEX_INITIALIZER, B=PTHREAD_MUTEX_INITIALIZER;

void* t1(void*){
    pthread_mutex_lock(&A); usleep(1000);
    pthread_mutex_lock(&B);
    std::puts("t1 ok");
    return nullptr;
}
void* t2(void*){
    pthread_mutex_lock(&B); usleep(1000);
    pthread_mutex_lock(&A);
    std::puts("t2 ok");
    return nullptr;
}

int main(){
    pthread_t x,y;
    pthread_create(&x,nullptr,t1,nullptr);
    pthread_create(&y,nullptr,t2,nullptr);
    pthread_join(x,nullptr);
    pthread_join(y,nullptr);
    return 0;
}

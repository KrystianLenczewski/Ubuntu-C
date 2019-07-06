#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
 
 
pthread_mutex_t czytelnik=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t zasob=PTHREAD_MUTEX_INITIALIZER;
 
int ilosc_czytelnikow;
int pracujacy_czytelnicy = 0;
int ilosc_pisarzy;
int pracujacy_pisarze = 0;
 

void *czytelnicy(void *arg) {
 int *id = (int*)arg;
    while (1) {
        pthread_mutex_lock(&czytelnik);  //mutex na czytelnika, tylko jeden czytelnik na raz
 
        if (pracujacy_czytelnicy == 0) {
            pthread_mutex_lock(&zasob); //jezli jeszcze nikogo nie ma w czytelni, to blokada na zasób
            }                                //aby zaden pisarz nie wszedl
        pracujacy_czytelnicy++;
printf("Czytelnik o id=%d\n",*id);
  printf("Na zewnatrz: %d czytelnikow i %d pisarzy. W czytelni: %d czytelnikow i %d pisarzy\n", (ilosc_czytelnikow-pracujacy_czytelnicy), (ilosc_pisarzy-pracujacy_pisarze), pracujacy_czytelnicy, pracujacy_pisarze);
        pthread_mutex_unlock(&czytelnik);
 
        sleep(1);
//usleep(200*1000);
        pthread_mutex_lock(&czytelnik);
    pracujacy_czytelnicy--;
 
        if (pracujacy_czytelnicy == 0) { // gdy wychodzi ostatni czytelnik, odblokowuje zasób->moga wejsc pisarze
            pthread_mutex_unlock(&zasob);
        }
        pthread_mutex_unlock(&czytelnik);
 
 
    }
    return 0;
}
 
void *pisarze(void *arg) {
 int *id = (int*)arg;
    while (1) {
        pthread_mutex_lock(&zasob);
    pracujacy_pisarze++;
printf("Pisarz o id=%d\n",*id);
    printf("Na zewnatrz: %d czytelnikow i %d pisarzy. W czytelni: %d czytelnikow i %d pisarzy\n", (ilosc_czytelnikow-pracujacy_czytelnicy), (ilosc_pisarzy-pracujacy_pisarze), pracujacy_czytelnicy, pracujacy_pisarze);
        sleep(1);
//usleep(200*1000);
       pracujacy_pisarze--;
        pthread_mutex_unlock(&zasob);
    }
    return 0;
}
 
 
int main(int argc, char *argv[]) {
 
    if(argc!=3){
        exit(-1);
    }
 
    ilosc_czytelnikow = atoi(argv[1]);
    ilosc_pisarzy = atoi(argv[2]);
 
 
    // przydzielanie miejsca
    pthread_t *watki_czytelnicy = malloc(ilosc_czytelnikow * sizeof(pthread_t));
    pthread_t *watki_pisarze = malloc(ilosc_pisarzy * sizeof(pthread_t));
 
 
    // tworzenie watkow
    for (int i = 0; i < ilosc_czytelnikow; ++i) {
 int* pom = (int*)(malloc(sizeof(int)));
        *pom = i;
        pthread_create(&watki_czytelnicy[i], NULL, czytelnicy, (void*)pom) ;
        }
    for (int i = 0; i < ilosc_pisarzy; ++i) {
	 int* pom = (int*)(malloc(sizeof(int)));
        *pom = i;
        pthread_create(&watki_pisarze[i], NULL, pisarze, (void*)pom);
    }
 
    // join watkow
    for (int i = 0; i < ilosc_czytelnikow; ++i) {
        pthread_join(watki_czytelnicy[i], NULL);
    }
    for (int i = 0; i < ilosc_pisarzy; ++i) {
        pthread_join(watki_pisarze[i], NULL);
    }
    free(watki_czytelnicy);
    free(watki_pisarze);
 
    return 0;
}

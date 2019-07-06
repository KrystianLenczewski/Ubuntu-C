#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>



int iloscCzytelnikowCzytelnia = 0;//ilosc czytelnikow w czytelni
int iloscPisarzyCzytelnia = 0;//ilosc pisarzy w czytelni
int czytelnicywbiblotece = 0;//ilosc czytelnikow ogolem
int pisarzewbiblotece = 0;//ilosc pisarzy ogolem

sem_t zasob;//semafor z dostepem do danych
sem_t czytelnicy_sem;//semafor z dostepem dla czytelnikow
//kolejka watkow
typedef struct {
    	int ilosc_watkow_czekajacych;
	int ilosc; //ilosc ludzi w sumie
    	int* watki_czekajace; //watki czekajace [tablica]
    	int* watki_zwolnione; //watki zwolnione [tablica]
	pthread_mutex_t mutex;//struktura mutexu
    	pthread_cond_t cond;//deklaracja zmiennej warunkowej
}Kolejka;


Kolejka kolejkaBibloteczna;//kolejka priorytetowa watkow


//zwraca ID watku ktory ma najwyzszy priorytet
int najwyzszyPriorytet(Kolejka *q){
    for(int i = q->ilosc - 1; i >= 0; i--)
        if(q->watki_czekajace[i] > 0)
            return q->watki_czekajace[i];
    return -1;
}

//sprawdza czy sa watki czekajace na dostep do zasobow
int czyWatekCzeka(Kolejka *q){
    for(int i = 0; i < q->ilosc; i++)
        if(q->watki_czekajace[i] > 0)
            return q->watki_czekajace[i];
    return -1;
}

//funkcja wstrzymuje watki o najwyzszym priorytecie
void czekaj(Kolejka *q){
	int pom = najwyzszyPriorytet(q);
    pthread_mutex_lock(&(q->mutex));
    q->ilosc_watkow_czekajacych--;
    if(q->ilosc_watkow_czekajacych < 0){
        q->watki_czekajace[pom]++;
        while(q->watki_zwolnione[pom] < 0)
            pthread_cond_wait(&(q->cond), &(q->mutex));
        q->watki_zwolnione[pom]--;
    }
pthread_mutex_unlock(&(q->mutex));
}


  //funkcja wznawia watki ktore zostaly wstrzymane i maja najwyzszy  priorytet
void wznow(Kolejka *q){
    pthread_mutex_lock(&(q->mutex));//oczekiwanie w nieskonczonosc az blokada zostanie zwolniona przez inny watek
    q->ilosc_watkow_czekajacych++;
    if(q->ilosc_watkow_czekajacych <= 0 && czyWatekCzeka(q)){
        int pom = najwyzszyPriorytet(q);
        q->watki_czekajace[pom]--;
        q->watki_zwolnione[pom]++;
        pthread_cond_broadcast(&(q->cond));
        pthread_mutex_unlock(&(q->mutex));
    }
    pthread_mutex_unlock(&(q->mutex));//zwolnienie blokady przez watek
}







//funkcja watkow czytelnikow
void* czytelnik(void* val){
    int *id = (int*)val;
    while(1){
	//blokowanie dostepu
        czekaj(&kolejkaBibloteczna);
        sem_wait(&czytelnicy_sem);
        if(czytelnicywbiblotece == 0)
            sem_wait(&zasob);
        czytelnicywbiblotece++;
       fprintf(stdout,"CzytelnikID=%d wszedl\n", *id);
        fprintf(stdout,"Czytelnikow w kolejce: %d Pisarzy w kolejce: %d [w srodku: R:%d W:%d]\n\n", iloscCzytelnikowCzytelnia - czytelnicywbiblotece, iloscPisarzyCzytelnia - pisarzewbiblotece, czytelnicywbiblotece, pisarzewbiblotece);
        wznow(&kolejkaBibloteczna);
		// dostep do czytelnikow
        sem_post(&czytelnicy_sem);
       // sleep(2);
        usleep(200*1000);
        sem_wait(&czytelnicy_sem);
        czytelnicywbiblotece--;
        fprintf(stdout,"CzytelnikID=%d wyszedl\n", *id);
        fprintf(stdout,"Czytelnikow w kolejce: %d Pisarzy w kolejce: %d [w srodku: R:%d W:%d]\n\n", iloscCzytelnikowCzytelnia - czytelnicywbiblotece, iloscPisarzyCzytelnia - pisarzewbiblotece, czytelnicywbiblotece, pisarzewbiblotece);
		//jezeli ostatni czytelnik wyszedl,nalezy odblokowac dostep
        if(czytelnicywbiblotece == 0)
           sem_post(&zasob);
        sem_post(&czytelnicy_sem);
    }
}
//funkcja watkow pisarzy ,gdy pierwszy czytelnik blokuje dostep do zasobow otwiera dostep pozostalym czytelnikom
void* pisarz(void* val){
    int *id = (int *)val;
    while(1){
		// blokowanie dostepu
        czekaj(&kolejkaBibloteczna);
        sem_wait(&zasob);
        wznow(&kolejkaBibloteczna);
        pisarzewbiblotece++;
       fprintf(stdout,"PisarzID=%d wszedl\n", *id);
        fprintf(stdout,"Czytelnikow w kolece: %d Pisarzy w kolejce: %d [w srodku: R:%d W:%d]\n\n", iloscCzytelnikowCzytelnia - czytelnicywbiblotece, iloscPisarzyCzytelnia - pisarzewbiblotece, czytelnicywbiblotece, pisarzewbiblotece);
       //
 //sleep(2);
        usleep(200*1000);
        pisarzewbiblotece--;
        fprintf(stdout,"PisarzID=%d wyszedl\n", *id);
        fprintf(stdout,"Czytelnicy w kolejce: %d Poisarze w kolejce: %d [w srodku: R:%d W:%d]\n\n", iloscCzytelnikowCzytelnia - czytelnicywbiblotece, iloscPisarzyCzytelnia - pisarzewbiblotece, czytelnicywbiblotece, pisarzewbiblotece);

        sem_post(&zasob);
    }
}

int main(int argc, char* const argv[]){
	if(argc !=3 )
    {
        fprintf(stderr,"Za malo parametrow\n");
        exit(-1);
    }
	//inicjalizacja semaforów binarnych
	sem_init(&zasob,0,1);
	sem_init(&czytelnicy_sem,0,1);

    iloscCzytelnikowCzytelnia = atoi(argv[1]);
    iloscPisarzyCzytelnia = atoi(argv[2]);

   pthread_t czytelnicy[iloscCzytelnikowCzytelnia];//tablica z watkami czytelnika
   pthread_t pisarze[iloscPisarzyCzytelnia];//tablica z watkami pisarza

    kolejkaBibloteczna.ilosc_watkow_czekajacych = 1; //wartosc domyslna
    kolejkaBibloteczna.ilosc = iloscCzytelnikowCzytelnia + iloscPisarzyCzytelnia;
    kolejkaBibloteczna.watki_czekajace = malloc(kolejkaBibloteczna.ilosc * sizeof(int));
    kolejkaBibloteczna.watki_zwolnione = malloc(kolejkaBibloteczna.ilosc * sizeof(int));
    kolejkaBibloteczna.watki_czekajace[0] = kolejkaBibloteczna.ilosc;
    kolejkaBibloteczna.watki_zwolnione[0] = 0;
    for(int i = 1; i < kolejkaBibloteczna.ilosc; i++){
        kolejkaBibloteczna.watki_czekajace[i] = 0;
        kolejkaBibloteczna.watki_zwolnione[i] = 0;
    }
	pthread_mutex_init(&(kolejkaBibloteczna.mutex), NULL);

    pthread_cond_init(&(kolejkaBibloteczna.cond), NULL);   //inicjalizacja zmiennej warunkowej


    for(int i = 0; i < iloscCzytelnikowCzytelnia; i++){
        int* pom = (int*)(malloc(sizeof(int)));
        *pom = i;
        pthread_create(&czytelnicy[i], NULL, &czytelnik, (void*)pom);//utwoerzenie watku
    }

    for(int i = 0; i < iloscPisarzyCzytelnia; i++){
        int *pom = (int*)(malloc(sizeof(int)));
        *pom = i;
        pthread_create(&pisarze[i], NULL, &pisarz, (void*)pom);//utworzenie watku
    }

    pthread_join(czytelnicy[0], NULL);
    sem_destroy(&zasob);
    sem_destroy(&czytelnicy_sem);
}

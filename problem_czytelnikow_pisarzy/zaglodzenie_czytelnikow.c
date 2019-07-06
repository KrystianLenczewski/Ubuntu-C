#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include<unistd.h>

int ilosc_czytelnikow; // iloœæ wszystkich czytelników
int ilosc_pisarzy; // iloœæ wszystkich pisarzy
int ilosc_czytajacych_czyt=0; //iloœc czytaj¹cych czytelników
int ilosc_piszacych_pisarzy=0; //zmienna pomocnicza
int pisarze_w_sekcji=0; //iloœæ pisarzy w sekcji krytycznej


sem_t czytelnicy_semafor;
sem_t pom_semafor;
sem_t pisarze_semafor;
sem_t zasob;


void * Czytelnik(void * id_watku)
{

    int *id = (int *)id_watku;

    while(1)
    {
        //smafor początkowy. jeśli pisarz zablokuje go, czytelnicy nie będą mogli wejsć.

        sem_wait(&pom_semafor);

        //Czytelnicy muszą wchodzić pojedyńczo, semafor ten także chroni przed wyścigiem spowodowanym
        //zmianą zmiennej "ilosc_czytajacych_czyt" przez 2 watki na raz.

        sem_wait(&czytelnicy_semafor);

        // W srodku jest czytelni wiec jest ich o 1 wiecej
        ilosc_czytajacych_czyt++;

        printf("Czytelnik o id: %d wszedl \n",*id);
        printf("Ilosc czytelnikow: %d , Ilosc pisarzy: %d [in: R:%d W:%d]\n",ilosc_czytelnikow-ilosc_czytajacych_czyt,ilosc_pisarzy-pisarze_w_sekcji,ilosc_czytajacych_czyt,pisarze_w_sekcji);

        //wszedl pierwszy czytelnik. zablowkowano zasob, aby pisarze nie wchodzili
        if(ilosc_czytajacych_czyt==1)
            sem_wait(&zasob);

        //zmienna zostala zmodyfikowana, mozna wposcic innych czytelnikow
        sem_post(&czytelnicy_semafor);


        //zwalnia wstepona blokade zasobu,jeśli w tym momencie pisarze zablokują "tryResource", żaden czytelnik już nie
        //wejdzie. nastąpi zagłodzenie
        sem_post(&pom_semafor);

        sleep(1);

        sem_wait(&czytelnicy_semafor);
        ilosc_czytajacych_czyt--;

        //Jesli w czytelni nie ma juz czytelnikow zasob jest odblkowany. Pisarz moze wejsc
        if(ilosc_czytajacych_czyt==0)
            sem_post(&zasob);

        sem_post(&czytelnicy_semafor);
    }

}

void * Pisarz(void * id_watku)
{


    int *id = (int *)id_watku;
    while(1)
    {

        //Tylko jeden pisarz  moze wejsc wiec blokuje semafor

        sem_wait(&pisarze_semafor);

        ilosc_piszacych_pisarzy++;
        if(ilosc_piszacych_pisarzy == 1)
            sem_wait(&pom_semafor);//Blokuje dostep innym czytelnikom do biblioteki

        sem_post(&pisarze_semafor);


        sem_wait(&zasob);
        pisarze_w_sekcji++;
        //poczatek sekcji krytycznej
        printf("Pisarz o id = %d wszedl \n",*id);
        printf("Ilosc czytelnikow: %d , Ilosc pisarzy: %d [in: R:%d W:%d]\n",ilosc_czytelnikow-ilosc_czytajacych_czyt,ilosc_pisarzy-pisarze_w_sekcji,ilosc_czytajacych_czyt,pisarze_w_sekcji);
       sleep(1);
        pisarze_w_sekcji--;
        //koniec sekcji krytycznej
        sem_post(&zasob);

        sem_wait(&pisarze_semafor);
        ilosc_piszacych_pisarzy--;
        if(ilosc_piszacych_pisarzy==0)
            sem_post(&pom_semafor);

        //Jesli w kolejce do zasobu czeka pisarz od razu daje mu dostep

        sem_post(&pisarze_semafor);
    }
}



int main(int argc, char ** argv)
{

    ilosc_czytelnikow = atoi(argv[1]);
    ilosc_pisarzy = atoi(argv[2]);

    //tablice do przechowywania watkow
    pthread_t readers[ilosc_czytelnikow];
    pthread_t writers[ilosc_pisarzy];

    //inicjalizacja semaforów binarnych

    sem_init(&pom_semafor, 0, 1);
    sem_init(&zasob, 0, 1);
    sem_init(&czytelnicy_semafor, 0, 1);
    sem_init(&pisarze_semafor, 0, 1);

    //tworzenie watkow czytelnikow
    for(int i=0; i< ilosc_czytelnikow; i++)
    {
        int* id = (int*)(malloc(sizeof(int)));
        *id = i;
        pthread_create(&readers[i], NULL, &Czytelnik, (void*)id);
    }

    //tworzenie pisarzy
    for(int i=0; i< ilosc_pisarzy; i++)
    {
        int *id = (int*)(malloc(sizeof(int)));
        *id = i;
        pthread_create(&writers[i], NULL, &Pisarz, (void*)id);
    }

    //Czeka na wykonanie 1 w dodatkowego watku aby watek main nie doszedl do return 0
    pthread_join(readers[0], NULL);
    sem_destroy(&pom_semafor);
    sem_destroy(&zasob);
    sem_destroy(&czytelnicy_semafor);
    sem_destroy(&pisarze_semafor);




    return 0;

}

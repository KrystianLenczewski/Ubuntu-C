#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/mman.h>
#include <signal.h>


typedef struct
{
    char file_name[1024];
    long int dateModification;
    int size;
} details;

//flags
int changeTime=0;
int signal_user=0;
int rec_flag=0;//czy rekurencja bedzie stosowana
int copy_range=0;//przedzial czy duzy czy maly

void myhandler(int signum)
{

    if(signum==SIGUSR1)
    {
        printf("received SIGUSR1\n");
    }
}

int copy(char *source, char *dest)
{
    int in, out, nbytes;
    char buffer[1024];
    if((in=open(source, O_RDONLY))<0)
    {
        return -1; //blad
    }
    if((out=open(dest,O_CREAT | O_WRONLY | O_TRUNC,0777))<0)
    {
        return -2; //blad
    }
    while(nbytes=read(in,buffer,1024))
    {
        if(nbytes<0)
        {
            close(in);
            close(out);
            return 1;
        }
        if(write(out,buffer,nbytes)<0)
        {
            close(in);
            close(out);
            return 1;
        }
    }
    close(in);
    close(out);
    return 0;
}

//copy function using mapping

int copy_map(char *source, char *dest,int size)
{
    int in, out;
    char *pom;

    //try to open source file

    if ((in = open(source, O_RDONLY)) < 0)
    {
        syslog(LOG_ERR, "Failed to open file: %s\n", source);
        return 0;
    }

    //try to open target file

    if ((out = open(dest, O_CREAT | O_WRONLY | O_TRUNC, 0777)) < 0)
    {
        syslog(LOG_ERR, "Failed to open file: %s\n", dest);
        return 0;
    }

    //try to copy

    if ((pom = mmap(0, size, PROT_READ, MAP_SHARED, in, 0)) < 0)
    {
        syslog(LOG_ERR, "Failed to read from file: %s\n", source);
        close(in);
        close(out);
        return 0;
    }
    if (write(out, pom, size) < 0)
    {
        syslog(LOG_ERR, "Failed to write to file: %s\n", dest);
        close(in);
        close(out);
        return 0;
    }
    munmap(pom, size);
    close(in);
    close(out);
    return 1;
}

int remove_file(details* source, details* destiny,char* path,int licznik_source,int licznik_destiny)
{

    int i,j,pom=0;
    char pompath[1025];
    for(i=0; i<licznik_destiny; i++)
    {
        for(j=0; j<licznik_source; j++)
        {
            if(strcmp(source[j].file_name,destiny[i].file_name)==0)
            {
                pom=1;
            }

        }
        if(pom==0)
        {
            snprintf(pompath, sizeof pompath, "%s/%s", path, destiny[i].file_name);
            unlink(pompath);

        }
        pom=0;
    }
    return 1;

}

int remove_directory(const char *path)
{
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;
    if (d)
    {
        struct dirent *p;
        r = 0;

        while (!r && (p=readdir(d)))
        {
            int r2 = -1;
            char *buf;
            size_t len;

            /* Skip the names "." and ".." as we don't want to recurse on them. */
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
            {
                continue;
            }

            len = path_len + strlen(p->d_name) + 2;
            buf = malloc(len);

            if (buf)
            {
                struct stat statbuf;

                snprintf(buf, len, "%s/%s", path, p->d_name);

                if (!stat(buf, &statbuf))
                {
                    if (S_ISDIR(statbuf.st_mode))
                    {
                        r2 = remove_directory(buf);
                    }
                    else
                    {
                        r2 = unlink(buf);
                    }
                }

                free(buf);
            }

            r = r2;
        }

        closedir(d);
    }

    if (!r)
    {
        r = rmdir(path);
    }

    return r;
}


int dir_search(char* path,char* name)  //jesli return=1 to znalazl, jesli 0 to nie znalazl
{
    struct dirent *file;
    DIR *dirr=opendir(path);
    while((file=readdir(dirr))!=NULL)
    {
        if(file->d_type==DT_DIR && strcmp(file->d_name,".")!=0 && strcmp(file->d_name,"..")!=0 && strcmp(file->d_name,name)==0)
        {
            closedir(dirr);
            return 1;
        }
    }
    closedir(dirr);
    return 0;
}

void directory_sync(char* path_1,char* path_2)
{
    struct dirent *file;
    DIR *source=opendir(path_1);
    DIR *dest=opendir(path_2);
    details source_files[100];
    details destiny_files[100];
    int source_counter=0;
    int destiny_counter=0;
    char folder_path_s[1025];
    char folder_path_d[1025];

    while((file=readdir(dest))!=NULL)
    {
        if(file->d_type==DT_DIR && strcmp(file->d_name,".")!=0 && strcmp(file->d_name,"..")!=0)
        {
            snprintf(folder_path_s, sizeof folder_path_s,"%s%s/",path_1,file->d_name);
            snprintf(folder_path_d, sizeof folder_path_d,"%s%s/",path_2,file->d_name);
            if(dir_search(path_1,file->d_name)==1)
            {
                fprintf(stderr,"REK!!!!!!!%s\n",folder_path_d);
                directory_sync(folder_path_s,folder_path_d);
            }
            if(dir_search(path_1,file->d_name)==0)
            {
                fprintf(stderr,"REMOVE!!!%s\n",folder_path_d);
                remove_directory(folder_path_d);
            }
        }
    }
    closedir(source);
    closedir(dest);
}




void ssync(char* path_1,char* path_2)
{
    DIR *source=opendir(path_1);
    DIR *dest=opendir(path_2);
    struct dirent *file; //struktury do obslugi systemu plikow
    struct stat file_details;
    details source_files[100];
    details destiny_files[100];
    int source_counter=0;
    int destiny_counter=0;
    char path_file[1025];
    char path_file2[1025];
    int i,j;
    int dodac=0;
    int pom=0;
    char folder_path_s[1025];
    char folder_path_d[1025];
    while((file=readdir(source))!=NULL)
    {
        if(rec_flag==1)
        {
            if(file->d_type==DT_DIR && strcmp(file->d_name,".")!=0 && strcmp(file->d_name,"..")!=0)
            {
                snprintf(folder_path_s, sizeof folder_path_s,"%s%s/",path_1,file->d_name);
                snprintf(folder_path_d, sizeof folder_path_d,"%s%s/",path_2,file->d_name);
                pom=mkdir(folder_path_d,0777);
                ssync(folder_path_s,folder_path_d);
            }
        }
        if(file->d_type!=DT_REG)
            continue; //sprawdzenie czy plik reguralny
        snprintf(path_file, sizeof path_file, "%s/%s", path_file, path_1); //tworzenie sciezki do pliku
        snprintf(path_file, sizeof path_file, "%s/%s", path_1, file->d_name);
        if (!stat(path_file, &file_details))  //dodawanie plików ze zródla do tablicy
        {
            snprintf(source_files[source_counter].file_name,sizeof source_files[source_counter].file_name,"%s",file->d_name);
            source_files[source_counter].size=file_details.st_size;
            source_files[source_counter].dateModification=file_details.st_mtime;
            source_counter++;
        }
        else  //jesli stat nie dziala
        {
            fprintf(stderr, "Cannot stat %s; %s\n", path_file, strerror(errno));
        }
    }
    //dla folderu docelowego
    while((file=readdir(dest))!=NULL)
    {
        if(file->d_type!=DT_REG)
            continue;
        snprintf(path_file, sizeof path_file, "%s/%s",path_file,path_2);
        snprintf(path_file, sizeof path_file, "%s/%s", path_2, file->d_name);
        if(!stat(path_file,&file_details))
        {
            snprintf(destiny_files[destiny_counter].file_name,sizeof destiny_files[destiny_counter].file_name,"%s",file->d_name);
            destiny_files[destiny_counter].dateModification=file_details.st_mtime;
            destiny_files[destiny_counter].size=file_details.st_size;
            destiny_counter++;
        }

    }
    pom=remove_file(source_files,destiny_files,path_2,source_counter,destiny_counter);
    //szukanie roznic miedzy folderami
    for(i=0; i<source_counter; i++)
    {
        for(j=0; j<destiny_counter; j++)
        {
            if((strcmp(source_files[i].file_name,destiny_files[j].file_name)==0))
            {
                dodac=2;
            }
            if((strcmp(source_files[i].file_name,destiny_files[j].file_name)==0)&&source_files[i].dateModification>destiny_files[j].dateModification)
            {
                dodac=1;
            }

        }
        if(dodac==1 || dodac==0)  //tu kopiowanie
        {
            syslog (LOG_NOTICE, "Skopiowano plik %s",source_files[i].file_name);
            fprintf(stderr,"Name: %s, Size: %d \n",source_files[i].file_name,source_files[i].size);
            snprintf(path_file, sizeof path_file, "%s/%s", path_1, source_files[i].file_name);
            snprintf(path_file2, sizeof path_file2, "%s/%s", path_2, source_files[i].file_name);

            if(copy_range==0)
            {
                syslog (LOG_NOTICE, "Przed kopiowaniem read/write");
                pom=copy(path_file,path_file2);
                syslog (LOG_NOTICE, "Kopiowanie przez read/write");


            }
            else if(source_files[i].size>=copy_range)
            {
                syslog (LOG_NOTICE, "Przed kopiowaniem przez mapowanie");
                pom=copy_map(path_file,path_file2,source_files[i].size);
                syslog (LOG_NOTICE, "Kopiowanie przez mapowanie");
            }
            else if(source_files[i].size<copy_range)
            {
                syslog (LOG_NOTICE, "Przed kopiowaniem read/write");
                pom=copy(path_file,path_file2);
                syslog (LOG_NOTICE, "Kopiowanie przez read/write");
            }

        }
        dodac=0;


    }

    closedir(source);
    closedir(dest);



}


int main(int argc, char *argv[])
{


    setlogmask (LOG_UPTO (LOG_NOTICE));//ustawienie maski logu
    syslog (LOG_NOTICE, "Program started by User %d", getuid ());
    char path_1[1024], path_2[1024]; //argv[1]/argv[2]
    strcpy(path_1,argv[1]);
    strcpy(path_2,argv[2]);
    if(argc==1 || argc==2)
    {
        printf("Musisz podac 2 argumenty(sciezki do folderow\n");
        syslog (LOG_NOTICE, "Brak 2 argumentów,wylaczenie programu");
        return 1;
    }
    DIR *source=opendir(argv[1]);
    DIR *dest=opendir(argv[2]);
    if(source==NULL || dest==NULL)  //czy sciezki sa folderem
    {
        printf("Podane sciezki sa nieprawidlowe\n");
        syslog (LOG_NOTICE, "Nieprawidlowa siezka, wylaczenie programu ");

        return 2;
    }
    closedir(source);
    closedir(dest);
    syslog (LOG_NOTICE, "Program stal sie demonem");
    //getopt obsluga argumentow
    int c;
    struct sigaction my_action,old_action;
    while((c=getopt(argc,argv,"A:RT:"))!=-1)
    {
        switch(c)
        {
        case 'A':
            copy_range=atoi(optarg);
            break;
        case 'R':
            rec_flag=1;
            break;
        case 'T':
            changeTime=atoi(optarg);
            break;
        case '?':
            if (optopt == 'c')
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
            return 1;
        default:
            abort();
        }
    }

    /* Our process ID and Session ID */
    pid_t pid, sid;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0)
    {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(0);

    /* Open any logs here */

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0)
    {
        /* Log the failure */
        exit(EXIT_FAILURE);
    }



    /* Change the current working directory */
    if ((chdir("/")) < 0)
    {
        /* Log the failure */
        exit(EXIT_FAILURE);
    }


    signal(SIGUSR1,myhandler);
    while (1)
    {
        ssync(path_1,path_2);
        if(rec_flag==1)//jezeli wlaczono rekurencyjna sychronizacje
        {
            directory_sync(path_1,path_2);
        }
        if(changeTime==0)
        {
            sleep(30);
        }
        else
        {
            sleep(changeTime);
        }

        syslog (LOG_NOTICE, "Demon wchodzi w stan uspienia");

        while(signal_user==1)
        {
            ssync(path_1,path_2);
            if(rec_flag==1)//jezeli wlaczono rekurencyjna sychronizacje
            {
                directory_sync(path_1,path_2);
            }
            switch(signal_user)
            {
            case 0:
                break;
            case 1:
            {
                fprintf(stderr,"Sygnal SIGUSR1");
                signal_user=0;
                break;
            }
            }


        }

    }

    exit(EXIT_SUCCESS);
}

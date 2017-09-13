#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define NAME   "/tmp/server.log"
#define TAM  (4)
#define FILESIZE (TAM * sizeof(struct logs))

struct logs
{
    int tipo; //estatico ou comprimido
    char fichHtml [1000];
    time_t inicio;
    time_t fim;
};
void print_med( const struct logs *med);
int main(int argc, char *argv[])
{
    int i;
    int fd;
    int result;
    struct logs *map;  /* mmapped array of char */

    fd = open(NAME, O_RDWR|O_CREAT|O_TRUNC,(mode_t)0600);
    if (fd == -1) {
        perror("Error opening file for reading");
        exit(EXIT_FAILURE);
    }

    result=lseek(fd,FILESIZE-1,SEEK_SET);

     if (result == -1)
    {
        close(fd);
        perror("Error calling lseek() to 'stretch' the file");
        exit(EXIT_FAILURE);
    }

    result = write(fd, "", 1);
    if (result != 1)
    {
        close(fd);
        perror("Error writing last byte of the file");
        exit(EXIT_FAILURE);
    }

    /* Now the file is ready to be mmapped.  */
    map = (struct logs *)mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED)
    {
        close(fd);
        perror("Error mmapping the file");
        exit(EXIT_FAILURE);
    }

    
    for (i = 0; i <TAM; ++i) {
        struct logs m;
       
        memset(&m, '\0', sizeof(m));

        /*if (get_new_key(map, i, &m.key) == EOF)
            break;*/

        m.tipo=1;
        strcpy(m.fichHtml, "blablabla");
        m.inicio=1;
        m.fim=0;

        map[i] = m;
    }

    for(i=0;i<TAM;i++){
        print_med(&map[i]);
       // printf("%d, %s %d,%d\n", map[i].tipo,map[i]->fichHtml, map[i].inicio, map[i].fim);
    }

   
    if (munmap(map, FILESIZE) == -1) {
        perror("Error un-mmapping the file");
    }
    close(fd);
    return 0;
}

void print_med( const struct logs *med)
{
    printf("%d, %s %d,%d\n", med->tipo,med->fichHtml, (int)med->inicio, (int)med->fim);
}

/*static int med_in_map(const struct med *map, int num_meds, int key)
{
    for (int i = 0; i < num_meds; ++i)
    {
        if (key == map[i].key)
        {
            printf("Med %d already exists.\n", key);
            return 1;
        }
    }
    return 0;
}

static int get_new_key(const struct med *map, int num_meds, int *key)
{
    while (printf("Key of med: ") > 0 && scanf("%d", key) == 1)
    {
        if (med_in_map(map, num_meds, *key) == 0)
            return 0;
    }
    return EOF;
}*/

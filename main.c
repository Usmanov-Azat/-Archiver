#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>


/*
Структура архива (информация и ее размер)
Для каталога:
Признак того, что это каталог - int
Название каталога - char[255]
Количество элементов в каталоге - int

Структура файла:
Признак того, что не каталог - int
Название файла - char[255]
Реальный размер файла - int
Размер файла в условных единицах - int
Сам файл

На вход архиватора должен подаваться именно каталог
*/

int WriteToFile(int in, int out, char* filepath, struct stat statbuf)
{
    char buf;
    char longbuf[255];
    int i = 0;
    int filesize = 0;
    int readsize = 0;
    int iscat = 0;

    //записываем, что это не каталог
    write(out, &iscat, sizeof(int));
    //копируем название файла в длинный буфер
    strcpy(longbuf, filepath);
    printf("%s\n", longbuf);
    //записываем название файла в архив
    write(out, longbuf, 255);

    //получаем информацию о файле
    filesize = statbuf.st_size; //получаем размер


    printf("Size of file %d bytes\n", filesize);

    //записываем размер в архив
    write(out, &filesize, sizeof(int));
    write(out, &readsize, sizeof(int)); //пустышка, сюда будет записан размер в "буферах"
    //записываем файл в архив

    while(read(in, &buf, 1) == 1)
    {
        write(out, &buf, 1);
        //поскольку у нас одна единица буфера, к сожалению, не равна
        //одному байту, надо считать размер файла в "буферах" и записать
        //перед файлом
        readsize++;
    }

    //смещаемся в место, которое после реального размера
    int a = lseek(out, -filesize-sizeof(int), SEEK_CUR); //в данном случае у нас указатель именно по объему файла,
    //записываем сколько надо считать                     //поэтому используем РЕАЛЬНЫЙ вес файла, а НЕ СЧИТАНЫЫЙ
    write(out, &readsize, sizeof(int));
    //возвращаемся в конец
    lseek(out, filesize, SEEK_CUR);

    return 0;
}

int arc(int out, char* dir)
{
    DIR *dp;

    int i;
    int in;
    struct dirent *entry;
    char namebuf[255];
    int isdir = 1;
    int numoffiles = 0;
    int numfilepos;
    struct stat statbuf;

    int check;


    //открываем поток каталога
    if ((dp = opendir(dir)) == NULL)
    {
        fprintf(stderr, "cannot open directory: %s\n", dir);
        return 1;
    }
    strcpy(namebuf, dir);
    //printf("%s\n", &namebuf);
    //записываем, что это каталог
    check = write(out, &isdir, sizeof(int));
    //printf("Bytes written: %d\n", check);
    //записываем название каталога в архив
    check = write(out, namebuf, 255);
     //printf("Bytes written: %d\n", check);
    //пустышка для кол-ва файлов
    numfilepos = lseek(out, 0, SEEK_CUR);
    //printf("Current position: %d\n", numfilepos);
    write(out, &isdir, sizeof(int));
    //меняем текущий каталог
    chdir(dir);
    while((entry = readdir(dp)) != NULL)
    {
        stat(entry->d_name, &statbuf);
        //провеврка на каталог
        if(S_ISDIR(statbuf.st_mode))
        {
            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
                continue;
            printf("%s%s/\n", "", entry->d_name);
            /* Рекурсивный вызов с новый отступом */
            arc(out ,entry->d_name);
            numoffiles++;
        }
        else
        {
            printf("%s\n", entry->d_name);
            //открываем данный файл
            in = open(entry->d_name, O_RDONLY);
            stat(entry->d_name, &statbuf);
            WriteToFile(in, out, entry->d_name, statbuf);
            close(in);
            numoffiles++;
        }
    }
    printf("Num of files: %d\n",  numoffiles);
    lseek(out, numfilepos,SEEK_SET);

    write(out, &numoffiles, sizeof(int));
    lseek(out, 0, SEEK_END);
    chdir("..");
    closedir(dp);

    return 0;
}
int StartArc()
{
    char* filepath;
    scanf("%s", &filepath);
    //создаем архив
    int out = open("arcive.arc", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
    arc(out, &filepath);
    close(out);
}

int ReadFromFile(int in)
{
    char buf;
    //char longbuf[255];
    int out;
    int namesize;
    int filesize = 0;
    int readsize = 0;

    char Outfile[255];
    struct stat statbuf;

    //читаем название файла
    read(in, &Outfile, 255);
    //читаем размер файла
    read(in, &filesize, sizeof(int));
    //читаем размер для чтения
    read(in, &readsize, sizeof(int));

    //создаем "разархивированный" файл
    out = open(Outfile, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);

    printf("Name of file %s\n", &Outfile);
    printf("Size of file %d bytes\n", filesize);



    for(int i = 0; i < readsize; i++)
    {
        read(in, &buf, 1);
        write(out, &buf, 1);
    }

    return 0;
}

int ReadCat(int in)
{
    char buf;
    //char longbuf[255];
    int out;
    int namesize;
    int filesize = 0;
    int readsize = 0;
    int numoffiles;
    int IsCat = 0;

    int check;

    char outfile[255];
    struct stat statbuf;
    //читаем информацию о том, каталог это или нет
    check = read(in, &IsCat, sizeof(int));
    printf("Bytes read: %d\n", check);
    printf("Is cat: %d\n", IsCat);
    //если каталог
    if(IsCat != 0)
    {
        //считываем название, кол-во файлов
        check = read(in, outfile, 255);
        printf("Bytes read: %d\n", check);
        check = read(in, &numoffiles, sizeof(int));
        printf("Bytes read: %d\n", check);
        printf("Num of files: %d\n",  numoffiles);
        printf("%s\n", &outfile);
        //создаем директорию с нужным названием
        //и заходим в нее
        mkdir(outfile, 
        S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|
        S_IWGRP|S_IROTH|S_IWOTH|S_IXOTH);
        chdir(outfile);
        //читаем полученные файлы
        for(int i = 0; i < numoffiles; i++)
            ReadCat(in);
        //возвращаемся на уровень назад
        chdir("..");
        
    }
    //если это файл - читаем и записываем его
    else
        ReadFromFile(in);
    

}

int dearc(const char* filepath)
{

    //char longbuf[255];
    int in, out;
    int namesize;
    int filesize = 0;


    //открываем архив
    in = open("arcive.arc", O_RDONLY);
    ReadCat(in);
    close(in);

    return 0;
}


int main()
{
    int ch;
    printf("Select action:\n");
    printf("1 - files to archive\n");
    printf("2 - read archive\n");
    scanf("%d", &ch);
    if(ch == 1)
        StartArc();
    else if (ch == 2)
        dearc("arcive.arc");

    return 0;
}

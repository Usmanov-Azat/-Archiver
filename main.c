#include <unistd.h>
#include <stdio.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

int WriteToFile(int in, int out, char* filepath, struct stat statbuf);
int archivate(int out, char* dir);
int StartArchivate();
int ReadFromFile(int in);
int ReadCatalog(int in);
int dearchivate(const char* filepath);

const int FILENAME_SIZE = 255;


int main()
{
    int change;
    printf("Select action:\n");
    printf("1 - files to archive\n");
    printf("2 - read archive\n");
    scanf("%d", &change);
    if (change == 1)
        StartArchivate();
    else if (change == 2)
        dearchivate("arcive.arc");

    return 0;
}

int WriteToFile(int in, int out, char* filepath, struct stat statbuf)
{
    char buffer;
    char long_buffer[FILENAME_SIZE];
    int i = 0;
    int readsize = 0;
    int filesize = 0;
    int is_catalog = 0;

    //записываем, что это не каталог
    write(out, &is_catalog, sizeof(int));    //(дескриптор в кот зап данные, записываемые данные, количество данных)
    //копируем название файла в длинный буфер
    strcpy(long_buffer, filepath);
    printf("%s\n", long_buffer);
    //записываем название файла в архив
    write(out, long_buffer, 255);

    //получаем информацию о файле
    filesize = statbuf.st_size; //получаем размер


    printf("Size of file %d bytes\n", filesize);

    //записываем размер в архив
    write(out, &filesize, sizeof(int));
    write(out, &readsize, sizeof(int)); //пустышка, сюда будет записан размер в "буферах"
    //записываем файл в архив

    while (read(in, &buffer, 1) == 1)
    {
        write(out, &buffer, 1);
        //поскольку у нас одна единица буфера, к сожалению, не равна
        //одному байту, надо считать размер файла в "буферах" и записать
        //перед файлом
        readsize++;
    }

    //смещаемся в место, которое после реального размера
    int a = lseek(out, -filesize - sizeof(int), SEEK_CUR); //в данном случае у нас указатель именно по объему файла,
    //записываем сколько надо считать                     //поэтому используем РЕАЛЬНЫЙ вес файла, а НЕ СЧИТАНЫЫЙ
    write(out, &readsize, sizeof(int));
    //возвращаемся в конец
    lseek(out, filesize, SEEK_CUR);

    return 0;
}

int archivate(int out, char* dir)
{
    DIR* dp;//открывает и возвращает поток каталога для чтения каталога
    struct dirent* entry;//создаем структуру типа вшкуте с названием entry
    struct stat statbuf;
    int i;
    int in;
    char name_buffer[255];//имя файла
    int is_directory = 1;
    int numoffiles = 0;
    int numfilepos;  
    int check;


    //открываем поток каталога
    if ((dp = opendir(dir)) == NULL)
    {
        fprintf(stderr, "cannot open directory: %s\n", dir);
        return 1;
    }
    strcpy(name_buffer, dir);//dir копируем namebuf
    //записываем, что это каталог
    check = write(out, &is_directory, sizeof(int));//!!!!!!!
    //записываем название каталога в архив
    check = write(out, name_buffer, 255);
   //пустое значение для кол-ва файлов
    numfilepos = lseek(out, 0, SEEK_CUR);//первый параметр куда смещаемся, второй на сколько, третий от куда, SEEK_CUR - текущее местоположение.В этой функции отображаем где бы находимся так как смещаемся на ноль.
    write(out, &is_directory, sizeof(int));
    //меняем текущий каталог
    chdir(dir); //chdir изменяет текущий каталог каталог на dir.
    while ((entry = readdir(dp)) != NULL) // при каждом вызове в ентри записывает
    {
        stat(entry->d_name, &statbuf);//вытащили из ентри название файла в статбав
        //провеврка на каталог
        if (S_ISDIR(statbuf.st_mode))
        {
            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)//стрсмп сравнивает две строки,  проверкак на еще один каталог
                continue;
            printf("%s%s/\n", "", entry->d_name);//выводим имя
            // Рекурсивный вызов с новый отступом 
            archivate(out, entry->d_name);
            numoffiles++;//увеличиваем кол-во файлов
        }
        else
        {
            printf("%s\n", entry->d_name);
            //открываем данный файл
            in = open(entry->d_name, O_RDONLY);
            if (in == -1)
            {
                printf("Ошибка при открытии файла!");
                return 1;
            }
            stat(entry->d_name, &statbuf);
            WriteToFile(in, out, entry->d_name, statbuf);
            close(in);
            numoffiles++;
        }
    }
    printf("Number of files: %d\n", numoffiles);
    lseek(out, numfilepos, SEEK_SET);

    write(out, &numoffiles, sizeof(int));
    lseek(out, 0, SEEK_END);
    chdir("..");
    closedir(dp);

    return 0;
}
int StartArchivate()
{
    char file_path[FILENAME_SIZE];//путь до файла
    scanf("%s", file_path);//ситывание
    //создаем архив
    int out = open("arcive.arc", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);//O_WRONLY - файл для записи, если файла нет то он будет создан, S_IRUSR - право на чтение, S_IWUSR - право на запись в файл
    if (out == -1)
    {
        printf("Ошибка записи!");
        return 1;
    }
    archivate(out, file_path);
    close(out);
}

int ReadFromFile(int in)
{
    char buffer;
    int out;
    int file_size = 0;
    int read_size = 0;

    char Outfile[FILENAME_SIZE];
    struct stat statbuf;

    //читаем название файла
    read(in, &Outfile, FILENAME_SIZE);//(файл с кот читаем, место хранения, скорлько байт считываем) 0 - считывать нечего, -1 - ошибка при чтении
    //читаем размер файла
    read(in, &file_size, sizeof(int));
    //читаем размер для чтения
    read(in, &read_size, sizeof(int));

    //создаем "разархивированный" файл
    out = open(Outfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (out == -1)
    {
        printf("Ошибка записи!");
        return 1;
    }

    printf("Name of file %s\n", Outfile);
    printf("Size of file %d bytes\n", file_size);



    for (int i = 0; i < read_size; i++)
    {
        read(in, &buffer, 1);
        write(out, &buffer, 1);
    }

    return 0;
}

int ReadCatalog(int in)
{
    int namesize;
    int numoffiles;
    int IsCatalog = 0;

    int check;

    char outfile[FILENAME_SIZE];
    struct stat statbuf;
    //читаем информацию о том, каталог это или нет
    check = read(in, &IsCatalog, sizeof(int));
    printf("Bytes read: %d\n", check);
    printf("Is catalogue: %d\n", IsCatalog);
    //если каталог
    if (IsCatalog != 0)
    {
        //считываем название, кол-во файлов
        check = read(in, outfile, FILENAME_SIZE);
        printf("Bytes read: %d\n", check);
        check = read(in, &numoffiles, sizeof(int));
        printf("Bytes read: %d\n", check);
        printf("Number of files: %d\n", numoffiles);
        printf("%s\n", outfile);
        //создаем директорию с нужным названием
        //и заходим в нее
        mkdir(outfile,
            S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP |
            S_IWGRP | S_IROTH | S_IWOTH | S_IXOTH);
        chdir(outfile);
        //читаем полученные файлы
        for (int i = 0; i < numoffiles; i++)
            ReadCatalog(in);
        //возвращаемся на уровень назад
        chdir("..");

    }
    //если это файл - читаем и записываем его
    else
        ReadFromFile(in);


}

int dearchivate(const char* filepath)
{
    int out;
    int in;

    //открываем архив
    in = open("arcive.arc", O_RDONLY);
    if (out == -1)
    {
        printf("Ошибка открытия архива!");
        return 1;
    }
    ReadCatalog(in);
    close(in);

    return 0;
}

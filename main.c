#include <stdio.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
/*
 * Разработать программу на языке С, которая осуществляет следующие действия

Создает область памяти размером A мегабайт, начинающихся с адреса B (если возможно) при помощи C=(malloc, mmap) заполненную случайными числами /dev/urandom в D потоков. Используя системные средства мониторинга определите адрес начала в адресном пространстве процесса и характеристики выделенных участков памяти. Замеры виртуальной/физической памяти необходимо снять:

До аллокации
После аллокации
После заполнения участка данными
После деаллокации
Записывает область памяти в файлы одинакового размера E мегабайт с использованием F=(блочного, некешируемого) обращения к диску. Размер блока ввода-вывода G байт. Преподаватель выдает в качестве задания последовательность записи/чтения блоков H=(последовательный, заданный  или случайный)

Генерацию данных и запись осуществлять в бесконечном цикле.

В отдельных I потоках осуществлять чтение данных из файлов и подсчитывать агрегированные характеристики данных - J=(сумму, среднее значение, максимальное, минимальное значение).

Чтение и запись данных в/из файла должна быть защищена примитивами синхронизации K=(futex, cv, sem, flock).

По заданию преподавателя изменить приоритеты потоков и описать изменения в характеристиках программы.

Для запуска программы возможно использовать операционную систему Windows 10 или  Debian/Ubuntu в виртуальном окружении.

Измерить значения затраченного процессорного времени на выполнение программы и на операции ввода-вывода используя системные утилиты.

Отследить трассу системных вызовов.

Используя stap построить графики системных характеристик.
 */
//vancA=197;B=0xA35E1090;C=mmap;D=78;E=72;F=block;G=10;H=random;I=35;J=min;K=sem
//mitin A=128;B=0x74594352;C=malloc;D=55;E=144;F=block;G=65;H=seq;I=48;J=avg;K=cv

#define A 197
#define D 78
#define E 72
#define G 10
#define I 35


const int filenumbers = 3;

pthread_mutex_t mut1;
pthread_mutex_t mut2;
pthread_mutex_t mut3;


int loop = 1;

typedef struct {
    char* address;
    size_t size;
    FILE* file;
}thread_info;


typedef struct {
    char* address;
    int filenumber;
}writer_thread_info;

typedef struct {
    int number_thread;
}reader_thread_info;


void* write_memory(void* data){

    thread_info* cur_data = (thread_info*) data;
    fread(cur_data->address, 1, cur_data->size, cur_data->file);

    pthread_exit(0);
}

void fill_space(char* address){

    char* address_offset = address;

    thread_info informations[D];
    pthread_t threads[D];

    size_t size_for_thread = A * 1024 * 1024 / D;
    FILE* filerandom = fopen("/dev/urandom", "rb");

    for (int i = 0; i < D; i ++){
        informations[i].address = address_offset;
        informations[i].size = size_for_thread;
        informations[i].file = filerandom;
        address_offset += size_for_thread;
    }

    informations[D - 1].size += A * 1024 * 1024 % D;


    for (int i = 0; i < D; i ++)
        pthread_create(&threads[i], NULL, write_memory, &informations[i]);


    for (int i = 0; i < D; i ++)
        pthread_join(threads[i], NULL);

    fclose(filerandom);

}

void* generate_info(void* data){
    char* cur_data = (char*) data;
    while(loop == 1){
        fill_space(cur_data);
    }

    pthread_exit(0);
}

pthread_mutex_t get_mut(int id){
    if (id == 0) return mut1;
    if (id == 1) return mut2;
    if (id == 2) return mut3;
}


void write_file(writer_thread_info* data, int idFile){//todo можно не передавать айди, а брать из data->

    pthread_mutex_t cur_mut = get_mut(idFile);
    pthread_mutex_lock(&cur_mut);



    char* defaultname = malloc(sizeof(char) * 6);
    snprintf(defaultname, sizeof(defaultname) + 1, "labOS%d",++idFile);
    FILE* file = fopen(defaultname, "wb");

    size_t file_size = E * 1024 * 1024;
    size_t rest = 0;
    while (rest < file_size) {
        long size = file_size - rest >= G ? G : file_size - rest;
        rest += fwrite(data->address + rest, 1, size, file);
    }
    fclose(file);
    pthread_mutex_unlock(&cur_mut);

}

void* write_files(void* data){
    writer_thread_info* cur_data = (writer_thread_info*) data;
    while (loop == 1){
        write_file(cur_data, cur_data->filenumber);

    }
}


void read_file(int id_thread, int idFile){
    pthread_mutex_t cur_mut = get_mut(idFile);
    pthread_mutex_lock(&cur_mut);


    char* defaultname = malloc(sizeof(char) * 6);
    snprintf(defaultname, sizeof(defaultname) + 1, "labOS%d",++idFile);
    FILE* file = fopen(defaultname, "rb");

    unsigned char block[G];
    //todo переделать под подсчитывание среднего
    fread(&block, 1 , G, file);

    int min = 0;
    int flag = 1;

    size_t parts = E * 1024 * 1024 / G;

    for (size_t part = 0; part < parts; part ++){
        int min_from_block = 0;
        int flag_for_block = 1;
        for (size_t i = 0; i < sizeof(block); i += 4){
            unsigned int num = 0;

            for (int j = 0; j < 4; j ++) {
                num = (num<<8) + block[i + j];
            }
            if (flag_for_block == 1){
                flag_for_block = 0;
                min_from_block = num;
            }else{
                if (min_from_block > num) min_from_block = num;
            }
        }
        if (flag == 1) {
            flag = 0;
            min = min_from_block;
        }else{
            if (min > min_from_block) min = min_from_block;
        }
    }
    fprintf(stdout,"\nMin from labOS%d from thread-%d is %d\n",idFile, id_thread, min );
    fflush(stdout);
    fclose(file);
    pthread_mutex_unlock(&cur_mut);
}

void* read_files(void* data){
    reader_thread_info* cur_data = (reader_thread_info*) data;
    while (loop) {
        for (int i = 0; i < filenumbers; i ++){
            read_file(cur_data->number_thread, i);
        }
    }
}



int main(void){
//    printf("Before allocation");
//    getchar();

    char* address = malloc(A*1024*1024);

//    printf("After allocation");
//    getchar();

    fill_space(address);

//    printf("After filling");
//    getchar();

    free(address);

//    printf("After deallocation");
//    getchar();

    pthread_t generate_thread;

    address = malloc(A*1024*1024);

    pthread_create(&generate_thread, NULL, generate_info, address);
     pthread_mutex_init(&mut1, NULL);
    pthread_mutex_init(&mut2, NULL);
    pthread_mutex_init(&mut3, NULL);


    pthread_t writer_thread[filenumbers];
    writer_thread_info writer_information[filenumbers];

    for (int  i = 0; i < filenumbers; i++) {
        writer_information[i].filenumber = i;
        writer_information[i].address = address;

    }
    //todo почему не создавать в прошлом цикле?
    for (int i = 0; i < filenumbers; i ++)
        pthread_create(&writer_thread[i], NULL, write_files, &writer_information[i]);

    pthread_t reader_thread[I];
    reader_thread_info reader_information[I];

    for (int i = 0; i < I; i++){
        reader_information[i].number_thread = i + 1;
        pthread_create(&reader_thread[i], NULL, read_files, &reader_information[i]);
    }



    printf("Pause");
    getchar();

    printf("End");
    getchar();

    loop = 0;

    for (int i = 0; i < I; i ++){
        pthread_join(reader_thread[i], NULL);
    }

    for (int i = 0; i < filenumbers; i ++)
        pthread_join(writer_thread[i], NULL);

    pthread_join(generate_thread, NULL);
    pthread_mutex_destroy(&mut1);
    pthread_mutex_destroy(&mut2);
    pthread_mutex_destroy(&mut3);

//    sem_destroy(&sem1);
//    sem_destroy(&sem2);
//    sem_destroy(&sem3);

    free(address);
    return 0;
}
//http://cppstudio.com/post/1641/ fread
//http://cppstudio.com/post/1249/ fflush
//https://learnc.info/c/pthreads_mutex_introduction.html mutex
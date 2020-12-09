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
// A=128;B=0x74594352;C=malloc;D=55;E=144;F=block;G=65;H=seq;I=48;J=avg;K=cv

#define A 128
#define D 55
#define E 144
#define G 65
#define I 48


const int number_of_files = 3;

pthread_mutex_t mut1;
pthread_mutex_t mut2;
pthread_mutex_t mut3;


int loop_flag = 1;

typedef struct {
    char *address;
    size_t size;
    FILE *file;
} info_for_thread;


typedef struct {
    char *address;
    int filenumber;
} info_for_writer;

typedef struct {
    int number_thread;
} info_for_reader;


void *write_memory(void *data) {

    info_for_thread *current_data = (info_for_thread *) data;
    fread(current_data->address, 1, current_data->size, current_data->file);

    pthread_exit(0);
}

void fill_memory(char *address) {

    char *address_offset = address;

    info_for_thread inf_for_thread[D];
    pthread_t threads[D];

    size_t size_for_thread = A * 1024 * 1024 / D;
    FILE *urandom_file = fopen("/dev/urandom", "rb");

    for (int i = 0; i < D; i++) {
        inf_for_thread[i].address = address_offset;
        inf_for_thread[i].size = size_for_thread;
        inf_for_thread[i].file = urandom_file;
        address_offset += size_for_thread;
    }

    inf_for_thread[D - 1].size += A * 1024 * 1024 % D;


    for (int i = 0; i < D; i++)
        pthread_create(&threads[i], NULL, write_memory, &inf_for_thread[i]);


    for (int i = 0; i < D; i++)
        pthread_join(threads[i], NULL);

    fclose(urandom_file);

}

void *generate_info(void *data) {
    char *current_data = (char *) data;
    while (loop_flag == 1) {
        fill_memory(current_data);
    }
    pthread_exit(0);
}

pthread_mutex_t get_mut(int id_file) {
    if (id_file == 0) return mut1;
    if (id_file == 1) return mut2;
    if (id_file == 2) return mut3;
}


void write_memory_in_file(info_for_writer *data, int id_file) {

    pthread_mutex_t current_mutex = get_mut(id_file);
    pthread_mutex_lock(&current_mutex);


    char *file_name = malloc(sizeof(char) * 6);
    snprintf(file_name, sizeof(file_name) + 1, "labOS%d", ++id_file);
    FILE *file = fopen(file_name, "wb");


    size_t memory_size = A * 1024 * 1024;
    size_t wrote_count = 0;
    while (wrote_count < memory_size) {
        long will_write_count = memory_size - wrote_count >= G ? G : memory_size - wrote_count;
        wrote_count += fwrite(data->address + wrote_count, 1, will_write_count, file);
    }
    fclose(file);
    pthread_mutex_unlock(&current_mutex);

}

void *write_files(void *data) {
    info_for_writer *current_data = (info_for_writer *) data;
    while (loop_flag == 1) {
        write_memory_in_file(current_data, current_data->filenumber);

    }
}


void read_file(int id_thread, int id_file) {
    pthread_mutex_t current_mutex = get_mut(id_file);
    pthread_mutex_lock(&current_mutex);


    char *file_name = malloc(sizeof(char) * 6);
    snprintf(file_name, sizeof(file_name) + 1, "labOS%d", ++id_file);
    FILE *file = fopen(file_name, "rb");

    unsigned char block_for_read[G];
    fread(&block_for_read, 1, G, file);


    unsigned int sum_in_block = 0;
    unsigned int sum = 0;
    size_t parts = E * 1024 * 1024 / G;

    for (size_t part = 0; part < parts; part++) {

        for (size_t i = 0; i < sizeof(block_for_read); i += 4) {
            unsigned int num = 0;
            for (int j = 0; j < 4; j++) {
                num = (num << 8) + block_for_read[i + j];
            }
            sum_in_block += num;
        }
        sum += sum_in_block;
    }
    fprintf(stdout, "\nСреднее значение из файла labOS%d потока номер %d равно %lu\n", id_file, id_thread,
            sum / (parts * 16));
    fflush(stdout);
    fclose(file);
    pthread_mutex_unlock(&current_mutex);
}

void *read_files(void *data) {
    info_for_reader *current_data = (info_for_reader *) data;
    while (loop_flag) {
        for (int i = 0; i < number_of_files; i++) {
            read_file(current_data->number_thread, i);
        }
    }
}


int main(void) {
    printf("До аллокации");
    getchar();

    char *address = malloc(A * 1024 * 1024);

    printf("После аллокации");
    getchar();

    fill_memory(address);

    printf("После заполнения памяти");
    getchar();

    free(address);

    printf("После деаллокации");
    getchar();

    pthread_t generate_thread;

    address = malloc(A * 1024 * 1024);

    pthread_create(&generate_thread, NULL, generate_info, address);
    pthread_mutex_init(&mut1, NULL);
    pthread_mutex_init(&mut2, NULL);
    pthread_mutex_init(&mut3, NULL);


    pthread_t writer_thread[number_of_files];
    info_for_writer writer_information[number_of_files];

    for (int i = 0; i < number_of_files; i++) {
        writer_information[i].filenumber = i;
        writer_information[i].address = address;
        pthread_create(&writer_thread[i], NULL, write_files, &writer_information[i]);
    }

    pthread_t reader_thread[I];
    info_for_reader reader_information[I];

    for (int i = 0; i < I; i++) {
        reader_information[i].number_thread = i + 1;
        pthread_create(&reader_thread[i], NULL, read_files, &reader_information[i]);
    }


    printf("Чтобы закончить бесконечный цикл нажмите ENTER два раза и дождитесь конца");
    getchar();
    getchar();

    loop_flag = 0;

    for (int i = 0; i < I; i++) {
        pthread_join(reader_thread[i], NULL);
    }

    for (int i = 0; i < number_of_files; i++)
        pthread_join(writer_thread[i], NULL);

    pthread_join(generate_thread, NULL);

    pthread_mutex_destroy(&mut1);
    pthread_mutex_destroy(&mut2);
    pthread_mutex_destroy(&mut3);

    free(address);
    return 0;
}

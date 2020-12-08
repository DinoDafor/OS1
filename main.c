#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <malloc.h>
//A=128;B=0x74594352;C=malloc;D=55;E=144;F=block;G=65;H=seq;I=48;J=avg;K=cv
// ванс A=197;B=0xA35E1090;C=mmap;D=78;E=72;F=block;G=10;H=random;I=35;J=min;K=sem
#define A 128
#define B 0x74594352 //you can't do it with malloc

#define D 3
#define E 144
#define F block
#define G 65
#define H seq
#define I 48
#define J avg
#define K cv
/*Разработать программу на языке С, которая осуществляет следующие действия

Создает область памяти размером A мегабайт, начинающихся с адреса B (если возможно) при помощи C=(malloc, mmap) заполненную случайными числами /dev/urandom в D потоков.
Используя системные средства мониторинга определите адрес начала в адресном пространстве процесса и характеристики выделенных участков памяти. Замеры виртуальной/физической памяти необходимо снять:

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
pthread_mutex_t mutex;
const int filenumbers =1; //todo не подходит кажется
int loop = 1;

typedef struct {
    char* address;
    size_t size;
    FILE* file;
}thread_args;
typedef struct {
    char* address;
    int filenumber;
}writer_thread_info;


void* writeInMemory(void* args) {
   thread_args* current_data = (thread_args*) args;
    fread(current_data->address, sizeof (char), current_data->size, current_data->file);
pthread_exit(0);
}
void fill_memory(char* point_address){

    thread_args  infForThreads[D];
    pthread_t poolOfThreads[D];

    size_t size_for_thread = A * 1024 * 1024 / D;
    FILE* urandomFileStream = fopen("/dev/urandom", "rb"); //modes?

    for (int i = 0; i < D; i ++){
        infForThreads[i].address = point_address;
        infForThreads[i].size = size_for_thread;
        infForThreads[i].file = urandomFileStream;
        point_address += size_for_thread;
    }
    infForThreads[D - 1].size += A * 1024 * 1024 % D;

    for (int i = 0; i < D; i ++)
        pthread_create(&poolOfThreads[i], NULL, writeInMemory, &infForThreads[i]);


    for (int i = 0; i < D; i ++)
        pthread_join(poolOfThreads[i], NULL);

    fclose(urandomFileStream); // проверить на 0 значение через if
    //


}
void* generate_info(void* data){
    char* cur_data = (char*) data;
    while(loop == 1){
        fill_memory(cur_data);
    }

    pthread_exit(0);
}

void write_file(writer_thread_info* data, int idFile){
    pthread_mutex_lock(&mutex);

    char* defaultname = malloc(sizeof(char) * 6);
    snprintf(defaultname, sizeof(defaultname) + 1, "labOS%d",++idFile);
    FILE* file = fopen(defaultname, "wb");

    size_t file_size = E * 1024 * 1024;
    size_t wrote_count = 0;
    //todo нейминг rest and size
    while (wrote_count < file_size) {

        long count_on_write = file_size - wrote_count >= G ? G : file_size - wrote_count;
        wrote_count += fwrite(data->address + wrote_count, 1, count_on_write, file);
    }
    pthread_mutex_unlock(&mutex);

}
void* write_files(void* data){
    writer_thread_info* cur_data = (writer_thread_info*) data;

    while (loop == 1){
        write_file(cur_data, cur_data->filenumber);

    }
}


int main() {

//    char* address = malloc(A*1024*1024);
//
//
//    fill_memory(address);
//
//
//   free(address);
//
   pthread_t generate_thread;

char* address = malloc(A*1024*1024);
    //WRITE BEGIN
    pthread_create(&generate_thread, NULL, generate_info, address);

    pthread_t writer_thread[filenumbers];
    pthread_mutex_init(&mutex, NULL);// todo инициализация мьютекса
    writer_thread_info writer_information[filenumbers];
    for (int  i = 0; i < filenumbers; i++) {
        writer_information[i].filenumber = i;
        writer_information[i].address = address;
    }
    for (int i = 0; i < filenumbers; i ++)
        pthread_create(&writer_thread[i], NULL, write_files, &writer_information[i]);
    //WRITE END


}

//https://prog-cpp.ru/c-pointers/
//https://ru.stackoverflow.com/questions/785269/Недопонимание-с-функцией-pthread-create
//https://learnc.info/c/pthreads_create_and_join.html
//http://cppstudio.com/post/1641/ fread
//https://learnc.info/c/functions.html
//https://learnc.info/c/structures.html
//https://stackoverflow.com/questions/58087929/incompatible-pointer-types-passing-void-void-to-parameter-of-type-void
//http://cppstudio.com/post/1245/
// https://it.wikireading.ru/24955 mmap, munmap, msync;
//https://prog-cpp.ru/c-files/
//https://ru.stackoverflow.com/questions/1067483/Ошибка-process-finished-with-exit-code-139-interrupted-by-signal-11-sigsegv
//http://uinc.ru/articles/34/ processes and threads
//https://stackoverflow.com/questions/1345670/stack-smashing-detected
//http://cppstudio.com/post/1637/ fwrite
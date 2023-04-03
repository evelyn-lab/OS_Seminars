// Общий модуль, осуществляющий одинаковые административные функции
// как для писателя, так и для читателя.
#include "common.h"
#include <semaphore.h>

// имя области разделяемой памяти
const char* shar_object = "/posix-shar-object";
int buf_id;        // дескриптор объекта памяти
shared_memory *buffer;    // указатель на разделямую память, хранящую буфер

// имя семафора для занятых ячеек
const char *full_sem_name = "/full-semaphore";
sem_t *full;   // указатель на семафор занятых ячеек

// имя семафора для свободных ячеек
const char *empty_sem_name = "/empty-semaphore";
sem_t *empty;   // указатель на семафор свободных ячеек

// имя семафора (мьютекса) для чтения данных из буфера
const char *mutex_sem_name = "/mutex-semaphore";
sem_t *mutex;   // указатель на семафор читателей

// Имя семафора для подсчета контроля за запуском процессов
const char *admin_sem_name = "/admin-semaphore";
sem_t *admin;   // указатель на семафор читателей

typedef struct {
  char* buffer;
  int start;
  int end;
  int full;
} ring_buffer;

// Функция осуществляющая при запуске общие манипуляции с памятью и семафорами
// для децентрализованно подключаемых процессов читателей и писателей.
void init(void) {
  // Создание или открытие семафора для администрирования (доступ открыт)
  if((admin = sem_open(admin_sem_name, O_CREAT, 0666, 0)) == 0) {
    perror("sem_open: Can not create admin semaphore");
    exit(-1);
  };
  // Создание или открытие мьютекса для доступа к буферу (доступ открыт)
  if((mutex = sem_open(mutex_sem_name, O_CREAT, 0666, 1)) == 0) {
    perror("sem_open: Can not create mutex semaphore");
    exit(-1);
  };
  // Количество свободных ячеек равно BUF_SIZE
  if((empty = sem_open(empty_sem_name, O_CREAT, 0666, BUF_SIZE)) == 0) {
    perror("sem_open: Can not create free semaphore");
    exit(-1);
  };
  // Количество занятых ячеек равно 0
  if((full = sem_open(full_sem_name, O_CREAT, 0666, 0)) == 0) {
    perror("sem_open: Can not create busy semaphore");
    exit(-1);
  };

  // Создание или открытие области разделяемой памяти
  if((buf_id = shm_open(shar_object, O_CREAT | O_RDWR, 0666)) == -1) {
    perror("shm_open");
    exit(-1);
  };

  // Определение размера разделяемой памяти
  if(ftruncate(buf_id, sizeof(ring_buffer) + BUF_SIZE) == -1) {
    perror("ftruncate");
    exit(-1);
  };
}

// Функция закрывающая семафоры общие для писателей и читателей
void close_common_semaphores(void) {
  if(sem_close(empty) == -1) {
    perror("sem_close: Incorrect close of empty semaphore");
    exit(-1);
  };
  if(sem_close(full) == -1) {
    perror("sem_close: Incorrect close of busy semaphore");
    exit(-1);
  };
  if(sem_close(admin) == -1) {
    perror("sem_close: Incorrect close of admin semaphore");
    exit(-1);
  };
  if(sem_close(mutex) == -1) {
    perror("sem_close: Incorrect close of mutex semaphore");
    exit(-1);
  };
}

// Функция, записывающая данные в разделяемую память
void write_buffer(char *data) {
// ожидание освобождения ячейки
if(sem_wait(empty) == -1) {
perror("sem_wait: Can not wait empty semaphore");
exit(-1);
};

// ожидание доступности разделяемой памяти
if(sem_wait(mutex) == -1) {
perror("sem_wait: Can not wait mutex semaphore");
exit(-1);
};

// запись данных в буфер
memcpy(buffer->data[buffer->write_pos], data, MAX_STRING_SIZE);
buffer->write_pos = (buffer->write_pos + 1) % BUF_SIZE;

// освобождение разделяемой памяти
if(sem_post(mutex) == -1) {
perror("sem_post: Can not unlock mutex semaphore");
exit(-1);
};

// увеличение счетчика занятых ячеек
if(sem_post(full) == -1) {
perror("sem_post: Can not post full semaphore");
exit(-1);
};
}

// Функция, считывающая данные из разделяемой памяти
void read_buffer(char *data) {
// ожидание заполнения ячейки
if(sem_wait(full) == -1) {
perror("sem_wait: Can not wait full semaphore");
exit(-1);
};

// ожидание доступности разделяемой памяти
if(sem_wait(mutex) == -1) {
perror("sem_wait: Can not wait mutex semaphore");
exit(-1);
};

// чтение данных из буфера
memcpy(data, buffer->data[buffer->read_pos], MAX_STRING_SIZE);
buffer->read_pos = (buffer->read_pos + 1) % BUF_SIZE;

// освобождение разделяемой памяти
if(sem_post(mutex) == -1) {
perror("sem_post: Can not unlock mutex semaphore");
exit(-1);
};

// увеличение счетчика свободных ячеек
if(sem_post(empty) == -1) {
perror("sem_post: Can not post empty semaphore");
exit(-1);
};
}

// Главная функция писателя
void writer_main() {
printf("Writer started\n");

// Запись данных в буфер
for (int i = 0; i < NUM_WRITES; i++) {
char data[MAX_STRING_SIZE];
sprintf(data, "Message %d", i+1);
write_buffer(data);
printf("Writer wrote: %s\n", data);
usleep(WRITE_DELAY);
}

printf("Writer finished\n");

// закрытие общих семафоров
close_common_semaphores();

// завершение процесса
exit(0);
}



// Функция, удаляющая все семафоры и разделяемую память
void unlink_all(void) {
  if(sem_unlink(mutex_sem_name) == -1) {
    perror("sem_unlink: Incorrect unlink of mutex semaphore");
    // exit(-1);
  };
  if(sem_unlink(empty_sem_name) == -1) {
    perror("sem_unlink: Incorrect unlink of empty semaphore");
    // exit(-1);
  };
  if(sem_unlink(full_sem_name) == -1) {
    perror("sem_unlink: Incorrect unlink of full semaphore");
    // exit(-1);
  };
  if(sem_unlink(admin_sem_name) == -1) {
    perror("sem_unlink: Incorrect unlink of admin semaphore");
    // exit(-1);
  };

  // удаление разделяемой памяти
  if(shm_unlink(shar_object) == -1) {
    perror("shm_unlink");
    // exit(-1);
  }
}

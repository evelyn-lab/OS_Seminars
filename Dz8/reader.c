#include "common.h"
#include <unistd.h>

// Семафор для отсекания лишних читателей
// То есть, для формирования только одного процесса-читателя
const char *reader_sem_name = "/reader-semaphore";
sem_t *reader;   // указатель на семафор пропуска читателей

// Указатели для кольцевого буфера
int write_index = 0;
int read_index = 0;

void sigfunc(int sig) {
  if(sig != SIGINT && sig != SIGTERM) {
    return;
  }
  if(sig == SIGINT) {
    kill(buffer->writer_pid, SIGTERM);
    printf("Reader(SIGINT) ---> Writer(SIGTERM)\n");
  } else if(sig == SIGTERM) {
    printf("Reader(SIGTERM) <--- Writer(SIGINT)\n");
  }
  // Закрывает свой семафор
  if(sem_close(reader) == -1) {
    perror("sem_close: Incorrect close of reader semaphore");
    exit(-1);
  };
  // Удаляет свой семафор
  if(sem_unlink(reader_sem_name) == -1) {
    perror("sem_unlink: Incorrect unlink of reader semaphore");
    // exit(-1);
  };
  printf("Reader: bye!!!\n");
  exit (10);
}

// Функция вычисления факториала
int factorial(int n) {
  int p = 1;
  for(int i = 1; i <= n; ++i) {
    p *= i;
  }
  return p;
}

int main() {
  signal(SIGINT,sigfunc);
  signal(SIGTERM,sigfunc);

  srand(time(0)); // Инициализация генератора случайных чисел
  init();         // Начальная инициализация общих семафоров
  // printf("init checkout\n");

  // Если читатель запустился раньше, он ждет разрешения от администратора,
  // Который находится в писателе
  if(sem_wait(admin) == -1) { // ожидание когда запустится писатель
    perror("sem_wait: Incorrect wait of admin semaphore");
    exit(-1);
  };
  printf("Consumer %d started\n", getpid());
  // После этого он вновь поднимает семафор, позволяющий запустить других читателей
  if(sem_post(admin) == -1) {
    perror("sem_post: Consumer can not increment admin semaphore");
    exit(-1);
  }

  // Читатель имеет доступ только к открытому объекту памяти,
  // но может не только читать, но и писать (забирать)
  if ( (buf_id = shm_open(shar_object, O_RDWR, 0666)) == -1 ) {
    perror("shm_open: Incorrect reader access");
    exit(-1);
  } else {
    printf("Memory object is opened: name = %s, id = 0x%x\n",
           shar_object, buf_id);
  }
  // Задание размера объекта памяти
  if (ftruncate(buf_id, sizeof(shared_memory)) == -1) {
    perror("ftruncate");
    exit(-1);
  } else {
    printf("Memory size set and = %lu\n", sizeof (shared_memory));
  }

  //получить доступ к памяти
  buffer = mmap(0, sizeof (shared_memory), PROT_WRITE|PROT_READ, MAP_SHARED, buf_id, 0);
  if (buffer == (shared_memory*)-1 ) {
    perror("reader: mmap");
    exit(-1);
  }
  // Критическая секция
// Блокировка буфера от других читателей
if(sem_wait(reader) == -1) { // Ожидание разрешения на чтение
  perror("sem_wait: Incorrect wait of reader semaphore");
  exit(-1);
};

// Получение очередного числа из буфера
n = buffer->data[buffer->read_pos];

// Записываем в ячейку 0, чтобы указать что ее можно использовать для записи
buffer->data[buffer->read_pos] = 0;

printf("%d!=%d\n", n, factorial(n));
fflush(stdout);

// Увеличение указателя чтения
buffer->read_pos = (buffer->read_pos + 1) % BUF_SIZE;

// Освобождение буфера
if(sem_post(writer) == -1) { // Увеличение разрешения на запись
  perror("sem_post: Incorrect increment of writer semaphore");
  exit(-1);
};

// Проверяем наличие дочерних процессов и завершаем их
if(buffer->num_children && !buffer->data[buffer->read_pos]) {
  for(int i = 0; i < buffer->num_children; ++i) {
    if(buffer->child_pids[i]) {
      kill(buffer->child_pids[i], SIGINT);
    }
  }
}

if(sem_post(admin) == -1) { // Увеличение разрешения от администратора
  perror("sem_post: Reader can not increment admin semaphore");
  exit(-1);
};

// Отсекание
if(sem_post(reader) == -1) { // Увеличение разрешения на чтение
  perror("sem_post: Reader can not increment reader semaphore");
  exit(-1);
};
}
}

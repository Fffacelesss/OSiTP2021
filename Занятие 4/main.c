#include "thread.h"
#include "periph/gpio.h"
#include "xtimer.h"
#include "msg.h"

#define LED_BLUE GPIO_PIN(PORT_C, 8)
#define LED_GREEN GPIO_PIN(PORT_C, 9)
#define BUTTON GPIO_PIN(PORT_A,0)
#define BLINK_PERIOD 100000

char thread_one_stack[THREAD_STACKSIZE_DEFAULT];
char thread_two_stack[THREAD_STACKSIZE_DEFAULT];

int start_time = 0;

static kernel_pid_t thread_one_pid, thread_two_pid;

void blink_in_loop_blicking(int iterations, int period)
{
xtimer_ticks32_t last_wakeup = xtimer_now();
gpio_set(LED_BLUE);
while(iterations--)
{
xtimer_periodic_wakeup(&last_wakeup, period);
gpio_toggle(LED_BLUE);
xtimer_periodic_wakeup(&last_wakeup, period);
gpio_toggle(LED_BLUE);
}
gpio_set(LED_BLUE);
}

// Обработчик прерывания с кнопки
void btn_handler_one(void *arg)
{
// Прием аргументов из главного потока
(void)arg;
// Создание объекта для хранения сообщения
msg_t msg;

static int last_handle = 0;
static int amount_of_blinks = 0;
int cur_handle = xtimer_usec_from_ticks(xtimer_now());

if (cur_handle - last_handle > 100000)
{
// Отправка сообщения в тред по его PID
// Сообщение остается пустым, поскольку нам сейчас важен только сам факт его наличия, и данные мы не передаем
msg.content.value = 1 + amount_of_blinks++ % 5;
msg_send(&msg, thread_one_pid);
last_handle = cur_handle;
}
}

void btn_handler_two_rise(void* arg)
{
(void)arg;
start_time = xtimer_usec_from_ticks(xtimer_now());

// msg_t msg;
// msg.content.value = 100000;
// msg_send(&msg, thread_one_pid);
}

void btn_handler_two_fall(void* arg)
{
(void)arg;

int cur_time = xtimer_usec_from_ticks(xtimer_now());
msg_t msg;

if (cur_time - start_time > 100000)
{
msg.content.value = (cur_time - start_time) / 100;
msg_send(&msg, thread_two_pid);
}

}

// Первый тред
void *thread_one(void *arg)
{
(void) arg;
msg_t msg;
gpio_init(LED_BLUE, GPIO_OUT);
while(1){
// Ждем, пока придет сообщение
msg_receive(&msg);
blink_in_loop_blicking(msg.content.value, BLINK_PERIOD);
// Переключаем светодиод
gpio_toggle(GPIO_PIN(PORT_C,8));
}
return NULL;
}

// Второй поток
void *thread_two(void *arg)
{
(void) arg;
gpio_init(LED_GREEN, GPIO_OUT);
xtimer_ticks32_t last_wakeup2 = xtimer_now();
int sleep_period = 333333;
msg_t msg;

while(1){
xtimer_periodic_wakeup(&last_wakeup2, sleep_period);
msg_receive(&msg);
blink_in_loop_blicking(5, msg.content.value);
}
return NULL;
}


int main(void)
{
  
// gpio_init_int(BUTTON, GPIO_IN, GPIO_RISING, btn_handler_one, NULL);
gpio_init_int(BUTTON, GPIO_IN, GPIO_RISING, btn_handler_two_rise, NULL);
gpio_init_int(BUTTON, GPIO_IN, GPIO_FALLING, btn_handler_two_fall, NULL);


// Для первого потока мы сохраняем себе то, что возвращает функция thread_create, чтобы потом пользоваться этим как идентификатором потока для отправки ему сообщений
thread_one_pid = thread_create(thread_one_stack, sizeof(thread_one_stack),
THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST,
thread_one, NULL, "thread_one");
  
thread_two_pid = thread_create(thread_two_stack, sizeof(thread_two_stack),
THREAD_PRIORITY_MAIN - 2, THREAD_CREATE_STACKTEST,
thread_two, NULL, "thread_two");

while (1){

}

return 0;
}

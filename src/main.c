#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

// Thread initializations
#define STACKSIZE 500
#define PRIORITY 5

K_SEM_DEFINE(release_sem, 0, 1);

//contidion variables 
K_MUTEX_DEFINE(red_mutex);
K_CONDVAR_DEFINE(red_signal);
K_MUTEX_DEFINE(green_mutex);
K_CONDVAR_DEFINE(green_signal);
K_MUTEX_DEFINE(yellow_mutex);
K_CONDVAR_DEFINE(yellow_signal);

// Valotaskien prototyypit
void red_task(void *a, void *b, void *c);
void green_task(void *a, void *b, void *c);
void yellow_task(void *a, void *b, void *c);


K_THREAD_DEFINE(red_thread, STACKSIZE, red_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(green_thread, STACKSIZE, green_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(yellow_thread, STACKSIZE, yellow_task, NULL, NULL, NULL, PRIORITY, 0, 0);


// UART initialization
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);


// Create dispatcher FIFO buffer
K_FIFO_DEFINE(dispatcher_fifo);

// FIFO dispatcher data type
struct data_t {
	void *fifo_reserved;
	char msg[20]; 
};

int init_uart(void) {
	// UART initialization
	if (!device_is_ready(uart_dev)) {
		return 1;
	} 
	return 0;
}

int main(void)
{
	int ret = init_uart();
	if (ret != 0) {
		printk("UART initialization failed!\n");
		return ret;
	}
        printk("System started!\n"); 
	return 0;
}

/********************
 * UART task
 */
void uart_task(void *unused1, void *unused2, void *unused3)
{
	char rc=0;
	char uart_msg[20];
	memset(uart_msg,0,20);
	int uart_msg_cnt = 0;

	while (true) {
		// Ask UART if data available
		if (uart_poll_in(uart_dev,&rc) == 0) {
			// printk("Received: %c\n",rc);
			// If character is not newline, add to UART message buffer
			if (rc != '\r') {
				if (uart_msg_cnt < (int)(sizeof(uart_msg) - 1)) {
					uart_msg[uart_msg_cnt] = rc;
					uart_msg_cnt++;
				}
			// Character is newline, copy dispatcher data and put to FIFO buffer
			} else {
				printk("UART msg: %s\n", uart_msg);
                
				struct data_t *buf = k_malloc(sizeof(struct data_t));
				if (buf == NULL) {
					return;
				}
				// Copy UART message to dispatcher data
				
				snprintf(buf->msg, 20, "%s", uart_msg);

				// You need to:
				// Put dispatcher data to FIFO buffer
                k_fifo_put(&dispatcher_fifo, buf);
                printk("Fifo data: %s\n" ,buf->msg);

				// Clear UART receive buffer
				uart_msg_cnt = 0;
				memset(uart_msg,0,20);
			}
		}
		k_msleep(10);
	}
}

/********************
 * Dispatcher task
 */
void dispatcher_task(void *unused1, void *unused2, void *unused3)
{
	while (true) {
		// Receive dispatcher data from uart_task fifo
		struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
		char sequence[20];
		memcpy(sequence,rec_item->msg,20);
		k_free(rec_item);

		printk("Dispatcher: %s\n", sequence);


        		// 2. Käy sekvenssi läpi kirjain kerrallaan (esim. "RYG")
		for (int i = 0; i < strlen(sequence); i++) {
			char color = sequence[i];
			printk("Next color: %c\n", color);

			// 3. Lähetä signaali oikealle valotaskille
			if (color == 'R') {
				k_mutex_lock(&red_mutex, K_FOREVER);
				k_condvar_signal(&red_signal);
				k_mutex_unlock(&red_mutex);
			} else if (color == 'Y') {
				k_mutex_lock(&yellow_mutex, K_FOREVER);
				k_condvar_signal(&yellow_signal);
				k_mutex_unlock(&yellow_mutex);
			} else if (color == 'G') {
				k_mutex_lock(&green_mutex, K_FOREVER);
				k_condvar_signal(&green_signal);
				k_mutex_unlock(&green_mutex);
			}

			// 4. Odota, että valotaski ilmoittaa olevansa valmis
			k_sem_take(&release_sem, K_FOREVER);
		}
	}
}


void red_task(void *a, void *b, void *c) {
    while (1) {
        k_mutex_lock(&red_mutex, K_FOREVER);
        k_condvar_wait(&red_signal, &red_mutex, K_FOREVER);
        k_mutex_unlock(&red_mutex);

        printk("Red on\n");
        k_msleep(500); // simulaatio, valon kesto
        printk("Red off\n");

        // Ilmoitetaan dispatcherille että tehtävä valmis
        k_sem_give(&release_sem);
    }
}

void green_task(void *a, void *b, void *c) {
    while (1) {
        k_mutex_lock(&green_mutex, K_FOREVER);
        k_condvar_wait(&green_signal, &green_mutex, K_FOREVER);
        k_mutex_unlock(&green_mutex);

        printk("Green on\n");
        k_msleep(500);
        printk("Green off\n");

        k_sem_give(&release_sem);
    }
}

void yellow_task(void *a, void *b, void *c) {
    while (1) {
        k_mutex_lock(&yellow_mutex, K_FOREVER);
        k_condvar_wait(&yellow_signal, &yellow_mutex, K_FOREVER);
        k_mutex_unlock(&yellow_mutex);

        printk("Yellow on\n");
        k_msleep(500);
        printk("Yellow off\n");

        k_sem_give(&release_sem);
    }
}

K_THREAD_DEFINE(dis_thread,STACKSIZE,dispatcher_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(uart_thread,STACKSIZE,uart_task,NULL,NULL,NULL,PRIORITY,0,0);

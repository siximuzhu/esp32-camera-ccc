#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"


#define KEY1			33
#define KEY2			32
#define KEY3			21
#define KEY4			8
#define FLASH_LED		22

typedef enum
{
	KEY1_SHORT_PRESSED = 1,
	KEY1_LONG_PRESSED,
    KEY2_SHORT_PRESSED,
	KEY2_LONG_PRESSED,
	KEY3_SHORT_PRESSED,
	KEY3_LONG_PRESSED,
	KEY4_SHORT_PRESSED,
	KEY4_LONG_PRESSED,
}KEY_PRESS_E;


#define ESP_INTR_FLAG_DEFAULT 0

unsigned int key_press = 0;


static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}
int key3_press_down_tick;
int key3_press_up_tick;

int press_down_tick;
int press_up_tick;


unsigned int  get_pressed_key(void)
{
	unsigned int result = key_press;
//	key_press = 0;
	return result;
}



static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
#if 0
			if(KEY3 == io_num)
			{
				if(0 == gpio_get_level(io_num))
				{
					key3_press_down_tick = xTaskGetTickCount();
				}
				else
				{
					key3_press_up_tick = xTaskGetTickCount();
					if(key3_press_up_tick - key3_press_down_tick > 3000 / portTICK_RATE_MS)
					{
						key_press = KEY3_LONG_PRESSED;
					//	printf("key3 long press!\n");
					}
					else
					{
						key_press = KEY3_SHORT_PRESSED;
					//	printf("key3 short press!\n");
					}
				}
			}
			else
			{		
				if(1 == gpio_get_level(io_num))
				{
					switch (io_num)
					{
						case KEY1:key_press = KEY1_PRESSED;break;
						case KEY2:key_press = KEY2_PRESSED;break;
						case KEY4:key_press = KEY4_PRESSED;break;
					}
				}
			}
#endif
            if(0 == gpio_get_level(io_num)){
                press_down_tick = xTaskGetTickCount();
                printf("press_down_tick = %d\n",press_down_tick);
            }else{
                    press_up_tick = xTaskGetTickCount();
                    printf("press_up_tick = %d\n",press_up_tick);
					if(press_up_tick - press_down_tick > 3000 / portTICK_RATE_MS)
					{
                        switch(io_num)
                        {
                            case KEY1:key_press = KEY1_LONG_PRESSED;break;
                            case KEY2:key_press = KEY2_LONG_PRESSED;break;
                            case KEY3:key_press = KEY3_LONG_PRESSED;break;
                            case KEY4:key_press = KEY4_LONG_PRESSED;break;
                        }
					}
					else
					{
                        switch(io_num)
                        {
                            case KEY1:key_press = KEY1_SHORT_PRESSED;break;
                            case KEY2:key_press = KEY2_SHORT_PRESSED;break;
                            case KEY3:key_press = KEY3_SHORT_PRESSED;break;
                            case KEY4:key_press = KEY4_SHORT_PRESSED;break;
                        }
					}
            }
        }
    }
}




void key_init(void)
{
    gpio_config_t io_conf;

    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (1ULL <<                FLASH_LED);
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 1;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_ANYEDGE;//GPIO_PIN_INTR_POSEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = ((1ULL << KEY1) | (1ULL << KEY2) | (1ULL << KEY3) | (1ULL << KEY4));
    //set as input mode    
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //change gpio intrrupt type for one pin
 //   gpio_set_intr_type(KEY3, GPIO_INTR_ANYEDGE);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(20, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(KEY1, gpio_isr_handler, (void*) KEY1);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(KEY2, gpio_isr_handler, (void*) KEY2);
	gpio_isr_handler_add(KEY3, gpio_isr_handler, (void*) KEY3);
	gpio_isr_handler_add(KEY4, gpio_isr_handler, (void*) KEY4);

#if 0
    int cnt = 0;
    while(1) {
        printf("cnt: %d  systeim tick = %d \n", cnt++,xTaskGetTickCount());
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(FLASH_LED, cnt % 2);
  //      gpio_set_level(GPIO_OUTPUT_IO_1, cnt % 2);
    }
#endif

}

void switch_flash_led(bool flag)

{
	gpio_set_level(FLASH_LED, flag);
}




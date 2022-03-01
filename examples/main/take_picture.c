/**
 * This example takes a picture every 5s and print its size on serial monitor.
 */

// =============================== SETUP ======================================

// 1. Board setup (Uncomment):
// #define BOARD_WROVER_KIT
// #define BOARD_ESP32CAM_AITHINKER

/**
 * 2. Kconfig setup
 * 
 * If you have a Kconfig file, copy the content from
 *  https://github.com/espressif/esp32-camera/blob/master/Kconfig into it.
 * In case you haven't, copy and paste this Kconfig file inside the src directory.
 * This Kconfig file has definitions that allows more control over the camera and
 * how it will be initialized.
 */

/**
 * 3. Enable PSRAM on sdkconfig:
 * 
 * CONFIG_ESP32_SPIRAM_SUPPORT=y
 * 
 * More info on
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig.html#config-esp32-spiram-support
 */

// ================================ CODE ======================================

#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_camera.h"

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"

#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "sdkconfig.h"
#include "driver/sdmmc_host.h"

#include "../../sensors/esp32_ov5640_af.h"

#include <esp_heap_caps.h>

#define WIFI_SSID			"HUAWEI P20 Pro"
#define WIFI_PWD			"87654321"
#define HOST_IP				"124.71.221.251"
#define HOST_PORT			55348

#define OV5640_AF			0

//#define BOARD_WROVER_KIT 1
//#define BOARD_ESP32CAM_AITHINKER 1
#define MY_BOARD	1


// WROVER-KIT PIN Map
#ifdef BOARD_WROVER_KIT

#define CAM_PIN_PWDN -1  //power down is not used
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0//21
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 19
#define CAM_PIN_D2 18
#define CAM_PIN_D1 5
#define CAM_PIN_D0 4
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 15

#endif

// ESP32Cam (AiThinker) PIN Map
#ifdef BOARD_ESP32CAM_AITHINKER

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22


#endif

#ifdef MY_BOARD

#define CAM_PIN_PWDN 19
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 25
#define CAM_PIN_SIOC 26
#define CAM_PIN_D7 	35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 13
#define CAM_PIN_D4 2
#define CAM_PIN_D3 4
#define CAM_PIN_D2 7
#define CAM_PIN_D1 5 //5      20 ?6��������Ϊ����ͷ��GPIO
#define CAM_PIN_D0 12 //0  ����ͷ�����ͣ�Ӱ��оƬ�� ?
#define CAM_PIN_VSYNC 27
#define CAM_PIN_HREF 14
#define CAM_PIN_PCLK 15

#endif


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



static const char *TAG = "example:take_picture";

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sscb_sda = CAM_PIN_SIOD,
    .pin_sscb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 4000000,//12000000 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_UXGA,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 12,//3 12, //0-63 lower number means higher quality
    .fb_count = 2,       //if more than one, i2s runs in continuous mode. Use only with JPEG
    .grab_mode = CAMERA_GRAB_LATEST,
};

static esp_err_t init_camera()
{
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}


extern void socket_test(void);
extern void connect_wifi(void);

extern int connect_host(char* ip,int port);
extern int socket_send_to_host(int sock,char *data,int len);
extern void key_init(void);
extern void key_detect(void);
extern void adc_init(void);
extern unsigned int adc_get_voltage(void);
extern int get_token(char *access_token);
extern int upload_image(const unsigned char *image_src,unsigned int image_len);
extern unsigned int get_pressed_key(void);
extern void switch_flash_led(bool flag);
extern void ble_init(void);
extern int figer_searchDict(const unsigned char *image_src, unsigned int image_len, int type);
extern int figer_question(const unsigned char *image_src, unsigned int image_len);
extern int figer_point_to_server(const unsigned char *image_src, unsigned int image_len);
extern int sned_picture(const unsigned char *image_src, unsigned int image_len);
extern int send_device_message(void);

void send_one_picture(int mode)
{
    int g_count = 0;

	int8_t rc;
	int64_t fr_start = esp_timer_get_time();
#if OV5640_AF
	int times = 0;
	sensor_t * s = esp_camera_sensor_get();
	do
	{
		rc = OV5640_getFWStatus(s);
	}
	while ((rc != FW_STATUS_S_FOCUSED) && (times++ < 20000) );
	
	ESP_LOGI(TAG,"----------rc = %d",rc);
	if(-1 == rc)
	{
		ESP_LOGI(TAG,"-------------check your ov5640");
	}
	else if(FW_STATUS_S_FOCUSED == rc)
	{
		ESP_LOGI(TAG,"----------------focused");
	}
	else if(FW_STATUS_S_FOCUSING == rc)
	{
		ESP_LOGI(TAG,"----------focusing");
	}
#endif	
   camera_fb_t *pic = esp_camera_fb_get();
   int64_t fr_end = esp_timer_get_time();
	g_count++;


  // ESP_LOGI(TAG, "%d Picture taken! Its size was: %d bytes��w = %d, h = %d time = %.2f ms", g_count,pic->len,pic->width,pic->height,((fr_end - fr_start)/1000.0));
//	upload_image(pic->buf,pic->len);
    ESP_LOGI(TAG, "%d callback mode", mode);

	switch (mode)
	{
	case 1:
		upload_image(pic->buf,pic->len);
		break;
	case 2:
		figer_searchDict(pic->buf,pic->len, 0);
		break;
	case 3:
		figer_searchDict(pic->buf,pic->len, 1);
		break;
	case 4:
		figer_question(pic->buf,pic->len);
		break;
	case 5:
		figer_point_to_server(pic->buf,pic->len);
		break;
	case 6:
		sned_picture(pic->buf,pic->len);
		break;
	case 7:
		send_device_message();
		break;
	default:
		break;
	}

   esp_camera_fb_return(pic);

}

void app_main()
{
	unsigned int key = 0;
	unsigned int old_key = 0;
	unsigned int mode = 0;
	unsigned int voltage;
	bool led_status = false;

    unsigned char data[5] = {0};

//	ble_init(); 

	key_init();
	adc_init();

	connect_wifi();
#if 1
    ESP_LOGI(TAG, "begin init camera...");
    if(ESP_OK != init_camera()) {
        ESP_LOGI(TAG, " init camera error...");
        return;
    }
 	ESP_LOGI(TAG, " init camera ok...");

	
	sensor_t * s = esp_camera_sensor_get();
	//s->set_hmirror(s,true);
#endif
#if OV5640_AF
	OV5640_start(s);
	if(OV5640_focusInit(s) == 0)
	{
		ESP_LOGI(TAG, "focus init ok...");
	}
	else
	{
		ESP_LOGI(TAG, "focus init fail...");
	}

	if(OV5640_autoFocusMode(s) == 0)
	{
		ESP_LOGI(TAG, "autoFocus ok...");
	}
	else
	{
		ESP_LOGI(TAG, "autoFocus fail...");
	}
#endif  
    static bool flag = false;
    while(1)
    {
		key = get_pressed_key();
		if(old_key != key)
		{
			old_key = key;
			switch (key)
			{
				case KEY1_SHORT_PRESSED:
					ESP_LOGI(TAG, "KEY1_SHORT_PRESSED");
					mode = 1;
				//	s->set_framesize(s,FRAMESIZE_P_HD);
				break;

				case KEY2_SHORT_PRESSED:
					ESP_LOGI(TAG, "KEY2_SHORT_PRESSED");
					mode = 2;
				//	s->set_framesize(s,FRAMESIZE_P_HD);
				break;

				case KEY3_SHORT_PRESSED:
					ESP_LOGI(TAG, "KEY3_SHORT_PRESSED");
					mode = 3;
				//	s->set_framesize(s,FRAMESIZE_P_HD);
				break;

				case KEY4_SHORT_PRESSED:
					ESP_LOGI(TAG, "KEY4_SHORT_PRESSED");
					mode = 4;
				//	s->set_framesize(s,FRAMESIZE_P_HD);
				break;

				case KEY1_LONG_PRESSED:
					ESP_LOGI(TAG, "KEY1_LONG_PRESSED");
				//	s->set_framesize(s,FRAMESIZE_HQVGA);
					mode = 5;
				break;

				case KEY2_LONG_PRESSED:
					ESP_LOGI(TAG, "KEY2_LONG_PRESSED");
				//	s->set_framesize(s,FRAMESIZE_HQVGA);
					mode = 6;
				break;

				case KEY3_LONG_PRESSED:
					ESP_LOGI(TAG, "KEY3_LONG_PRESSED");
					led_status = !led_status;
					switch_flash_led(led_status);
				break;

				case KEY4_LONG_PRESSED:
					ESP_LOGI(TAG, "KEY4_LONG_PRESSED");
					mode = 7;
				break;
			} 
		}

		voltage = adc_get_voltage();
		if(voltage < 2400)//mV
		{
			if( (key == KEY1_LONG_PRESSED) || (key == KEY2_LONG_PRESSED))
			{
				send_one_picture(mode);
			}
			else
			{
				if (!flag) 
				{
					ESP_LOGI(TAG,"voltage = %d",voltage);
	                send_one_picture(mode);
	                flag = true;
	            }
			}
		} 
		else if (voltage >= 3000) 
		{
            flag = false;
        }

	 	vTaskDelay(10 / portTICK_RATE_MS);
    }


}
 
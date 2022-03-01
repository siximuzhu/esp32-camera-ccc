#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
//#include "protocol_examples_common.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"

#include "esp_http_client.h"
#include "cJSON.h"
//#include "base64.h"


#define MAX_HTTP_RECV_BUFFER 2048
#define MAX_HTTP_OUTPUT_BUFFER 2048
static const char *TAG = "HTTP_CLIENT";


int response_count = 0;
char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
            ESP_LOGI(TAG,"%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
    //        if (!esp_http_client_is_chunked_response(evt->client)) {
     //           ESP_LOGI(TAG,"%.*s", evt->data_len, (char*)evt->data);
				memcpy(local_response_buffer + response_count,evt->data,evt->data_len);
				response_count += evt->data_len;
   //         }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}


extern char * base64_encode(const void *src, size_t len, size_t *out_len);


int get_token(char *access_token)
{
	int len;
	const cJSON *token = NULL;

	const char *token_url = "https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=f0ZCCvt5ST5UMUP6XUQMST0z&client_secret=WOF6r0AAUu6wlR2U24w2mXCxeq9N7tIr";
	esp_http_client_config_t config = {
		.url = token_url,
		.event_handler = _http_event_handle,
//		.user_data = local_response_buffer,
		.method = HTTP_METHOD_GET,
		};

	esp_http_client_handle_t client = esp_http_client_init(&config);
//	esp_http_client_set_url(client,token_url);
//	esp_http_client_set_method(client,HTTP_METHOD_GET);
	esp_err_t err = esp_http_client_perform(client);

	if(err == ESP_OK)
	{
		len = esp_http_client_get_content_length(client);
		ESP_LOGI(TAG,"HTTP GET STATUS = %d,len = %d",esp_http_client_get_status_code(client),len);
		//ESP_LOGI(TAG,"token:%s",local_response_buffer);
	}
	else
	{
		ESP_LOGI(TAG,"HTTP GET FAILED:%s",esp_err_to_name(err));
	}

	cJSON *token_json = cJSON_Parse(local_response_buffer);
	if(NULL == token_json)
	{
		ESP_LOGI(TAG,"JSON error.free heap size = %d  %d",esp_get_free_heap_size(),esp_get_minimum_free_heap_size());
	}
	else
	{
		token = cJSON_GetObjectItemCaseSensitive(token_json, "access_token");

		ESP_LOGI(TAG,"access_token:%s",token->valuestring);

		strcpy(access_token,token->valuestring);
	}
	
	memset(local_response_buffer,0x0,MAX_HTTP_OUTPUT_BUFFER);
	response_count = 0;

	esp_http_client_cleanup(client);

	return 0;


	
}

int upload_image(const unsigned char *image_src,unsigned int image_len)
{
   char* image_base64;
    int len;
    char *post_data;
    size_t out_len;
    uint8_t chipid[6];
    char access_token[100] = {0};

    get_token(access_token);

    image_base64 = base64_encode(image_src,image_len,&out_len);

    if(image_base64 == NULL)
    {
        ESP_LOGI(TAG,"base64 error");
        return -1;
    }

    post_data = (char *) malloc(out_len + 150);
    if(post_data != NULL)
    {
        ESP_LOGI(TAG,"malloc %d bytes mem success",out_len + 150);
    }
    else{
        ESP_LOGI(TAG,"malloc %d bytes mem fail",out_len + 150);
        return -2;
    }

    esp_efuse_mac_get_default(&chipid);

//    sprintf(post_data,"{\"image_base64\":\"%s\",\"token\":\"%s\"}",image_base64,access_token);
    sprintf(post_data,"{\"image_base64\":\"%s\",\"token\":\"%s\",\"uuid\":\"%02x%02x%02x%02x%02x%02x\"}",image_base64,access_token,chipid[0],chipid[1],chipid[2],chipid[3],chipid[4],chipid[5]);


    //  ESP_LOGI(TAG,"%d post_data:%s",strlen(post_data),post_data);

    const char *iamge_post_url = "http://yida.qjun.com.cn:8078/getAudioUrlToServer";
    esp_http_client_config_t config = {
            .url = iamge_post_url,
            .event_handler = _http_event_handle,
            .method = HTTP_METHOD_POST,
    };

    ESP_LOGI(TAG,"---------------1");
    esp_http_client_handle_t client = esp_http_client_init(&config);
    ESP_LOGI(TAG,"---------------2");
    esp_http_client_set_header(client, "Content-Type", "application/json");
    ESP_LOGI(TAG,"---------------3");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    ESP_LOGI(TAG,"---------------4");

    esp_err_t err = esp_http_client_perform(client);
    ESP_LOGI(TAG,"---------------5");
    if(err == ESP_OK)
    {
        len = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG,"HTTP GET STATUS = %d,len = %d",esp_http_client_get_status_code(client),len);
        ESP_LOGI(TAG,"%s",local_response_buffer);
        memset(local_response_buffer,0x0,MAX_HTTP_OUTPUT_BUFFER);
        response_count = 0;

    }
    else
    {
        ESP_LOGI(TAG,"HTTP GET FAILED:%s",esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(post_data);
    free(image_base64);

    return 0;
}

int figer_searchDict(const unsigned char *image_src, unsigned int image_len, int type)
{
    char* image_base64;
    int len;
    char *post_data;
    size_t out_len;
    uint8_t chipid[6];

    image_base64 = base64_encode(image_src,image_len,&out_len);

    if(image_base64 == NULL)
    {
        ESP_LOGI(TAG,"base64 error");
        return -1;
    }

    post_data = (char *) malloc(out_len + 150);
    if(post_data != NULL)
    {
        ESP_LOGI(TAG,"malloc %d bytes mem success",out_len + 150);
    }
    else{
        ESP_LOGI(TAG,"malloc %d bytes mem fail",out_len + 150);
        return -2;
    }

    esp_efuse_mac_get_default(&chipid);

    sprintf(post_data,"{\"img_base64\":\"%s\",\"type\":%d,\"uuid\":\"%02x%02x%02x%02x%02x%02x\"}",image_base64,type,chipid[0],chipid[1],chipid[2],chipid[3],chipid[4],chipid[5]);

   // ESP_LOGI(TAG,"%d post_data:%s",strlen(post_data),post_data);

    const char *iamge_post_url = "http://yida.qjun.com.cn:30000/getWordFromBitmap";
    esp_http_client_config_t config = {
            .url = iamge_post_url,
            .event_handler = _http_event_handle,
            .method = HTTP_METHOD_POST,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);

    if(err == ESP_OK)
    {
        len = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG,"HTTP GET STATUS = %d,len = %d",esp_http_client_get_status_code(client),len);
        ESP_LOGI(TAG,"%s",local_response_buffer);
        memset(local_response_buffer,0x0,MAX_HTTP_OUTPUT_BUFFER);
        response_count = 0;

    }
    else
    {
        ESP_LOGI(TAG,"HTTP GET FAILED:%s",esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(post_data);
    free(image_base64);

    return 0;
}

int figer_question(const unsigned char *image_src, unsigned int image_len)
{
    char* image_base64;
    int len;
    char *post_data;
    size_t out_len;
    uint8_t chipid[6];

    image_base64 = base64_encode(image_src,image_len,&out_len);

    if(image_base64 == NULL)
    {
        ESP_LOGI(TAG,"base64 error");
        return -1;
    }

    post_data = (char *) malloc(out_len + 150);
    if(post_data != NULL)
    {
        ESP_LOGI(TAG,"malloc %d bytes mem success",out_len + 150);
    }
    else{
        ESP_LOGI(TAG,"malloc %d bytes mem fail",out_len + 150);
        return -2;
    }

    esp_efuse_mac_get_default(&chipid);

    sprintf(post_data,"{\"img_base64\":\"%s\",\"uuid\":\"%02x%02x%02x%02x%02x%02x\"}",image_base64,chipid[0],chipid[1],chipid[2],chipid[3],chipid[4],chipid[5]);

   // ESP_LOGI(TAG,"%d post_data:%s",strlen(post_data),post_data);

    const char *iamge_post_url = "http://yida.qjun.com.cn:30001/searchQuestion";
    esp_http_client_config_t config = {
            .url = iamge_post_url,
            .event_handler = _http_event_handle,
            .method = HTTP_METHOD_POST,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);

    if(err == ESP_OK)
    {
        len = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG,"HTTP GET STATUS = %d,len = %d",esp_http_client_get_status_code(client),len);
        ESP_LOGI(TAG,"%s",local_response_buffer);
        memset(local_response_buffer,0x0,MAX_HTTP_OUTPUT_BUFFER);
        response_count = 0;

    }
    else
    {
        ESP_LOGI(TAG,"HTTP GET FAILED:%s",esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(post_data);
    free(image_base64);

    return 0;
}

int figer_point_to_server(const unsigned char *image_src, unsigned int image_len)
{
    char* image_base64;
    int len;
    char *post_data;
    size_t out_len;
    uint8_t chipid[6];

    image_base64 = base64_encode(image_src,image_len,&out_len);

    if(image_base64 == NULL)
    {
        ESP_LOGI(TAG,"base64 error");
        return -1;
    }

    post_data = (char *) malloc(out_len + 150);
    if(post_data != NULL)
    {
        ESP_LOGI(TAG,"malloc %d bytes mem success",out_len + 150);
    }
    else{
        ESP_LOGI(TAG,"malloc %d bytes mem fail",out_len + 150);
        return -2;
    }

    esp_efuse_mac_get_default(&chipid);

    sprintf(post_data,"{\"img_base64\":\"%s\",\"uuid\":\"%02x%02x%02x%02x%02x%02x\"}",image_base64,chipid[0],chipid[1],chipid[2],chipid[3],chipid[4],chipid[5]);

   // ESP_LOGI(TAG,"%d post_data:%s",strlen(post_data),post_data);

    const char *iamge_post_url = "http://yida.qjun.com.cn:30002/getFigerPoint2Server";
    esp_http_client_config_t config = {
            .url = iamge_post_url,
            .event_handler = _http_event_handle,
            .method = HTTP_METHOD_POST,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);

    if(err == ESP_OK)
    {
        len = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG,"HTTP GET STATUS = %d,len = %d",esp_http_client_get_status_code(client),len);
        ESP_LOGI(TAG,"%s",local_response_buffer);
        memset(local_response_buffer,0x0,MAX_HTTP_OUTPUT_BUFFER);
        response_count = 0;

    }
    else
    {
        ESP_LOGI(TAG,"HTTP GET FAILED:%s",esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(post_data);
    free(image_base64);

    return 0;
}

int sned_picture(const unsigned char *image_src, unsigned int image_len)
{
    char* image_base64;
    int len;
    char *post_data;
    size_t out_len;
    uint8_t chipid[6];

    image_base64 = base64_encode(image_src,image_len,&out_len);

    if(image_base64 == NULL)
    {
        ESP_LOGI(TAG,"base64 error");
        return -1;
    }

    post_data = (char *) malloc(out_len + 150);
    if(post_data != NULL)
    {
        ESP_LOGI(TAG,"malloc %d bytes mem success",out_len + 150);
    }
    else{
        ESP_LOGI(TAG,"malloc %d bytes mem fail",out_len + 150);
        return -2;
    }

    esp_efuse_mac_get_default(&chipid);

    sprintf(post_data,"{\"img_base64\":\"%s\",\"uuid\":\"%02x%02x%02x%02x%02x%02x\"}",image_base64,chipid[0],chipid[1],chipid[2],chipid[3],chipid[4],chipid[5]);

   // ESP_LOGI(TAG,"%d post_data:%s",strlen(post_data),post_data);

    const char *iamge_post_url = "http://yida.qjun.com.cn:30002/sendPicture";
    esp_http_client_config_t config = {
            .url = iamge_post_url,
            .event_handler = _http_event_handle,
            .method = HTTP_METHOD_POST,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);

    if(err == ESP_OK)
    {
        len = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG,"HTTP GET STATUS = %d,len = %d",esp_http_client_get_status_code(client),len);
        ESP_LOGI(TAG,"%s",local_response_buffer);
        memset(local_response_buffer,0x0,MAX_HTTP_OUTPUT_BUFFER);
        response_count = 0;

    }
    else
    {
        ESP_LOGI(TAG,"HTTP GET FAILED:%s",esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(post_data);
    free(image_base64);

    return 0;
}


int send_device_message(void/*const unsigned char *image_src, unsigned int image_len*/)
{
#if 0
    char* image_base64;
    int len;
    char *post_data;
    size_t out_len = 5;
    uint8_t chipid[6];
    esp_err_t err;
    char output_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
/*
    image_base64 = base64_encode(image_src,image_len,&out_len);

    if(image_base64 == NULL)
    {
        ESP_LOGI(TAG,"base64 error");
        return -1;
    }
    */

    post_data = (char *) malloc(150);
    if(post_data != NULL)
    {
        ESP_LOGI(TAG,"malloc %d bytes mem success",out_len + 150);
    }
    else{
        ESP_LOGI(TAG,"malloc %d bytes mem fail",out_len + 150);
        return -2;
    }

    esp_efuse_mac_get_default(&chipid);

    sprintf(post_data,"{\"deviceId\":\"1234\",\"message\":\"555\"}");

    const char *iamge_post_url = "http://yida.qjun.com.cn/api/app/device/send";
    esp_http_client_config_t config = {
            .url = iamge_post_url,
     //       .event_handler = _http_event_handle,
            .method = HTTP_METHOD_POST,
        //    .is_async = true,
        //    .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    err = esp_http_client_open(client, strlen(post_data));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    } else {
        int wlen = esp_http_client_write(client, post_data, strlen(post_data));
        if (wlen < 0) {
            ESP_LOGE(TAG, "Write failed");
        }
        int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_OUTPUT_BUFFER);
        if (data_read >= 0) {
            ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
            esp_http_client_get_status_code(client),
            esp_http_client_get_content_length(client));
            ESP_LOGI(TAG,"%s",output_buffer);
        } else {
            ESP_LOGE(TAG, "Failed to read response");
        }
    }



    esp_http_client_cleanup(client);
    free(post_data);
 //   free(image_base64);

    return 0;
#endif

   char output_buffer[700] = {0};   // Buffer to store response of http request
    int content_length = 0;
    esp_http_client_config_t config = {
        .url = "http://httpbin.org/get",
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // GET Request
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    } else {
        content_length = esp_http_client_fetch_headers(client);
        if (content_length < 0) {
            ESP_LOGE(TAG, "HTTP client fetch headers failed");
        } else {
            int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_OUTPUT_BUFFER);
            if (data_read >= 0) {
                ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
                ESP_LOG_BUFFER_HEX(TAG, output_buffer, strlen(output_buffer));
            //    ESP_LOGE(TAG,"%s",output_buffer);
            } else {
                ESP_LOGE(TAG, "Failed to read response");
            }
        }
    }
    esp_http_client_close(client);

    // POST Request
    const char *post_data = "{\"field1\":\"value1\"}";
    esp_http_client_set_url(client, "http://httpbin.org/post");
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    ESP_LOGI(TAG,"1----%d",xTaskGetTickCount());
    err = esp_http_client_open(client, strlen(post_data));
    ESP_LOGI(TAG,"2----%d",xTaskGetTickCount());
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    } else {
        int wlen = esp_http_client_write(client, post_data, strlen(post_data));
        ESP_LOGI(TAG,"3----%d",xTaskGetTickCount());
        if (wlen < 0) {
            ESP_LOGE(TAG, "Write failed");
        }
        int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_OUTPUT_BUFFER);
        ESP_LOGI(TAG,"4----%d",xTaskGetTickCount());
        if (data_read >= 0) {
            ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
            esp_http_client_get_status_code(client),
            esp_http_client_get_content_length(client));
            ESP_LOG_BUFFER_HEX(TAG, output_buffer, strlen(output_buffer));
            ESP_LOGI(TAG,"%s",output_buffer);
        } else {
            ESP_LOGE(TAG, "Failed to read response");
        }
    }
    esp_http_client_cleanup(client);

    return 0;


}

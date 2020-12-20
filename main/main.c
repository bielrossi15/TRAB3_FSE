#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/semphr.h"
#include "esp_http_client.h"
#include "esp_log.h"


// my own files
#include "wifi.h"
#include "http_client.h"
#include "led.h"

#define BUFFER_SIZE 1024

#define KEY_1 CONFIG_ESP_IPSTACK_KEY
#define KEY_2 CONFIG_ESP_OPEN_WEATHER_KEY

void http_handler(void * params);
void led_handler(void * params);
void blink();
char * ipstack_url();
char * ow_url(double lat, double lng);

int connected = 0;
char *buf;
char url_1[200];
char url_2[200];
int size = 0;

xSemaphoreHandle wifiSem;
xSemaphoreHandle httpSem;
xSemaphoreHandle ledSem;

void app_main(void)
{
    init_led();
    ledSem = xSemaphoreCreateBinary();
    xSemaphoreGive(ledSem);
    xTaskCreate(&led_handler,  "Led handler", 4096, NULL, 1, NULL);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    
    wifiSem = xSemaphoreCreateBinary();
    httpSem = xSemaphoreCreateBinary();
    wifi_start();

    xTaskCreate(&http_handler,  "Processa HTTP", 4096*2, NULL, 1, NULL);
}

void http_handler(void * params)
{
  while(true)
  {
    if(xSemaphoreTake(wifiSem, portMAX_DELAY))
    {
        blink();
    

        ESP_LOGI("Main Task", "HTTP Request");

        // http request to ipstack
        http_request(ipstack_url());

        // parsing response from ipstack
        buf[size] = '\0';
        cJSON * json = cJSON_Parse (buf);
        double lat = cJSON_GetObjectItemCaseSensitive(json, "latitude")->valuedouble;
        double lng = cJSON_GetObjectItemCaseSensitive(json, "longitude")->valuedouble;
        cJSON_Delete(json);
        
        free(buf);
        buf = NULL;
        size = 0;
        
        // http request to openweather
        http_request(ow_url(lat, lng));

        // parsing response from openweather
        buf[size] = '\0';
        cJSON * json2 = cJSON_Parse (buf);
        cJSON * mainwt = cJSON_GetObjectItemCaseSensitive(json2, "main");
        double tempt = cJSON_GetObjectItemCaseSensitive(mainwt, "temp")->valuedouble;
        double min_temp = cJSON_GetObjectItemCaseSensitive(mainwt, "temp_min")->valuedouble;
        double max_temp = cJSON_GetObjectItemCaseSensitive(mainwt, "temp_max")->valuedouble;
        int hmdt = cJSON_GetObjectItemCaseSensitive(mainwt, "hmdt")->valueint;
        
        cJSON_Delete(json2);
        
        printf("Temperatura atual: %.2lf\n", tempt - 273);
        printf("Maxima prevista: %.2lf\n", max_temp - 273);
        printf("Minima prevista: %.2lf\n", min_temp - 273);
        printf("Humidade: %d %%\n", hmdt);
        
        free(buf);
        buf = NULL;
        size = 0;
        xSemaphoreGive(wifiSem);
      }

      vTaskDelay(1000 * 5 * 60 / portTICK_PERIOD_MS);

    }  
}

void led_handler(void * params)
{
  while(true)
  {
    if(xSemaphoreTake(ledSem, portMAX_DELAY))
    {
        if(connected)
        {
            set_led_state(1);
        }
        else
        {
            blink();
        }
    }
    xSemaphoreGive(ledSem);
  }
}

void blink()
{
    set_led_state(1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    set_led_state(0);
    vTaskDelay(500 / portTICK_PERIOD_MS);   
}

char * ipstack_url()
{
    sprintf(url_1, "http://api.ipstack.com/45.4.43.136?access_key=%s", KEY_1);
    return url_1;

}

char * ow_url(double lat, double lng)
{
    sprintf(url_2, "http://api.openweathermap.org/data/2.5/weather?lat=%lf&lon=%lf&appid=%s", lat, lng, KEY_2);
    return url_2;
}

#endif
#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <cJSON.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_sntp.h>
#include <string.h>
#include <string>
#include <stdio.h> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_crt_bundle.h"
#include <stdlib.h>
#include "esp_err.h"
#include <sys/param.h>
#include "freertos/timers.h"
#include "freertos/event_groups.h"


#include "CPPSPI/cppspi.h"
#include "bme280_spi/bme280_spi.h"


//TODO::Assign correct pins
constexpr static int SPI_3_MISO = 37;
constexpr static int SPI_3_MOSI = 35;
constexpr static int SPI_3_SCLK = 36;
constexpr static int BME280_SS_PIN = 41;

CPPSPI::Spi spi3;

#define DATABASE_URL "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"  // replace with the database URL

const char *TAG = "Firebase";

const char *ntpServer = "pool.ntp.org";

// Firebase variables
std::string id_token;
std::string firebase_id;

static char *response_buffer = NULL;
static int response_len = 0;

//TODO::
#define SSID "xxxxxxxxxxxx"  // replace with SSID
#define pass "xxxxxxxxxxxx"  // replace with password of the network


int retry_num=0;

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,void *event_data){
if(event_id == WIFI_EVENT_STA_START)
{
  printf("WIFI CONNECTING....\n");
}
else if (event_id == WIFI_EVENT_STA_CONNECTED)
{
  printf("WiFi CONNECTED\n");
}
else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
{
  printf("WiFi lost connection\n");
  if(retry_num<5){esp_wifi_connect();retry_num++;printf("Retrying to Connect...\n");}
}
else if (event_id == IP_EVENT_STA_GOT_IP)
{
  printf("Wifi got IP...\n\n");
}
}

void wifi_connection()
{
    esp_netif_init();
    esp_event_loop_create_default();     // event loop                    s1.2
    esp_netif_create_default_wifi_sta(); // WiFi station                      s1.3
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation); //     
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {};

    strcpy((char*)wifi_configuration.sta.ssid, SSID);
    strcpy((char*)wifi_configuration.sta.password, pass);    
    //esp_log_write(ESP_LOG_INFO, "Kconfig", "SSID=%s, PASS=%s", ssid, pass);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_configuration);
    // 3 - Wi-Fi Start Phase
    esp_wifi_start();
    esp_wifi_set_mode(WIFI_MODE_STA);
    // 4- Wi-Fi Connect Phase
    esp_wifi_connect();
    printf( "wifi_init_softap finished. SSID:%s  password:%s",SSID,pass);
    
}


void initialize_sntp() {
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setservername(0, ntpServer);
    esp_sntp_init();

    while (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
        ESP_LOGI(TAG, "Waiting for system time to be set...");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "SNTP synchronized");
}


// Client
esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
        case HTTP_EVENT_ON_DATA: {
            // Check if the response is chunked
            if (esp_http_client_is_chunked_response(evt->client)) {
                // Allocate or reallocate buffer to hold the accumulating response
                response_buffer = (char *)realloc(response_buffer, response_len + evt->data_len + 1);
                if (response_buffer == NULL) {
                    ESP_LOGE("HTTP_EVENT", "Failed to allocate memory for chunked response");
                    return 0;
                }
                // Copy the new chunk into the buffer
                memcpy(response_buffer + response_len, evt->data, evt->data_len);
                response_len += evt->data_len;
                response_buffer[response_len] = '\0';  // Null-terminate the buffer
            } else {
                // Handle non-chunked response (as previously implemented)
                char *data = (char *)malloc(evt->data_len + 1);
                if (data == NULL) {
                    ESP_LOGE("JSON_PARSE", "Failed to allocate memory");
                    return 0;
                }
                memcpy(data, evt->data, evt->data_len);
                data[evt->data_len] = '\0';  // Null-terminate the string

                cJSON *json = cJSON_Parse(data);
                if (json == NULL) {
                    ESP_LOGE("JSON_PARSE", "Failed to parse JSON");
                    free(data);
                    return 0;
                }

                // Extract idToken and localId as before
                cJSON *id_token_json = cJSON_GetObjectItemCaseSensitive(json, "idToken");
                if (cJSON_IsString(id_token_json) && (id_token_json->valuestring != NULL)) {
                    ESP_LOGI("JSON_PARSE", "idToken: %s", id_token_json->valuestring);
                    //id_token = id_token_json->valuestring;
                    id_token = "";
                } else {
                    ESP_LOGE("JSON_PARSE", "idToken not found in JSON");
                }

                cJSON *local_id_json = cJSON_GetObjectItemCaseSensitive(json, "localId");
                if (cJSON_IsString(local_id_json) && (local_id_json->valuestring != NULL)) {
                    ESP_LOGI("JSON_PARSE", "localId: %s", local_id_json->valuestring);
                    firebase_id = local_id_json->valuestring;
                } else {
                    ESP_LOGE("JSON_PARSE", "localId not found in JSON");
                }

                // Cleanup
                cJSON_Delete(json);
                free(data);
            }
            break;
        }

        case HTTP_EVENT_ON_FINISH:{
            if (response_buffer != NULL) {
                // Parse the accumulated response once the transfer is complete
                cJSON *json = cJSON_Parse(response_buffer);
                if (json == NULL) {
                    ESP_LOGE("JSON_PARSE", "Failed to parse JSON");
                } else {
                    // Extract idToken and localId as before
                    cJSON *id_token_json = cJSON_GetObjectItemCaseSensitive(json, "idToken");
                    if (cJSON_IsString(id_token_json) && (id_token_json->valuestring != NULL)) {
                        ESP_LOGI("JSON_PARSE", "idToken: %s", id_token_json->valuestring);
                        //id_token = id_token_json->valuestring;
                        id_token = "";
                    } else {
                        ESP_LOGE("JSON_PARSE", "idToken not found in JSON");
                    }

                    cJSON *local_id_json = cJSON_GetObjectItemCaseSensitive(json, "localId");
                    if (cJSON_IsString(local_id_json) && (local_id_json->valuestring != NULL)) {
                        ESP_LOGI("JSON_PARSE", "localId: %s", local_id_json->valuestring);
                        firebase_id = local_id_json->valuestring;
                    } else {
                        ESP_LOGE("JSON_PARSE", "localId not found in JSON");
                    }

                    cJSON_Delete(json);
                }

                // Cleanup
                free(response_buffer);
                response_buffer = NULL;
                response_len = 0;
            }
            break;
        }
    default:
        break;
    }
    return ESP_OK;
}



static void client_post_rest_function()
{
    esp_http_client_config_t config_post = {
        .url = "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",//TODO:: //Note:: API_KEY is here, after =
        .method = HTTP_METHOD_POST,
        .event_handler = client_event_get_handler,
        .skip_cert_common_name_check = true,
    };
        
    esp_http_client_handle_t client = esp_http_client_init(&config_post);

    char  *post_data = "{\"email\":\"xxxxxxxxxxxxxxxxx\",\"password\":\"xxxxxxxxxxxxxxxx\",\"returnSecureToken\":true}";//TODO:: replace with email address and the password
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}

void send_data_to_firebase(float temperature, float pressure, int humidity) {
    char url[1280];
    snprintf(url, sizeof(url), "%sUsersData/%s/readings.json?auth=%s", DATABASE_URL, firebase_id.c_str(),id_token.c_str());

    cJSON *json_data = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_data, "temperature", temperature);
    cJSON_AddNumberToObject(json_data, "pressure", pressure);
    cJSON_AddNumberToObject(json_data, "humidity", humidity);
    cJSON_AddNumberToObject(json_data, "timestamp", time(NULL));

    char *post_str = cJSON_PrintUnformatted(json_data);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .skip_cert_common_name_check = false,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_post_field(client, post_str, strlen(post_str));
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Data sent to Firebase successfully. Status Code: %d", status_code);
        char *response = (char *)malloc(esp_http_client_get_content_length(client) + 1);
        int len = esp_http_client_read(client, response, esp_http_client_get_content_length(client));
        response[len] = '\0';
        ESP_LOGI(TAG, "Response: %s", response);
        free(response);
    } else {
        ESP_LOGE(TAG, "Failed to send data to Firebase: %s", esp_err_to_name(err));
    }

    cJSON_Delete(json_data);
    free(post_str);
    esp_http_client_cleanup(client);
}


extern "C" void app_main(void)
{


    esp_netif_init();
    esp_event_loop_create_default();

            esp_log_level_set("*", ESP_LOG_INFO);
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    nvs_flash_init();
    wifi_connection();
    initialize_sntp();

    client_post_rest_function();


    CPPBME280::BME280SPI bme280spi;
    spi3.Init(SPI3_HOST, SPI_3_MISO, SPI_3_MOSI, SPI_3_SCLK);
    bme280spi.InitSpi(&spi3, BME280_SS_PIN);
    bme280spi.Init();
    bme280spi.SetMode(1);
    bme280spi.SetConfigFilter(1);

    while(true)
    {
        float temperature = (int)(bme280spi.GetTemperature()*100+0.5)/100;
        float pressure = (int)(bme280spi.GetPressure()*100+0.5)/100;
        int humidity = bme280spi.GetHumidity();

        send_data_to_firebase(temperature, pressure, humidity);

        vTaskDelay(pdMS_TO_TICKS(3000));  // Send every 3 seconds
    }
    // float temperature = (int)(bme280spi.GetTemperature()*100+0.5)/100;
    // float pressure = (int)(bme280spi.GetPressure()*100+0.5)/100;
    // int humidity = bme280spi.GetHumidity();

    // send_data_to_firebase(temperature, pressure, humidity);

}


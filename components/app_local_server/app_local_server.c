/**
 * @file app_local_server.c
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "lwip/inet.h"
#include "esp_ota_ops.h"
#include <cJSON.h>
#include "nvs_storage.h"
#include "app_local_server.h"
#include "dns_server.h"
#include "rfid_manager.h"

#define URI_HANDLER_MARGIN (1u) // Margin for the URI Handlers
#define URI_HANDLERS_COUNT (sizeof(uri_handlers) / sizeof(uri_handlers[0]))

#define HTTP_SERVER_MAX_URI_HANDLERS (20u)
#define HTTP_SERVER_RECEIVE_WAIT_TIMEOUT (10u) // in seconds
#define HTTP_SERVER_SEND_WAIT_TIMEOUT (10u)    // in seconds
#define HTTP_SERVER_MONITOR_QUEUE_LEN (3u)
#define HTTP_SERVER_BUFFER_SIZE (3 * 1024) // 3KB buffer size

#define OTA_UPDATE_PENDING (0)
#define OTA_UPDATE_SUCCESSFUL (1)
#define OTA_UPDATE_FAILED (-1)

// GLOBAL VARIABLES
static const char *TAG = "app_local_server";
//  Embedded Files: JQuery, index.html, app.css, app.js, and favicon.ico files
extern const char jquery_3_3_1_min_js_start[] asm("_binary_jquery_3_3_1_min_js_start");
extern const char jquery_3_3_1_min_js_end[] asm("_binary_jquery_3_3_1_min_js_end");
extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[] asm("_binary_index_html_end");
extern const char app_css_start[] asm("_binary_app_css_start");
extern const char app_css_end[] asm("_binary_app_css_end");
extern const char app_js_start[] asm("_binary_app_js_start");
extern const char app_js_end[] asm("_binary_app_js_end");
extern const char favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const char favicon_ico_end[] asm("_binary_favicon_ico_end");
extern const char root_start[] asm("_binary_root_html_start");
extern const char root_end[] asm("_binary_root_html_end");

static httpd_handle_t http_server_handle = NULL;
// Queue Handle used to manipulate the main queue of events
static QueueHandle_t http_server_monitor_q_handle;
// Firmware Update Status
static int fw_update_status = OTA_UPDATE_PENDING;
// Local Time Status
static bool g_is_local_time_set = false;
static char http_server_buffer[HTTP_SERVER_BUFFER_SIZE] = {0};

// ESP32 Timer Configuration Passed to esp_timer_create
static const esp_timer_create_args_t fw_update_reset_args =
    {
        .callback = &http_server_fw_update_reset_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "fw_update_reset"};
esp_timer_handle_t fw_update_reset;

static http_server_wifi_connect_status_e g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_NONE;

// FUNCTION PROTOTYPES
static BaseType_t http_server_monitor_send_msg(http_server_msg_e msg_id);
static void http_server_monitor(void);
static void start_webserver(void);
static void http_server_fw_update_reset_timer(void);
static esp_err_t http_server_j_query_handler(httpd_req_t *req);
static esp_err_t http_server_index_html_handler(httpd_req_t *req);
static esp_err_t http_server_app_css_handler(httpd_req_t *req);
static esp_err_t http_server_app_js_handler(httpd_req_t *req);
static esp_err_t http_server_favicon_handler(httpd_req_t *req);
static esp_err_t http_server_ota_update_handler(httpd_req_t *req);
static esp_err_t http_server_ota_status_handler(httpd_req_t *req);
static esp_err_t http_server_get_ssid_handler(httpd_req_t *req);
static esp_err_t http_server_time_handler(httpd_req_t *req);
static esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err);
static void get_local_time_string_utc(char *time_str, size_t len);
static void get_local_time_string(char *time_str, size_t len);
static esp_err_t http_server_get_sensor_data_handler(httpd_req_t *req);
static esp_err_t http_server_get_data_handler(httpd_req_t *req);
static int16_t get_humidity(void);
static int16_t get_temperature(void);
bool get_data_rsp_string(char *key, char *buffer, uint16_t len);
static esp_err_t http_server_wifi_connect_handler(httpd_req_t *req);
static esp_err_t http_server_rfid_manager_list_cards_handler(httpd_req_t *req);
static esp_err_t http_server_rfid_manager_add_card_handler(httpd_req_t *req);
static esp_err_t http_server_rfid_manager_remove_card_handler(httpd_req_t *req);
static esp_err_t http_server_rfid_manager_get_card_count_handler(httpd_req_t *req);
static esp_err_t http_server_rfid_manager_check_card_handler(httpd_req_t *req);
static esp_err_t http_server_rfid_manager_reset_cards_handler(httpd_req_t *req);

static const httpd_uri_t uri_handlers[] = {
    {"/jquery-3.3.1.min.js", HTTP_GET, http_server_j_query_handler, NULL},
    {"/", HTTP_GET, http_server_index_html_handler, NULL},
    {"/app.css", HTTP_GET, http_server_app_css_handler, NULL},
    {"/app.js", HTTP_GET, http_server_app_js_handler, NULL},
    {"/favicon.ico", HTTP_GET, http_server_favicon_handler, NULL},
    {"/OTAupdate", HTTP_POST, http_server_ota_update_handler, NULL},
    {"/OTAstatus", HTTP_POST, http_server_ota_status_handler, NULL},
    {"/apSSID", HTTP_GET, http_server_get_ssid_handler, NULL},
    {"/localTime", HTTP_GET, http_server_time_handler, NULL},
    {"/Sensor", HTTP_GET, http_server_get_sensor_data_handler, NULL},
    {"/getData", HTTP_POST, http_server_get_data_handler, NULL},
    {"/wifiConnect", HTTP_POST, http_server_wifi_connect_handler, NULL},
    // RFID Manager Handlers
    {"/cards/get", HTTP_GET, http_server_rfid_manager_list_cards_handler, NULL},
    {"/cards/add", HTTP_POST, http_server_rfid_manager_add_card_handler, NULL},
    {"/cards/remove", HTTP_DELETE, http_server_rfid_manager_remove_card_handler, NULL},
    {"/cards/count", HTTP_GET, http_server_rfid_manager_get_card_count_handler, NULL},
    {"/cards/check", HTTP_GET, http_server_rfid_manager_check_card_handler, NULL},
    {"/cards/reset", HTTP_POST, http_server_rfid_manager_reset_cards_handler, NULL},

};

// FUNCTIONS
bool app_local_server_init(void)
{
    // create a message queue
    http_server_monitor_q_handle = xQueueCreate(HTTP_SERVER_MONITOR_QUEUE_LEN,
                                                sizeof(http_server_q_msg_t));

    return true;
}

bool app_local_server_start(void)
{
    // Start the web server
    start_webserver();
    start_dns_server();

    return true;
}

bool app_local_server_process(void)
{
    // Process the HTTP Server Monitor Task
    http_server_monitor();

    return true;
}

/*
 * Timer Callback function which calls esp_restart function upon successful
 * firmware update
 */
void http_server_fw_update_reset_cb(void *arg)
{
    ESP_LOGI(TAG, "http_fw_update_reset_cb: Timer timed-out, restarting the device");
    esp_restart();
}

/*
 * Sends a message to the Queue
 * @param msg_id Message ID from the http_server_msg_e enum
 * @return pdTRUE if an item was successfully sent to the queue, otherwise pdFALSE
 */
static BaseType_t http_server_monitor_send_msg(http_server_msg_e msg_id)
{
    http_server_q_msg_t msg;
    msg.msg_id = msg_id;
    return xQueueSend(http_server_monitor_q_handle, &msg, portMAX_DELAY);
}

/*
 * HTTP Server Monitor Task used to track events of the HTTP Server.
 * @param pvParameter parameters which can be passed to the task
 * @return http server instance handle if successful, NULL otherwise
 */
static void http_server_monitor(void)
{
    http_server_q_msg_t msg;

    if (xQueueReceive(http_server_monitor_q_handle, &msg, portMAX_DELAY))
    {
        switch (msg.msg_id)
        {
        case HTTP_MSG_WIFI_CONNECT_INIT:
            ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_INIT");
            g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECTING;
            break;
        case HTTP_MSG_WIFI_CONNECT_SUCCESS:
            ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_SUCCESS");
            g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_SUCCESS;
            break;
        case HTTP_MSG_WIFI_CONNECT_FAIL:
            ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_FAIL");
            g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_FAILED;
            break;
        case HTTP_MSG_WIFI_USER_DISCONNECT:
            ESP_LOGI(TAG, "HTTP_MSG_WIFI_USER_DISCONNECT");
            g_wifi_connect_status = HTTP_WIFI_STATUS_DISCONNECTED;
            break;
        case HTTP_MSG_WIFI_OTA_UPDATE_SUCCESSFUL:
            ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_SUCCESSFUL");
            fw_update_status = OTA_UPDATE_SUCCESSFUL;
            http_server_fw_update_reset_timer();
            break;
        case HTTP_MSG_WIFI_OTA_UPDATE_FAILED:
            ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_FAILED");
            fw_update_status = OTA_UPDATE_FAILED;
            break;
        case HTTP_MSG_TIME_SERVICE_INITIALIZED:
            ESP_LOGI(TAG, "HTTP_MSG_TIME_SERVICE_INITIALIZED");
            g_is_local_time_set = true;
            break;
        default:
            break;
        }
    }
}

static void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = URI_HANDLERS_COUNT + URI_HANDLER_MARGIN;
    config.max_open_sockets = 13;
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting on port: '%d'", config.server_port);
    if (httpd_start(&http_server_handle, &config) == ESP_OK)
    {
        // Set URI handlers
        for (size_t i = 0; i < URI_HANDLERS_COUNT; i++)
        {
            ESP_LOGI(TAG, "Registering URI handler: %s", uri_handlers[i].uri);
            httpd_register_uri_handler(http_server_handle, &uri_handlers[i]);
        }
        httpd_register_err_handler(http_server_handle, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to start HTTP server");
    }
}

/*
 * Check the fw_update_status and creates the fw_update_reset time if the
 * fw_update_status is true
 */
static void http_server_fw_update_reset_timer(void)
{
    if (fw_update_status == OTA_UPDATE_SUCCESSFUL)
    {
        ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FW Update successful starting FW update reset timer");
        // Give the web page a chance to receive an acknowledge back and initialize the timer
        ESP_ERROR_CHECK(esp_timer_create(&fw_update_reset_args, &fw_update_reset));
        ESP_ERROR_CHECK(esp_timer_start_once(fw_update_reset, 8 * 1000 * 1000));
    }
    else
    {
        ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FW Update unsuccessful");
    }
}

/*
 * jQuery get handler requested when accessing the web page.
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_j_query_handler(httpd_req_t *req)
{
    esp_err_t error;
    ESP_LOGI(TAG, "JQuery Requested");
    httpd_resp_set_type(req, "application/javascript");
    error = httpd_resp_send(req, (const char *)jquery_3_3_1_min_js_start, jquery_3_3_1_min_js_end - jquery_3_3_1_min_js_start);
    if (error != ESP_OK)
    {
        ESP_LOGI(TAG, "http_server_j_query_handler: Error %d while sending Response", error);
    }
    else
    {
        ESP_LOGI(TAG, "http_server_j_query_handler: Response Sent Successfully");
    }
    return error;
}

/*
 * Send the index HTML page
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_index_html_handler(httpd_req_t *req)
{
    esp_err_t error;
    ESP_LOGI(TAG, "Index HTML Requested");
    httpd_resp_set_type(req, "text/html");
    error = httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
    if (error != ESP_OK)
    {
        ESP_LOGI(TAG, "http_server_index_html_handler: Error %d while sending Response", error);
    }
    else
    {
        ESP_LOGI(TAG, "http_server_index_html_handler: Response Sent Successfully");
    }
    return error;
}

/*
 * app.css get handler is requested when accessing the web page
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_app_css_handler(httpd_req_t *req)
{
    esp_err_t error;
    ESP_LOGI(TAG, "APP CSS Requested");
    httpd_resp_set_type(req, "text/css");
    error = httpd_resp_send(req, (const char *)app_css_start, app_css_end - app_css_start);
    if (error != ESP_OK)
    {
        ESP_LOGI(TAG, "http_server_app_css_handler: Error %d while sending Response", error);
    }
    else
    {
        ESP_LOGI(TAG, "http_server_app_css_handler: Response Sent Successfully");
    }
    return error;
}

/*
 * app.js get handler requested when accessing the web page.
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_app_js_handler(httpd_req_t *req)
{
    esp_err_t error;
    ESP_LOGI(TAG, "APP JS Requested");
    httpd_resp_set_type(req, "application/javascript");
    error = httpd_resp_send(req, (const char *)app_js_start, app_js_end - app_js_start);
    if (error != ESP_OK)
    {
        ESP_LOGI(TAG, "http_server_app_js_handler: Error %d while sending Response", error);
    }
    else
    {
        ESP_LOGI(TAG, "http_server_app_js_handler: Response Sent Successfully");
    }
    return error;
}

/*
 * Sends the .ico file when accessing the web page
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_favicon_handler(httpd_req_t *req)
{
    esp_err_t error;
    ESP_LOGI(TAG, "Favicon.ico Requested");
    httpd_resp_set_type(req, "image/x-icon");
    error = httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_end - favicon_ico_start);
    if (error != ESP_OK)
    {
        ESP_LOGI(TAG, "http_server_favicon_handler: Error %d while sending Response", error);
    }
    else
    {
        ESP_LOGI(TAG, "http_server_favicon_handler: Response Sent Successfully");
    }
    return error;
}

/**
 * @brief Receives the *.bin file via the web page and handles the firmware update
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK, other ESP_FAIL if timeout occurs and the update canot be started
 */
static esp_err_t http_server_ota_update_handler(httpd_req_t *req)
{
    esp_err_t error;
    esp_ota_handle_t ota_handle;
    char ota_buffer[1024];
    int content_len = req->content_len; // total content length
    int content_received = 0;
    int recv_len = 0;
    bool is_req_body_started = false;
    bool flash_successful = false;

    // get the next OTA app partition which should be written with a new firmware
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

    // our ota_buffer is not sufficient to receive all data in a one go
    // hence we will read the data in chunks and write in chunks, read the below
    // mentioned comments for more information
    do
    {
        // The following is the API to read content of data from the HTTP request
        /* This API will read HTTP content data from the HTTP request into the
         * provided buffer. Use content_len provided in the httpd_req_t structure to
         *  know the length of the data to be fetched.
         *  If the content_len is to large for the buffer then the user may have to
         *  make multiple calls to this functions (as done here), each time fetching
         *  buf_len num of bytes (which is ota_buffer length here), while the pointer
         *  to content data is incremented internally by the same number
         *  This function returns
         *  Bytes: Number of bytes read into the buffer successfully
         *  0: Buffer length parameter is zero/connection closed by peer.
         *  HTTPD_SOCK_ERR_INVALID: Invalid Arguments
         *  HTTPD_SOCK_ERR_TIMEOUT: Timeout/Interrupted while calling socket recv()
         *  HTTPD_SOCK_ERR_FAIL: Unrecoverable error while calling socket recv()
         *  Parameters to this function are:
         *  req: The request being responded to
         *  ota_buffer: Pointer to a buffer that the data will be read into
         *  length: length of the buffer which ever is minimum (as we don't want to
         *          read more data which buffer can't handle)
         */
        recv_len = httpd_req_recv(req, ota_buffer, MIN(content_len, sizeof(ota_buffer)));
        // if recv_len is less than zero, it means some problem (but if timeout error, then try again)
        if (recv_len < 0)
        {
            // Check if timeout occur, then we will retry again
            if (recv_len == HTTPD_SOCK_ERR_TIMEOUT)
            {
                ESP_LOGI(TAG, "http_server_ota_update_handler: Socket Timeout");
                continue; // Retry Receiving if Timeout Occurred
            }
            // If there is some other error apart from Timeout, then exit with fail
            ESP_LOGI(TAG, "http_server_ota_update_handler: OTA Other Error, %d", recv_len);
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "http_server_ota_update_handler: OTA RX: %d of %d", content_received, content_len);

        // We are here which means that "recv_len" is positive, now we have to check
        // if this is the first data we are receiving or not, If so, it will have
        // the information in the header that we need
        if (!is_req_body_started)
        {
            is_req_body_started = true;
            // Now we have to identify from where the binary file content is starting
            // this can be done by actually checking the escape characters i.e. \r\n\r\n
            // Get the location of the *.bin file content (remove the web form data)
            // the strstr will return the pointer to the \r\n\r\n in the ota_buffer
            // and then by adding 4 we reach to the start of the binary content/start
            char *body_start_p = strstr(ota_buffer, "\r\n\r\n") + 4u;
            int body_part_len = recv_len - (body_start_p - ota_buffer);
            ESP_LOGI(TAG, "http_server_ota_update_handler: OTA File Size: %d", content_len);
            /*
             * esp_ota_begin function commence an OTA update writing to the specified
             * partition. The specified partition is erased to the specified image
             * size. If the image size is not yet known, OTA_SIZE_UNKNOWN is passed
             * which will cause the entire partition to be erased.
             * On Success this function allocates memory that remains in use until
             * esp_ota_end is called with the return handle.
             */
            error = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
            if (error != ESP_OK)
            {
                ESP_LOGI(TAG, "http_server_ota_update_handler: Error with OTA Begin, Canceling OTA");
                return ESP_FAIL;
            }
            else
            {
                ESP_LOGI(TAG, "http_server_ota_update_handler: Writing to partition subtype %d at offset 0x%lx", update_partition->subtype, update_partition->address);
            }
            /*
             * esp_ota_write function, writes the OTA update to the partition.
             * This function can be called multiple times as data is received during
             * the OTA operation. Data is written sequentially to the partition.
             * Here we are writing the body start for the first time.
             */
            esp_ota_write(ota_handle, body_start_p, body_part_len);
            content_received += body_part_len;
        }
        else
        {
            /* Continue to receive data above using httpd_req_recv function, and write
             * using esp_ota_write (below), until all the content is received. */
            esp_ota_write(ota_handle, ota_buffer, recv_len);
            content_received += recv_len;
        }

    } while ((recv_len > 0) && (content_received < content_len));
    // till complete data is received and written or some error is there we will
    // remain in the above mentioned do-while loop

    /* Finish the OTA update and validate newly written app image.
     * After calling esp_ota_end, the handle is no longer valid and memory associated
     * with it is freed (regardless of the results).
     */
    if (esp_ota_end(ota_handle) == ESP_OK)
    {
        // let's update the partition i.e. configure OTA data for new boot partition
        if (esp_ota_set_boot_partition(update_partition) == ESP_OK)
        {
            const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
            ESP_LOGI(TAG, "http_server_ota_update_handler: Next boot partition subtype %d at offset 0x%lx", boot_partition->subtype, boot_partition->address);
            flash_successful = true;
        }
        else
        {
            ESP_LOGI(TAG, "http_server_ota_update_handler: Flash Error");
        }
    }
    else
    {
        ESP_LOGI(TAG, "http_server_ota_update_handler: esp_ota_end Error");
    }

    // We won't update the global variables throughout the file, so send the message about the status
    if (flash_successful)
    {
        http_server_monitor_send_msg(HTTP_MSG_WIFI_OTA_UPDATE_SUCCESSFUL);
    }
    else
    {
        http_server_monitor_send_msg(HTTP_MSG_WIFI_OTA_UPDATE_FAILED);
    }
    return ESP_OK;
}

/*
 * OTA status handler responds with the firmware update status after the OTA
 * update is started and responds with the compile time & date when the page is
 * first requested
 * @param req HTTP request for which the URI needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_ota_status_handler(httpd_req_t *req)
{
    char ota_JSON[100];
    ESP_LOGI(TAG, "OTA Status Requested");
    sprintf(ota_JSON, "{\"ota_update_status\":%d,\"compile_time\":\"%s\",\"compile_date\":\"%s\"}", fw_update_status, __TIME__, __DATE__);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, ota_JSON, strlen(ota_JSON));

    return ESP_OK;
}

// HTTP Error (404) Handler - Redirects all requests to the root page
static esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    // Set status
    httpd_resp_set_status(req, "302 Temporary Redirect");
    // Redirect to the "/" root directory
    httpd_resp_set_hdr(req, "Location", "/");
    // iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "Redirecting to root");
    return ESP_OK;
}

static esp_err_t http_server_get_ssid_handler(httpd_req_t *req)
{
    char ssid_json[100];
    ESP_LOGI(TAG, "SSID Requested");
    sprintf(ssid_json, "{\"ssid\":\"%s\"}", CONFIG_ESP_WIFI_AP_SSID);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, ssid_json, strlen(ssid_json));

    return ESP_OK;
}

static esp_err_t http_server_time_handler(httpd_req_t *req)
{
    char local_time[64];
    char utc_time[64];
    char time_json[200];
    ESP_LOGI(TAG, "Time Requested");

    get_local_time_string(local_time, sizeof(local_time));
    get_local_time_string_utc(utc_time, sizeof(utc_time));

    snprintf(time_json, sizeof(time_json),
             "{\"local_time\":\"%s\",\"utc_time\":\"%s\"}",
             local_time, utc_time);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, time_json, strlen(time_json));

    return ESP_OK;
}

static void get_local_time_string(char *time_str, size_t len)
{
    time_t now = time(NULL);
    struct tm timeinfo = {0};
    localtime_r(&now, &timeinfo);
    strftime(time_str, len, "%Y-%m-%d %H:%M:%S", &timeinfo);
}

static void get_local_time_string_utc(char *time_str, size_t len)
{
    time_t now = time(NULL);
    struct tm timeinfo = {0};
    gmtime_r(&now, &timeinfo);
    strftime(time_str, len, "%Y-%m-%d %H:%M:%S", &timeinfo);
}

static esp_err_t http_server_get_sensor_data_handler(httpd_req_t *req)
{
    char sensor_data_json[100];
    ESP_LOGI(TAG, "Sensor Data Requested");

    // Simulate sensor data retrieval
    int temperature = 25;
    int humidity = 60;

    sprintf(sensor_data_json, "{\"temp\":%d,\"humidity\":%d}", temperature, humidity);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, sensor_data_json, strlen(sensor_data_json));

    return ESP_OK;
}

static esp_err_t http_server_get_data_handler(httpd_req_t *req)
{
    bool isComma = false;
    int32_t length = 0;
    uint16_t rsp_len = 0;
    char temp_buff[256] = {0};
    ESP_LOGI(TAG, "Parameters Request Received");

    // Read request content
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0)
    {
        ESP_LOGE(TAG, "Failed to receive request data");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    // Process parameters (this is a placeholder for actual processing logic)
    ESP_LOGI(TAG, "Received parameters: %s", buf);

    // Parse JSON
    cJSON *json = cJSON_Parse(buf);
    if (json == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse JSON");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Extract the "kay" value
    //
    // { "key": "SSID" } -> { "SSID": "NetworkA" }
    cJSON *key_obj = cJSON_GetObjectItemCaseSensitive(json, "key");
    if (cJSON_IsString(key_obj) && (key_obj->valuestring != NULL))
    {
        // ESP_LOGI(TAG, "Key value: %s", key_obj->valuestring);
        // I am sending comma separated values in key_obj->valuestring like "SSID,Temp,Humidity"
        // extract them based on the comma token, give me code to extract them
        // ',' ASCII value of comma
        // "," const string
        // {"SSID":"NetworkA", "Temp":"25", "Humidity":"60"}
        char *token;

        token = strtok(key_obj->valuestring, ",");

        memset(http_server_buffer, 0, HTTP_SERVER_BUFFER_SIZE);

        length = snprintf(http_server_buffer,
                          HTTP_SERVER_BUFFER_SIZE,
                          "{");

        while (token != NULL)
        {
            ESP_LOGI(TAG, "Key value: %s", token);
            memset(temp_buff, 0, sizeof(temp_buff));
            get_data_rsp_string(token, temp_buff, sizeof(temp_buff));
            length += snprintf(http_server_buffer + length,
                               HTTP_SERVER_BUFFER_SIZE - length,
                               "%s%s",
                               isComma ? "," : "",
                               temp_buff);
            token = strtok(NULL, ",");
            isComma = true;
        }

        length += snprintf(http_server_buffer + length,
                           HTTP_SERVER_BUFFER_SIZE - length,
                           "}");

        ESP_LOGI(TAG, "%s [%ld]: %s", key_obj->valuestring, length, http_server_buffer);
    }
    else
    {
        ESP_LOGE(TAG, "Invalid or missing 'name' key in JSON");
        cJSON_Delete(json);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Clean up JSON object
    cJSON_Delete(json);

    // Send success response
    const char *response = NULL;

    if (length > 0)
    {
        response = http_server_buffer;
        rsp_len = length;
    }
    else
    {
        response = "{\"status\": \"error\"}";
        rsp_len = strlen(response);
    }

    httpd_resp_set_type(req, "application/json");
    esp_err_t error = httpd_resp_send(req, response, rsp_len);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d while sending params response", error);
    }
    else
    {
        ESP_LOGI(TAG, "Params response sent successfully");
    }

    return error;
}

bool get_data_rsp_string(char *key, char *buffer, uint16_t len)
{
    if (!key || !buffer || len == 0)
    {
        ESP_LOGE(TAG, "Buffer is NULL or length is zero");
        return false;
    }

    if (strstr(key, "SSID") != NULL)
    {
        snprintf(buffer, len, "\"SSID\":\"%s\"", CONFIG_ESP_WIFI_SSID);
    }
    else if (strstr(key, "Temp") != NULL)
    {
        snprintf(buffer, len, "\"Temp\":\"%d\"", get_temperature()); // Placeholder for temperature
    }
    else if (strstr(key, "Humidity") != NULL)
    {
        snprintf(buffer, len, "\"Humidity\":\"%d\"", get_humidity()); // Placeholder for humidity
    }
    else if (strstr(key, "UTC") != NULL)
    {
        char time_str[20];
        get_local_time_string_utc(time_str, sizeof(time_str));
        snprintf(buffer, len, "\"UTC\":\"%s\"", time_str);
    }
    else if (strstr(key, "Local") != NULL)
    {
        char time_str[20];
        get_local_time_string(time_str, sizeof(time_str));
        snprintf(buffer, len, "\"Local\":\"%s\"", time_str);
    }
    else if (strstr(key, "CompileTime") != NULL)
    {
        snprintf(buffer, len, "\"CompileTime\":\"%s\"", __TIME__);
    }
    else if (strstr(key, "CompileDate") != NULL)
    {
        snprintf(buffer, len, "\"CompileDate\":\"%s\"", __DATE__);
    }
    else if (strstr(key, "FirmwareVersion") != NULL)
    {
        snprintf(buffer, len, "\"FirmwareVersion\":\"%s\"", "V1.0.0");
    }
    else if (strstr(key, "WiFiStatus") != NULL)
    {
        snprintf(buffer, len, "\"WiFiStatus\":\"%d\"", g_wifi_connect_status);
    }
    else
    {
        snprintf(buffer, len, "\"%s\":\"\"", key); // Default case
    }

    return true;
}

static int16_t get_temperature(void)
{
    // return random number between 0 and 100
    return (rand() % 100);
}

static int16_t get_humidity(void)
{
    // return random number between 0 and 100
    return (rand() % 100);
}

static esp_err_t http_server_wifi_connect_handler(httpd_req_t *req)
{
    bool isComma = false;
    int32_t length = 0;
    uint16_t rsp_len = 0;
    char response[100] = {0};
    char temp_buff[256] = {0};
    ESP_LOGI(TAG, "Parameters Request Received");
    // Read request content
    char buf[256];

    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);

    if (ret <= 0)
    {
        ESP_LOGE(TAG, "Failed to receive request data");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    // Process parameters (this is a placeholder for actual processing logic)
    ESP_LOGI(TAG, "Received parameters: %s", buf);

    // Extract the "ssid" and "password" values
    char ssid[64] = {0};
    char password[64] = {0};

    httpd_req_get_hdr_value_str(req, "my-connect-ssid", ssid, sizeof(ssid));
    httpd_req_get_hdr_value_str(req, "my-connect-pswd", password, sizeof(password));

    rsp_len = snprintf(response, sizeof(response), "{\"ssid\":\"%s\",\"password\":\"%s\"}", ssid, password);
    ESP_LOGI(TAG, "SSID: %s, Password: %s", ssid, password);
    nvs_storage_set_wifi_credentials(ssid, password);

    httpd_resp_set_type(req, "application/json");
    esp_err_t error = httpd_resp_send(req, response, rsp_len);

    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d while sending params response", error);
    }
    else
    {
        ESP_LOGI(TAG, "Params response sent successfully");
    }

    return ESP_OK;
}

static esp_err_t http_server_rfid_manager_list_cards_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "RFID card list requested");

    // Default empty response
    const char *response = "{\"status\":\"error\",\"message\":\"Failed to get RFID cards\",\"cards\":[]}";

    httpd_resp_set_type(req, "application/json");

    // Check if RFID manager is initialized
    if (!rfid_manager_is_database_valid())
    {
        ESP_LOGE(TAG, "RFID database is not valid");
        response = "{\"status\":\"error\",\"message\":\"RFID database is not valid\",\"cards\":[]}";
        httpd_resp_send(req, response, strlen(response));
        return ESP_OK;
    }

    // Try to get the card list as JSON
    esp_err_t result = rfid_manager_get_card_list_json(http_server_buffer, HTTP_SERVER_BUFFER_SIZE);

    if (result == ESP_OK)
    {
        // Success - use the generated JSON
        response = http_server_buffer;
        ESP_LOGI(TAG, "RFID card list generated successfully");
    }
    else if (result == ESP_ERR_NO_MEM)
    {
        // Buffer too small
        ESP_LOGE(TAG, "Buffer too small for RFID card list");
        response = "{\"status\":\"error\",\"message\":\"Buffer too small for RFID card list\",\"cards\":[]}";
    }
    else
    {
        // Other error
        ESP_LOGE(TAG, "Failed to get RFID card list: %s", esp_err_to_name(result));
        char error_msg[100];
        snprintf(error_msg, sizeof(error_msg),
                 "{\"status\":\"error\",\"message\":\"Failed to get RFID card list: %s\",\"cards\":[]}",
                 esp_err_to_name(result));
        response = error_msg;
    }

    // Send the response
    esp_err_t error = httpd_resp_send(req, response, strlen(response));

    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d while sending RFID cards response", error);
        return error;
    }

    ESP_LOGI(TAG, "RFID cards response sent successfully");
    return ESP_OK;
}

static esp_err_t http_server_rfid_manager_add_card_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "RFID card add requested");

    // Read the request body
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0)
    {
        ESP_LOGE(TAG, "Failed to receive request data");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    // Parse the JSON request
    cJSON *json = cJSON_Parse(buf);
    if (json == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse JSON");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Extract the card ID
    cJSON *card_id_obj = cJSON_GetObjectItemCaseSensitive(json, "id");
    cJSON *card_name_obj = cJSON_GetObjectItemCaseSensitive(json, "nm");

    if (!cJSON_IsNumber(card_id_obj) || !(cJSON_IsString(card_name_obj)))
    {
        ESP_LOGE(TAG, "Invalid or missing 'card_id' key in JSON");
        cJSON_Delete(json);
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "{\"status\":\"error\",\"message\":\"Invalid or missing card_id\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    const char *card_id_str = card_id_obj->valuestring;

    // Convert string to uint32_t
    //    uint32_t card_id = (uint32_t)strtoul(card_id_str, NULL, 10);
    uint32_t card_id = (uint32_t)card_id_obj->valueint;
    if (card_id == 0)
    {
        ESP_LOGE(TAG, "Invalid card ID: %lu", card_id);
        cJSON_Delete(json);
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "{\"status\":\"error\",\"message\":\"Invalid card_id\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    // Add the card to the RFID manager
    esp_err_t result = rfid_manager_add_card(card_id,
                                             card_name_obj->valuestring);

    // Clean up JSON object
    cJSON_Delete(json);

    if (result == ESP_OK)
    {
        ESP_LOGI(TAG, "RFID card added successfully: %lu", card_id);
        httpd_resp_set_status(req, "201 Created");
        httpd_resp_send(req, "{\"status\":\"success\",\"message\":\"Card added successfully\"}", HTTPD_RESP_USE_STRLEN);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to add RFID card: %s", esp_err_to_name(result));
        httpd_resp_send_500(req);
    }

    return ESP_OK;
}

static esp_err_t http_server_rfid_manager_remove_card_handler(httpd_req_t *req)
{
    char urlBuffer[256];
    uint16_t lengthOfURI = 0;
    char idStrBuffer[48];
    uint32_t cardId = 0;

    ESP_LOGI(TAG, "/api/rfid/cards/{id} (DELETE) requested: %s", req->uri);

    memset(idStrBuffer, 0, sizeof(idStrBuffer));

    lengthOfURI = httpd_req_get_url_query_len(req) + 1; // +1 for null terminator

    if (lengthOfURI > 1)
    {
        if (httpd_req_get_url_query_str(req, urlBuffer, lengthOfURI) == ESP_OK)
        {
            ESP_LOGI(TAG, "urlBuffer:%s", urlBuffer);

            if (httpd_query_key_value(urlBuffer, "id", idStrBuffer, sizeof(idStrBuffer)) == ESP_OK)
            {
                ESP_LOGI(TAG, "idStrBuffer:%s", idStrBuffer);

                cardId = strtoul(idStrBuffer, NULL, 10);

                if (cardId == 0)
                {
                    ESP_LOGE(TAG, "Card ID missing in URI");
                    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Card ID missing in URI");
                    return ESP_FAIL;
                }
            }
        }
    }

    esp_err_t ret = rfid_manager_remove_card(cardId);

    if (ret == ESP_OK)
    {
        httpd_resp_send(req, "{\"status\":\"success\", \"message\":\"Card removed\"}", HTTPD_RESP_USE_STRLEN);
    }
    else if (ret == ESP_ERR_NOT_FOUND)
    {
        char err_msg[100];
        sprintf(err_msg, "Card ID %ld not found", cardId);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, err_msg);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to remove RFID card: %s (Error: %s)", esp_err_to_name(ret), ret == ESP_FAIL ? "Generic Fail" : "Other");
        httpd_resp_send_500(req);
    }
    return ESP_OK;
}

static esp_err_t http_server_rfid_manager_get_card_count_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "RFID card count requested");

    // Get the card count from the RFID manager
    int card_count = rfid_manager_get_card_count();

    // Prepare the JSON response
    char response[50];
    snprintf(response, sizeof(response), "{\"card_count\":%d}", card_count);

    httpd_resp_set_type(req, "application/json");
    esp_err_t error = httpd_resp_send(req, response, strlen(response));

    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d while sending RFID card count response", error);
        return error;
    }

    ESP_LOGI(TAG, "RFID card count response sent successfully");
    return ESP_OK;
}

static esp_err_t http_server_rfid_manager_check_card_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "RFID card check requested");

    // Read the request body
    char buf[256];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0)
    {
        ESP_LOGE(TAG, "Failed to receive request data");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    // Parse the JSON request
    cJSON *json = cJSON_Parse(buf);
    if (json == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse JSON");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Extract the card ID
    cJSON *card_id_obj = cJSON_GetObjectItemCaseSensitive(json, "card_id");
    if (!cJSON_IsString(card_id_obj) || (card_id_obj->valuestring == NULL))
    {
        ESP_LOGE(TAG, "Invalid or missing 'card_id' key in JSON");
        cJSON_Delete(json);
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "{\"status\":\"error\",\"message\":\"Invalid or missing card_id\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    const char *card_id_str = card_id_obj->valuestring;

    // Convert string to uint32_t
    uint32_t card_id = (uint32_t)strtoul(card_id_str, NULL, 10);

    // Check if the card exists in the RFID manager
    esp_err_t check_result = rfid_manager_check_card(card_id);
    bool exists = (check_result == ESP_OK);

    // Clean up JSON object
    cJSON_Delete(json);

    // Prepare the response
    char response[50];
    snprintf(response, sizeof(response), "{\"exists\":%s}", exists ? "true" : "false");

    httpd_resp_set_type(req, "application/json");
    esp_err_t error = httpd_resp_send(req, response, strlen(response));

    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %d while sending RFID card check response", error);
        return error;
    }

    ESP_LOGI(TAG, "RFID card check response sent successfully");
    return ESP_OK;
}

static esp_err_t http_server_rfid_manager_reset_cards_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "RFID card reset requested");

    // Reset the RFID card database
    esp_err_t result = rfid_manager_format_database();

    if (result == ESP_OK)
    {
        ESP_LOGI(TAG, "RFID card database reset successfully");
        httpd_resp_set_status(req, "200 OK");
        httpd_resp_send(req, "{\"status\":\"success\",\"message\":\"RFID card database reset successfully\"}", HTTPD_RESP_USE_STRLEN);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to reset RFID card database: %s", esp_err_to_name(result));
        httpd_resp_send_500(req);
    }

    return ESP_OK;
}

#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "esp32-hal-ledc.h"
#include "sdkconfig.h"
#include "esp32-hal-log.h"

#define LED_LEDC_CHANNEL 2
#define CONFIG_LED_MAX_INTENSITY 255

int led_duty = 0;
bool isStreaming = false;

typedef struct
{
    httpd_req_t *req;
    size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

typedef struct {
    size_t size;  //number of values used for filtering
    size_t index; //current value index
    size_t count; //value count
    int sum;
    int *values; //array to be filled with values
} ra_filter_t;

static ra_filter_t ra_filter;

static ra_filter_t *ra_filter_init(ra_filter_t *filter, size_t sample_size) {
    memset(filter, 0, sizeof(ra_filter_t));

    filter->values = (int *)malloc(sample_size * sizeof(int));
    
    if (!filter->values) return NULL;
    
    memset(filter->values, 0, sample_size * sizeof(int));

    filter->size = sample_size;
    return filter;
}

static int ra_filter_run(ra_filter_t *filter, int value) {
    if (!filter->values) return value;

    filter->sum -= filter->values[filter->index];
    filter->values[filter->index] = value;
    filter->sum += filter->values[filter->index];
    filter->index++;
    filter->index = filter->index % filter->size;
    
    if (filter->count < filter->size)
        filter->count++;

    return filter->sum / filter->count;
}

void enable_led(bool en) {
    int duty = en ? led_duty : 0;
    if (en && isStreaming && (led_duty > CONFIG_LED_MAX_INTENSITY)) 
        duty = CONFIG_LED_MAX_INTENSITY;
    ledcWrite(LED_LEDC_CHANNEL, duty);
    log_i("Set LED intensity to %d", duty);
}

static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len) {
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if (!index) j->len = 0;
    if (httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK) return 0;
    j->len += len;
    return len;
}

static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t *fb = NULL;
    struct timeval _timestamp;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char *part_buf[128];

    static int64_t last_frame = 0;
    if (!last_frame)
        last_frame = esp_timer_get_time();

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK) return res;

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "X-Framerate", "60");
    isStreaming = true;
    enable_led(true);

    while (true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            log_e("Camera capture failed");
            res = ESP_FAIL;
        }else{
            _timestamp.tv_sec = fb->timestamp.tv_sec;
            _timestamp.tv_usec = fb->timestamp.tv_usec;

            if (fb->format != PIXFORMAT_JPEG) {
                bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                esp_camera_fb_return(fb);
                fb = NULL;
                if (!jpeg_converted) {
                    log_e("JPEG compression failed");
                    res = ESP_FAIL;
                }
            }else{
                _jpg_buf_len = fb->len;
                _jpg_buf = fb->buf;
            }
        }
        if (res == ESP_OK)
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        if (res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 128, _STREAM_PART, _jpg_buf_len, _timestamp.tv_sec, _timestamp.tv_usec);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK)
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        if (fb) {
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if (_jpg_buf) {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if (res != ESP_OK) {
            log_e("Send frame failed");
            break;
        }
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        frame_time /= 1000;
        uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time);
        log_i("MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps)");
    }

    isStreaming = false;
    enable_led(false);
    return res;
}

void startCameraServer(int& port) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;

    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL
    #ifdef CONFIG_HTTPD_WS_SUPPORT
        ,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
    #endif
    };

    ra_filter_init(&ra_filter, 20);

    log_i("Starting web server on port: '%d'", config.server_port);
    config.server_port += 1;
    config.ctrl_port += 1;
    log_i("Starting stream server on port: '%d'", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK)
        httpd_register_uri_handler(stream_httpd, &stream_uri);

    port = config.server_port;
}

void setupLedFlash(int pin) {
    ledcSetup(LED_LEDC_CHANNEL, 5000, 8);
    ledcAttachPin(pin, LED_LEDC_CHANNEL);
}
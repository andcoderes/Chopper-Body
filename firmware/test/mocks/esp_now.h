#pragma once

#include "Arduino.h"

typedef int esp_err_t;
#define ESP_OK 0
#define ESP_NOW_SEND_SUCCESS 0

typedef int esp_now_send_status_t;

// IDF 5.x recv/send callback info structs
typedef struct {
    uint8_t* src_addr;
    uint8_t* des_addr;
    int      rssi;
} esp_now_recv_info_t;

typedef struct {
    uint8_t* des_addr;
    uint8_t* src_addr;
} esp_now_send_info_t;

typedef void (*esp_now_recv_cb_t)(const esp_now_recv_info_t*, const uint8_t*, int);
typedef void (*esp_now_send_cb_t)(const esp_now_send_info_t*, esp_now_send_status_t);

typedef struct {
    uint8_t peer_addr[6];
    uint8_t lmk[16];
    uint8_t channel;
    int     ifidx;
    bool    encrypt;
} esp_now_peer_info_t;

inline esp_err_t esp_now_init() { return ESP_OK; }
inline esp_err_t esp_now_deinit() { return ESP_OK; }
inline esp_err_t esp_now_register_recv_cb(esp_now_recv_cb_t) { return ESP_OK; }
inline esp_err_t esp_now_register_send_cb(esp_now_send_cb_t) { return ESP_OK; }
inline esp_err_t esp_now_add_peer(const esp_now_peer_info_t*) { return ESP_OK; }
inline esp_err_t esp_now_del_peer(const uint8_t*) { return ESP_OK; }
inline esp_err_t esp_now_send(const uint8_t*, const uint8_t*, size_t) { return ESP_OK; }
inline esp_err_t esp_now_set_pmk(const uint8_t*) { return ESP_OK; }

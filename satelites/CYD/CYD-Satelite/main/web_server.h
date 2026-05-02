#pragma once
#include "interfaces.h"
#include "esp_http_server.h"

class WebServer {
public:
    WebServer(IConfig& config, IWifiManager& wifi, IMqttManager& mqtt);
    ~WebServer();

    void start();
    void stop();

private:
    IConfig&      m_config;
    IWifiManager& m_wifi;
    IMqttManager& m_mqtt;
    httpd_handle_t m_server = nullptr;

    // Static trampoline handlers — httpd requires C function pointers
    static esp_err_t handleRoot  (httpd_req_t* req);
    static esp_err_t handleGetCfg(httpd_req_t* req);
    static esp_err_t handleSetCfg(httpd_req_t* req);
    static esp_err_t handleScan  (httpd_req_t* req);
    static esp_err_t handleStatus(httpd_req_t* req);
};

#include "web_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include <string.h>
#include <stdio.h>

static const char* TAG = "WebServer";

// ── Embedded HTML ─────────────────────────────────────────────────────────────

static const char HTML[] = R"html(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>CYD Satelite</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{font-family:sans-serif;background:#111;color:#eee;max-width:480px;margin:0 auto;padding:16px}
  h1{font-size:1.2rem;margin-bottom:16px;color:#4af}
  h2{font-size:.9rem;text-transform:uppercase;letter-spacing:.1em;color:#888;margin:20px 0 8px}
  label{display:block;font-size:.85rem;margin-bottom:4px;color:#aaa}
  input,select{width:100%;padding:8px;background:#222;border:1px solid #444;color:#eee;border-radius:4px;font-size:.9rem;margin-bottom:12px}
  input[type=range]{padding:4px}
  button{width:100%;padding:10px;background:#4af;border:none;border-radius:4px;color:#000;font-weight:bold;cursor:pointer;font-size:.95rem}
  button:active{background:#28d}
  #status{font-size:.8rem;padding:8px;background:#1a1a1a;border-radius:4px;margin-bottom:16px}
  .dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:6px}
  .on{background:#4f4} .off{background:#f44}
  #msg{margin-top:12px;text-align:center;font-size:.85rem;color:#4af;min-height:1.2em}
  #bright-val{display:inline;margin-left:8px;font-size:.85rem;color:#aaa}
</style>
</head>
<body>
<h1>CYD Satelite</h1>
<div id="status">Loading…</div>

<h2>Device</h2>
<label>Name</label>
<input id="device_name" type="text" maxlength="31">
<label>LED Brightness <span id="bright-val">255</span></label>
<input id="led_brightness" type="range" min="0" max="255" value="255"
       oninput="document.getElementById('bright-val').textContent=this.value">

<h2>WiFi</h2>
<label>Network</label>
<select id="wifi_ssid_sel" onchange="document.getElementById('wifi_ssid').value=this.value">
  <option value="">— scan results —</option>
</select>
<label>SSID (manual)</label>
<input id="wifi_ssid" type="text" maxlength="63">
<label>Password</label>
<input id="wifi_pass" type="password" maxlength="63">
<button onclick="scan()" style="margin-bottom:12px;background:#333;color:#eee">Scan</button>

<h2>MQTT</h2>
<label>Broker URL</label>
<input id="mqtt_url" type="text" placeholder="mqtt://192.168.1.100" maxlength="127">
<label>Topic</label>
<input id="mqtt_topic" type="text" placeholder="tally/device/gpio/4" maxlength="127">

<button onclick="save()">Save &amp; Restart</button>
<div id="msg"></div>

<script>
async function load(){
  try{
    const r=await fetch('/api/config');
    const d=await r.json();
    document.getElementById('device_name').value=d.device_name||'';
    document.getElementById('led_brightness').value=d.led_brightness||255;
    document.getElementById('bright-val').textContent=d.led_brightness||255;
    document.getElementById('wifi_ssid').value=d.wifi_ssid||'';
    document.getElementById('mqtt_url').value=d.mqtt_url||'';
    document.getElementById('mqtt_topic').value=d.mqtt_topic||'';
  }catch(e){msg('Failed to load config')}
}
async function scan(){
  try{
    const r=await fetch('/api/scan');
    const d=await r.json();
    const sel=document.getElementById('wifi_ssid_sel');
    sel.innerHTML='<option value="">— scan results —</option>';
    d.forEach(s=>{const o=document.createElement('option');o.value=s;o.textContent=s;sel.appendChild(o)});
  }catch(e){msg('Scan failed')}
}
async function save(){
  const body={
    device_name:document.getElementById('device_name').value,
    led_brightness:parseInt(document.getElementById('led_brightness').value),
    wifi_ssid:document.getElementById('wifi_ssid').value,
    wifi_pass:document.getElementById('wifi_pass').value,
    mqtt_url:document.getElementById('mqtt_url').value,
    mqtt_topic:document.getElementById('mqtt_topic').value
  };
  try{
    const r=await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
    if(r.ok){msg('Saved — restarting…')}else{msg('Save failed')}
  }catch(e){msg('Save failed')}
}
async function pollStatus(){
  try{
    const r=await fetch('/api/status');
    const d=await r.json();
    document.getElementById('status').innerHTML=
      '<span class="dot '+(d.wifi?'on':'off')+'"></span>WiFi: '+(d.wifi?d.ip:'disconnected')+
      ' &nbsp; <span class="dot '+(d.mqtt?'on':'off')+'"></span>MQTT: '+(d.mqtt?'connected':'disconnected');
  }catch(e){}
}
function msg(t){document.getElementById('msg').textContent=t}
load();scan();pollStatus();
setInterval(pollStatus,3000);
</script>
</body>
</html>)html";

// ── WebServer ─────────────────────────────────────────────────────────────────

WebServer::WebServer(IConfig& config, IWifiManager& wifi, IMqttManager& mqtt)
    : m_config(config), m_wifi(wifi), m_mqtt(mqtt) {}

WebServer::~WebServer() { stop(); }

void WebServer::start()
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.max_uri_handlers = 8;

    if (httpd_start(&m_server, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return;
    }

    // Store `this` in each handler's user_ctx
    httpd_uri_t uris[] = {
        { "/",           HTTP_GET,  handleRoot,   this },
        { "/api/config", HTTP_GET,  handleGetCfg, this },
        { "/api/config", HTTP_POST, handleSetCfg, this },
        { "/api/scan",   HTTP_GET,  handleScan,   this },
        { "/api/status", HTTP_GET,  handleStatus, this },
    };
    for (auto& u : uris)
        httpd_register_uri_handler(m_server, &u);

    ESP_LOGI(TAG, "HTTP server started on port %d", cfg.server_port);
}

void WebServer::stop()
{
    if (m_server) {
        httpd_stop(m_server);
        m_server = nullptr;
    }
}

// ── handlers ─────────────────────────────────────────────────────────────────

esp_err_t WebServer::handleRoot(httpd_req_t* req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, HTML, HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebServer::handleGetCfg(httpd_req_t* req)
{
    auto* self = static_cast<WebServer*>(req->user_ctx);
    const DeviceConfig& c = self->m_config.get();
    char buf[512];
    snprintf(buf, sizeof(buf),
        "{\"device_name\":\"%s\","
        "\"led_brightness\":%d,"
        "\"wifi_ssid\":\"%s\","
        "\"mqtt_url\":\"%s\","
        "\"mqtt_topic\":\"%s\"}",
        c.device_name, c.led_brightness,
        c.wifi_ssid, c.mqtt_url, c.mqtt_topic);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, buf);
}

esp_err_t WebServer::handleSetCfg(httpd_req_t* req)
{
    auto* self = static_cast<WebServer*>(req->user_ctx);

    char body[768] = {};
    int  received  = httpd_req_recv(req, body,
                                    sizeof(body) - 1 < (size_t)req->content_len
                                        ? sizeof(body) - 1
                                        : req->content_len);
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty body");
        return ESP_FAIL;
    }

    // Simple field extractor: find "key":"value" or "key":number
    auto strField = [&](const char* key, char* out, size_t maxLen) {
        char search[64];
        snprintf(search, sizeof(search), "\"%s\":\"", key);
        const char* p = strstr(body, search);
        if (!p) return;
        p += strlen(search);
        size_t i = 0;
        while (*p && *p != '"' && i < maxLen - 1) out[i++] = *p++;
        out[i] = '\0';
    };
    auto u8Field = [&](const char* key, uint8_t& out) {
        char search[64];
        snprintf(search, sizeof(search), "\"%s\":", key);
        const char* p = strstr(body, search);
        if (!p) return;
        p += strlen(search);
        out = (uint8_t)atoi(p);
    };

    DeviceConfig cfg = self->m_config.get();
    strField("device_name",   cfg.device_name,  sizeof(cfg.device_name));
    strField("wifi_ssid",     cfg.wifi_ssid,    sizeof(cfg.wifi_ssid));
    strField("wifi_pass",     cfg.wifi_pass,    sizeof(cfg.wifi_pass));
    strField("mqtt_url",      cfg.mqtt_url,     sizeof(cfg.mqtt_url));
    strField("mqtt_topic",    cfg.mqtt_topic,   sizeof(cfg.mqtt_topic));
    u8Field ("led_brightness", cfg.led_brightness);

    self->m_config.set(cfg);
    if (!self->m_config.save()) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Save failed");
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "{\"ok\":true}");
    // Restart after the response is flushed
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_restart();
    return ESP_OK;
}

esp_err_t WebServer::handleScan(httpd_req_t* req)
{
    auto* self = static_cast<WebServer*>(req->user_ctx);

    // Kick off a background scan so the *next* request gets fresher results
    self->m_wifi.triggerScan();

    // Heap-allocate: wifi_ap_record_t is ~88 bytes × 16 = ~1400 bytes — too large for httpd task stack
    auto* records = new wifi_ap_record_t[16];
    int count = self->m_wifi.getApRecords(records, 16);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "[");
    bool first = true;
    for (int i = 0; i < count; i++) {
        if (records[i].ssid[0] == '\0') continue;
        char entry[40];
        snprintf(entry, sizeof(entry), "%s\"%s\"", first ? "" : ",", (char*)records[i].ssid);
        httpd_resp_sendstr_chunk(req, entry);
        first = false;
    }
    httpd_resp_sendstr_chunk(req, "]");
    delete[] records;
    return httpd_resp_sendstr_chunk(req, nullptr);
}

esp_err_t WebServer::handleStatus(httpd_req_t* req)
{
    auto* self = static_cast<WebServer*>(req->user_ctx);

    char ipStr[16] = "0.0.0.0";
    if (self->m_wifi.isConnected()) {
        esp_ip4_addr_t ip = self->m_wifi.getStaIp();
        snprintf(ipStr, sizeof(ipStr), IPSTR, IP2STR(&ip));
    }

    char buf[128];
    snprintf(buf, sizeof(buf),
        "{\"wifi\":%s,\"mqtt\":%s,\"ip\":\"%s\"}",
        self->m_wifi.isConnected()  ? "true" : "false",
        self->m_mqtt.isConnected()  ? "true" : "false",
        ipStr);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, buf);
}

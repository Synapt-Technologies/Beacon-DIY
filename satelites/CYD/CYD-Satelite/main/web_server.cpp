#include "web_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
body{font-family:sans-serif;background:#111;color:#eee;max-width:520px;margin:0 auto;padding:16px}
h1{font-size:1.2rem;margin-bottom:16px;color:#4af}
h2{font-size:.85rem;text-transform:uppercase;letter-spacing:.1em;color:#888;margin:20px 0 8px}
label{display:block;font-size:.85rem;margin-bottom:4px;color:#aaa}
input,select,textarea{width:100%;padding:8px;background:#222;border:1px solid #444;color:#eee;border-radius:4px;font-size:.9rem;margin-bottom:12px}
input[type=range]{padding:4px}
textarea{resize:vertical;min-height:60px;font-family:monospace;font-size:.8rem}
button{width:100%;padding:10px;background:#4af;border:none;border-radius:4px;color:#000;font-weight:bold;cursor:pointer;font-size:.95rem}
button:active{background:#28d}
.scan-btn{background:#333;color:#eee;margin-bottom:12px}
#status{font-size:.8rem;padding:8px;background:#1a1a1a;border-radius:4px;margin-bottom:16px}
.dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:6px}
.on{background:#4f4}.off{background:#f44}
.alert{display:none;background:#3a2a00;border:1px solid #b8860b;color:#ffd46b;border-radius:4px;padding:10px;margin-bottom:12px;font-size:.85rem}
.alert.show{display:block}
#msg{margin-top:12px;text-align:center;font-size:.85rem;color:#4af;min-height:1.2em}
#bright-val{display:inline;margin-left:8px;font-size:.85rem;color:#aaa}
code{display:block;padding:6px 8px;background:#1a1a1a;border-radius:4px;font-size:.8rem;color:#8cf;margin-bottom:12px;word-break:break-all}
.hint{font-size:.75rem;color:#666;margin-top:-8px;margin-bottom:10px}
</style>
</head>
<body>
<h1>CYD Satelite</h1>
<div id="status">Loading…</div>
<div id="reboot_alert" class="alert">
  Reboot required to apply saved changes.
  <button onclick="rebootDevice()">Reboot now</button>
</div>

<h2>Device</h2>
<label>Name</label>
<input id="device_name" type="text" maxlength="31">
<label>LED Brightness <span id="bright-val">255</span></label>
<input id="led_brightness" type="range" min="0" max="255" value="255"
       oninput="document.getElementById('bright-val').textContent=this.value">
<button onclick="saveSection('device')">Save Device</button>

<h2>WiFi</h2>
<label>Network</label>
<select id="wifi_ssid_sel" onchange="document.getElementById('wifi_ssid').value=this.value">
  <option value="">— scan results —</option>
</select>
<label>SSID (manual)</label>
<input id="wifi_ssid" type="text" maxlength="63">
<label>Password</label>
<input id="wifi_pass" type="password" maxlength="63" placeholder="leave blank to keep current">
<button class="scan-btn" onclick="scan()">Scan</button>
<button onclick="saveSection('wifi')">Save WiFi</button>

<h2>MQTT Broker</h2>
<label>Broker URL</label>
<input id="mqtt_url" type="text" placeholder="mqtt://192.168.1.100" maxlength="127">
<button onclick="saveSection('mqtt')">Save MQTT</button>

<h2>Beacon Identity</h2>
<label>Consumer ID</label>
<input id="consumer_id" type="text" placeholder="aedes" maxlength="31" oninput="updateTopic()">
<label>Device ID</label>
<input id="device_id" type="text" placeholder="leave blank to use MAC" maxlength="47" oninput="updateTopic()">
<p class="hint">Leave Device ID blank to auto-assign from MAC address on next boot.</p>
<label>Tally topic (derived)</label>
<code id="derived_topic">tally/device/…/…</code>
<button onclick="saveSection('beacon')">Save Beacon</button>

<h2>LED Layout</h2>
<label>Layout descriptor</label>
<textarea id="led_layout" rows="3"></textarea>
<p class="hint">
  Format: <b>F=&lt;target&gt;</b> for fixed LED, <b>S=&lt;start&gt;,&lt;count&gt;,&lt;target&gt;</b> for strip sections.<br>
  Targets: OPERATOR | TALENT | ALL | NONE &nbsp;—&nbsp; separated by semicolons.<br>
  Example: <code style="display:inline;padding:2px 4px">F=OPERATOR;S=0,11,TALENT;S=11,10,OPERATOR</code>
</p>
<button onclick="saveSection('layout')">Save Layout</button>

<div id="msg"></div>

<script>
function updateTopic(){
  const c=document.getElementById('consumer_id').value||'…';
  const d=document.getElementById('device_id').value||'&lt;mac&gt;';
  document.getElementById('derived_topic').textContent='tally/device/'+c+'/'+d;
}
async function load(){
  try{
    const r=await fetch('/api/config');
    const d=await r.json();
    document.getElementById('device_name').value=d.device_name||'';
    document.getElementById('led_brightness').value=d.led_brightness??255;
    document.getElementById('bright-val').textContent=d.led_brightness??255;
    document.getElementById('wifi_ssid').value=d.wifi_ssid||'';
    document.getElementById('mqtt_url').value=d.mqtt_url||'';
    document.getElementById('consumer_id').value=d.consumer_id||'aedes';
    document.getElementById('device_id').value=d.device_id||'';
    document.getElementById('led_layout').value=d.led_layout||'';
    updateTopic();
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
async function saveSection(section){
  let body={};
  switch(section){
    case 'device':
      body={device_name:document.getElementById('device_name').value,led_brightness:parseInt(document.getElementById('led_brightness').value)};
      break;
    case 'wifi':
      body={wifi_ssid:document.getElementById('wifi_ssid').value};
      const pass=document.getElementById('wifi_pass').value;
      if(pass)body.wifi_pass=pass;
      break;
    case 'mqtt':
      body={mqtt_url:document.getElementById('mqtt_url').value};
      break;
    case 'beacon':
      body={consumer_id:document.getElementById('consumer_id').value,device_id:document.getElementById('device_id').value};
      break;
    case 'layout':
      body={led_layout:document.getElementById('led_layout').value};
      break;
  }
  try{
    const r=await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
    if(r.ok){
      setRebootNeeded(true);
      msg('Saved — reboot required');
    }else{
      msg('Save failed');
    }
  }catch(e){msg('Save failed')}
}
async function rebootDevice(){
  try{
    const r=await fetch('/api/reboot',{method:'POST'});
    if(r.ok){
      msg('Rebooting…');
      setRebootNeeded(false);
    }else{
      msg('Reboot failed');
    }
  }catch(e){
    msg('Reboot failed');
  }
}
function setRebootNeeded(needed){
  document.getElementById('reboot_alert').className=needed?'alert show':'alert';
}
async function pollStatus(){
  try{
    const r=await fetch('/api/status');
    const d=await r.json();
    document.getElementById('status').innerHTML=
      '<span class="dot '+(d.wifi?'on':'off')+'"></span>WiFi: '+(d.wifi?d.ip:'disconnected')+
      ' &nbsp;<span class="dot '+(d.mqtt?'on':'off')+'"></span>MQTT'+
      ' &nbsp;<span class="dot '+(d.beacon?'on':'off')+'"></span>Beacon';
    setRebootNeeded(d.reboot_needed===true);
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

    httpd_uri_t uris[] = {
        { "/",           HTTP_GET,  handleRoot,   this },
        { "/api/config", HTTP_GET,  handleGetCfg, this },
        { "/api/config", HTTP_POST, handleSetCfg, this },
        { "/api/reboot", HTTP_POST, handleReboot, this },
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

// ── Handlers ──────────────────────────────────────────────────────────────────

esp_err_t WebServer::handleRoot(httpd_req_t* req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, HTML, HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebServer::handleGetCfg(httpd_req_t* req)
{
    auto* self = static_cast<WebServer*>(req->user_ctx);
    const DeviceConfig& c = self->m_config.get();

    char buf[640];
    snprintf(buf, sizeof(buf),
        "{\"device_name\":\"%s\","
        "\"led_brightness\":%d,"
        "\"wifi_ssid\":\"%s\","
        "\"mqtt_url\":\"%s\","
        "\"consumer_id\":\"%s\","
        "\"device_id\":\"%s\","
        "\"led_layout\":\"%s\"}",
        c.device_name, c.led_brightness,
        c.wifi_ssid, c.mqtt_url,
        c.consumer_id, c.device_id, c.led_layout);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, buf);
}

esp_err_t WebServer::handleSetCfg(httpd_req_t* req)
{
    auto* self = static_cast<WebServer*>(req->user_ctx);

    char body[768] = {};
    int received = httpd_req_recv(req, body,
                                  (int)sizeof(body) - 1 < req->content_len
                                      ? (int)sizeof(body) - 1
                                      : req->content_len);
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty body");
        return ESP_FAIL;
    }

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
        if (p) out = (uint8_t)atoi(p + strlen(search));
    };

    DeviceConfig cfg = self->m_config.get();
    
    auto hasField = [&](const char* key) {
        char search[64];
        snprintf(search, sizeof(search), "\"%s\":", key);
        return strstr(body, search) != nullptr;
    };
    
    if (hasField("device_name"))  strField("device_name",  cfg.device_name,  sizeof(cfg.device_name));
    if (hasField("wifi_ssid"))    strField("wifi_ssid",    cfg.wifi_ssid,    sizeof(cfg.wifi_ssid));
    if (hasField("wifi_pass"))    strField("wifi_pass",    cfg.wifi_pass,    sizeof(cfg.wifi_pass));
    if (hasField("mqtt_url"))     strField("mqtt_url",     cfg.mqtt_url,     sizeof(cfg.mqtt_url));
    if (hasField("consumer_id"))  strField("consumer_id",  cfg.consumer_id,  sizeof(cfg.consumer_id));
    if (hasField("device_id"))    strField("device_id",    cfg.device_id,    sizeof(cfg.device_id));
    if (hasField("led_layout"))   strField("led_layout",   cfg.led_layout,   sizeof(cfg.led_layout));
    if (hasField("led_brightness")) u8Field ("led_brightness", cfg.led_brightness);

    self->m_config.set(cfg);
    if (!self->m_config.save()) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Save failed");
        return ESP_FAIL;
    }

    self->m_rebootNeeded = true;
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

esp_err_t WebServer::handleReboot(httpd_req_t* req)
{
    auto* self = static_cast<WebServer*>(req->user_ctx);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    self->m_rebootNeeded = false;
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_restart();
    return ESP_OK;
}

esp_err_t WebServer::handleScan(httpd_req_t* req)
{
    auto* self = static_cast<WebServer*>(req->user_ctx);
    self->m_wifi.triggerScan();

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

    char buf[160];
    snprintf(buf, sizeof(buf),
        "{\"wifi\":%s,\"mqtt\":%s,\"beacon\":%s,\"reboot_needed\":%s,\"ip\":\"%s\"}",
        self->m_wifi.isConnected() ? "true" : "false",
        self->m_mqtt.isConnected() ? "true" : "false",
        self->m_beaconOnline       ? "true" : "false",
        self->m_rebootNeeded       ? "true" : "false",
        ipStr);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, buf);
}

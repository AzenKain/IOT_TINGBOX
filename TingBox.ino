
#include <Arduino.h>
#include <queue>
#include <String.h>
#include <Preferences.h>
#include <WiFi.h>
#include <vector>
#include <algorithm>
#include <WebServer.h>
#include <Audio.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <deque>
#include <set>

#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC 26

struct AudioTask {
    enum Type { TING, TTS } type;
    String msg;
};

std::queue<AudioTask> audioQueue;
Preferences preferences;

String url_ting_ting = "https://tiengdong.com/wp-content/uploads/Tieng-tinh-tinh-www_tiengdong_com.mp3";
String tts_msg = "Chào mừng bạn đến với dự án IOT của nhom 7";

unsigned long lastCall = 0;
const unsigned long interval = 4000;

std::deque<String> txQueue; 
std::set<String> playedTxIds;
const size_t MAX_TX = 10;

Audio audio;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

WebServer server(80);


void setupGlobals() {
    preferences.begin("tingbox", false);
}

void setupWiFi() {
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  String authToken = preferences.getString("authToken", "");
  String vaAccount = preferences.getString("vaAccount", "");
  int volume = preferences.getInt("volume", 21);
  String lastTransactionId = preferences.getString("lastTxId", "");

  Serial.println("Loaded preferences");

  // STA + AP mode
  if (ssid != "" && password != "") WiFi.begin(ssid.c_str(), password.c_str());
  WiFi.softAP("ESP32 Network", "admin@123");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Web server
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();

}


String buildWifiOptionsHTML() {
  String options = "";
  int n = WiFi.scanNetworks();
  Serial.println("Scan done, found: " + String(n));
  if (n == 0) {
    options += "<option value=''>-- No networks found --</option>";
    return options;
  }

  struct Net {
    String ssid;
    int rssi;
    int enc;
  };
  std::vector<Net> nets;
  for (int i = 0; i < n; ++i) {
    Net e;
    e.ssid = WiFi.SSID(i);
    e.rssi = WiFi.RSSI(i);
    e.enc = WiFi.encryptionType(i);
    if (e.ssid.length() == 0) continue;
    nets.push_back(e);
  }

  std::sort(nets.begin(), nets.end(), [](const Net &a, const Net &b) {
    return a.rssi > b.rssi;
  });

  for (auto &net : nets) {
    String label = net.ssid + " (" + String(net.rssi) + "dBm";
    if (net.enc == WIFI_AUTH_OPEN) label += ", open";
    label += ")";
    options += "<option value=\"" + net.ssid + "\">" + label + "</option>";
  }
  return options;
}

const char PAGE_STYLE[] PROGMEM = R"RAW(
<style>
  :root {
    --primary-color: #007bff;
    --dark-color: #0056b3;
    --secondary-color: #6c757d;
    --secondary-dark: #5a6268;
    --bg-color: #f4f4f4;
    --card-color: #ffffff;
    --text-color: #333;
    --border-color: #ccc;
    --shadow: 0 4px 10px rgba(0,0,0,0.1);
  }
  * {
    box-sizing: border-box;
  }
  body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    background-color: var(--bg-color);
    color: var(--text-color);
    margin: 0;
    padding: 0;
    display: flex;
    justify-content: center;
    align-items: center;
    min-height: 100vh;
  }
  .container {
    max-width: 500px;
    width: 90%;
    margin: 20px;
    padding: 25px;
    background-color: var(--card-color);
    border-radius: 8px;
    box-shadow: var(--shadow);
  }
  h3 {
    color: var(--primary-color);
    text-align: center;
    margin-top: 0;
    margin-bottom: 20px;
  }
  form {
    display: flex;
    flex-direction: column;
    gap: 18px;
  }
  form.rescan {
    margin-top: 18px;
  }
  .form-group {
    display: flex;
    flex-direction: column;
  }
  label {
    font-weight: bold;
    margin-bottom: 6px;
    font-size: 14px;
  }

  /* --- Tất cả input/select cùng chiều cao và căn giữa --- */
  input[type='text'],
  input[type='password'],
  input[type='number'],
  select {
    width: 100%;
    height: 48px;
    padding: 0 14px;
    border: 1px solid var(--border-color);
    border-radius: 5px;
    font-size: 16px;
    line-height: 48px;
    appearance: none;
    background-color: #fff;
  }

  input[type='submit'] {
    width: 100%;
    height: 48px;
    background-color: var(--primary-color);
    color: white;
    border: none;
    border-radius: 5px;
    cursor: pointer;
    font-size: 16px;
    font-weight: bold;
    transition: background-color 0.2s ease;
  }
  input[type='submit']:hover {
    background-color: var(--dark-color);
  }
  input.secondary {
    background-color: var(--secondary-color);
  }
  input.secondary:hover {
    background-color: var(--secondary-dark);
  }

  /* --- Thanh trượt volume --- */
  input[type='range'] {
    -webkit-appearance: none;
    appearance: none;
    width: 100%;
    height: 10px;
    background: #ddd;
    outline: none;
    opacity: 0.8;
    transition: opacity .2s;
    border-radius: 5px;
    margin-top: 8px;
  }
  input[type='range']:hover {
    opacity: 1;
  }
  input[type='range']::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 24px;
    height: 24px;
    background: var(--primary-color);
    cursor: pointer;
    border-radius: 50%;
  }
  input[type='range']::-moz-range-thumb {
    width: 24px;
    height: 24px;
    background: var(--primary-color);
    cursor: pointer;
    border-radius: 50%;
  }
  label[for='vol'] span {
    font-weight: bold;
    color: var(--primary-color);
  }

  p.footer {
    text-align: center;
    font-size: 12px;
    color: #888;
    margin-top: 20px;
    margin-bottom: 0;
  }
  a {
    color: var(--primary-color);
    text-decoration: none;
  }
  a:hover {
    text-decoration: underline;
  }
</style>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
)RAW";

void handleRoot() {
  String wifiOptions = buildWifiOptionsHTML();
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  String authToken = preferences.getString("authToken", "");
  String vaAccount = preferences.getString("vaAccount", "");
  int volume = preferences.getInt("volume", 21);
  
  String html = "<!doctype html><html><head><meta charset='utf-8'><title>ESP32 Config</title>";
  html += PAGE_STYLE;
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h3>ESP32 Configuration</h3>";

  html += "<form action='/save' method='get'>";
  html += "<div class='form-group'><label for='ssid'>Select WiFi:</label>";
  html += "<select name='ssid' id='ssid'>" + wifiOptions + "</select></div>";

  html += "<div class='form-group'><label for='ssid_custom'>Or enter SSID:</label>";
  html += "<input name='ssid_custom' id='ssid_custom' placeholder='(Optional)'></div>";

  html += "<div class='form-group'><label for='password'>Password:</label>";
  html += "<input name='password' id='password' type='password'></div>";

  html += "<div class='form-group'><label for='token'>Auth Token:</label>";
  html += "<input name='token' id='token' value='" + authToken + "'></div>";

  html += "<div class='form-group'><label for='va'>VA Account:</label>";
  html += "<input name='va' id='va' value='" + vaAccount + "'></div>";

  html += "<div class='form-group'>";
  html += "<label for='vol'>Volume: <span id='vol-val'>" + String(volume) + "</span> / 21</label>";
  html += "<input name='vol' id='vol' value='" + String(volume) + "' type='range' min='0' max='21' step='1' oninput='document.getElementById(\"vol-val\").innerText=this.value;'>";
  html += "</div>";

  html += "<input type='submit' value='Save and Connect'>";
  html += "</form>";

  html += "<form method='get' action='/' class='rescan'>";
  html += "<input type='submit' class='secondary' value='Rescan WiFi'>";
  html += "</form>";

  html += "<p class='footer'>AP IP: " + WiFi.softAPIP().toString() + "</p>";
  html += "</div></body></html>";

  server.send(200, "text/html", html);
}

void handleSave() {
  String selected = server.arg("ssid");
  String ssid_custom = server.arg("ssid_custom");
  String chosenSSID = ssid_custom.length() > 0 ? ssid_custom : selected;
  String pass = server.arg("password");

  String authToken = server.arg("token");
  String vaAccount = server.arg("va");
  int volume = server.arg("vol").toInt();
  audio.setVolume(volume);

  preferences.putString("ssid", chosenSSID);
  preferences.putString("password", pass);
  preferences.putString("authToken", authToken);
  preferences.putString("vaAccount", vaAccount);
  preferences.putInt("volume", volume);

  String reply = "<!doctype html><html><head><meta charset='utf-8'><title>ESP32 Config</title>";
  reply += PAGE_STYLE;
  reply += "</head><body>";

  reply += "<div class='container'>";
  reply += "<h3>Configuration Saved</h3>";
  reply += "<p>Đã lưu cấu hình. Đang thử kết nối tới SSID: <b>" + chosenSSID + "</b></p>";
  reply += "<p>Nếu kết nối thất bại, vui lòng kết nối lại vào mạng ESP32 này và thử lại.</p>";

  reply += "<form method='get' action='/'><input class='secondary' type='submit' value='Back to Config'></form>";

  reply += "</div></body></html>";
  server.send(200, "text/html", reply);

  if (chosenSSID.length() > 0) {
    Serial.println("Attempting WiFi connect to: " + chosenSSID);
    showConnectingWiFi();
    WiFi.begin(chosenSSID.c_str(), pass.c_str());
    unsigned long start = millis();
    while (millis() - start < 10000) {
      if (WiFi.status() == WL_CONNECTED) break;
      delay(200);
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to WiFi: " + WiFi.localIP().toString());
      showConnectedWiFi();
    } else {
      Serial.println("Failed to connect within timeout.");
    }
  }
}

void setupAudio()
{
  int volume = preferences.getInt("volume", 21);
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(volume);

  audioQueue.push({AudioTask::TING, url_ting_ting});
  audioQueue.push({AudioTask::TTS, tts_msg});
}

void loopAudio()
{
  if (WiFi.status() != WL_CONNECTED)
    return;
  if (audio.isRunning())
  {
    return;
  };

  if (!audioQueue.empty())
  {
    AudioTask task = audioQueue.front();
    audioQueue.pop();

    switch (task.type)
    {
    case AudioTask::TING:
      audio.connecttohost(task.msg.c_str());
      break;
    case AudioTask::TTS:
      audio.connecttospeech(task.msg.c_str(), "vi");
      break;
    }
  }
}
void processTransactions(JsonArray txs) {
    bool init = playedTxIds.empty();

    for (JsonObject tx : txs) {
        String txId = tx["id"].as<String>();

        if (playedTxIds.find(txId) == playedTxIds.end()) {
            if (init) {
                playedTxIds.insert(txId);
                txQueue.push_back(txId);
            } else {
                double amount = atof((const char*)tx["amount_in"]);
                String tts_msg = "Bạn đã nhận được " + String((long)amount) + " đồng";

                audioQueue.push({AudioTask::TING, url_ting_ting});
                audioQueue.push({AudioTask::TTS, tts_msg});

                playedTxIds.insert(txId);
                txQueue.push_back(txId);
            }

            if (txQueue.size() > MAX_TX) {
                String oldId = txQueue.front();
                txQueue.pop_front();
                playedTxIds.erase(oldId);
            }
        }
    }
}

void loopAPI() {
    if (WiFi.status() != WL_CONNECTED) return;
    if (audio.isRunning()) return;

    unsigned long now = millis();
    if (now - lastCall < interval) return;

    lastCall = now;

    HTTPClient http;
    String vaAccount = preferences.getString("vaAccount", "");
    String authToken = preferences.getString("authToken", "");

    String url = "https://my.sepay.vn/userapi/transactions/list?account_number=" + vaAccount + "%26limit=10";
    http.begin(url);
    http.addHeader("Authorization", "Bearer " + authToken);
    http.addHeader("Content-Type", "application/json");

    int code = http.GET();
    if (code == 200) {
        String payload = http.getString();
        JsonDocument doc;
        if (deserializeJson(doc, payload) == DeserializationError::Ok) {
            JsonArray txs = doc["transactions"].as<JsonArray>();
            if (txs.size() > 0) {
                processTransactions(txs);
                showHomeScreen(txs);
            }
        } else {
            Serial.println("❌ JSON parse error");
        }
    } else {
        Serial.printf("❌ HTTP request failed! Code: %d\n", code);
    }

    http.end();
}

void setupDisplay() {
    u8g2.begin();
    u8g2.setFont(u8g2_font_6x10_tr);

    u8g2.clearBuffer();
    u8g2.drawStr(5, 25, "Khởi động...");
    u8g2.sendBuffer();

    u8g2.clearBuffer(); 
    u8g2.drawStr(5, 10, "ESP32 Nhom 7"); 
    u8g2.drawStr(5, 25, "Wifi: ESP32 Network"); 
    u8g2.drawStr(5, 40, "Password: admin@123"); 
    u8g2.drawStr(5, 55, "IP: 192.168.4.1"); 
    u8g2.sendBuffer();
}

void showConnectingWiFi() {
    String ssid = preferences.getString("ssid", "");
    u8g2.clearBuffer();
    u8g2.drawStr(5, 10, "Dang ket noi toi Wi-Fi:");
    u8g2.drawStr(5, 25, ssid.c_str());
    u8g2.drawStr(5, 40, "Vui long cho...");
    u8g2.sendBuffer();
}

void showConnectedWiFi() {
    String ssid = preferences.getString("ssid", "");
    u8g2.clearBuffer();
    u8g2.drawStr(5, 10, "Đa ket noi toi Wi-Fi:");
    u8g2.drawStr(5, 25, ssid.c_str());
    u8g2.drawStr(5, 40, "Đang tai du lieu");
    u8g2.sendBuffer();
}

void showHomeScreen(JsonArray txs) {
    u8g2.clearBuffer();
    u8g2.drawStr(5, 10, "3 Giao dich gan nhat:");
    int count = 0;

    for (JsonObject tx : txs)
    {
        if (count >= 3) break;

        double amount = atof((const char *)tx["amount_in"]);

        String amountStr = String((long)amount);
        int len = amountStr.length();
        String formatted = "";
        int commaPos = len % 3;
        if (commaPos == 0) commaPos = 3;

        for (int i = 0; i < len; i++)
        {
            formatted += amountStr[i];
            if ((i + 1) % 3 == commaPos && i != len - 1)
            {
                formatted += ",";
                commaPos = 3;
            }
        }

        String line = String(count + 1) + ": " + formatted + " VND";
        u8g2.drawStr(5, 25 + count * 15, line.c_str());
        count++;
    }

    u8g2.sendBuffer();
}


void setup() {
  Serial.begin(115200);
  setupGlobals();
  setupWiFi();
  setupAudio();
  setupDisplay();
}

void loop() {
  server.handleClient();
  audio.loop();
  loopAudio();
  loopAPI();
}

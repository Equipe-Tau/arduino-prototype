#include <WiFi.h>
#include <ESP32Servo.h>

#define MAX_LISTENERS 10 
#define WIFI_LED 2

TaskHandle_t WifiTask;

typedef void (*Callable)();

typedef struct {
  Callable function;
  int interval;
  int execute;
  bool repeat;
} Schedule;

Schedule schedules[MAX_LISTENERS];

Servo servo;

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing");

  pinMode(WIFI_LED, OUTPUT);
  servo.attach(23);

  addSchedule(teste, 1, true);
  //Serial.println("Executing in 5s");

  //addSchedule(move180, 10, true);
  addSchedule(move, 2, true);

  xTaskCreatePinnedToCore(wifiHandler, "wifiHandler", 4096, NULL, 1, &WifiTask, 0);
}


void loop() {
  schedule();
  delay(10);
}

bool enabled = false;
void move () {
  servo.write((enabled = !enabled) ? 180 : 0);
}


void wifiHandler (void * parameter ) {
  Serial.print("Conectando-se ao Wi-Fi");
  WiFi.begin("Wokwi-GUEST", "", 6);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Conectado!");

  while (true) {
    switch (WiFi.status()) {
      case WL_CONNECTED:
        digitalWrite(WIFI_LED, HIGH);
        break;
      case WL_NO_SSID_AVAIL:
        Serial.println(F("Rede não encontrada"));
        break;
      case WL_CONNECT_FAILED:
      case WL_CONNECTION_LOST:
      case WL_DISCONNECTED:
        digitalWrite(WIFI_LED, LOW);
        Serial.println(F("Reconectando..."));
        WiFi.reconnect();
        break;
      default:
        Serial.println(F("Rede ociosa..."));
        break;
    }
    delay(10);
  }
}

void teste () {
  Serial.println("Agora são: " + String(millis()) + "...");
}

void addSchedule (Callable function, int interval, bool repeat) {
  const int now = millis();
  const Schedule newSchedule = {function, interval, (interval * 1000) + now, repeat};

  for (int i = 0; i < MAX_LISTENERS; i++) {
    const Schedule currentSchedule = schedules[i];
    
    if(currentSchedule.interval == 0 || currentSchedule.execute <= now) {
      schedules[i] = newSchedule;
      return;
    }
  }
}

void schedule () {
  int now = millis();

  for (int i = 0; i < MAX_LISTENERS; i++) {
    const Schedule currentSchedule = schedules[i];
    if(currentSchedule.interval && currentSchedule.execute <= now) {
      currentSchedule.function();
      if(currentSchedule.repeat) {
        addSchedule(
          currentSchedule.function, 
          currentSchedule.interval, 
          currentSchedule.repeat
        );
        return;
      }
      schedules[i] = {0,0,0,0};
    }
  }
}

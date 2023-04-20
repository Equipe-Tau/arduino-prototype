#include <WiFi.h>
#include <IPAddress.h>
#include <HTTPClient.h>

#include <Adafruit_Fingerprint.h>
#include <ESP32Servo.h>

#define SCHEDULE_SIZE 10

#define WIFI_SSID "IFMG_Servidores"
#define WIFI_PASSWORD NULL

#define WIFI_LED 2
#define RED_LED 18
#define GREEN_LED 22

#define LOCK_SERVO_PORT 26
#define LOCK_SERVO_OPENED 130
#define LOCK_SERVO_CLOSED 0

#define PUSH_BUTTON 15

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);
Servo lockServo;

void setup() {
  Serial.begin(115200);
  Serial.println(F("Inicializando"));

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(WIFI_LED, OUTPUT);

  setupWifi(WIFI_SSID, WIFI_PASSWORD);

  sendRequest("Iniciando...", true, 1);

  Serial.println(F("Iniciando sensor de impressão digital..."));
  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println(F("Sensor de impressão digital encontrado!"));
  } else {
    Serial.println(F("Não foi possível encontrar o sensor de impressão digital :("));
    return;
  }

  Serial.println(F("Iniciando servo..."));
  lockServo.attach(LOCK_SERVO_PORT);
  moveServo(lockServo, LOCK_SERVO_OPENED);
}

void loop() {
  wifiManager();
  fingerManager();
  schedule();
  delay(10); // TALVEZ AUMENTAR ISSO SEJA INTERESSANTE
}

String getName(int number) {
  String nomes[] = { "Migue%20Pinheiro", "Miguel%20BOB", "Raissa", "Roniery", "Leandro", "Gustavo", "Julia" };
  if (number > ((sizeof(nomes) / sizeof(nomes[0])) - 1) || number < 0) {
    return "Nome não definido";
  }
  return nomes[number];
}

void setupWifi(char *name, char *password) {

  Serial.println(F("Conectando ao Wi-Fi..."));

  WiFi.mode(WIFI_STA);
  WiFi.begin(name, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  digitalWrite(WIFI_LED, HIGH);
  Serial.println(F("\nConectado!"));
}

void wifiManager() {
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
}

void fingerManager() {
  Serial.print(F("*"));

  int result = finger.getImage();
  if (result == FINGERPRINT_OK) {
    Serial.println(F("Nova ocorrencia de dedo!"));
    if (finger.image2Tz() != FINGERPRINT_OK) {
      Serial.println(F("Erro na conversão da imagem!"));
      return;
    }

    if (finger.fingerFastSearch() != FINGERPRINT_OK) {
      Serial.println(F("Dedo não encontrado!"));
      digitalWrite(RED_LED, HIGH);
      addSchedule(RED_LED, LOW, 5);
      return;
    }

    digitalWrite(GREEN_LED, HIGH);
    addSchedule(GREEN_LED, LOW, 5);
    lockServo.write(LOCK_SERVO_CLOSED);

    sendRequest(getName(finger.fingerID), true /* VERIFICAR PUSH BUTTON */, 308);
    Serial.println("Digital de " + getName(finger.fingerID) + " encontrada!");

    return;
  }
}

bool sendRequest(String name, bool took, int port) {
  HTTPClient http;

  http.begin("https://docs.google.com/forms/d/e/1FAIpQLSczsF3e2sIlJW1pv5M4xh0arTnAgQu9jG1RlhVZYEeMOv-adA/formResponse?ifq&entry.669983157=" + name + "&entry.1164638721=" + (took ? "Pegou" : "Guardou") + "%20a%20chave%20" + String(port) + "&submit=Submit");

  int httpCode = http.GET();

  Serial.printf("[HTTP] Status Code: %d\n", httpCode);

  if (httpCode <= 0) {
    Serial.printf("[HTTP] A requisição GET falou, erro: %s\n", http.errorToString(httpCode).c_str());
  } 
  
  http.end();
  return true;
}



void moveServo(Servo servo, int target) {
  int current = servo.read();
  int increase = current < target;
  for (int i = current; (increase ? (i <= target) : (i >= target)); (increase ? i++ : i--)) {
    servo.write(i);
  }
}

/*
 * PARTE DE SCHEDULE (AGENDAMENTO DE TAREFAS)
 */

typedef struct
{
  int port;
  int time;
  int state;
} Schedule;

Schedule schedules[SCHEDULE_SIZE];

void schedule() {
  int now = millis();
  for (int i = 0; i < (sizeof(schedules) / sizeof(schedules[0])); i++) {
    Schedule currentSchedule = schedules[i];
    if ((currentSchedule.time - now) <= 0) {
      digitalWrite(currentSchedule.port, currentSchedule.state);
      schedules[i] = { 0, 0, 0 };
    }
  }
}

bool addSchedule(int port, int state, int toSeconds) {
  int now = millis();
  Schedule newSchedule = { port, now + (toSeconds * 1000), state };

  for (int i = 0; i < (sizeof(schedules) / sizeof(schedules[0])); i++) {
    Schedule currentSchedule = schedules[i];
    if ((currentSchedule.time - now) <= 0) {
      schedules[i] = newSchedule;
    }
    return true;
  }

  return false;
}

#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "vivo Y22";
const char* password = "move1away2";

WebServer server(80);

// Define traffic light pins
int redPins[] = {25, 26, 27, 14};
int yellowPins[] = {12, 13, 15, 16};
int greenPins[] = {17, 18, 19, 21};

volatile bool emergencyMode = false;

// Variables for non-blocking timing
unsigned long lastChangeTime = 0;
int lightState = 0;
int currentLight = 0;

// Emergency state
bool greenOn = false;
unsigned long emergencyStartTime = 0;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");
  Serial.print("Access the web server at: http://");
  Serial.print(WiFi.localIP());
   // Explicitly specifying port 80


  // Initialize all pins
  for (int i = 0; i < 4; i++) {
    pinMode(redPins[i], OUTPUT);
    pinMode(yellowPins[i], OUTPUT);
    pinMode(greenPins[i], OUTPUT);
    digitalWrite(redPins[i], HIGH);   // Red ON initially
    digitalWrite(yellowPins[i], LOW);
    digitalWrite(greenPins[i], LOW);
  }

  server.on("/control", handleControl);
  server.begin();
  Serial.println("Server started");
}

void loop() {
  server.handleClient();

  if (emergencyMode) {
    activateGreenNonBlocking();
  } else {
    normalCycleNonBlocking();
  }
}

void handleControl() {
  if (server.hasArg("light")) {
    String lightColor = server.arg("light");
    Serial.print("Received request: ");
    Serial.println(lightColor);

    if (lightColor == "green") {
      emergencyMode = true;
      server.send(200, "text/plain", "✅ Emergency Green Light Activated!");
    } else if (lightColor == "normal") {
      emergencyMode = false;
      greenOn = false;  // Reset emergency state
      server.send(200, "text/plain", "🔄 Returning to Normal Mode.");
    } else {
      server.send(400, "text/plain", "❌ Invalid request.");
    }
  }
}

void activateGreenNonBlocking() {
  if (!greenOn) {
    resetLights();
    digitalWrite(greenPins[0], HIGH);
    digitalWrite(redPins[1], HIGH);
    digitalWrite(redPins[2], HIGH);
    digitalWrite(redPins[3], HIGH);
    emergencyStartTime = millis();
    greenOn = true;
    Serial.println("🚨 Emergency Mode: Green Light ON at intersection 0");
  }

  if (millis() - emergencyStartTime >= 5000) {
    digitalWrite(greenPins[0], LOW);
    greenOn = false;
    emergencyMode = false;  // Optional: auto return to normal
    Serial.println("✅ Emergency cycle complete. Returning to normal...");
  }
}

void normalCycleNonBlocking() {
  unsigned long now = millis();

  switch (lightState) {
    case 0:  // Green phase
      digitalWrite(greenPins[currentLight], HIGH);
      digitalWrite(yellowPins[currentLight], LOW);
      digitalWrite(redPins[currentLight], LOW);

      if (now - lastChangeTime >= 5000) {
        digitalWrite(greenPins[currentLight], LOW);
        digitalWrite(yellowPins[currentLight], HIGH);
        lastChangeTime = now;
        lightState = 1;
        Serial.print("➡ Switching to YELLOW at intersection ");
        Serial.println(currentLight);
      }
      break;

    case 1:  // Yellow phase
      if (now - lastChangeTime >= 2000) {
        digitalWrite(yellowPins[currentLight], LOW);
        digitalWrite(redPins[currentLight], HIGH);

        currentLight = (currentLight + 1) % 4;
        lastChangeTime = now;
        lightState = 0;
        Serial.print("🔴 Switching to RED. Moving to intersection ");
        Serial.println(currentLight);
      }
      break;
  }
}

void resetLights() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(redPins[i], LOW);
    digitalWrite(yellowPins[i], LOW);
    digitalWrite(greenPins[i], LOW);
  }
}

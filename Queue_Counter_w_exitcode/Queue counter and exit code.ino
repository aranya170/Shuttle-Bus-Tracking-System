// Analog LDRs for entry/exit
const int ldr1 = A0; // Front of queue
const int ldr2 = A1; // Back of queue

// Digital LDRs for boarding detection
const int ldr3 = 4;  // Boarding sensor 1
const int ldr4 = 5;  // Boarding sensor 2

const int buzzer = 8;

int threshold = 500; // Adjust based on LDR+laser brightness
int queueCount = 0;

// For sequence detection
bool eventInProgress = false;
unsigned long eventStart = 0;
const unsigned long sequenceTimeout = 1000;

void setup() {
  Serial.begin(9600);
  Serial.println("🚍 Bus Queue Counter Initialized");

  pinMode(buzzer, OUTPUT);
  pinMode(ldr3, INPUT);
  pinMode(ldr4, INPUT);
  digitalWrite(buzzer, LOW);
}

void loop() {
  // Read LDR values
  int val1 = analogRead(ldr1);
  int val2 = analogRead(ldr2);
  bool ldr3Broken = digitalRead(ldr3) == HIGH;
  bool ldr4Broken = digitalRead(ldr4) == HIGH;

  bool ldr1Broken = val1 > threshold;
  bool ldr2Broken = val2 > threshold;

  Serial.print("LDR1: "); Serial.print(val1);
  Serial.print(" | LDR2: "); Serial.print(val2);
  Serial.print(" | LDR3: "); Serial.print(ldr3Broken);
  Serial.print(" | LDR4: "); Serial.println(ldr4Broken);

  // ENTRY: LDR1 → LDR2
  if (ldr1Broken) {
    waitFor(ldr2, true);  // Wait for LDR2 break
    if (analogRead(ldr2) > threshold) {
      queueCount++;
      Serial.print("🟢 Entry Detected. Queue Count: ");
      Serial.println(queueCount);
      triggerBuzzerIfNeeded();
      delay(500);
    }
  }

  // EXIT (gave up): LDR2 → LDR1
  if (ldr2Broken) {
    waitFor(ldr1, true); // Wait for LDR1 break
    if (analogRead(ldr1) > threshold) {
      if (queueCount > 0) queueCount--;
      Serial.print("🟡 Person Left Queue. Queue Count: ");
      Serial.println(queueCount);
      delay(500);
    }
  }

  // BOARDING: LDR3 → LDR4
  if (ldr3Broken) {
    waitFor(ldr4, false); // Wait for LDR4 to trigger
    if (digitalRead(ldr4) == HIGH) {
      if (queueCount > 0) queueCount--;
      Serial.print("🔵 Boarded Bus. Queue Count: ");
      Serial.println(queueCount);
      delay(500);
    }
  }

  delay(200); // Debounce
}

// Wait for a pin (analog or digital) to become "triggered"
void waitFor(int pin, bool isAnalog) {
  unsigned long start = millis();
  while (millis() - start < sequenceTimeout) {
    if (isAnalog && analogRead(pin) > threshold) return;
    if (!isAnalog && digitalRead(pin) == HIGH) return;
  }
}

void triggerBuzzerIfNeeded() {
  if (queueCount >= 5) {
    Serial.println("🚨 Queue Full! Buzzing...");
    digitalWrite(buzzer, HIGH);
    delay(2000);
    digitalWrite(buzzer, LOW);
    Serial.println("Buzzer done. Queue remains at:");
    Serial.println(queueCount);
  }
}

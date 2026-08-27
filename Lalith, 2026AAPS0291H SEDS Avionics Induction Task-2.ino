#include <LiquidCrystal.h>                          // Pull in the library that lets us talk to a standard parallel-interface LCD screen
 
// LCD pins
const int RS = 12, EN = 11, D4 = 5, D5 = 4, D6 = 3, D7 = 2;  // Digital pins wired to the LCD's control/data lines
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);           // Create the LCD object, telling the library which pins map to which LCD signal
 
// Ultrasonic (Parallax PING))) - single SIG pin, shared trigger/echo)
const int PING_PIN = 9;   // Single signal pin used for both sending the ultrasonic pulse and reading the echo back <-- CONFIRM this matches your wiring
const int LDR_PIN = A0;      // Analog pin the light-dependent resistor (light sensor) is wired to
const int BUTTON_PIN = 8;    // Digital pin the anchor push-button is wired to
const int LED_PIN = 7;       // Digital pin driving the storm-warning LED
const int BUZZER_PIN = 10;   // Digital pin driving the Charybdis/wrecked warning buzzer
 
enum ShipState { OPEN_SEA, ANCHOR_DROPPED, STORM, CHARYBDIS, WRECKED };  // Define the five possible states as a named type
ShipState currentState = OPEN_SEA;    // The ship always starts sailing normally, per the spec
ShipState previousState = OPEN_SEA;   // Tracks the last state so we can detect the exact moment a transition happens
 
unsigned long hazardStartTime = 0;    // Timestamp marking when the current STORM/CHARYBDIS danger began, for the 5-second wreck countdown
unsigned long lastLEDToggle = 0;      // Timestamp of the last LED blink flip, used to blink without blocking the rest of the loop
bool ledState = LOW;                  // Tracks whether the storm LED is currently on or off
 
bool lastButtonState = HIGH;          // Raw button reading from the previous loop iteration, to detect changes (button is HIGH when not pressed, due to pull-up)
unsigned long lastButtonChangeTime = 0;  // Timestamp of the last raw button change
const unsigned long DEBOUNCE_MS = 50; // Minimum time a button reading must stay stable before we trust it (filters out electrical noise/bounce)
 
bool anchorActive = false;            // Tracks whether the anchor is currently dropped (true) or raised (false)
 
void setup() {                        // Runs once when the Arduino powers on or resets
  Serial.begin(9600);                 // Start serial communication at 9600 baud so we can print sensor readings for debugging
  lcd.begin(16, 2);                   // Initialize the LCD as a 16-column, 2-row display
 
  pinMode(LED_PIN, OUTPUT);           // Configure the LED pin to send voltage out (drive the LED)
  pinMode(BUZZER_PIN, OUTPUT);        // Configure the buzzer pin to send voltage out (drive the buzzer)
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Configure the button pin as an input with the internal pull-up resistor enabled, so it reads HIGH when unpressed
  // PING_PIN mode is set dynamically inside getDistance()  // The ultrasonic pin's direction is switched between OUTPUT and INPUT on every reading, so it isn't fixed here
 
  updateLCD("OPEN SEA");              // Show the initial state on the LCD immediately at startup
}
 
void loop() {                         // Runs repeatedly, forever, after setup() finishes
  long distance = getDistance();      // Take a fresh ultrasonic distance reading this cycle
  int lightLevel = analogRead(LDR_PIN);  // Take a fresh light sensor reading (0-1023) this cycle
 
  Serial.print("Light Sensor (A0): ");  // Label the upcoming light value in the serial monitor
  Serial.print(lightLevel);             // Print the actual light reading
  Serial.print(" | Distance (cm): ");   // Label the upcoming distance value
  Serial.println(distance);             // Print the distance and move to a new line
 
  // NOTE: flipped vs. original — your LDR divider reads LOW in bright light.
  // If you rewire the divider the other way round, flip this back to '<'.
  bool stormTriggered = (lightLevel > 512);     // Storm condition: true once the light reading crosses the midpoint threshold (per this divider's wiring)
  bool charybdisTriggered = (distance < 100);   // Charybdis condition: true once an object is detected closer than 100 cm
 
  // --- Debounced button read (non-blocking) ---
  bool currentButtonState = digitalRead(BUTTON_PIN);   // Read the button's raw electrical state this cycle
  if (currentButtonState != lastButtonState) {         // Check if the raw reading just changed from last cycle
    lastButtonChangeTime = millis();                   // If it changed, reset the debounce timer to "now"
  }
  if ((millis() - lastButtonChangeTime) > DEBOUNCE_MS) {  // Only trust the reading once it's been stable for longer than the debounce window
    static bool debouncedState = HIGH;                 // Remembers the last confirmed state between loop iterations
    if (currentButtonState != debouncedState) {        // Check if this stable reading is actually a genuine new state
      debouncedState = currentButtonState;             // Lock in the new debounced state
      if (debouncedState == LOW && currentState != WRECKED) {  // A LOW reading means the button is physically pressed (due to pull-up wiring); ignore presses once wrecked
        anchorActive = !anchorActive;                  // Toggle the anchor: drop it if raised, raise it if dropped
      }
    }
  }
  lastButtonState = currentButtonState;                // Store this cycle's raw reading for comparison next cycle
 
  // --- State selection ---
  if (currentState != WRECKED) {                       // Once wrecked, skip all further state logic entirely — the ship stays wrecked forever
    if (anchorActive) {                                // Anchor overrides everything else and protects from danger
      currentState = ANCHOR_DROPPED;                   // Force the state to ANCHOR_DROPPED regardless of sensor readings
    } else if (currentState == STORM) {                // If currently in a storm, check whether to switch or stay
      if (charybdisTriggered) currentState = CHARYBDIS; // If Charybdis also triggers now, hand priority to it 
      else if (!stormTriggered) currentState = OPEN_SEA; // If the storm has cleared and no new hazard exists, return to normal sailing
      // else: stay in STORM                            // Otherwise, remain in STORM and let the danger timer keep running
    } else if (currentState == CHARYBDIS) {             // If currently near Charybdis, check whether to switch or stay
      if (!charybdisTriggered) {                        // If the ship has escaped Charybdis's range
        currentState = stormTriggered ? STORM : OPEN_SEA;  // Drop straight into STORM if one is now active, otherwise return to OPEN_SEA
      }
      // else: stay in CHARYBDIS (distance hazard takes priority once active)  // Otherwise remain in CHARYBDIS, keeping its timer running
    } else {                                            // Currently in OPEN_SEA (or ANCHOR_DROPPED just ended) — check for a fresh hazard
      if (stormTriggered) currentState = STORM;         // Enter STORM if the light condition is met
      else if (charybdisTriggered) currentState = CHARYBDIS;  // Otherwise enter CHARYBDIS if the distance condition is met
      else currentState = OPEN_SEA;                     // Otherwise remain safely in OPEN_SEA
    }
  }
 
  // --- Handle state transition ---
  if (currentState != previousState) {                 // Only run this block on the exact loop where the state actually changed
    if (currentState == STORM || currentState == CHARYBDIS) {  // Entering a danger state starts (or restarts) its 5-second wreck countdown
      hazardStartTime = millis();                       // Record the moment this danger began
    }
    digitalWrite(LED_PIN, LOW);                         // Reset the LED off by default on every transition, to avoid it getting stuck
    noTone(BUZZER_PIN);                                 // Reset the buzzer off by default on every transition, for the same reason
 
    switch (currentState) {                             // Update the LCD text to match whichever state we just entered
      case OPEN_SEA:       updateLCD("OPEN SEA"); break;         // Show plain sailing status
      case ANCHOR_DROPPED: updateLCD("ANCHOR DROPPED"); break;   // Show that the anchor is currently down
      case STORM:           updateLCD("STORM"); break;           // Show that a storm is active
      case CHARYBDIS:       updateLCD("CHARYBDIS"); tone(BUZZER_PIN, 1000); break;  // Show Charybdis and immediately start the warning tone
      case WRECKED:         updateLCD("WRECKED"); break;         // Show the terminal wrecked state
    }
    previousState = currentState;                       // Update our record of "last known state" so we don't repeat this block next loop
  }
 
  // --- Per-state ongoing behavior ---
  switch (currentState) {                               // Run continuous, every-loop behavior appropriate to whichever state we're currently in
    case STORM:                                          // While the storm is ongoing...
      if (millis() - lastLEDToggle >= 250) {             // Every 250ms (without blocking the rest of the loop)...
        ledState = !ledState;                            // Flip the LED's remembered on/off state
        digitalWrite(LED_PIN, ledState);                 // Apply that flipped state to the actual LED pin, producing a blink
        lastLEDToggle = millis();                        // Reset the blink timer
      }
      if (millis() - hazardStartTime >= 5000) currentState = WRECKED;  // If 5 full seconds of continuous storm have passed, the ship becomes wrecked
      break;
 
    case CHARYBDIS:                                      // While near Charybdis...
      if (millis() - hazardStartTime >= 5000) currentState = WRECKED;  // Same 5-second wreck rule applies here too
      break;
 
    case WRECKED:                                        // Once wrecked...
      digitalWrite(LED_PIN, HIGH);                       // Keep the LED solidly on as a visual "game over" indicator
      tone(BUZZER_PIN, 150);                              // Play a continuous low-pitched tone as an audible "game over" indicator
      break;
 
    default:                                              // Covers OPEN_SEA and ANCHOR_DROPPED, where nothing hazardous should be active
      digitalWrite(LED_PIN, LOW);                        // Make sure the LED stays off
      noTone(BUZZER_PIN);                                 // Make sure the buzzer stays silent
      break;
  }
 
  delay(200);                                            // Pause briefly between loop cycles to keep sensor readings and serial output at a readable pace
}
 
void updateLCD(String statusText) {                       // Helper function that redraws the LCD with a given status line
  lcd.clear();                                            // Wipe the screen so old text doesn't overlap with new text
  lcd.setCursor(0, 0);                                    // Move the cursor to the top-left character position
  lcd.print("State:");                                    // Print a constant label on the first row
  lcd.setCursor(0, 1);                                    // Move the cursor to the start of the second row
  lcd.print(statusText);                                  // Print the actual current state name on the second row
}
 
long getDistance() {                                      // Helper function that triggers the ultrasonic sensor and returns the measured distance in cm
  pinMode(PING_PIN, OUTPUT);                              // Temporarily set the shared pin to OUTPUT so we can send the trigger pulse
  digitalWrite(PING_PIN, LOW);                            // Ensure the pin starts LOW for a clean pulse edge
  delayMicroseconds(2);                                   // Brief settle time before triggering
  digitalWrite(PING_PIN, HIGH);                           // Send the trigger pulse HIGH...
  delayMicroseconds(5);                                   // ...hold it for the sensor's required pulse width...
  digitalWrite(PING_PIN, LOW);                             // ...then bring it back LOW to end the trigger pulse
 
  pinMode(PING_PIN, INPUT);                                // Switch the same pin to INPUT so we can now listen for the echo response
  long duration = pulseIn(PING_PIN, HIGH, 30000UL);        // Measure how long the echo pulse stays HIGH, in microseconds, timing out after 30ms (~5m range)
 
  long distanceCm = duration * 0.034 / 2;                  // Convert the round-trip time to a one-way distance using the speed of sound (0.034 cm/µs), dividing by 2 for the round trip
  if (distanceCm == 0) return 999;                         // If no echo was received (timeout), report a large "out of range" distance instead of a misleading zero
  return distanceCm;                                       // Return the calculated distance in centimeters
}
 
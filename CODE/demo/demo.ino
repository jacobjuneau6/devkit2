/**
 * devkit2 Demo Sketch
 *
 * Demonstrates the onboard peripherals of the devkit2 ESP32-S3 board:
 *   - WS2812B RGB LED (GPIO 48) — color cycle + blink patterns
 *   - Boot button (GPIO 0) — cycles through modes
 *   - USB-CDC serial output
 *
 * Hardware:
 *   Board: ESP32S3 Dev Module
 *   USB CDC On Boot: Enabled
 *   PSRAM: OPI PSRAM
 *
 * Dependencies:
 *   - Adafruit NeoPixel library
 */

#include <Adafruit_NeoPixel.h>

// ── Pin Definitions ────────────────────────────────────────────────

#define PIN_NEOPIXEL    2    // Onboard WS2812B data line
#define PIN_BOOT_BTN    0    // Boot button (active low)

// ── Constants ──────────────────────────────────────────────────────

#define NUMPIXELS       1    // Single onboard NeoPixel
#define DEBOUNCE_MS    50    // Button debounce window
#define SERIAL_BAUD 115200

// ── Globals ────────────────────────────────────────────────────────

Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

enum Mode {
  MODE_RAINBOW,       // Smooth hue sweep across the wheel
  MODE_BREATHE,       // Single-color breathing pulse
  MODE_STROBE,        // Fast RGB flash
  MODE_BLINK,         // Simple on/off blink in white
  MODE_COUNT
};

Mode    currentMode   = MODE_RAINBOW;
uint8_t breatheHue    = 0;
bool    lastBtnState  = HIGH;
unsigned long lastDebounce = 0;
unsigned long lastFrame    = 0;

// ── Setup ──────────────────────────────────────────────────────────

void setup() {
  // ── Serial ──
  Serial.begin(SERIAL_BAUD);
  delay(500);  // Let CDC settle
  Serial.println();
  Serial.println(F("╔══════════════════════════════╗"));
  Serial.println(F("║        devkit2  demo        ║"));
  Serial.println(F("╚══════════════════════════════╝"));
  Serial.println();
  Serial.println(F("Press BOOT button to cycle modes."));
  Serial.println();

  // ── NeoPixel ──
  pixels.begin();
  pixels.setBrightness(32);  // Keep it comfortable (0–255)
  pixels.clear();
  pixels.show();

  // ── Boot Button ──
  pinMode(PIN_BOOT_BTN, INPUT_PULLUP);  // External pull-up + internal

  Serial.printf("Mode: %s\n", modeName(currentMode));
}

// ── Loop ───────────────────────────────────────────────────────────

void loop() {
  unsigned long now = millis();

  // ── Button handling (debounced, falling edge) ──
  bool reading = digitalRead(PIN_BOOT_BTN);
  if (reading != lastBtnState) {
    lastDebounce = now;
  }
  if ((now - lastDebounce) > DEBOUNCE_MS) {
    if (reading == LOW && lastBtnState == HIGH) {
      // Button pressed — advance mode
      currentMode = (Mode)((currentMode + 1) % MODE_COUNT);
      Serial.printf("Mode: %s\n", modeName(currentMode));
      pixels.clear();
      pixels.show();
    }
  }
  lastBtnState = reading;

  // ── Animation (non-blocking) ──
  static uint16_t frame = 0;
  frame++;

  switch (currentMode) {
    case MODE_RAINBOW:   animateRainbow(frame);   break;
    case MODE_BREATHE:   animateBreathe(frame);   break;
    case MODE_STROBE:    animateStrobe(frame);    break;
    case MODE_BLINK:     animateBlink(frame);     break;
  }

  delay(5);  // ~200 fps max — plenty for human vision
}

// ═══════════════════════════════════════════════════════════════════
//  Animation helpers
// ═══════════════════════════════════════════════════════════════════

// ── Rainbow: continuous hue sweep ──────────────────────────────────

void animateRainbow(uint16_t frame) {
  uint32_t color = pixels.ColorHSV(frame * 256);  // 256 steps per hue tick
  pixels.setPixelColor(0, color);
  pixels.show();
}

// ── Breathe: single-color brightness pulse ─────────────────────────

void animateBreathe(uint16_t frame) {
  // Slowly rotate the hue
  if (frame % 10 == 0) breatheHue++;

  // Triangle wave for breathing effect
  uint16_t phase = frame & 0x1FF;  // 0–511
  uint8_t brightness;
  if (phase < 256) {
    brightness = phase;        // Fade in
  } else {
    brightness = 511 - phase;  // Fade out
  }

  uint32_t color = pixels.ColorHSV(breatheHue * 256);
  pixels.setPixelColor(0, color);
  pixels.setBrightness(constrain(brightness, 1, 255));
  pixels.show();
}

// ── Strobe: fast color flashes ─────────────────────────────────────

void animateStrobe(uint16_t frame) {
  static const uint32_t colors[] = {
    0xFF0000,  // Red
    0x00FF00,  // Green
    0x0000FF,  // Blue
    0xFFFFFF,  // White
  };
  static const int numColors = sizeof(colors) / sizeof(colors[0]);

  uint8_t phase = (frame / 20) % (numColors * 2);
  if (phase < numColors) {
    pixels.setBrightness(128);
    pixels.setPixelColor(0, colors[phase]);
  } else {
    pixels.setBrightness(0);  // Off between flashes
  }
  pixels.show();
  pixels.setBrightness(32);  // Restore for other modes
}

// ── Blink: simple on/off in warm white ─────────────────────────────

void animateBlink(uint16_t frame) {
  bool on = ((frame / 100) % 2) == 0;  // 2 Hz
  if (on) {
    pixels.setBrightness(64);
    pixels.setPixelColor(0, pixels.Color(255, 200, 100));  // Warm white
  } else {
    pixels.setBrightness(0);
  }
  pixels.show();
  pixels.setBrightness(32);
}

// ── Mode name helper ───────────────────────────────────────────────

const char* modeName(Mode m) {
  switch (m) {
    case MODE_RAINBOW:  return "Rainbow";
    case MODE_BREATHE:  return "Breathe";
    case MODE_STROBE:   return "Strobe";
    case MODE_BLINK:    return "Blink";
    default:            return "???";
  }
}

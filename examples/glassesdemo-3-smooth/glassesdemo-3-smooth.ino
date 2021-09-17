// Third example for Adafruit LED glasses. This is an evolution of the
// prior one, but demonstrates extra smooth graphics on the matrix part.
// This requires an extra 1.5K RAM and probably won't work on small boards
// like the Arduino Uno, but any 32-bit microcontroller should be fine.

#include <Adafruit_IS31FL3741.h>
#include <Fonts/FreeSansOblique9pt7b.h> // Different font!

// Some boards have just one I2C interface, but some have more...
TwoWire *i2c = &Wire; // e.g. change this to &Wire1 for QT Py RP2040

// Object declarations are nearly the same as before, with the "_buffered"
// postfix, but notice the matrix item has an extra "true" argument now.
// This creates an offscreen drawing surface that's 3X larger than the
// matrix, and later we can shrink it down with antialiasing.

// This is the glasses' core LED controller object
Adafruit_IS31FL3741_buffered ledcontroller;

// And these objects relate to the glasses' matrix and rings.
// Each expects the address of the controller object.
Adafruit_IS31FL3741_GlassesMatrix_buffered    ledmatrix(&ledcontroller, true);
Adafruit_IS31FL3741_GlassesLeftRing_buffered  leftring(&ledcontroller);
Adafruit_IS31FL3741_GlassesRightRing_buffered rightring(&ledcontroller);

// Notice the slight change here to the text positioning. The initial X
// position isn't known until we're in setup() and can access the offscreen
// canvas. Also, the Y position is now 14 instead of 5 (because 3X larger).

char text[] = "ADAFRUIT!";      // A message to scroll
int text_x;                     // Pos is initialized in setup()
int text_min;                   // Pos. where text resets (calc'd later)
int text_y = 14;                // Text base line at bottom of canvas
uint16_t ring_hue = 0;          // For ring animation

GFXcanvas16 *canvas;            // Pointer to canvas object

void setup() {
  Serial.begin(115200);
  Serial.println("ISSI3741 LED Glasses Adafruit GFX Test");

  if (! ledcontroller.begin(IS3741_ADDR_DEFAULT, i2c)) {
    Serial.println("IS41 not found");
    for (;;);
  }

  canvas = ledmatrix.getCanvas();
  if (! canvas) {
    Serial.println("Couldn't allocate canvas");
    for (;;);
  }

  Serial.println("IS41 found!");

  // By default the LED controller communicates over I2C at 400 KHz.
  // Arduino Uno can usually do 800 KHz, and 32-bit microcontrollers 1 MHz.
  Wire.setClock(800000);

  // Set brightness to max and bring controller out of shutdown state
  ledcontroller.setLEDscaling(0xFF);
  ledcontroller.setGlobalCurrent(0xFF);
  ledcontroller.enable(true);

  // Because canvas is a pointer to an object, not an object itself,
  // functions are accessed with -> instead of .
  text_x = canvas->width(); // Initial text position = off right edge

  // Clear canvas, set matrix to normal upright orientation
  canvas->fillScreen(0);
  ledmatrix.setRotation(0);

  // We're using a different font, because Tom Thumb is too tiny for this.
  // Legibility isn't great with this other font, but it's what we had on
  // hand. The ideal font would be 15 pixels tall but 'hinted' for a
  // 5-pixel grid. Oblique fonts tend to read a little better on scrollers.
  canvas->setFont(&FreeSansOblique9pt7b);
  canvas->setTextWrap(false); // Allow text to extend off edges

  // Rings work just as before
  rightring.setBrightness(50);  // Turn down the LED rings brightness,
  leftring.setBrightness(50);   // 0 = off, 255 = max

  // Get text dimensions to determine X coord where scrolling resets
  uint16_t w, h;
  int16_t ignore;
  canvas->getTextBounds(text, 0, 0, &ignore, &ignore, &w, &h);
  text_min = -w; // Off left edge this many pixels
}

void loop() {
  canvas->fillScreen(0); // Clear the whole drawing canvas

  // Update text to new position, and draw (now in canvas, not matrix)
  if (--text_x < text_min) {  // If text scrolls off left edge,
    text_x = canvas->width(); // reset position off right edge
  }
  canvas->setCursor(text_x, text_y);
  for (int i = 0; i < (int)strlen(text); i++) {
    // Get 24-bit color for this character, cycling through color wheel
    uint32_t color888 = ledcontroller.ColorHSV(65536 * i / strlen(text));
    // Remap 24-bit color to '565' color used by Adafruit_GFX
    uint16_t color565 = ledcontroller.color565(color888);
    canvas->setTextColor(color565); // Set text color
    canvas->print(text[i]);         // and print one character
  }

  // Now here's where something different happens. Calling the scale()
  // function will downsample the canvas and place it in the LED matrix
  // buffer. A side-effect of this is that it's always "opaque." Unlike
  // the prior examples where the rings could be drawn behind the text,
  // that's not an option here. Black pixels in the canvas will be black
  // on the LED matrix.
  ledmatrix.scale();

  // Animate the LED rings with a color wheel.
  for (int i=0; i < leftring.numPixels(); i++) {
    leftring.setPixelColor(i, ledcontroller.ColorHSV(
      ring_hue + i * 65536 / leftring.numPixels()));
  }
  for (int i=0; i < rightring.numPixels(); i++) {
    rightring.setPixelColor(i, ledcontroller.ColorHSV(
      ring_hue - i * 65536 / rightring.numPixels()));
  }
  ring_hue += 1000; // Shift color a bit on next frame - makes it spin

  ledcontroller.show(); // Always show() with a buffered controller!

  // There's no delay() in this example, we just let it run full-tilt
  // because the drawing and scaling takes a bit longer.
}
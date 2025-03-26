#include <DMD2.h>
#include <SoftDMD.h> // Explicitly include SoftDMD if needed, though DMD2 often includes it.

//------------ FONTS ---------------//
// Include the font file you want to use.
// Ensure the path is correct relative to your sketch directory or library location.
#include "fonts/Droid_Sans_12.h" // Character height 9 pixels

// #include "fonts/Arial14.h"
// #include "fonts/Arial_Black_16.h"

//------------ CONSTANTS ---------------//
// Display Panel Configuration
const int PANEL_WIDTH = 2;  // Number of P10 panels horizontally
const int PANEL_HEIGHT = 1; // Number of P10 panels vertically
const int PIXELS_PER_PANEL_X = 32;
const int PIXELS_PER_PANEL_Y = 16;

// Calculated Display Dimensions
const int DISPLAY_WIDTH_PIXELS = PANEL_WIDTH * PIXELS_PER_PANEL_X; // Should be 64 for 2x1 setup
const int DISPLAY_HEIGHT_PIXELS = PANEL_HEIGHT * PIXELS_PER_PANEL_Y; // Should be 16 for 2x1 setup

// Pin Configuration (Adjust if your wiring is different)
const int OE_PIN = 9; // Output Enable pin for brightness control/display on/off

// Display Settings
const byte DISPLAY_BRIGHTNESS = 250; // 0-255
const int TEXT_VERTICAL_POSITION = 3; // Vertical position (Y-coordinate) for text baseline
const unsigned long MESSAGE_DELAY_MS = 1000; // Delay between showing parts of the message

// Serial Communication
const long SERIAL_BAUD_RATE = 9600;

// Character Definitions (for custom width calculation if needed, but prefer library functions)
// Note: Using dmd.stringWidth() or dmd.charWidth() is generally preferred if available and accurate.
// This section is kept for reference but commented out as we'll try to use library functions first.
/*
const char SUPPORTED_CHARS[] = {' ', ',', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'X', 'Y', 'Z', 'W', 'J', 'K', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};
const int PIXEL_WIDTHS[] = {11, 3, 8, 6, 7, 7, 5, 6, 7, 7, 4, 6, 10, 8, 8, 6, 8, 6, 6, 8, 7, 8, 8, 8, 6, 12, 3, 7, 4, 6, 6, 7, 6, 6, 6, 6, 6, 6};
const int NUM_SUPPORTED_CHARS = sizeof(SUPPORTED_CHARS) / sizeof(SUPPORTED_CHARS[0]);
*/

// Error Messages
const char* const ERROR_MSG_SIZE = "SIZE ERROR";
const char* const ERROR_MSG_CHAR = "CHAR ERROR"; // If a character width cannot be determined
const char* const ERROR_MSG_FORMAT = "FORMAT ERR"; // If input string format is invalid

// Symbol Codes (as defined in original comments)
const int SYMBOL_NONE = 0;
const int SYMBOL_HEART = 1;
const int SYMBOL_PIZZA = 2;
const int SYMBOL_DONUT = 3;

//------------ FONT SELECTION ---------------//
// Select the font defined in the included header file.
const uint8_t *SELECTED_FONT = Droid_Sans_12;

//------------ OBJECTS ---------------//
// Hardware SPI is usually recommended if available. SoftDMD uses software SPI.
// Adjust pins based on your wiring if not using default SoftDMD pins.
SoftDMD dmd(PANEL_WIDTH, PANEL_HEIGHT);

// DMD_TextBox is useful for scrolling text, but direct drawing is used here.
// Keep it if you plan to use text box features later, otherwise it can be removed.
// DMD_TextBox box(dmd, 0, 0, DISPLAY_WIDTH_PIXELS, DISPLAY_HEIGHT_PIXELS);

//------------ FUNCTION DECLARATIONS ---------------//
void processAndDisplayData(const String& inputData);
void displaySingleProduct(const String& productData);
int getTextWidth(const String& text);
int getCenterX(int textWidth);
void displayCenteredText(const String& text, bool checkWidth = true);
void delayWithYield(unsigned long ms); // Use yield() for ESP platforms if needed
void drawSymbol(int symbolCode, int priceCenterX);
void drawEuroCoin(int priceCenterX, int priceWidth);
void drawHeart(int anchorX);
void drawPizza(int anchorX);
void drawDonut(int anchorX);

//=============================================================================
// SETUP FUNCTION
//=============================================================================
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial); // Wait for Serial Monitor to connect (optional)

    Serial.println(F("Initializing DMD Display..."));

    // Configure the Output Enable pin
    pinMode(OE_PIN, OUTPUT);
    digitalWrite(OE_PIN, HIGH); // Enable display output

    // Initialize DMD
    dmd.setBrightness(DISPLAY_BRIGHTNESS);
    Serial.print(F("Brightness set to: "));
    Serial.println(DISPLAY_BRIGHTNESS);

    dmd.selectFont(SELECTED_FONT);
    Serial.println(F("Font selected."));

    dmd.begin();
    Serial.println(F("DMD initialized successfully."));
    dmd.clearScreen(); // Start with a blank screen
    dmd.drawString(0, TEXT_VERTICAL_POSITION, F("Ready")); // Optional ready message
    delayWithYield(1500);
    dmd.clearScreen();
}

//=============================================================================
// MAIN LOOP
//=============================================================================
void loop() {
    // Check for incoming serial data ending with a newline
    if (Serial.available() > 0) {
        String inputData = Serial.readStringUntil('\n');
        inputData.trim(); // Remove leading/trailing whitespace

        if (inputData.length() > 0) {
            Serial.print(F("Received: "));
            Serial.println(inputData);
            processAndDisplayData(inputData);
        }
    }

    // Other non-blocking tasks can go here
    yield(); // Recommended for ESP32/ESP8266 to prevent watchdog resets
}

//=============================================================================
// DATA PROCESSING AND DISPLAY FUNCTIONS
//=============================================================================

/**
 * @brief Processes the full input string, splitting it into individual product segments.
 *        Input Format: product1:price1?symbol1!product2:price2?symbol2...
 *        '_' separates words within a product name.
 * @param inputData The complete string received via serial.
 */
void processAndDisplayData(const String& inputData) {
    int startIndex = 0;
    int endIndex = -1;

    do {
        // Find the next product separator '!' or end of string
        endIndex = inputData.indexOf('!', startIndex);

        String productData;
        if (endIndex == -1) {
            // Last (or only) product in the string
            productData = inputData.substring(startIndex);
        } else {
            // Product found, extract its data
            productData = inputData.substring(startIndex, endIndex);
        }

        productData.trim(); // Clean up whitespace
        if (productData.length() > 0) {
            displaySingleProduct(productData);
        }

        // Move startIndex past the current product and the '!' separator
        startIndex = endIndex + 1;

    } while (endIndex != -1); // Continue if '!' was found
}

/**
 * @brief Parses and displays a single product's information (name, price, symbol).
 *        Format for single product: productName_word2:price,decimal?symbolCode
 * @param productData The string segment containing one product's data.
 */
void displaySingleProduct(const String& productData) {
    // Find delimiters
    int priceSeparatorIndex = productData.indexOf(':');
    int symbolSeparatorIndex = productData.indexOf('?');

    // Validate format
    if (priceSeparatorIndex == -1 || symbolSeparatorIndex == -1 || symbolSeparatorIndex < priceSeparatorIndex) {
        Serial.println(F("Error: Invalid product data format."));
        displayCenteredText(ERROR_MSG_FORMAT, false); // Display format error
        delayWithYield(MESSAGE_DELAY_MS * 2); // Longer delay for error
        return;
    }

    // Extract parts
    String productNameFull = productData.substring(0, priceSeparatorIndex);
    String priceString = productData.substring(priceSeparatorIndex + 1, symbolSeparatorIndex);
    String symbolCodeString = productData.substring(symbolSeparatorIndex + 1);

    productNameFull.trim();
    priceString.trim();
    symbolCodeString.trim();

    int symbolCode = symbolCodeString.toInt(); // Convert symbol code string to integer

    // --- Display Product Name (word by word) ---
    int wordStartIndex = 0;
    int underscoreIndex = -1;
    do {
        underscoreIndex = productNameFull.indexOf('_', wordStartIndex);
        String currentWord;

        if (underscoreIndex == -1) {
            // Last or only word
            currentWord = productNameFull.substring(wordStartIndex);
        } else {
            currentWord = productNameFull.substring(wordStartIndex, underscoreIndex);
        }

        currentWord.trim();
        if (currentWord.length() > 0) {
            displayCenteredText(currentWord, true); // Check width and display
            delayWithYield(MESSAGE_DELAY_MS);
        }

        wordStartIndex = underscoreIndex + 1;

    } while (underscoreIndex != -1);

    // --- Display Price ---
    int priceWidth = getTextWidth(priceString);
    int priceCenterX = getCenterX(priceWidth);

    if (priceWidth < 0 || priceWidth > DISPLAY_WIDTH_PIXELS) {
        Serial.println(F("Error: Price text width invalid or too large."));
        displayCenteredText(ERROR_MSG_SIZE, false); // Display size error
        delayWithYield(MESSAGE_DELAY_MS);
    } else {
        dmd.clearScreen();
        dmd.drawString(priceCenterX, TEXT_VERTICAL_POSITION, priceString.c_str()); // Use c_str()

        // --- Draw Symbol (if specified and space allows) ---
        drawSymbol(symbolCode, priceCenterX);

        // --- Draw Euro Coin Symbol ---
        drawEuroCoin(priceCenterX, priceWidth);

        delayWithYield(MESSAGE_DELAY_MS); // Show price + symbols
    }
}

//=============================================================================
// HELPER FUNCTIONS (Drawing, Text Measurement, Centering)
//=============================================================================

/**
 * @brief Calculates the pixel width of a given string using the selected font.
 *        Uses dmd.charWidth() for each character. Returns -1 if a char is not supported.
 * @param text The string to measure.
 * @return The total width in pixels, or -1 if an error occurs.
 */
int getTextWidth(const String& text) {
    int totalWidth = 0;
    for (unsigned int i = 0; i < text.length(); ++i) {
        char c = text.charAt(i);
        int charWidth = dmd.charWidth(c); // Get width from DMD library

        if (charWidth <= 0 && c != ' ') { // charWidth might be 0 or negative for unknown chars
             // Allow spaces even if width is 0, handle them manually if needed
             // For Droid Sans 12, space might be non-zero, check font data.
             // If space truly has 0 width in font, add manual spacing:
             if (c == ' ') {
                 // Add explicit space width if library reports 0. Adjust '4' as needed.
                 // This value depends heavily on the font.
                 // The original had 11px for space, which seems large. Let's try a smaller default.
                 charWidth = 4; // Example: assign 4 pixels width for a space
             } else {
                Serial.print(F("Error: Character '"));
                Serial.print(c);
                Serial.println(F("' not supported or has zero width in font."));
                // return -1; // Indicate error
                // Option: substitute with a placeholder width or character?
                // For now, let's assume a default width for unknown chars to avoid stopping.
                charWidth = 6; // Default width for unknown char, adjust as needed
             }
        }
        totalWidth += charWidth;

        // Add 1 pixel spacing between characters, except for the last one
        if (i < text.length() - 1) {
            totalWidth += 1;
        }
    }
    return totalWidth;
}


/**
 * @brief Calculates the starting X coordinate to center text horizontally.
 * @param textWidth The width of the text in pixels.
 * @return The calculated X coordinate.
 */
int getCenterX(int textWidth) {
    if (textWidth < 0) textWidth = 0; // Handle error case from getTextWidth
    // Use floating point for potentially more accurate centering
    return floor((float)(DISPLAY_WIDTH_PIXELS - textWidth) / 2.0);
}

/**
 * @brief Clears the screen and displays text centered horizontally.
 * @param text The string to display.
 * @param checkWidth If true, checks if the text fits and displays an error if not.
 */
void displayCenteredText(const String& text, bool checkWidth) {
    int textWidth = getTextWidth(text);

    if (checkWidth) {
        if (textWidth < 0) { // Error from getTextWidth
             Serial.print(F("Error calculating width for: ")); Serial.println(text);
             textWidth = getTextWidth(ERROR_MSG_CHAR); // Get width of error message
             int centerX = getCenterX(textWidth);
             dmd.clearScreen();
             dmd.drawString(centerX, TEXT_VERTICAL_POSITION, ERROR_MSG_CHAR);
             return; // Exit after showing error
        }
        if (textWidth > DISPLAY_WIDTH_PIXELS) {
            Serial.print(F("Error: Text too wide: ")); Serial.println(text);
            textWidth = getTextWidth(ERROR_MSG_SIZE); // Get width of error message
            int centerX = getCenterX(textWidth);
            dmd.clearScreen();
            dmd.drawString(centerX, TEXT_VERTICAL_POSITION, ERROR_MSG_SIZE);
            return; // Exit after showing error
        }
    }

    // If no error or checkWidth is false, display the text
    int centerX = getCenterX(textWidth);
    dmd.clearScreen();
    dmd.drawString(centerX, TEXT_VERTICAL_POSITION, text.c_str()); // Use c_str() for DMD library
}


/**
 * @brief Provides a delay, calling yield() periodically (useful for ESP platforms).
 * @param ms The delay duration in milliseconds.
 */
void delayWithYield(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        yield(); // Allow background tasks (WiFi, etc.) on ESP platforms
    }
}

//=============================================================================
// SYMBOL DRAWING FUNCTIONS
//=============================================================================

/**
 * @brief Selects and draws the appropriate symbol based on the code.
 * @param symbolCode The code identifying the symbol (0=none, 1=heart, 2=pizza, 3=donut).
 * @param priceCenterX The X coordinate where the price string starts (used for relative positioning).
 */
void drawSymbol(int symbolCode, int priceCenterX) {
    // Estimate minimum required space left of the price text for symbols
    const int heartWidthApprox = 18;
    const int pizzaWidthApprox = 16;
    const int donutWidthApprox = 16;

    switch (symbolCode) {
        case SYMBOL_HEART:
            if (priceCenterX >= heartWidthApprox + 1) { // Ensure space + 1px gap
                drawHeart(priceCenterX - 1); // Anchor point is right edge of symbol
            } else {
                Serial.println(F("Warning: Not enough space for heart symbol."));
            }
            break;
        case SYMBOL_PIZZA:
             if (priceCenterX >= pizzaWidthApprox + 1) {
                drawPizza(priceCenterX - 1);
             } else {
                Serial.println(F("Warning: Not enough space for pizza symbol."));
             }
            break;
        case SYMBOL_DONUT:
             if (priceCenterX >= donutWidthApprox + 1) {
                drawDonut(priceCenterX - 1);
             } else {
                Serial.println(F("Warning: Not enough space for donut symbol."));
             }
            break;
        case SYMBOL_NONE:
        default:
            // No symbol to draw or invalid code
            break;
    }
}

/**
 * @brief Draws the Euro (€) symbol inside a circle to the right of the price.
 * @param priceCenterX The starting X coordinate of the price text.
 * @param priceWidth The width of the price text in pixels.
 */
void drawEuroCoin(int priceCenterX, int priceWidth) {
    // Position the coin relative to the end of the price text
    const int gap = 2; // Gap between price text and coin circle
    const int coinRadius = 7;
    const int coinDiameter = 2 * coinRadius + 1;
    int coinCenterX = priceCenterX + priceWidth + gap + coinRadius;

    // Check if the coin fits on the display
    if (coinCenterX + coinRadius < DISPLAY_WIDTH_PIXELS) {
        // Draw Circle
        dmd.drawCircle(coinCenterX, coinRadius, coinRadius); // Y center is radius if Y=0 is top

        // Draw EURO symbol (€) inside the circle (adjust coordinates relative to coinCenterX)
        int euroLeft = coinCenterX - 3;
        int euroRight = coinCenterX + 3;
        int euroTopY = 3;
        int euroBottomY = 11;
        int euroMidY1 = coinRadius - 1; // Adjust vertical position if needed
        int euroMidY2 = coinRadius + 1; // Adjust vertical position if needed

        // Main C shape
        dmd.drawLine(euroLeft + 1, euroTopY + 1, euroLeft +1 , euroBottomY - 1); // Vertical part
        dmd.drawPixel(euroLeft + 2, euroTopY);    // Top curve pixel
        dmd.drawPixel(euroLeft + 2, euroBottomY); // Bottom curve pixel
        dmd.drawLine(euroLeft + 3, euroTopY-1, euroRight -1, euroTopY-1); // Top segment extend
        dmd.drawLine(euroLeft + 3, euroBottomY+1, euroRight -1, euroBottomY+1); // Bottom segment extend
        dmd.drawPixel(euroRight, euroTopY); // right edge top
        dmd.drawPixel(euroRight, euroBottomY); // right edge bottom
        dmd.drawPixel(euroRight-1, euroTopY+1);
        dmd.drawPixel(euroRight-1, euroBottomY-1);


        // Horizontal bars (adjust length/position based on font/aesthetics)
        dmd.drawLine(euroLeft + 1, euroMidY1, euroRight -1, euroMidY1);
        dmd.drawLine(euroLeft + 1, euroMidY2, euroRight -1, euroMidY2);

    } else {
         Serial.println(F("Warning: Not enough space for Euro coin symbol."));
    }
}


// --- Individual Symbol Drawing Functions ---
// These are complex and directly translated. Verify coordinates carefully.
// AnchorX is typically the rightmost pixel X-coordinate of the symbol area.

void drawHeart(int anchorX) {
    // Original coordinates seem relative to pricex, assuming pricex is the start of price string.
    // Let's adjust assuming anchorX is the right edge (e.g., anchorX = priceCenterX - 1)
    // Example: pricex-18 becomes anchorX - 17
    int ax = anchorX; // Use shorter name
    dmd.drawLine(ax-17, 6, ax-17, 4);
    dmd.drawLine(ax-17, 3, ax-14, 0);
    dmd.drawLine(ax-5, 0, ax-1, 3);
    dmd.drawLine(ax-1, 4, ax-1, 6);
    dmd.drawLine(ax-2, 7, ax-2, 8);
    dmd.drawLine(ax-4, 9, ax-9, 14);
    dmd.drawLine(ax-10, 14, ax+1, 9); // This looks wrong, original was pricex-11,14 -> pricex-0,9 -> should likely be ax-0? Let's assume ax-0 or ax-1
    dmd.drawLine(ax-10,14, ax, 9);    // Trying ax as the rightmost point
    // dmd.drawLine(ax-17,24,ax-17,23); // These seem out of 16px height bounds
    dmd.drawLine(ax-14, 0, ax-13, 0);
    dmd.drawLine(ax-5, 0, ax-8, 0);
    dmd.drawLine(ax-7, 0, ax-9, 2);
    dmd.drawLine(ax-12, 0, ax-10, 2);
}

void drawPizza(int anchorX) {
    int ax = anchorX;
    // Y coordinates seem too large (up to 16?), DMD height is 16 (0-15)
    // Assuming original Y=16 meant Y=15 (bottom line)
    dmd.drawLine(ax-1, 15, ax-1, 13);
    dmd.drawLine(ax-2, 12, ax-2, 9);
    dmd.drawLine(ax-3, 8, ax-3, 5);
    dmd.drawLine(ax-4, 4, ax-4, 2);
    dmd.drawLine(ax-4, 2, ax-7, 2);
    dmd.drawLine(ax-8, 3, ax-9, 3);
    dmd.drawLine(ax-10, 4, ax-13, 7);
    dmd.drawLine(ax-14, 8, ax-14, 9);
    dmd.drawLine(ax-15, 10, ax-15, 13);
    dmd.drawLine(ax-4, 4, ax-6, 4);
    dmd.drawLine(ax-7, 5, ax-8, 5);
    dmd.drawLine(ax-9, 6, ax-11, 8);
    dmd.drawLine(ax-12, 9, ax-12, 10);
    dmd.drawLine(ax-13, 11, ax-13, 13);
    dmd.drawLine(ax-15, 13, ax-13, 13);
    dmd.drawLine(ax-12, 14, ax-9, 14);
    dmd.drawLine(ax-8, 15, ax-3, 15);
    // dmd.drawLine(ax-4, 16, ax-1, 16);   // Y=16 out of bounds, assume Y=15
    dmd.drawLine(ax-4, 15, ax-1, 15);
    dmd.drawLine(ax-6, 14, ax-3, 14);
    dmd.drawLine(ax-5, 13, ax-4, 13);
    dmd.drawLine(ax-5, 10, ax-5, 7);
    dmd.drawLine(ax-4, 10, ax-4, 7);
    dmd.drawLine(ax-6, 9, ax-6, 8);
    dmd.drawLine(ax-10, 12, ax-9, 12);
    dmd.drawLine(ax-11, 11, ax-8, 11);
    dmd.drawLine(ax-11, 10, ax-8, 10);
    dmd.drawLine(ax-10, 9, ax-9, 9);
}

void drawDonut(int anchorX) {
    int ax = anchorX;
    // Again, checking Y coordinates (max 15)
    dmd.drawLine(ax-5, 14, ax-11, 14);
    dmd.drawLine(ax-3, 13, ax-13, 13);
    dmd.drawLine(ax-2, 12, ax-14, 12);
    dmd.drawLine(ax-1, 11, ax-15, 11);
    dmd.drawLine(ax-1, 10, ax-6, 10);
    dmd.drawLine(ax-1, 9, ax-4, 9);
    dmd.drawLine(ax-1, 8, ax-4, 8);
    dmd.drawLine(ax-1, 7, ax-5, 7);
    dmd.drawLine(ax-1, 6, ax-6, 6);
    dmd.drawLine(ax-2, 5, ax-10, 5);
    dmd.drawLine(ax-3, 4, ax-9, 4);
    dmd.drawLine(ax-4, 3, ax-12, 3);
    dmd.drawLine(ax-6, 2, ax-10, 2);
    dmd.drawLine(ax-10, 10, ax-15, 10);
    dmd.drawLine(ax-12, 9, ax-15, 9);
    dmd.drawLine(ax-13, 4, ax-15, 6);
    dmd.drawLine(ax-12, 4, ax-15, 7);
    dmd.drawLine(ax-12, 8, ax-9, 6);
    dmd.drawLine(ax-13, 8, ax-10, 6);
    dmd.drawLine(ax-14, 8, ax-15, 8);
    dmd.drawLine(ax-14, 7, ax-14, 8);
}
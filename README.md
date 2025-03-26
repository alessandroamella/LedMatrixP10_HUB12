# Arduino P10 DMD Serial Text Display

This Arduino sketch controls one or more P10 LED matrix panels (configured as 2 wide x 1 high in the code) using the DMD2 library. It listens for specifically formatted strings sent over the Serial connection and displays the content (product names, prices, and optional symbols) centered on the panels.

## Info / Requirements

- **Microcontroller:** Arduino (Tested primarily on **Arduino Nano**). May work on other boards compatible with the DMD2 library (like Uno, ESP8266/ESP32 with pin adjustments) but not explicitly tested.
- **Display:** P10 LED Matrix Panel(s). The code is configured for **2 panels horizontally and 1 vertically (64x16 pixels total)**. Adjust `PANEL_WIDTH` and `PANEL_HEIGHT` constants in the code if your setup differs.
- **Library:** [`DMD2`](https://github.com/freetronics/DMD2) library must be installed in your Arduino IDE.
- **Font File:** Requires a font file compatible with DMD2 (e.g., `fonts/Droid_Sans_12.h` as used in the code). Ensure the `#include` path in the code points to a valid font file accessible by your sketch.
- **Wiring:** Connect the P10 panel(s) to your Arduino according to the DMD2 library documentation or the pin configuration used in `SoftDMD dmd(...)`. The code uses `SoftDMD`, check its pin requirements or modify for hardware SPI if preferred. Pin 9 is specifically used for Output Enable (`OE_PIN`).

## How to Use

1.  **Setup:**
    - Install the DMD2 library.
    - Ensure you have the required font file (e.g., `Droid_Sans_12.h`) located correctly relative to your sketch or within the library path.
    - Wire the P10 panel(s) and the OE pin (Pin 9 by default) to your Arduino.
2.  **Upload:** Compile and upload the sketch to your Arduino board.
3.  **Serial Monitor:** Open the Arduino IDE's Serial Monitor (or any other serial terminal).
    - Set the baud rate to **9600**.
    - Ensure the line ending is set to **Newline** (`\n`).
4.  **Send Data:** Type a correctly formatted string (see format below) into the Serial Monitor's input field and press Enter/Send. The display should update accordingly.

## Serial Input Format

The sketch expects a single string via the Serial input, terminated by a **newline character (`\n`)**. This string contains information for one or more products to be displayed sequentially.

**Overall Format:**

```
productName1:price1?symbolCode1!productName2_word2:price2?symbolCode2!productName3:price3?symbolCode3
```

**Delimiters and Sections:**

- **`!` (Exclamation Mark):** Separates individual product entries. If you only send one product, this is not needed.
- **`_` (Underscore):** Used _within_ a `productName` to separate words that should be displayed sequentially on the screen _for the same product_. Each word will be displayed centered for a short duration before moving to the next word or the price. **Do not use spaces** in the product name; use underscores if multiple words are needed.
- **`:` (Colon):** Separates the `productName` section from the `price` section.
- **`?` (Question Mark):** Separates the `price` section from the optional `symbolCode`.
- **`\n` (Newline):** Marks the end of the entire command string (usually sent automatically when you press Enter in the Serial Monitor if configured correctly).

**Sections Explained:**

1.  **`productName`**

    - The name of the product.
    - If the name has multiple parts to display one after another (e.g., "Bomboloni" then "Crema"), separate them with `_` (e.g., `Bomboloni_Crema`).
    - Avoid using characters not present in the selected font file, as this may cause errors (`CHAR ERROR`).
    - If a single word is too long to fit the display width (64 pixels), it will show `SIZE ERROR`.

2.  **`price`**

    - The price of the product.
    - Use a comma `,` as the decimal separator (e.g., `1,20` or `15,69`).
    - This price string will be displayed centered, followed by a Euro coin symbol to its right.
    - If the price string is too long, it will show `SIZE ERROR`.

3.  **`symbolCode`**
    - An optional single digit number that specifies a symbol to draw to the _left_ of the price.
    - Available codes:
      - `0`: No symbol
      - `1`: Heart symbol ❤️
      - `2`: Pizza slice symbol 🍕
      - `3`: Donut symbol 🍩
    - If the code is missing, invalid, or `0`, no symbol will be drawn.
    - Note: The symbol might not be drawn if the price text starts too close to the left edge of the display, leaving insufficient space.

**Examples:**

- Display "Pizza", price "5,00", with a pizza symbol:
  ```
  Pizza:5,00?2
  ```
- Display "Coffee", price "2,50", no symbol:
  ```
  Coffee:2,50?0
  ```
- Display "BIG" then "MAC", price "7,50", no symbol:
  ```
  BIG_MAC:7,50?0
  ```
- Display "Donut", price "1,20" with donut symbol, immediately followed by "Water", price "1,00" with no symbol:
  ```
  Donut:1,20?3!Water:1,00?0
  ```

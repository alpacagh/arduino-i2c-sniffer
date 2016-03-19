/**
 * Software signal sniffer targeted for i2c (twi) protocol.
 *
 * Size limit
 * Sniffer is using internal buffer for data before transmitting to serial to enhance maximum gathering speed, so
 * maximum session length is limited by available avr memory. Keep in mind that one i2c transferred byte
 * (including address) is using about 11-14 bytes of sniffer buffer. So default buffer size is capable of intercepting
 * only a 100-120 bytes session (like single raspberry's i2cdetect).
 * If session collection is stopped by a buffer limit, last byte will be BUFFER_OVERFLOW_MARK.
 *
 * Speed limit
 * Maximum confirmed sniffing speed using nano 328p is 50kbps (driven by raspberry). Using 100kbps there was
 * lost intermediate states, so data becomes unrecoverable.
 *
 * source: https://github.com/alpacagh/arduino-i2c-sniffer
 * MIT license Copyright (c) 2016 alpacagm@gmail.com
 * https://opensource.org/licenses/MIT
 */

/** configuration section. normally no other changes should be required */
//#define USE_PULLUP                // uncomment to use internal PULLUP on input pins
#define PIN_SCL A2                  // pins must be on the same chip port (see datasheet or just bruteforce)
#define PIN_SDA A3
#define BYTE_SHIFT 'a'              // value to add to each state measure to avoid control characters in output
#define SERIAL_SPEED 57600          // serial output speed. does not affect measures quality
#define SESSION_BORDER 0x7FFF       // ~ 300ms at 16mHz - time limit for transfer session to be flushed to serial
#define BUFFER_SIZE 1500            // Size of session buffer, good value for my 328 chip
#define BUFFER_OVERFLOW_MARK 0x7    // value to be sent when session is terminated by buffer overflow

/** end of configuration */

#define MOD_WAIT 0
#define MOD_LISTEN 1
#define MOD_SEND 2

uint8_t status;
uint8_t bitmask;
volatile uint8_t *portAddr;
uint_fast32_t counter = 0;
uint8_t buffer[BUFFER_SIZE];
uint8_t *bufPointer = buffer;
uint8_t *bufEnd = buffer + BUFFER_SIZE - 1;
uint8_t mode = MOD_LISTEN;

/**
 * Faster implementation of digitalRead, given that we already know port address and common bitmask
 */
#define SCAN_PINS() (*portAddr & bitmask)

/**
 * Macro to avoid control-characters in transfers
 */
#ifdef BYTE_SHIFT
#define STATUS_TO_PRESENTATION(status) (status + BYTE_SHIFT)
#else
#define STATUS_TO_PRESENTATION(status) (status)
#endif

void setup() {
    Serial.begin(SERIAL_SPEED);
    Serial.println("!! Configuring..");
    // Pre-calculate port number and bitmask
    uint8_t scl_port = digitalPinToPort(PIN_SCL);
    uint8_t sda_port = digitalPinToPort(PIN_SDA);
    if (scl_port != sda_port) {
        Serial.println("!! Error: SDL and SDA pins must be within same port!");
    }
    portAddr = portInputRegister(scl_port);
    uint8_t scl_bit = digitalPinToBitMask(PIN_SCL);
    uint8_t sda_bit = digitalPinToBitMask(PIN_SDA);
    bitmask = scl_bit | sda_bit;
    TIMSK0 = 0; // disable timer interrupts

    // Print configuration
    Serial.print("!! Data mask is ");
    Serial.print(bitmask);
    Serial.print(" where SCL is ");
    Serial.print(scl_bit);
    Serial.print(" and SDA is ");
    Serial.print(sda_bit);
#ifdef BYTE_SHIFT
    Serial.println();
    Serial.print("!! Transfer is shifted by character '");
    Serial.print(BYTE_SHIFT);
    Serial.print('\'');
#endif
    Serial.println();
    Serial.print(">>");
    Serial.print(scl_bit);
    Serial.print(":");
    Serial.print(sda_bit);
#ifdef BYTE_SHIFT
    Serial.print(":");
    Serial.print(BYTE_SHIFT);
#endif
    Serial.println();

#ifdef USE_PULLUP
    pinMode(PIN_SDA, INPUT_PULLUP);
    pinMode(PIN_SCL, INPUT_PULLUP);
#else
    pinMode(PIN_SDA, INPUT);
    pinMode(PIN_SCL, INPUT);
#endif

    Serial.println("!! Processing signals.");
    hasUpdate();
    Serial.write(STATUS_TO_PRESENTATION(status));
}

/**
 * Check if status has been updated
 */
inline bool hasUpdate() {
    register uint8_t tmp = SCAN_PINS();
    if (status == tmp) return false;
    status = tmp;
    return true;
}

inline void writeBuf() {
    *bufPointer = status;
    ++bufPointer;
    if (bufPointer == bufEnd) {
        *bufPointer = BUFFER_OVERFLOW_MARK;
        ++bufPointer;
        mode = MOD_SEND;
    }
}

void loop() {
    switch (mode) {
        case MOD_WAIT:
            if (hasUpdate()) {
                counter = 0;
                mode = MOD_LISTEN;
                writeBuf();
            }
            break;
        case MOD_LISTEN:
            if (hasUpdate()) {
                counter = 0;
                writeBuf();
            }
            if (++counter == SESSION_BORDER) {
                mode = MOD_SEND;
            }
            break;
        case MOD_SEND:
            for (uint8_t *ptr = buffer; ptr < bufPointer; ++ptr) {
                Serial.write(STATUS_TO_PRESENTATION(*ptr));
            }
            Serial.flush();
            Serial.println();
            hasUpdate();
            Serial.write(STATUS_TO_PRESENTATION(status));
            mode = MOD_WAIT;
            bufPointer = buffer;
            break;
    }
}

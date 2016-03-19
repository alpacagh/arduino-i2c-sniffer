# arduino-i2c-sniffer
Software implemented i2c/twi sniffer for arduino using python to visualize intercepted data.

Using nano 328p: 

- 50khz maximum speed, (100khz is common standard)
- 100-150 bytes per single transfer session.

See `sample_data.html` for output examples.

Output for each session looks like

![Example](https://cloud.githubusercontent.com/assets/670789/13896574/795b7058-eda1-11e5-8606-5a803e8fa34d.png)

## Motivation
Hardware oscilloscopes and signal analyzers are too expensive.

## Usage


### 1. collecting
- upload sketch to arduino board
- connect board ground pin to analyzed bus ground
- connect A2 pin to analyzed scl bus, A3 pin to sda bus
- open serial monitor using 57600 bitrate

### 2. analyzing
- save serial monitor data into text file (like sample_data.txt)
- run `python2 convertToHtml.py >sample_data.html`
- open `sample_data.html` in browser

## Description

### Arduino sketch twi_sniff - data collector

Data collected on signal line changes. So no consecutive equal 
states possible.
 
Sniffer itself is working in sessions mode. 

1. it gathers data from pins until buffer overflow or inactivity timeout
      - no serial activity during this period
2. it encodes and sends gathered data to serial port
      - no sampling during this period
      
This allows to expect more predictable sampling rate and achieve bigger 
supported data rates.

Sniffer require so both pins must be on the same avr GPIO port, so only 
one sampling is performed for both pins (and it is more-or-less atomic).
See datasheet or just bruteforce options to find acceptable pin pair.
Or use pre-configured A2+A3.

### Data visualization converter

- `convertToHtml.py` python2 converter
- `style.css` common css file for generated html
- `svg/*` source files for css in-line backgrounds

You may use `convertToHtml.py` as module to create your more nice data 
converter or even implement direct serial connection with arduino,
 
If nothing broken, every data session is converted into html table with
- row with visualization of SDA signal levels
- row with visualization of SCL signal levels
- row with signal interpretation by I2C/TWI protocol (S - start, E - end)
    
### Examples

`sample_data.txt` and `sample_data.txt` contain commented test 
data gathered by me.
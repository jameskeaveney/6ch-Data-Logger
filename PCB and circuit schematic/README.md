Schematic and PCB layout produced using RS DesignSpark v7.0 (http://www.rs-online.com/designspark/electronics/eng/page/designspark-pcb-home-page)

Circuit is essentially takes all inputs through unity gain buffers (as a crude over-voltage protection for the arduino ADC pins). Output goes through Arduino DUE DAC0 into a potential divider and fixed gain amplifier, for 0-5V operation.

All components are through-hole, for ease of soldering.
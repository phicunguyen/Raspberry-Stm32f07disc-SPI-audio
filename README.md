# CubeMx-Stm32f07disc-and-Raspberry-PI4-SPI-audio
Play an audio wave file over SPI interface using Raspberry Pi4 as spi-master and stm32f407disc as spi-slave. 
The stm32f407 is generated using Cubemx.

Below is the spi pins connection between Raspberry and Stm32:

             Raspberry   Stm32f407-disc
     MOSI      P1-19        PC3
     MISO      P1-21        PC2
     SCLK      P1-23        PB10
     CE0       P1-24        PB12
     INT       P1-29        PB13
     GND       P1-25        GND
     
The code was first written in python using the python spidev but I couldn't get the spi clock speed to go over 2*125000. 
When spi speed is set higher than that then spi clock does not toggle ann more.

Then move to c using the "/dev/spidev0.0" to transfer audio data between Raspberry and Stm32. Here I can only get spi clock up
to 3.2Mhz. If the spi clock set higher then 3.2Mhz then spi clock does not toggle any more. 

This code was based on the this https://www.youtube.com/watch?v=6g2jSqvmpt4 and stm32f407 Audio_playback_and_record. 
Both code were play audio from a sdcard but I don't have one therefore I changed the code to play using spi Raspberry.

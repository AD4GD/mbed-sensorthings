# mbed_sensorthings

A very simple example on how to send data from an IoT device to a FROST server.

## Getting started
This tiny project is based on the tutorial "[ARM mbed LPC1768: Connect to SensorUp SensorThings](https://developers.sensorup.com/tutorials/mbed/)". The original tutorial permits to send the temperature to a SensorUp SensorThings Playground server from a mbed application board. The source code of the tutorial was modified to replace the temperature sensor by a generator of random numbers, the sending of data to a FROST server hosted on the IoT Lab infrastructure, the support to use the [Keil Studio Cloud](https://studio.keil.arm.com/) compiler. This compiler is free and available online, without the need to install it locally on a computer. It is a good alternative to Keil uVision5 compiler, which requires a local installation and a license to get all the features.

## Modifications of the source code
The source code of this project can be modified by uploading the project on [Keil Studio Cloud](https://studio.keil.arm.com/). Keil Studio Cloud is using Mercurial as distributed version control: so, do all the modifications only through Keil Studio Cloud.

## Upload the build file on a mbed LPC1768 board
Follow the following steps to upload the build file provided here on a mbed board:
1.  Download the built file on your computer.
1.  Connect the mbed board on an USB port of your computer.
1.  Copy and paste the `mbed_sensorthings.LPC1768.bin` file from your computer to the mbed board. The mbed board is seen as an USB stick.

## Authors and acknowledgment
Cédric Crettaz (IoT Lab), based on the work done by [Robin Luo](https://os.mbed.com/users/robinlk/).

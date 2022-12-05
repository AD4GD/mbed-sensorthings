/*
 * This content is released under the (https://opensource.org/licenses/MIT) MIT License.
 *
 * Simple code to upload temperature readings to SensorUp SensorThings Playground (http://pg.sensorup.com)
 * from the LM75B temperature sensor(https://developer.mbed.org/cookbook/LM75B-Temperature-Sensor)   
 * in an mbed application board(https://developer.mbed.org/cookbook/mbed-application-board)
 * It works with mbed LPC1768. (https://developer.mbed.org/platforms/mbed-LPC1768/)
 */
#include "mbed.h"
//#include "EthernetNetIf.h"
#include "EthernetNetIf/LPC1768/if/eth/EthernetNetIf.h"
//#include "TCPSocket.h"
#include "EthernetNetIf/LPC1768/api/TCPSocket.h"
#include "TinyHTTP.h"
#include "LM75B.h"
#include "C12832.h"
#include "wait_api.h"

/*
* Change the DATASTREAM_ID_TEMP to the id of you SensorThigns Datastream.
* You can get the Datastream <id> from the SensorUp playground's Observation
* API Request:/st-playground/proxy/v1.0/Datastreams(<id>)/Observations
*/
const int DATASTREAM_ID_TEMP = 1;
/*
* Change the ACCESS_TOKEN to the token of you SensorThigns Datastream
* You can get the ACCESS_TOKEN from the SensorUp playground's Observation 
* API Request: St-P-Access-Token: 78cc7eb2-9394-4675-b231-ff0a377a3674
*/
const char *ACCESS_TOKEN = "78cc7eb2-9394-4675-b231-ff0a377a3674";
/*
* Interval(second) to post temperature
*/
const int INTERVAL  = 5;
// Temperature sensor
//LM75B sensor(p28,p27);
//Ethernet network interface
EthernetNetIf eth;
//LCD
//C12832 lcd(p5, p7, p6, p8, p11);
// Analog input
AnalogIn analog (p20);

int postToServer(float temp)
{
    Host host;
	char msg[50],uri[100], head[160];
	// header
	snprintf(head, sizeof(head), "Content-type: application/json\r\n");
	// uri
	snprintf(uri, sizeof(uri), "/FROST-Server/v1.1/Datastreams(%d)/Observations", DATASTREAM_ID_TEMP);
	// msg
	snprintf(msg, sizeof(msg), "{\"result\":%.3f\n}", temp);

	host.setName("10.81.2.66");
	host.setPort(8090);
	return httpRequest(METHOD_POST, &host, uri, head, msg);
}

// Random number generator
unsigned int randomGenerator(void)
{
    unsigned int x = 0;
    unsigned int random = 0;

    for(x = 0; x <= 32; x += 2)
    {
        random += ((analog.read_u16() % 3) << x);
        wait_us(10);
    }

    return random;
}

int main () {
    EthernetErr ethErr;
    Host host;
    int r;
	// Start the Ethernet connection
    ethErr = eth.setup();
    if(ethErr) {
        printf("connect error\r\n");
        return -1;
    }
	printf("start\r\n");

 	//Try to open the LM75B
    //if (sensor.open()) {
    //    printf("Device detected!\n");

    while (1) {
        //lcd.cls();
        //lcd.locate(0,3);
        //Display the air temperature on LCD
        //The returned temperature is in degrees Celcius.
        //lcd.printf("Temp = %.3f\n", (float)sensor);
        // Generate a random number for the payload to be sent
        unsigned int sensor = randomGenerator();
        // Post temperature to SensorUp SensorThings Playground
        r = postToServer((float)sensor);
        printf("status %d\r\n", r);
        wait(INTERVAL);
    }
    //} else {
    //    error("Device not detected!\n");
    //}
    return 0;
}

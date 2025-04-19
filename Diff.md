
### 1. Packet Structure

- **First version**   
  ```cpp
  struct dataPacket {
      uint32_t counter = 0;
  };
  dataPacket* helloPacket = new dataPacket;
  ```
  - Only carries a single 32‑bit counter; no need for a type tag or union.
  - Only one packet buffer is allocated (no separate return packet).

- **Second version**   
  ```cpp
  struct dataPacket {
      int type = 0; // 0 for counter, 1 for wan packet
      union {
          uint32_t counter = 0;
          uint8_t data[4];
      } data;
  };
  dataPacket* helloPacket = new dataPacket;
  dataPacket* returnPacket = new dataPacket;
  ```
  - Includes a `type` field to distinguish between “counter” packets and raw WAN data.
  - Uses a `union` so the same 4‑byte payload can be interpreted either as a `uint32_t` counter or a 4‑byte array.
  - Allocates both `helloPacket` (outbound) and `returnPacket` (for gateway responses).

---

### 2. Packet Processing & Printing

- **First version**   
  - `printPacket(dataPacket data)` unconditionally prints `"Hello Counter received nº <counter>"`.
  - `printDataPacket(...)` shows source and size, then iterates over the single `counter` field.

- **Second version**   
  - `printPacket(dataPacket data, uint16_t src)` branches on `data.type`:
    - **Type 0**: prints `DATA:<counter> <src>`  
    - **Type 1**: dumps raw `data.data[i]` bytes.
  - `printDataPacket(...)` shows both the source address and payload size before iterating.

---

### 3. Gateway vs. Node Logic

- **First version**   
  - No gateway differentiation—every device simply calls:  
    ```cpp
    radio.createPacketAndSend(BROADCAST_ADDR, helloPacket, 1);
    ```
  - Simpler “everyone is a node” model, always broadcasting.

- **Second version**   
  - **Conditional compilation** on `IS_GATEWAY`:  
    - **Gateway** side reads 7 bytes from `Serial`, packages them into `returnPacket`, and sends to a specified node address.  
    - **Node** side finds the closest gateway (`getClosestGateway()`) and sends either to that address or does a broadcast.
  - Optionally adds a gateway role via `radio.addGatewayRole()`.

---

### 4. LoRaMesher Configuration

- **First version**   
  - Hard‑codes `config.loraCs = 18;` etc., and always uses `SX1276_MOD`.  
  - Omits any `SPI.begin` call or `HELTEC` checks.

- **Second version**   
  - Uses `LoraMesherConfig` with conditional pin/SPI setup based on `HELTEC` macro.  
  - Chooses between `SX1262_MOD` or `SX1276_MOD` depending on board.
  - Calls `SPI.begin(...)` explicitly for Heltec boards.

---

### 5. Task Setup & Initialization Order

- **First version**   
  1. `radio.begin(config)`  
  2. `createReceiveMessages()`  
  3. `radio.setReceiveAppDataTaskHandle(...)`  
  4. `radio.start()`

- **Second version**   
  1. `radio.begin(config)`  
  2. `createReceiveMessages()` → creates FreeRTOS task, then calls  
     `radio.setReceiveAppDataTaskHandle(...)`  
  3. `radio.start()`  
  4. Optionally `radio.addGatewayRole()`

---

## Why These Differences Matter

1. **Flexibility vs. Simplicity**  
   - The **second version** (original first file) is built for a dual‑role network: gateways that receive serial commands and return data, and nodes that send counters to the closest gateway or broadcast. Its packet type tagging and union allow mixing “counter” and raw data packets.  
   - The **first version** (original second file) assumes a flat, broadcast‑only network where every device is identical. Fewer features but much simpler code.

2. **Hardware Support**  
   - By checking `HELTEC` and selecting the correct SPI/pin setup and radio module, the **second version** can run on multiple board revisions (e.g. different TTGO T‑Beam and HELTEC boards).  
   - The **first version** hard‑wires one set of pins and SX1276, so it’s only suitable if you know exactly which hardware you have.

3. **Packet Handling & Debugging**  
   - Detailed printing in the **second version** helps diagnose both counter packets and arbitrary 4‑byte payloads, which is invaluable during development of gateway logic.  
   - The **first version**’s simpler printout is fine when all you care about is that node counters are arriving.

4. **Role Management**  
   - Adding `radio.addGatewayRole()` makes the **second version** actively participate in mesh routing as a gateway. Nodes can discover it and choose optimal paths.  
   - Without any gateway role, the **first version** can only broadcast, relying on range rather than routing.

```cpp
#include <Arduino.h>						#include <Arduino.h>
#include "LoraMesher.h"						#include "LoraMesher.h"

//Using LILYGO TTGO T-BEAM v1.1 				//Using LILYGO TTGO T-BEAM v1.1 
#define BOARD_LED   4						#define BOARD_LED   4
#define LED_ON      LOW						#define LED_ON      LOW
#define LED_OFF     HIGH					#define LED_OFF     HIGH

							      <
LoraMesher& radio = LoraMesher::getInstance();			LoraMesher& radio = LoraMesher::getInstance();

uint32_t dataCounter = 0;					uint32_t dataCounter = 0;
struct dataPacket {						struct dataPacket {
    int type = 0; // 0 for counter, 1 for wan packet	      |	    uint32_t counter = 0;
    union {						      <
        uint32_t counter = 0;				      <
        uint8_t data[4];				      <
    } data;						      <
};								};

dataPacket* helloPacket = new dataPacket;			dataPacket* helloPacket = new dataPacket;

dataPacket* returnPacket = new dataPacket;		      <
							      <
//Led flash							//Led flash
void led_Flash(uint16_t flashes, uint16_t delaymS) {		void led_Flash(uint16_t flashes, uint16_t delaymS) {
    uint16_t index;						    uint16_t index;
    for (index = 1; index <= flashes; index++) {		    for (index = 1; index <= flashes; index++) {
        digitalWrite(BOARD_LED, LED_ON);			        digitalWrite(BOARD_LED, LED_ON);
        delay(delaymS);						        delay(delaymS);
        digitalWrite(BOARD_LED, LED_OFF);			        digitalWrite(BOARD_LED, LED_OFF);
        delay(delaymS);						        delay(delaymS);
    }								    }
}								}

/**								/**
 * @brief Print the counter of the packet			 * @brief Print the counter of the packet
 *								 *
 * @param data							 * @param data
 */								 */
void printPacket(dataPacket data, uint16_t src) {	      |	void printPacket(dataPacket data) {
    if (data.type == 0)					      |	    Serial.printf("Hello Counter received nº %d\n", data.coun
    {							      <
        Serial.printf("DATA:%d %x\n", data.data.counter, src) <
        // Serial.printf("Hello Counte r received nº %d\n", d <
    }							      <
    else{						      <
        for(int i = 0; i < 4; i++)			      <
        {						      <
            Serial.println(data.data.data[i]);		      <
        }						      <
    }							      <
}								}

/**								/**
 * @brief Iterate through the payload of the packet and print	 * @brief Iterate through the payload of the packet and print
 *								 *
 * @param packet						 * @param packet
 */								 */
void printDataPacket(AppPacket<dataPacket>* packet) {		void printDataPacket(AppPacket<dataPacket>* packet) {
    Serial.printf("Packet arrived from %X with size %d\n", pa	    Serial.printf("Packet arrived from %X with size %d\n", pa

    //Get the payload to iterate through it			    //Get the payload to iterate through it
    dataPacket* dPacket = packet->payload;			    dataPacket* dPacket = packet->payload;
    size_t payloadLength = packet->getPayloadLength();		    size_t payloadLength = packet->getPayloadLength();
    uint16_t src = packet->src;				      <

    for (size_t i = 0; i < payloadLength; i++) {		    for (size_t i = 0; i < payloadLength; i++) {
        //Print the packet					        //Print the packet
        printPacket(dPacket[i], src);			      |	        printPacket(dPacket[i]);
    }								    }
}								}

/**								/**
 * @brief Function that process the received packets		 * @brief Function that process the received packets
 *								 *
 */								 */
void processReceivedPackets(void*) {				void processReceivedPackets(void*) {
    for (;;) {							    for (;;) {
        /* Wait for the notification of processReceivedPacket	        /* Wait for the notification of processReceivedPacket
        ulTaskNotifyTake(pdPASS, portMAX_DELAY);		        ulTaskNotifyTake(pdPASS, portMAX_DELAY);
        led_Flash(1, 100); //one quick LED flashes to indicat	        led_Flash(1, 100); //one quick LED flashes to indicat

        //Iterate through all the packets inside the Received	        //Iterate through all the packets inside the Received
        while (radio.getReceivedQueueSize() > 0) {		        while (radio.getReceivedQueueSize() > 0) {
            Serial.println("ReceivedUserData_TaskHandle notif	            Serial.println("ReceivedUserData_TaskHandle notif
            Serial.printf("Queue receiveUserData size: %d\n",	            Serial.printf("Queue receiveUserData size: %d\n",

            //Get the first element inside the Received User 	            //Get the first element inside the Received User 
            AppPacket<dataPacket>* packet = radio.getNextAppP	            AppPacket<dataPacket>* packet = radio.getNextAppP

            //Print the data packet				            //Print the data packet
            printDataPacket(packet);				            printDataPacket(packet);

            //Delete the packet when used. It is very importa	            //Delete the packet when used. It is very importa
            radio.deletePacket(packet);				            radio.deletePacket(packet);
        }							        }
    }								    }
}								}

TaskHandle_t receiveLoRaMessage_Handle = NULL;			TaskHandle_t receiveLoRaMessage_Handle = NULL;

/**								/**
 * @brief Create a Receive Messages Task and add it to the Lo	 * @brief Create a Receive Messages Task and add it to the Lo
 *								 *
 */								 */
void createReceiveMessages() {					void createReceiveMessages() {
    int res = xTaskCreate(					    int res = xTaskCreate(
        processReceivedPackets,					        processReceivedPackets,
        "Receive App Task",					        "Receive App Task",
        4096,							        4096,
        (void*) 1,						        (void*) 1,
        2,							        2,
        &receiveLoRaMessage_Handle);				        &receiveLoRaMessage_Handle);
    if (res != pdPASS) {					    if (res != pdPASS) {
        Serial.printf("Error: Receive App Task creation gave 	        Serial.printf("Error: Receive App Task creation gave 
    }								    }
							      <
    radio.setReceiveAppDataTaskHandle(receiveLoRaMessage_Hand <
}								}


/**								/**
 * @brief Initialize LoRaMesher					 * @brief Initialize LoRaMesher
 *								 *
 */								 */
void setupLoraMesher() {					void setupLoraMesher() {
    // Example on how to change the module. See LoraMesherCon |	    //Get the configuration of the LoRaMesher
    LoraMesher::LoraMesherConfig config;		      |	    LoraMesher::LoraMesherConfig config = LoraMesher::LoraMes
    #ifdef HELTEC					      |
        config.module = LoraMesher::LoraModules::SX1262_MOD;  |	    //Set the configuration of the LoRaMesher (TTGO T-BEAM v1
    #endif						      |	    config.loraCs = 18;
							      |	    config.loraRst = 23;
    #ifndef HELTEC					      |	    config.loraIrq = 26;
        config.module = LoraMesher::LoraModules::SX1276_MOD;  |	    config.loraIo1 = 33;
    #endif						      <
							      <
    config.loraCs = CS;					      <
    config.loraRst = RST;				      <
    config.loraIrq = IRQ;				      <
    config.loraIo1 = IO1;				      <
							      <
    #ifdef HELTEC					      <
        SPI.begin(9, 11, 10, CS); //Initialize SPI with the c <
        config.spi = &SPI;				      <
    #endif						      <

    //Init the loramesher with a processReceivedPackets funct |	    config.module = LoraMesher::LoraModules::SX1276_MOD;
							      >
							      >	    //Init the loramesher with a configuration
    radio.begin(config);					    radio.begin(config);

    //Create the receive task and add it to the LoRaMesher	    //Create the receive task and add it to the LoRaMesher
    createReceiveMessages();					    createReceiveMessages();

							      >	    //Set the task handle to the LoRaMesher
							      >	    radio.setReceiveAppDataTaskHandle(receiveLoRaMessage_Hand
							      >
    //Start LoRaMesher						    //Start LoRaMesher
    radio.start();						    radio.start();

    #ifdef IS_GATEWAY					      <
        radio.addGatewayRole();				      <
    #endif						      <
							      <
    Serial.println("Lora initialized");				    Serial.println("Lora initialized");
}								}


void setup() {							void setup() {
    Serial.begin(115200);					    Serial.begin(115200);

    Serial.println("initBoard");				    Serial.println("initBoard");
    pinMode(BOARD_LED, OUTPUT); //setup pin as output for ind	    pinMode(BOARD_LED, OUTPUT); //setup pin as output for ind
    led_Flash(2, 125);          //two quick LED flashes to in	    led_Flash(2, 125);          //two quick LED flashes to in
    setupLoraMesher();						    setupLoraMesher();
}								}


void loop() {							void loop() {
        for (;;) {					      |	    for (;;) {
							      >	        Serial.printf("Send packet %d\n", dataCounter);

            #ifdef IS_GATEWAY				      |	        helloPacket->counter = dataCounter++;
                if (Serial.available())			      <
                {					      <
                    uint8_t data[7];			      <
                    Serial.readBytes(data, 7);		      <
                    returnPacket->type = 1;		      <
                    // Serial.println("TO SEND: ");	      <
                    for(int i = 0; i < 4; i++)		      <
                    {					      <
                        returnPacket->data.data[i] = data[i]; <
                        // Serial.println(data[i]);	      <
                    }					      <
                    uint16_t dst = data[5] + 256 * data[4];   <
                    // Serial.println(dst);		      <
                    Serial.flush();			      <
                    radio.createPacketAndSend(dst, returnPack <
                }					      <
            #endif					      <
							      <
            #ifndef IS_GATEWAY				      <
                Serial.printf("Send packet %d\n", dataCounter <
							      <
                helloPacket->data.counter = dataCounter++;    <
                helloPacket->type = 0;			      <
                					      <
                RouteNode* dst = radio.getClosestGateway(); / <
                uint16_t addr;				      <
                if (dst != nullptr)			      <
                    addr = dst->networkNode.address; //Get th <
                else 					      <
                    addr = BROADCAST_ADDR;		      <
                Serial.printf("Sending packet to %X\n", addr) <
							      <
                //Create packet and send it.		      <
                // radio.createPacketAndSend(BROADCAST_ADDR,  <
                radio.createPacketAndSend(addr, helloPacket,  <
            #endif					      <

            //Wait 20 seconds to send the next packet	      |	        //Create packet and send it.
            vTaskDelay(20000 / portTICK_PERIOD_MS);	      |	        radio.createPacketAndSend(BROADCAST_ADDR, helloPacket
        }						      |
							      >	        //Wait 20 seconds to send the next packet
							      >	        vTaskDelay(20000 / portTICK_PERIOD_MS);
							      >	    }
}								}
```

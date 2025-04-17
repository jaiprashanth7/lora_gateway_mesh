/*******************************************************************************
* Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
* Copyright (c) 2018 Terry Moore, MCCI
*
* Permission is hereby granted, free of charge, to anyone
* obtaining a copy of this document and accompanying files,
* to do whatever they want with them without any restriction,
* including, but not limited to, copying, modification and redistribution.
* NO WARRANTY OF ANY KIND IS PROVIDED.
*
* This example sends a valid LoRaWAN packet with payload "Hello,
* world!", using frequency and encryption settings matching those of
* the The Things Network.
*
* This uses OTAA (Over-the-air activation), where where a DevEUI and
* application key is configured, which are used in an over-the-air
* activation procedure where a DevAddr and session keys are
* assigned/generated for use with all further communication.
*
* Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
* g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
* violated by this sketch when left running for longer)!


* To use this sketch, first register your application and device with
* the things network, to set or generate an AppEUI, DevEUI and AppKey.
* Multiple devices can use the same AppEUI, but each device has its own
* DevEUI and AppKey.
*
* Do not forget to define the radio type correctly in
* arduino-lmic/project_config/lmic_project_config.h or from your BOARDS.txt.
*
*******************************************************************************/


#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

// #ifdef CFG_us915
// #undef CFG_us915
// # define CFG_in866 1
// #endif


//
// For normal use, we require that you edit the sketch to replace FILLMEIN
// with values assigned by the TTN console. However, for regression tests,
// we want to be able to compile these scripts. The regression tests define
// COMPILE_REGRESSION_TEST, and in that case we define FILLMEIN to a non-
// working but innocuous value.
//
#ifdef COMPILE_REGRESSION_TEST
# define FILLMEIN 0
#else
# warning "You must replace the values marked FILLMEIN with real values from the TTN control panel!"
# define FILLMEIN (#dont edit this, edit the lines that use FILLMEIN)
#endif

// #include <heltec_unofficial.h>

bool hasJoined = false;
bool hasFoundDR = false;


// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
static const u1_t PROGMEM APPEUI[8]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}


// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8]={0x8a, 0xbf, 0xaa, 0xdd, 0x59, 0xb2, 0x2f, 0x7e}; //7E2FB259DDAABF8A
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}


// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
static const u1_t PROGMEM APPKEY[16] = {0xBC, 0xD0, 0x7B, 0x8B, 0x6C, 0x91, 0xB4, 0x62, 0xC3, 0x22, 0xA8, 0xAC, 0xAB, 0x4B, 0x52, 0xBB};
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}


static uint8_t mydata[] = "Hello World";
static osjob_t sendjob;


// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 10;


// class cHalConfiguration_t: public Arduino_LMIC::HalConfiguration_t
// {
//   public:
//     virtual u1_t queryBusyPin(void) override {return 13;};
//     virtual bool queryUsingDcdc(void) override {return true;};
//     virtual bool queryUsingDIO2AsRfSwitch(void) override {return true;};
//     virtual bool queryUsingDIO3AsTCXOSwitch(void) override {return true;};
// };

// cHalConfiguration_t myConfig;


// Pin mapping
const lmic_pinmap lmic_pins = {
   .nss = 10, // nano 10 | heltec 8
   .rxtx = LMIC_UNUSED_PIN,
   .rst = 9, // nano 9 | heltec 12
   .dio = {2, 6, 7}, // nano 2 6 7 | heltec 14 un un
  //  .pConfig = &myConfig,
};


void printHex2(unsigned v) {
   v &= 0xff;
   if (v < 16)
       Serial.print('0');
   Serial.print(v, HEX);
}


void onEvent (ev_t ev) {
   Serial.print(os_getTime());
   Serial.print(": ");
   switch(ev) {
       case EV_SCAN_TIMEOUT:
           Serial.println(F("EV_SCAN_TIMEOUT"));
           break;
       case EV_BEACON_FOUND:
           Serial.println(F("EV_BEACON_FOUND"));
           break;
       case EV_BEACON_MISSED:
           Serial.println(F("EV_BEACON_MISSED"));
           break;
       case EV_BEACON_TRACKED:
           Serial.println(F("EV_BEACON_TRACKED"));
           break;
       case EV_JOINING:
           Serial.println(F("EV_JOINING"));
           break;
       case EV_JOINED:
           Serial.println(F("EV_JOINED"));
           {
             u4_t netid = 0;
             devaddr_t devaddr = 0;
             u1_t nwkKey[16];
             u1_t artKey[16];
             LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
             Serial.print("netid: ");
             Serial.println(netid, DEC);
             Serial.print("devaddr: ");
             Serial.println(devaddr, HEX);
             Serial.print("AppSKey: ");
             for (size_t i=0; i<sizeof(artKey); ++i) {
               if (i != 0)
                 Serial.print("-");
               printHex2(artKey[i]);
             }
             Serial.println("");
             Serial.print("NwkSKey: ");
             for (size_t i=0; i<sizeof(nwkKey); ++i) {
                     if (i != 0)
                             Serial.print("-");
                     printHex2(nwkKey[i]);
             }
             Serial.println();
           }
           // Disable link check validation (automatically enabled
           // during join, but because slow data rates change max TX
     // size, we don't use it in this example.
           LMIC_setLinkCheckMode(0);
           hasJoined = true;
           break;
       /*
       || This event is defined but not used in the code. No
       || point in wasting codespace on it.
       ||
       || case EV_RFU1:
       ||     Serial.println(F("EV_RFU1"));
       ||     break;
       */
       case EV_JOIN_FAILED:
           Serial.println(F("EV_JOIN_FAILED"));
           break;
       case EV_REJOIN_FAILED:
           Serial.println(F("EV_REJOIN_FAILED"));
           break;
       case EV_TXCOMPLETE:
           Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
           if (LMIC.txrxFlags & TXRX_ACK)
             Serial.println(F("Received ack"));
           if (LMIC.dataLen) {
             Serial.print(F("Received "));
             Serial.print(LMIC.dataLen);
             Serial.println(F(" bytes of payload"));
             Serial.print("RETURN:");
            for (int i = 0; i < LMIC.dataLen; i++)
              {
                  Serial.print((LMIC.frame[LMIC.dataBeg + i]));
                  Serial.print(" ");
              }
            // uint16_t dst = LMIC.frame[LMIC.dataBeg + LMIC.dataLen - 1] + 256 * LMIC.frame[LMIC.dataBeg + LMIC.dataLen - 2];
            Serial.println();
           }
            if (!hasFoundDR)
            {
                uint8_t data[] = "hello";
                do_send(data, sizeof(data));
            }
           // Schedule next transmission
           break;
       case EV_LOST_TSYNC:
           Serial.println(F("EV_LOST_TSYNC"));
           break;
       case EV_RESET:
           Serial.println(F("EV_RESET"));
           break;
       case EV_RXCOMPLETE:
           // data received in ping slot
           Serial.println(F("EV_RXCOMPLETE"));
           break;
       case EV_LINK_DEAD:
           Serial.println(F("EV_LINK_DEAD"));
           break;
       case EV_LINK_ALIVE:
           Serial.println(F("EV_LINK_ALIVE"));
           break;
       /*
       || This event is defined but not used in the code. No
       || point in wasting codespace on it.
       ||
       || case EV_SCAN_FOUND:
       ||    Serial.println(F("EV_SCAN_FOUND"));
       ||    break;
       */
       case EV_TXSTART:
           Serial.println(F("EV_TXSTART"));
           break;
       case EV_TXCANCELED:
           Serial.println(F("EV_TXCANCELED"));
           break;
       case EV_RXSTART:
           /* do not print anything -- it wrecks timing */
           break;
       case EV_JOIN_TXCOMPLETE:
           Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
           break;


       default:
           Serial.print(F("Unknown event: "));
           Serial.println((unsigned) ev);
           break;
   }
}

void do_send(uint8_t myData[], size_t size) {
    // Send the payload as a byte array
   
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        Serial.println(size);
        int status = LMIC_setTxData2(5, myData, size, 0);

        Serial.println(status);
        if (status == -1)
        {
            Serial.println("Adjusting TX Data Rate... Data Not Sent");
            hasFoundDR = false;
        }
        else if (status == 0)
        {
            Serial.println("Adjusted DR");
            hasFoundDR = true;
        }
        // LMIC_setTxData(5, mydata, size - 1, 0);
        // LMIC_setTxData2(1, jsonString, sizeof(mydata)-1, 0);
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
 }



void setup() {

  //  heltec_setup();
  //  heltec_ve(true);

  //  display.init();

  // display.setFont(ArialMT_Plain_10);
   Serial.begin(115200);
  //  SPI.begin(9, 11, 10, 8);

   Serial.println(F("Starting"));
   


   #ifdef VCC_ENABLE
   // For Pinoccio Scout boards
   pinMode(VCC_ENABLE, OUTPUT);
   digitalWrite(VCC_ENABLE, HIGH);
   delay(1000);
   #endif


   // LMIC init
   os_init();
   // Reset the MAC state. Session and pending data transfers will be discarded.
   LMIC_reset();
 
  uint8_t initData[] = "Hello";
    do_send(initData, sizeof(initData)); //Send the packet to LMIC
}

void printByteArrayHex(uint8_t* x, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (x[i] < 16) Serial.print("0");  // For leading zero
    Serial.print(x[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}


void loop() {
      if (Serial.available()) {
        String msg = Serial.readStringUntil('\n');
        uint8_t buffer[51];
        size_t len = msg.length();
        msg.getBytes(buffer, len+1);
        for (size_t i = 0; i < len; i++) {
          Serial.print(buffer[i]);
          Serial.print(" ");
        }
        // display.drawString(0,0,msg);
        do_send(buffer, len);
    }
   os_runloop_once();
}

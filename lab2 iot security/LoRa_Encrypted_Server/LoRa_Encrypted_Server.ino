/*
  Both the TX and RX ProRF boards will need a wire antenna. We recommend a 3" piece of wire.
  This example is a modified version of the example provided by the Radio Head
  Library which can be found here: 
  www.github.com/PaulStoffregen/RadioHeadd
*/

#include <SPI.h>

//Radio Head Library: 
#include <RH_RF95.h>

#include <Crypto.h>

// We need to provide the RFM95 module's chip select and interrupt pins to the 
// rf95 instance below.On the SparkFun ProRF those pins are 12 and 6 respectively.
RH_RF95 rf95(12, 6);

int LED = 13; //Status LED on pin 13

int packetCounter = 0; //Counts the number of packets sent
long timeSinceLastPacket = 0; //Tracks the time stamp of last packet received
// The broadcast frequency is set to 921.2, but the SADM21 ProRf operates
// anywhere in the range of 902-928MHz in the Americas.
// Europe operates in the frequencies 863-870, center frequency at 
// 868MHz.This works but it is unknown how well the radio configures to this frequency:
//float frequency = 864.1;
float frequency = 921.2;


enum pairing_phase {
  INIT_ENCRYPT_PAIRING_RECV,
  INIT_ENCRYPT_PAIRING_ACK,
  LONG_ENCRYPT_PAIRING_RECV,
  LONG_ENCRYPT_PAIRING_ACK,
  SIGNATURE_PAIRING_RECV,
  SIGNATURE_PAIRING_ACK,
  PAIRED,
  ERROR
};

enum pairing_phase p_phase;

/****************Start of Encryption Keys*****************/
// Place to Store Encryption Key(s)
unsigned char encrypt_key[16];

// Place to store Signature Key(s)
unsigned char pub_sign_key[32];
unsigned char priv_sign_key[32];

unsigned char their_sign_key[32];

// Add more keys as you see fit

/*****************End of Encryption Keys******************/

uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t len = sizeof(buf);


void setup()
{
  pinMode(LED, OUTPUT);

  SerialUSB.begin(9600);
  // It may be difficult to read serial messages on startup. The following
  // line will wait for serial to be ready before continuing. Comment out if not needed.
  while(!SerialUSB);
  SerialUSB.println("RFM Server!");

  //Initialize the Radio. 
  if (rf95.init() == false){
    SerialUSB.println("Radio Init Failed - Freezing");
    while (1);
  }
  else{
  // An LED indicator to let us know radio initialization has completed.
    SerialUSB.println("Receiver up!");
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
    delay(500);
  }

  rf95.setFrequency(frequency); 

  p_phase = ERROR; // TODO: Set the initial phase for your system
}

void loop()
{
  bool received = false;
  if (rf95.available()){
    // Should be a message for us now

    if (rf95.recv(buf, &len)){
      digitalWrite(LED, HIGH); //Turn on status LED
      timeSinceLastPacket = millis(); //Timestamp this packet

      digitalWrite(LED, LOW); //Turn off status LED
      received = true;

    }
    else
      SerialUSB.println("Receive failed");

  }
  //Turn off status LED if we haven't received a packet after 1s
  if(millis() - timeSinceLastPacket > 1000){
    digitalWrite(LED, LOW); //Turn off status LED
    timeSinceLastPacket = millis(); //Don't write LED but every 1s
  }

  // TODO: Fill out state machine logic
  switch (p_phase) {
    case INIT_ENCRYPT_PAIRING_RECV:{
      if(received){
        // Check if packet should move data to next stage

        p_phase = ERROR; // TODO: Set the next phase for your system
      }

      break;
    }

    case INIT_ENCRYPT_PAIRING_ACK:{
      if(received){
        // Check if packet should move data to next stage

        p_phase = ERROR; // TODO: Set the next phase for your system
      }
      uint8_t send_len = 0;

      //TODO: Set buf with ACK message

      rf95.send(buf, send_len);
      rf95.waitPacketSent();
      break;
    }

    case LONG_ENCRYPT_PAIRING_RECV:{
      p_phase = ERROR; // TODO: Set the next phase for your system
      break;
    }

    case LONG_ENCRYPT_PAIRING_ACK:{
      p_phase = ERROR; // TODO: Set the next phase for your system
      break;
    }

    case SIGNATURE_PAIRING_RECV:{
      p_phase = ERROR; // TODO: Set the next phase for your system
      break;
    }
    case SIGNATURE_PAIRING_ACK:{
      p_phase = ERROR; // TODO: Set the next phase for your system      
      break;
    }
    case PAIRED:{
      p_phase = ERROR; // TODO: Set the next phase for your system
      break;
    }
    default:
      SerialUSB.print("Error");
  }
}
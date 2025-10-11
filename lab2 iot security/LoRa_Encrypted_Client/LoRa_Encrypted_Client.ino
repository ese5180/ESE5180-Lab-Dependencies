// rf95_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messageing client
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example rf95_server
// Tested with Anarduino MiniWirelessLoRa, Rocket Scream Mini Ultra Pro with
// the RFM95W, Adafruit Feather M0 with RFM95

#include <SPI.h>
#include <RH_RF95.h>

#include <Crypto.h>


/*****************Definitions**********************/
#define LISTENING_INTERVAL 3000
#define PACKET_INTERVAL 3000


/**************End of Definitions******************/

// Singleton instance of the radio driver
RH_RF95 rf95;
//RH_RF95 rf95(5, 2); // Rocket Scream Mini Ultra Pro with the RFM95W
//RH_RF95 rf95(8, 3); // Adafruit Feather M0 with RFM95 

// Need this on Arduino Zero with SerialUSB port (eg RocketScream Mini Ultra Pro)
//#define Serial SerialUSB

// The broadcast frequency is set to 921.2, but the SADM21 ProRf operates
// anywhere in the range of 902-928MHz in the Americas.
// Europe operates in the frequencies 863-870, center frequency at 
// 868MHz.This works but it is unknown how well the radio configures to this frequency:
//float frequency = 864.1;
float frequency = 921.2;


enum pairing_phase {
  INIT_ENCRYPT_PAIRING,
  LONG_ENCRYPT_PAIRING,
  SIGNATURE_PAIRING,
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

bool received = false;

char message[] = "The founding story of McGrill\'s Web House is one of innovation, passion for technology, \
                  and a vision to connect everyday devices in ways that would transform how people live and \
                  work.\n\nIt all started in 2018 when Mick McGrill, a talented computer engineer, found himself \
                  fascinated by the growing world of the Internet of Things (IoT). At the time, McGrill was \
                  working for a major tech firm, developing software for industrial automation. However, his \
                  interest extended beyond industrial solutions - he saw the potential for IoT to touch every \
                  corner of daily life, from smart homes to health monitoring to energy efficiency.";

void setup() 
{
  Serial.begin(9600);
  while (!Serial) ; // Wait for serial port to be available
  if (!rf95.init())
    SerialUSB.println("Radio Init Failed - Freezing");
    while (1);

  rf95.setFrequency(frequency); 

  p_phase = ERROR; // TODO: Set the initial phase for your system
}

void loop()
{
  Serial.println("Sending to rf95_server");
  // Send a message to rf95_server
  
  // TODO: Fill out state machine logic
  switch (p_phase) {
    case INIT_ENCRYPT_PAIRING:{
      if(received){
        // Check if packet should move data to next stage

        p_phase = ERROR; // TODO: Set the next phase for your system
      }
      //TODO: Set buf with correct message
      int send_len = 0;

      rf95.send(buf, send_len);
      break;
    }

    case LONG_ENCRYPT_PAIRING:{
      p_phase = ERROR; // TODO: Set the next phase for your system
      break;
    }

    case SIGNATURE_PAIRING:{
      p_phase = ERROR; // TODO: Set the next phase for your system
      break;
    }

    case PAIRED:{
      p_phase = ERROR; // TODO: Set the next phase for your system
      break;
    }
    default:
      SerialUSB.print("Error");
      while(1);
  }

  rf95.waitPacketSent();
  // Now wait for a reply (AKA the ACK for your message)
  received = false;
  if (rf95.waitAvailableTimeout(LISTENING_INTERVAL))
  { 
    // Should be a reply message for us now   
    if (rf95.recv(buf, &len))
    {
      Serial.print("got reply: ");
      Serial.println((char*)buf);

      received = true;
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
  else
  {
    Serial.println("No reply from rf95_server");
  }

  delay(PACKET_INTERVAL);
}



#include "jarvis.h"

SoftwareSerial deskSerial(DTX);

void Jarvis::begin(Telnet *telnetPtr) {
  deskSerial.begin(9600);
  pinMode(DTX, INPUT);
  resetPacket(0);
  telnet = telnetPtr;
  registerTelnetCallbacks();
};

void Jarvis::resetPacket(unsigned char data) {
  cmd = 0;
  argc = 0;
  // fill the args with 0
  memset(argv, 0U, MAX_ARGS);
  // if we're resetting it means we encountered an unexpected byte
  // however if the byte we received is the sync byte then
  // we want to count it as the start of the next sequence
  state = static_cast<state_t>(SYNC + (data == CTRLR_ADDR));
};

void Jarvis::loop() {
  while (deskSerial.available()) {
    unsigned char data = deskSerial.read();

    bool commandFinished = registerByte(data);
    if (commandFinished) {
      // printPacket();
      decodePacket();
    }
  }
}

void Jarvis::moveDown() {
  telnet->stream->println("Moving down!");
}

void Jarvis::moveUp() {
  telnet->stream->println("Moving up!");
}

bool Jarvis::registerByte(unsigned char data) {
  bool checksumMatched = false;
  bool didError = false;

  switch(state) {
    case SYNC: // 0
    case SYNC2: // 1
      if (data != CTRLR_ADDR) {
        // we were waiting for a sync byte but got something else
        didError = false;
      }
      break;
    case CMD: // 2
      // first byte after the second sync byte is the command
      cmd = data;
      // CMD is the first addend of the checksum
      checksum = data;
      break;
    case LENGTH: // 3
      // length should always be 0, 1, 2, 3, (then why do we have an array of size 5??)
      if (data > MAX_ARGS) {
        didError = true;
        break;
      }
      argc = data;
      checksum += data;
      // since the number of parameters is variable we need to jump the state based
      // on the number of expected parameters
      // we want the state to be decremented from the checksum as it's the next expected state after the parameters
      // we need to create enough room for the expected args, but have an extra decrement since we increment the state after the switch
      state = static_cast<state_t>(CHKSUM - data - 1);
      break;
    default: // ARGS 4-8
      if (state <= LENGTH || state > ARGS) {
        // we shouldn't hit this, but in case we somehow get a fall-through case
        didError = true;
        break;
      }

      argv[argc - (CHKSUM - state)] = data;
      checksum += data;
      break;
    case CHKSUM: // 9
      // by this point we should have added the command, length, and each param to the checksum,
      // so it should match the data we receive for the checksum here
      // if not, something was missed, so throw the message away
      if (data != checksum) {
        didError = true;
      }

      checksumMatched = true;
      break;
    case ENDMSG:
      didError = true;
      break;
   }

  if (state < SYNC || state > ENDMSG || didError) {
    resetPacket(data);
    return false;
  }

  // increment state to look for next arg
  state = static_cast<state_t>(state + 1);

  return checksumMatched;
}

void Jarvis::printPacket() {
  telnet->stream->print("Got Command: ");
  telnet->stream->print(CTRLR_ADDR, HEX);
  telnet->stream->print(" ");
  telnet->stream->print(CTRLR_ADDR, HEX);
  telnet->stream->print(" ");
  telnet->stream->print(cmd, HEX);
  telnet->stream->print(" ");
  telnet->stream->print(argc, HEX);
  telnet->stream->print(" ");

  if (argc > 0) {
    telnet->stream->print("{");
  }
  for (uint8_t i = 0; i < argc; i++) {
    telnet->stream->print(argv[i], HEX);
    if (i < argc - 1) {
      telnet->stream->print(", ");
    }
  }
  if (argc > 0) {
    telnet->stream->print("} ");
  }

  telnet->stream->print(checksum, HEX);
  telnet->stream->print(" ");
  telnet->stream->println(0x7e, HEX);
  telnet->stream->println();
}

void Jarvis::decodePacket() {
  static uint16_t lastHeight = 0;

  switch(cmd) {
    case HEIGHT: {
      uint16_t height = argv[0];
      height <<= 8;
      height |= argv[1];

      if (height != lastHeight) {
        lastHeight = height;

        telnet->stream->print("Desk height is ");
        float adjusted = (float)height / 10;
        telnet->stream->print(adjusted, 1);
        Serial.println(adjusted, 1);
        telnet->stream->println(" inches");
      }
    }
  }
}

void Jarvis::registerTelnetCallbacks() {
  telnet->registerCallback("help", "I am a good desk! I am happy to meet you!");
  telnet->registerCallback("down", std::bind(&Jarvis::moveDown, this));
  telnet->registerCallback("up", std::bind(&Jarvis::moveUp, this));
}

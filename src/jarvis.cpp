#include "jarvis.h"

SoftwareSerial deskSerial(DTX);

void Jarvis::begin(Telnet *telnetPtr) {
  deskSerial.begin(9600);
  pinMode(DTX, INPUT);
  resetPin(HS0);
  resetPin(HS1);
  resetPin(HS2);
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
  processHandsetData();

  if (button_timer.overdue()) {
    release();
  }
}

void Jarvis::processHandsetData() {
  while (deskSerial.available()) {
    unsigned char data = deskSerial.read();

    bool commandReceived = registerByte(data);
    if (commandReceived) {
      // printPacket();
      decodePacket();
    }
  }
}

void Jarvis::moveDown(uint8_t num_seconds) {
  if (!button_timer.is_active()) {
    telnet->stream->print("Moving down for ");
    telnet->stream->print(num_seconds);
    telnet->stream->println(" seconds!");
    setPin(HS0);
    button_timer.begin(num_seconds * 1000);
  }
}

void Jarvis::moveUp(uint8_t num_seconds) {
  if (!button_timer.is_active()) {
    telnet->stream->print("Moving up for ");
    telnet->stream->print(num_seconds);
    telnet->stream->println(" seconds!");
    setPin(HS1);
    button_timer.begin(num_seconds * 1000);
  }
}

void Jarvis::moveToPreset(uint8_t memory_number) {
  if (button_timer.is_active()) {
    return;
  }

  switch (memory_number)
  {
  case 1:
    setPin(HS0);
    setPin(HS1);
    break;
  case 2:
    setPin(HS2);
    break;
  case 3:
    setPin(HS0);
    setPin(HS2);
    break;
  case 4:
    setPin(HS1);
    setPin(HS2);
    break;
  default:
    return;
  }

  button_timer.begin(BUTTON_PRESS_MS);
  telnet->stream->print("Moving to memory position ");
  telnet->stream->print(memory_number);
  telnet->stream->println("!");
}

void Jarvis::release() {
  telnet->stream->println("Buttons released!");
  resetPin(HS0);
  resetPin(HS1);
  resetPin(HS2);
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

void Jarvis::setPin(uint8_t pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}

void Jarvis::resetPin(uint8_t pin) {
  pinMode(pin, INPUT_PULLUP);
}

void Jarvis::help() {
  telnet->stream->println("up {1,2,3,4}    -  Move the desk up for the given number of seconds");
  telnet->stream->println("down {1,2,3,4}  -  Move the desk down for the given number of seconds");
  telnet->stream->println("goto {1,2,3,4}  - Go to the memory preset specified");
}

void Jarvis::registerTelnetCallbacks() {
  telnet->registerCallback("help", std::bind(&Jarvis::help, this));
  telnet->registerCallback("down 1", std::bind(&Jarvis::moveDown, this, 1));
  telnet->registerCallback("down 2", std::bind(&Jarvis::moveDown, this, 2));
  telnet->registerCallback("down 3", std::bind(&Jarvis::moveDown, this, 3));
  telnet->registerCallback("down 4", std::bind(&Jarvis::moveDown, this, 4));
  telnet->registerCallback("up 1", std::bind(&Jarvis::moveUp, this, 1));
  telnet->registerCallback("up 2", std::bind(&Jarvis::moveUp, this, 2));
  telnet->registerCallback("up 3", std::bind(&Jarvis::moveUp, this, 3));
  telnet->registerCallback("up 4", std::bind(&Jarvis::moveUp, this, 4));
  telnet->registerCallback("goto 1", std::bind(&Jarvis::moveToPreset, this, 1));
  telnet->registerCallback("goto 2", std::bind(&Jarvis::moveToPreset, this, 2));
  telnet->registerCallback("goto 3", std::bind(&Jarvis::moveToPreset, this, 3));
  telnet->registerCallback("goto 4", std::bind(&Jarvis::moveToPreset, this, 4));
}

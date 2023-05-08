#include <TelnetStream.h>
#include <sntp.h>

#ifndef TELNET_H
#define TELNET_H

typedef std::function<void()> callback;

struct telnet_hook {
  char const *trigger;
  char const *message;
  callback cb;
};

typedef std::vector<telnet_hook> telnet_hooks_vector;

class Telnet {
  public:
    void begin();
    void loop();
    void registerCallback(char const *trigger, callback cb);
    void registerCallback(char const *trigger, char const *message);
    TelnetStreamClass *stream;
  private:
    telnet_hooks_vector hooks;
};

#endif

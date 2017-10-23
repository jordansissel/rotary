typedef void (*quad_callback)(int value, void *arg);

class Quad {
  //enum Direction { up, down, none };

  char prior_value = 0;
  //Direction prior_direction = none;
  //double quiesce_until = mgos_uptime();
  //double prior_time = mgos_uptime();
  quad_callback callback;
  void *callback_arg;

  int pin1;
  int pin2;

public:
  Quad(int pin1, int pin2);
  void interrupt(int pin);
  void setCallback(quad_callback, void *);
};

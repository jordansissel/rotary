#include "Quad.h"
#include "mgos.h"

char greycode(char input) {
  switch (input) {
    case 0: return 0; /* 00 => 0 */
    case 1: return 1; /* 01 => 1 */
    case 3: return 2; /* 11 => 2 */
    case 2: return 3; /* 10 => 3 */
    default:
      LOG(LL_INFO, ("Invalid greycode: %d%d", input >> 1, input & 1));
      return 0;
  }
}

// 20ms quiscence to ignore funky rotation
#define QUIESCE_FUNKY_ROTATION 0.120

void Quad::setCallback(quad_callback callback, void *arg) {
  this->callback = callback;
  this->callback_arg = arg;
}

void Quad::interrupt(int pin) {
  //double now = mgos_uptime();
  char bits = mgos_gpio_read(pin1) << 1 | mgos_gpio_read(pin2);
  char value = greycode(bits);

  //Direction direction;
  //LOG(LL_INFO, ("Value %d%d (old %d%d)", value >> 1, value & 1, prior_value >> 1, prior_value & 1));
  if (value == prior_value) {
    return;
  }

  short move = 0;
  if (value == 0 && prior_value == 3) {
    // rollover clockwise
    move = 1;
    //direction = up;
  } else if (value == 3 && prior_value == 0) {
    // rollover counter-clockwise
    move = -1;
    //direction = down;
  } else {
    // otherwise, movement is the difference between current + prior
    move = value - prior_value;
    //direction = (move >= 0) ? up : down;
  }

  //LOG(LL_INFO, ("Movement: %d", move));

  if (callback) {
    callback(move, callback_arg);
  }
  //prior_direction = direction;
  prior_value = value;
  //prior_time = now;
  (void) pin;
}

void quad_interrupt(int pin, void *arg) {
  Quad *q = static_cast<Quad *>(arg);
  q->interrupt(pin);
}

Quad::Quad(int p1, int p2) {
  pin1 = p1;
  pin2 = p2;

  mgos_gpio_set_mode(pin1, MGOS_GPIO_MODE_INPUT);
  mgos_gpio_set_pull(pin1, MGOS_GPIO_PULL_DOWN);
  mgos_gpio_set_int_handler(pin1, MGOS_GPIO_INT_EDGE_POS, quad_interrupt, this);
  mgos_gpio_set_int_handler(pin1, MGOS_GPIO_INT_EDGE_NEG, quad_interrupt, this);
  mgos_gpio_enable_int(pin1);

  mgos_gpio_set_mode(pin2, MGOS_GPIO_MODE_INPUT);
  mgos_gpio_set_pull(pin2, MGOS_GPIO_PULL_DOWN);
  mgos_gpio_set_int_handler(pin2, MGOS_GPIO_INT_EDGE_POS, ::quad_interrupt, this);
  mgos_gpio_set_int_handler(pin2, MGOS_GPIO_INT_EDGE_NEG, ::quad_interrupt, this);
  mgos_gpio_enable_int(pin2);
  LOG(LL_INFO, ("Quadrature input configured using pins %d and %d", pin1, pin2));
}
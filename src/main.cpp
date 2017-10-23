#include "mgos.h"
#include "mgos_gpio.h"
#include "mgos_wifi.h"

#include "Denon.h"
#include "Quad.h"
#include "device.h"


static int counter = 0;

void incr(int move, void *arg) {
  Denon *denon = static_cast<Denon *>(arg);
  counter += move;
  //LOG(LL_INFO, ("Value: %d", counter));

  if (move < 0) {
    denon->send("MVDOWN", 6);
  } else {
    denon->send("MVUP", 4);
  }
}

void discover(enum mgos_wifi_status event, void *arg) {
  Denon *denon = static_cast<Denon*>(arg);
  switch (event) {
    case MGOS_WIFI_IP_ACQUIRED:
      denon->discover();
      break;
    case MGOS_WIFI_DISCONNECTED:
    case MGOS_WIFI_CONNECTED:
    case MGOS_WIFI_CONNECTING:
      break;
  }

  (void) arg;
}

static bool power_toggle = true;
void button(int pin, void *arg) {
  Denon *denon = static_cast<Denon *>(arg);

  auto value = mgos_gpio_read(pin);

  if (value != 0) {
    return;
  }

  power_toggle = !power_toggle;
  if (power_toggle) {
    LOG(LL_INFO, ("Power ON + Select Input: Game"));
    denon->send("PWON", 4);
    denon->send("SIGAME", 6);
  } else {
    LOG(LL_INFO, ("Power STANDBY"));
    denon->send("PWSTANDBY", 9);
  }
}

enum mgos_app_init_result mgos_app_init(void) {
  Denon *denon = new Denon();
  mgos_wifi_add_on_change_cb(discover, denon);

  Quad *q = new Quad(D5, D6);
  q->setCallback(incr, denon);

  mgos_gpio_set_mode(D7, MGOS_GPIO_MODE_INPUT);
  mgos_gpio_set_pull(D7, MGOS_GPIO_PULL_DOWN);
  mgos_gpio_set_int_handler(D7, MGOS_GPIO_INT_EDGE_POS, button, denon);
  mgos_gpio_set_int_handler(D7, MGOS_GPIO_INT_EDGE_NEG, button, denon);
  mgos_gpio_enable_int(D7);


  return MGOS_APP_INIT_SUCCESS;
}
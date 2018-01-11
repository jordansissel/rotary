#include "mgos.h"
#include "mgos_gpio.h"
#include "mgos_wifi.h"
#include "mgos_mqtt.h"

#include "Denon.h"
#include "Quad.h"
#include "device.h"

static const struct mg_str DENON_PWSTANDBY = MG_MK_STR("PWSTANDBY");
static const struct mg_str DENON_PWON = MG_MK_STR("PWON");
static const struct mg_str DENON_MV50 = MG_MK_STR("MV50");
static const struct mg_str DENON_MVUP = MG_MK_STR("MVUP");
static const struct mg_str DENON_MVDOWN = MG_MK_STR("MVDOWN");

static int counter = 0;

void incr(int move, void *arg) {
  Denon *denon = static_cast<Denon *>(arg);
  counter += move;
  //LOG(LL_INFO, ("Value: %d", counter));

  if (move < 0) {
    denon->send(DENON_MVDOWN);
  } else {
    denon->send(DENON_MVUP);
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
    LOG(LL_INFO, ("Power STANDBY"));
    denon->send(DENON_PWSTANDBY);
  } else {
    LOG(LL_INFO, ("Power ON"));
    // Sometimes when the AVR goes to sleep, it mutes and sets the volume to 0.
    // work around this by setting the volume back to 50% when we power on.
    denon->send(DENON_MV50);

    denon->send(DENON_PWON);
  }
}

void click(int pin, void *arg) {
  (void) arg;
  auto value = mgos_gpio_read(pin);

  LOG(LL_INFO, ("Click: %d", value));
  
  auto ipmi = "power -N pork-ipmi -u -U root -P fancypants";
  mgos_mqtt_pub("/workstation/ipmi", ipmi, strlen(ipmi), 0, false);
  LOG(LL_INFO, ("Sending IPMI power up"));
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

  mgos_gpio_set_mode(D3, MGOS_GPIO_MODE_INPUT);
  mgos_gpio_set_pull(D3, MGOS_GPIO_PULL_DOWN);
  mgos_gpio_set_int_handler(D3, MGOS_GPIO_INT_EDGE_POS, click, NULL);
  mgos_gpio_set_int_handler(D3, MGOS_GPIO_INT_EDGE_NEG, click, NULL);
  mgos_gpio_enable_int(D3);

  return MGOS_APP_INIT_SUCCESS;
}
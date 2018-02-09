#include "mgos.h"
#include "mgos_mqtt.h"

#include "Quad.h"
#include "device.h"

static const struct mg_str DENON_PWSTANDBY = MG_MK_STR("PWSTANDBY");
static const struct mg_str DENON_PWON = MG_MK_STR("PWON");
static const struct mg_str DENON_MV50 = MG_MK_STR("MV50");
static const struct mg_str DENON_MVUP = MG_MK_STR("MVUP");
static const struct mg_str DENON_MVDOWN = MG_MK_STR("MVDOWN");

static int counter = 0;

static bool boot_signal_sent = 0;

void read_rotary_encoder(int move, void *arg) {
  counter += move;
  // XXX: Send mqtt signal for rotary motion
}

static void network_status_cb(int ev, void *evd, void *arg) {
  switch (ev) {
    case MGOS_NET_EV_DISCONNECTED:
      LOG(LL_INFO, ("%s", "Net disconnected"));
      break;
    case MGOS_NET_EV_CONNECTING:
      LOG(LL_INFO, ("%s", "Net connecting..."));
      break;
    case MGOS_NET_EV_CONNECTED:
      LOG(LL_INFO, ("%s", "Net connected"));
      break;
    case MGOS_NET_EV_IP_ACQUIRED:
      LOG(LL_INFO, ("%s", "Net got IP address"));
      // YAY WE ARE ONLINE.
      if (!boot_signal_sent) {
        boot_signal_sent = true;
        // XXX: Send mqtt signal for "power on"
      }
      break;
  }

  (void) evd;
  (void) arg;
}

void button(int pin, void *arg) {
  // XXX: send mqtt message "dial button pressed"
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
  mgos_event_add_group_handler(MGOS_EVENT_GRP_NET, network_status_cb, NULL);

  // Rotary Encoder.
  Quad *q = new Quad(D5 /* Clock */, D6 /* Data */);
  q->setCallback(read_rotary_encoder, NULL);

  // Rotary encoder has a built-in push-button switch.
  mgos_gpio_set_mode(D7, MGOS_GPIO_MODE_INPUT);
  mgos_gpio_set_pull(D7, MGOS_GPIO_PULL_DOWN);
  mgos_gpio_set_int_handler(D7, MGOS_GPIO_INT_EDGE_POS, button, NULL);
  mgos_gpio_set_int_handler(D7, MGOS_GPIO_INT_EDGE_NEG, button, NULL);
  mgos_gpio_enable_int(D7);

  // Momentary button used for 
  mgos_gpio_set_mode(D3, MGOS_GPIO_MODE_INPUT);
  mgos_gpio_set_pull(D3, MGOS_GPIO_PULL_DOWN);
  mgos_gpio_set_int_handler(D3, MGOS_GPIO_INT_EDGE_POS, click, NULL);
  mgos_gpio_set_int_handler(D3, MGOS_GPIO_INT_EDGE_NEG, click, NULL);
  mgos_gpio_enable_int(D3);

  return MGOS_APP_INIT_SUCCESS;
}
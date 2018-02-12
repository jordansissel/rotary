#include "mgos.h"
#include "mgos_mqtt.h"

#include <Adafruit_SSD1306.h>

#include "Quad.h"
#include "device.h"

static Adafruit_SSD1306 *display = nullptr;
static int counter = 0;

#define FONT_HEIGHT1 8
#define FONT_HEIGHT2 16
#define FONT_HEIGHT3 24

static bool boot_signal_sent = 0;

void read_rotary_encoder(int move, void *arg) {
  counter += move;
  // XXX: Send mqtt signal for rotary motion
}

static void display_status(Adafruit_SSD1306 *d, const char *text) {
  d->setTextSize(1);

  // Blank the top line
  d->fillRect(0, 0, 128, 16, WHITE);
  d->setTextColor(BLACK, WHITE);
  d->setCursor(2, 3);
  d->printf("%s", text);
  d->display();
}

static void display_text(Adafruit_SSD1306 *d, const char *s) {
  // Clear the main display
  d->fillRect(0, FONT_HEIGHT2, 128, 64, BLACK);

  d->setTextSize(2);
  d->setTextColor(WHITE);
  d->setCursor(0, FONT_HEIGHT2 /* first row */);
  d->printf("%s", s);
  d->display();
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
      display_status(display, "Connected");
      LOG(LL_INFO, ("%s", "Net connected"));
      break;
    case MGOS_NET_EV_IP_ACQUIRED:
      display_status(display, "Online.");
      break;
  }

  (void) evd;
  (void) arg;
}

static void mqtt_ev_handler(struct mg_connection *c, int ev, void *p, void *user_data) {
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *) p;
  if (ev == MG_EV_MQTT_CONNACK) {
    // When we successfully connect to MQTT for the first time, send an 'I am online' notification.
    if (!boot_signal_sent) {
      display_status(display, "Ready.");
      LOG(LL_INFO, ("%s", "Sending boot complete message"));
      boot_signal_sent = true;
      mgos_mqtt_pub("/office/panel/online", "ok", 2, 1, false);
    }
  }
  (void) user_data;
  (void) c;
}


void button(int pin, void *arg) {
  // XXX: send mqtt message "dial button pressed"
}

void click(int pin, void *arg) {
  (void) arg;
  auto value = mgos_gpio_read(pin);

  LOG(LL_INFO, ("Click: %d", value));
  
  auto ipmi = "power -N pork-ipmi -u -U root -P fancypants";
  mgos_mqtt_pub("/workstation/power", "on", 2, 0, false);
  LOG(LL_INFO, ("Sending IPMI power up"));
}


void mq_audio(struct mg_connection *nc, const char *topic, int topic_len, const char *msg, int msg_len, void *ud) {
  LOG(LL_INFO, ("Got message on topic %.*s: %.*s", topic_len, topic, msg_len, msg));
  (void) nc;
  char status[10];
  boolean power;
  int volume;
  if (json_scanf(msg, msg_len, "{power:%B,volume:%d}", &power, &volume) >= 0) {
    snprintf(status, 10, "%s (%d)", power ? "ON" : "OFF", volume);
    display_text(display, status);
  } else {
    LOG(LL_WARN, ("Got invalid JSON: %.*s", msg_len, msg));
  }
  (void) ud;
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

  display = new Adafruit_SSD1306(4 /* RST GPIO */, Adafruit_SSD1306::RES_128_64);

  if (display != nullptr) {
    display->begin(SSD1306_SWITCHCAPVCC, 0x3C /* Check the ID on the display or use i2c scanning */, true /* reset */);
    display->display();
  }

  display->fillScreen(BLACK);
  display_status(display, "Booting...");

  mgos_mqtt_add_global_handler(mqtt_ev_handler, NULL);

  mgos_mqtt_sub("/office/audio", mq_audio, NULL);

  return MGOS_APP_INIT_SUCCESS;
}
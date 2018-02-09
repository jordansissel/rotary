#include "mgos.h"
#include "mgos_mongoose.h"
#include "Denon.h"

extern "C" {
  #include "lwip/udp.h"
  #include "lwip/ip_addr.h"
  #include "lwip/pbuf.h"
}

static void denonProcessQueue(void *arg) {
  Denon *denon = static_cast<Denon*>(arg);
  denon->processQueue();
}

static void ssnpCallback(struct mg_connection *nc, int ev, void *ev_data, void *user_data) {
  Denon *denon = static_cast<Denon*>(user_data);
  struct http_message *hm;
  int i = 0;

  switch (ev) {
    case MG_EV_CONNECT:
      mg_send(nc, denon->denonQuery, denon->denonQueryLength);
      break;
    case MG_EV_SEND:
      LOG(LL_INFO, ("SENT %d bytes", *(int *)(ev_data)));
      break;
    case MG_EV_CLOSE:
      LOG(LL_INFO, ("CLOSE"));
      break;
    case MG_EV_HTTP_CHUNK:
      hm = (struct http_message *) ev_data;
      for (i = 0; hm->header_names[i].len > 0; i++) {
        LOG(LL_INFO, ("Header[%d] %.*s: %.*s", i, hm->header_names[i].len, hm->header_names[i].p, hm->header_values[i].len, hm->header_values[i].p));
        if (!strncmp("LOCATION", hm->header_names[i].p, hm->header_names[i].len)) {
          LOG(LL_INFO, ("Location: %.*s", hm->header_values[i].len, hm->header_values[i].p));
          denon->setAddressFromURI(hm->header_values[i]);
        }
      }
      mgos_disconnect(nc);
      break;
    case MG_EV_RECV:
      break;
    case MG_EV_POLL:
      // XXX: Timeout after some period.
      break;
    default:
      LOG(LL_INFO, ("Unknown event %d", ev));
      break;
  }
}

static void httpCallback(struct mg_connection *nc, int ev, void *ev_data, void *user_data) {
  switch (ev) {
  case MG_EV_HTTP_REPLY:
    nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    break;
  }
  (void)user_data;
  (void)ev_data;
}


void Denon::discover() {
  // SSNP is multicast HTTP over UDP.
  mgos_connect_http("udp://239.255.255.250:1900", ssnpCallback, this);
}

void Denon::setAddressFromURI(const struct mg_str uri) {
  int rc = mg_parse_uri(uri, NULL, NULL, &address, NULL, NULL, NULL, NULL);
  if (rc != 0) {
    LOG(LL_WARN, ("Failed to parse URI: %.*s", uri.len, uri.p));
    return;
  }
  address.p = strndup(address.p, address.len);

  LOG(LL_INFO, ("Found Denon AVR at %.*s", address.len, address.p));
}

void Denon::send(struct mg_str command) {
  LOG(LL_INFO, ("Queueing command: %.*s", command.len, command.p));
  queue.push_back(command);
  if (!queue_pump_timer) {
    // Limit message rate to the AVR at once per 40ms.
    // If we send too quickly, the AVR softlocks and, well, computers are great.
    queue_pump_timer = mgos_set_timer(40, true, denonProcessQueue, this);
  }
}

void Denon::processQueue() {
  if (queue.size() == 0) {
    mgos_clear_timer(queue_pump_timer);
    queue_pump_timer = 0;
    return;
  }

  struct mg_str command = queue.front();
  LOG(LL_INFO, ("Sending command[%.*s]: %.*s", address.len, address.p, command.len, command.p));

  // % curl -D - 'http://192.168.1.213:80/goform/formiPhoneAppDirect.xml?MVUP'
  char url[255];
  sprintf(url, "http://%.*s:80/goform/formiPhoneAppDirect.xml?%.*s", address.len, address.p, command.len, command.p);;
  mg_connect_http(mgos_get_mgr(), httpCallback, this, url, NULL, NULL);

  queue.pop_front();
}
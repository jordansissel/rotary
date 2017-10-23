class Denon {
public:
  static constexpr const char *denonQuery =  "M-SEARCH * HTTP/1.1\r\n"
    "Content-Length: 0\r\n"
    "ST: urn:schemas-upnp-org:device:MediaRenderer:1\r\n"
    "MX: 3\r\n"
    "MAN: \"ssdp:discover\"\r\n"
    "HOST: 239.255.255.250:1900\r\n"
    "\r\n";
  static constexpr const int denonQueryLength = strlen(denonQuery);

  void discover();
  void setAddressFromURI(const struct mg_str uri);

  void send(const char *command, const int length);

private:
  struct mg_connection *connection;
  struct mg_str address;
  short port = 23;

  void connect();
};
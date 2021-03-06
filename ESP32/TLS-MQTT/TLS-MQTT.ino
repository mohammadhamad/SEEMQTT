/*
 * MQTT over TLS
 */
 #include <pbc.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>


#define SERIAL_DEBUG
#define TLS_DEBUG
#define TIME_DEBUG
#define RECON
const char* ssid = "TP-Link_904A";
const char* password = "30576988";
const char* mqtt_server =  "192.168.0.110";
const int mqttPort = 8883; //1883


#define Size_Byte 16 // size of the message



const char* ca_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDPDCCAiQCCQDHy84YoL+AsjANBgkqhkiG9w0BAQsFADBgMQswCQYDVQQGEwJE\n" \
"RTEQMA4GA1UECAwHQmF2YXJpYTEPMA0GA1UEBwwGTXVuaWNoMRYwFAYDVQQLDA1J\n" \
"b1QgVFUgTXVuaWNoMRYwFAYDVQQDDA0xOTIuMTY4LjAuMTEwMB4XDTIxMDgxMTE1\n" \
"NTkzM1oXDTIyMDgxMTE1NTkzM1owYDELMAkGA1UEBhMCREUxEDAOBgNVBAgMB0Jh\n" \
"dmFyaWExDzANBgNVBAcMBk11bmljaDEWMBQGA1UECwwNSW9UIFRVIE11bmljaDEW\n" \
"MBQGA1UEAwwNMTkyLjE2OC4wLjExMDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC\n" \
"AQoCggEBAKQXEWqF/leB4lxH1JGlEhj+zIet3ZP0admYi681myu7ge/eGQr3c9nB\n" \
"LO5JHWK9LneeIEg4najIlaTGsEmo26kQ9iVtCN0+XRVNrCP+3ZcLOUXVuL095oVn\n" \
"dn7JQuRSK5zv6t4WQw1tXcI5ommtaY6VvYXxCW99HkwCBeT+lMFszy7G5lY2LOcU\n" \
"cF/ztDIK7Aty7JuZZn3s+OoNaH5t5K7D8fEoIKQ+gA9qhBmmlZKZglSWSwrTKOnj\n" \
"uSbK9bUSnGW3t2nvwkMBeB31/PWhvK5gNB6xK9xgmyo+J+loQKinTnD+UyeqZ6Ib\n" \
"yGAPJ+zA207lXNhePjzYrV/4x26GgqcCAwEAATANBgkqhkiG9w0BAQsFAAOCAQEA\n" \
"Brwm3jJ+hfTLlLVFZqxX8sMDsSMRjRLln+6PFoqlb3KzpHYiDH0ohyKLakpXocgH\n" \
"tYdV1w08wMJrr3q/TWdBkNzQR5gquZsXURqr9jEwbnZxyLlFX4INq3yh4dcO9w08\n" \
"7OUzyPKspdqFPjlF+dUkcC6wCk+CZKlaBVVLJ1MqBliZksV7s83SqXehhtd7rMoY\n" \
"T4eLGr3rn1pHygKnxdCtzQGBA1JbsiWUxEEp9vhd/9OKDbJ4QPHUruFe2v6fT4jy\n" \
"2Y3vidMEG6Cu0ILRpkdWyKsiWcSnKkeWbeQAL3mvWFzApC5QWVedc9+raNM19jDX\n" \
"OzV+z9Xq8Wi/Nt7CtvKOfg==\n" \
"-----END CERTIFICATE-----\n";




WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);
String clientId = "ESP32Client-"; /* MQTT client ID (will add random hex suffix during setup) */



unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;


char * mymsg;



void setup()
{


#ifdef SERIAL_DEBUG
  /* Initialize serial output for debug */
  Serial.setDebugOutput(true);
 Serial.begin(115200);
  Serial.println();

#endif
#ifdef SERIAL_DEBUG
  Serial.print("connecting to ");
  Serial.println(ssid);
#endif
  unsigned long b_conap= millis();
  /*  Connect to local WiFi access point */
 // WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

#ifdef SERIAL_DEBUG
  Serial.print("Connecting");

#endif
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
#ifdef SERIAL_DEBUG
    Serial.print(".");
#endif
  }
#ifdef SERIAL_DEBUG

 unsigned long e_conap= millis();
  #ifdef TIME_DEBUG
  Serial.print(" Time to connect to Ap is:" );
  Serial.println(e_conap - b_conap);
  #endif
  /* When WiFi connection is complete, debug log connection info */
  Serial.println("");
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
#endif

#ifdef SERIAL_DEBUG
  Serial.print("connecting to ");
  Serial.println(mqtt_server);
#endif

 espClient.setCACert(ca_cert);

#ifdef TLS_DEBUG
  /* Call verifytls to verify connection can be done securely and validated - this is optional but was useful during debug */
  verifytls();
#endif

  /* Configure MQTT Broker settings */
  mqttClient.setServer(mqtt_server, mqttPort);

  /* Add random hex client ID suffix once during each reboot */
  clientId += String(random(0xffff), HEX);
  mymsg = rand_string_alloc(Size_Byte);
}



void reconnect() {
  /* Loop until we're reconnected */
  while (!mqttClient.connected()) {

      Serial.print("Attempting MQTT broker connection...");

      unsigned long b_mssl= millis();
    /* Attempt to connect */
     if (mqttClient.connect(clientId.c_str())) {
      unsigned long e_mssl = millis();
      Serial.println("connected");
       #ifdef TIME_DEBUG
      Serial.print("time to estiblsh SSL:");
      Serial.println(e_mssl - b_mssl);
      #endif

    }
    else {

      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Trying again in 5 seconds...");

      /* Wait 5 seconds between retries */
      delay(5000);
    }
  }
}



static char *rand_string(char *str, size_t size)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXZY";
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
    return str;
}

char* rand_string_alloc(size_t size)
{
     char *s =(char *) malloc(size + 1);
     if (s) {
         rand_string(s, size);
     }
     return s;
}



/* verifytls()
 *  Test WiFiClientSecure connection using supplied cert and fingerprint
 */
bool verifytls() {
  bool success = false;

  Serial.print("Verifying TLS connection to ");
  Serial.println(mqtt_server);


  success = espClient.connect(mqtt_server, mqttPort);


  if (success) {
    Serial.println("Connection complete, valid cert, valid fingerprint.");
  }
  else {
    Serial.println("Connection failed!");
  }


  return (success);
}


void loop()
{
  /* Main loop. Attempt to re-connect to MQTT broker if connection drops, and service the mqttClient task. */
  if(!mqttClient.connected()) {
    reconnect();
  }


  unsigned long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    ++value;

    unsigned long b_msgsend = millis();
    /*

    mqttClient.beginPublish("mhh_my_topic", Size_Byte, true);
    mqttClient.write((byte *)mymsg, (int)Size_Byte);
    mqttClient.endPublish ();
    */
    mqttClient.publish("mhh_my_topic",mymsg);
   unsigned long e_msgsend = millis();

   snprintf (msg, MSG_BUFFER_SIZE, "Message ID #%ld ", value );
    Serial.print("Publish message: ");
    Serial.println(msg);
    #ifdef TIME_DEBUG
    char timemsg [50] ;
    snprintf (timemsg, MSG_BUFFER_SIZE, "time to publish %ld Byte in Ms:", Size_Byte );
    Serial.print(timemsg);
    Serial.println(e_msgsend - b_msgsend);
    #endif
   #ifdef RECON
   mqttClient.disconnect();
   #endif
  }

mqttClient.loop();
}

#include <WiFi.h>
#include <SecMqttKSn.h>

/* print the time measurment */ 
#define TIME_DEBUG
//#define DBG_MSG

#define BUILTIN_LED 2 

/* by enable that we enforrce the connection to be closed after each transmission */
#define RECON

#define Size_Byte 1024 // message payload size 1024,5120,10240,15360,20480
//#define C_SIZE 128 // rsa encryption message size

// Update these with values suitable for your network.
const char* ssid = "testhotspot";
const char* password = "1234567890";
const char* mqtt_server =  "10.42.0.1";
const int mqttPort = 1883; 
const char* topic = "mhh_my_topic";
String clientId = "ESP32Client-";
unsigned long lastMsg = 0;
unsigned char* mymsg;

int test_times = 1;

/*
 * Key Store Public key 
 */

// Note:
// 1. IBE: give identity
// 2. SSS:
// load file

/* 1024-bit RSA key pair */ 
const unsigned char ks1_pk_key[] = \
"-----BEGIN PUBLIC KEY-----\n" \
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCe+tG6/Rh/2M4r95hsjMPsofc7\n" \
"ZmJ6q1uxkumYNhz8W3VuIG2mtTNsA53DS0m6A93529M7OAfkeMXMxNoN/tHATDXH\n" \
"88jh2I6EuNdt/AbX0CYRX4Ja/+vh6NOdut30mY6fK4Xl1doOgFvLEorIM7TczzxJ\n" \
"+P42w74dtKdrmeKIKwIDAQAB\n" \
"-----END PUBLIC KEY-----\n";

const unsigned char ks2_pk_key[] = \
"-----BEGIN PUBLIC KEY-----\n" \
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC+4qH0aKPA4izGTmFzG+Fx1nX2\n" \
"15bZLryf9C5TBwlNu+a5XfuckPNgNG+SChENRvbuWLgMyEaaOv4eNHluwGGoMS1f\n" \
"Dav4d89atcchvwXPYCKSHbFRy6vi1zQqHZal9pRsdHcDeXZ9g+vwfwQ7vQZ8FQgB\n" \
"dnoKNOh8wHg96ZwWbQIDAQAB\n" \
"-----END PUBLIC KEY-----\n";


const unsigned char ks3_pk_key[] = \
"-----BEGIN PUBLIC KEY-----\n" \
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDJ6ocHk4CJ9AEYSs7jKrLKFw3e\n" \
"uALZ9iZBxWKD61EuRKtyASkH4x6BZP99Wn7mEziGolCybdKJteuyt56AkCEAft1D\n" \
"uLsqXoY69L1Hm9iqOSmHHlI81wYHqatja4ERQpTEqtpIUbr2E1Jw746Uuue08p5Y\n" \
"1CVuO+xjIxxdHJ27mQIDAQAB\n" \
"-----END PUBLIC KEY-----\n";

const unsigned char ks4_pk_key[] = \
"-----BEGIN PUBLIC KEY-----\n" \
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDAKR6YSmcP5oR9/pDIpm82GIPP\n" \
"89Bp75eO6kdXgiXPSzcpTTWv9n6EawoR2K8GPr6tuucYDXqhodqi26kUw1suxzji\n" \
"h7ykEufhYRXTyY9f5IEhMTa39LTloWkWhl22ddIz83jEMTjF+Uy44aR/Lw++baLd\n" \
"oy0+5r4UfMss0FyYKQIDAQAB\n" \
"-----END PUBLIC KEY-----\n";

const unsigned char ks5_pk_key[] = \
"-----BEGIN PUBLIC KEY-----\n" \
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCzEUsbSL7OgdvLuBohciQEz0nr\n" \
"SAEBBbzG/eRUsRYF986R5EE6gbf4xTsmhUlrwhjq5BEI8J0BaZ61Xzo/ym+w0sar\n" \
"ByWZsY67U5xcas+/qD/92zGl5nGeEM0JRmf2orMqFq0HPSC07SI1by8F0jPoFR1h\n" \
"WLYgLz9NYn7igT7fjQIDAQAB\n" \
"-----END PUBLIC KEY-----\n";



/*
 * IOT RSA public and prrivate keys generated by openssl 
 */

/* public key 1024 bit */
const unsigned char iot_pk_key[]= \
"-----BEGIN PUBLIC KEY-----\n" \
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC007GfgttefDhYI5Mcv7IrPlWp\n" \
"62tCsqBdcfP/UqneljzLl0uk+1sSMCvR7oCtL+kZcCDbkbv4CjFCKkuhwebZ7YNM\n" \
"TRJ6ws37emQfS8IM7XRlYaIQmmOacDvFFjrWBd0pYKXnP8ZnBSPVmKHu8as+mz0i\n" \
"7BnKdUGhRn9atFzZDQIDAQAB\n" \
"-----END PUBLIC KEY-----\n";

const unsigned char iot_pr_key[]= \
"-----BEGIN RSA PRIVATE KEY-----\n" \
"MIICXAIBAAKBgQC007GfgttefDhYI5Mcv7IrPlWp62tCsqBdcfP/UqneljzLl0uk\n" \
"+1sSMCvR7oCtL+kZcCDbkbv4CjFCKkuhwebZ7YNMTRJ6ws37emQfS8IM7XRlYaIQ\n" \
"mmOacDvFFjrWBd0pYKXnP8ZnBSPVmKHu8as+mz0i7BnKdUGhRn9atFzZDQIDAQAB\n" \
"AoGAU75CYXwJug1PTspS5BqHGe3JYGMNjpsJF52hgVo4H0R2rVbJCoP53kd+079f\n" \
"ylUI3+YE4YrxgWK/A0RxOF2DWhusL0MAUdg2sSDf9htfiCbflBD0scYFXCLQOr+9\n" \
"3GNoNIv5nPIzuqDCTbr7leAQTJfYGoz5rlcfd7s99astOkECQQDsn8fyi12I2sHc\n" \
"LkA5DngTq/z3M0cTRxhU0Bh1Hge2H2eJTZRjaD5GXnEjZvFmDU/7luUJYUzxyTYY\n" \
"IgH68Hy9AkEAw6JEYjlFqFaXPOHcyNEoRsk8J5cXGofwY4ABaifYg+EU9o9vs0+I\n" \
"8DCzyrRZbU0jt/GX8zvXJEuhxck5+pcakQJBAOv5aEBUhcHuTvhSU4/TAyKzGQI5\n" \
"a/8onnYuVMWvXgddCDbgXERKeBhbJL8mcUTRr9r6H40cMMzLWZv1hj4HyI0CQD50\n" \
"PRSGaCB3lEyRZmSNsSf38kZJS8zifPGm2czD77EaWBDmdahuya60PZGGxc3JBJAi\n" \
"hnnWLradloWH2gSP3KECQB2GGMO2Lpjvn1d2Zcpf/r4TpYYa5LbQ9sBIUyj+Obvs\n" \
"H3JAp8zyNQe3mFnp64DI3y0wB95+lU0EULtL95DhYvk=\n" \
"-----END RSA PRIVATE KEY-----\n";

WiFiClient espClient;
SecMqtt mqttclient(espClient);

/*
 * call plattform-specific  TRNG driver
 */
static int myrand(void *rng_state, unsigned char *output, size_t len)
{
    size_t olen;
    return mbedtls_hardware_poll(rng_state, output, len, &olen);
}


/* 
 *  adjust the length of the ciphered text
 */
void bufferSize(char* text, int &length)
{
  int i = strlen(text);
  int buf = round(i / BLOCK_SIZE) * BLOCK_SIZE;
  // length = (buf <= i) ? buf + BLOCK_SIZE : length = buf;
  length = (buf <= i) ? buf + BLOCK_SIZE : buf;
}

static uint8_t  *rand_string(uint8_t  *str, size_t size) {
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXZY";
  if (size) {
    --size;
    for (size_t n = 0; n < size; n++) {
      int key = rand() % 16;
      str[n] = key;
    }
  }
  return str;
}

uint8_t* rand_string_alloc(size_t size) {
  uint8_t* s =(uint8_t  *) malloc(size);
  if (s) {
    rand_string(s, size);
  }
  return s;
}


void setup_wifi() {
    delay(10);
    
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(ssid, password);
    unsigned long b_conap= micros();

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    unsigned long e_conap= micros();
    Serial.print(" Time to connect to AP is :" );
    Serial.print(e_conap - b_conap);
    Serial.println(" (us)"); 
    randomSeed(micros());

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}


void reconnect() {
  
    // Loop until we're reconnected
    while (!mqttclient.connected()) {
        Serial.print("Attempting MQTT connection...");

        mqttclient.SecConnect(clientId.c_str());
        delay(5000);
    }
}


/*
 * Print array
 * size: the size of the array 
 * beg: index to start printing 
 */
void dbgPrint(uint8_t * arr, int beg ,int size) {
  Serial.println();
  int start = beg; //defualt = 0
  int row = 0 ;
  Serial.print(row); 
  Serial.print(" >> ");  
  for (int i = start; i < size; i++)
  {
     if(i > 0 && i % BLOCK_SIZE == 0)
     {
      row++;
      Serial.println(); 
      Serial.print(row); 
      Serial.print(" >> ");
     }
    Serial.print(arr[i], HEX); 
    Serial.print(" "); 
  }
  Serial.println();
}

void setup() {
  //pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(13, OUTPUT);
  Serial.begin(115200);

  setup_wifi();
  mymsg = rand_string_alloc(Size_Byte);

  clientId += String(random(0xffff), HEX);
  
  mqttclient.setServer(mqtt_server, mqttPort);

  Serial.println("set keys");
  mqttclient.secmqtt_load_ksn_pk_key(ks1_pk_key, sizeof(ks1_pk_key), 1);
  mqttclient.secmqtt_load_ksn_pk_key(ks2_pk_key, sizeof(ks2_pk_key), 2);
  mqttclient.secmqtt_load_ksn_pk_key(ks3_pk_key, sizeof(ks3_pk_key), 3);
  mqttclient.secmqtt_load_ksn_pk_key(ks4_pk_key, sizeof(ks4_pk_key), 4);
  mqttclient.secmqtt_load_ksn_pk_key(ks5_pk_key, sizeof(ks5_pk_key), 5);
  
  mqttclient.secmqtt_set_iot_pk_key(iot_pk_key, (int)sizeof(iot_pk_key));
  mqttclient.secmqtt_set_iot_pr_key(iot_pr_key, (int)sizeof(iot_pr_key));
  mqttclient.secmqtt_set_enc_mode("rsa");
  mqttclient.secmqtt_set_secret_share_mode("sss");

  /* 
   * 1. connection to Key Store will be done only once during setup 
   * 2. session key will be updated once during setup
   */
  unsigned long se_b = micros();
  mqttclient.SecConnect(clientId.c_str());
  unsigned long se_e = micros();

  #ifdef TIME_DEBUG
  // phase1: connect to broker + tenc + tsend + tothers
  // phase2: max(trecv) 

  /* tack1 = keystore: tdec + tenc + tsend + tothers
   * 
   * 
     tks1 = tack1 + tdec + tcmp done
     tks2 = tack2 + tdec + tcmp done
     tks3 = tack3 + tdec + tcmp done
  */
  
  Serial.printf("time connect: \t\t%lu (us)\n", mqttclient.time_info.t_connect);
  Serial.printf("time enc: \t\t%lu (us)\n", mqttclient.time_info.t_enc);
  Serial.printf("time send: \t\t%lu (us)\n", mqttclient.time_info.t_send_pk);
  Serial.printf("time phase 1: \t\t%lu (us)\n", mqttclient.time_info.t_p11 + mqttclient.time_info.t_connect);

  for (int i = 0; i < KSN_NUM; i++) {
    Serial.printf("time receive ack%d: \t%lu (us)\n", i+1, mqttclient.time_info.t_recv[i]);
  }

  Serial.printf("time dec: \t\t%lu (us)\n", mqttclient.time_info.t_dec);
  Serial.printf("time phase 2: \t\t%lu (us)\n", se_e - se_b - mqttclient.time_info.t_p11);
  #endif
}

void loop() {
  
  if (!mqttclient.connected()) {
    reconnect();
  }

  unsigned long now = millis();
  if (now - lastMsg > 3000) {
    lastMsg = now;

    if(mqttclient.get_state() == SECMQTT_KS_CONNECTED) {
        Serial.printf("test %d\n", test_times);
        mqttclient.SecPublish(topic, mymsg, Size_Byte);
        test_times += 1;
    } else {
        mqttclient.SecConnect(clientId.c_str());
    }

    #ifdef RECON
    mqttclient.disconnect();
    #endif
  }
  
  mqttclient.loop();
}

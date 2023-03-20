#ifndef _CONNECTION_H
#define _CONNECTION_H
/*WIFI configuration*/
#define ESP_WIFI_SSID      "HOKKEY168"
#define ESP_WIFI_PASS      "11112222"
//#define ESP_WIFI_SSID      "AIFARM ROBOTICS FACTORY"
//#define ESP_WIFI_PASS      "@AIFARM2022"
#define ESP_MAXIMUM_RETRY  10
#define ESP_WIFI_CHANNEL   0
#define MAX_STA_CONN       4

static const char *TAG1 = "wifi station";
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

void wifi_init_sta(void);
void wifi_init_softap(void);

#endif


// This #include statement was automatically added by the Particle IDE.
#include "IoTHub.h"
#include "application.h"
#include "sha256.h"

#define HOST "IoTCampAU.azure-devices.net"
#define DEVICE "photon"
#define KEY "LO/v4iQMTGcebUhCYLMz5d+gl7vgr1AnNbf6UtouvDw="

#define LET_ENCRYPT_CA_PEM                                               \
  "-----BEGIN CERTIFICATE----- \r\n"                                     \
  "MIIFjTCCA3WgAwIBAgIRANOxciY0IzLc9AUoUSrsnGowDQYJKoZIhvcNAQELBQAw\r\n" \
  "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\r\n" \
  "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTYxMDA2MTU0MzU1\r\n" \
  "WhcNMjExMDA2MTU0MzU1WjBKMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg\r\n" \
  "RW5jcnlwdDEjMCEGA1UEAxMaTGV0J3MgRW5jcnlwdCBBdXRob3JpdHkgWDMwggEi\r\n" \
  "MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCc0wzwWuUuR7dyXTeDs2hjMOrX\r\n" \
  "NSYZJeG9vjXxcJIvt7hLQQWrqZ41CFjssSrEaIcLo+N15Obzp2JxunmBYB/XkZqf\r\n" \
  "89B4Z3HIaQ6Vkc/+5pnpYDxIzH7KTXcSJJ1HG1rrueweNwAcnKx7pwXqzkrrvUHl\r\n" \
  "Npi5y/1tPJZo3yMqQpAMhnRnyH+lmrhSYRQTP2XpgofL2/oOVvaGifOFP5eGr7Dc\r\n" \
  "Gu9rDZUWfcQroGWymQQ2dYBrrErzG5BJeC+ilk8qICUpBMZ0wNAxzY8xOJUWuqgz\r\n" \
  "uEPxsR/DMH+ieTETPS02+OP88jNquTkxxa/EjQ0dZBYzqvqEKbbUC8DYfcOTAgMB\r\n" \
  "AAGjggFnMIIBYzAOBgNVHQ8BAf8EBAMCAYYwEgYDVR0TAQH/BAgwBgEB/wIBADBU\r\n" \
  "BgNVHSAETTBLMAgGBmeBDAECATA/BgsrBgEEAYLfEwEBATAwMC4GCCsGAQUFBwIB\r\n" \
  "FiJodHRwOi8vY3BzLnJvb3QteDEubGV0c2VuY3J5cHQub3JnMB0GA1UdDgQWBBSo\r\n" \
  "SmpjBH3duubRObemRWXv86jsoTAzBgNVHR8ELDAqMCigJqAkhiJodHRwOi8vY3Js\r\n" \
  "LnJvb3QteDEubGV0c2VuY3J5cHQub3JnMHIGCCsGAQUFBwEBBGYwZDAwBggrBgEF\r\n" \
  "BQcwAYYkaHR0cDovL29jc3Aucm9vdC14MS5sZXRzZW5jcnlwdC5vcmcvMDAGCCsG\r\n" \
  "AQUFBzAChiRodHRwOi8vY2VydC5yb290LXgxLmxldHNlbmNyeXB0Lm9yZy8wHwYD\r\n" \
  "VR0jBBgwFoAUebRZ5nu25eQBc4AIiMgaWPbpm24wDQYJKoZIhvcNAQELBQADggIB\r\n" \
  "ABnPdSA0LTqmRf/Q1eaM2jLonG4bQdEnqOJQ8nCqxOeTRrToEKtwT++36gTSlBGx\r\n" \
  "A/5dut82jJQ2jxN8RI8L9QFXrWi4xXnA2EqA10yjHiR6H9cj6MFiOnb5In1eWsRM\r\n" \
  "UM2v3e9tNsCAgBukPHAg1lQh07rvFKm/Bz9BCjaxorALINUfZ9DD64j2igLIxle2\r\n" \
  "DPxW8dI/F2loHMjXZjqG8RkqZUdoxtID5+90FgsGIfkMpqgRS05f4zPbCEHqCXl1\r\n" \
  "eO5HyELTgcVlLXXQDgAWnRzut1hFJeczY1tjQQno6f6s+nMydLN26WuU4s3UYvOu\r\n" \
  "OsUxRlJu7TSRHqDC3lSE5XggVkzdaPkuKGQbGpny+01/47hfXXNB7HntWNZ6N2Vw\r\n" \
  "p7G6OfY+YQrZwIaQmhrIqJZuigsrbe3W+gdn5ykE9+Ky0VgVUsfxo52mwFYs1JKY\r\n" \
  "2PGDuWx8M6DlS6qQkvHaRUo0FMd8TsSlbF0/v965qGFKhSDeQoMpYnwcmQilRh/0\r\n" \
  "ayLThlHLN81gSkJjVrPI0Y8xCVPB4twb1PFUd2fPM3sA1tJ83sZ5v8vgFv2yofKR\r\n" \
  "PB0t6JzUA81mSqM3kxl5e+IZwhYAyO0OTg3/fs8HqGTNKd9BqoUwSRBzp06JMg5b\r\n" \
  "rUCGwbCUDI0mxadJ3Bz4WxR6fyNpBK2yAinWEsikxqEt\r\n"                     \
  "-----END CERTIFICATE----- "

char *letencryptCaPem = LET_ENCRYPT_CA_PEM;

IoT iotHub(HOST, DEVICE, KEY, letencryptCaPem);

const char *json = "{\"utc\":\"%s\",\"Celsius\":%.2f, \"hPa\":%.0f, \"Humidity\":%.0f, \"Id\":%d, \"Mem\":%d}";

int count;

#define DATASIZE 256
char data[DATASIZE];
char startUtc[30];

#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
unsigned long lastSync = millis();

void setup()
{
  Serial.begin(9600);
  delay(1000);

  Particle.syncTime();
  lastSync = millis();

  String startTime = Time.timeStr();
  Serial.print(startTime);

  strncpy(startUtc, startTime.c_str(), sizeof(startUtc) - 1);
}

void loop()
{

  if (!WiFi.ready())
  {
    Serial.println("wifi connecting");
    delay(5);
    // WiFi.connect();
    return;
  }

  if (millis() - lastSync > ONE_DAY_MILLIS)
  { // keep time in sync for epoch sas token generation
    Particle.syncTime();
    lastSync = millis();
    Serial.print(Time.timeStr());
  }

  count++;
  // Serial.println(count);

  snprintf(data, DATASIZE, json, startUtc, 21.0, 1010.0, 80.0, count, System.freeMemory());

  iotHub.publish(data);

  // Serial.println(iotHub.publish(data));

  delay(100);
}
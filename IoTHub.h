#ifndef IoTHub_h
#define IoTHub_h

#include "application.h"

#include <Base64.h>
#include "sha256.h"
#include <TlsTcpClient.h>



#define FULL_SAS_SIZE 150
#define BUFSIZE 512

class IoT
{
public:
  IoT(char * host, char * deviceId, char * key, char * letencryptCaPem){
    this->host = host;
    this->deviceId = deviceId;
    this->key = key;
    this->letencryptCaPem = letencryptCaPem;

    initialiseHub();
  }
  
  char * publish(char * data);

  char *host; // = "IoTCampAU.azure-devices.net";
  char *deviceId; // = "photon";
  char *key; // = "LO/v4iQMTGcebUhCYLMz5d+gl7vgr1AnNbf6UtouvDw=";

  time_t sasExpiryTime = 0;
  time_t sasExpiryPeriodInSeconds = 60 * 15; // Default to 15 minutes

protected:


private:

    TlsTcpClient * client = new TlsTcpClient();
    struct tm t;
    char * letencryptCaPem;

    char fullSas[250];
    char endPoint[100];
    unsigned char buff[BUFSIZE];
    
    String sasUrl;
    
    String createSas(char *key, String url);
    int buildHttpRequestion(unsigned char* buffer, int len, char* data);
    void generateSas();
    void initialiseHub();
    time_t currentEpochTime();
    time_t tmConvert_t(int YYYY, byte MM, byte DD, byte hh, byte mm, byte ss);
    String urlEncode(const char *msg);
    
    // const char *letencryptCaPem = LET_ENCRYPT_CA_PEM;
    
    const char *TARGET_URL = "/devices/";
    const char *IOT_HUB_END_POINT = "/messages/events?api-version=2016-02-03";
    const char *httpRequest = "POST %s HTTP/1.1\r\nHost: %s\r\nAuthorization: SharedAccessSignature %s\r\nContent-Type: application/atom+xml;type=entry;charset=utf-8\r\nContent-Length: %d\r\n\r\n%s";
};

#endif
#include "IoTHub.h"



void IoT::initialiseHub(){
  sasUrl = (String)host + urlEncode(TARGET_URL) + (String)deviceId;

  memset(endPoint, 0, 100);
  snprintf(endPoint, 100, "%s%s%s", TARGET_URL, deviceId, IOT_HUB_END_POINT);
}


String IoT::urlEncode(const char* msg)
{
    const char *hex = "0123456789abcdef";
    String encodedMsg = "";

    while (*msg!='\0'){
        if( ('a' <= *msg && *msg <= 'z')
                || ('A' <= *msg && *msg <= 'Z')
                || ('0' <= *msg && *msg <= '9') ) {
            encodedMsg += *msg;
        } else {
            encodedMsg += '%';
            encodedMsg += hex[*msg >> 4];
            encodedMsg += hex[*msg & 15];
        }
        msg++;
    }
    return encodedMsg;
}


String IoT::createSas(char* key, String url){    

    sasExpiryTime = currentEpochTime() + sasExpiryPeriodInSeconds;

    String stringToSign = url + "\n" + sasExpiryTime;
    int keyLength = strlen(key);
    
    int decodedKeyLength = base64_dec_len(key, keyLength);
    memset(buff, 0, decodedKeyLength + 1);  // initialised extra byte to allow for null termination
    base64_decode((char*)buff, key, keyLength);  //decode key
    
    Sha256.initHmac((const uint8_t*)buff, decodedKeyLength);
    Sha256.print(stringToSign);

    char* sign = (char*) Sha256.resultHmac();
    
    int encodedSignLen = base64_enc_len(HASH_LENGTH);   // START: Get base64 of signature
    memset(buff, 0, encodedSignLen + 1);    // initialised extra byte to allow for null termination
    base64_encode((char*)buff, sign, HASH_LENGTH); 
    
    String SharedAccessSignature = "sr=" + url + "&sig=" + urlEncode((char*)buff) + "&se=" + sasExpiryTime;
    
    return SharedAccessSignature;
}


void IoT::generateSas(){
    if (currentEpochTime() < sasExpiryTime) { return; }
    
    String token = createSas(key, sasUrl);
    strncpy(fullSas, token.c_str(), sizeof(fullSas) - 1);
}


char * IoT::publish(char * data){
    
    if (!client->isConnected()){
        client->init(letencryptCaPem, strlen(letencryptCaPem) + 1); // it wants the length on the cert string include null terminator
        client->connect(host, 443);
    }
    
    if (!client->isConnected()) { return "Not Connected"; }

    generateSas();
    

    memset(buff, 0, BUFSIZE);
    int postLen = buildHttpRequestion(buff, BUFSIZE - 1, data);
    
    client->write(buff, postLen);
    

    memset(buff, 0, BUFSIZE);
    while(true) {
        delay(100);
        memset(buff, 0, BUFSIZE);
        int ret = client->read(buff, BUFSIZE - 1);
        if (ret == MBEDTLS_ERR_SSL_WANT_READ) {
            delay(100);
        } else if (client->available() == 0) {
            break;
        }
    }
    
    return (char *)buff;
}


int IoT::buildHttpRequestion(unsigned char* buffer, int len, char* data){
    memset(buffer, 0, len);
    return snprintf((char*)buffer, len, httpRequest, endPoint, host, fullSas, strlen(data), data);
}

time_t IoT::currentEpochTime(){
    return tmConvert_t(Time.year(), Time.month(), Time.day(), Time.hour(), Time.minute(), Time.second());
}


time_t IoT::tmConvert_t(int YYYY, byte MM, byte DD, byte hh, byte mm, byte ss)
{
  t.tm_year = YYYY-1900;
  t.tm_mon = MM - 1;
  t.tm_mday = DD;
  t.tm_hour = hh;
  t.tm_min = mm;
  t.tm_sec = ss;
  t.tm_isdst = 0;

  
  time_t t_of_day = mktime(&t);
  return t_of_day;
}


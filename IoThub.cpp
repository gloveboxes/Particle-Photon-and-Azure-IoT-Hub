#include "IoTHub.h"

void IoT::initialiseHub()
{
    urlEncode(buff, (char *)TARGET_URL);
    sasUrl = (String)host + buff + (String)deviceId;

    memset(endPoint, 0, sizeof(endPoint));
    snprintf(endPoint, sizeof(endPoint), "%s%s%s\0", TARGET_URL, deviceId, IOT_HUB_END_POINT);
}

int IoT::urlEncode(char *dest, char *msg)
{
    const char *hex = "0123456789abcdef";
    char *startPtr = dest;

    while (*msg != '\0')
    {
        if (('a' <= *msg && *msg <= 'z') || ('A' <= *msg && *msg <= 'Z') || ('0' <= *msg && *msg <= '9'))
        {
            *dest++ = *msg;
        }
        else
        {
            *dest++ = '%';
            *dest++ = hex[*msg >> 4];
            *dest++ = hex[*msg & 15];
        }
        msg++;
    }
    *dest = '\0';
    return dest - startPtr;
}

void IoT::createSas(char *key, String url)
{
    sasExpiryTime = currentEpochTime() + sasExpiryPeriodInSeconds;

    int keyLength = strlen(key);

    int decodedKeyLength = base64_dec_len(key, keyLength);
    memset(buff, 0, decodedKeyLength + 1); // initialised extra byte to allow for null termination

    base64_decode(buff, key, keyLength); //decode key
    Sha256.initHmac((const uint8_t *)buff, decodedKeyLength);

    int len = snprintf(buff, sizeof(buff), "%s\n%d\0", url.c_str(), sasExpiryTime);
    Sha256.print(buff);

    char *sign = (char *)Sha256.resultHmac();

    int encodedSignLen = base64_enc_len(HASH_LENGTH); // START: Get base64 of signature
    memset(buff, 0, encodedSignLen + 1);              // initialised extra byte to allow for null termination

    base64_encode(buff, sign, HASH_LENGTH);

    // String SharedAccessSignature = "sr=" + url + "&sig=" + urlEncode(buff) + "&se=" + sasExpiryTime;

    char *sasPointer = fullSas;
    sasPointer += snprintf(sasPointer, sizeof(fullSas), "sr=%s&sig=", url.c_str());
    sasPointer += urlEncode(sasPointer, buff);
    snprintf(sasPointer, sizeof(fullSas) - (sasPointer - fullSas), "&se=%d\0", sasExpiryTime);
}

void IoT::generateSas()
{
    if (currentEpochTime() < sasExpiryTime)
    {
        return;
    }
    createSas(key, sasUrl);
}

void IoT::flush()
{
    int maxRetry = 0;
    while (WiFi.ready() && client->isConnected() && client->available() > 0 && maxRetry < 20)
    {
        maxRetry++;
        int ret = client->read((unsigned char *)buff, BUFSIZE - 1);
        if (ret == MBEDTLS_ERR_SSL_WANT_READ)
        {
            delay(150);
        }
    }
}

char *IoT::publishEnd()
{
    int maxRetry = 0;
    
    if (!WiFi.ready() || !client->isConnected()) {
        return "not connected";
    }
    
    delay(200);
    
    while (WiFi.ready() && client->isConnected() && client->available() == 0 && maxRetry < 10){
        delay(200);
        maxRetry++;
    }
    
    if (maxRetry == 10) {
        return "no response";
    }
    
    maxRetry = 0;
    memset(buff, 0, BUFSIZE);
    
    while (maxRetry < 20  && WiFi.ready() && client->isConnected())
    {
        maxRetry++;
        delay(150);
        memset(buff, 0, BUFSIZE);
        int ret = client->read((unsigned char *)buff, BUFSIZE - 1); // allow for null termination
        if (ret == MBEDTLS_ERR_SSL_WANT_READ)
        {
            delay(150);
        }
        else if (client->available() == 0)
        {
            break;
        }
    }
    return buff;
}

int IoT::publishBegin(int dataLength)
{
    generateSas();
    
    flush(); // flush response buffer before next HTTP POST


    memset(buff, 0, BUFSIZE);
    int postLen = buildHttpRequestion(buff, BUFSIZE, dataLength);

    return publishData(buff, postLen);
}

int IoT::publishData(char *data, int dataLength)
{
    int startChar = 0;
    unsigned char *dataPointer = (unsigned char *)data;
    int ret  = 0;
    int len = 0;
    int totalBytes = -1;

    
    if (WiFi.ready() && !client->isConnected())
    {
        client->init(letencryptCaPem, strlen(letencryptCaPem) + 1); // it wants the length on the cert string include null terminator
        client->connect(host, 443);
    }
    

    while (startChar < dataLength && WiFi.ready() && client->isConnected()) // write out data in chunks
    {
        delay(10);
        len = (startChar + SEGMENT_LENGTH < dataLength) ? SEGMENT_LENGTH : dataLength - startChar;
        ret = client->write(dataPointer, len);
        if (ret < 0) {
            return ret;
        }
        totalBytes += ret;
        startChar += len;
        dataPointer += len;
    }
    return totalBytes;
}

char *IoT::publish(char *data)
{
    int result = 0;
    int dataLength = strlen(data);

    digitalWrite(statusLed, HIGH);

    result = publishBegin(dataLength);
    if (result < 0){
        // client->close();
        Serial.println("write fail");
        digitalWrite(statusLed, LOW);
        return "http post write failure";
    }

    result = publishData(data, dataLength);
    if (result < 0){
        // client->close();
        Serial.println("write fail");
        digitalWrite(statusLed, LOW);
        return "http post write failure";
    }
    
    digitalWrite(statusLed, LOW);

    digitalWrite(builtinled, HIGH); // toggle status led

    char *response = publishEnd();

    digitalWrite(builtinled, LOW); // toggle status led
    

    return response;
}

int IoT::buildHttpRequestion(char *buffer, int len, int dataLength)
{
    memset(buffer, 0, len);
    return snprintf(buffer, len, httpRequest, endPoint, host, fullSas, dataLength);
}

time_t IoT::currentEpochTime()
{
    return tmConvert_t(Time.year(), Time.month(), Time.day(), Time.hour(), Time.minute(), Time.second());
}

time_t IoT::tmConvert_t(int YYYY, byte MM, byte DD, byte hh, byte mm, byte ss)
{
    t.tm_year = YYYY - 1900;
    t.tm_mon = MM - 1;
    t.tm_mday = DD;
    t.tm_hour = hh;
    t.tm_min = mm;
    t.tm_sec = ss;
    t.tm_isdst = 0;

    time_t t_of_day = mktime(&t);
    return t_of_day;
}

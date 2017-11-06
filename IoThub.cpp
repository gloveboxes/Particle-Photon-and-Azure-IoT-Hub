#include "IoTHub.h"

void IoT::initialiseHub()
{
  sasUrl = (String)host + urlEncode(TARGET_URL) + (String)deviceId;

  memset(endPoint, 0, 100);
  snprintf(endPoint, 100, "%s%s%s", TARGET_URL, deviceId, IOT_HUB_END_POINT);
}

String IoT::urlEncode(const char *msg)
{
  const char *hex = "0123456789abcdef";
  String encodedMsg = "";

  while (*msg != '\0')
  {
    if (('a' <= *msg && *msg <= 'z') || ('A' <= *msg && *msg <= 'Z') || ('0' <= *msg && *msg <= '9'))
    {
      encodedMsg += *msg;
    }
    else
    {
      encodedMsg += '%';
      encodedMsg += hex[*msg >> 4];
      encodedMsg += hex[*msg & 15];
    }
    msg++;
  }
  return encodedMsg;
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

// String IoT::createSas(char* key, String url){

//     sasExpiryTime = currentEpochTime() + sasExpiryPeriodInSeconds;

//     String stringToSign = url + "\n" + sasExpiryTime;
//     int keyLength = strlen(key);

//     int decodedKeyLength = base64_dec_len(key, keyLength);
//     memset(buff, 0, decodedKeyLength + 1);  // initialised extra byte to allow for null termination
//     base64_decode((char*)buff, key, keyLength);  //decode key

//     Sha256.initHmac((const uint8_t*)buff, decodedKeyLength);
//     Sha256.print(stringToSign);

//     char* sign = (char*) Sha256.resultHmac();

//     int encodedSignLen = base64_enc_len(HASH_LENGTH);   // START: Get base64 of signature
//     memset(buff, 0, encodedSignLen + 1);    // initialised extra byte to allow for null termination
//     base64_encode((char*)buff, sign, HASH_LENGTH);

//     String SharedAccessSignature = "sr=" + url + "&sig=" + urlEncode((char*)buff) + "&se=" + sasExpiryTime;

//     return SharedAccessSignature;
// }

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

void IoT::sendData(char *data, int dataLength)
{
  int startChar = 0;
  unsigned char *dataPointer = (unsigned char *)data;

  while (startChar + SEGMENT_LENGTH < dataLength) // write out data in chunks
  {
    delay(10);
    client->write(dataPointer, SEGMENT_LENGTH);
    startChar += SEGMENT_LENGTH;
    dataPointer += SEGMENT_LENGTH;
  }

  dataLength = dataLength - startChar;
  delay(10);
  client->write(dataPointer, dataLength);
}

void IoT::flush()
{
  int maxRead = 0;
  while (client->available() && maxRead < 50)
  {
    maxRead++;
    int ret = client->read((unsigned char *)buff, BUFSIZE - 1);
    if (ret == MBEDTLS_ERR_SSL_WANT_READ)
    {
      delay(150);
    }
  }
}

char *IoT::publishReadResponse()
{
  int maxRead = 0;
  delay(200);
  memset(buff, 0, BUFSIZE);
  while (true && maxRead < 50)
  {
    maxRead++;
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

bool IoT::publishBegin(int dataLength)
{
  if (!WiFi.ready())
  {
    return false;
  }

  if (!client->isConnected())
  {
    client->init(letencryptCaPem, strlen(letencryptCaPem) + 1); // it wants the length on the cert string include null terminator
    client->connect(host, 443);
  }

  if (!client->isConnected())
  {
    return false;
  }

  generateSas();

  flush(); // flush response buffer before next HTTP POST

  memset(buff, 0, BUFSIZE);
  int postLen = buildHttpRequestion(buff, BUFSIZE, dataLength);

  sendData((char *)buff, postLen);
}

void IoT::publishData(char *data, int dataLength)
{
  sendData(data, dataLength);
}

char *IoT::publish(char *data)
{

  int dataLength = strlen(data);

  digitalWrite(statusLed, HIGH);

  if (!publishBegin(dataLength))
  {
    return "No wifi or IoT Hub Connection";
  }

  publishData(data, dataLength);

  digitalWrite(statusLed, LOW);

  digitalWrite(builtinled, HIGH); // toggle status led

  char *response = publishReadResponse();

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

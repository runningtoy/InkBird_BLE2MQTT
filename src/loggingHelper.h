
#define CONFIG_LOG_DEFAULT_LEVEL ESP_LOG_VERBOSE
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "esp_log.h"
// #include <FS.h>
#include <SD_MMC.h>
#include "SPIFFS.h"

#undef ESP_LOGE
#undef ESP_LOGW
#undef ESP_LOGI
#undef ESP_LOGD
#undef ESP_LOGV

// #define ESP_LOGE( tag, format, ... )  if (LOG_LOCAL_LEVEL >= ESP_LOG_ERROR)   { esp_log_write(ESP_LOG_ERROR,   tag, LOG_FORMAT(E, format), esp_log_timestamp(), tag, ##__VA_ARGS__); }
// #define ESP_LOGW( tag, format, ... )  if (LOG_LOCAL_LEVEL >= ESP_LOG_WARN)    { esp_log_write(ESP_LOG_WARN,    tag, LOG_FORMAT(W, format), esp_log_timestamp(), tag, ##__VA_ARGS__); }
// #define ESP_LOGI( tag, format, ... )  if (LOG_LOCAL_LEVEL >= ESP_LOG_INFO)    { esp_log_write(ESP_LOG_INFO,    tag, LOG_FORMAT(I, format), esp_log_timestamp(), tag, ##__VA_ARGS__); }
// #define ESP_LOGD( tag, format, ... )  if (LOG_LOCAL_LEVEL >= ESP_LOG_DEBUG)   { esp_log_write(ESP_LOG_DEBUG,   tag, LOG_FORMAT(D, format), esp_log_timestamp(), tag, ##__VA_ARGS__); }
// #define ESP_LOGV( tag, format, ... )  if (LOG_LOCAL_LEVEL >= ESP_LOG_VERBOSE) { esp_log_write(ESP_LOG_VERBOSE, tag, LOG_FORMAT(V, format), esp_log_timestamp(), tag, ##__VA_ARGS__); }


#define ESP_LOGE( tag, format, ... )  { esp_log_write(ESP_LOG_ERROR,   tag, LOG_FORMAT(E, format), esp_log_timestamp(), tag, ##__VA_ARGS__); }
#define ESP_LOGW( tag, format, ... )  { esp_log_write(ESP_LOG_WARN,    tag, LOG_FORMAT(W, format), esp_log_timestamp(), tag, ##__VA_ARGS__); }
#define ESP_LOGI( tag, format, ... )  { esp_log_write(ESP_LOG_INFO,    tag, LOG_FORMAT(I, format), esp_log_timestamp(), tag, ##__VA_ARGS__); }
#define ESP_LOGD( tag, format, ... )  { esp_log_write(ESP_LOG_DEBUG,   tag, LOG_FORMAT(D, format), esp_log_timestamp(), tag, ##__VA_ARGS__); }
#define ESP_LOGV( tag, format, ... )  { esp_log_write(ESP_LOG_VERBOSE, tag, LOG_FORMAT(V, format), esp_log_timestamp(), tag, ##__VA_ARGS__); }


static char log_print_buffer[512];
int vprintf_into_SD(const char *szFormat, va_list args)
{
    //write evaluated format string into buffer
    int ret = vsnprintf(log_print_buffer, sizeof(log_print_buffer), szFormat, args);

    //output is now in buffer. write to file.
    if (ret >= 0)
    {
        if (!SD_MMC.exists("/LOGS.txt"))
        {
            File writeLog = SD_MMC.open("/LOGS.txt", FILE_WRITE);
            if (!writeLog)
                Serial.println("Couldn't open LOGS.txt");
            delay(50);
            writeLog.close();
        }

        File SDLogFile = SD_MMC.open("/LOGS.txt", FILE_APPEND);
        //debug output
        printf("[Writing to SD_MMC] %.*s\r\n", ret, log_print_buffer);
        SDLogFile.write((uint8_t *)log_print_buffer, (size_t)ret);
        SDLogFile.println("");
        //to be safe in case of crashes: flush the output
        SDLogFile.flush();
        SDLogFile.close();
    }
    return ret;
}





int vprintf_into_spiffs(const char *szFormat, va_list args)
{
    //write evaluated format string into buffer
    int ret = vsnprintf(log_print_buffer, sizeof(log_print_buffer), szFormat, args);

    //output is now in buffer. write to file.
    if (ret >= 0)
    {
        if (!SPIFFS.exists("/LOGS.txt"))
        {
            File writeLog = SPIFFS.open("/LOGS.txt", FILE_WRITE);
            if (!writeLog)
                Serial.println("Couldn't open spiffs_log.txt");
            delay(50);
            writeLog.close();
        }

        File spiffsLogFile = SPIFFS.open("/LOGS.txt", FILE_APPEND);
        //debug output
        //printf("[Writing to SPIFFS] %.*s", ret, log_print_buffer);
        spiffsLogFile.write((uint8_t *)log_print_buffer, (size_t)ret);
        //to be safe in case of crashes: flush the output
        spiffsLogFile.flush();
        spiffsLogFile.close();
    }
    return ret;
}

void setLogLevel(){
    // esp_log_level_set("TAG", ESP_LOG_DEBUG);
    esp_log_level_set("*", ESP_LOG_ERROR);  
    esp_log_level_set("BBQ",ESP_LOG_DEBUG);
}

#if !defined(MQTT_LOGGING_H)
#define MQTT_LOGGING_H

#define STREAM      stdout
#if !defined(DEBUG)
#define DEBUG(...)    \
    {\
    fprintf(STREAM, "DEBUG:   %s L#%d ", __PRETTY_FUNCTION__, __LINE__);  \
    fprintf(STREAM, ##__VA_ARGS__); \
    fflush(STREAM); \
    }
#endif
#if !defined(LOG)
#define LOG(...)    \
    {\
    fprintf(STREAM, "LOG:   %s L#%d ", __PRETTY_FUNCTION__, __LINE__);  \
    fprintf(STREAM, ##__VA_ARGS__); \
    fflush(STREAM); \
    }
#endif
#if !defined(WARN)
#define WARN(...)   \
    { \
    fprintf(STREAM, "WARN:  %s L#%d ", __PRETTY_FUNCTION__, __LINE__);  \
    fprintf(STREAM, ##__VA_ARGS__); \
    fflush(STREAM); \
    }
#endif 
#if !defined(ERROR)
#define ERROR(...)  \
    { \
    fprintf(STREAM, "ERROR: %s L#%d ", __PRETTY_FUNCTION__, __LINE__); \
    fprintf(STREAM, ##__VA_ARGS__); \
    fflush(STREAM); \
    exit(1); \
    }
#endif

#endif

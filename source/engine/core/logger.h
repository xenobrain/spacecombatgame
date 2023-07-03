#ifndef ENGINE_CORE_LOGGER_H
#define ENGINE_CORE_LOGGER_H

//extern auto print(char const*, ...) -> void;

#define LOG_INFO(fmt, ...) do { \
print("%s:%d \n\t%s()\n\t\t" fmt, __FILE__, __LINE__, __func__, __VA_ARGS__); \
} while (0)

#define LOG_WARN(fmt, ...) do { \
print("%s:%d \n\t%s()\n\t\t" fmt, __FILE__, __LINE__, __func__, __VA_ARGS__); \
} while (0)

#define LOG_ERROR(fmt, ...) do { \
print("%s:%d \n\t%s()\n\t\t" fmt, __FILE__, __LINE__, __func__, __VA_ARGS__); \
} while (0)

#define LOG_VERBOSE(fmt, ...) do { \
print("%s:%d \n\t%s()\n\t\t" fmt, __FILE__, __LINE__, __func__, __VA_ARGS__); \
} while (0)

#endif
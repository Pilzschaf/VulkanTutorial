#pragma once
#include <string.h>
#include <stdint.h>
#include <sstream>
#include <string>

//TODO: Windows UTF-16 to UTF-8
//TODO: Variadic args
//TODO: Extern templates
//TODO: std::string printing

//#define LOGGING_DISABLE_INFO
//#define LOGGING_DISABLE_WARN
//#define LOGGING_DISABLE_ERROR

//#define LOG_DEBUG(message) logDebug(message, __LINE__, __FILE__)

#ifdef LOGGING_DISABLE_INFO
#define LOG_INFO(...)
#else
#define LOG_INFO(...) _logInfo(__LINE__, __FILE__, __VA_ARGS__)
#endif

#ifdef LOGGING_DISABLE_WARN
#define LOG_WARN(...)
#else
#define LOG_WARN(...) _logWarn(__LINE__, __FILE__, __VA_ARGS__)
#endif

#ifdef LOGGING_DISABLE_ERROR
#define LOG_ERROR(...)
#else
#define LOG_ERROR(...) _logError(__LINE__, __FILE__, __VA_ARGS__)
#endif

// Logger backends only have to supply these functions:
void initLogger(void);
//void log(const char* message, size_t messageLength, size_t lineNumber, const char* file);
//void logDebug(const char* message, size_t messageLength);
void logInfo(size_t lineNumber, const char* file, const char* message, size_t messageLength);
void logWarn(size_t lineNumber, const char* file, const char* message, size_t messageLength);
void logError(size_t lineNumber, const char* file, const char* message, size_t messageLength);
void exitLogger(void);

// int, long, long long, float, double, long double

inline void variadicUnpack(std::string& buf) {}

template<typename... Args>
void variadicUnpack(std::string& buf, int arg, Args... args);
template<typename... Args>
void variadicUnpack(std::string& buf, unsigned arg, Args... args);
template<typename... Args>
void variadicUnpack(std::string& buf, long arg, Args... args);
template<typename... Args>
void variadicUnpack(std::string& buf, unsigned long arg, Args... args);
template<typename... Args>
void variadicUnpack(std::string& buf, long long arg, Args... args);
template<typename... Args>
void variadicUnpack(std::string& buf, unsigned long long arg, Args... args);
template<typename... Args>
void variadicUnpack(std::string& buf, float arg, Args... args);
template<typename... Args>
void variadicUnpack(std::string& buf, double arg, Args... args);
template<typename... Args>
void variadicUnpack(std::string& buf, long double arg, Args... args);

template<typename... Args>
void variadicUnpack(std::string& buf, const char* arg, Args... args);
template<typename T, typename... Args>
void variadicUnpack(std::string& buf, T arg, Args... args);
template<typename... Args>
void variadicUnpack(std::string& buf, void* arg, Args... args);

template<typename T, typename... Args>
inline void variadicUnpack(std::string& buf, T arg, Args... args) {
    //TODO: Remove stringstream
    std::ostringstream oss;
    oss << arg;
    buf.append(oss.str());
    variadicUnpack(buf, args...);
}

template<typename... Args>
inline void variadicUnpack(std::string& buf, void* arg, Args... args) {
    //TODO: Remove stringstream
    std::ostringstream oss;
    oss << arg;
    buf.append(oss.str());
    variadicUnpack(buf, args...);
}

template<typename... Args>
inline void variadicUnpack(std::string& buf, char* arg, Args... args) {
    buf.append((const char*)arg);
    variadicUnpack(buf, args...);
}

template<typename... Args>
inline void variadicUnpack(std::string& buf, const char* arg, Args... args) {
    buf.append(arg);
    variadicUnpack(buf, args...);
}

template<typename... Args>
inline void variadicUnpack(std::string& buf, int arg, Args... args) {
    buf.append(std::to_string(arg));
    variadicUnpack(buf, args...);
}

template<typename... Args>
inline void variadicUnpack(std::string& buf, unsigned arg, Args... args) {
    buf.append(std::to_string(arg));
    variadicUnpack(buf, args...);
}

template<typename... Args>
inline void variadicUnpack(std::string& buf, long arg, Args... args) {
    buf.append(std::to_string(arg));
    variadicUnpack(buf, args...);
}

template<typename... Args>
inline void variadicUnpack(std::string& buf, unsigned long arg, Args... args) {
    buf.append(std::to_string(arg));
    variadicUnpack(buf, args...);
}

template<typename... Args>
inline void variadicUnpack(std::string& buf, long long arg, Args... args) {
    buf.append(std::to_string(arg));
    variadicUnpack(buf, args...);
}

template<typename... Args>
inline void variadicUnpack(std::string& buf, unsigned long long arg, Args... args) {
    buf.append(std::to_string(arg));
    variadicUnpack(buf, args...);
}

template<typename... Args>
inline void variadicUnpack(std::string& buf, float arg, Args... args) {
    buf.append(std::to_string(arg));
    variadicUnpack(buf, args...);
}

template<typename... Args>
inline void variadicUnpack(std::string& buf, double arg, Args... args) {
    buf.append(std::to_string(arg));
    variadicUnpack(buf, args...);
}

template<typename... Args>
inline void variadicUnpack(std::string& buf, long double arg, Args... args) {
    buf.append(std::to_string(arg));
    variadicUnpack(buf, args...);
}

template<typename... Args>
inline void _logInfo(size_t lineNumber, const char* file, const Args &... args) {
    std::string buf;
    variadicUnpack(buf, args...);
    logInfo(lineNumber, file, buf.c_str(), buf.size());
}

// Faster shortcut if no variadic arguments are used
template<size_t N>
inline void _logInfo(size_t lineNumber, const char* file, const char (& message)[N]) {
    //logInfo(0, 0, "Correct", 0);
    logInfo(lineNumber, file, message, N-1);
}

template<typename... Args>
inline void _logWarn(size_t lineNumber, const char* file, const Args &... args) {
    std::string buf;
    variadicUnpack(buf, args...);
    logWarn(lineNumber, file, buf.c_str(), buf.size());
}

// Faster shortcut if no variadic arguments are used
template<size_t N>
inline void _logWarn(size_t lineNumber, const char* file, const char (& message)[N]) {
    //logInfo(0, 0, "Correct", 0);
    logWarn(lineNumber, file, message, N-1);
}

template<typename... Args>
inline void _logError(size_t lineNumber, const char* file, const Args &... args) {
    std::string buf;
    variadicUnpack(buf, args...);
    logError(lineNumber, file, buf.c_str(), buf.size());
}

// Faster shortcut if no variadic arguments are used
template<size_t N>
inline void _logError(size_t lineNumber, const char* file, const char (& message)[N]) {
    //logInfo(0, 0, "Correct", 0);
    logError(lineNumber, file, message, N-1);
}

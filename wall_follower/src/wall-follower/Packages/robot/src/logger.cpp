/**
 * @file logger.h
 * @brief Implementation of a multithreaded safe singleton logger class
 *
 * @author Atabak Hafeez [atabakhafeez]
 * @author Maria Ficiu [MariaFiciu]
 * @author Rubin Deliallisi [rdeliallisi]
 * @author Siddharth Shukla [thunderboltsid]
 * @bug No known bugs.
 */
#include <stdexcept>
#include "logger.h"

using namespace std;

/**
* @brief Define the types of loggers
*/ 

const string Logger::log_level_debug = "DEBUG";
const string Logger::log_level_info = "INFO";
const string Logger::log_level_error = "ERROR";

/**
* @brief Define the name of the log file
*/ 
const char* const Logger::log_file_name = "test_log.out";

/**
* @brief Initialise the logger instance
*/ 
Logger* Logger::p_instance = nullptr;

/**
* @brief Initialise the mutex
*/ 
mutex Logger::s_mutex;

Logger& Logger::Instance() {
    static Cleanup cleanup;

    lock_guard<mutex> guard(s_mutex);
    if (p_instance == nullptr)
        p_instance = new Logger();
    return *p_instance;
}

Logger::Cleanup::~Cleanup() {
    lock_guard<mutex> guard(Logger::s_mutex);
    delete Logger::p_instance;
    Logger::p_instance = nullptr;
}

Logger::~Logger() {
    output_stream_.close();
}

Logger::Logger() {
    output_stream_.open(log_file_name, ios_base::app);
    if (!output_stream_.good()) {
        throw runtime_error("Unable to initialize the Logger!");
    }
}

void Logger::Log(const std::string &in_message, const std::string &in_log_level) {
    lock_guard<mutex> guard(s_mutex);
    LogHelper(in_message, in_log_level);
}

void Logger::Log(const std::vector <std::string> &in_messages, const std::string &in_log_level) {
    lock_guard<mutex> guard(s_mutex);
    for (size_t i = 0; i < in_messages.size(); i++) {
        LogHelper(in_messages[i], in_log_level);
    }
}

void Logger::LogHelper(const std::string& in_message, const std::string& in_log_level) {
    output_stream_ << in_log_level << ": " << in_message << endl;
}

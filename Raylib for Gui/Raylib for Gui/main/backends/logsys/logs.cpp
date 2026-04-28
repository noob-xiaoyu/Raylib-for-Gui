#include "logs.h"
#include <stdio.h>
#include <stdarg.h>
#include <mutex>
#include <chrono>
#include <thread>
#include <string>

namespace Log
{  
    static std::mutex logMutex;
    
    /// <summary>
    /// 
    /// </summary>
    /// <param name="logType"> Raylib, Imgui, Logs</param>
    void cout(prefix logType, const char* text, ...)
    {
        std::lock_guard<std::mutex> lock(logMutex);
        
        // // 获取当前时间
        // auto now = std::chrono::system_clock::now();
        // auto now_time_t = std::chrono::system_clock::to_time_t(now);
        // auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        //     now.time_since_epoch()) % 1000;
        
        // // 获取线程ID
        // std::string thread_id = std::to_string(
        //     std::hash<std::thread::id>{}(std::this_thread::get_id()));
        // if (thread_id.length() > 4) {
        //     thread_id = thread_id.substr(thread_id.length() - 4);
        // }
        
        // // 输出时间戳和线程ID
        // char time_buf[100];
        // struct tm tm_time;
        // localtime_s(&tm_time, &now_time_t);
        // std::strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tm_time);
        // printf("[%s.%03lld][%s] ", time_buf, now_ms.count(), thread_id.c_str());
        
        if (logType == Raylib)
        {
            printf("[Raylib] > ");
        }
        else if (logType == Font)
        {
            printf("[Font] > ");
		}
        else if (logType == config)
        {
            printf("[cfgsys] > ");
		}
        else if (logType == System)
        {
			printf("[system] > ");
        }
        else if (logType == logs)
        {
            printf("[Logs] > ");
        }
        va_list args;
        va_start(args, text);
        vprintf(text, args);
        va_end(args);
        printf("\n");
        fflush(stdout);
    }
}
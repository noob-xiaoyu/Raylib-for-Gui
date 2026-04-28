#pragma once

typedef enum { 
    Raylib,
    Font,
    config,
	System,
    logs 
} prefix;

namespace Log
{
    void cout(prefix logType, const char* text, ...);
}

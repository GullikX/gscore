#pragma once

#pragma pack(push, 1)
typedef struct {
    const char* key;
    const char* prompt;
    const char* items;
} QueryRequest;
#pragma pack(pop)

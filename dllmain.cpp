// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <Windows.h>
#include <stdio.h>
#include <detours/detours.h>

// Define the SODA config structure.
typedef void (*RecognitionResultHandler)(const char*, const bool, void*);

typedef struct
{
    // The channel count and sample rate of the audio stream. SODA does not
    // support changing these values mid-stream, so a new SODA instance must be
    // created if these values change.
    int channel_count;
    int sample_rate;

    // The fully-qualified path to the language pack.
    const char* language_pack_directory;

    // The callback that gets executed on a recognition event. It takes in a
    // char*, representing the transcribed text; a bool, representing whether the
    // result is final or not; and a void* pointer to the SodaRecognizerImpl
    // instance associated with the stream.
    RecognitionResultHandler callback;

    // A void pointer to the SodaRecognizerImpl instance associated with the
    // stream.
    void* callback_handle;

    // The API key for the SODA module.
    const char* api_key;
} SodaConfig;

// Declare the prototype for the function that will be detoured.
typedef void(__thiscall* CreateSodaAsync)(SodaConfig config);

// Get the address of the CreateSodaAsync function from the SODA module DLL.
void* detouredCreateSodaAsync = DetourFindFunction("SODA.dll", "CreateSodaAsync");

// Set trampoline to the original function.
static void(__thiscall* originalCreateSodaAsync)(SodaConfig config) = (CreateSodaAsync)detouredCreateSodaAsync;

// Define detour function.
void detourCreateSodaAsync(SodaConfig config)
{
    // Attach console.
    AllocConsole();
    FILE* stream;

    // Open standard output, and error for writing.
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);

    // Dump the config properties in standard output.
    printf("SodaConfig config = {channelCount: %d, sampleRate: %d, modelPath: \"%s\", apiKey: \"%s\"};\n", config.channel_count, config.sample_rate, config.language_pack_directory, config.api_key);

    // Call the original function.
    originalCreateSodaAsync(config);
}

__declspec(dllexport) void CALLBACK DetourFinishHelperProcess() {}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    // Detour the CreateSodaAsync function.
    if (DetourIsHelperProcess())
    {
        return TRUE;
    }

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach((PVOID*)&originalCreateSodaAsync, (PVOID)detourCreateSodaAsync);
        DetourTransactionCommit();
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
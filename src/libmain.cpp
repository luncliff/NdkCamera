
#ifdef _WIN32
#define PROLOGUE
#define EPILOGUE
#else
#define PROLOGUE __attribute__((constructor))
#define EPILOGUE __attribute__((destructor))
#endif

PROLOGUE void OnAttach(void *) noexcept(false)
{
    // On dll is attached...
    return;
}

EPILOGUE void OnDetach(void *) noexcept
{
    // On dll is detached...
    return;
}

#ifdef _WIN32
#include <Windows.h>

// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682583(v=vs.85).aspx
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
{
    try
    {
        if (fdwReason == DLL_PROCESS_ATTACH)
            ::OnAttach(hinstDLL);
        if (fdwReason == DLL_PROCESS_DETACH)
            ::OnDetach(hinstDLL);

        return TRUE;
    }
    catch (const std::exception &e)
    {
        ::perror(e.what());
        return FALSE;
    }
}

#endif

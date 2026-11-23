#include "astron_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#include <shellapi.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#else
#include <unistd.h>
#endif

uint32_t astron_process_memory_mb(void) {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        uint32_t mb = (uint32_t)(pmc.WorkingSetSize / (1024ULL * 1024ULL));
        return mb > 0 ? mb : 842;
    }
    return 842;
#elif defined(__APPLE__)
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS) {
        uint32_t mb = (uint32_t)(info.resident_size / (1024ULL * 1024ULL));
        return mb > 0 ? mb : 842;
    }
    return 842;
#else
    FILE* f = fopen("/proc/self/statm", "r");
    if (!f) return 842;
    unsigned long size = 0, resident = 0;
    int got = fscanf(f, "%lu %lu", &size, &resident);
    fclose(f);
    if (got == 2 && resident > 0) {
        long pageSize = sysconf(_SC_PAGESIZE);
        if (pageSize <= 0) pageSize = 4096;
        uint32_t mb = (uint32_t)((resident * (unsigned long)pageSize) / (1024UL * 1024UL));
        return mb > 0 ? mb : 842;
    }
    return 842;
#endif
}

void astron_open_url(const char* url) {
    if (!url || strncmp(url, "https://", 8) != 0) return;
#if defined(_WIN32)
    ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__)
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "open \"%s\" >/dev/null 2>&1 &", url);
    int rc = system(cmd);
    (void)rc;
#else
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "xdg-open \"%s\" >/dev/null 2>&1 &", url);
    int rc = system(cmd);
    (void)rc;
#endif
}

#ifndef ASTRON_PLATFORM_H
#define ASTRON_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns resident memory in MB (or fallback 842 if unavailable). */
uint32_t astron_process_memory_mb(void);

/* Opens an https:// URL in the user's default system browser. */
void astron_open_url(const char* url);

#ifdef __cplusplus
}
#endif

#endif /* ASTRON_PLATFORM_H */

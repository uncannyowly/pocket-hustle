/* Leaving through OnionOS's GameSwitcher. When a port exits, OnionOS's runtime opens the
 * GameSwitcher if this flag file exists: the same flag its own MENU handling sets for
 * RetroArch. The Mini build sets it when MENU is held, then saves and quits. */
#ifndef PH_SWITCHER_H
#define PH_SWITCHER_H

#define SWITCHER_FLAG_ONION "/mnt/SDCARD/.tmp_update/.runGameSwitcher"

/* Creates the flag file at path. 1 when it exists afterwards; 0 when it could not be made
 * (no such folder, so not OnionOS), path is NULL or empty. */
int switcher_request(const char *path);

#endif

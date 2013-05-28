#include "vice.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"
#include "uiapi.h"

int monitor_is_remote(void)
{
    return 0;
}

ui_jam_action_t monitor_network_ui_jam_dialog(const char *format, ...)
{
    return 0;
}
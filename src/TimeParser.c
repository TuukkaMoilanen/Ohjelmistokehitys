#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "TimeParser.h"

// time format: HHMMSS (6 characters)
int time_parse(char *time) {
    if (time == NULL) return TIME_ARRAY_ERROR;
    if (strlen(time) != 6) return TIME_LEN_ERROR;

    for (int i = 0; i < 6; i++) {
        if (!isdigit((unsigned char)time[i])) return TIME_VALUE_ERROR;
    }

    int hours = (time[0]-'0')*10 + (time[1]-'0');
    int minutes = (time[2]-'0')*10 + (time[3]-'0');
    int seconds = (time[4]-'0')*10 + (time[5]-'0');

    if (hours < 0 || hours > 23) return TIME_VALUE_ERROR;
    if (minutes < 0 || minutes > 59) return TIME_VALUE_ERROR;
    if (seconds < 0 || seconds > 59) return TIME_VALUE_ERROR;

    return minutes * 60 + seconds;
}

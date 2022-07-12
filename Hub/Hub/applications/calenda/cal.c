#include <sys/time.h>
#include <rtthread.h>

#include "Gpio.h"

typedef struct  s_cal
{
    time_t      time;
    struct tm   *tm;
}t_cal;

# define BLACK          "\x1B[30m"
# define GREY           "\x1b[1;30m"
# define RED            "\x1B[31m"
# define LIGHT_RED      "\x1B[1;31m"
# define GREEN          "\x1B[32m"
# define LIGHT_GREEN    "\x1B[1;32m"
# define YELLOW         "\x1B[33m"
# define LIGHT_YELLOW   "\x1B[1;33m"
# define BLUE           "\x1B[34m"
# define LIGHT_BLUE     "\x1B[1;34m"
# define MAGENTA        "\x1B[35m"
# define LIGHT_MAGENTA  "\x1B[1;35m"
# define CYAN           "\x1B[36m"
# define LIGHT_CYAN     "\x1B[1;36m"
# define WHITE          "\x1B[37m"
# define LIGHT_WHITE    "\x1B[1;37m"
# define RESET          "\x1B[0m"

#define CAL_NEWLINE() do{rt_kprintf("%c",'\n');}while(0)

// Normal text color
static const char   normal[] = WHITE;

// Dark text color, used for day tags
static const char   darken[] = LIGHT_RED;

// Active color, for time, and current day indication
static const char   active[] = LIGHT_GREEN;

// Time & date format,
// SEE strftime MANPAGE FOR MORE EXPANSIONS
// http://man7.org/linux/man-pages/man3/strftime.3.html

// Time format, by default 'HH:MM'
static const char   cal_fmt0[] = "It's %H:%M o'clock!";

// Date format, by default 'Day. XX Month Year'
static const char   cal_fmt1[] = "%a. %d %B %Y";

static void cal_init(t_cal *cal, time_t time, struct tm *local_time)
{
    cal->time = time;
    cal->tm = local_time;
}

static void cal_str(t_cal *cal)
{
    char    str0[64];
    char    str1[64];

    rt_memset(str0, '\0', sizeof(str0));
    rt_memset(str1, '\0', sizeof(str1));
    strftime(str0, sizeof(str0), cal_fmt0, cal->tm);
    strftime(str1, sizeof(str1), cal_fmt1, cal->tm);
    CAL_NEWLINE();

    rt_kprintf(normal);
    rt_kprintf("%*s\n", (int)rt_strlen(str0) + 3, str0);
    rt_kprintf("%*s\n", (int)rt_strlen(str1) + 3, str1);
    rt_kprintf(RESET);
}

static const char *const daytag_en[] = {"m", "t", "w", "t", "f", "s", "S", NULL};
static void cal_daytag(t_cal *cal)
{
    int today;
    int i;
    char    **daytags;

    rt_kprintf(normal);
    daytags = (char **)daytag_en;
    CAL_NEWLINE();

    today = cal->tm->tm_wday - 1;
    today < 0 ? today = 6 : (0);
    i = 0;
    while (*daytags != NULL)
    {
        rt_kprintf(darken);
        if (i == today)
            rt_kprintf(active);
        rt_kprintf("%4s", *daytags);
        rt_kprintf(RESET);
        ++daytags;
        ++i;
    }
    rt_kprintf(RESET);
    CAL_NEWLINE();
}

static int cal_daycount(t_cal *cal)
{
    int count;
    int month;
    int year;
    int is_leap;

    month = cal->tm->tm_mon + 1;
    if (month == 4 || month == 6 || month == 9 || month == 11)
        count = 30;
    else if (month == 2)
    {
        year = cal->tm->tm_year;
        is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        count = is_leap ? 29 : 28;
    }
    else
        count = 31;
    return (count);
}

static void cal_calendar(t_cal *cal)
{
    int padding;
    int off;
    int daycount;
    int col;
    int i;
    int today;

    rt_kprintf(normal);
    daycount = cal_daycount(cal);
    padding = cal->tm->tm_wday;
    off = cal->tm->tm_mday;
    col = 0;
    i = 0;
    while (off--)
    {
        --padding;
        if (padding < 0)
            padding = 6;
    }
    while (padding--)
    {
        ++col;
        rt_kprintf("%4c", ' ');
    }
    today = cal->tm->tm_mday - 1;
    if (today < 0)
        today = daycount - 1;
    while (i < daycount)
    {
        rt_kprintf(normal);
        if (i == today)
            rt_kprintf(active);
        rt_kprintf("%4i", i + 1);
        rt_kprintf(RESET);
        ++i;
        ++col;
        if (col == 7)
        {
            col = 0;
            CAL_NEWLINE();
        }
    }
    CAL_NEWLINE();
    CAL_NEWLINE();
}

int cal(void)
{
    t_cal cal;
    time_t t;
    struct tm local_time;

    /* set time */
    time(&t);
    localtime_r(&t, &local_time);

    /* show the calendar */
    cal_init(&cal, t, &local_time);
    cal_str(&cal);
    cal_daytag(&cal);
    cal_calendar(&cal);

    return RT_EOK;
}
//MSH_CMD_EXPORT(cal, calendar);

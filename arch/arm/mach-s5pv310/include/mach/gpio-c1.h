#ifndef __MACH_GPIO_C1_H
#define __MACH_GPIO_C1_H __FILE__

#if defined(CONFIG_MACH_C1_REV02)
#include "gpio-c1-rev02.h"
#elif defined(CONFIG_MACH_C1Q1_REV02)
#include "gpio-c1q1-rev02.h"
#elif defined(CONFIG_MACH_P6_REV02)
#include "gpio-p6-rev02.h"
#elif defined(CONFIG_MACH_TALBOT_REV02)
#include "gpio-talbot-rev02.h"
#elif defined(CONFIG_MACH_C1_REV01)
#include "gpio-c1-rev01.h"
#elif defined(CONFIG_MACH_C1_NA_SPR_REV02)
#include "gpio-c1-na-spr-rev02.h"
#elif defined(CONFIG_MACH_C1_NA_SPR_REV05) || defined(CONFIG_MACH_C1_NA_SPR_EPIC2_REV00)
#include "gpio-c1-na-spr-rev05.h"
#else
#include "gpio-c1-rev00.h"
#endif

#endif /* __MACH_GPIO_C1_H */

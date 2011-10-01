
#define BOOTPARAM_FILEIO
#define PARAM_STRING_SIZE	1024
#define COUNT_PARAM_FILE1   7
#define MAX_PARAM           20
#define MAX_STRING_PARAM    5

#define PARAM_FILE "/mnt/.lfs/param.blk"
#define STOP_LOG		"!@)bAdRfS"
#define STOP_LOG_LEN	9
//#define STOP_LOG		"!@freeze data"
//#define STOP_LOG_LEN	13
#define REBOOT_MODE		0x07

int modify_bootparam();

typedef enum 
{
	__SERIAL_SPEED,
    __LOAD_RAMDISK,
    __BOOT_DELAY,
    __LCD_LEVEL,
    __SWITCH_SEL,
    __PHONE_DEBUG_ON,
    __LCD_DIM_LEVEL,
    __LCD_DIM_TIME,
    __MELODY_MODE,
    __REBOOT_MODE,
   	__NATION_SEL,
    __LANGUAGE_SEL,
    __SET_DEFAULT_PARAM,
    __PARAM_INT_13, /* Reserved. */
    __PARAM_INT_14, /* Reserved. */
    __VERSION,
    __CMDLINE,
    __DELTA_LOCATION,
    __PARAM_STR_3,  /* Reserved. */
   	__PARAM_STR_4   /* Reserved. */
} param_idx;

typedef struct _param_int_t 
{
	param_idx ident;
    s32  value;
} param_int_t;

typedef struct _param_str_t 
{
	param_idx ident;
    s8 value[PARAM_STRING_SIZE];
} param_str_t;

typedef struct _param_union_t 
{
	param_int_t param_int_file1[COUNT_PARAM_FILE1];
    param_int_t param_int_file2[MAX_PARAM - MAX_STRING_PARAM - COUNT_PARAM_FILE1];
} param_union_t;


typedef struct 
{
	int param_magic;
    int param_version;

    union _param_int_list{
    param_int_t param_list[MAX_PARAM - MAX_STRING_PARAM];
    param_union_t param_union;
   	} param_int_list;

    param_str_t param_str_list[MAX_STRING_PARAM];
} status_t;


#define KEY_ERASER	KEY_LEFTALT

extern int wacom_i2c_master_send(struct i2c_client *client,const char *buf ,int count, unsigned short addr);
extern int wacom_i2c_master_recv(struct i2c_client *client,char *buf ,int count, unsigned short addr);
extern int wacom_i2c_test(struct wacom_i2c *wac_i2c);
extern int wacom_i2c_coord(struct wacom_i2c *wac_i2c);
extern int wacom_i2c_query(struct wacom_i2c *wac_i2c);
extern void check_emr_device(bool bOn);
//int wacom_i2c_frequency(struct wacom_i2c *wac_i2c, char buf);
#ifndef _AUDIENCE_H
#define _AUDIENCE_H

extern int audience_probe(void);
extern int audience_bypass(u8 *p);
extern int audience_closetalk(u8 *p);
extern int audience_fartalk(u8 *p);
extern int audience_NS0(u8 *p);
extern int audience_state(void);
extern int factory_sub_mic_status(void);

#define AUDIENCE_OFF 0
#define AUDIENCE_ON 1
#define AUDIENCE_STATE 3

#define FACTORY_SUB_MIC_OFF 10
#define FACTORY_SUB_MIC_ON 11

#endif

#ifdef E_TYPEDEFS

typedef struct _E_Event_Desklock E_Event_Desklock;

#else
#ifndef E_DESKLOCK_H
#define E_DESKLOCK_H

struct _E_Event_Desklock
{
   int on;
};

EINTERN int e_desklock_init(void);
EINTERN int e_desklock_shutdown(void);

EAPI int e_desklock_show(void);
EAPI int e_desklock_show_autolocked(void);
EAPI void e_desklock_hide(void);

extern EAPI int E_EVENT_DESKLOCK;

#endif
#endif

#ifdef E_TYPEDEFS
#else
#ifndef E_SCREENSAVER_H
#define E_SCREENSAVER_H

#ifndef _F_DISABLE_E_SCREENSAVER

EINTERN int e_screensaver_init(void);
EINTERN int e_screensaver_shutdown(void);

EAPI void e_screensaver_update(void);
EAPI void e_screensaver_force_update(void);

#endif
#endif
#endif

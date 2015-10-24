#ifndef _E_TZSH_PRIVATE_H_
#define _E_TZSH_PRIVATE_H_

#include "e.h"

#include <wayland-server.h>
#include "tzsh_protocol.h"

#ifdef ERR
#undef ERR
#endif
#ifdef INF
#undef INF
#endif
#ifdef DBG
#undef DBG
#endif

#define DEBUG_MODE 0


# define ERR(f, x...)               ELBF(ELBT_TZSH, 0, 0, "ERR: " f, ##x);
# define INF(f, x...)               ELBF(ELBT_TZSH, 0, 0, "INF: " f, ##x);
# define DBG(f, x...)               ELBF(ELBT_TZSH, 0, 0, "DBG: " f, ##x);

#ifdef DEBUG_MODE
# define E_TZSH_CHECK_RET(x)        NULL
# define E_TZSH_CHECK_RET_VAL(x, r) NULL
#else
# define E_TZSH_CHECK_RET(x)        EINA_SAFETY_ON_NULL_RETURN(x);
# define E_TZSH_CHECK_RET_VAL(x, r) EINA_SAFETY_ON_NULL_RETURN_VAL(x, r);
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({                  \
   const __typeof__( ((type *)0)->member ) *__mptr = (ptr); \
   (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

typedef struct _E_Tzsh_Mgr E_Tzsh_Mgr;
typedef struct _E_Tzsh_Client E_Tzsh_Client;

#include "e_tzsh_event.h"
#include "e_tzsh_region.h"
#include "e_tzsh_service.h"
#include "e_tzsh_client.h"
#include "e_tzsh_user.h"

#include "quickpanel.h"

void            e_tzsh_mgr_service_register(E_Tzsh_Service *service);
void            e_tzsh_mgr_service_remove(E_Tzsh_Service *service);
void            e_tzsh_mgr_client_remove(E_Tzsh_Client *client);
E_Tzsh_Service *e_tzsh_mgr_service_get(E_Tzsh_Service_Role role);

#endif

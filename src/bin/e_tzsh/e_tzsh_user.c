#include "e_tzsh_private.h"

static void
_e_tzsh_user_simple_event_free(void *ev_data)
{
   free(ev_data);
}

Eina_Bool
e_tzsh_user_event_send(Ecore_X_Window win, E_Tzsh_Service_Role role, E_Tzsh_Message msg)
{
   E_Event_Tzsh_Client_Message *ev;

   ev = E_NEW(E_Event_Tzsh_Client_Message, 1);
   E_TZSH_CHECK_RET_VAL(ev, EINA_FALSE);

   ev->role = role;
   ev->msg = msg;

   if (!e_tzsh_event_send(win, E_EVENT_TZSH_CLIENT_MESSAGE, ev,
                          _e_tzsh_user_simple_event_free))
     {
        free(ev);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

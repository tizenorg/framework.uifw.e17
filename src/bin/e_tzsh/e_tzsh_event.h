#ifndef _E_TZSH_EVENT_H_
#define _E_TZSH_EVENT_H_

typedef void (*E_Tzsh_Event_End_Cb)(void *ev_data);

int          e_tzsh_event_init(void);
void         e_tzsh_event_shutdown(void);
Eina_Bool    e_tzsh_event_send(Ecore_X_Window win, int ev_type, void *ev_data, E_Tzsh_Event_End_Cb fn_free);
Eina_Bool    e_tzsh_event_destroy_send(Ecore_X_Window win, int ev_type, void *ev_data, E_Tzsh_Event_End_Cb fn_free);


#endif

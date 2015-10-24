#include "e_tzsh_private.h"

typedef struct _E_Tzsh_Event_Pending_Data E_Tzsh_Event_Pending_Data;

struct _E_Tzsh_Event_Pending_Data
{
   Ecore_X_Window win;
   int ev_type;
   void *ev_data;
   E_Tzsh_Event_End_Cb fn_free;
   Ecore_Timer *timeout;
   Eina_Bool managed;
};

EAPI int E_EVENT_TZSH_SERVICE_CREATE;
EAPI int E_EVENT_TZSH_SERVICE_DESTROY;
EAPI int E_EVENT_TZSH_SERVICE_REGION_SET;
EAPI int E_EVENT_TZSH_SERVICE_MESSAGE;
EAPI int E_EVENT_TZSH_CLIENT_MESSAGE;

static Eina_List *_event_list = NULL;  // store the E_Tzsh_Event_Pending_Data in order.
static E_Border_Hook *_e_tzsh_bd_new_hook = NULL;

static void
_e_tzsh_event_base_free(void *data, void *ev)
{
   E_Event_Tzsh_Service_Base *e = ev;
   E_Tzsh_Event_End_Cb fn_free = data;

   E_TZSH_CHECK_RET(e);

   e_object_unref(E_OBJECT(e->bd));

   if (fn_free)
     fn_free(e);
}

static Eina_Bool
_e_tzsh_border_event_send(E_Border *bd, int ev_type, void *ev_data, E_Tzsh_Event_End_Cb fn_free)
{
   E_Event_Tzsh_Simple *ev = ev_data;

   E_TZSH_CHECK_RET_VAL(bd, EINA_FALSE);
   E_TZSH_CHECK_RET_VAL(ev, EINA_FALSE);

   ev->bd = bd;
   e_object_ref(E_OBJECT(bd));
   if (!ecore_event_add(ev_type, ev, _e_tzsh_event_base_free, fn_free))
     {
        e_object_unref(E_OBJECT(bd));
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static void
_e_tzsh_event_pending_data_destroy(E_Tzsh_Event_Pending_Data *pd, Eina_Bool data_free)
{
   E_TZSH_CHECK_RET(pd);

   DBG("Destroy Pended event, win:0x%08x", pd->win);

   if (data_free)
     {
        if (pd->fn_free)
          pd->fn_free(pd->ev_data);
     }

   E_FN_DEL(ecore_timer_del, pd->timeout);
   free(pd);
}

static void
_e_tzsh_hook_new_border(void *data __UNUSED__, void *data2)
{
   E_Border *bd;
   Eina_List *l, *ll;
   E_Tzsh_Event_Pending_Data *pd;
   Eina_Bool pass = EINA_FALSE, res = EINA_FALSE;

   if (!(bd = data2)) return;

   EINA_LIST_FOREACH_SAFE(_event_list, l, ll, pd)
     {
        if (!pass)
          {
             if ((pd->managed) ||
                 (pd->win == bd->client.win))
               {
                  DBG("Added Border, Send pended event, win:0x%08x", pd->win);

                  res = _e_tzsh_border_event_send(bd, pd->ev_type,
                                                  pd->ev_data, pd->fn_free);

                  _event_list = eina_list_remove(_event_list, pd);
                  _e_tzsh_event_pending_data_destroy(pd, !res);
               }
             else
               pass = EINA_TRUE;

             continue;
          }

        if (pd->win == bd->client.win)
          pd->managed = EINA_TRUE;
     }

   if (!eina_list_count(_event_list))
     {
        DBG("Delete E_BORDER_HOOK_NEW_BORDER hook");
        if (_e_tzsh_bd_new_hook)
          {
             e_border_hook_del(_e_tzsh_bd_new_hook);
             _e_tzsh_bd_new_hook = NULL;
          }
     }
}


// NOTE: if there is so many pending events, it maybe causes overload.
// I'm not sure whether it's necessary for now.
// As defensive style, just leave it.
static Eina_Bool
_e_tzsh_event_pending_data_cb_timeout(void *data)
{
   E_Tzsh_Event_Pending_Data *pd = data;

   E_TZSH_CHECK_RET_VAL(pd, ECORE_CALLBACK_CANCEL);

   DBG("**** TIMEOUT **** for pending event, win:0x%08x", pd->win);
   DBG("Border we've wait is not added until now");
   DBG("We can't wait anymore");

   _event_list = eina_list_remove(_event_list, pd);

   _e_tzsh_event_pending_data_destroy(pd, EINA_TRUE);

   return ECORE_CALLBACK_CANCEL;
}

static E_Tzsh_Event_Pending_Data *
_e_tzsh_event_pending_data_create(Ecore_X_Window win, int ev_type, void *ev_data, E_Tzsh_Event_End_Cb fn_free, double timeout)
{
   E_Tzsh_Event_Pending_Data *pd;

   DBG("Create Pending event, win: 0x%08x", win);

   pd = E_NEW(E_Tzsh_Event_Pending_Data, 1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd, NULL);

   pd->win = win;
   pd->ev_type = ev_type;
   pd->ev_data = ev_data;
   pd->fn_free = fn_free;
   pd->managed = EINA_FALSE;
   pd->timeout = ecore_timer_add(timeout, _e_tzsh_event_pending_data_cb_timeout, pd);

   return pd;
}

static Eina_Bool
_e_tzsh_event_pending_data_add(Ecore_X_Window win, int ev_type, void *ev_data, E_Tzsh_Event_End_Cb fn_free)
{
   E_Tzsh_Event_Pending_Data *pd;
   double timeout = 1.0; // this is arbitrary

   if (win < 1)
     return EINA_FALSE;

   DBG("Add pending event (Start Timeout: %fs from now), win:0x%08x",
       timeout, win);

   pd = _e_tzsh_event_pending_data_create(win, ev_type, ev_data,
                                          fn_free, timeout);

   _event_list = eina_list_append(_event_list, pd);

   if (!_e_tzsh_bd_new_hook)
     {
        _e_tzsh_bd_new_hook =
                      e_border_hook_add(E_BORDER_HOOK_NEW_BORDER,
                                        _e_tzsh_hook_new_border, NULL);
     }

   return EINA_TRUE;
}

Eina_Bool
e_tzsh_event_send(Ecore_X_Window win, int ev_type, void *ev_data, E_Tzsh_Event_End_Cb fn_free)
{
   E_Border *bd;
   E_Event_Tzsh_Simple *ev = ev_data;

   EINA_SAFETY_ON_FALSE_RETURN_VAL((win > 1), EINA_FALSE);

   bd = e_border_find_by_client_window(win);
   if (bd)
     return _e_tzsh_border_event_send(bd, ev_type, ev_data, fn_free);
   else
     return _e_tzsh_event_pending_data_add(win, ev_type, ev, fn_free);
}

Eina_Bool
e_tzsh_event_destroy_send(Ecore_X_Window win, int ev_type, void *ev_data, E_Tzsh_Event_End_Cb fn_free)
{
   E_Border *bd;
   Eina_List *l, *ll;
   E_Tzsh_Event_Pending_Data *pd;

   EINA_SAFETY_ON_FALSE_RETURN_VAL((win > 1), EINA_FALSE);

   bd = e_border_find_by_client_window(win);
   if (bd)
     return _e_tzsh_border_event_send(bd, ev_type, ev_data, fn_free);
   else
     {
        // remove all pending event data regarding this window.
        EINA_LIST_FOREACH_SAFE(_event_list, l, ll, pd)
          {
             if (pd->win != win) continue;

             _event_list = eina_list_remove(_event_list, pd);

             _e_tzsh_event_pending_data_destroy(pd, EINA_TRUE);
          }

        return EINA_FALSE;
     }
}

int
e_tzsh_event_init(void)
{
   E_EVENT_TZSH_SERVICE_CREATE = ecore_event_type_new();
   E_EVENT_TZSH_SERVICE_DESTROY = ecore_event_type_new();
   E_EVENT_TZSH_SERVICE_REGION_SET = ecore_event_type_new();
   E_EVENT_TZSH_SERVICE_MESSAGE = ecore_event_type_new();
   E_EVENT_TZSH_CLIENT_MESSAGE = ecore_event_type_new();

   return 1;
}

void
e_tzsh_event_shutdown(void)
{
   E_Tzsh_Event_Pending_Data *pd;

   if (_event_list)
     {
        EINA_LIST_FREE(_event_list, pd)
           _e_tzsh_event_pending_data_destroy(pd, EINA_TRUE);
     }

   DBG("Delete E_BORDER_HOOK_NEW_BORDER hook");
   if (_e_tzsh_bd_new_hook)
     {
        e_border_hook_del(_e_tzsh_bd_new_hook);
        _e_tzsh_bd_new_hook = NULL;
     }
}


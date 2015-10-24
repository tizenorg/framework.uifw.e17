#include "e_tzsh_private.h"

struct _E_Tzsh_Service
{
   E_Tzsh_Client *tzsh_client;
   E_Tzsh_Service_Role role;
   Ecore_X_Window win;
   struct wl_resource *resource;
   char *name;
};

static void
_e_tzsh_service_simple_event_free(void *ev_data)
{
   free(ev_data);
}

static void
_e_tzsh_service_region_set_event_free(void *ev_data)
{
   E_Event_Tzsh_Service_Region_Set *e = ev_data;

   E_TZSH_CHECK_RET(e);

   if (e->tiler)
     eina_tiler_free(e->tiler);

   free(e);
}

Eina_Bool
e_tzsh_service_create_notify(E_Tzsh_Service *service)
{
   E_Event_Tzsh_Service_Create *ev;

   E_TZSH_CHECK_RET_VAL(service, EINA_FALSE);

   if ((service->role <= E_TZSH_SERVICE_ROLE_UNKNOWN) ||
       (service->role >= E_TZSH_SERVICE_ROLE_RESERVE_FOR_LIMIT))
     return EINA_FALSE;

   ev = E_NEW(E_Event_Tzsh_Service_Create, 1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(ev, EINA_FALSE);

   ev->role = service->role;

   if (!e_tzsh_event_send(service->win, E_EVENT_TZSH_SERVICE_CREATE, ev,
                          _e_tzsh_service_simple_event_free))
     {
        free(ev);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static void
_e_tzsh_service_destroy_notify(E_Tzsh_Service *service)
{
   E_Event_Tzsh_Service_Destroy *ev;
   Eina_Bool res;

   ev = E_NEW(E_Event_Tzsh_Service_Destroy, 1);
   EINA_SAFETY_ON_NULL_RETURN(ev);

   ev->role = service->role;

   res = e_tzsh_event_destroy_send(service->win, E_EVENT_TZSH_SERVICE_DESTROY, ev,
                                  _e_tzsh_service_simple_event_free);
   if (!res)
     free(ev);
}

static Eina_Bool
_e_tzsh_service_region_set_notify(E_Tzsh_Service *service, int angle, Eina_Tiler *tiler, int type)
{
   E_Event_Tzsh_Service_Region_Set *ev;
   int w, h;

   E_TZSH_CHECK_RET_VAL(service, EINA_FALSE);
   E_TZSH_CHECK_RET_VAL(tiler, EINA_FALSE);

   DBG("Service Window Region Set: win 0x%08x, role: %d angle: %d type: %d",
       service->win, service->role, angle, type);

   ev = E_NEW(E_Event_Tzsh_Service_Region_Set, 1);
   if (!ev)
     return EINA_FALSE;

   eina_tiler_area_size_get(tiler, &w, &h);
   ev->tiler = eina_tiler_new(w, h);
   eina_tiler_tile_size_set(ev->tiler, 1, 1);
   eina_tiler_union(ev->tiler, tiler);

   ev->angle = angle;
   ev->region_type = type;

   if (!e_tzsh_event_send(service->win, E_EVENT_TZSH_SERVICE_REGION_SET,
                          ev, _e_tzsh_service_region_set_event_free))
     {
        eina_tiler_free(ev->tiler);
        free(ev);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static void
_e_tzsh_service_cb_destroy(struct wl_client *client EINA_UNUSED,
                           struct wl_resource *resource)
{
   E_TZSH_CHECK_RET(resource);

   wl_resource_destroy(resource);
}

static void
_e_tzsh_service_cb_region_set(struct wl_client *client EINA_UNUSED,
                              struct wl_resource *resource,
                              int32_t type,
                              int32_t angle,
                              struct wl_resource *region)
{
   E_Tzsh_Service *service;
   E_Tzsh_Region *region_h;
   Eina_Tiler *tiler;

   if (!resource)
     goto err_invalid;

   service = wl_resource_get_user_data(resource);
   if (!service)
     goto err_invalid;

   region_h = wl_resource_get_user_data(region);
   if (!region_h)
     goto err_invalid;

   tiler = e_tzsh_region_tiler_get(region_h);
   if (!tiler)
     goto err_invalid;

   if (!_e_tzsh_service_region_set_notify(service, angle, tiler, type))
     {
        ERR("Failed to notify region set");
        wl_resource_post_no_memory(resource);
     }

   return;
err_invalid:
   ERR("Invalid object");
//   wl_resource_post_error("");
}

static struct tws_service_interface _e_tzsh_service_interface =
{
   _e_tzsh_service_cb_destroy,
   _e_tzsh_service_cb_region_set,
   NULL, // indicator_get
   NULL  // quickpanel_get
};

static void
_e_tzsh_service_resource_destroy(struct wl_resource *resource)
{
   E_Tzsh_Service *service;

   E_TZSH_CHECK_RET(resource);

   service = wl_resource_get_user_data(resource);
   E_TZSH_CHECK_RET(service);

   _e_tzsh_service_destroy_notify(service);

   e_tzsh_client_service_remove(service->tzsh_client, service);
   e_tzsh_mgr_service_remove(service);

   free(service->name);
   free(service);
}

E_Tzsh_Service *
e_tzsh_service_create(struct wl_client *client,
                      uint32_t id,
                      E_Tzsh_Client *tzsh_client,
                      Ecore_X_Window win,
                      char *name, int role)
{
   E_Tzsh_Service *service;
   struct wl_resource *resource;
   char *service_name;
   int len;

   DBG("Create '%s' service: win 0x%08x", name, win);

   resource = wl_resource_create(client, &tws_service_interface, 1, id);
   E_TZSH_CHECK_RET_VAL(service, NULL);

   service = E_NEW(E_Tzsh_Service, 1);
   if (!service)
     {
        wl_resource_destroy(resource);
        return NULL;
     }

   service_name = strdup(name);
   if (!service_name)
     {
        wl_resource_destroy(resource);
        free(service);
        return NULL;
     }

   service->tzsh_client = tzsh_client;
   service->role = role;
   service->win = win;
   service->resource = resource;
   service->name = service_name;

   wl_resource_set_implementation(resource, &_e_tzsh_service_interface,
                                  service, _e_tzsh_service_resource_destroy);

   return service;
}

void
e_tzsh_service_destroy(E_Tzsh_Service *service)
{
   E_TZSH_CHECK_RET(service);

   DBG("Destroy '%s' service: win 0x%08x", service->name, service->win);

   E_FN_DEL(wl_resource_destroy, service->resource);
}

Ecore_X_Window
e_tzsh_service_window_get(E_Tzsh_Service *service)
{
   E_TZSH_CHECK_RET_VAL(service, 0);

   return service->win;
}

char *
e_tzsh_service_name_get(E_Tzsh_Service *service)
{
   E_TZSH_CHECK_RET_VAL(service, NULL);

   return service->name;
}

E_Tzsh_Service_Role
e_tzsh_service_role_get_by_name(char *name)
{
   E_Tzsh_Service_Role role = E_TZSH_SERVICE_ROLE_UNKNOWN;

   if (!strcmp(name, "call"))
     role = E_TZSH_SERVICE_ROLE_CALL;
   else if (!strcmp(name, "volume"))
     role = E_TZSH_SERVICE_ROLE_VOLUME;
   else if (!strcmp(name, "quickpanel"))
     role = E_TZSH_SERVICE_ROLE_QUICKPANEL;
   else if (!strcmp(name, "lockscreen"))
     role = E_TZSH_SERVICE_ROLE_LOCKSCREEN;
   else if (!strcmp(name, "indicator"))
     role = E_TZSH_SERVICE_ROLE_INDICATOR;

   return role;
}

E_Tzsh_Service_Role
e_tzsh_service_role_get(E_Tzsh_Service *service)
{
   E_TZSH_CHECK_RET(service);

   return e_tzsh_service_role_get_by_name(service->name);
}

void
e_tzsh_service_indicator_get_cb_set(E_Tzsh_Service_Cb func)
{
   _e_tzsh_service_interface.indicator_get = func;
}

void
e_tzsh_service_quickpanel_get_cb_set(E_Tzsh_Service_Cb func)
{
   _e_tzsh_service_interface.quickpanel_get = func;
}

Eina_Bool
e_tzsh_service_message_send(E_Tzsh_Service *service, E_Tzsh_Message msg)
{
   E_Event_Tzsh_Service_Message *ev;

   ev = E_NEW(E_Event_Tzsh_Service_Message, 1);
   E_TZSH_CHECK_RET_VAL(ev, EINA_FALSE);

   ev->role = service->role;
   ev->msg = msg;

   if (!e_tzsh_event_send(service->win, E_EVENT_TZSH_SERVICE_MESSAGE, ev,
                          _e_tzsh_service_simple_event_free))
     {
        free(ev);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

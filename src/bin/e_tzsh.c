#include "e_tzsh_private.h"

struct _E_Tzsh_Mgr
{
   struct wl_display *disp;
   struct wl_event_loop *loop;
   Ecore_Fd_Handler *fd_hdlr;
   Eina_Hash *service_windows;
   Eina_List *client_list;
   E_Tzsh_Service *registered_service[E_TZSH_SERVICE_ROLE_RESERVE_FOR_LIMIT];
};

static E_Tzsh_Mgr *_tzsh = NULL;

static inline E_Tzsh_Mgr *
_e_tzsh_mgr_get(void)
{
   return _tzsh;
}

static inline void
_e_tzsh_mgr_set(E_Tzsh_Mgr *tzsh)
{
   _tzsh = tzsh;
}

static void
_e_tzsh_mgr_all_registered_service_send(E_Tzsh_Client *tzsh_client)
{
   E_Tzsh_Mgr *tzsh = _e_tzsh_mgr_get();
   E_Tzsh_Service *service;
   char *name;
   int i;

   E_TZSH_CHECK_RET(tzsh);
   E_TZSH_CHECK_RET(tzsh_client);

   // Send lists of registered service name to the client.
   for (i = E_TZSH_SERVICE_ROLE_UNKNOWN + 1;
        i < E_TZSH_SERVICE_ROLE_RESERVE_FOR_LIMIT; i++)
     {
        service = tzsh->registered_service[i];
        if (!service) continue;
        if (!(name = e_tzsh_service_name_get(service))) continue;

        e_tzsh_client_service_register_notify(tzsh_client, name);
     }
}

static void
_e_tzsh_mgr_cb_tws_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
   E_Tzsh_Mgr *tzsh = data;
   E_Tzsh_Client *tzsh_client;

   E_TZSH_CHECK_RET(tzsh);

   tzsh_client = e_tzsh_client_create(client, version, id);
   if (!tzsh_client)
     {
        ERR("failed to create client");
        wl_client_post_no_memory(client);
        return;
     }

   tzsh->client_list = eina_list_append(tzsh->client_list, tzsh_client);

   _e_tzsh_mgr_all_registered_service_send(tzsh_client);
}

static Eina_Bool
_e_tzsh_mgr_cb_read(void *data, Ecore_Fd_Handler *hdlr EINA_UNUSED)
{
   E_Tzsh_Mgr *tzsh = data;

   E_TZSH_CHECK_RET_VAL(tzsh, ECORE_CALLBACK_RENEW);

   wl_event_loop_dispatch(tzsh->loop, 0);

   return ECORE_CALLBACK_RENEW;
}

static void
_e_tzsh_mgr_cb_prepare(void *data, Ecore_Fd_Handler *hdlr EINA_UNUSED)
{
   E_Tzsh_Mgr *tzsh = data;

   E_TZSH_CHECK_RET(tzsh);

   wl_display_flush_clients(tzsh->disp);
}

static void
_e_tzsh_mgr_free(E_Tzsh_Mgr *tzsh)
{
   E_Tzsh_Client *tzsh_client;

   E_TZSH_CHECK_RET(tzsh);

   if (tzsh->client_list)
     {
        EINA_LIST_FREE(tzsh->client_list, tzsh_client)
           e_tzsh_client_destroy(tzsh_client);
     }

   E_FN_DEL(eina_hash_free, tzsh->service_windows);
   E_FN_DEL(ecore_main_fd_handler_del, tzsh->fd_hdlr);
   E_FN_DEL(wl_display_destroy, tzsh->disp);

   free(tzsh);
}

static E_Tzsh_Mgr *
_e_tzsh_mgr_new(void)
{
   E_Tzsh_Mgr *tzsh = NULL;
   int fd = -1, i;
   const char *socket_name;

   tzsh = E_NEW(E_Tzsh_Mgr, 1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(tzsh, NULL);

   for (i = E_TZSH_SERVICE_ROLE_UNKNOWN;
        i < E_TZSH_SERVICE_ROLE_RESERVE_FOR_LIMIT; i++)
     tzsh->registered_service[i] = NULL;

   _e_tzsh_mgr_set(tzsh);

   tzsh->disp = wl_display_create();
   if (!tzsh->disp)
     {
        ERR("Failed to create wl_display");
        goto err;
     }

   tzsh->loop = wl_display_get_event_loop(tzsh->disp);

   socket_name = wl_display_add_socket_auto(tzsh->disp);
   if (!socket_name)
     {
        ERR("Failed to add socket: %m");
        goto err;
     }

   setenv("TZSH_SOCKET", socket_name, 1);

   if (!wl_global_create(tzsh->disp, &tizen_ws_shell_interface,
                         1, tzsh, _e_tzsh_mgr_cb_tws_bind))
     {
        ERR("Failed to create global for tizen_ws_shell_interface");
        goto err;
     }

   fd = wl_event_loop_get_fd(tzsh->loop);

   tzsh->fd_hdlr =
      ecore_main_fd_handler_add(fd, (ECORE_FD_READ | ECORE_FD_ERROR),
                                _e_tzsh_mgr_cb_read, tzsh, NULL, NULL);
   ecore_main_fd_handler_prepare_callback_set(tzsh->fd_hdlr,
                                              _e_tzsh_mgr_cb_prepare, tzsh);

   tzsh->service_windows =
      eina_hash_string_superfast_new(NULL);

   return tzsh;
err:
   _e_tzsh_mgr_free(tzsh);

   return NULL;
}

void
e_tzsh_mgr_client_remove(E_Tzsh_Client *client)
{
   E_Tzsh_Mgr *tzsh = _e_tzsh_mgr_get();

   DBG("removed client from manager: client %p", client);

   tzsh->client_list = eina_list_remove(tzsh->client_list, client);
}

void
e_tzsh_mgr_service_register(E_Tzsh_Service *service)
{
   E_Tzsh_Mgr *tzsh = _e_tzsh_mgr_get();
   E_Tzsh_Client *tzsh_client;
   E_Tzsh_Service_Role role;
   Ecore_X_Window win;
   Eina_List *l;
   char *name;

   E_TZSH_CHECK_RET(tzsh);
   E_TZSH_CHECK_RET(service);

   win = e_tzsh_service_window_get(service);
   name = e_tzsh_service_name_get(service);
   role = e_tzsh_service_role_get(service);

   DBG("'%s' service registered", name);

   if (tzsh->registered_service[role])
     {
        ERR("The %s service is already existed.", name);
        return;
     }

   tzsh->registered_service[role] = service;

   eina_hash_add(tzsh->service_windows, e_util_winid_str_get(win), service);

   EINA_LIST_FOREACH(tzsh->client_list, l, tzsh_client)
      e_tzsh_client_service_register_notify(tzsh_client, name);
}

void
e_tzsh_mgr_service_remove(E_Tzsh_Service *service)
{
   E_Tzsh_Mgr *tzsh = _e_tzsh_mgr_get();
   E_Tzsh_Client *tzsh_client;
   E_Tzsh_Service_Role role;
   Eina_List *l;
   char *name;

   E_TZSH_CHECK_RET(tzsh);
   E_TZSH_CHECK_RET(service);

   role = e_tzsh_service_role_get(service);
   name = e_tzsh_service_name_get(service);
   if (!name)
     return;

   DBG("'%s' service removed", name);

   EINA_LIST_FOREACH(tzsh->client_list, l, tzsh_client)
      e_tzsh_client_service_remove_notify(tzsh_client, name);

   eina_hash_del_by_data(tzsh->service_windows, service);

   tzsh->registered_service[role] = NULL;
}

E_Tzsh_Service *
e_tzsh_mgr_service_get(E_Tzsh_Service_Role role)
{
   E_Tzsh_Mgr *tzsh = _e_tzsh_mgr_get();

   E_TZSH_CHECK_RET_VAL(tzsh, EINA_FALSE);

   return tzsh->registered_service[role];
}

static void
_e_tzsh_shutdown_internal(void)
{
   E_Tzsh_Mgr *tzsh = _e_tzsh_mgr_get();

   E_TZSH_CHECK_RET(tzsh);

   e_tzsh_event_shutdown();
   _e_tzsh_mgr_free(tzsh);
   _e_tzsh_mgr_set(NULL);
}

EINTERN int
e_tzsh_init(void)
{
   E_Tzsh_Mgr *tzsh;

   tzsh = _e_tzsh_mgr_get();
   if (tzsh)
     return EINA_TRUE;

   tzsh = _e_tzsh_mgr_new();
   if (!tzsh)
     return 0;

   if (!e_tzsh_event_init())
     {
        _e_tzsh_mgr_free(tzsh);
        return 0;
     }

   _e_tzsh_mgr_set(tzsh);

   // NOTE: additional service should be added here.
   e_tzsh_quickpanel_init();

   return 1;
}

EINTERN int
e_tzsh_shutdown(void)
{
   _e_tzsh_shutdown_internal();

   return 1;
}

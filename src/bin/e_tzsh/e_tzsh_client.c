#include "e_tzsh_private.h"

struct _E_Tzsh_Client
{
   struct wl_resource *resource;
   Eina_List *service_list;
};

static void
_e_tzsh_client_destroy_internal(E_Tzsh_Client *client)
{
   E_Tzsh_Service *service;

   DBG("destroy client");

   EINA_LIST_FREE(client->service_list, service)
      E_FN_DEL(e_tzsh_service_destroy, service);

   e_tzsh_mgr_client_remove(client);

   free(client);
}

static void
_e_tzsh_client_cb_destroy(struct wl_client *client EINA_UNUSED, struct wl_resource *resource)
{
   DBG("destroy client resource: %p", resource);

   E_TZSH_CHECK_RET(resource);

   wl_resource_destroy(resource);
}

static void
_e_tzsh_client_cb_service_create(struct wl_client *client,
                                 struct wl_resource *resource,
                                 uint32_t id, uint32_t win, const char *name)
{
   E_Tzsh_Client *tzsh_client;
   E_Tzsh_Service *service;
   E_Tzsh_Service_Role role;

   E_TZSH_CHECK_RET(client);
   E_TZSH_CHECK_RET(resource);

   DBG("wl callback: service create: %s", name);

   tzsh_client = wl_resource_get_user_data(resource);
   if (!tzsh_client)
     {
        wl_resource_post_error(resource,
                               WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "No Tzsh Client for service");
        return;
     }

   role = e_tzsh_service_role_get_by_name(name);
   if (role == E_TZSH_SERVICE_ROLE_UNKNOWN)
     {
        ERR("the %s service is not supported.", name);
        //wl_resource_post_error("");
        return;
     }

   if (e_tzsh_mgr_service_get(role))
     {
        ERR("the %s service is already existed.", name);
        //wl_resource_post_err("");
        return;
     }

   service = e_tzsh_service_create(client, id, tzsh_client, win, name, role);
   if (!service)
     {
        ERR("Failed to create e_tzsh_service: %s", name);
        wl_client_post_no_memory(client);
        return;
     }

   if (!e_tzsh_service_create_notify(service))
     {
        ERR("Failed to send to event of create service: %s", name);
        e_tzsh_service_destroy(service);
        wl_client_post_no_memory(client);
        return;
     }

   e_tzsh_mgr_service_register(service);

   tzsh_client->service_list = eina_list_append(tzsh_client->service_list, service);
}

static void
_e_tzsh_client_resource_destroy(struct wl_resource *resource)
{
   E_Tzsh_Client *client;

   E_TZSH_CHECK_RET(resource);

   DBG("client resource destroy: %p", resource);

   client = wl_resource_get_user_data(resource);

   _e_tzsh_client_destroy_internal(client);
}

static struct tizen_ws_shell_interface _e_tzsh_client_interface =
{
   _e_tzsh_client_cb_destroy,
   _e_tzsh_client_cb_service_create,
   e_tzsh_region_tws_region_create,
   NULL // quickpanel_get
};

E_Tzsh_Client *
e_tzsh_client_create(struct wl_client *client, int32_t version, int32_t id)
{
   E_Tzsh_Client *client_h;

   DBG("create client: %d", id);

   client_h = E_NEW(E_Tzsh_Client, 1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(client_h, NULL);

   client_h->resource =
      wl_resource_create(client, &tizen_ws_shell_interface, version, id);
   if (!client_h->resource)
     {
        ERR("Out of memory for tizen_ws_shell_interface");
        free(client_h);
        return NULL;
     }

   wl_resource_set_implementation(client_h->resource, &_e_tzsh_client_interface,
                                  client_h, _e_tzsh_client_resource_destroy);

   return client_h;
}

void
e_tzsh_client_destroy(E_Tzsh_Client *client)
{
   E_TZSH_CHECK_RET(client);

   DBG("destroy client");

   wl_resource_destroy(client->resource);
}

void
e_tzsh_client_service_remove(E_Tzsh_Client *client, E_Tzsh_Service *service)
{
   E_TZSH_CHECK_RET(client);

   DBG("service remove from client's service list");

   client->service_list = eina_list_remove(client->service_list, service);
}

void
e_tzsh_client_service_register_notify(E_Tzsh_Client *client, char *name)
{
   DBG("notify service registered: %s", name);

   tizen_ws_shell_send_service_register(client->resource, name);
}

void
e_tzsh_client_service_remove_notify(E_Tzsh_Client *client, char *name)
{
   DBG("notify service removed: %s", name);

   tizen_ws_shell_send_service_unregister(client->resource, name);
}

void
e_tzsh_client_quickpanel_get_cb_set(E_Tzsh_Client_Cb func)
{
   _e_tzsh_client_interface.quickpanel_get = func;
}

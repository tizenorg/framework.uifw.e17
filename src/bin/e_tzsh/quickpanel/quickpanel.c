#include "e_tzsh_private.h"

typedef struct _E_Tzsh_Quickpanel
{
   Ecore_X_Window win;
   struct wl_resource *resource;
} E_Tzsh_Quickpanel;

static void
_e_tzsh_quickpanel_cb_release(struct wl_client *client EINA_UNUSED, struct wl_resource *resource)
{
   E_TZSH_CHECK_RET(resource);
   DBG("User interface Release");
   wl_resource_destroy(resource);
}

static void
_e_tzsh_quickpanel_cb_open(struct wl_client *client EINA_UNUSED, struct wl_resource *resource)
{
   E_Tzsh_Quickpanel *qp;

   E_TZSH_CHECK_RET(resource);

   qp = wl_resource_get_user_data(resource);
   E_TZSH_CHECK_RET(qp);

   DBG("User Message : Open Quickpanel");

   if (!e_tzsh_user_event_send(qp->win,
                               E_TZSH_SERVICE_ROLE_QUICKPANEL,
                               E_TZSH_MESSAGE_QUICKPANEL_OPEN))
     {
        wl_resource_post_no_memory(resource);
        return;
     }
}

static void
_e_tzsh_quickpanel_cb_close(struct wl_client *client EINA_UNUSED, struct wl_resource *resource)
{
   E_Tzsh_Quickpanel *qp;

   E_TZSH_CHECK_RET(resource);

   qp = wl_resource_get_user_data(resource);
   E_TZSH_CHECK_RET(qp);

   DBG("User Message : Close Quickpanel");

   if (!e_tzsh_user_event_send(qp->win,
                               E_TZSH_SERVICE_ROLE_QUICKPANEL,
                               E_TZSH_MESSAGE_QUICKPANEL_CLOSE))
     {
        wl_resource_post_no_memory(resource);
        return;
     }
}

static void
_e_tzsh_quickpanel_cb_enable(struct wl_client *client EINA_UNUSED, struct wl_resource *resource EINA_UNUSED)
{
   E_Tzsh_Quickpanel *qp;

   E_TZSH_CHECK_RET(resource);

   qp = wl_resource_get_user_data(resource);
   E_TZSH_CHECK_RET(qp);

   DBG("User Message : Enable Quickpanel");

   if (!e_tzsh_user_event_send(qp->win,
                               E_TZSH_SERVICE_ROLE_QUICKPANEL,
                               E_TZSH_MESSAGE_QUICKPANEL_ENABLE))
     {
        wl_resource_post_no_memory(resource);
        return;
     }
}

static void
_e_tzsh_quickpanel_cb_disable(struct wl_client *client EINA_UNUSED, struct wl_resource *resource EINA_UNUSED)
{
   E_Tzsh_Quickpanel *qp;

   E_TZSH_CHECK_RET(resource);

   qp = wl_resource_get_user_data(resource);
   E_TZSH_CHECK_RET(qp);

   DBG("User Message : Disable Quickpanel");

   if (!e_tzsh_user_event_send(qp->win,
                               E_TZSH_SERVICE_ROLE_QUICKPANEL,
                               E_TZSH_MESSAGE_QUICKPANEL_DISABLE))
     {
        wl_resource_post_no_memory(resource);
        return;
     }
}

static const struct tws_quickpanel_interface _e_tzsh_quickpanel_interface =
{
   _e_tzsh_quickpanel_cb_release,
   _e_tzsh_quickpanel_cb_open,
   _e_tzsh_quickpanel_cb_close,
   _e_tzsh_quickpanel_cb_enable,
   _e_tzsh_quickpanel_cb_disable,
};

static void
_e_tzsh_quickpanel_cb_client_get(struct wl_client *client, struct wl_resource *resource EINA_UNUSED, uint32_t id, uint32_t win)
{
   E_Tzsh_Quickpanel *qp;

   DBG("Get interface : Quickpanel");

   if (win < 1)
     return;

   qp = E_NEW(E_Tzsh_Quickpanel, 1);
   E_TZSH_CHECK_RET(qp);

   qp->resource =
      wl_resource_create(client, &tws_quickpanel_interface, 1, id);
   if (!qp->resource)
     {
        ERR("Out of memory");
        wl_client_post_no_memory(client);
        free(qp);
        return;
     }

   qp->win = win;

   wl_resource_set_implementation(qp->resource,
                                  &_e_tzsh_quickpanel_interface,
                                  qp, NULL);
}

static void
_e_tzsh_quickpanel_service_cb_destroy(struct wl_client *client EINA_UNUSED, struct wl_resource *resource)
{
   E_TZSH_CHECK_RET(resource);
   DBG("Quickpanel service interface destroy");
   wl_resource_destroy(resource);
}

static void
_e_tzsh_quickpanel_service_cb_msg(struct wl_client *client EINA_UNUSED, struct wl_resource *resource, uint32_t msg)
{
   E_Tzsh_Service *service;

   E_TZSH_CHECK_RET(resource);

   DBG("Quickpanel message: %d", msg);

   service = wl_resource_get_user_data(resource);
   if (!service)
     {
        wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "no service for resource");
        return;
     }

   if (!e_tzsh_service_message_send(service, msg))
     {
        wl_resource_post_no_memory(resource);
        return;
     }
}

static const struct tws_service_quickpanel_interface _e_tzsh_quickpanel_service_interface =
{
   _e_tzsh_quickpanel_service_cb_destroy,
   _e_tzsh_quickpanel_service_cb_msg
};

static void
_e_tzsh_quickpanel_cb_service_get(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
   E_Tzsh_Service *service;
   struct wl_resource *new_resource;

   DBG("Get quickpanel service interface: %d", id);

   service = wl_resource_get_user_data(resource);

   new_resource =
      wl_resource_create(client, &tws_service_quickpanel_interface, 1, id);
   if (!new_resource)
     {
        wl_resource_post_no_memory(resource);
        return;
     }

   wl_resource_set_implementation(new_resource,
                                  &_e_tzsh_quickpanel_service_interface,
                                  service, NULL);
}

void
e_tzsh_quickpanel_init(void)
{
   e_tzsh_client_quickpanel_get_cb_set(_e_tzsh_quickpanel_cb_client_get);
   e_tzsh_service_quickpanel_get_cb_set(_e_tzsh_quickpanel_cb_service_get);
}

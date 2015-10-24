#ifndef _E_TZSH_SERVICE_H_
#define _E_TZSH_SERVICE_H_

typedef struct _E_Tzsh_Service E_Tzsh_Service;
typedef void (*E_Tzsh_Service_Cb)(struct wl_client *client, struct wl_resource *resource, uint32_t id);

E_Tzsh_Service       *e_tzsh_service_create(struct wl_client *client, uint32_t id, E_Tzsh_Client *tzsh_client, Ecore_X_Window win, char *name, int role);
void                  e_tzsh_service_destroy(E_Tzsh_Service *service);
Eina_Bool             e_tzsh_service_create_notify(E_Tzsh_Service *service);
char                 *e_tzsh_service_name_get(E_Tzsh_Service *service);
Ecore_X_Window        e_tzsh_service_window_get(E_Tzsh_Service *service);
E_Tzsh_Service_Role   e_tzsh_service_role_get(E_Tzsh_Service *service);
Eina_Bool             e_tzsh_service_message_send(E_Tzsh_Service *service, E_Tzsh_Message msg);
E_Tzsh_Service_Role   e_tzsh_service_role_get_by_name(char *name);
void                  e_tzsh_service_indicator_get_cb_set(E_Tzsh_Service_Cb func);
void                  e_tzsh_service_quickpanel_get_cb_set(E_Tzsh_Service_Cb func);

#endif

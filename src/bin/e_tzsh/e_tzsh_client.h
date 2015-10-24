#ifndef _E_TZSH_CLIENT_H_
#define _E_TZSH_CLIENT_H_

typedef struct _E_Tzsh_Client E_Tzsh_Client;
typedef void (*E_Tzsh_Client_Cb)(struct wl_client *client, struct wl_resource *resource, uint32_t id, uint32_t win);

E_Tzsh_Client  *e_tzsh_client_create(struct wl_client *client, int32_t version, int32_t id);
void            e_tzsh_client_destroy(E_Tzsh_Client *client);
void            e_tzsh_client_service_remove(E_Tzsh_Client *client, E_Tzsh_Service *service);
void            e_tzsh_client_service_register_notify(E_Tzsh_Client *client, char *name);
void            e_tzsh_client_service_remove_notify(E_Tzsh_Client *client, char *name);
void            e_tzsh_client_quickpanel_get_cb_set(E_Tzsh_Client_Cb func);

#endif

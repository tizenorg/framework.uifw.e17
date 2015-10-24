#ifndef _E_TZSH_REGION_H_
#define _E_TZSH_REGION_H_

typedef struct _E_Tzsh_Region E_Tzsh_Region;

void         e_tzsh_region_tws_region_create(struct wl_client *client, struct wl_resource *resource, uint32_t id);
Eina_Tiler  *e_tzsh_region_tiler_get(E_Tzsh_Region *region);

#endif

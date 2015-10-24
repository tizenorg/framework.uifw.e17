#include "e_tzsh_private.h"

struct _E_Tzsh_Region
{
   struct wl_resource *resource;
   struct wl_listener destroy_listener;
   Eina_Tiler *tiler;
};

static void
_e_tzsh_region_cb_destroy(struct wl_client *client EINA_UNUSED, struct wl_resource *resource)
{
   E_TZSH_CHECK_RET(resource);

   DBG("Region Destroy: %d", wl_resource_get_id(resource));

   wl_resource_destroy(resource);
}

static void
_e_tzsh_region_cb_add(struct wl_client *client EINA_UNUSED, struct wl_resource *resource, int32_t x, int32_t y, int32_t w, int32_t h)
{
   E_Tzsh_Region *region;
   Eina_Tiler *src;
   int area_w = 0, area_h = 0;

   E_TZSH_CHECK_RET(resource);

   DBG("Region Add: %d", wl_resource_get_id(resource));
   DBG("\tGeom: %d %d %d %d", x, y, w, h);

   /* get the tiler from the resource */
   region = wl_resource_get_user_data(resource);
   if (region->tiler)
     {
        eina_tiler_area_size_get(region->tiler, &area_w, &area_h);
        src = eina_tiler_new(area_w, area_h);
        eina_tiler_tile_size_set(src, 1, 1);
        eina_tiler_rect_add(src, &(Eina_Rectangle){x, y, w, h});
        eina_tiler_union(region->tiler, src);
        eina_tiler_free(src);
     }
}

static void
_e_tzsh_region_cb_subtract(struct wl_client *client EINA_UNUSED, struct wl_resource *resource, int32_t x, int32_t y, int32_t w, int32_t h)
{
   E_Tzsh_Region *region;
   Eina_Tiler *src;

   E_TZSH_CHECK_RET(resource);

   DBG("Region Subtract: %d", wl_resource_get_id(resource));
   DBG("\tGeom: %d %d %d %d", x, y, w, h);

   /* get the tiler from the resource */
   region = wl_resource_get_user_data(resource);
   if (region->tiler)
     {
        src = eina_tiler_new(w, h);
        eina_tiler_tile_size_set(src, 1, 1);
        eina_tiler_rect_add(src, &(Eina_Rectangle){x, y, w, h});
        eina_tiler_subtract(region->tiler, src);
        eina_tiler_free(src);
     }
}

static const struct tws_region_interface _e_tzsh_region_interface =
{
   _e_tzsh_region_cb_destroy,
   _e_tzsh_region_cb_add,
   _e_tzsh_region_cb_subtract
};

static void
_e_tzsh_region_destroy(E_Tzsh_Region *region)
{
   E_TZSH_CHECK_RET(region);

   wl_list_remove(&region->destroy_listener.link);
   eina_tiler_free(region->tiler);
   free(region);
}

static void
_e_tzsh_region_resource_destroy(struct wl_resource *resource)
{
   E_Tzsh_Region *region;

   E_TZSH_CHECK_RET(resource);

   DBG("Tizen WS Shell Region Destroy: %d", wl_resource_get_id(resource));

   /* try to get the tiler from the region resource */
   if ((region = wl_resource_get_user_data(resource)))
     _e_tzsh_region_destroy(region);
}

void
_e_tzsh_region_cb_client_destroy(struct wl_listener *listener, void *data EINA_UNUSED)
{
   E_Tzsh_Region *region = container_of(listener, E_Tzsh_Region, destroy_listener);

   _e_tzsh_region_destroy(region);
}

void
e_tzsh_region_tws_region_create(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
   E_Manager *m = e_manager_current_get();
   E_Zone *zone;
   E_Tzsh_Client *tzsh_client;
   E_Tzsh_Region *region;
   struct wl_resource *new_resource;

   E_TZSH_CHECK_RET(client);
   E_TZSH_CHECK_RET(resource);

   DBG("Region Create: %d", wl_resource_get_id(resource));

   tzsh_client = wl_resource_get_user_data(resource);
   if (!tzsh_client)
     {
        wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "No Tzsh Client for service");
        return;
     }

   region = E_NEW(E_Tzsh_Region, 1);
   if (!region)
     {
        wl_resource_post_no_memory(resource);
        return;
     }

   if ((!(zone = e_zone_current_get(e_container_current_get(m)))) ||
       (!(region->tiler = eina_tiler_new(zone->w, zone->h))))
     {
        ERR("Could not create Eina_Tiler");
        wl_resource_post_no_memory(resource);
        free(region);
        return;
     }

   eina_tiler_tile_size_set(region->tiler, 1, 1);

   if (!(new_resource = wl_resource_create(client, &tws_region_interface, 1, id)))
     {
        ERR("Failed to create region resource");
        wl_resource_post_no_memory(resource);
        eina_tiler_free(region->tiler);
        free(region);
        return;
     }

   wl_resource_set_implementation(new_resource, &_e_tzsh_region_interface, region,
                                  _e_tzsh_region_resource_destroy);

   region->destroy_listener.notify = _e_tzsh_region_cb_client_destroy;
   wl_resource_add_destroy_listener(resource, &region->destroy_listener);
}

Eina_Tiler *
e_tzsh_region_tiler_get(E_Tzsh_Region *region)
{
   E_TZSH_CHECK_RET_VAL(region, NULL);

   return region->tiler;
}

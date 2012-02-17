#include "e.h"

static void _e_shelf_free(E_Shelf *es);
static void _e_shelf_gadcon_min_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h);
static void _e_shelf_gadcon_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h);
static Evas_Object *_e_shelf_gadcon_frame_request(void *data, E_Gadcon_Client *gcc, const char *style);
static void _e_shelf_toggle_border_fix(E_Shelf *es);
static void _e_shelf_cb_menu_config(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_shelf_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_shelf_cb_menu_contents(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_shelf_cb_confirm_dialog_yes(void *data);
static void _e_shelf_cb_menu_delete(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_shelf_menu_append(E_Shelf *es, E_Menu *mn);
static void _e_shelf_cb_menu_items_append(void *data, E_Gadcon_Client *gcc, E_Menu *mn);
static void _e_shelf_cb_locked_set(void *data, int lock);
static void _e_shelf_cb_urgent_show(void *data);
static void _e_shelf_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static Eina_Bool _e_shelf_cb_mouse_in(void *data, int type, void *event);
static Eina_Bool _e_shelf_cb_mouse_out(void *data, int type, void *event);
static int _e_shelf_cb_id_sort(const void *data1, const void *data2);
static Eina_Bool _e_shelf_cb_hide_animator(void *data);
static Eina_Bool _e_shelf_cb_hide_animator_timer(void *data);
static Eina_Bool _e_shelf_cb_hide_urgent_timer(void *data);
static Eina_Bool _e_shelf_cb_instant_hide_timer(void *data);
static void _e_shelf_menu_pre_cb(void *data, E_Menu *m);
static void _e_shelf_gadcon_client_remove(void *data, E_Gadcon_Client *gcc);
static int _e_shelf_gadcon_client_add(void *data, const E_Gadcon_Client_Class *cc);
static const char * _e_shelf_orient_icon_name_get(E_Shelf *s);
static void _e_shelf_bindings_add(E_Shelf *es);
static void _e_shelf_bindings_del(E_Shelf *es);
static Eina_Bool _e_shelf_on_current_desk(E_Shelf *es, E_Event_Zone_Edge *ev);

static Eina_List *shelves = NULL;
static Eina_Hash *winid_shelves = NULL;

/* externally accessible functions */
EINTERN int
e_shelf_init(void)
{
   return 1;
}

EINTERN int
e_shelf_shutdown(void)
{
   if (x_fatal) return 1;
   while (shelves)
     {
	E_Shelf *es;

	es = eina_list_data_get(shelves);
	e_object_del(E_OBJECT(es));
     }

   return 1;
}

EAPI void
e_shelf_config_update(void)
{
   Eina_List *l;
   E_Config_Shelf *cf_es;
   int id = 0;

   while (shelves)
     {
	E_Shelf *es;

	es = eina_list_data_get(shelves);
	e_object_del(E_OBJECT(es));
     }

   EINA_LIST_FOREACH(e_config->shelves, l, cf_es)
     {
	E_Zone *zone;

	if (cf_es->id <= 0) cf_es->id = id + 1;
	zone = e_util_container_zone_id_get(cf_es->container, cf_es->zone);
	if (zone)
	  e_shelf_config_new(zone, cf_es);
	id = cf_es->id;
     }
}

EAPI Eina_List *
e_shelf_list(void)
{
   shelves = eina_list_sort(shelves, -1, _e_shelf_cb_id_sort);
   return shelves;
}

EAPI E_Shelf *
e_shelf_zone_new(E_Zone *zone, const char *name, const char *style, int popup, int layer, int id)
{
   E_Shelf *es;
   const char *option;
   char buf[1024];
   const char *locname;

   es = E_OBJECT_ALLOC(E_Shelf, E_SHELF_TYPE, _e_shelf_free);
   if (!es) return NULL;
   es->id = id;
   es->x = 0;
   es->y = 0;
   es->w = 32;
   es->h = 32;
   es->zone = zone;
   e_zone_useful_geometry_dirty(zone);
   if (popup)
     {
	es->popup = e_popup_new(zone, es->x, es->y, es->w, es->h);
        e_popup_name_set(es->popup, "shelf");
	e_popup_layer_set(es->popup, layer);
	es->ee = es->popup->ecore_evas;
	es->evas = es->popup->evas;
     }
   else
     {
	es->ee = zone->container->bg_ecore_evas;
	es->evas = zone->container->bg_evas;
     }
   es->fit_along = 1;
   es->layer = layer;
   es->style = eina_stringshare_add(style);

   es->o_event = evas_object_rectangle_add(es->evas);
   evas_object_color_set(es->o_event, 0, 0, 0, 0);
   evas_object_resize(es->o_event, es->w, es->h);
   evas_object_event_callback_add(es->o_event, EVAS_CALLBACK_MOUSE_DOWN, _e_shelf_cb_mouse_down, es);

   /* TODO: We should have a mouse out on the evas object if we are on the desktop */
   es->handlers = eina_list_append(es->handlers,
	 ecore_event_handler_add(E_EVENT_ZONE_EDGE_MOVE, _e_shelf_cb_mouse_in, es));
   es->handlers = eina_list_append(es->handlers,
	 ecore_event_handler_add(ECORE_X_EVENT_MOUSE_IN, _e_shelf_cb_mouse_in, es));
   es->handlers = eina_list_append(es->handlers,
	 ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE, _e_shelf_cb_mouse_in, es));
   es->handlers = eina_list_append(es->handlers,
	 ecore_event_handler_add(ECORE_X_EVENT_MOUSE_OUT, _e_shelf_cb_mouse_out, es));

   es->o_base = edje_object_add(es->evas);
   es->name = eina_stringshare_add(name);
   snprintf(buf, sizeof(buf), "e/shelf/%s/base", es->style);
   evas_object_resize(es->o_base, es->w, es->h);
   if (!e_theme_edje_object_set(es->o_base, "base/theme/shelf", buf))
     e_theme_edje_object_set(es->o_base, "base/theme/shelf",
			     "e/shelf/default/base");
   if (es->popup)
     {
	evas_object_show(es->o_event);
	evas_object_show(es->o_base);
	e_popup_edje_bg_object_set(es->popup, es->o_base);
	ecore_x_netwm_window_type_set(es->popup->evas_win, ECORE_X_WINDOW_TYPE_DOCK);
     }
   else
     {
	evas_object_move(es->o_event, es->zone->x + es->x, es->zone->y + es->y);
	evas_object_move(es->o_base, es->zone->x + es->x, es->zone->y + es->y);
	evas_object_layer_set(es->o_event, layer);
	evas_object_layer_set(es->o_base, layer);
     }

   es->gadcon = e_gadcon_swallowed_new(es->name, es->id, es->o_base, "e.swallow.content");
   locname = es->name;
   if (!name) locname = _("Shelf #");
   snprintf(buf, sizeof(buf), "%s %i", locname, es->id);
   es->gadcon->location = e_gadcon_location_new(buf, E_GADCON_SITE_SHELF, _e_shelf_gadcon_client_add, es, _e_shelf_gadcon_client_remove, es);
   e_gadcon_location_register(es->gadcon->location);
// hmm dnd in ibar and ibox kill this. ok. need to look into this more   
//   es->gadcon->instant_edit = 1;
   e_gadcon_min_size_request_callback_set(es->gadcon,
					  _e_shelf_gadcon_min_size_request, es);

   e_gadcon_size_request_callback_set(es->gadcon,
				      _e_shelf_gadcon_size_request, es);
   e_gadcon_frame_request_callback_set(es->gadcon,
				       _e_shelf_gadcon_frame_request, es);
   e_gadcon_orient(es->gadcon, E_GADCON_ORIENT_TOP);
   snprintf(buf, sizeof(buf), "e,state,orientation,%s", 
	    e_shelf_orient_string_get(es));
   edje_object_signal_emit(es->o_base, buf, "e");
   edje_object_message_signal_process(es->o_base);
   e_gadcon_zone_set(es->gadcon, zone);
   e_gadcon_ecore_evas_set(es->gadcon, es->ee);
   e_gadcon_shelf_set(es->gadcon, es);
   if (popup)
     {
        if (!winid_shelves) 
          winid_shelves = eina_hash_string_superfast_new(NULL);
        eina_hash_add(winid_shelves, 
                      e_util_winid_str_get(es->popup->evas_win), es);
	e_drop_xdnd_register_set(es->popup->evas_win, 1);
	e_gadcon_xdnd_window_set(es->gadcon, es->popup->evas_win);
	e_gadcon_dnd_window_set(es->gadcon, es->popup->evas_win);
     }
   else
     {
	e_drop_xdnd_register_set(es->zone->container->bg_win, 1);
	e_gadcon_xdnd_window_set(es->gadcon, es->zone->container->bg_win);
	e_gadcon_dnd_window_set(es->gadcon, es->zone->container->event_win);
     }
   e_gadcon_util_menu_attach_func_set(es->gadcon,
				      _e_shelf_cb_menu_items_append, es);

   e_gadcon_util_lock_func_set(es->gadcon,
			       _e_shelf_cb_locked_set, es);
   e_gadcon_util_urgent_show_func_set(es->gadcon,
				      _e_shelf_cb_urgent_show, es);

   shelves = eina_list_append(shelves, es);

   es->hidden = 0;
   es->hide_step = 0;
   es->locked = 0;

   option = edje_object_data_get(es->o_base, "hidden_state_size");
   if (option)
     es->hidden_state_size = atoi(option);
   else
     es->hidden_state_size = 4;
   option = edje_object_data_get(es->o_base, "instant_delay");
   if (option)
     es->instant_delay = atof(option);
   else
     es->instant_delay = -1.0;

   es->hide_origin = -1;

   return es;
}

EAPI void
e_shelf_zone_move_resize_handle(E_Zone *zone)
{
   Eina_List *l;
   E_Shelf *es;
   Evas_Coord w, h;

   EINA_LIST_FOREACH(shelves, l, es)
     {
	if (es->zone == zone) 
	  {
	     E_Gadcon * gc;

	     gc = es->gadcon;
	     if (gc->min_size_request.func)
	       {
	          /* let gadcon container decrease to any size */
	          edje_extern_object_min_size_set(gc->o_container, 0, 0);
	       }
	     evas_object_smart_callback_call (gc->o_container, "min_size_request", NULL);
	     e_shelf_position_calc(es);
	     if (gc->min_size_request.func)
	       {
	          evas_object_geometry_get(gc->o_container, NULL, NULL, &w, &h);
	          /* fix gadcon container min size to current geometry */
	          edje_extern_object_min_size_set(gc->o_container, w, h);
	       }
	  }
     }
}

EAPI void
e_shelf_populate(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);
   e_gadcon_populate(es->gadcon);
}

EAPI void
e_shelf_show(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);
   if (es->popup)
     e_popup_show(es->popup);
   else
     {
	evas_object_show(es->o_event);
	evas_object_show(es->o_base);
     }
}

EAPI void
e_shelf_hide(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);
   if (es->popup)
     e_popup_hide(es->popup);
   else
     {
	evas_object_hide(es->o_event);
	evas_object_hide(es->o_base);
     }
}

EAPI void
e_shelf_locked_set(E_Shelf *es, int lock)
{
   if (lock)
     {
	e_shelf_toggle(es, 1);
	es->locked++;
     }
   else
     {
       if (es->locked > 0)
	 es->locked--;
       if (!es->locked)
	 e_shelf_toggle(es, es->toggle);
     }
}

EAPI void
e_shelf_toggle(E_Shelf *es, int show)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);

   es->toggle = show;
   if (es->locked) return;
   es->interrupted = -1;
   es->urgent_show = 0;
   if ((show) && (es->hidden))
     {
	es->hidden = 0;
	edje_object_signal_emit(es->o_base, "e,state,visible", "e");
	if (es->instant_delay >= 0.0)
	  {
	     _e_shelf_cb_instant_hide_timer(es);
	     es->hide_timer = 
               ecore_timer_add(es->cfg->hide_timeout, 
                               _e_shelf_cb_hide_urgent_timer, es);
	  }
	else
	  {
	     if (es->hide_timer)
	       {
		  ecore_timer_del(es->hide_timer);
		  es->hide_timer = NULL;
	       }
	     if (!es->hide_animator)
	       es->hide_animator = 
               ecore_animator_add(_e_shelf_cb_hide_animator, es);
	  }
     }
   else if ((!show) && (!es->hidden) && (!es->gadcon->editing) && 
            (es->cfg->autohide))
     {
	edje_object_signal_emit(es->o_base, "e,state,hidden", "e");
	if (es->instant_delay >= 0.0)
	  {
             if (es->hide_timer)
               {
                  ecore_timer_del(es->hide_timer);
                  es->hide_timer = NULL;
               }
	     es->hidden = 1; 
	     if (!es->instant_timer)
	       es->instant_timer = 
               ecore_timer_add(es->instant_delay, 
                               _e_shelf_cb_instant_hide_timer, es);
	  }
	else
	  {
	     if (es->hide_animator)
	       {
		  es->interrupted = show;
		  return;
	       }
	     es->hidden = 1; 
	     if (es->hide_timer) ecore_timer_del(es->hide_timer);
	     es->hide_timer = 
               ecore_timer_add(es->cfg->hide_timeout, 
                               _e_shelf_cb_hide_animator_timer, es);
	  }
     }
}

EAPI void
e_shelf_urgent_show(E_Shelf *es)
{
   e_shelf_toggle(es, 1);
   es->urgent_show = 1;
}

EAPI void
e_shelf_move(E_Shelf *es, int x, int y)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);
   es->x = x;
   es->y = y;
   if (es->popup)
     e_popup_move(es->popup, es->x, es->y);
   else
     {
	evas_object_move(es->o_event, es->zone->x + es->x, es->zone->y + es->y);
	evas_object_move(es->o_base, es->zone->x + es->x, es->zone->y + es->y);
     }
}

EAPI void
e_shelf_resize(E_Shelf *es, int w, int h)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);
   es->w = w;
   es->h = h;
   if (es->popup) e_popup_resize(es->popup, es->w, es->h);
   evas_object_resize(es->o_event, es->w, es->h);
   evas_object_resize(es->o_base, es->w, es->h);
}

EAPI void
e_shelf_move_resize(E_Shelf *es, int x, int y, int w, int h)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);
   es->x = x;
   es->y = y;
   es->w = w;
   es->h = h;
   if (es->popup) 
     e_popup_move_resize(es->popup, es->x, es->y, es->w, es->h); 
   else
     {
	evas_object_move(es->o_event, es->zone->x + es->x, es->zone->y + es->y);
	evas_object_move(es->o_base, es->zone->x + es->x, es->zone->y + es->y);
     }
   evas_object_resize(es->o_event, es->w, es->h);
   evas_object_resize(es->o_base, es->w, es->h);
}

EAPI void
e_shelf_layer_set(E_Shelf *es, int layer)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);

   es->layer = layer;
   if (es->popup)
     e_popup_layer_set(es->popup, es->layer);
   else
     {
	evas_object_layer_set(es->o_event, es->layer);
	evas_object_layer_set(es->o_base, es->layer);
     }
}

EAPI void
e_shelf_save(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);
   if (es->cfg)
     {
	es->cfg->orient = es->gadcon->orient;
	if (es->cfg->style) eina_stringshare_del(es->cfg->style);
	es->cfg->style = eina_stringshare_add(es->style);
     }
   else
     {
	E_Config_Shelf *cf_es;

	cf_es = E_NEW(E_Config_Shelf, 1);
	cf_es->name = eina_stringshare_add(es->name);
	cf_es->container = es->zone->container->num;
	cf_es->zone = es->zone->num;
	if (es->popup) cf_es->popup = 1;
	cf_es->layer = es->layer;
	e_config->shelves = eina_list_append(e_config->shelves, cf_es);
	cf_es->orient = es->gadcon->orient;
	cf_es->style = eina_stringshare_add(es->style);
	cf_es->fit_along = es->fit_along;
	cf_es->fit_size = es->fit_size;
	cf_es->overlap = 0;
	cf_es->autohide = 0; 
	cf_es->hide_timeout = 1.0; 
	cf_es->hide_duration = 1.0;
	es->cfg = cf_es;
     }
   e_config_save_queue();
}

EAPI void
e_shelf_unsave(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);
   if (es->cfg)
     {
	e_config->shelves = eina_list_remove(e_config->shelves, es->cfg);
	eina_stringshare_del(es->cfg->name);
	if (es->cfg->style) eina_stringshare_del(es->cfg->style);
	free(es->cfg);
     }
}

EAPI void
e_shelf_orient(E_Shelf *es, E_Gadcon_Orient orient)
{
   char buf[PATH_MAX];

   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);

   e_gadcon_orient(es->gadcon, orient);
   snprintf(buf, sizeof(buf), "e,state,orientation,%s", 
            e_shelf_orient_string_get(es));
   edje_object_signal_emit(es->o_base, buf, "e");
   edje_object_message_signal_process(es->o_base);
   e_gadcon_location_set_icon_name(es->gadcon->location, _e_shelf_orient_icon_name_get(es));
   e_zone_useful_geometry_dirty(es->zone);
}

EAPI const char *
e_shelf_orient_string_get(E_Shelf *es)
{
   const char *sig = "";

   switch (es->gadcon->orient)
     {
      case E_GADCON_ORIENT_FLOAT:
	sig = "float";
	break;
      case E_GADCON_ORIENT_HORIZ:
	sig = "horizontal";
	break;
      case E_GADCON_ORIENT_VERT:
	sig = "vertical";
	break;
      case E_GADCON_ORIENT_LEFT:
	sig = "left";
	break;
      case E_GADCON_ORIENT_RIGHT:
	sig = "right";
	break;
      case E_GADCON_ORIENT_TOP:
	sig = "top";
	break;
      case E_GADCON_ORIENT_BOTTOM:
	sig = "bottom";
	break;
      case E_GADCON_ORIENT_CORNER_TL:
	sig = "top_left";
	break;
      case E_GADCON_ORIENT_CORNER_TR:
	sig = "top_right";
	break;
      case E_GADCON_ORIENT_CORNER_BL:
	sig = "bottom_left";
	break;
      case E_GADCON_ORIENT_CORNER_BR:
	sig = "bottom_right";
	break;
      case E_GADCON_ORIENT_CORNER_LT:
	sig = "left_top";
	break;
      case E_GADCON_ORIENT_CORNER_RT:
	sig = "right_top";
	break;
      case E_GADCON_ORIENT_CORNER_LB:
	sig = "left_bottom";
	break;
      case E_GADCON_ORIENT_CORNER_RB:
	sig = "right_bottom";
	break;
      default:
	break;
     }
   return sig;
}

EAPI void
e_shelf_position_calc(E_Shelf *es)
{
   E_Gadcon_Orient orient = E_GADCON_ORIENT_FLOAT;
   int size = 40;

   if (es->cfg)
     {
	orient = es->cfg->orient;
	size = es->cfg->size * e_scale;
     }
   else
     orient = es->gadcon->orient;
   switch (orient)
     {
      case E_GADCON_ORIENT_FLOAT:
	if (!es->fit_along) es->w = es->zone->w;
	if (!es->fit_size) es->h = size;
	break;
      case E_GADCON_ORIENT_HORIZ:
	if (!es->fit_along) es->w = es->zone->w;
	if (!es->fit_size) es->h = size;
	es->x = (es->zone->w - es->w) / 2;
	break;
      case E_GADCON_ORIENT_VERT:
	if (!es->fit_along) es->h = es->zone->h;
	if (!es->fit_size) es->w = size;
	es->y = (es->zone->h - es->h) / 2;
	break;
      case E_GADCON_ORIENT_LEFT:
	if (!es->fit_along) es->h = es->zone->h;
	if (!es->fit_size) es->w = size;
	es->x = 0;
	es->y = (es->zone->h - es->h) / 2;
	break;
      case E_GADCON_ORIENT_RIGHT:
	if (!es->fit_along) es->h = es->zone->h;
	if (!es->fit_size) es->w = size;
	es->x = es->zone->w - es->w;
	es->y = (es->zone->h - es->h) / 2;
	break;
      case E_GADCON_ORIENT_TOP:
	if (!es->fit_along) es->w = es->zone->w;
	if (!es->fit_size) es->h = size;
	es->x = (es->zone->w - es->w) / 2;
	es->y = 0;
	break;
      case E_GADCON_ORIENT_BOTTOM:
	if (!es->fit_along) es->w = es->zone->w;
	if (!es->fit_size) es->h = size;
	es->x = (es->zone->w - es->w) / 2;
	es->y = es->zone->h - es->h;
	break;
      case E_GADCON_ORIENT_CORNER_TL:
	if (!es->fit_along) es->w = es->zone->w;
	if (!es->fit_size) es->h = size;
	es->x = 0;
	es->y = 0;
	break;
      case E_GADCON_ORIENT_CORNER_TR:
	if (!es->fit_along) es->w = es->zone->w;
	if (!es->fit_size) es->h = size;
	es->x = es->zone->w - es->w;
	es->y = 0;
	break;
      case E_GADCON_ORIENT_CORNER_BL:
	if (!es->fit_along) es->w = es->zone->w;
	if (!es->fit_size) es->h = size;
	es->x = 0;
	es->y = es->zone->h - es->h;
	break;
      case E_GADCON_ORIENT_CORNER_BR:
	if (!es->fit_along) es->w = es->zone->w;
	if (!es->fit_size) es->h = size;
	es->x = es->zone->w - es->w;
	es->y = es->zone->h - es->h;
	break;
      case E_GADCON_ORIENT_CORNER_LT:
	if (!es->fit_along) es->h = es->zone->h;
	if (!es->fit_size) es->w = size;
	es->x = 0;
	es->y = 0;
	break;
      case E_GADCON_ORIENT_CORNER_RT:
	if (!es->fit_along) es->h = es->zone->h;
	if (!es->fit_size) es->w = size;
	es->x = es->zone->w - es->w;
	es->y = 0;
	break;
      case E_GADCON_ORIENT_CORNER_LB:
	if (!es->fit_along) es->h = es->zone->h;
	if (!es->fit_size) es->w = size;
	es->x = 0;
	es->y = es->zone->h - es->h;
	break;
      case E_GADCON_ORIENT_CORNER_RB:
	if (!es->fit_along) es->h = es->zone->h;
	if (!es->fit_size) es->w = size;
	es->x = es->zone->w - es->w;
	es->y = es->zone->h - es->h;
	break;
      default:
	break;
     }
   es->hide_step = 0;
   es->hide_origin = -1;

   e_shelf_move_resize(es, es->x, es->y, es->w, es->h);
   if (es->hidden)
     {
	es->hidden = 0;
	e_shelf_toggle(es, 0);
     }
   e_zone_useful_geometry_dirty(es->zone);
   _e_shelf_bindings_add(es);
}

EAPI void 
e_shelf_style_set(E_Shelf *es, const char *style) 
{
   const char *option;
   char buf[1024];

   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);

   if (!es->o_base) return;
   if (es->style) eina_stringshare_del(es->style);
   es->style = eina_stringshare_add(style);

   if (style)
     snprintf(buf, sizeof(buf), "e/shelf/%s/base", style);
   else
     snprintf(buf, sizeof(buf), "e/shelf/%s/base", "default");
     
   if (!e_theme_edje_object_set(es->o_base, "base/theme/shelf", buf))
     e_theme_edje_object_set(es->o_base, "base/theme/shelf", 
			     "e/shelf/default/base");

   option =  edje_object_data_get(es->o_base, "hidden_state_size");
   if (option)
     es->hidden_state_size = atoi(option);
   else
     es->hidden_state_size = 4;
   option =  edje_object_data_get(es->o_base, "instant_delay");
   if (option)
     es->instant_delay = atof(option);
   else
     es->instant_delay = -1.0;

   e_gadcon_unpopulate(es->gadcon);
   e_gadcon_populate(es->gadcon);
}

EAPI void 
e_shelf_popup_set(E_Shelf *es, int popup) 
{
   /* FIXME: Needs to recreate the evas objects. */
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_SHELF_TYPE);

   if (!es->cfg) return;
   if (((popup) && (es->popup)) || ((!popup) && (!es->popup))) return;

   if (popup) 
     {
	es->popup = e_popup_new(es->zone, es->x, es->y, es->w, es->h);
        e_popup_name_set(es->popup, "shelf");
	e_popup_layer_set(es->popup, es->cfg->layer);

	es->ee = es->popup->ecore_evas;
	es->evas = es->popup->evas;
	evas_object_show(es->o_event);
	evas_object_show(es->o_base);
	e_popup_edje_bg_object_set(es->popup, es->o_base);
	ecore_x_netwm_window_type_set(es->popup->evas_win, ECORE_X_WINDOW_TYPE_DOCK);
	
	e_drop_xdnd_register_set(es->popup->evas_win, 1);
	e_gadcon_xdnd_window_set(es->gadcon, es->popup->evas_win);
	e_gadcon_dnd_window_set(es->gadcon, es->popup->evas_win);
     }
   else 
     {
	e_drop_xdnd_register_set(es->popup->evas_win, 0);
	e_object_del(E_OBJECT(es->popup));
	es->popup = NULL;

	es->ee = es->zone->container->bg_ecore_evas;
	es->evas = es->zone->container->bg_evas;

	evas_object_move(es->o_event, es->zone->x + es->x, es->zone->y + es->y);
	evas_object_move(es->o_base, es->zone->x + es->x, es->zone->y + es->y);
	evas_object_layer_set(es->o_event, es->cfg->layer);
	evas_object_layer_set(es->o_base, es->cfg->layer);

	e_drop_xdnd_register_set(es->zone->container->bg_win, 1);
	e_gadcon_xdnd_window_set(es->gadcon, es->zone->container->bg_win);
	e_gadcon_dnd_window_set(es->gadcon, es->zone->container->event_win);
     }
}

EAPI E_Shelf *
e_shelf_config_new(E_Zone *zone, E_Config_Shelf *cf_es)
{
   E_Shelf *es;

   es = e_shelf_zone_new(zone, cf_es->name, cf_es->style,
                         cf_es->popup, cf_es->layer, cf_es->id);
   if (!es) return NULL;

   if (!cf_es->hide_timeout) cf_es->hide_timeout = 1.0; 
   if (!cf_es->hide_duration) cf_es->hide_duration = 1.0; 
   es->cfg = cf_es;
   es->fit_along = cf_es->fit_along;
   es->fit_size = cf_es->fit_size;

   e_shelf_orient(es, cf_es->orient);
   e_shelf_position_calc(es);
   e_shelf_populate(es);

   if (cf_es->desk_show_mode)
     {
	E_Desk *desk;
	Eina_List *ll;
	E_Config_Shelf_Desk *sd;

	desk = e_desk_current_get(zone);
	EINA_LIST_FOREACH(cf_es->desk_list, ll, sd)
	  {
	     if ((desk->x == sd->x) && (desk->y == sd->y))
	       {
		  e_shelf_show(es);
		  break;
	       }
	  }
     }
   else
     e_shelf_show(es);

   e_shelf_toggle(es, 0);
   return es;
}

/* local subsystem functions */
static void
_e_shelf_free(E_Shelf *es)
{
   _e_shelf_bindings_del(es);

   e_gadcon_location_unregister(es->gadcon->location);
   e_gadcon_location_free(es->gadcon->location); 
   e_zone_useful_geometry_dirty(es->zone);
   E_FREE_LIST(es->handlers, ecore_event_handler_del);

   e_object_del(E_OBJECT(es->gadcon));
   if (es->hide_timer)
     {
	ecore_timer_del(es->hide_timer);
	es->hide_timer  = NULL;
     }
   if (es->hide_animator)
     {
	ecore_animator_del(es->hide_animator);
	es->hide_animator = NULL;
     }
   if (es->instant_timer)
     {
	ecore_timer_del(es->instant_timer);
	es->instant_timer = NULL;
     }

   if (es->menu)
     {
	e_menu_post_deactivate_callback_set(es->menu, NULL, NULL);
	e_object_del(E_OBJECT(es->menu));
	es->menu = NULL;
     }
   if (es->config_dialog) e_object_del(E_OBJECT(es->config_dialog));
   shelves = eina_list_remove(shelves, es);
   eina_stringshare_del(es->name);
   eina_stringshare_del(es->style);
   evas_object_del(es->o_event);
   evas_object_del(es->o_base);
   if (es->popup)
     {
	e_drop_xdnd_register_set(es->popup->evas_win, 0);
	eina_hash_del(winid_shelves, 
                      e_util_winid_str_get(es->popup->evas_win), es);
	if (!eina_hash_population(winid_shelves))
	  {
	    eina_hash_free(winid_shelves);
	    winid_shelves = NULL;
	  }
	e_object_del(E_OBJECT(es->popup));
     }
   free(es);
}

static void
_e_shelf_gadcon_min_size_request(void *data __UNUSED__, E_Gadcon *gc __UNUSED__, Evas_Coord w __UNUSED__, Evas_Coord h __UNUSED__)
{
   return;
}

static void
_e_shelf_gadcon_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h)
{
   E_Shelf *es;
   Evas_Coord nx, ny, nw, nh, ww, hh, wantw, wanth;

   es = data;
   nx = es->x;
   ny = es->y;
   nw = es->w;
   nh = es->h;
   ww = hh = 0;
   evas_object_geometry_get(gc->o_container, NULL, NULL, &ww, &hh);
   switch (gc->orient)
     {
      case E_GADCON_ORIENT_FLOAT:
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_BOTTOM:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_BR:
	if (!es->fit_along) w = ww;
	if (!es->fit_size) h = hh;
	break;
      case E_GADCON_ORIENT_VERT:
      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_RIGHT:
      case E_GADCON_ORIENT_CORNER_LT:
      case E_GADCON_ORIENT_CORNER_RT:
      case E_GADCON_ORIENT_CORNER_LB:
      case E_GADCON_ORIENT_CORNER_RB:
	if (!es->fit_along) h = hh;
	if (!es->fit_size) w = ww;
	break;
      default:
	break;
     }
   e_gadcon_swallowed_min_size_set(gc, w, h);
   edje_object_size_min_calc(es->o_base, &nw, &nh);
   wantw = nw;
   wanth = nh;
   switch (gc->orient)
     {
      case E_GADCON_ORIENT_FLOAT:
	if (!es->fit_along) nw = es->w;
	if (!es->fit_size) nh = es->h;
	if (nw > es->zone->w) nw = es->zone->w;
	if (nh > es->zone->h) nh = es->zone->h;
	if (nw != es->w) nx = es->x + ((es->w - nw) / 2);
	break;
      case E_GADCON_ORIENT_HORIZ:
	if (!es->fit_along) nw = es->w;
	if (!es->fit_size) nh = es->h;
	if (nw > es->zone->w) nw = es->zone->w;
	if (nh > es->zone->h) nh = es->zone->h;
	if (nw != es->w) nx = es->x + ((es->w - nw) / 2);
	break;
      case E_GADCON_ORIENT_VERT:
	if (!es->fit_along) nh = es->h;
	if (!es->fit_size) nw = es->w;
	if (nw > es->zone->w) nw = es->zone->w;
	if (nh > es->zone->h) nh = es->zone->h;
	if (nh != es->h) ny = es->y + ((es->h - nh) / 2);
	break;
      case E_GADCON_ORIENT_LEFT:
	if (!es->fit_along) nh = es->h;
	if (!es->fit_size) nw = es->w;
	if (nw > es->zone->w) nw = es->zone->w;
	if (nh > es->zone->h) nh = es->zone->h;
	if (nh != es->h) ny = (es->zone->h - nh) / 2;
	// nx = 0;
	break;
      case E_GADCON_ORIENT_RIGHT:
	if (!es->fit_along) nh = es->h;
	if (!es->fit_size) nw = es->w;
	if (nw > es->zone->w) nw = es->zone->w;
	if (nh > es->zone->h) nh = es->zone->h;
	if (nh != es->h) ny = (es->zone->h - nh) / 2;
	// nx = es->zone->w - nw;
	break;
      case E_GADCON_ORIENT_TOP:
	if (!es->fit_along) nw = es->w;
	if (!es->fit_size) nh = es->h;
	if (nw > es->zone->w) nw = es->zone->w;
	if (nh > es->zone->h) nh = es->zone->h;
	if (nw != es->w) nx = (es->zone->w - nw) / 2;
	// ny = 0;
	break;
      case E_GADCON_ORIENT_BOTTOM:
	if (!es->fit_along) nw = es->w;
	if (!es->fit_size) nh = es->h;
	if (nw > es->zone->w) nw = es->zone->w;
	if (nh > es->zone->h) nh = es->zone->h;
	if (nw != es->w) nx = (es->zone->w - nw) / 2;
	//ny = es->zone->h - nh;
	break;
      case E_GADCON_ORIENT_CORNER_TL:
	if (!es->fit_along) nw = es->w;
	if (!es->fit_size) nh = es->h;
	if (nw > es->zone->w) nw = es->zone->w;
	if (nh > es->zone->h) nh = es->zone->h;
	if (nw != es->w) nx = 0;
	// ny = 0;
	break;
      case E_GADCON_ORIENT_CORNER_TR:
	if (!es->fit_along) nw = es->w;
	if (!es->fit_size) nh = es->h;
	if (nw > es->zone->w) nw = es->zone->w;
	if (nh > es->zone->h) nh = es->zone->h;
	nx = es->zone->w - nw;
	// ny = 0;
	break;
      case E_GADCON_ORIENT_CORNER_BL:
	if (!es->fit_along) nw = es->w;
	if (!es->fit_size) nh = es->h;
	if (nw > es->zone->w) nw = es->zone->w;
	if (nh > es->zone->h) nh = es->zone->h;
	if (nw != es->w) nx = 0;
	// ny = es->zone->h - nh;
	break;
      case E_GADCON_ORIENT_CORNER_BR:
	if (!es->fit_along) nw = es->w;
	if (!es->fit_size) nh = es->h;
	if (nw > es->zone->w) nw = es->zone->w;
	if (nh > es->zone->h) nh = es->zone->h;
	nx = es->zone->w - nw;
	//ny = es->zone->h - nh;
	break;
      case E_GADCON_ORIENT_CORNER_LT:
	if (!es->fit_along) nh = es->h;
	if (!es->fit_size) nw = es->w;
	if (nw > es->zone->w) nw = es->zone->w;
	if (nh > es->zone->h) nh = es->zone->h;
	if (nh != es->h) ny = 0;
	// nx = 0;
	break;
      case E_GADCON_ORIENT_CORNER_RT:
	if (!es->fit_along) nh = es->h;
	if (!es->fit_size) nw = es->w;
	if (nw > es->zone->w) nw = es->zone->w;
	if (nh > es->zone->h) nh = es->zone->h;
	if (nh != es->h) ny = 0;
	// nx = es->zone->w - nw;
	break;
      case E_GADCON_ORIENT_CORNER_LB:
	if (!es->fit_along) nh = es->h;
	if (!es->fit_size) nw = es->w;
	if (nw > es->zone->w) nw = es->zone->w;
	if (nh > es->zone->h) nh = es->zone->h;
	if (nh != es->h) ny = es->zone->h - nh;
	// nx = 0;
	break;
      case E_GADCON_ORIENT_CORNER_RB:
	if (!es->fit_along) nh = es->h;
	if (!es->fit_size) nw = es->w;
	if (nw > es->zone->w) nw = es->zone->w;
	if (nh > es->zone->h) nh = es->zone->h;
	if (nh != es->h) ny = es->zone->h - nh;
	// nx = es->zone->w - nw;
	break;
      default:
	break;
     }
   w -= (wantw - nw);
   h -= (wanth - nh);
   e_gadcon_swallowed_min_size_set(gc, w, h);
   e_shelf_move_resize(es, nx, ny, nw, nh);
   e_zone_useful_geometry_dirty(es->zone);
}

static Evas_Object *
_e_shelf_gadcon_frame_request(void *data, E_Gadcon_Client *gcc, const char *style)
{
   E_Shelf *es;
   Evas_Object *o;
   char buf[PATH_MAX];

   es = data;
   o = edje_object_add(gcc->gadcon->evas);

   snprintf(buf, sizeof(buf), "e/shelf/%s/%s", es->style, style);
   if (!e_theme_edje_object_set(o, "base/theme/shelf", buf))
     {
	/* if an inset style (e.g. plain) isn't implemented for a given
	 * shelf style, fall back to the default one. no need for every
	 * theme to implement the plain style */
	snprintf(buf, sizeof(buf), "e/shelf/default/%s", style);
	if (!e_theme_edje_object_set(o, "base/theme/shelf", buf))
	  {
	     evas_object_del(o);
	     return NULL;
	  }
     }
   snprintf(buf, sizeof(buf), "e,state,orientation,%s", 
            e_shelf_orient_string_get(es));
   edje_object_signal_emit(es->o_base, buf, "e");
   edje_object_message_signal_process(o);
   return o;
}

static void
_e_shelf_toggle_border_fix(E_Shelf *es)
{
   Eina_List *l;
   E_Border *bd;

   if ((es->cfg->overlap) || (!e_config->border_fix_on_shelf_toggle))
     return;

   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
	if ((bd->maximized & E_MAXIMIZE_TYPE) == E_MAXIMIZE_NONE)
	  {
	     if (bd->lock_client_location) continue;
	     if (es->hidden)
	       {
		  if (!bd->shelf_fix.modified) continue;
		  if (!--bd->shelf_fix.modified)
		    {
		       e_border_move(bd, bd->shelf_fix.x, bd->shelf_fix.y);
		       continue;
		    }
	       }

	     if (!E_INTERSECTS(bd->x, bd->y, bd->w, bd->h,
                               es->x, es->y, es->w, es->h))
	       continue;

	     if (!es->hidden)
	       {
		  if (!bd->shelf_fix.modified++)
		    bd->shelf_fix.x = bd->x;
		    bd->shelf_fix.y = bd->y;
	       }

	     switch (es->gadcon->orient)
	       {
		case E_GADCON_ORIENT_TOP:
		case E_GADCON_ORIENT_CORNER_TL:
		case E_GADCON_ORIENT_CORNER_TR:
                  if (!es->hidden)
                    e_border_move(bd, bd->x, bd->y + es->h);
                  break;
		case E_GADCON_ORIENT_BOTTOM:
		case E_GADCON_ORIENT_CORNER_BL:
		case E_GADCON_ORIENT_CORNER_BR:
                  if (!es->hidden)
                    e_border_move(bd, bd->x, bd->y - es->h);
                  break;
		case E_GADCON_ORIENT_LEFT:
		case E_GADCON_ORIENT_CORNER_LB:
		case E_GADCON_ORIENT_CORNER_LT:
                  if (!es->hidden)
                    e_border_move(bd, bd->x + es->w, bd->y);
                  break; 
		case E_GADCON_ORIENT_RIGHT:
		case E_GADCON_ORIENT_CORNER_RB:
		case E_GADCON_ORIENT_CORNER_RT:
                  if (!es->hidden)
                    e_border_move(bd, bd->x - es->w, bd->y);
                  break; 
		default:
		   break;
	       }
	  }
	else
	  {
	     E_Maximize max;

	     max = bd->maximized;
	     e_border_unmaximize(bd, E_MAXIMIZE_BOTH);
	     e_border_maximize(bd, max);
	  }
     }
}

static void
_e_shelf_menu_item_free(void *data)
{
   E_Shelf *es;

   es = e_object_data_get(data);
   e_shelf_locked_set(es, 0);
}

static void
_e_shelf_menu_append(E_Shelf *es, E_Menu *mn)
{
   E_Menu_Item *mi;
   E_Menu *subm;
   const char *name;
   char buf[256];

   name = e_shelf_orient_string_get (es);
   snprintf(buf, sizeof(buf), "Shelf %s", name);

   e_shelf_locked_set(es, 1);

   subm = e_menu_new();
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, buf);
   e_util_menu_item_theme_icon_set(mi, "preferences-desktop-shelf");
   e_menu_pre_activate_callback_set(subm, _e_shelf_menu_pre_cb, es);
   e_object_free_attach_func_set(E_OBJECT(mi), _e_shelf_menu_item_free);
   e_object_data_set(E_OBJECT(mi), es);
   e_menu_item_submenu_set(mi, subm);
}

static void
_e_shelf_cb_menu_items_append(void *data, E_Gadcon_Client *gcc __UNUSED__, E_Menu *mn)
{
   E_Shelf *es;

   es = data;
   _e_shelf_menu_append(es, mn);
}

static void
_e_shelf_cb_locked_set(void *data, int lock)
{
   E_Shelf *es;

   es = data;
   e_shelf_locked_set(es, lock);
}

static void
_e_shelf_cb_urgent_show(void *data)
{
   E_Shelf *es;

   es = data;
   e_shelf_urgent_show(es);
}

static void
_e_shelf_cb_menu_config(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Shelf *es;

   es = data;
   if (!es->config_dialog) e_int_shelf_config(es);
}

static void
_e_shelf_cb_menu_edit(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Shelf *es;

   es = data;
   if (es->gadcon->editing)
     {
	e_gadcon_edit_end(es->gadcon);
	e_shelf_toggle(es, 0);
     }
   else
     {
	e_shelf_toggle(es, 1);
	e_gadcon_edit_begin(es->gadcon);
     }
}

static void
_e_shelf_cb_menu_contents(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Shelf *es;

   es = data;
   if (!es->gadcon->config_dialog) e_int_gadcon_config_shelf(es->gadcon);
}

static void
_e_shelf_cb_confirm_dialog_destroy(void *data)
{
   E_Shelf *es;

   es = data;
   e_object_unref(E_OBJECT(es));
}

static void
_e_shelf_cb_confirm_dialog_yes(void *data)
{
   E_Config_Shelf *cfg;
   E_Shelf *es;

   es = data;
   cfg = es->cfg;
   if (e_object_is_del(E_OBJECT(es))) return;
   e_object_del(E_OBJECT(es));
   e_config->shelves = eina_list_remove(e_config->shelves, cfg);
   if (cfg->name) eina_stringshare_del(cfg->name);
   if (cfg->style) eina_stringshare_del(cfg->style);
   E_FREE(cfg);

   e_config_save_queue();
}


static void
_e_shelf_cb_menu_delete(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Shelf *es;
   E_Config_Shelf *cfg;

   es = data;
   if (e_config->cnfmdlg_disabled) 
     {
	cfg = es->cfg;
	if (e_object_is_del(E_OBJECT(es))) return;
	e_object_del(E_OBJECT(es));
	e_config->shelves = eina_list_remove(e_config->shelves, cfg);
	if (cfg->name) eina_stringshare_del(cfg->name);
	if (cfg->style) eina_stringshare_del(cfg->style);
	E_FREE(cfg);

	e_config_save_queue();
	return;
     }

   e_object_ref(E_OBJECT(es));
   e_confirm_dialog_show(_("Are you sure you want to delete this shelf?"), "enlightenment",
			 _("You requested to delete this shelf.<br>"
			      "<br>"
			      "Are you sure you want to delete it?"), NULL, NULL,
			 _e_shelf_cb_confirm_dialog_yes, NULL, data, NULL, 
			 _e_shelf_cb_confirm_dialog_destroy, data);
}

static void
_e_shelf_cb_menu_post(void *data, E_Menu *m __UNUSED__)
{
   E_Shelf *es;

   es = data;
   if (!es->menu) return;
   e_object_del(E_OBJECT(es->menu));
   es->menu = NULL;
}

static void
_e_shelf_cb_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   E_Shelf *es;
   E_Menu *mn;
   int cx, cy;

   es = data;
   ev = event_info;
   switch (ev->button)
     {
      case 1:
	if (es->cfg->autohide_show_action) e_shelf_toggle(es, 1);
	break;
      case 3:
	mn = e_menu_new();
	e_menu_post_deactivate_callback_set(mn, _e_shelf_cb_menu_post, es);
	es->menu = mn;

	_e_shelf_menu_pre_cb(es, mn);

        e_gadcon_canvas_zone_geometry_get(es->gadcon, &cx, &cy, NULL, NULL);
	e_menu_activate_mouse(mn,
			      e_util_zone_current_get(e_manager_current_get()),
			      cx + ev->output.x, 
                              cy + ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
	break;
     }
}

static Eina_Bool
_e_shelf_cb_mouse_in(void *data, int type, void *event)
{
   E_Shelf *es;

   es = data;
   if (es->cfg->autohide_show_action) return ECORE_CALLBACK_PASS_ON;

   if (type == E_EVENT_ZONE_EDGE_MOVE)
     {
	E_Event_Zone_Edge *ev;
	int show = 0;

	ev = event;
	if (es->zone != ev->zone) return ECORE_CALLBACK_PASS_ON;
	if (!_e_shelf_on_current_desk(es, ev)) return ECORE_CALLBACK_PASS_ON;

	switch (es->gadcon->orient)
	  {
	   case E_GADCON_ORIENT_FLOAT:
	   case E_GADCON_ORIENT_HORIZ:
	   case E_GADCON_ORIENT_VERT:
	      /* noop */
	      break;
	   case E_GADCON_ORIENT_LEFT:
	   case E_GADCON_ORIENT_CORNER_LT:
	   case E_GADCON_ORIENT_CORNER_LB:
	      if (((ev->edge == E_ZONE_EDGE_LEFT) ||
		   (ev->edge == E_ZONE_EDGE_TOP_LEFT) ||
		   (ev->edge == E_ZONE_EDGE_BOTTOM_LEFT)) &&
		  (ev->y >= es->y) && (ev->y <= (es->y + es->h)))
		show = 1;
	      break;
	   case E_GADCON_ORIENT_RIGHT:
	   case E_GADCON_ORIENT_CORNER_RT:
	   case E_GADCON_ORIENT_CORNER_RB:
	      if (((ev->edge == E_ZONE_EDGE_RIGHT) ||
		   (ev->edge == E_ZONE_EDGE_TOP_RIGHT) ||
		   (ev->edge == E_ZONE_EDGE_BOTTOM_RIGHT)) &&
		  (ev->y >= es->y) && (ev->y <= (es->y + es->h)))
		show = 1;
	      break;
	   case E_GADCON_ORIENT_TOP:
	   case E_GADCON_ORIENT_CORNER_TL:
	   case E_GADCON_ORIENT_CORNER_TR:
	      if (((ev->edge == E_ZONE_EDGE_TOP) ||
		   (ev->edge == E_ZONE_EDGE_TOP_LEFT) ||
		   (ev->edge == E_ZONE_EDGE_TOP_RIGHT)) &&
		  (ev->x >= es->x) && (ev->x <= (es->x + es->w)))
		show = 1;
	      break;
	   case E_GADCON_ORIENT_BOTTOM:
	   case E_GADCON_ORIENT_CORNER_BL:
	   case E_GADCON_ORIENT_CORNER_BR:
	      if (((ev->edge == E_ZONE_EDGE_BOTTOM) ||
		   (ev->edge == E_ZONE_EDGE_BOTTOM_LEFT) ||
		   (ev->edge == E_ZONE_EDGE_BOTTOM_RIGHT)) &&
		  (ev->x >= es->x) && (ev->x <= (es->x + es->w)))
		show = 1;
	      break;
	  }

	if (show)
	  {
	     edje_object_signal_emit(es->o_base, "e,state,focused", "e");
	     e_shelf_toggle(es, 1);
	  }
	else
	  e_shelf_toggle(es, 0);
     }
   else if (type == ECORE_X_EVENT_MOUSE_IN)
     {
	Ecore_X_Event_Mouse_In *ev;

	ev = event;
	if (!es->popup) return ECORE_CALLBACK_PASS_ON;
	if (ev->win == es->popup->evas_win)
	  {
	     edje_object_signal_emit(es->o_base, "e,state,focused", "e");
	     e_shelf_toggle(es, 1);
	  }
     }
   else if (type == ECORE_EVENT_MOUSE_MOVE)
     {
	Ecore_Event_Mouse_Move *ev;

	ev = event;
	if (!es->popup) return ECORE_CALLBACK_PASS_ON;
	if (ev->event_window == es->popup->evas_win)
	  {
	     edje_object_signal_emit(es->o_base, "e,state,focused", "e");
	     e_shelf_toggle(es, 1);
	  }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_shelf_cb_mouse_out(void *data, int type, void *event)
{
   E_Shelf *es;
   Ecore_X_Window win;

   es = data;

   if (type == ECORE_X_EVENT_MOUSE_OUT)
     {
	Ecore_X_Event_Mouse_Out *ev;

	ev = event;

	if (es->popup) win = es->popup->evas_win;
	else win = es->zone->container->event_win;
	if (ev->win != win) return ECORE_CALLBACK_PASS_ON;

	/*
	 * ECORE_X_EVENT_DETAIL_INFERIOR means focus went to children windows
	 * so do not hide shelf on this case (ie: systray base window, or
	 * embedded icons).
	 *
	 * Problem: when child window get mouse out, shelf window will
	 * not get mouse out itself, so it will stay visible and
	 * autohide will fail.
	 */
	if (ev->detail == ECORE_X_EVENT_DETAIL_INFERIOR) return ECORE_CALLBACK_PASS_ON;

	e_shelf_toggle(es, 0);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static int
_e_shelf_cb_id_sort(const void *data1, const void *data2)
{
   const E_Shelf *es1, *es2;

   es1 = data1;
   es2 = data2;
   if ((es1->id) < (es2->id)) return -1;
   else if (es1->id > es2->id) return 1;
   return 0;
}

static Eina_Bool
_e_shelf_cb_hide_animator(void *data)
{
   E_Shelf *es;
   int step, hide_max = 0;

   es = data;
   switch (es->gadcon->orient)
     {
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
      case E_GADCON_ORIENT_BOTTOM:
      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_BR:
        hide_max = es->h - es->hidden_state_size;
        if (es->hide_origin == -1) es->hide_origin = es->y;	 
        break;
      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_CORNER_LB:
      case E_GADCON_ORIENT_CORNER_LT:
      case E_GADCON_ORIENT_RIGHT:
      case E_GADCON_ORIENT_CORNER_RB:
      case E_GADCON_ORIENT_CORNER_RT:
        hide_max = es->w - es->hidden_state_size;
        if (es->hide_origin == -1) es->hide_origin = es->x;
        break;
      case E_GADCON_ORIENT_FLOAT:
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_VERT:
	break;
     }

   step = (hide_max / e_config->framerate) / es->cfg->hide_duration;
   if (!step) step = 1;

   if (es->hidden)
     {
	if (es->hide_step < hide_max)
	  {
	     if (es->hide_step + step > hide_max)
               es->hide_step = hide_max;
	     else
               es->hide_step += step;
	  }
	else goto end;
     }
   else
     {
	if (es->hide_step > 0)
	  {
	     if (es->hide_step < step)
               es->hide_step = 0;
	     else
               es->hide_step -= step;	       
	  }
	else goto end;
     }

   switch (es->gadcon->orient)
     {
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
        e_shelf_move(es, es->x, es->hide_origin - es->hide_step);
        break;
      case E_GADCON_ORIENT_BOTTOM:
      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_BR:
        e_shelf_move(es, es->x, es->hide_origin + es->hide_step);
        break;
      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_CORNER_LB:
      case E_GADCON_ORIENT_CORNER_LT:
        e_shelf_move(es, es->hide_origin - es->hide_step, es->y);
        break;
      case E_GADCON_ORIENT_RIGHT:
      case E_GADCON_ORIENT_CORNER_RB:
      case E_GADCON_ORIENT_CORNER_RT:
        e_shelf_move(es, es->hide_origin + es->hide_step, es->y);
        break;
      case E_GADCON_ORIENT_FLOAT:
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_VERT:
	 break;
     }

   return ECORE_CALLBACK_RENEW;

end:
   es->hide_animator = NULL;
   if (es->interrupted > -1)
     e_shelf_toggle(es, es->interrupted);
   else if (es->urgent_show)
     e_shelf_toggle(es, 0);
   else
     _e_shelf_toggle_border_fix(es);
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_shelf_cb_hide_animator_timer(void *data)
{
   E_Shelf *es;

   es = data;
   if (!es->hide_animator)
     es->hide_animator = ecore_animator_add(_e_shelf_cb_hide_animator, es);
   es->hide_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_shelf_cb_hide_urgent_timer(void *data)
{
   E_Shelf *es;

   es = data;
   es->hide_timer = NULL;
   if (es->urgent_show) e_shelf_toggle(es, 0);
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_shelf_cb_instant_hide_timer(void *data)
{
   E_Shelf *es;

   es = data;
   switch (es->gadcon->orient)
     {
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
        if (es->hidden)
          e_shelf_move(es, es->x, es->y - es->h + es->hidden_state_size);
        else
          e_shelf_move(es, es->x, es->y + es->h - es->hidden_state_size);
        break;
      case E_GADCON_ORIENT_BOTTOM:
      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_BR:
        if (es->hidden)
          e_shelf_move(es, es->x, es->y + es->h - es->hidden_state_size);
        else
          e_shelf_move(es, es->x, es->y - es->h + es->hidden_state_size);
        break;
      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_CORNER_LB:
      case E_GADCON_ORIENT_CORNER_LT:
        if (es->hidden)
          e_shelf_move(es, es->x - es->w + es->hidden_state_size, es->y);
        else
          e_shelf_move(es, es->x + es->w - es->hidden_state_size, es->y);
        break; 
      case E_GADCON_ORIENT_RIGHT:
      case E_GADCON_ORIENT_CORNER_RB:
      case E_GADCON_ORIENT_CORNER_RT:
        if (es->hidden)
          e_shelf_move(es, es->x + es->w - es->hidden_state_size, es->y);
        else
          e_shelf_move(es, es->x - es->w + es->hidden_state_size, es->y);
        break; 
      default:
	 break;
     }
   es->instant_timer = NULL;
   _e_shelf_toggle_border_fix(es);
   return ECORE_CALLBACK_CANCEL;
}

static void 
_e_shelf_menu_pre_cb(void *data, E_Menu *m) 
{
   E_Shelf *es;
   E_Menu_Item *mi;

   es = data;
   e_menu_pre_activate_callback_set(m, NULL, NULL);

   mi = e_menu_item_new(m);
   if (es->gadcon->editing)
     e_menu_item_label_set(mi, _("Stop Moving/Resizing Gadgets"));
   else
     e_menu_item_label_set(mi, _("Begin Moving/Resizing Gadgets"));
   e_util_menu_item_theme_icon_set(mi, "transform-scale");
   e_menu_item_callback_set(mi, _e_shelf_cb_menu_edit, es);

   mi = e_menu_item_new(m);
   e_menu_item_separator_set(mi, 1);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Contents"));
   e_util_menu_item_theme_icon_set(mi, "preferences-desktop-shelf");
   e_menu_item_callback_set(mi, _e_shelf_cb_menu_contents, es);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Settings"));
   e_util_menu_item_theme_icon_set(mi, "configure");
   e_menu_item_callback_set(mi, _e_shelf_cb_menu_config, es);

   mi = e_menu_item_new(m);
   e_menu_item_separator_set(mi, 1);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Delete"));
   e_util_menu_item_theme_icon_set(mi, "list-remove");
   e_menu_item_callback_set(mi, _e_shelf_cb_menu_delete, es);   
}

static void
_e_shelf_gadcon_client_remove(void *data, E_Gadcon_Client *gcc)
{
   E_Shelf *s;
   E_Gadcon *gc;

   s = data;
   gc = s->gadcon;
   e_gadcon_client_config_del(gc->cf, gcc->cf);
   e_gadcon_unpopulate(gc);
   e_gadcon_populate(gc);
   e_config_save_queue();
}

static int
_e_shelf_gadcon_client_add(void *data, const E_Gadcon_Client_Class *cc)
{
   E_Shelf *s;
   E_Gadcon *gc;

   s = data;
   gc = s->gadcon;
   if (!e_gadcon_client_config_new(gc, cc->name)) return 0;
   e_gadcon_unpopulate(gc);
   e_gadcon_populate(gc);
   e_config_save_queue();
   return 1;
}

static const char *
_e_shelf_orient_icon_name_get(E_Shelf * s)
{
   const char * name;

   name = NULL;
   switch (s->cfg->orient) 
     {
      case E_GADCON_ORIENT_LEFT:
	name = "preferences-position-left";
	break;
      case E_GADCON_ORIENT_RIGHT:
	name = "preferences-position-right";
	break;
      case E_GADCON_ORIENT_TOP:
	name = "preferences-position-top";
	break;
      case E_GADCON_ORIENT_BOTTOM:
	name = "preferences-position-bottom";
	break;
      case E_GADCON_ORIENT_CORNER_TL:
	name = "preferences-position-top-left";
	break;
      case E_GADCON_ORIENT_CORNER_TR:
	name = "preferences-position-top-right";
	break;
      case E_GADCON_ORIENT_CORNER_BL:
	name = "preferences-position-bottom-left";
	break;
      case E_GADCON_ORIENT_CORNER_BR:
	name = "preferences-position-bottom-right";
	break;
      case E_GADCON_ORIENT_CORNER_LT:
	name = "preferences-position-left-top";
	break;
      case E_GADCON_ORIENT_CORNER_RT:
	name = "preferences-position-right-top";
	break;
      case E_GADCON_ORIENT_CORNER_LB:
	name = "preferences-position-left-bottom";
	break;
      case E_GADCON_ORIENT_CORNER_RB:
	name = "preferences-position-right-bottom";
	break;
      default:
	name = "preferences-desktop-shelf";
	break;
     }
   return name;
}

static void
_e_shelf_bindings_add(E_Shelf *es)
{
   char buf[1024];

   /* TODO: This might delete edge windows, and then add them again further down. Should prevent this. */
   _e_shelf_bindings_del(es);

   /* Don't need edge binding if we don't hide shelf */
   if ((!es->cfg->autohide) && (!es->cfg->autohide_show_action)) return;

   snprintf(buf, sizeof(buf), "shelf.%d", es->id);
   switch (es->gadcon->orient)
     {
      case E_GADCON_ORIENT_FLOAT:
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_VERT:
	 /* noop */
	 break;
      case E_GADCON_ORIENT_LEFT:
	 e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_LEFT, E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
	 break;
      case E_GADCON_ORIENT_RIGHT:
	 e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_RIGHT, E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
	 break;
      case E_GADCON_ORIENT_TOP:
	 e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_TOP, E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
	 break;
      case E_GADCON_ORIENT_BOTTOM:
	 e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_BOTTOM, E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
	 break;
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_LT:
	 e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_TOP, E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
	 e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_LEFT, E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
	 e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_TOP_LEFT, E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
	 break;
      case E_GADCON_ORIENT_CORNER_TR:
      case E_GADCON_ORIENT_CORNER_RT:
	 e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_TOP, E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
	 e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_RIGHT, E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
	 e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_TOP_RIGHT, E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
	 break;
      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_LB:
	 e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_BOTTOM, E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
	 e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_LEFT, E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
	 e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_BOTTOM_LEFT, E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
	 break;
      case E_GADCON_ORIENT_CORNER_BR:
      case E_GADCON_ORIENT_CORNER_RB:
	 e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_BOTTOM, E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
	 e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_RIGHT, E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
	 e_bindings_edge_add(E_BINDING_CONTEXT_ZONE, E_ZONE_EDGE_BOTTOM_RIGHT, E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
	 break;
     }
}

static void
_e_shelf_bindings_del(E_Shelf *es)
{
   char buf[1024];
   E_Zone_Edge edge;

   snprintf(buf, sizeof(buf), "shelf.%d", es->id);
   for (edge = E_ZONE_EDGE_LEFT; edge <= E_ZONE_EDGE_BOTTOM_LEFT; edge++)
     e_bindings_edge_del(E_BINDING_CONTEXT_ZONE, edge, E_BINDING_MODIFIER_NONE, 1, buf, NULL, 0);
}

static Eina_Bool
_e_shelf_on_current_desk(E_Shelf *es, E_Event_Zone_Edge *ev)
{
   E_Config_Shelf_Desk *sd;
   Eina_List *ll;
   int on_current_desk = 0;
   int on_all_desks = 1;

   EINA_LIST_FOREACH(es->cfg->desk_list, ll, sd)
     {
	if (!sd) continue;
	on_all_desks = 0;
	if ((sd->x == ev->zone->desk_x_current) && (sd->y == ev->zone->desk_y_current))
	  {
	     on_current_desk = 1;
	     break;
	  }
     }
   if (!on_all_desks && !on_current_desk)
     return EINA_FALSE;


   return EINA_TRUE;
}

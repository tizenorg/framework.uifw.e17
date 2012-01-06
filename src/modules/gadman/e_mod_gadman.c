#include <e.h>
#include "e_mod_main.h"
#include "e_mod_gadman.h"
#include "e_mod_config.h"

/* local protos */
static void _attach_menu(void *data, E_Gadcon_Client *gcc, E_Menu *menu);
static void _save_widget_position(E_Gadcon_Client *gcc);
static void _apply_widget_position(E_Gadcon_Client *gcc);
static char *_get_bind_text(const char* action);
static void _on_menu_layer(E_Gadcon_Client *gcc, Gadman_Layer_Type layer);
static E_Gadcon_Client *_gadman_gadget_add(const E_Gadcon_Client_Class *cc, Gadman_Layer_Type layer, E_Config_Gadcon_Client *src_cf);

static Evas_Object* _create_mover(E_Gadcon *gc);
static Evas_Object* _get_mover(E_Gadcon_Client *gcc);
static E_Gadcon* _gadman_gadcon_new(const char* name, Gadman_Layer_Type layer, E_Zone *zone, E_Gadcon_Location * loc);

static void on_shape_change(void *data, E_Container_Shape *es, E_Container_Shape_Change ch);

static void on_top(void *data, Evas_Object *o, const char *em, const char *src);
static void on_right(void *data, Evas_Object *o, const char *em, const char *src);
static void on_down(void *data, Evas_Object *o, const char *em, const char *src);
static void on_left(void *data, Evas_Object *o, const char *em, const char *src);
static void on_move(void *data, Evas_Object *o, const char *em, const char *src);

static void on_frame_click(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void on_bg_click(void *data, Evas_Object *o, const char *em, const char *src);
static void on_hide_stop(void *data __UNUSED__, Evas_Object *o __UNUSED__,
			 const char *em __UNUSED__, const char *src __UNUSED__);

static void on_menu_style_plain(void *data, E_Menu *m, E_Menu_Item *mi);
static void on_menu_style_inset(void *data, E_Menu *m, E_Menu_Item *mi);
static void on_menu_style_float(void *data, E_Menu *m, E_Menu_Item *mi);
static void on_menu_style_horiz(void *data, E_Menu *m, E_Menu_Item *mi);
static void on_menu_style_vert(void *data, E_Menu *m, E_Menu_Item *mi);
static void on_menu_layer_bg(void *data, E_Menu *m, E_Menu_Item *mi);
static void on_menu_layer_top(void *data, E_Menu *m, E_Menu_Item *mi);
static void on_menu_delete(void *data, E_Menu *m, E_Menu_Item *mi);
static void on_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi);
static void on_menu_add(void *data, E_Menu *m, E_Menu_Item *mi);

static int _e_gadman_client_add (void *data __UNUSED__, const E_Gadcon_Client_Class *cc);
static void _e_gadman_client_remove (void *data __UNUSED__, E_Gadcon_Client *gcc);

E_Gadcon_Client *current = NULL;
Manager *Man = NULL;
static E_Gadcon_Location * location = NULL;

/* Implementation */
void
gadman_init(E_Module *m)
{
   const Eina_List *l;
   E_Zone *zone;

   /* Create Manager */
   Man = calloc(1, sizeof(Manager));
   if (!Man) return;

   Man->module = m;
   Man->container = e_container_current_get(e_manager_current_get());
   Man->width = Man->container->w;
   Man->height = Man->container->h;

   /* Check if composite is enable */
   if (ecore_x_screen_is_composited(0) || e_config->use_composite)
     Man->use_composite = 1;

   /* with this we can trap screen resolution change (a better way?)*/
   e_container_shape_change_callback_add(Man->container, on_shape_change, NULL);

   /* create and register "desktop" location */
   location = e_gadcon_location_new ("Desktop", E_GADCON_SITE_DESKTOP, 
				     _e_gadman_client_add, NULL, 
				     _e_gadman_client_remove, NULL);
   e_gadcon_location_set_icon_name(location, "preferences-desktop");
   e_gadcon_location_register(location);

   /* iterating through zones - and making gadmans on each */
   EINA_LIST_FOREACH(Man->container->zones, l, zone)
     {
	const char *layer_name[] = {"gadman", "gadman_top"};
	unsigned int layer;

	for (layer = 0; layer < GADMAN_LAYER_COUNT; layer++)
	  {
	     E_Gadcon *gc;

	     gc = _gadman_gadcon_new(layer_name[layer], layer, zone, location);
	     Man->gadcons[layer] = eina_list_append(Man->gadcons[layer], gc);
	  }
     }
}

void
gadman_shutdown(void)
{
   E_Gadcon *gc;
   unsigned int layer;

   e_gadcon_location_unregister(location);
   e_container_shape_change_callback_del(Man->container, on_shape_change, NULL);

   for (layer = 0; layer < GADMAN_LAYER_COUNT; layer++)
     {
	EINA_LIST_FREE(Man->gadcons[layer], gc)
	  {
	     e_gadcon_unpopulate(gc);
	     e_gadcon_custom_del(gc);

	     /* free gadcons */
	     e_config->gadcons = eina_list_remove(e_config->gadcons, gc);
	     eina_stringshare_del(gc->name);

	     if (gc->config_dialog) e_object_del(E_OBJECT(gc->config_dialog));
	  }

	evas_object_del(Man->movers[layer]);
     }

   if (Man->icon_name) eina_stringshare_del(Man->icon_name);

   /* free manager */
   if (Man->top_ee)
     {
        e_canvas_del(Man->top_ee);
        //ecore_evas_free(Man->top_ee);
     }
   free(Man);
   Man = NULL;
}

void
gadman_populate_class(void *data, E_Gadcon *gc, const E_Gadcon_Client_Class *cc)
{
   Gadman_Layer_Type layer = (Gadman_Layer_Type)(long)data;
   const Eina_List *l, *ll;
   E_Config_Gadcon_Client *cf_gcc;
   E_Gadcon_Client *gcc = NULL;

   EINA_LIST_FOREACH(gc->cf->clients, l, cf_gcc)
     {
        if (cf_gcc->name && cc->name && !strcmp(cf_gcc->name, cc->name) && (gc->cf->zone == gc->zone->id))
	  {
	     EINA_LIST_FOREACH(Man->gadgets[layer], ll, gcc) 
	       {
		  if ((gcc->cf) && (gcc->cf->id) && (cf_gcc->id))
		    if (gcc->cf->id == cf_gcc->id) break;
	       }
	     if (!gcc) gadman_gadget_place(cf_gcc, layer, gc->zone);
	  }
     }
}

E_Gadcon *
gadman_gadcon_get(const E_Zone *zone, Gadman_Layer_Type layer)
{
   const Eina_List *l;
   E_Gadcon *gc;

   EINA_LIST_FOREACH(Man->gadcons[layer], l, gc)
     if (gc->zone == zone) return gc;
   return NULL;
}

E_Gadcon_Client *
gadman_gadget_place(E_Config_Gadcon_Client *cf, Gadman_Layer_Type layer, E_Zone *zone)
{
   const Eina_List *l;
   E_Gadcon *gc;
   E_Gadcon_Client *gcc;
   E_Gadcon_Client_Class *cc = NULL;

   if (!cf->name) return NULL;

   gc = gadman_gadcon_get(zone, layer);

   /* Find provider */
   EINA_LIST_FOREACH(e_gadcon_provider_list(), l, cc)
     {
        if (!strcmp(cc->name, cf->name))
          break;
        else
          cc = NULL;
     }
   if (!cc) return NULL;

   /* init Gadcon_Client */
   gcc = cc->func.init(gc, cf->name, cf->id, cc->default_style);
   if (!gcc) return NULL;
   e_object_del_func_set(E_OBJECT(gcc), E_OBJECT_CLEANUP_FUNC(gadman_gadget_del));
   gcc->cf = cf;
   gcc->client_class = cc;

   Man->gadgets[layer] = eina_list_append(Man->gadgets[layer], gcc);

   //printf("Place Gadget %s (style: %s id: %s) (gadcon: %s)\n", gcc->name, cf->style, cf->id, gc->name);

   /* create frame */
   gcc->o_frame = edje_object_add(gc->evas);
   e_theme_edje_object_set(gcc->o_frame, "base/theme/gadman", "e/gadman/frame");

   if ((cf->style) && (!strcmp(cf->style, E_GADCON_CLIENT_STYLE_INSET)))
     edje_object_signal_emit(gcc->o_frame, "e,state,visibility,inset", "e");
   else
     edje_object_signal_emit(gcc->o_frame, "e,state,visibility,plain", "e");

   /* swallow the client inside the frame */
   edje_object_part_swallow(gcc->o_frame, "e.swallow.content", gcc->o_base);
   evas_object_event_callback_add(gcc->o_frame, EVAS_CALLBACK_MOUSE_DOWN, 
                                  on_frame_click, gcc);

   /* Call the client orientation function */
   if (cc->func.orient)
     cc->func.orient(gcc, gcc->cf->orient);

   _apply_widget_position(gcc);

   if (gcc->gadcon->id == ID_GADMAN_LAYER_TOP)
     edje_object_signal_emit(gcc->o_frame, "e,state,visibility,hide", "e");

   evas_object_show(gcc->o_frame);

   return gcc;
}

E_Gadcon_Client *
gadman_gadget_add(const E_Gadcon_Client_Class *cc, Gadman_Layer_Type layer)
{
  return _gadman_gadget_add(cc, layer, NULL);
}

static E_Gadcon_Client *
_gadman_gadget_add(const E_Gadcon_Client_Class *cc, Gadman_Layer_Type layer, E_Config_Gadcon_Client *src_cf)
{
   E_Config_Gadcon_Client *cf = NULL;
   E_Gadcon_Client *gcc = NULL;
   E_Gadcon *gc;
   int w, h;

   gc = gadman_gadcon_get(e_util_zone_current_get(e_manager_current_get()), 
			  layer);

   /* Create Config_Gadcon_Client */
   cf = e_gadcon_client_config_new(gc, cc->name);
   if (cf) 
     {
       if (!src_cf)
         {
           cf->style = eina_stringshare_add(cc->default_style);
           cf->geom.pos_x = DEFAULT_POS_X;
           cf->geom.pos_y = DEFAULT_POS_Y;
           cf->geom.size_w = DEFAULT_SIZE_W;
           cf->geom.size_h = DEFAULT_SIZE_H;
         }
       else
         {
           cf->style = strdup(src_cf->style);
           cf->geom.pos_x = src_cf->geom.pos_x;
           cf->geom.pos_y = src_cf->geom.pos_y;
           cf->geom.size_w = src_cf->geom.size_w;
           cf->geom.size_h = src_cf->geom.size_h;
         }
     }

   /* Place the new gadget */
   if (cf)
     gcc = gadman_gadget_place(cf, layer, gc->zone);
   if (!gcc) return NULL;

   /* Respect Aspect */
   evas_object_geometry_get(gcc->o_frame, NULL, NULL, &w, &h);
   if (gcc->aspect.w && gcc->aspect.h)
     {
	if (gcc->aspect.w > gcc->aspect.h)
	  w = ((float)h / (float)gcc->aspect.h) * (gcc->aspect.w);
	else
	  h =  ((float)w / (float)gcc->aspect.w) * (gcc->aspect.h);
	if (h < gcc->min.h) h =  gcc->min.h;
	if (w < gcc->min.w) w =  gcc->min.w;
	evas_object_resize(gcc->o_frame, w, h);
     }

   return gcc;
}

void
gadman_gadget_del(E_Gadcon_Client *gcc)
{
   Gadman_Layer_Type layer = gcc->gadcon->id - ID_GADMAN_LAYER_BASE;
   Man->gadgets[layer] = eina_list_remove(Man->gadgets[layer], gcc);

//   edje_object_part_unswallow(gcc->o_frame, gcc->o_base);

   if (gcc->cf) e_gadcon_client_config_del(gcc->gadcon->cf, gcc->cf);
   gcc->cf = NULL;
   e_object_del(E_OBJECT(gcc));
   current = NULL;
}

void
gadman_gadget_edit_start(E_Gadcon_Client *gcc)
{
   E_Gadcon *gc;
   Evas_Object *mover;
   int x, y, w, h;

   current = gcc;

   gc = gcc->gadcon;
   gc->editing = 1;

   /* Move/resize the correct mover */
   evas_object_geometry_get(gcc->o_frame, &x, &y, &w, &h);
   mover = _get_mover(gcc);

   evas_object_move(mover, x, y);
   evas_object_resize(mover, w, h);
   evas_object_raise(mover);
   evas_object_show(mover);
}

void
gadman_gadget_edit_end(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   unsigned int layer;

   for (layer = 0; layer < GADMAN_LAYER_COUNT; layer++)
     {
	const Eina_List *l;
	E_Gadcon *gc;

	evas_object_hide(Man->movers[layer]);

	EINA_LIST_FOREACH(Man->gadcons[layer], l, gc)
	  gc->editing = 0;
     }

   if (current) _save_widget_position(current);
}

void
gadman_gadgets_show(void)
{
   const Eina_List *l;
   E_Gadcon_Client *gcc;

   Man->visible = 1;
   ecore_evas_show(Man->top_ee);
   if (Man->conf->bg_type == BG_STD)
     {
	if (Man->conf->anim_bg)
	  edje_object_signal_emit(Man->full_bg,
	                         "e,state,visibility,show", "e");
	else
	  edje_object_signal_emit(Man->full_bg,
	                          "e,state,visibility,show,now", "e");
     }
   else
     {
	if (Man->conf->anim_bg)
	  edje_object_signal_emit(Man->full_bg,
	                         "e,state,visibility,show,custom", "e");
	else
	  edje_object_signal_emit(Man->full_bg,
	                         "e,state,visibility,show,custom,now", "e");
     }

   /* Showing top gadgets */
   EINA_LIST_FOREACH(Man->gadgets[GADMAN_LAYER_TOP], l, gcc)
     {
	if (Man->conf->anim_gad)
	  edje_object_signal_emit(gcc->o_frame,
	                          "e,state,visibility,show", "e");
	else
	  edje_object_signal_emit(gcc->o_frame,
	                          "e,state,visibility,show,now", "e");
     }
}

void
gadman_gadgets_hide(void)
{
   const Eina_List *l;
   E_Gadcon_Client *gcc;

   Man->visible = 0;

   if (Man->conf->bg_type == BG_STD)
     {
	if (Man->conf->anim_bg)
	     edje_object_signal_emit(Man->full_bg,
	                             "e,state,visibility,hide", "e");
	else
	     edje_object_signal_emit(Man->full_bg,
	                             "e,state,visibility,hide", "e");
     }
   else
     {
	if (Man->conf->anim_bg)
	  edje_object_signal_emit(Man->full_bg,
	                          "e,state,visibility,hide,custom", "e");
	else
	  edje_object_signal_emit(Man->full_bg,
	                          "e,state,visibility,hide,custom,now", "e");
     }

   /* Hiding top gadgets */
   EINA_LIST_FOREACH(Man->gadgets[GADMAN_LAYER_TOP], l, gcc)
     {
	if (Man->conf->anim_gad)
	  edje_object_signal_emit(gcc->o_frame,
	                          "e,state,visibility,hide", "e");
	else
	  edje_object_signal_emit(gcc->o_frame,
	                          "e,state,visibility,hide,now", "e");
     }
}

void
gadman_gadgets_toggle(void)
{
   if (Man->visible)
     gadman_gadgets_hide();
   else
     gadman_gadgets_show();
}

void
gadman_update_bg(void)
{
   Evas_Object *obj;
   const char *ext;

   obj = edje_object_part_swallow_get(Man->full_bg, "e.swallow.bg");
   if (obj)
     {
	edje_object_part_unswallow(Man->full_bg, obj);
	evas_object_del(obj);
     }

   switch (Man->conf->bg_type)
     {
	case BG_STD:
	case BG_TRANS:
	  break;
	case BG_COLOR:
	  obj = evas_object_rectangle_add(Man->gc_top->evas);
	  evas_object_color_set(obj, Man->conf->color_r, Man->conf->color_g,
	                        Man->conf->color_b, 200);
	  edje_object_part_swallow(Man->full_bg, "e.swallow.bg", obj);
	  break;
	case BG_CUSTOM:
	  ext = strrchr(Man->conf->custom_bg, '.');
	  if (!strcmp(ext, ".edj") || !strcmp(ext, ".EDJ"))
	    {
		//THIS IS FOR E17 backgrounds
		obj = edje_object_add(Man->gc_top->evas);
		edje_object_file_set(obj, Man->conf->custom_bg,
		                     "e/desktop/background");
	    }
	  else
	    {
		//THIS IS FOR A NORMAL IMAGE
		obj = evas_object_image_add(Man->gc_top->evas);
		evas_object_image_file_set(obj, Man->conf->custom_bg, NULL);
		evas_object_image_fill_set(obj, 0, 0, Man->container->w,
		                           Man->container->h);
	    }
	  edje_object_part_swallow(Man->full_bg, "e.swallow.bg", obj);
	  break;
	default:
	  break;
     }
}

/* Internals */
static E_Gadcon*
_gadman_gadcon_new(const char* name, Gadman_Layer_Type layer, E_Zone *zone, E_Gadcon_Location * loc)
{
   const Eina_List *l;
   E_Gadcon *gc;
   E_Config_Gadcon *cg;

   /* Create Gadcon */
   gc = E_OBJECT_ALLOC(E_Gadcon, E_GADCON_TYPE, NULL);
   if (!gc) return NULL;

   gc->name = eina_stringshare_add(name);
   gc->layout_policy = E_GADCON_LAYOUT_POLICY_PANEL;
   gc->orient = E_GADCON_ORIENT_FLOAT;
   gc->location = loc;

   /* Create ecore fullscreen window */
   if (layer > GADMAN_LAYER_BG)
     {
        if (!Man->top_ee)
	  {
	     Man->top_ee = e_canvas_new(e_config->evas_engine_popups,
					Man->container->win, 0, 0, 0, 0, 1, 1,
					&(Man->top_win));
	  }

        if (Man->use_composite)
          {
             ecore_evas_alpha_set(Man->top_ee, 1);
             Man->top_win = ecore_evas_software_x11_window_get(Man->top_ee);
             ecore_x_window_shape_rectangle_set(Man->top_win, 0, 0,
                                                Man->width, Man->height);
          }
        else
          ecore_evas_shaped_set(Man->top_ee, 1);

        e_canvas_add(Man->top_ee); //??
        e_container_window_raise(Man->container, Man->top_win, 250);

        ecore_evas_move_resize(Man->top_ee, 0, 0, Man->width, Man->height);
        ecore_evas_hide(Man->top_ee);

        gc->evas = ecore_evas_get(Man->top_ee);
        e_gadcon_ecore_evas_set(gc, Man->top_ee);

        /* create full background object */
        Man->full_bg = edje_object_add(gc->evas);
        e_theme_edje_object_set(Man->full_bg, "base/theme/gadman",
                                "e/gadman/full_bg");
        edje_object_signal_callback_add(Man->full_bg, "mouse,down,*",
                                        "grabber", on_bg_click, NULL);
        edje_object_signal_callback_add(Man->full_bg, "e,action,hide,stop",
                                        "", on_hide_stop, NULL);
        
        evas_object_move(Man->full_bg, 0, 0);
        evas_object_resize(Man->full_bg, Man->width, Man->height);
        evas_object_show(Man->full_bg);
     }
   /* ... or use the e background window */
   else
     {
        gc->evas = Man->container->bg_evas;
        e_gadcon_ecore_evas_set(gc, Man->container->bg_ecore_evas);
        e_gadcon_xdnd_window_set(gc, Man->container->bg_win);
        e_gadcon_dnd_window_set(gc, Man->container->event_win);
        e_drop_xdnd_register_set(Man->container->bg_win, 1);
     }

   e_gadcon_zone_set(gc, zone);
   e_gadcon_util_menu_attach_func_set(gc, _attach_menu, NULL);
   e_gadcon_populate_callback_set(gc, gadman_populate_class, (void*)layer);

   gc->id = ID_GADMAN_LAYER_BASE + layer;
   gc->edje.o_parent = NULL;
   gc->edje.swallow_name = NULL;
   gc->shelf = NULL;
   gc->toolbar = NULL;
   gc->editing = 0;
   gc->o_container = NULL;
   gc->frame_request.func = NULL;
   gc->resize_request.func = NULL;
   gc->min_size_request.func = NULL;

   /* Search for existing gadcon config */
   gc->cf = NULL;
   EINA_LIST_FOREACH(e_config->gadcons, l, cg)
     {
        if ((!strcmp(cg->name, name)) && (cg->zone == zone->id))
          {
             gc->cf = cg;
             break;
          }
     }

   /* ... or create a new one */
   if (!gc->cf)
     {
        gc->cf = E_NEW(E_Config_Gadcon, 1);
        gc->cf->name = eina_stringshare_add(name);
        gc->cf->id = gc->id;
        gc->cf->zone = zone->id;
        gc->cf->clients = NULL;
        e_config->gadcons = eina_list_append(e_config->gadcons, gc->cf);
        e_config_save_queue();
     }

   e_gadcon_custom_new(gc);

   if (!Man->movers[layer])
     Man->movers[layer] = _create_mover(gc);

   /* Assigning top gadcon - needed in gadman_update_bg() */
   if ((layer > GADMAN_LAYER_BG) && !Man->gc_top)
     Man->gc_top = gc;

   return gc;
}

static Evas_Object *
_create_mover(E_Gadcon *gc)
{
   Evas_Object *mover;

   /* create mover object */
   mover = edje_object_add(gc->evas);
   e_theme_edje_object_set(mover, "base/theme/gadman", "e/gadman/control");

   edje_object_signal_callback_add(mover, "e,action,move,start", "",
                                   on_move, (void*)DRAG_START);
   edje_object_signal_callback_add(mover, "e,action,move,stop", "",
                                   on_move, (void*)DRAG_STOP);
   edje_object_signal_callback_add(mover, "e,action,move,go", "",
                                   on_move, (void*)DRAG_MOVE);
   edje_object_signal_callback_add(mover, "mouse,down,3", "overlay",
                                   gadman_gadget_edit_end, NULL);

   edje_object_signal_callback_add(mover, "e,action,resize,left,start", "",
                                   on_left, (void*)DRAG_START);
   edje_object_signal_callback_add(mover, "e,action,resize,left,stop", "",
                                   on_left, (void*)DRAG_STOP);
   edje_object_signal_callback_add(mover, "e,action,resize,left,go", "",
                                   on_left, (void*)DRAG_MOVE);
   edje_object_signal_callback_add(mover, "e,action,resize,down,start", "",
                                   on_down, (void*)DRAG_START);
   edje_object_signal_callback_add(mover, "e,action,resize,down,stop", "",
                                   on_down, (void*)DRAG_STOP);
   edje_object_signal_callback_add(mover, "e,action,resize,down,go", "",
                                   on_down, (void*)DRAG_MOVE);
   edje_object_signal_callback_add(mover, "e,action,resize,right,start", "",
                                   on_right, (void*)DRAG_START);
   edje_object_signal_callback_add(mover, "e,action,resize,right,stop", "",
                                   on_right, (void*)DRAG_STOP);
   edje_object_signal_callback_add(mover, "e,action,resize,right,go", "",
                                   on_right, (void*)DRAG_MOVE);
   edje_object_signal_callback_add(mover, "e,action,resize,up,start", "",
                                   on_top, (void*)DRAG_START);
   edje_object_signal_callback_add(mover, "e,action,resize,up,stop", "",
                                   on_top, (void*)DRAG_STOP);
   edje_object_signal_callback_add(mover, "e,action,resize,up,go", "",
                                   on_top, (void*)DRAG_MOVE);

   evas_object_move(mover, 20, 30);
   evas_object_resize(mover, 100, 100);
   evas_object_hide(mover);

   return mover;
}

static Evas_Object *
_get_mover(E_Gadcon_Client *gcc)
{
   return Man->movers[gcc->gadcon->id - ID_GADMAN_LAYER_BASE];
}

static void
_save_widget_position(E_Gadcon_Client *gcc)
{
   int x, y, w, h;

   evas_object_geometry_get(gcc->o_frame, &x, &y, &w, &h);
   current->cf->geom.pos_x = (double)x / (double)Man->width;
   current->cf->geom.pos_y = (double)y / (double)Man->height;
   current->cf->geom.size_w = (double)w / (double)Man->width;;
   current->cf->geom.size_h = (double)h / (double)Man->height;

   e_config_save_queue();
}

static void
_apply_widget_position(E_Gadcon_Client *gcc)
{
   int x, y, w, h;
   E_Zone *zone;

   x = gcc->cf->geom.pos_x * Man->width;
   y = gcc->cf->geom.pos_y * Man->height;
   w = gcc->cf->geom.size_w * Man->width;
   h = gcc->cf->geom.size_h * Man->height;

   /* Obtain zone from parent gadcon */
   zone = gcc->gadcon->zone;

   /* Respect min sizes */
   if (h < gcc->min.h) h = gcc->min.h;
   if (w < gcc->min.w) w = gcc->min.w;
   if (h < 1) h = 100;
   if (w < 1) w = 100;

   /* Respect zone marigin */
   if (x < zone->x) x = zone->x;
   if (y < zone->y) y = zone->y;
   if (x > (zone->x + zone->w)) x = zone->x;
   if (y > (zone->y + zone->h)) y = zone->y;

   if ((y + h) > (zone->y + zone->h + MIN_VISIBLE_MARIGIN)) 
     h = ((zone->y + zone->h + MIN_VISIBLE_MARIGIN) - y);
   if ((x + w) > (zone->x + zone->w + MIN_VISIBLE_MARIGIN)) 
     w = ((zone->x + zone->w + MIN_VISIBLE_MARIGIN) - x);

   evas_object_move(gcc->o_frame, x, y);
   evas_object_resize(gcc->o_frame, w, h);
}

static void
_attach_menu(void *data __UNUSED__, E_Gadcon_Client *gcc, E_Menu *menu)
{
   E_Menu *mn;
   E_Menu_Item *mi;
   char buf[128];
   char *key;

   //printf("Attach menu (gcc: %x id: %s) [%s]\n", gcc, gcc->cf->id, gcc->cf->style);
   if (!gcc) return;

   if (e_menu_item_nth(menu, 0))
     {
        mi = e_menu_item_new(menu);
        e_menu_item_separator_set(mi, 1);
     }

   /* Move / resize*/
   mi = e_menu_item_new(menu);
   e_menu_item_label_set(mi, _("Begin move/resize"));
   e_util_menu_item_theme_icon_set(mi, "transform-scale");
   e_menu_item_callback_set(mi, on_menu_edit, gcc);


   /* plain / inset */
   if (!gcc->cf->style)
     gcc->cf->style = eina_stringshare_add(E_GADCON_CLIENT_STYLE_INSET);

   mn = e_menu_new();
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Plain"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (!strcmp(gcc->cf->style, E_GADCON_CLIENT_STYLE_PLAIN))
     e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, on_menu_style_plain, gcc);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Inset"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (!strcmp(gcc->cf->style, E_GADCON_CLIENT_STYLE_INSET))
     e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, on_menu_style_inset, gcc);

   mi = e_menu_item_new(mn);
   e_menu_item_separator_set(mi, 1);

   /* orient */
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Free"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (gcc->cf->orient == E_GADCON_ORIENT_FLOAT)
     e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, on_menu_style_float, gcc);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Horizontal"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (gcc->cf->orient == E_GADCON_ORIENT_HORIZ)
     e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, on_menu_style_horiz, gcc);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Vertical"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (gcc->cf->orient == E_GADCON_ORIENT_VERT)
     e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, on_menu_style_vert, gcc);

   mi = e_menu_item_new(menu);
   e_menu_item_label_set(mi, _("Appearance"));
   e_util_menu_item_theme_icon_set(mi, "preferences-look");
   e_menu_item_submenu_set(mi, mn);
   e_object_del(E_OBJECT(mn));

   /* bg / ontop */
   mn = e_menu_new();
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Always on desktop"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (gcc->gadcon->id == ID_GADMAN_LAYER_BG)
     e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, on_menu_layer_bg, gcc);

   mi = e_menu_item_new(mn);
   key = _get_bind_text("gadman_toggle");
   snprintf(buf, sizeof(buf), "%s %s",
            _("On top pressing"), key);
   free(key);
   e_menu_item_label_set(mi, buf);
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (gcc->gadcon->id == ID_GADMAN_LAYER_TOP)
     e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, on_menu_layer_top, gcc);

   mi = e_menu_item_new(menu);
   e_menu_item_label_set(mi, _("Behavior"));
   e_util_menu_item_theme_icon_set(mi, "preferences-look");
   e_menu_item_submenu_set(mi, mn);
   e_object_del(E_OBJECT(mn));

   mi = e_menu_item_new(menu);
   e_menu_item_separator_set(mi, 1);

   e_gadcon_client_add_location_menu(gcc, menu);

   /* Remove this gadgets */
   mi = e_menu_item_new(menu);
   e_menu_item_label_set(mi, _("Remove"));
   e_util_menu_item_theme_icon_set(mi, "list-remove");
   e_menu_item_callback_set(mi, on_menu_delete, gcc);

   /* Add other gadgets */
   mi = e_menu_item_new(menu);
   e_menu_item_separator_set(mi, 1);

   mi = e_menu_item_new(menu);
   e_menu_item_label_set(mi, _("Add other gadgets"));
   e_util_menu_item_theme_icon_set(mi, "list-add");
   e_menu_item_callback_set(mi, on_menu_add, gcc);
}

static char *
_get_bind_text(const char* action)
{
   E_Binding_Key *bind;
   char b[256] = "";

   bind = e_bindings_key_get(action);   
   if ((bind) && (bind->key))
     {
        if ((bind->mod) & (E_BINDING_MODIFIER_CTRL))
          strcat(b, _("CTRL"));

        if ((bind->mod) & (E_BINDING_MODIFIER_ALT))
          {
             if (b[0]) strcat(b, " + ");
             strcat(b, _("ALT"));
          }

        if ((bind->mod) & (E_BINDING_MODIFIER_SHIFT))
          {
             if (b[0]) strcat(b, " + ");
             strcat(b, _("SHIFT"));
          }

        if ((bind->mod) & (E_BINDING_MODIFIER_WIN))
          {
             if (b[0]) strcat(b, " + ");
             strcat(b, _("WIN"));
          }

        if ((bind->key) && (bind->key[0]))
          {
             char *l;

             if (b[0]) strcat(b, " + ");
             l = strdup(bind->key);
             l[0] = (char)toupper(bind->key[0]);
             strcat(b, l);
             free(l);
          }
        return strdup(b);
     }
   return "(You must define a binding)";
}

/* Callbacks */
static void
on_shape_change(void *data __UNUSED__, E_Container_Shape *es, E_Container_Shape_Change ch __UNUSED__)
{
   const Eina_List *l, *g;
   E_Gadcon *gc;
   E_Config_Gadcon_Client *cf_gcc;
   E_Container  *con;
   unsigned int layer;

   con = e_container_shape_container_get(es);
   if ((con->w == Man->width) && (con->h == Man->height)) return;

   /* The screen size is changed */
   Man->width = con->w;
   Man->height = con->h;

   /* ReStart gadgets */

   for (layer = 0; layer < GADMAN_LAYER_COUNT; layer++)
     {
	EINA_LIST_FOREACH(Man->gadcons[layer], g, gc)
	  {
	     e_gadcon_unpopulate(gc);
	     EINA_LIST_FOREACH(gc->cf->clients, l, cf_gcc)
	       gadman_gadget_place(cf_gcc, layer, gc->zone);
	  }
     }
}

static void
on_menu_style_plain(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Gadcon_Client *gcc;

   gcc = current;
   if (gcc->style) eina_stringshare_del(gcc->style);
   gcc->style = eina_stringshare_add(E_GADCON_CLIENT_STYLE_PLAIN);

   if (gcc->cf->style) eina_stringshare_del(gcc->cf->style);
   gcc->cf->style = eina_stringshare_add(E_GADCON_CLIENT_STYLE_PLAIN);

   edje_object_signal_emit(gcc->o_frame, "e,state,visibility,plain", "e");

   e_config_save_queue();
}

static void
on_menu_style_inset(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Gadcon_Client *gcc;

   gcc = current;

   if (gcc->style) eina_stringshare_del(gcc->style);
   gcc->style = eina_stringshare_add(E_GADCON_CLIENT_STYLE_INSET);

   if (gcc->cf->style) eina_stringshare_del(gcc->cf->style);
   gcc->cf->style = eina_stringshare_add(E_GADCON_CLIENT_STYLE_INSET);

   edje_object_signal_emit(gcc->o_frame, "e,state,visibility,inset", "e");

   e_config_save_queue();
}

static void
_menu_style_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   int w, h;
   gcc->cf->orient = orient;

   if (gcc->client_class->func.orient)
       gcc->client_class->func.orient(gcc, orient);

   if (orient == E_GADCON_ORIENT_VERT)
   {
      w = DEFAULT_SIZE_W * Man->width;
      if (w < gcc->min.w) w = gcc->min.w;
      
      if (gcc->aspect.w && gcc->aspect.h)
         h = ((float)gcc->aspect.h / (float)gcc->aspect.w) * w;
      else
      {
         h = DEFAULT_SIZE_H * Man->height;
         if (h < gcc->min.h) h = gcc->min.h;
      }
   }
   else
   {
      h = DEFAULT_SIZE_H * Man->height;
      if (h < gcc->min.h) h = gcc->min.h;
      
      if (gcc->aspect.w && gcc->aspect.h)
         w = ((float)gcc->aspect.w / (float)gcc->aspect.h) * h;
      else
      {
         w = DEFAULT_SIZE_W * Man->width;
         if (w < gcc->min.w) w = gcc->min.w;
      }
   }

   evas_object_resize(gcc->o_frame, w, h);
   _save_widget_position(gcc);
}

static void
on_menu_style_float(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   _menu_style_orient(data, E_GADCON_ORIENT_FLOAT);
}

static void
on_menu_style_horiz(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   _menu_style_orient(data, E_GADCON_ORIENT_HORIZ);
}

static void
on_menu_style_vert(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   _menu_style_orient(data, E_GADCON_ORIENT_VERT);
}

static void 
_on_menu_layer(E_Gadcon_Client *gcc, Gadman_Layer_Type layer)
{
  const E_Gadcon_Client_Class *cc;  

  E_Gadcon_Client *new_gcc;
  E_Config_Gadcon_Client *cf;

  cc = gcc->client_class;
  cf = gcc->cf;
  
  new_gcc = _gadman_gadget_add(cc, layer, cf);
  gadman_gadget_del(gcc);
  current = new_gcc;
  
  e_config_save_queue();
}

static void
on_menu_layer_bg(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
  _on_menu_layer(data, GADMAN_LAYER_BG);
}

static void
on_menu_layer_top(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
  _on_menu_layer(data, GADMAN_LAYER_TOP);
  gadman_gadgets_show();
}

static void
on_menu_edit(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   gadman_gadget_edit_start(data);
}

static void
on_menu_add(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   if (Man->visible)
     gadman_gadgets_hide();
   e_configure_registry_call("extensions/gadman", m->zone->container, NULL);
}

static void
on_menu_delete(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   gadman_gadget_del(data);
   e_config_save_queue();
}

static void
on_frame_click(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   E_Gadcon_Client *gcc;

   ev = event_info;

   gcc = data;

   if (gcc->gadcon->editing) gadman_gadget_edit_end(NULL, NULL, NULL, NULL);

   current = gcc;

   if (ev->button == 5)
     {
	E_Menu *m;
	int cx, cy, cw, ch;

	m = e_menu_new();
	m = e_gadcon_client_util_menu_items_append(gcc, m, 0);
	gcc->menu = m;
	e_gadcon_canvas_zone_geometry_get(gcc->gadcon, &cx, &cy, &cw, &ch);
	e_menu_activate_mouse(m,
			      e_util_zone_current_get(e_manager_current_get()),
			      cx + ev->output.x, cy + ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
     }
}

static void
on_top(void *data, Evas_Object *o __UNUSED__, const char *em __UNUSED__, const char *src __UNUSED__)
{
   static int ox, oy, ow, oh; //Object coord
   static int dy;             //Mouse offset
   int mx, my;                //Mouse coord
   int action = (int)(long)data;
   Evas_Object *mover;

   mover = _get_mover(current);

   if (action == DRAG_START)
     {
	current->resizing = 1;
        evas_pointer_output_xy_get(current->gadcon->evas, &mx, &my);
        evas_object_geometry_get(mover, &ox, &oy, &ow, &oh);
	dy = my - oy;
     }
   else if (action == DRAG_STOP)
     {
	current->resizing = 0;
	dy = 0;
        _save_widget_position(current);
     }
   else if ((action == DRAG_MOVE) && current->resizing)
     {
        int h;

        evas_pointer_output_xy_get(current->gadcon->evas, &mx, &my);

        h = oy + oh + dy - my;

        if (h < current->min.h)
	  {
	     my -=current->min.h - h;
	     h = current->min.h;
	  }
        /* don't go out of the screen */
	if (my < dy)
	  {
	     h += my - dy;
	     my = dy;
	  }

        evas_object_resize(mover, ow, h);
        evas_object_move(mover, ox, my - dy);

        evas_object_resize(current->o_frame, ow, h);
        evas_object_move(current->o_frame, ox, my - dy);
     }
}

static void
on_right(void *data, Evas_Object *o __UNUSED__, const char *em __UNUSED__, const char *src __UNUSED__)
{
   Evas_Object *mover;
   static int ox, oy, ow, oh; //Object coord
   static int dx;             //Mouse offset
   int mx, my;                //Mouse coord
   int action;

   mover = _get_mover(current);

   action = (int)(long)data;
   if (action == DRAG_START)
     {
	current->resizing = 1;
        evas_pointer_output_xy_get(current->gadcon->evas, &mx, &my);
        evas_object_geometry_get(mover, &ox, &oy, &ow, &oh);
	dx = mx - ow;
     }
   else if (action == DRAG_STOP)
     {
	current->resizing = 0;
	dx = 0;
        _save_widget_position(current);
     }
   else if ((action == DRAG_MOVE) && current->resizing)
     {
        int w;

        evas_pointer_output_xy_get(current->gadcon->evas, &mx, &my);
        w = mx - dx;

        if (w < current->min.w) w = current->min.w;
        /* don't go out of the screen */
	if (w > (Man->width - ox)) w = Man->width - ox;

        evas_object_resize(mover, w, oh);
        evas_object_resize(current->o_frame, w, oh);
     }
}

static void
on_down(void *data, Evas_Object *o __UNUSED__, const char *em __UNUSED__, const char *src __UNUSED__)
{
   Evas_Object *mover;
   static int ox, oy, ow, oh; //Object coord
   static int dy;             //Mouse offset
   int mx, my;                //Mouse coord
   int action;

   action = (int)(long)data;
   mover = _get_mover(current);

   if (action == DRAG_START)
     {
	current->resizing = 1;
        evas_pointer_output_xy_get(current->gadcon->evas, &mx, &my);
        evas_object_geometry_get(mover, &ox, &oy, &ow, &oh);
	dy = my - oh;
     }
   else if (action == DRAG_STOP)
     {
	current->resizing = 0;
	dy = 0;
        _save_widget_position(current);
     }
   else if ((action == DRAG_MOVE) && current->resizing)
     {
        int h;

        evas_pointer_output_xy_get(current->gadcon->evas, &mx, &my);
        h = my - dy;

        if (h < current->min.h) h = current->min.h;
        /* don't go out of the screen */
	if (h > (Man->height - oy)) h = Man->height - oy;

        evas_object_resize(mover, ow, h);
        evas_object_resize(current->o_frame, ow, h);
     }
}

static void
on_left(void *data, Evas_Object *o __UNUSED__, const char *em __UNUSED__, const char *src __UNUSED__)
{
   Evas_Object *mover;
   static int ox, oy, ow, oh; //Object coord
   static int dx;             //Mouse offset
   int mx, my;                //Mouse coord
   int action;

   action = (int)(long)data;
   mover = _get_mover(current);

   if (action == DRAG_START)
     {
	current->resizing = 1;
        evas_pointer_output_xy_get(current->gadcon->evas, &mx, &my);
        evas_object_geometry_get(mover, &ox, &oy, &ow, &oh);
	dx = mx - ox;
     }
   else if (action == DRAG_STOP)
     {
	current->resizing = 0;
	dx = 0;
        _save_widget_position(current);
     }
   else if ((action == DRAG_MOVE) && current->resizing)
     {
        int w;

        evas_pointer_output_xy_get(current->gadcon->evas, &mx, &my);

        w = ox + ow + dx - mx;

        if (w < current->min.w)
	  {
	     mx -= current->min.w - w;
	     w = current->min.w;
	  }
        /* don't go out of the screen */
	if (mx < dx)
	  {
	     w += mx - dx;
	     mx = dx;
	  }

        evas_object_resize(mover, w, oh);
        evas_object_move(mover, mx - dx, oy);

        evas_object_resize(current->o_frame, w, oh);
        evas_object_move(current->o_frame, mx - dx, oy);
     }
}

static void
on_move(void *data, Evas_Object *o __UNUSED__, const char *em __UNUSED__, const char *src __UNUSED__)
{
   Evas_Object *mover;
   static int dx, dy;  //Offset of mouse pointer inside the mover
   static int ox, oy;  //Starting object position
   static int ow, oh;  //Starting object size
   int mx, my;         //Mouse coord
   int action;

   action = (int)(long)data;
   mover = _get_mover(current);

   /* DRAG_START */
   if (action == DRAG_START)
     {
	current->moving = 1;
        evas_pointer_output_xy_get(current->gadcon->evas, &mx, &my);
        evas_object_geometry_get(mover, &ox, &oy, &ow, &oh);

        dx = mx - ox;
        dy = my - oy;

        return;
     }

   /* DRAG_STOP */
   if (action == DRAG_STOP)
     {
        E_Config_Gadcon_Client *cf;
        E_Zone *dst_zone = NULL;
        E_Gadcon *dst_gadcon;
        int gx, gy;

        current->moving = 0;
        dx = dy = 0;

        /* checking if zone was changed for dragged gadget */
        evas_object_geometry_get(current->o_frame, &gx, &gy, NULL, NULL);
        dst_zone = e_container_zone_at_point_get(e_container_current_get(e_manager_current_get()), gx, gy);
        if (dst_zone && (current->gadcon->zone != dst_zone))
          {
	     unsigned int layer = current->gadcon->id - ID_GADMAN_LAYER_BASE;
             cf = current->cf;

             current->gadcon->cf->clients = eina_list_remove(current->gadcon->cf->clients, cf);
             dst_gadcon = gadman_gadcon_get(dst_zone, layer);
             if (dst_gadcon)
               {
                  dst_gadcon->cf->clients = eina_list_append(dst_gadcon->cf->clients, cf);
                  e_config_save_queue();
               }
          }
        else
          _save_widget_position(current);
        return;
     }

   /* DRAG_MOVE */
   if ((action == DRAG_MOVE) && current->moving)
     {
        int x, y;

        evas_pointer_output_xy_get(current->gadcon->evas, &mx, &my);

        x = mx - dx;
        y = my - dy;

        /* don't go out of the screen */
        if (x < 0) x = 0;
        if (x > (Man->width - ow)) x = Man->width - ow;
        if (y < 0) y = 0;
        if (y > (Man->height - oh)) y = Man->height - oh;

        evas_object_move(current->o_frame, x , y);
        evas_object_move(mover, x, y);
        evas_object_raise(current->o_frame);
        evas_object_raise(mover);
     }
}

static void
on_bg_click(void *data __UNUSED__, Evas_Object *o __UNUSED__, const char *em __UNUSED__, const char *src __UNUSED__)
{
   gadman_gadgets_hide();
}

static void
on_hide_stop(void *data __UNUSED__, Evas_Object *o __UNUSED__, const char *em __UNUSED__, const char *src __UNUSED__)
{
   ecore_evas_hide(Man->top_ee);
}

static int 
_e_gadman_client_add(void *data __UNUSED__, const E_Gadcon_Client_Class *cc)
{
   return !!gadman_gadget_add(cc, GADMAN_LAYER_BG);
}

static void 
_e_gadman_client_remove(void *data __UNUSED__, E_Gadcon_Client *gcc)
{
   gadman_gadget_del(gcc);
}

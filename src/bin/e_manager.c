#include "e.h"

#include "dlog.h"
#undef LOG_TAG
#define LOG_TAG "E17"
/* local subsystem functions */
static void _e_manager_free(E_Manager *man);

static Eina_Bool _e_manager_cb_window_show_request(void *data, int ev_type, void *ev);
static Eina_Bool _e_manager_cb_window_configure(void *data, int ev_type, void *ev);
static Eina_Bool _e_manager_cb_key_up(void *data, int ev_type, void *ev);
static Eina_Bool _e_manager_cb_key_down(void *data, int ev_type, void *ev);
static Eina_Bool _e_manager_cb_frame_extents_request(void *data, int ev_type, void *ev);
static Eina_Bool _e_manager_cb_ping(void *data, int ev_type, void *ev);
static Eina_Bool _e_manager_cb_screensaver_notify(void *data, int ev_type, void *ev);
static Eina_Bool _e_manager_cb_client_message(void *data, int ev_type, void *ev);

static Eina_Bool _e_manager_frame_extents_free_cb(const Eina_Hash *hash __UNUSED__,
						  const void *key __UNUSED__,
						  void *data, void *fdata __UNUSED__);
static E_Manager *_e_manager_get_for_root(Ecore_X_Window root);
static Eina_Bool _e_manager_clear_timer(void *data);
#if 0 /* use later - maybe */
static int _e_manager_cb_window_destroy(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_hide(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_reparent(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_create(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_configure_request(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_gravity(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_stack(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_stack_request(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_property(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_colormap(void *data, int ev_type, void *ev);
static int _e_manager_cb_window_shape(void *data, int ev_type, void *ev);
static int _e_manager_cb_client_message(void *data, int ev_type, void *ev);
#endif

#ifdef _F_E_MANAGER_COMP_OBJECT_
typedef struct _E_Smart_Data               E_Smart_Data;
typedef struct _E_Manager_Comp_Object_Item E_Manager_Comp_Object_Item;

struct _E_Smart_Data
{
   E_Manager       *man;
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
   Evas_Object     *child;
   unsigned char    changed : 1;
   struct
     {
        int         id;
        int         x, y, w, h;
        Eina_Bool   managed;
     } input;
};

struct _E_Manager_Comp_Object_Item
{
   E_Smart_Data    *sd;
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
};

static E_Manager_Comp_Object_Item *_e_manager_comp_object_smart_adopt(E_Smart_Data *sd, Evas_Object *obj);
static void                        _e_manager_comp_object_smart_disown(Evas_Object *obj);
static void                        _e_manager_comp_object_smart_item_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void                        _e_manager_comp_object_smart_reconfigure(E_Smart_Data *sd);
static void                        _e_manager_comp_object_smart_move_resize_item(E_Manager_Comp_Object_Item *ci);

static void                        _e_manager_comp_object_smart_init(void);
static void                        _e_manager_comp_object_smart_add(Evas_Object *obj);
static void                        _e_manager_comp_object_smart_del(Evas_Object *obj);
static void                        _e_manager_comp_object_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void                        _e_manager_comp_object_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void                        _e_manager_comp_object_smart_show(Evas_Object *obj);
static void                        _e_manager_comp_object_smart_hide(Evas_Object *obj);

static Evas_Smart *_e_smart = NULL;
#endif /* end of _F_E_MANAGER_COMP_OBJECT_ */

/* local subsystem globals */

typedef struct _Frame_Extents Frame_Extents;

struct _Frame_Extents
{
   int l, r, t, b;
};

static Eina_List *managers = NULL;
static Eina_Hash *frame_extents = NULL;
static Ecore_Timer *timer_post_screensaver_lock = NULL;

#ifdef _F_TEMPORARY_COMP_EVENT_
EAPI int E_EVENT_COMP_SOURCE_VISIBILITY = 0;
EAPI int E_EVENT_COMP_SOURCE_ADD = 0;
EAPI int E_EVENT_COMP_SOURCE_DEL = 0;
EAPI int E_EVENT_COMP_SOURCE_CONFIGURE = 0;
EAPI int E_EVENT_COMP_SOURCE_STACK = 0;
#endif /* end of _F_TEMPORARY_COMP_EVENT_ */

/* externally accessible functions */
EINTERN int
e_manager_init(void)
{
   ecore_x_screensaver_event_listen_set(1);
   frame_extents = eina_hash_string_superfast_new(NULL);

#ifdef _F_TEMPORARY_COMP_EVENT_
   E_EVENT_COMP_SOURCE_VISIBILITY = ecore_event_type_new();
   E_EVENT_COMP_SOURCE_ADD = ecore_event_type_new();
   E_EVENT_COMP_SOURCE_DEL = ecore_event_type_new();
   E_EVENT_COMP_SOURCE_CONFIGURE = ecore_event_type_new();
   E_EVENT_COMP_SOURCE_STACK = ecore_event_type_new();
#endif /* end of _F_TEMPORARY_COMP_EVENT_ */

   return 1;
}

EINTERN int
e_manager_shutdown(void)
{
   E_FREE_LIST(managers, e_object_del);

   if (frame_extents)
     {
	eina_hash_foreach(frame_extents, _e_manager_frame_extents_free_cb, NULL);
	eina_hash_free(frame_extents);
	frame_extents = NULL;
     }

   if (timer_post_screensaver_lock)
     {
	ecore_timer_del(timer_post_screensaver_lock);
	timer_post_screensaver_lock = NULL;
     }

   return 1;
}

EAPI Eina_List *
e_manager_list(void)
{
   return managers;
}

EAPI E_Manager *
e_manager_new(Ecore_X_Window root, int num)
{
   E_Manager *man;

   if (!ecore_x_window_manage(root)) return NULL;
   man = E_OBJECT_ALLOC(E_Manager, E_MANAGER_TYPE, _e_manager_free);
   if (!man) return NULL;
   managers = eina_list_append(managers, man);
   man->root = root;
   man->num = num;
   ecore_x_window_size_get(man->root, &(man->w), &(man->h));
   man->win = man->root;

   man->handlers =
     eina_list_append(man->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHOW_REQUEST,
                                              _e_manager_cb_window_show_request,
                                              man));
   man->handlers =
     eina_list_append(man->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CONFIGURE,
                                              _e_manager_cb_window_configure,
                                              man));
   man->handlers =
     eina_list_append(man->handlers,
                      ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                              _e_manager_cb_key_down,
                                              man));
   man->handlers =
     eina_list_append(man->handlers,
                      ecore_event_handler_add(ECORE_EVENT_KEY_UP,
                                              _e_manager_cb_key_up,
                                              man));
   man->handlers =
     eina_list_append(man->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_FRAME_EXTENTS_REQUEST,
                                              _e_manager_cb_frame_extents_request,
                                              man));
   man->handlers =
     eina_list_append(man->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_PING,
                                              _e_manager_cb_ping,
                                              man));

#ifndef _F_DISABLE_E_SCREENSAVER
   man->handlers =
     eina_list_append(man->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_SCREENSAVER_NOTIFY,
                                              _e_manager_cb_screensaver_notify,
                                              man));

#endif
   man->handlers =
     eina_list_append(man->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,
                                              _e_manager_cb_client_message,
                                              man));

   man->pointer = e_pointer_window_new(man->root, 1);

   ecore_x_window_background_color_set(man->root, 0, 0, 0);

   man->clear_timer = ecore_timer_add(10.0, _e_manager_clear_timer, man);
   return man;
}

EAPI void
e_manager_manage_windows(E_Manager *man)
{
   Ecore_X_Window *windows;
   int wnum;

   /* a manager is designated for each root. lets get all the windows in
      the managers root */
   windows = ecore_x_window_children_get(man->root, &wnum);
   if (windows)
     {
        int i;
        const char *atom_names[] =
          {
             "_XEMBED_INFO",
             "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR",
             "KWM_DOCKWINDOW"
          };
        Ecore_X_Atom atoms[3];
        Ecore_X_Atom atom_xmbed, atom_kde_netwm_systray, atom_kwm_dockwindow;
        unsigned char *data = NULL;
        int count;

        ecore_x_atoms_get(atom_names, 3, atoms);
        atom_xmbed = atoms[0];
        atom_kde_netwm_systray = atoms[1];
        atom_kwm_dockwindow = atoms[2];
        for (i = 0; i < wnum; i++)
          {
             Ecore_X_Window_Attributes att;
             unsigned int ret_val, deskxy[2];
             int ret;

             if (e_border_find_by_client_window(windows[i]))
               continue;
             ecore_x_window_attributes_get(windows[i], &att);
             if ((att.override) || (att.input_only))
               {
                  if (att.override)
                    {
                       char *wname = NULL, *wclass = NULL;

                       ecore_x_icccm_name_class_get(windows[i],
                                                    &wname, &wclass);
                       if ((wname) && (wclass) &&
                           (!strcmp(wname, "E")) &&
                           (!strcmp(wclass, "Init_Window")))
                         {
                            free(wname);
                            free(wclass);
                            man->initwin = windows[i];
                         }
                       else
                         {
                            if (wname) free(wname);
                            if (wclass) free(wclass);
                            continue;
                         }
                    }
                  else
                    continue;
               }
             /* XXX manage xembed windows as long as they are not override_redirect..
              * if (!ecore_x_window_prop_property_get(windows[i],
              *					   atom_xmbed,
              *					   atom_xmbed, 32,
              *					   &data, &count))
              *   data = NULL;
              * if (!data) */
               {
                  if (!ecore_x_window_prop_property_get(windows[i],
                                                        atom_kde_netwm_systray,
                                                        atom_xmbed, 32,
                                                        &data, &count))
                    {
                       if (data) free(data);
                       data = NULL;
                    }
               }
             if (!data)
               {
                  if (!ecore_x_window_prop_property_get(windows[i],
                                                        atom_kwm_dockwindow,
                                                        atom_kwm_dockwindow, 32,
                                                        &data, &count))
                    {
                       if (data) free(data);
                       data = NULL;
                    }
               }
             if (data)
               {
                  free(data);
                  data = NULL;
                  continue;
               }
             ret = ecore_x_window_prop_card32_get(windows[i],
                                                  E_ATOM_MANAGED,
                                                  &ret_val, 1);

             /* we have seen this window before */
             if ((ret > -1) && (ret_val == 1))
               {
                  E_Container  *con = NULL;
                  E_Zone       *zone = NULL;
                  E_Desk       *desk = NULL;
                  E_Border     *bd = NULL;
                  unsigned int  id;
                  char *path;
                  Efreet_Desktop *desktop = NULL;

                  /* get all information from window before it is
                   * reset by e_border_new */
                  ret = ecore_x_window_prop_card32_get(windows[i],
                                                       E_ATOM_CONTAINER,
                                                       &id, 1);
                  if (ret == 1)
                    con = e_container_number_get(man, id);
                  if (!con)
                    con = e_container_current_get(man);

                  ret = ecore_x_window_prop_card32_get(windows[i],
                                                       E_ATOM_ZONE,
                                                       &id, 1);
                  if (ret == 1)
                    zone = e_container_zone_number_get(con, id);
                  if (!zone)
                    zone = e_zone_current_get(con);
                  ret = ecore_x_window_prop_card32_get(windows[i],
                                                       E_ATOM_DESK,
                                                       deskxy, 2);
                  if (ret == 2)
                    desk = e_desk_at_xy_get(zone,
                                            deskxy[0],
                                            deskxy[1]);

                  path = ecore_x_window_prop_string_get(windows[i],
                                                        E_ATOM_DESKTOP_FILE);
                  if (path)
                    {
#ifndef _F_DISABLE_E_EFREET_
                       desktop = efreet_desktop_get(path);
#endif
                       free(path);
                    }

                    {
                       bd = e_border_new(con, windows[i], 1, 0);
                       if (bd)
                         {
                            ELB(ELBT_BD, "NEW BORDER shown before starting WM", windows[i]);
                            bd->ignore_first_unmap = 1;
                            /* FIXME:
                             * It's enough to set the desk, the zone will
                             * be set according to the desk */
                            //			    if (zone) e_border_zone_set(bd, zone);
                            if (desk) e_border_desk_set(bd, desk);
                            bd->desktop = desktop;
                         }
                    }
               }
             else if ((att.visible) && (!att.override) &&
                      (!att.input_only))
               {
                  /* We have not seen this window, and X tells us it
                   * should be seen */
                  E_Container *con;
                  E_Border *bd;

                  con = e_container_current_get(man);
                  bd = e_border_new(con, windows[i], 1, 0);
                  if (bd)
                    {
                       ELB(ELBT_BD, "NEW BORDER shown before starting WM", windows[i]);
                       bd->ignore_first_unmap = 1;
                       //e_border_show(bd);
                    }
               }
          }
        free(windows);
     }
}

EAPI void
e_manager_show(E_Manager *man)
{
   Eina_List *l;
   E_Container *con;

   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (man->visible) return;
   EINA_LIST_FOREACH(man->containers, l, con)
     {
	e_container_show(con);
     }
   if (man->root != man->win)
     {
	Ecore_X_Window mwin;

	mwin = e_menu_grab_window_get();
	if (!mwin) mwin = man->initwin;
	if (!mwin)
	  ecore_x_window_raise(man->win);
	else
	  ecore_x_window_configure(man->win,
				   ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
				   ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
				   0, 0, 0, 0, 0,
				   mwin, ECORE_X_WINDOW_STACK_BELOW);
	ecore_x_window_show(man->win);
     }
   man->visible = 1;
}

EAPI void
e_manager_hide(E_Manager *man)
{
   Eina_List *l;
   E_Container *con;

   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (!man->visible) return;
   EINA_LIST_FOREACH(man->containers, l, con)
     {
	e_container_hide(con);
     }
   if (man->root != man->win)
     ecore_x_window_hide(man->win);
   man->visible = 0;
}

EAPI void
e_manager_move(E_Manager *man, int x, int y)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if ((x == man->x) && (y == man->y)) return;
   if (man->root != man->win)
     {
	man->x = x;
	man->y = y;
	ecore_x_window_move(man->win, man->x, man->y);
     }
}

EAPI void
e_manager_resize(E_Manager *man, int w, int h)
{
   Eina_List *l;
   E_Container *con;

   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   man->w = w;
   man->h = h;
   if (man->root != man->win)
     ecore_x_window_resize(man->win, man->w, man->h);

   EINA_LIST_FOREACH(man->containers, l, con)
     {
	e_container_resize(con, man->w, man->h);
     }

   ecore_x_netwm_desk_size_set(man->root, man->w, man->h);
}

EAPI void
e_manager_move_resize(E_Manager *man, int x, int y, int w, int h)
{
   Eina_List *l;
   E_Container *con;

   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (man->root != man->win)
     {
	man->x = x;
	man->y = y;
     }
   man->w = w;
   man->h = h;
   ecore_x_window_move_resize(man->win, man->x, man->y, man->w, man->h);

   EINA_LIST_FOREACH(man->containers, l, con)
     {
	e_container_resize(con, man->w, man->h);
     }
}

EAPI void
e_manager_raise(E_Manager *man)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (man->root != man->win)
     {
	Ecore_X_Window mwin;

	mwin = e_menu_grab_window_get();
	if (!mwin) mwin = man->initwin;
	if (!mwin)
	  ecore_x_window_raise(man->win);
	else
	  ecore_x_window_configure(man->win,
				   ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
				   ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
				   0, 0, 0, 0, 0,
				   mwin, ECORE_X_WINDOW_STACK_BELOW);
     }
}

EAPI void
e_manager_lower(E_Manager *man)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (man->root != man->win)
     ecore_x_window_lower(man->win);
}

EAPI E_Manager *
e_manager_current_get(void)
{
   Eina_List *l;
   E_Manager *man;
   int x, y;

   if (!managers) return NULL;
   EINA_LIST_FOREACH(managers, l, man)
     {
	ecore_x_pointer_xy_get(man->win, &x, &y);
	if (x == -1 && y == -1)
	  continue;
	if (E_INSIDE(x, y, man->x, man->y, man->w, man->h))
	  return man;
     }
   return eina_list_data_get(managers);
}

EAPI E_Manager *
e_manager_number_get(int num)
{
   Eina_List *l;
   E_Manager *man;

   if (!managers) return NULL;
   EINA_LIST_FOREACH(managers, l, man)
     {
	if (man->num == num)
	  return man;
     }
   return NULL;
}

EAPI void
e_managers_keys_grab(void)
{
   Eina_List *l;
   E_Manager *man;

   EINA_LIST_FOREACH(managers, l, man)
     {
	e_bindings_key_grab(E_BINDING_CONTEXT_ANY, man->root);
     }
}

EAPI void
e_managers_keys_ungrab(void)
{
   Eina_List *l;
   E_Manager *man;

   EINA_LIST_FOREACH(managers, l, man)
     {
	e_bindings_key_ungrab(E_BINDING_CONTEXT_ANY, man->root);
     }
}

EAPI void
e_manager_comp_set(E_Manager *man, E_Manager_Comp *comp)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   man->comp = comp;
   e_msg_send("comp.manager", "change.comp", // name + info
              0, // val
              E_OBJECT(man), // obj
              NULL, // msgdata
              NULL, NULL); // afterfunc + afterdata
}

EAPI Evas *
e_manager_comp_evas_get(E_Manager *man)
{
   E_OBJECT_CHECK_RETURN(man, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, NULL);
   if (!man->comp) return NULL;
   if (!man->comp->func.evas_get) return NULL;
   return man->comp->func.evas_get(man->comp->data, man);
}

EAPI void
e_manager_comp_evas_update(E_Manager *man)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (!man->comp) return;
   if (!man->comp->func.update) return;
   return man->comp->func.update(man->comp->data, man);
}

EAPI const Eina_List *
e_manager_comp_src_list(E_Manager *man)
{
   E_OBJECT_CHECK_RETURN(man, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, NULL);
   if (!man->comp) return NULL;
   if (!man->comp->func.src_list_get) return NULL;
   return man->comp->func.src_list_get(man->comp->data, man);
}

EAPI E_Manager_Comp_Source*
e_manager_comp_border_src_get(E_Manager *man, Ecore_X_Window win)
{
   E_OBJECT_CHECK_RETURN(man, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, NULL);
   if (!man->comp) return NULL;
   if (!man->comp->func.border_src_get) return NULL;
   return man->comp->func.border_src_get(man->comp->data, man, win);
}

EAPI E_Manager_Comp_Source*
e_manager_comp_src_get(E_Manager *man, Ecore_X_Window win)
{
   E_OBJECT_CHECK_RETURN(man, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, NULL);
   if (!man->comp) return NULL;
   if (!man->comp->func.src_get) return NULL;
   return man->comp->func.src_get(man->comp->data, man, win);
}

EAPI Evas_Object *
e_manager_comp_src_image_get(E_Manager *man, E_Manager_Comp_Source *src)
{
   E_OBJECT_CHECK_RETURN(man, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, NULL);
   if (!man->comp) return NULL;
   if (!man->comp->func.src_image_get) return NULL;
   return man->comp->func.src_image_get(man->comp->data, man, src);
}

EAPI Evas_Object *
e_manager_comp_src_shadow_get(E_Manager *man, E_Manager_Comp_Source *src)
{
   E_OBJECT_CHECK_RETURN(man, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, NULL);
   if (!man->comp) return NULL;
   if (!man->comp->func.src_shadow_get) return NULL;
   return man->comp->func.src_shadow_get(man->comp->data, man, src);
}

EAPI Evas_Object *
e_manager_comp_src_image_mirror_add(E_Manager *man, E_Manager_Comp_Source *src)
{
   E_OBJECT_CHECK_RETURN(man, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, NULL);
   if (!man->comp) return NULL;
   if (!man->comp->func.src_image_mirror_add) return NULL;
   return man->comp->func.src_image_mirror_add(man->comp->data, man, src);
}

EAPI Eina_Bool
e_manager_comp_src_visible_get(E_Manager *man, E_Manager_Comp_Source *src)
{
   E_OBJECT_CHECK_RETURN(man, EINA_FALSE);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, EINA_FALSE);
   if (!man->comp) return EINA_FALSE;
   if (!man->comp->func.src_visible_get) return EINA_FALSE;
   return man->comp->func.src_visible_get(man->comp->data, man, src);
}

EAPI void
e_manager_comp_src_hidden_set(E_Manager *man, E_Manager_Comp_Source *src, Eina_Bool hidden)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (!man->comp) return;
   if (!man->comp->func.src_hidden_set) return;
   return man->comp->func.src_hidden_set(man->comp->data, man, src, hidden);
}

EAPI Eina_Bool
e_manager_comp_src_hidden_get(E_Manager *man, E_Manager_Comp_Source *src)
{
   E_OBJECT_CHECK_RETURN(man, EINA_FALSE);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, EINA_FALSE);
   if (!man->comp) return EINA_FALSE;
   if (!man->comp->func.src_hidden_get) return EINA_FALSE;
   return man->comp->func.src_hidden_get(man->comp->data, man, src);
}

EAPI Ecore_X_Window
e_manager_comp_src_window_get(E_Manager *man, E_Manager_Comp_Source *src)
{
   E_OBJECT_CHECK_RETURN(man, 0);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, 0);
   if (!man->comp) return 0;
   if (!man->comp->func.src_window_get) return 0;
   return man->comp->func.src_window_get(man->comp->data, man, src);
}

EAPI E_Popup *
e_manager_comp_src_popup_get(E_Manager *man, E_Manager_Comp_Source *src)
{
   E_OBJECT_CHECK_RETURN(man, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, NULL);
   if (!man->comp) return NULL;
   if (!man->comp->func.src_popup_get) return NULL;
   return man->comp->func.src_popup_get(man->comp->data, man, src);
}

EAPI E_Border *
e_manager_comp_src_border_get(E_Manager *man, E_Manager_Comp_Source *src)
{
   E_OBJECT_CHECK_RETURN(man, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, NULL);
   if (!man->comp) return NULL;
   if (!man->comp->func.src_border_get) return NULL;
   return man->comp->func.src_border_get(man->comp->data, man, src);
}

#ifdef _F_COMP_SCREEN_LOCK_
EAPI void
e_manager_comp_screen_lock(E_Manager *man)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (!man->comp) return;
   if (!man->comp->func.screen_lock) return;
   ELB(ELBT_MNG, ">> SCREEN LOCK", 0);
   return man->comp->func.screen_lock(man->comp->data, man);
}

EAPI void
e_manager_comp_screen_unlock(E_Manager *man)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (!man->comp) return;
   if (!man->comp->func.screen_unlock) return;
   ELB(ELBT_MNG, "<< SCREEN UNLOCK", 0);
   return man->comp->func.screen_unlock(man->comp->data, man);
}
#endif

#ifdef _F_COMP_INPUT_REGION_SET_
EAPI Eina_Bool
e_manager_comp_src_input_region_set(E_Manager *man, E_Manager_Comp_Source *src, int x, int y, int w, int h)
{
   E_OBJECT_CHECK_RETURN(man, EINA_FALSE);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, EINA_FALSE);
   if (!man->comp) return EINA_FALSE;
   if (!man->comp->func.src_input_region_set) return EINA_FALSE;
   return man->comp->func.src_input_region_set(man->comp->data, man, src, x, y, w, h);
}

EAPI int
e_manager_comp_input_region_new(E_Manager *man)
{
   E_OBJECT_CHECK_RETURN(man, 0);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, 0);
   if (!man->comp) return 0;
   if (!man->comp->func.input_region_new) return 0;
   return man->comp->func.input_region_new(man->comp->data, man);
}

EAPI Eina_Bool
e_manager_comp_input_region_set(E_Manager *man, int id, int x, int y, int w, int h)
{
   E_OBJECT_CHECK_RETURN(man, EINA_FALSE);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, EINA_FALSE);
   if (id <= 0) return EINA_FALSE;
   if (!man->comp) return EINA_FALSE;
   if (!man->comp->func.input_region_set) return EINA_FALSE;
   return man->comp->func.input_region_set(man->comp->data, man, id, x, y, w, h);
}

EAPI Eina_Bool
e_manager_comp_input_region_managed_set(E_Manager *man, int id, Evas_Object *obj, Eina_Bool set)
{
   E_OBJECT_CHECK_RETURN(man, 0);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, 0);
   if (!man->comp) return 0;
   if (!man->comp->func.input_region_managed_set) return 0;
   return man->comp->func.input_region_managed_set(man->comp->data, man, id, obj, set);
}

EAPI Eina_Bool
e_manager_comp_input_region_del(E_Manager *man, int id)
{
   E_OBJECT_CHECK_RETURN(man, EINA_FALSE);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, EINA_FALSE);
   if (id <= 0) return EINA_FALSE;
   if (!man->comp) return EINA_FALSE;
   if (!man->comp->func.input_region_del) return EINA_FALSE;
   return man->comp->func.input_region_del(man->comp->data, man, id);
}

EAPI int
e_manager_comp_input_region_id_new(E_Manager *man)
{
   E_OBJECT_CHECK_RETURN(man, 0);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, 0);
   if (!man->comp) return 0;
   if (!man->comp->func.input_region_new) return 0;
   return man->comp->func.input_region_new(man->comp->data, man);
}

EAPI Eina_Bool
e_manager_comp_input_region_id_set(E_Manager *man, int id, int x, int y, int w, int h)
{
   E_OBJECT_CHECK_RETURN(man, EINA_FALSE);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, EINA_FALSE);
   if (id <= 0) return EINA_FALSE;
   if (!man->comp) return EINA_FALSE;
   if (!man->comp->func.input_region_set) return EINA_FALSE;
   return man->comp->func.input_region_set(man->comp->data, man, id, x, y, w, h);
}

EAPI Eina_Bool
e_manager_comp_input_region_id_del(E_Manager *man, int id)
{
   E_OBJECT_CHECK_RETURN(man, EINA_FALSE);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, EINA_FALSE);
   if (id <= 0) return EINA_FALSE;
   if (!man->comp) return EINA_FALSE;
   if (!man->comp->func.input_region_del) return EINA_FALSE;
   return man->comp->func.input_region_del(man->comp->data, man, id);
}
#endif

#ifdef _F_COMP_MOVE_LOCK_
EAPI Eina_Bool
e_manager_comp_src_move_lock(E_Manager *man, E_Manager_Comp_Source *src)
{
   E_OBJECT_CHECK_RETURN(man, EINA_FALSE);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, EINA_FALSE);
   if (!man->comp) return EINA_FALSE;
   if (!man->comp->func.src_move_lock) return EINA_FALSE;
   return man->comp->func.src_move_lock(man->comp->data, man, src);
}

EAPI Eina_Bool
e_manager_comp_src_move_unlock(E_Manager *man, E_Manager_Comp_Source *src)
{
   E_OBJECT_CHECK_RETURN(man, EINA_FALSE);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, EINA_FALSE);
   if (!man->comp) return EINA_FALSE;
   if (!man->comp->func.src_move_unlock) return EINA_FALSE;
   return man->comp->func.src_move_unlock(man->comp->data, man, src);
}
#endif

#ifdef _F_COMP_COMPOSITE_MODE_
EAPI void
e_manager_comp_composite_mode_set(E_Manager *man, E_Zone *zone, Eina_Bool set)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);
   if (!man->comp) return;
   if (!man->comp->func.composite_mode_set) return;
   man->comp->func.composite_mode_set(man->comp->data, man, zone, set);
}

EAPI Eina_Bool
e_manager_comp_composite_mode_get(E_Manager *man, E_Zone *zone)
{
   E_OBJECT_CHECK_RETURN(man, EINA_FALSE);
   E_OBJECT_CHECK_RETURN(zone, EINA_FALSE);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, EINA_FALSE);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, EINA_FALSE);
   if (!man->comp) return EINA_FALSE;
   if (!man->comp->func.composite_mode_get) return EINA_FALSE;
   return man->comp->func.composite_mode_get(man->comp->data, man, zone);
}
#endif /* _F_COMP_COMPOSITE_MODE_ */

EAPI void
e_manager_comp_event_resize_send(E_Manager *man)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   e_msg_send("comp.manager", "resize.comp", // name + info
              0, // val
              E_OBJECT(man), // obj
              NULL, // msgdata
              NULL, NULL); // afterfunc + afterdata
}

EAPI void
e_manager_comp_event_src_add_send(E_Manager *man, E_Manager_Comp_Source *src,
                                  void (*afterfunc) (void *data, E_Manager *man, E_Manager_Comp_Source *src),
                                  void *data)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   e_msg_send("comp.manager", "add.src", // name + info
              0, // val
              E_OBJECT(man), // obj
              src, // msgdata
              (void (*)(void *, E_Object *, void *))afterfunc, data); // afterfunc + afterdata
}

EAPI void
e_manager_comp_event_src_del_send(E_Manager *man, E_Manager_Comp_Source *src,
                                  void (*afterfunc) (void *data, E_Manager *man, E_Manager_Comp_Source *src),
                                  void *data)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   e_msg_send("comp.manager", "del.src", // name + info
              0, // val
              E_OBJECT(man), // obj
              src, // msgdata
              (void (*)(void *, E_Object *, void *))afterfunc, data); // afterfunc + afterdata
}

EAPI void
e_manager_comp_event_src_config_send(E_Manager *man, E_Manager_Comp_Source *src,
                                     void (*afterfunc) (void *data, E_Manager *man, E_Manager_Comp_Source *src),
                                     void *data)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   e_msg_send("comp.manager", "config.src", // name + info
              0, // val
              E_OBJECT(man), // obj
              src, // msgdata
              (void (*)(void *, E_Object *, void *))afterfunc, data); // afterfunc + afterdata
}

EAPI void
e_manager_comp_event_src_visibility_send(E_Manager *man, E_Manager_Comp_Source *src,
                                         void (*afterfunc) (void *data, E_Manager *man, E_Manager_Comp_Source *src),
                                         void *data)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   e_msg_send("comp.manager", "visibility.src", // name + info
              0, // val
              E_OBJECT(man), // obj
              src, // msgdata
              (void (*)(void *, E_Object *, void *))afterfunc, data); // afterfunc + afterdata
}

#ifdef _F_USE_EXTENDED_ICONIFY_
EAPI void
e_manager_comp_src_shadow_show(E_Manager *man, E_Manager_Comp_Source *src)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (!man->comp) return;
   if (!man->comp->func.src_shadow_show) return;
   return man->comp->func.src_shadow_show(man->comp->data, man, src);
}

EAPI void
e_manager_comp_src_shadow_hide(E_Manager *man, E_Manager_Comp_Source *src)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   if (!man->comp) return;
   if (!man->comp->func.src_shadow_show) return;
   return man->comp->func.src_shadow_hide(man->comp->data, man, src);
}
#endif

#ifdef _F_COMP_LAYER_
EAPI Evas_Object *
e_manager_comp_layer_get(E_Manager *man, E_Zone *zone, const char *name)
{
   E_OBJECT_CHECK_RETURN(man, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(man, E_MANAGER_TYPE, NULL);
   E_OBJECT_CHECK_RETURN(zone, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, NULL);
   if (!name) return NULL;
   if (!man->comp) return NULL;
   if (!man->comp->func.layer_get) return NULL;
   return man->comp->func.layer_get(man->comp->data, man, zone, name);
}

EAPI void
e_manager_comp_layer_raise_above(E_Manager     *man,
                                 E_Zone        *zone,
                                 const char    *name,
                                 E_Border      *bd)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (!name) return;

   return man->comp->func.layer_raise_above(man->comp->data, man, zone, name, bd);
}

EAPI void
e_manager_comp_layer_lower_below(E_Manager     *man,
                                 E_Zone        *zone,
                                 const char    *name,
                                 E_Border      *bd)
{
   E_OBJECT_CHECK(man);
   E_OBJECT_TYPE_CHECK(man, E_MANAGER_TYPE);
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (!name) return;

   return man->comp->func.layer_lower_below(man->comp->data, man, zone, name, bd);
}
#endif

#ifdef _F_E_MANAGER_COMP_OBJECT_
EAPI Evas_Object *
e_manager_comp_object_add(Evas *evas)
{
   _e_manager_comp_object_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

EAPI void
e_manager_comp_object_pack(Evas_Object *obj,
                           Evas_Object *child)
{
   E_Smart_Data *sd;
   E_Manager_Comp_Object_Item *ci;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERRNR();
   sd = evas_object_smart_data_get(obj);
   _e_manager_comp_object_smart_adopt(sd, child);
   sd->child = child;
   ci = evas_object_data_get(child, "e_manager_comp_object_data");
   _e_manager_comp_object_smart_move_resize_item(ci);
}

EAPI void
e_manager_comp_object_unpack(Evas_Object *obj)
{
   E_Smart_Data *sd;
   E_Manager_Comp_Object_Item *ci;

   ci = evas_object_data_get(obj, "e_manager_comp_object_data");
   if (!ci) return;
   sd = ci->sd;
   if (sd->input.managed)
     {
        e_manager_comp_input_region_managed_set(sd->man, sd->input.id, obj, EINA_FALSE);
        sd->input.managed = EINA_FALSE;
     }

   sd->child = NULL;
   _e_manager_comp_object_smart_disown(obj);
}

EAPI void
e_manager_comp_object_managed_set(Evas_Object *obj, Eina_Bool set)
{
   E_Smart_Data *sd;
   E_Manager_Comp_Object_Item *ci;

   if (evas_object_smart_smart_get(obj) != _e_smart) SMARTERRNR();
   sd = evas_object_smart_data_get(obj);
   sd->input.managed = set;

   e_manager_comp_input_region_managed_set(sd->man, sd->input.id, sd->child, set);
}
#endif /* end of _F_E_MANAGER_COMP_OBJECT_ */

/* local subsystem functions */
static void
_e_manager_free(E_Manager *man)
{
   Eina_List *l;

   E_FREE_LIST(man->handlers, ecore_event_handler_del);
   l = man->containers;
   man->containers = NULL;
   E_FREE_LIST(l, e_object_del);
   if (man->root != man->win)
     {
	ecore_x_window_free(man->win);
     }
   if (man->pointer) e_object_del(E_OBJECT(man->pointer));
   managers = eina_list_remove(managers, man);
   if (man->clear_timer) ecore_timer_del(man->clear_timer);
   free(man);
}

static Eina_Bool
_e_manager_cb_window_show_request(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   Ecore_X_Event_Window_Show_Request *e;

   man = data;
   e = ev;
   if (e_stolen_win_get(e->win)) return 1;
   if (ecore_x_window_parent_get(e->win) != man->root)
     return ECORE_CALLBACK_PASS_ON;  /* try other handlers for this */

   LOGE("Show request(0x%08x)", e->win);
   traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:WINDOW:SHOW_REQUEST");

   E_Container *con;
   E_Border *bd;

   con = e_container_current_get(man);
   if (!e_border_find_by_client_window(e->win))
     {
        bd = e_border_new(con, e->win, 0, 0);
        if (!bd)
          {
             /* the wm shows only valid input_only window.
              * it doesn't need to deal with invalid window and
              * override_redirect window.
              */
             Ecore_X_Window_Attributes att;
             if ((ecore_x_window_attributes_get(e->win, &att)) &&
                 (!att.override))
               {
                  ecore_x_window_show(e->win);
               }
             else
               {
                  ELB(ELBT_MNG, "show request of invalid win", e->win);
               }
          }
     }

   traceEnd(TTRACE_TAG_WINDOW_MANAGER);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_manager_cb_window_configure(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   Ecore_X_Event_Window_Configure *e;

   man = data;
   e = ev;
   if (e->win != man->root) return ECORE_CALLBACK_PASS_ON;
   if ((man->w != e->w) || (man->h != e->h))
     e_manager_resize(man, e->w, e->h);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_manager_cb_key_down(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   Ecore_Event_Key *e;

   man = data;
   e = ev;

   if (e->event_window != man->root) return ECORE_CALLBACK_PASS_ON;
   if (e->root_window != man->root) man = _e_manager_get_for_root(e->root_window);
   if (e_bindings_key_down_event_handle(E_BINDING_CONTEXT_MANAGER, E_OBJECT(man), ev))
     return ECORE_CALLBACK_DONE;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_manager_cb_key_up(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   Ecore_Event_Key *e;

   man = data;
   e = ev;

   if (e->event_window != man->root) return ECORE_CALLBACK_PASS_ON;
   if (e->root_window != man->root) man = _e_manager_get_for_root(e->root_window);
   if (e_bindings_key_up_event_handle(E_BINDING_CONTEXT_MANAGER, E_OBJECT(man), ev))
     return ECORE_CALLBACK_DONE;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_manager_cb_frame_extents_request(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   E_Container *con;
   Ecore_X_Event_Frame_Extents_Request *e;
   Ecore_X_Window_Type type;
   Ecore_X_MWM_Hint_Decor decor;
   Ecore_X_Window_State *state;
   Frame_Extents *extents;
   const char *border, *sig, *key;
   int ok;
   unsigned int i, num;

   man = data;
   con = e_container_current_get(man);
   e = ev;

   if (ecore_x_window_parent_get(e->win) != man->root) return ECORE_CALLBACK_PASS_ON;

   /* TODO:
    * * We need to check if we remember this window, and border locking is set
    */
   border = "default";
   key = border;
   ok = ecore_x_mwm_hints_get(e->win, NULL, &decor, NULL);
   if ((ok) &&
       (!(decor & ECORE_X_MWM_HINT_DECOR_ALL)) &&
       (!(decor & ECORE_X_MWM_HINT_DECOR_TITLE)) &&
       (!(decor & ECORE_X_MWM_HINT_DECOR_BORDER)))
     {
	border = "borderless";
	key = border;
     }

   ok = ecore_x_netwm_window_type_get(e->win, &type);
   if ((ok) &&
       ((type == ECORE_X_WINDOW_TYPE_DESKTOP) ||
	(type == ECORE_X_WINDOW_TYPE_DOCK)))
     {
	border = "borderless";
	key = border;
     }

   sig = NULL;
   ecore_x_netwm_window_state_get(e->win, &state, &num);
   if (state)
     {
	int maximized = 0;

	for (i = 0; i < num; i++)
	  {
	     switch (state[i])
	       {
		case ECORE_X_WINDOW_STATE_MAXIMIZED_VERT:
		  maximized++;
		  break;
		case ECORE_X_WINDOW_STATE_MAXIMIZED_HORZ:
		  maximized++;
		  break;
		case ECORE_X_WINDOW_STATE_FULLSCREEN:
		  border = "borderless";
		  key = border;
		  break;
		case ECORE_X_WINDOW_STATE_SHADED:
		case ECORE_X_WINDOW_STATE_SKIP_TASKBAR:
		case ECORE_X_WINDOW_STATE_SKIP_PAGER:
		case ECORE_X_WINDOW_STATE_HIDDEN:
		case ECORE_X_WINDOW_STATE_ICONIFIED:
		case ECORE_X_WINDOW_STATE_MODAL:
		case ECORE_X_WINDOW_STATE_STICKY:
		case ECORE_X_WINDOW_STATE_ABOVE:
		case ECORE_X_WINDOW_STATE_BELOW:
		case ECORE_X_WINDOW_STATE_DEMANDS_ATTENTION:
		case ECORE_X_WINDOW_STATE_UNKNOWN:
		  break;
	       }
	  }
	if ((maximized == 2) &&
	    (e_config->maximize_policy == E_MAXIMIZE_FULLSCREEN))
	  {
	     sig = "e,action,maximize,fullscreen";
	     key = "maximize,fullscreen";
	  }
	free(state);
     }

   extents = eina_hash_find(frame_extents, key);
   if ((!extents) && (con))
     {
	extents = E_NEW(Frame_Extents, 1);
	if (extents)
	  {
	     Evas_Object *o;
	     char buf[1024];

	     o = edje_object_add(con->bg_evas);
	     snprintf(buf, sizeof(buf), "e/widgets/border/%s/border", border);
	     ok = e_theme_edje_object_set(o, "base/theme/borders", buf);
	     if (ok)
	       {
		  Evas_Coord x, y, w, h;

		  if (sig)
		    {
		       edje_object_signal_emit(o, sig, "e");
		       edje_object_message_signal_process(o);
		    }

		  evas_object_resize(o, 1000, 1000);
		  edje_object_calc_force(o);
		  edje_object_part_geometry_get(o, "e.swallow.client",
						&x, &y, &w, &h);
		  extents->l = x;
		  extents->r = 1000 - (x + w);
		  extents->t = y;
		  extents->b = 1000 - (y + h);
	       }
	     else
	       {
		  extents->l = 0;
		  extents->r = 0;
		  extents->t = 0;
		  extents->b = 0;
	       }
	     evas_object_del(o);
	     eina_hash_add(frame_extents, key, extents);
	  }
     }

   if (extents)
     ecore_x_netwm_frame_size_set(e->win, extents->l, extents->r, extents->t, extents->b);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_manager_cb_ping(void *data, int ev_type __UNUSED__, void *ev)
{
   E_Manager *man;
   E_Border *bd;
   Ecore_X_Event_Ping *e;

   man = data;
   e = ev;

   if (e->win != man->root) return ECORE_CALLBACK_PASS_ON;

   bd = e_border_find_by_client_window(e->event_win);
   if (!bd) return ECORE_CALLBACK_PASS_ON;

   bd->ping_ok = 1;
   return ECORE_CALLBACK_PASS_ON;
}

#ifndef _F_DISABLE_E_SCREENSAVER
static Eina_Bool
_e_manager_cb_timer_post_screensaver_lock(void *data __UNUSED__)
{
   e_desklock_show_autolocked();
   timer_post_screensaver_lock = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_manager_cb_screensaver_notify(void *data __UNUSED__, int ev_type __UNUSED__, void *ev)
{
   Ecore_X_Event_Screensaver_Notify *e = ev;

   if (timer_post_screensaver_lock)
     {
	ecore_timer_del(timer_post_screensaver_lock);
	timer_post_screensaver_lock = NULL;
     }

   if (e->on)
     {
	if (e_config->desklock_autolock_screensaver)
	  {
	     if (e_config->desklock_post_screensaver_time <= 1.0)
	       {
		  e_desklock_show_autolocked();
	       }
	     else
	       {
		  timer_post_screensaver_lock = ecore_timer_add
		    (e_config->desklock_post_screensaver_time,
		     _e_manager_cb_timer_post_screensaver_lock, NULL);
	       }
	  }
     }
   return ECORE_CALLBACK_PASS_ON;
}

#endif

#ifdef _F_WINDOW_GROUP_RAISE_
static void
_e_manager_windows_group_raise (E_Border* bd)
{
   Ecore_X_Window leader_win;

   if (!bd) return;

   if ((e_config->focus_setting == E_FOCUS_NEW_WINDOW) ||
      (e_config->focus_setting == E_FOCUS_NEW_WINDOW_IF_TOP_STACK) ||
      ((bd->parent) &&
      ((e_config->focus_setting == E_FOCUS_NEW_DIALOG) ||
      ((bd->parent->focused) &&
      (e_config->focus_setting == E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED)))))
     {
        leader_win = bd->client.icccm.client_leader;
        if (leader_win)
          {
             Eina_List* restack_list;
             E_Border *temp_bd;
             E_Border *top_bd;
             E_Border_List *bl;

             restack_list = NULL;
             top_bd = NULL;
             bl = e_container_border_list_last(bd->zone->container);
             if (bl)
               {
                  while ((temp_bd = e_container_border_list_prev(bl)))
                    {
                       if (temp_bd->client.icccm.client_leader == leader_win)
                         {
                            restack_list = eina_list_prepend(restack_list, temp_bd);
                         }
                    }
                  e_container_border_list_free(bl);
               }

             if (restack_list)
               {
                  EINA_LIST_FREE (restack_list, temp_bd)
                    {
                       if (temp_bd->iconic)
                         {
                            if (e_config->clientlist_warp_to_iconified_desktop == 1)
                               e_desk_show(temp_bd->desk);

                            if (!temp_bd->lock_user_iconify)
                               e_border_uniconify(temp_bd);
                         }

                       if (!temp_bd->iconic) e_desk_show(temp_bd->desk);
                       if (!temp_bd->lock_user_stacking) e_border_raise(temp_bd);

                       // set focus to top window
                       if (!temp_bd->lock_focus_out)
                         {
                            top_bd = temp_bd;

                            if (e_config->focus_policy != E_FOCUS_CLICK)
                               ecore_x_pointer_warp(top_bd->zone->container->win, top_bd->x + (top_bd->w / 2), top_bd->y + (top_bd->h / 2));

                            if (e_config->focus_setting != E_FOCUS_NEW_WINDOW_IF_TOP_STACK)
                               e_border_focus_set(top_bd, 1, 1);
                            else
                              {
                                 Eina_List* l2;
                                 E_Border* temp_bd2;
                                 l2 = NULL;
                                 temp_bd2 = NULL;

                                 bl = e_container_border_list_last(bd->zone->container);
                                 if (bl)
                                   {
                                      while ((temp_bd2 = e_container_border_list_prev(bl)))
                                        {
                                           if ((!temp_bd2->iconic) && (temp_bd2->visible) && (temp_bd2->desk == top_bd->desk) &&
                                               (temp_bd2->client.icccm.accepts_focus || temp_bd2->client.icccm.take_focus) &&
                                               (temp_bd2->client.netwm.type != ECORE_X_WINDOW_TYPE_DOCK) &&
                                               (temp_bd2->client.netwm.type != ECORE_X_WINDOW_TYPE_TOOLBAR) &&
                                               (temp_bd2->client.netwm.type != ECORE_X_WINDOW_TYPE_MENU) &&
                                               (temp_bd2->client.netwm.type != ECORE_X_WINDOW_TYPE_SPLASH) &&
                                               (temp_bd2->client.netwm.type != ECORE_X_WINDOW_TYPE_DESKTOP))
                                             {
                                                if (top_bd == temp_bd2)
                                                  {
                                                     e_border_focus_set(top_bd, 1, 1);
                                                  }
                                                break;
                                             }
                                        }
                                      e_container_border_list_free(bl);
                                   }
                              }
                         }
                    }
               }
          }
     }
}
#endif // group raise

static Eina_Bool
_e_manager_cb_client_message(void *data __UNUSED__, int ev_type __UNUSED__, void *ev)
{
   Ecore_X_Event_Client_Message *e;
   E_Border *bd;

   e = ev;

   if (e->message_type == ECORE_X_ATOM_NET_ACTIVE_WINDOW)
     {
        ELB(ELBT_BD, "ACTIVE REQUEST", e->win);
		LOGE("ACTIVE REQUEST(0x%08x)", e->win);

        traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:WINDOW:ACTIVE_REQUEST");
        bd = e_border_find_by_client_window(e->win);
        if (bd)
          {
#ifdef _F_E_WIN_AUX_HINT_
             if (e_config->win_aux_hint)
               {
                  if (bd->client.e.fetch.aux_hint.hints)
                    {
                       e_border_aux_hint_info_update(bd);
                       bd->client.e.fetch.aux_hint.hints = 0;
                    }
               }
#endif /* end of _F_E_WIN_AUX_HINT_ */

             bd->client.e.state.deiconify_approve.render_only = 0;

             // reset flag for iconified window requested by client
             if (bd->iconify_by_client == 1)
               {
                  ELBF(ELBT_BD, 0, bd->client.win, "Reset ICONIFY BY CLIENT");
                  bd->iconify_by_client = 0;
               }

#if 0 /* notes */
             if (e->data.l[0] == 0 /* 0 == old, 1 == client, 2 == pager */)
               {
                  // FIXME: need config for the below - what to do given each
                  //  request (either do nothng, make app look urgent/want
                  //  attention or actiually flip to app as below is the
                  //  current default)
                  // if 0 == just make app demand attention somehow
                  // if 1 == just make app demand attention somehow
                  // if 2 == activate window as below
               }
             timestamp = e->data.l[1];
             requestor_id e->data.l[2];
#endif

#ifdef _F_WINDOW_GROUP_RAISE_
             if (e->data.l[0] == 3) // 'e->data.l[0] == 3' means group raise
               {
                  if (bd->client.icccm.client_leader)
                    {
                       _e_manager_windows_group_raise (bd);
                       traceEnd(TTRACE_TAG_WINDOW_MANAGER);
                       return ECORE_CALLBACK_PASS_ON;
                    }
               }
#endif

#ifdef _F_FOCUS_WINDOW_IF_TOP_STACK_
             if ((e_config->focus_setting == E_FOCUS_NEW_WINDOW) ||
                 (e_config->focus_setting == E_FOCUS_NEW_WINDOW_IF_TOP_STACK) ||
                 ((bd->parent) &&
                  ((e_config->focus_setting == E_FOCUS_NEW_DIALOG) ||
                   ((bd->parent->focused) &&
                    (e_config->focus_setting == E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED)))))
               {
                  if (bd->iconic)
                    {
                       if (e_config->clientlist_warp_to_iconified_desktop == 1)
                         e_desk_show(bd->desk);

                       if (!bd->lock_user_iconify)
                         e_border_uniconify(bd);
                    }
                  if ((!bd->iconic) && (!bd->sticky))
                    e_desk_show(bd->desk);
#ifdef _F_DEICONIFY_APPROVE_
                  if (!bd->iconic)
                    {
                       if (!bd->lock_user_stacking)
                         {
#ifdef _F_USE_TILED_DESK_LAYOUT_
                            if (bd != e_border_focused_get())
                              {
                                 if (e_config->use_tiled_desk_layout)
                                   {
                                      if (!bd->client.e.state.ly.changing)
                                        {
                                           if (bd->client.e.state.ly.curr_ly)
                                             {
                                                e_border_desk_layout_set(bd, bd->client.e.state.ly.curr_ly, EINA_TRUE, EINA_TRUE);
                                             }
                                        }
                                   }
                              }
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

                            e_border_raise(bd);
                         }
                    }
#else
                  if (!bd->lock_user_stacking) e_border_raise(bd);
#endif
                  if (!bd->lock_focus_out)
                    {
                       /* XXX ooffice does send this request for
                          config dialogs when the main window gets focus.
                          causing the pointer to jump back and forth.  */
                       if ((e_config->focus_policy != E_FOCUS_CLICK) &&
                           !(bd->client.icccm.name && !strcmp(bd->client.icccm.name, "VCLSalFrame")))
                         ecore_x_pointer_warp(bd->zone->container->win,
                                              bd->x + (bd->w / 2), bd->y + (bd->h / 2));

                       if (!bd->client.icccm.accepts_focus)
                         {
                            Eina_Bool accepts_focus = EINA_FALSE;
                            if (ecore_x_icccm_hints_get(bd->client.win, &accepts_focus, 0, 0, 0, 0, 0, 0))
                              {
                                 bd->client.icccm.accepts_focus = accepts_focus;
                              }
                         }

                       if (e_config->focus_setting != E_FOCUS_NEW_WINDOW_IF_TOP_STACK)
                         e_border_focus_set(bd, 1, 1);
                       else
                         {
                            e_border_focus_latest_set(bd);

                            E_Border* temp_bd = NULL;
                            E_Border_List *bl;
                            E_Border *cur_focus;

                            cur_focus = e_border_focused_get();
                            bl = e_container_border_list_last(bd->zone->container);
                            if (bl)
                              {
                                 while ((temp_bd = e_container_border_list_prev(bl)))
                                   {
                                      if (temp_bd == cur_focus) break;

                                      if ((temp_bd->x >= bd->zone->w) || (temp_bd->y >= bd->zone->h)) continue;
                                      if (((temp_bd->x + temp_bd->w) <= 0) || ((temp_bd->y + temp_bd->h) <= 0)) continue;
                                      if ((temp_bd != bd) &&
                                          (temp_bd->client.illume.win_state.state != ECORE_X_ILLUME_WINDOW_STATE_NORMAL))
                                        continue;

                                      if ((!temp_bd->iconic) && (temp_bd->visible) && (temp_bd->desk == bd->desk) &&
                                          (temp_bd->client.icccm.accepts_focus || temp_bd->client.icccm.take_focus) &&
                                          (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DOCK) &&
                                          (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_TOOLBAR) &&
                                          (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_MENU) &&
                                          (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_SPLASH) &&
                                          (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DESKTOP))
                                        {
                                           e_border_focus_set(temp_bd, 1, 1);
                                           break;
                                        }
                                   }
                                 e_container_border_list_free(bl);
                              }
                         }
                    }
               }
#else // original
             if (!bd->focused) e_border_activate(bd, EINA_FALSE);
             else e_border_raise(bd);
#endif
          }

        traceEnd(TTRACE_TAG_WINDOW_MANAGER);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_manager_frame_extents_free_cb(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__,
				 void *data, void *fdata __UNUSED__)
{
   free(data);
   return EINA_TRUE;
}

static E_Manager *
_e_manager_get_for_root(Ecore_X_Window root)
{
   Eina_List *l;
   E_Manager *man;

   if (!managers) return NULL;
   EINA_LIST_FOREACH(managers, l, man)
     {
	if (man->root == root)
	  return man;
     }
   return eina_list_data_get(managers);
}

static Eina_Bool
_e_manager_clear_timer(void *data)
{
   E_Manager *man = data;
   ecore_x_window_background_color_set(man->root, 0, 0, 0);
   man->clear_timer = NULL;
   return EINA_FALSE;
}

#if 0 /* use later - maybe */
static int _e_manager_cb_window_destroy(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_hide(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_reparent(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_create(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_configure_request(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_configure(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_gravity(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_stack(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_stack_request(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_property(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_colormap(void *data, int ev_type, void *ev){return 1;}
static int _e_manager_cb_window_shape(void *data, int ev_type, void *ev){return 1;}
#endif

#ifdef _F_E_MANAGER_COMP_OBJECT_
static E_Manager_Comp_Object_Item *
_e_manager_comp_object_smart_adopt(E_Smart_Data *sd,
                                   Evas_Object  *obj)
{
   E_Manager_Comp_Object_Item *ci;

   ci = evas_object_data_get(obj, "e_manager_comp_object_data");
   if (ci) e_manager_comp_object_unpack(obj);

   ci = E_NEW(E_Manager_Comp_Object_Item, 1);
   if (!ci) return NULL;
   ci->sd = sd;
   ci->obj = obj;
   /* defaults */
   ci->x = 0;
   ci->y = 0;
   ci->w = 0;
   ci->h = 0;
   evas_object_smart_member_add(obj, ci->sd->obj);
   evas_object_data_set(obj, "e_manager_comp_object_data", ci);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_FREE,
                                  _e_manager_comp_object_smart_item_del_hook, NULL);
   return ci;
}

static void
_e_manager_comp_object_smart_disown(Evas_Object *obj)
{
   E_Manager_Comp_Object_Item *ci;

   ci = evas_object_data_get(obj, "e_manager_comp_object_data");
   if (!ci) return;
   evas_object_event_callback_del(obj, EVAS_CALLBACK_FREE,
                                  _e_manager_comp_object_smart_item_del_hook);
   evas_object_smart_member_del(obj);
   evas_object_data_del(obj, "e_manager_comp_object_data");
   E_FREE(ci);
}

static void
_e_manager_comp_object_smart_item_del_hook(void        *data,
                                           Evas        *e,
                                           Evas_Object *obj,
                                           void        *event_info)
{
   e_manager_comp_object_unpack(obj);
}

static void
_e_manager_comp_object_smart_reconfigure(E_Smart_Data *sd)
{
   if (!sd->changed) return;

   if (sd->child)
     {
        E_Manager_Comp_Object_Item *ci;
        ci = evas_object_data_get(sd->child, "e_manager_comp_object_data");
        _e_manager_comp_object_smart_move_resize_item(ci);
     }

   e_manager_comp_input_region_id_set(sd->man,
                                      sd->input.id,
                                      sd->input.x,
                                      sd->input.y,
                                      sd->input.w,
                                      sd->input.h);
   sd->changed = 0;
}

static void
_e_manager_comp_object_smart_move_resize_item(E_Manager_Comp_Object_Item *ci)
{
   if (!ci) return;
   evas_object_move(ci->obj, ci->sd->x, ci->sd->y);
   evas_object_resize(ci->obj, ci->sd->w, ci->sd->h);
}

static void
_e_manager_comp_object_smart_init(void)
{
   if (_e_smart) return;
     {
        static const Evas_Smart_Class sc =
          {
             "e_manager_comp_object",
             EVAS_SMART_CLASS_VERSION,
             _e_manager_comp_object_smart_add,
             _e_manager_comp_object_smart_del,
             _e_manager_comp_object_smart_move,
             _e_manager_comp_object_smart_resize,
             _e_manager_comp_object_smart_show,
             _e_manager_comp_object_smart_hide,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL
          };
        _e_smart = evas_smart_class_new(&sc);
     }
}

static void
_e_manager_comp_object_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   E_Manager *man;
   int id;

   sd = E_NEW(E_Smart_Data, 1);
   if (!sd) return;

   man = e_manager_current_get();
   id = e_manager_comp_input_region_id_new(man);
   if (id < 0)
     {
        E_FREE(sd);
        return;
     }

   sd->man = man;
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   sd->obj = obj;
   sd->input.id = id;
   sd->input.x = -1;
   sd->input.y = -1;
   sd->input.w = 1;
   sd->input.h = 1;

   evas_object_smart_data_set(obj, sd);
}

static void
_e_manager_comp_object_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->child)
     {
        if (sd->input.managed)
          {
             e_manager_comp_input_region_managed_set(sd->man, sd->input.id, sd->child, EINA_FALSE);
             sd->input.managed = EINA_FALSE;
          }
        e_manager_comp_object_unpack(sd->child);
     }
   e_manager_comp_input_region_id_del(sd->man, sd->input.id);
   E_FREE(sd);
}

static void
_e_manager_comp_object_smart_move(Evas_Object *obj,
                                  Evas_Coord   x,
                                  Evas_Coord   y)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((x == sd->x) && (y == sd->y)) return;

   sd->x = x;
   sd->y = y;

   if (evas_object_visible_get(obj))
     {
        sd->input.x = sd->x;
        sd->input.y = sd->y;
        sd->input.w = sd->w;
        sd->input.h = sd->h;
     }

   if (!sd->child) return;
   sd->changed = 1;
   _e_manager_comp_object_smart_reconfigure(sd);
}

static void
_e_manager_comp_object_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((w == sd->w) && (h == sd->h)) return;

   sd->w = w;
   sd->h = h;
   sd->input.x = sd->x;
   sd->input.y = sd->y;
   sd->input.w = sd->w;
   sd->input.h = sd->h;

   if (!sd->child) return;
   sd->changed = 1;
   _e_manager_comp_object_smart_reconfigure(sd);
}

static void
_e_manager_comp_object_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   sd->input.x = sd->x;
   sd->input.y = sd->y;
   sd->input.w = sd->w;
   sd->input.h = sd->h;
   sd->changed = 1;
   _e_manager_comp_object_smart_reconfigure(sd);
}

static void
_e_manager_comp_object_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd) return;

   sd->input.x = -1;
   sd->input.y = -1;
   sd->input.w = 1;
   sd->input.h = 1;
   sd->changed = 1;
   _e_manager_comp_object_smart_reconfigure(sd);
}
#endif /* end of _F_E_MANAGER_COMP_OBJECT_ */

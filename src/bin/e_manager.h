#ifdef E_TYPEDEFS

typedef struct _E_Manager             E_Manager;
typedef struct _E_Manager_Comp        E_Manager_Comp;
typedef struct _E_Manager_Comp_Source E_Manager_Comp_Source;
#ifdef _F_TEMPORARY_COMP_EVENT_
typedef struct _E_Event_Comp_Source_Visibility E_Event_Comp_Source_Visibility;
#endif /* end of _F_TEMPORARY_COMP_EVENT_ */
#ifdef _F_E_MANAGER_COMP_OBJECT_
typedef struct _E_Manager_Comp_Object          E_Manager_Comp_Object;
#endif /* end of _F_E_MANAGER_COMP_OBJECT_ */

#else
#ifndef E_MANAGER_H
#define E_MANAGER_H

#define E_MANAGER_TYPE (int) 0xE0b01008

struct _E_Manager
{
   E_Object             e_obj_inherit;

   Ecore_X_Window       win;
   int                  num;
   int                  x, y, w, h;
   char                 visible : 1;
   Ecore_X_Window       root;
   Eina_List           *handlers;
   Eina_List           *containers;

   E_Pointer           *pointer;
   Ecore_X_Window       initwin;

   E_Manager_Comp      *comp;
   Ecore_Timer         *clear_timer;
};

struct _E_Manager_Comp
{
  struct {
    Evas             * (*evas_get)             (void *data, E_Manager *man);
    void               (*update)               (void *data, E_Manager *man);
    const Eina_List  * (*src_list_get)         (void *data, E_Manager *man);
    Evas_Object      * (*src_image_get)        (void *data, E_Manager *man, E_Manager_Comp_Source *src);
    Evas_Object      * (*src_shadow_get)       (void *data, E_Manager *man, E_Manager_Comp_Source *src);
    Evas_Object      * (*src_image_mirror_add) (void *data, E_Manager *man, E_Manager_Comp_Source *src);
    Eina_Bool          (*src_visible_get)      (void *data, E_Manager *man, E_Manager_Comp_Source *src);
    void               (*src_hidden_set)       (void *data, E_Manager *man, E_Manager_Comp_Source *src, Eina_Bool hidden);
    Eina_Bool          (*src_hidden_get)       (void *data, E_Manager *man, E_Manager_Comp_Source *src);
    E_Manager_Comp_Source * (*src_get)         (void *data, E_Manager *man, Ecore_X_Window win);
    E_Manager_Comp_Source * (*border_src_get)         (void *data, E_Manager *man, Ecore_X_Window win);
    E_Popup          * (*src_popup_get)        (void *data, E_Manager *man, E_Manager_Comp_Source *src);
    E_Border         * (*src_border_get)       (void *data, E_Manager *man, E_Manager_Comp_Source *src);
    Ecore_X_Window     (*src_window_get)       (void *data, E_Manager *man, E_Manager_Comp_Source *src);
#ifdef _F_COMP_SCREEN_LOCK_
    void               (*screen_lock)          (void *data, E_Manager *man);
    void               (*screen_unlock)        (void *data, E_Manager *man);
#endif
#ifdef _F_COMP_INPUT_REGION_SET_
    Eina_Bool          (*src_input_region_set) (void *data, E_Manager *man, E_Manager_Comp_Source *src, int x, int y, int w, int h);
    int                (*input_region_new)     (void *data, E_Manager *man);
    Eina_Bool          (*input_region_set)     (void *data, E_Manager *man, int id, int x, int y, int w, int h);
    Eina_Bool          (*input_region_managed_set)     (void *data, E_Manager *man, int id, Evas_Object *obj, Eina_Bool set);
    Eina_Bool          (*input_region_del)     (void *data, E_Manager *man, int id);
#endif
#ifdef _F_COMP_MOVE_LOCK_
    Eina_Bool          (*src_move_lock)        (void *data, E_Manager *man, E_Manager_Comp_Source *src);
    Eina_Bool          (*src_move_unlock)      (void *data, E_Manager *man, E_Manager_Comp_Source *src);
#endif
#ifdef _F_COMP_COMPOSITE_MODE_
    void               (*composite_mode_set)   (void *data, E_Manager *man, E_Zone *zone, Eina_Bool set);
    Eina_Bool          (*composite_mode_get)   (void *data, E_Manager *man, E_Zone *zone);
#endif
#ifdef _F_USE_EXTENDED_ICONIFY_
    void               (*src_shadow_show)      (void *data, E_Manager *man, E_Manager_Comp_Source *src);
    void               (*src_shadow_hide)      (void *data, E_Manager *man, E_Manager_Comp_Source *src);
#endif
#ifdef _F_COMP_LAYER_
    Evas_Object      * (*layer_get)            (void *data, E_Manager *man, E_Zone *zone, const char *name);
    void               (*layer_raise_above)    (void *data, E_Manager *man, E_Zone *zone, const char *name, E_Border *bd);
    void               (*layer_lower_below)    (void *data, E_Manager *man, E_Zone *zone, const char *name, E_Border *bd);
#endif
  } func;
  void                   *data;
};

#ifdef _F_E_MANAGER_COMP_OBJECT_
struct _E_Manager_Comp_Object
{
   E_Manager   *man;
   Evas_Object *o;
   int          input_reg;
   int          x, y, w, h;
};
#endif /* end of _F_E_MANAGER_COMP_OBJECT_ */

EINTERN int        e_manager_init(void);
EINTERN int        e_manager_shutdown(void);
EAPI Eina_List *e_manager_list(void);

EAPI E_Manager      *e_manager_new(Ecore_X_Window root, int num);
EAPI void            e_manager_manage_windows(E_Manager *man);
EAPI void            e_manager_show(E_Manager *man);
EAPI void            e_manager_hide(E_Manager *man);
EAPI void            e_manager_move(E_Manager *man, int x, int y);
EAPI void            e_manager_resize(E_Manager *man, int w, int h);
EAPI void            e_manager_move_resize(E_Manager *man, int x, int y, int w, int h);
EAPI void            e_manager_raise(E_Manager *man);
EAPI void            e_manager_lower(E_Manager *man);
EAPI E_Manager      *e_manager_current_get(void);
EAPI E_Manager      *e_manager_number_get(int num);

EAPI void            e_managers_keys_grab(void);
EAPI void            e_managers_keys_ungrab(void);

#ifdef _F_TEMPORARY_COMP_EVENT_
/* Later, these events will be moved to e_comp.h */
struct _E_Event_Comp_Source_Visibility
{
   Ecore_X_Window win;
   Eina_Bool      visible;
};

extern EAPI int E_EVENT_COMP_SOURCE_VISIBILITY;
extern EAPI int E_EVENT_COMP_SOURCE_ADD;
extern EAPI int E_EVENT_COMP_SOURCE_DEL;
extern EAPI int E_EVENT_COMP_SOURCE_CONFIGURE;
extern EAPI int E_EVENT_COMP_SOURCE_STACK;
#endif /* end of _F_TEMPORARY_COMP_EVENT_ */

// tenative api for e's compositor to advertise to the rest of e the comp
// canvas. on comp evas register (set evas) we send:
//   e_msg_send("comp.manager", "comp.change", 0, man);
// so to hook up to it:
//   static void handler(void *data, const char *name, const char *info, int val, E_Object *obj, void *msgdata)
//   {
//     if (!strcmp(name, "comp.manager"))
//       {
//         if (!strcmp(info, "change.comp"))
//           { // compositor canvas added or deleted
//             Evas *e = e_manager_comp_evas_get((E_Manager *)obj);
//             if (!e) printf("No comp manager\n");
//             else printf("comp canvas = %p\n", e);
//           }
/// FIXME: implement below
//         else if (!strcmp(info, "resize.comp"))
//           { // compositor canvas resized
//           }
//         else if (!strcmp(info, "add.src"))
//           { // compositor source added
//           }
//         else if (!strcmp(info, "del.src"))
//           { // compositor source deleted
//           }
//         else if (!strcmp(info, "config.src"))
//           { // compositor src reconfigured (moved, resized)
//           }
//         else if (!strcmp(info, "visible.src"))
//           { // compositor src shown or hidden
//           }
//       }
//   }
//   e_msg_handler_add(handler, mydata);
//
// remember to listen to zone confiugre events like:
// E_EVENT_ZONE_MOVE_RESIZE
// E_EVENT_ZONE_ADD
// E_EVENT_ZONE_DEL
//
// only 1 compositor can own a manager at a time, so before you "set" the
// comp evas, you need to get it and make sure it's NULL, if so, then
// you can set the update func and the comp evas
EAPI void             e_manager_comp_set(E_Manager *man, E_Manager_Comp *comp);
EAPI Evas            *e_manager_comp_evas_get(E_Manager *man);
EAPI void             e_manager_comp_evas_update(E_Manager *man);
EAPI const Eina_List *e_manager_comp_src_list(E_Manager *man);
EAPI Evas_Object     *e_manager_comp_src_image_get(E_Manager *man, E_Manager_Comp_Source *src);
EAPI Evas_Object     *e_manager_comp_src_shadow_get(E_Manager *man, E_Manager_Comp_Source *src);
EAPI Evas_Object     *e_manager_comp_src_image_mirror_add(E_Manager *man, E_Manager_Comp_Source *src);
EAPI Eina_Bool        e_manager_comp_src_visible_get(E_Manager *man, E_Manager_Comp_Source *src);
EAPI void             e_manager_comp_src_hidden_set(E_Manager *man, E_Manager_Comp_Source *src, Eina_Bool hidden);
EAPI Eina_Bool        e_manager_comp_src_hidden_get(E_Manager *man, E_Manager_Comp_Source *src);
EAPI void             e_manager_comp_event_resize_send(E_Manager *man);
EAPI void             e_manager_comp_event_src_add_send(E_Manager *man, E_Manager_Comp_Source *src, void (*afterfunc) (void *data, E_Manager *man, E_Manager_Comp_Source *src), void *data);
EAPI void             e_manager_comp_event_src_del_send(E_Manager *man, E_Manager_Comp_Source *src, void (*afterfunc) (void *data, E_Manager *man, E_Manager_Comp_Source *src), void *data);
EAPI void             e_manager_comp_event_src_config_send(E_Manager *man, E_Manager_Comp_Source *src, void (*afterfunc) (void *data, E_Manager *man, E_Manager_Comp_Source *src), void *data);
EAPI void             e_manager_comp_event_src_visibility_send(E_Manager *man, E_Manager_Comp_Source *src, void (*afterfunc) (void *data, E_Manager *man, E_Manager_Comp_Source *src), void *data);
EAPI E_Manager_Comp_Source *e_manager_comp_src_get(E_Manager *man, Ecore_X_Window win);
EAPI E_Manager_Comp_Source *e_manager_comp_border_src_get(E_Manager *man, Ecore_X_Window win);
EAPI E_Popup         *e_manager_comp_src_popup_get(E_Manager *man, E_Manager_Comp_Source *src);
EAPI E_Border        *e_manager_comp_src_border_get(E_Manager *man, E_Manager_Comp_Source *src);
EAPI Ecore_X_Window   e_manager_comp_src_window_get(E_Manager *man, E_Manager_Comp_Source *src);
#ifdef _F_COMP_SCREEN_LOCK_
EAPI void             e_manager_comp_screen_lock(E_Manager *man);
EAPI void             e_manager_comp_screen_unlock(E_Manager *man);
#endif
#ifdef _F_COMP_INPUT_REGION_SET_
EAPI Eina_Bool        e_manager_comp_src_input_region_set(E_Manager *man, E_Manager_Comp_Source *src, int x, int y, int w, int h);
EAPI int              e_manager_comp_input_region_new(E_Manager *man);
EAPI Eina_Bool        e_manager_comp_input_region_set(E_Manager *man, int id, int x, int y, int w, int h);
EAPI Eina_Bool        e_manager_comp_input_region_managed_set(E_Manager *man, int id, Evas_Object *obj, Eina_Bool set);
EAPI Eina_Bool        e_manager_comp_input_region_del(E_Manager *man, int id);
EAPI int              e_manager_comp_input_region_id_new(E_Manager *man);
EAPI Eina_Bool        e_manager_comp_input_region_id_set(E_Manager *man, int id, int x, int y, int w, int h);
EAPI Eina_Bool        e_manager_comp_input_region_id_del(E_Manager *man, int id);
#endif
#ifdef _F_COMP_MOVE_LOCK_
EAPI Eina_Bool        e_manager_comp_src_move_lock(E_Manager *man, E_Manager_Comp_Source *src);   /* deprecated */
EAPI Eina_Bool        e_manager_comp_src_move_unlock(E_Manager *man, E_Manager_Comp_Source *src); /* deprecated */
#endif
#ifdef _F_COMP_COMPOSITE_MODE_
// set the composite rendering state of a zone.
// if set is EINA_TRUE, then composite module will be running with composite rendering mode.
// if set is EINA_FALSE, then composite module may be running with nocomposite mode on given zone.
EAPI void             e_manager_comp_composite_mode_set(E_Manager *man, E_Zone *zone, Eina_Bool set);
// get the composite rendering state of a zone.
// if return value is EINA_TRUE, zone is rendered with composite mode.
EAPI Eina_Bool        e_manager_comp_composite_mode_get(E_Manager *man, E_Zone *zone);
#endif
#ifdef _F_USE_EXTENDED_ICONIFY_
EAPI void             e_manager_comp_src_shadow_show(E_Manager *man, E_Manager_Comp_Source *src);
EAPI void             e_manager_comp_src_shadow_hide(E_Manager *man, E_Manager_Comp_Source *src);
#endif
#ifdef _F_COMP_LAYER_
// get compositor's layer by specific name
EAPI Evas_Object     *e_manager_comp_layer_get(E_Manager *man, E_Zone *zone, const char *name);
EAPI void             e_manager_comp_layer_raise_above(E_Manager *man, E_Zone *zone, const char *name, E_Border *bd);
EAPI void             e_manager_comp_layer_lower_below(E_Manager *man, E_Zone *zone, const char *name, E_Border *bd);
#endif
#ifdef _F_E_MANAGER_COMP_OBJECT_
EAPI Evas_Object     *e_manager_comp_object_add(Evas *evas);
EAPI void             e_manager_comp_object_pack(Evas_Object *obj, Evas_Object *child);
EAPI void             e_manager_comp_object_unpack(Evas_Object *obj);
EAPI void             e_manager_comp_object_managed_set(Evas_Object *obj, Eina_Bool set);
#endif /* end of _F_E_MANAGER_COMP_OBJECT_ */
#endif
#endif
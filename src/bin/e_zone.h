#ifdef E_TYPEDEFS

typedef enum _E_Zone_Edge
{
   E_ZONE_EDGE_NONE,
   E_ZONE_EDGE_LEFT,
   E_ZONE_EDGE_RIGHT,
   E_ZONE_EDGE_TOP,
   E_ZONE_EDGE_BOTTOM,
   E_ZONE_EDGE_TOP_LEFT,
   E_ZONE_EDGE_TOP_RIGHT,
   E_ZONE_EDGE_BOTTOM_RIGHT,
   E_ZONE_EDGE_BOTTOM_LEFT
} E_Zone_Edge;

typedef struct _E_Zone                      E_Zone;

typedef struct _E_Event_Zone_Desk_Count_Set E_Event_Zone_Desk_Count_Set;
typedef struct _E_Event_Zone_Move_Resize    E_Event_Zone_Move_Resize;
typedef struct _E_Event_Zone_Add            E_Event_Zone_Add;
typedef struct _E_Event_Zone_Del            E_Event_Zone_Del;
/* TODO: Move this to a general place? */
typedef struct _E_Event_Pointer_Warp        E_Event_Pointer_Warp;
typedef struct _E_Event_Zone_Edge           E_Event_Zone_Edge;
#ifdef _F_ZONE_WINDOW_ROTATION_
typedef struct _E_Event_Zone_Rotation_Change E_Event_Zone_Rotation_Change;
#endif

#else
#ifndef E_ZONE_H
#define E_ZONE_H

#define E_ZONE_TYPE (int)0xE0b0100d

struct _E_Zone
{
   E_Object     e_obj_inherit;

   int          x, y, w, h;
   const char  *name;
   /* num matches the id of the xinerama screen
    * this zone belongs to. */
   unsigned int num;
   E_Container *container;
   int          fullscreen;

   Evas_Object *bg_object;
   Evas_Object *bg_event_object;
   Evas_Object *bg_clip_object;
   Evas_Object *prev_bg_object;
   Evas_Object *transition_object;

   int          desk_x_count, desk_y_count;
   int          desk_x_current, desk_y_current;
   E_Desk     **desks;

   Eina_List   *handlers;

   struct
   {
      unsigned char      switching : 1;
      E_Shelf           *es;
      E_Event_Zone_Edge *ev;
      E_Binding_Edge    *bind;
   } flip;

   struct
   {
      Ecore_X_Window top, right, bottom, left;
   } edge;
   struct
   {
      Ecore_X_Window left_top, top_left, top_right, right_top,
                     right_bottom, bottom_right, bottom_left, left_bottom;
   } corner;

   E_Action      *cur_mouse_action;
   Eina_List     *popups;

   Ecore_Evas    *black_ecore_evas;
   Evas          *black_evas;
   Ecore_X_Window black_win;
   int            black_need;
   int            id;

   struct
   {
      int       x, y, w, h;
      Eina_Bool dirty : 1;
   } useful_geometry;
#ifdef _F_ZONE_WINDOW_ROTATION_
   struct
   {
      int       prev, curr, next;
      Eina_Bool wait_for_done : 1;
      Eina_Bool pending : 1;
   } rot;
#endif
};

struct _E_Event_Zone_Desk_Count_Set
{
   E_Zone *zone;
};

struct _E_Event_Zone_Move_Resize
{
   E_Zone *zone;
};

struct _E_Event_Zone_Add
{
   E_Zone *zone;
};

struct _E_Event_Zone_Del
{
   E_Zone *zone;
};

struct _E_Event_Pointer_Warp
{
   struct
   {
      int x, y;
   } prev;
   struct
   {
      int x, y;
   } curr;
};

struct _E_Event_Zone_Edge
{
   E_Zone     *zone;
   E_Zone_Edge edge;
   int         x, y;
   int         modifiers;
   int         button;
};

#ifdef _F_ZONE_WINDOW_ROTATION_
struct _E_Event_Zone_Rotation_Change
{
   E_Zone     *zone;
};
#endif

EINTERN int    e_zone_init(void);
EINTERN int    e_zone_shutdown(void);
EAPI E_Zone   *e_zone_new(E_Container *con, int num, int id, int x, int y, int w, int h);
EAPI void      e_zone_name_set(E_Zone *zone, const char *name);
EAPI void      e_zone_move(E_Zone *zone, int x, int y);
EAPI void      e_zone_resize(E_Zone *zone, int w, int h);
EAPI void      e_zone_move_resize(E_Zone *zone, int x, int y, int w, int h);
EAPI void      e_zone_fullscreen_set(E_Zone *zone, int on);
EAPI E_Zone   *e_zone_current_get(E_Container *con);
EAPI void      e_zone_bg_reconfigure(E_Zone *zone);
EAPI void      e_zone_flip_coords_handle(E_Zone *zone, int x, int y);
EAPI void      e_zone_desk_count_set(E_Zone *zone, int x_count, int y_count);
EAPI void      e_zone_desk_count_get(E_Zone *zone, int *x_count, int *y_count);
EAPI void      e_zone_desk_flip_by(E_Zone *zone, int dx, int dy);
EAPI void      e_zone_desk_flip_to(E_Zone *zone, int x, int y);
EAPI void      e_zone_desk_linear_flip_by(E_Zone *zone, int dx);
EAPI void      e_zone_desk_linear_flip_to(E_Zone *zone, int x);
EAPI void      e_zone_edge_flip_eval(E_Zone *zone);
EAPI void      e_zone_edge_new(E_Zone_Edge edge);
EAPI void      e_zone_edge_free(E_Zone_Edge edge);
EAPI void      e_zone_edge_enable(void);
EAPI void      e_zone_edge_disable(void);
EAPI void      e_zone_edges_desk_flip_capable(E_Zone *zone, Eina_Bool l, Eina_Bool r, Eina_Bool t, Eina_Bool b);
EAPI Eina_Bool e_zone_exists_direction(E_Zone *zone, E_Zone_Edge edge);
EAPI void      e_zone_edge_win_layer_set(E_Zone *zone, int layer);

EAPI void      e_zone_useful_geometry_dirty(E_Zone *zone);
EAPI void      e_zone_useful_geometry_get(E_Zone *zone, int *x, int *y, int *w, int *h);

#ifdef _F_ZONE_WINDOW_ROTATION_
EAPI void      e_zone_rotation_set(E_Zone *zone, int rot);
EAPI int       e_zone_rotation_get(E_Zone *zone);
EAPI void      e_zone_rotation_update_done(E_Zone *zone);
EAPI void      e_zone_rotation_update_cancel(E_Zone *zone);
#endif

extern EAPI int E_EVENT_ZONE_DESK_COUNT_SET;
extern EAPI int E_EVENT_ZONE_MOVE_RESIZE;
extern EAPI int E_EVENT_ZONE_ADD;
extern EAPI int E_EVENT_ZONE_DEL;
extern EAPI int E_EVENT_POINTER_WARP;
extern EAPI int E_EVENT_ZONE_EDGE_IN;
extern EAPI int E_EVENT_ZONE_EDGE_OUT;
extern EAPI int E_EVENT_ZONE_EDGE_MOVE;
#ifdef _F_ZONE_WINDOW_ROTATION_
extern EAPI int E_EVENT_ZONE_ROTATION_CHANGE;
#endif

#endif
#endif

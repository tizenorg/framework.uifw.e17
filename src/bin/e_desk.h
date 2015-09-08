#ifdef E_TYPEDEFS

typedef struct _E_Desk E_Desk;
typedef struct _E_Event_Desk_Show E_Event_Desk_Show;
typedef struct _E_Event_Desk_Before_Show E_Event_Desk_Before_Show;
typedef struct _E_Event_Desk_After_Show E_Event_Desk_After_Show;
typedef struct _E_Event_Desk_DeskShow E_Event_Desk_DeskShow;
typedef struct _E_Event_Desk_Name_Change E_Event_Desk_Name_Change;
#ifdef _F_USE_DESK_WINDOW_PROFILE_
typedef struct _E_Event_Desk_Window_Profile_Change E_Event_Desk_Window_Profile_Change;
#endif
#ifdef _F_USE_TILED_DESK_LAYOUT_
typedef struct _E_Desk_Layout              E_Desk_Layout;
typedef struct _E_Event_Desk_Layout_Simple E_Event_Desk_Layout_Add;
typedef struct _E_Event_Desk_Layout_Simple E_Event_Desk_Layout_Remove;
typedef struct _E_Event_Desk_Layout_Simple E_Event_Desk_Layout_Move_Resize;
typedef struct _E_Event_Desk_Layout_Simple E_Event_Desk_Layout_Show;
typedef struct _E_Event_Desk_Layout_Simple E_Event_Desk_Layout_Hide;
typedef struct _E_Event_Desk_Layout_Simple E_Event_Desk_Layout_Active;
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

#else
#ifndef E_DESK_H
#define E_DESK_H

#define E_DESK_TYPE 0xE0b01005

#ifdef _F_USE_TILED_DESK_LAYOUT_
#define E_DESK_LAYOUT_TYPE (int)0xE9b01001
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

struct _E_Desk
{
   E_Object             e_obj_inherit;

   E_Zone              *zone;
   const char          *name;
#ifdef _F_USE_DESK_WINDOW_PROFILE_
   const char          *window_profile;
#endif
   int                  x, y;
   unsigned char        visible : 1;
   unsigned int         deskshow_toggle : 1;
   int                  fullscreen_borders;

   Evas_Object         *bg_object;

   Ecore_Animator      *animator;
   Eina_Bool            animating;
#ifdef _F_USE_TILED_DESK_LAYOUT_
   struct
   {
      Eina_Bool         tiled;
      Eina_Inlist      *list;
      E_Desk_Layout    *active_ly;
      Eina_Bool         changed;
   } ly;
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
};

#ifdef _F_USE_TILED_DESK_LAYOUT_
struct _E_Desk_Layout
{
   EINA_INLIST;

   E_Object             e_obj_inherit;

   E_Desk              *desk;
   Eina_Bool            changed;
   int                  x, y, w, h;
   int                  id;
   //Eina_Bool            use;
   //Eina_List           *borders;
};
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

struct _E_Event_Desk_Show
{
   E_Desk   *desk;
};

struct _E_Event_Desk_Before_Show
{
   E_Desk   *desk;
};

struct _E_Event_Desk_After_Show
{
   E_Desk   *desk;
};

struct _E_Event_Desk_Name_Change
{
   E_Desk   *desk;
};

#ifdef _F_USE_DESK_WINDOW_PROFILE_
struct _E_Event_Desk_Window_Profile_Change
{
   E_Desk   *desk;
};
#endif

#ifdef _F_USE_TILED_DESK_LAYOUT_
struct _E_Event_Desk_Layout_Simple
{
   E_Desk_Layout *desk_ly;
};
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

EINTERN int          e_desk_init(void);
EINTERN int          e_desk_shutdown(void);
EAPI E_Desk      *e_desk_new(E_Zone *zone, int x, int y);
EAPI void         e_desk_name_set(E_Desk *desk, const char *name);
EAPI void         e_desk_name_add(int container, int zone, int desk_x, int desk_y, const char *name);
EAPI void         e_desk_name_del(int container, int zone, int desk_x, int desk_y);
EAPI void         e_desk_name_update(void);
EAPI void         e_desk_show(E_Desk *desk);
EAPI void         e_desk_deskshow(E_Zone *zone);
EAPI void         e_desk_last_focused_focus(E_Desk *desk);
EAPI E_Desk      *e_desk_current_get(E_Zone *zone);
EAPI E_Desk      *e_desk_at_xy_get(E_Zone *zone, int x, int y);
EAPI E_Desk      *e_desk_at_pos_get(E_Zone *zone, int pos);
EAPI void         e_desk_xy_get(E_Desk *desk, int *x, int *y);
EAPI void         e_desk_next(E_Zone *zone);
EAPI void         e_desk_prev(E_Zone *zone);
EAPI void         e_desk_row_add(E_Zone *zone);
EAPI void         e_desk_row_remove(E_Zone *zone);
EAPI void         e_desk_col_add(E_Zone *zone);
EAPI void         e_desk_col_remove(E_Zone *zone);
#ifdef _F_USE_DESK_WINDOW_PROFILE_
EAPI void         e_desk_window_profile_set(E_Desk *desk, const char *profile);
EAPI void         e_desk_window_profile_add(int container, int zone, int desk_x, int desk_y, const char *profile);
EAPI void         e_desk_window_profile_del(int container, int zone, int desk_x, int desk_y);
EAPI void         e_desk_window_profile_update(void);
#endif
#ifdef _F_USE_TILED_DESK_LAYOUT_
EAPI void           e_desk_active_desk_layout_set(E_Desk *desk, E_Desk_Layout *desk_ly);
EAPI E_Desk_Layout *e_desk_active_desk_layout_get(E_Desk *desk);

EAPI E_Desk_Layout *e_desk_layout_new(E_Desk *desk, int id, int x, int y, int w, int h);
EAPI void           e_desk_layout_move_resize(E_Desk_Layout *desk_ly, int x, int y, int w, int h);
EAPI E_Desk_Layout *e_desk_layout_get(E_Desk *desk, int id);
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

extern EAPI int E_EVENT_DESK_SHOW;
extern EAPI int E_EVENT_DESK_BEFORE_SHOW;
extern EAPI int E_EVENT_DESK_AFTER_SHOW;
extern EAPI int E_EVENT_DESK_DESKSHOW;
extern EAPI int E_EVENT_DESK_NAME_CHANGE;
#ifdef _F_USE_DESK_WINDOW_PROFILE_
extern EAPI int E_EVENT_DESK_WINDOW_PROFILE_CHANGE;
#endif
#ifdef _F_USE_TILED_DESK_LAYOUT_
extern EAPI int E_EVENT_DESK_LAYOUT_ADD;
extern EAPI int E_EVENT_DESK_LAYOUT_REMOVE;
extern EAPI int E_EVENT_DESK_LAYOUT_MOVE_RESIZE;
extern EAPI int E_EVENT_DESK_LAYOUT_SHOW;
extern EAPI int E_EVENT_DESK_LAYOUT_HIDE;
extern EAPI int E_EVENT_DESK_LAYOUT_ACTIVE;
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

#endif
#endif

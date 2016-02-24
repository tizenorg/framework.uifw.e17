/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * This file is a modified version of BSD licensed file and
 * licensed under the Flora License, Version 1.1 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Please, see the COPYING file for the original copyright owner and
 * license.
 */
#include "e.h"

#include <pixman.h>
#include <X11/extensions/Xrender.h>
//#define INOUTDEBUG_MOUSE 1
//#define INOUTDEBUG_FOCUS 1

#ifdef _F_USE_BORDER_TRANSFORM_
#include <X11/extensions/Xcomposite.h>
#endif

#include "dlog.h"

#ifdef _F_USE_WM_ZONE_HOST_
#define _GNU_SOURCE
#include <vasum.h>
#endif

#undef LOG_TAG
#define LOG_TAG "E17"

/* These are compatible with netwm */
#define RESIZE_TL   0
#define RESIZE_T    1
#define RESIZE_TR   2
#define RESIZE_R    3
#define RESIZE_BR   4
#define RESIZE_B    5
#define RESIZE_BL   6
#define RESIZE_L    7
#define MOVE        8
#define RESIZE_NONE 11

/* local subsystem functions */
static void _e_border_pri_raise(E_Border *bd);
static void _e_border_pri_norm(E_Border *bd);
static void _e_border_free(E_Border *bd);
static void _e_border_del(E_Border *bd);

#ifdef PRINT_LOTS_OF_DEBUG
#define E_PRINT_BORDER_INFO(X) \
  _e_border_print(X, __PRETTY_FUNC__)

static void _e_border_print(E_Border   *bd,
                            const char *func);
#endif

/* FIXME: these likely belong in a separate icccm/client handler */
/* and the border needs to become a dumb object that just does what its */
/* told to do */
static Eina_Bool _e_border_cb_window_show_request(void *data,
                                                  int   ev_type,
                                                  void *ev);
static Eina_Bool _e_border_cb_window_destroy(void *data,
                                             int   ev_type,
                                             void *ev);
static Eina_Bool _e_border_cb_window_hide(void *data,
                                          int   ev_type,
                                          void *ev);
static Eina_Bool _e_border_cb_window_reparent(void *data,
                                              int   ev_type,
                                              void *ev);
static Eina_Bool _e_border_cb_window_configure_request(void *data,
                                                       int   ev_type,
                                                       void *ev);
static Eina_Bool _e_border_cb_window_resize_request(void *data,
                                                    int   ev_type,
                                                    void *ev);
static Eina_Bool _e_border_cb_window_gravity(void *data,
                                             int   ev_type,
                                             void *ev);
static Eina_Bool _e_border_cb_window_stack_request(void *data,
                                                   int   ev_type,
                                                   void *ev);
static Eina_Bool _e_border_cb_window_property(void *data,
                                              int   ev_type,
                                              void *ev);
static Eina_Bool _e_border_cb_window_colormap(void *data,
                                              int   ev_type,
                                              void *ev);
static Eina_Bool _e_border_cb_window_shape(void *data,
                                           int   ev_type,
                                           void *ev);
static Eina_Bool _e_border_cb_window_focus_in(void *data,
                                              int   ev_type,
                                              void *ev);
static Eina_Bool _e_border_cb_window_focus_out(void *data,
                                               int   ev_type,
                                               void *ev);
static Eina_Bool _e_border_cb_client_message(void *data,
                                             int   ev_type,
                                             void *ev);

static Eina_Bool _e_border_cb_window_state_request(void *data,
                                                   int   ev_type,
                                                   void *ev);
static Eina_Bool _e_border_cb_window_move_resize_request(void *data,
                                                         int   ev_type,
                                                         void *ev);
static Eina_Bool _e_border_cb_desktop_change(void *data,
                                             int   ev_type,
                                             void *ev);
static Eina_Bool _e_border_cb_sync_alarm(void *data,
                                         int   ev_type,
                                         void *ev);
#ifndef _F_DISABLE_E_EFREET_
static Eina_Bool _e_border_cb_efreet_cache_update(void *data,
                                                  int   ev_type,
                                                  void *ev);
#endif
static Eina_Bool _e_border_cb_config_icon_theme(void *data,
                                                int   ev_type,
                                                void *ev);

static Eina_Bool _e_border_cb_pointer_warp(void *data,
                                           int   ev_type,
                                           void *ev);
static void _e_border_cb_signal_bind(void        *data,
                                     Evas_Object *obj,
                                     const char  *emission,
                                     const char  *source);
static Eina_Bool _e_border_cb_mouse_in(void *data,
                                       int   type,
                                       void *event);
static Eina_Bool _e_border_cb_mouse_out(void *data,
                                        int   type,
                                        void *event);
static Eina_Bool _e_border_cb_mouse_wheel(void *data,
                                          int   type,
                                          void *event);
static Eina_Bool _e_border_cb_mouse_down(void *data,
                                         int   type,
                                         void *event);
static Eina_Bool _e_border_cb_mouse_up(void *data,
                                       int   type,
                                       void *event);
static Eina_Bool _e_border_cb_mouse_move(void *data,
                                         int   type,
                                         void *event);
static Eina_Bool _e_border_cb_grab_replay(void *data,
                                          int   type,
                                          void *event);
static void _e_border_cb_drag_finished(E_Drag *drag,
                                       int     dropped);
#ifdef _F_USE_DESK_WINDOW_PROFILE_
static Eina_Bool _e_border_cb_desk_window_profile_change(void *data,
                                                         int   ev_type,
                                                         void *ev);
#endif
#ifdef _F_ZONE_WINDOW_ROTATION_
static Eina_Bool _e_border_cb_zone_rotation_change_begin(void *data,
                                                         int   ev_type,
                                                         void *ev);
static Eina_Bool _e_border_rotation_change_prepare_timeout(void *data);
static void      _e_border_rotation_list_send(void);
static void      _e_border_rotation_change_message_send(E_Border *bd);
static void      _e_border_rotation_change_request(E_Border *bd, int rotation);

static Eina_Bool _e_border_rotation_change_done_timeout(void *data);
static void      _e_border_rotation_change_done(void);
static Eina_Bool _e_border_rotation_geom_get(E_Border  *bd,
                                             E_Zone    *zone,
                                             int        ang,
                                             int       *x,
                                             int       *y,
                                             int       *w,
                                             int       *h,
                                             Eina_Bool *move);
static void      _e_border_rotation_list_remove(E_Border *bd);
static Eina_Bool _e_border_rotation_prepare_send(E_Border *bd, int rotation);
static Eina_Bool _e_border_rotation_zone_set(E_Zone *zone);
static int       _e_border_rotation_curr_next_get(E_Border *bd);
static Eina_Bool _e_border_rotation_is_dependent_parent(E_Border *bd);
static Eina_Bool _e_border_is_vkbd(E_Border *bd);
static Eina_Bool _e_border_cb_window_configure(void *data,
                                               int   ev_type,
                                               void *ev);
static Eina_Bool _e_border_vkbd_show_prepare_timeout(void *data);
static Eina_Bool _e_border_vkbd_hide_prepare_timeout(void *data);
static void      _e_border_vkbd_show(E_Border *bd);
static void      _e_border_vkbd_hide(E_Border *bd, Eina_Bool clean);
#endif
#ifdef _F_TEMPORARY_COMP_EVENT_
static Eina_Bool _e_border_cb_comp_src_visibility(void *data,
                                                  int   ev_type,
                                                  void *event);
#endif
#ifdef _F_USE_TILED_DESK_LAYOUT_
static void      _e_border_event_border_desk_layout_set_begin_free(void *data,
                                                                   void *ev);
static void      _e_border_event_border_desk_layout_set_end_free(void *data,
                                                                 void *ev);
static Eina_Bool _prop_card32_get(Ecore_X_Window win,
                                  Ecore_X_Atom   atom,
                                  int           *ret_val);
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
static void      _e_border_move_resize_internal(E_Border *bd,
                                                int       x,
                                                int       y,
                                                int       w,
                                                int       h,
                                                Eina_Bool without_border,
                                                Eina_Bool move);

static void      _e_border_eval(E_Border *bd);
static void      _e_border_eval0(E_Border *bd);
static void      _e_border_container_layout_hook(E_Container *con);

static void      _e_border_moveinfo_gather(E_Border   *bd,
                                           const char *source);
static void      _e_border_resize_handle(E_Border *bd);

static Eina_Bool _e_border_shade_animator(void *data);

static void      _e_border_event_border_add_free(void *data,
                                                 void *ev);
static void      _e_border_event_border_remove_free(void *data,
                                                    void *ev);
static void      _e_border_event_border_zone_set_free(void *data,
                                                      void *ev);
static void      _e_border_event_border_desk_set_free(void *data,
                                                      void *ev);
static void      _e_border_event_border_stack_free(void *data,
                                                   void *ev);
static void      _e_border_event_border_icon_change_free(void *data,
                                                         void *ev);
static void      _e_border_event_border_urgent_change_free(void *data,
                                                           void *ev);
static void      _e_border_event_border_focus_in_free(void *data,
                                                      void *ev);
static void      _e_border_event_border_focus_out_free(void *data,
                                                       void *ev);
static void      _e_border_event_border_resize_free(void *data,
                                                    void *ev);
static void      _e_border_event_border_move_free(void *data,
                                                  void *ev);
static void      _e_border_event_border_show_free(void *data,
                                                  void *ev);
static void      _e_border_event_border_hide_free(void *data,
                                                  void *ev);
static void      _e_border_event_border_iconify_free(void *data,
                                                     void *ev);
static void      _e_border_event_border_uniconify_free(void *data,
                                                       void *ev);
#ifdef _F_DEICONIFY_APPROVE_
static void       _e_border_event_border_force_render_free(void *data,
                                                                     void *ev);
#endif
static void      _e_border_event_border_stick_free(void *data,
                                                   void *ev);
static void      _e_border_event_border_unstick_free(void *data,
                                                     void *ev);
static void      _e_border_event_border_property_free(void *data,
                                                      void *ev);
static void      _e_border_event_border_fullscreen_free(void *data,
                                                        void *ev);
static void      _e_border_event_border_unfullscreen_free(void *data,
                                                          void *ev);
#ifdef _F_ZONE_WINDOW_ROTATION_
static void      _e_border_event_border_rotation_change_begin_free(void *data,
                                                                   void *ev);
static void      _e_border_event_border_rotation_change_cancel_free(void *data,
                                                                    void *ev);
static void      _e_border_event_border_rotation_change_end_free(void *data,
                                                                 void *ev);
static void      _e_border_event_border_rotation_change_begin_send(E_Border *bd);
#endif

static void      _e_border_zone_update(E_Border *bd);

static int       _e_border_resize_begin(E_Border *bd);
static int       _e_border_resize_end(E_Border *bd);
static void      _e_border_resize_update(E_Border *bd);

static int       _e_border_move_begin(E_Border *bd);
static int       _e_border_move_end(E_Border *bd);
static void      _e_border_move_update(E_Border *bd);

static Eina_Bool _e_border_cb_ping_poller(void *data);
static Eina_Bool _e_border_cb_kill_timer(void *data);

static void      _e_border_pointer_resize_begin(E_Border *bd);
static void      _e_border_pointer_resize_end(E_Border *bd);
static void      _e_border_pointer_move_begin(E_Border *bd);
static void      _e_border_pointer_move_end(E_Border *bd);

static void      _e_border_hook_call(E_Border_Hook_Point hookpoint,
                                     void               *bd);

static void _e_border_client_move_resize_send(E_Border *bd);

static void _e_border_frame_replace(E_Border *bd,
            Eina_Bool argb);

static void _e_border_shape_input_rectangle_set(E_Border* bd);
static void _e_border_show(E_Border *bd);
static void _e_border_hide(E_Border *bd);

#ifdef _F_USE_VIRT_RESOLUTION_
static Eina_Bool
_e_mod_revert_resolution( Ecore_X_Window win);

static Eina_Bool
_e_mod_check_change_resolution( Ecore_X_Window win);

#define INIT_ZERO 0
#define PLANE_MUTE_TIMER 2
#define PLANE_MUTE_DELAY_TIMER 0.5
#endif /* end of _F_USE_VIRT_RESOLUTION_ */
#ifdef _F_FOCUS_WINDOW_IF_TOP_STACK_
static void _e_border_latest_stacked_focus (E_Border* bd);
static void _e_border_check_stack (E_Border *bd);
static void _e_border_focus_top_stack_set (E_Border* bd);
static void _e_border_transient_next_focus_set(E_Border *bd);
#endif
#if _F_BORDER_CLIP_TO_ZONE_
static void      _e_border_shape_input_clip_to_zone(E_Border *bd);
#endif /* _F_BORDER_CLIP_TO_ZONE_ */

#ifdef _F_DEICONIFY_APPROVE_
static void _e_border_msg_handler(void       *data,
                                  const char *name,
                                  const char *info,
                                  int         val,
                                  E_Object   *obj,
                                  void       *msgdata);

static void _e_border_window_pending_event_send(E_Border *bd, E_Border *hold_bd);
static void _e_border_window_pending_event_flush(E_Border *bd);
static void _e_border_deiconify_approve_send_pending_end(E_Border *bd);

static void _e_border_uniconify_pending_list_add(E_Border *bd);
static void _e_border_uniconify_pending_list_remove(E_Border *bd);
#endif

#ifdef _F_USE_TILED_DESK_LAYOUT_
static int _e_border_desk_layout_launch_pos_get(E_Border *bd);
static int _e_border_desk_layout_launcher_get(E_Border *bd);
static void _e_border_desk_layout_clear(E_Border *bd);
static void _e_border_desk_layout_set(E_Border *bd);
#endif

#ifdef _F_USE_BORDER_TRANSFORM_
static E_Border_Transform_Rect   _e_border_transform_rect_client_rect_get(E_Border *bd);
static E_Border_Transform_Rect   _e_border_transform_rect_create(int x, int y, int w, int h, int angle);
static E_Border_Transform_Rect   _e_border_transform_vertex_to_rect(E_Border_Transform_Vertex* vertex);
static E_Border_Transform_Rect   _e_border_transform_rect_ratio_rect_get(E_Border_Transform_Rect *dest,
                                                                         E_Border_Transform_Rect *source);
static pixman_transform_t        _e_border_transform_matrix_calculate(E_Border_Transform_Rect *dest,
                                                                      E_Border_Transform_Rect *source);
static E_Border_Transform_Vertex _e_border_transform_matrix_apply(E_Border_Transform_Rect *source,
                                                                  pixman_transform_t *matrix);
static void                      _e_border_transform_input_redirect_send(E_Border *bd,
                                                                         pixman_transform_t *matrix);
static void                      _e_border_transform_change_event_send(E_Border *bd);
static void                      _e_border_event_border_transform_changed_free(void *data,
                                                                               void *ev);
static pixman_transform_t _e_border_transform_matrix_get(E_Border *bd);
static void               _e_border_transform_matrix_set(E_Border* bd,
                                                         pixman_transform_t* matrix);
#endif /* end of _F_USE_BORDER_TRANSFORM_ */

static void
_e_border_event_border_anr_free(void *data __UNUSED__,
                                void      *ev);

#ifdef _F_E_WIN_AUX_HINT_
static void _e_border_aux_hint_job(void *data);
static void _e_border_aux_hint_handle(E_Border *bd);
#endif /* end of _F_E_WIN_AUX_HINT_ */


/* local subsystem globals */
static Eina_List *handlers = NULL;
static Eina_List *borders = NULL;
static Eina_Hash *borders_hash = NULL;
static E_Border *focused = NULL;
static E_Border *focusing = NULL;
static Eina_List *focus_next = NULL;
static Ecore_X_Time focus_time = 0;

static E_Border *bdresize = NULL;
static E_Border *bdmove = NULL;
static E_Drag *drag_border = NULL;

static int grabbed = 0;

static Eina_List *focus_stack = NULL;
static Eina_List *raise_stack = NULL;

static Ecore_X_Randr_Screen_Size screen_size = { -1, -1 };
static int screen_size_index = -1;

static int focus_track_frozen = 0;

static int warp_to = 0;
static int warp_to_x = 0;
static int warp_to_y = 0;
static int warp_x = 0;
static int warp_y = 0;
static Ecore_X_Window warp_to_win;
static Ecore_Timer *warp_timer = NULL;
#ifdef _F_USE_VIRT_RESOLUTION_
static Eina_List *res_windows_list = NULL;
static Ecore_Timer *mute_timer = NULL;
static unsigned int pref_w      = INIT_ZERO;
static unsigned int pref_h      = INIT_ZERO;
#endif /* end of _F_USE_VIRT_RESOLUTION_ */

#ifdef _F_DEICONIFY_APPROVE_
static E_Msg_Handler *_msg_handler = NULL;
static Eina_List *_uniconify_pending_list = NULL;
static Eina_List *_uniconify_waiting_list = NULL;
#endif

#ifdef _F_ZONE_WINDOW_ROTATION_
typedef struct _E_Border_Rotation      E_Border_Rotation;

struct _E_Border_Rotation
{
   Eina_List     *list;
   Eina_List     *async_list;

   Eina_Bool      wait_prepare_done;
   Ecore_Timer   *prepare_timer;
   Ecore_Timer   *done_timer;

   Ecore_X_Window vkbd_ctrl_win;
   E_Border      *vkbd;
   E_Border      *vkbd_prediction;
   E_Border      *vkbd_parent;

   /* vkbd show/hide preprare */
   Eina_Bool      vkbd_show_prepare_done;
   Ecore_Timer   *vkbd_show_prepare_timer;
   Ecore_Timer   *vkbd_show_timer;

   Eina_Bool      vkbd_hide_prepare_done;
   Ecore_Timer   *vkbd_hide_prepare_timer;
   Ecore_Timer   *vkbd_hide_timer;

   Eina_Bool      screen_lock;
   Eina_Bool      fetch;
   Eina_List     *msgs;

   struct {
     Eina_Bool    state;
     E_Zone      *zone;
   } cancel;
};

static E_Border_Rotation rot =
{
   NULL,
   NULL,
   EINA_FALSE,
   NULL,
   NULL,
   0,
   NULL,
   NULL,
   NULL,
   EINA_FALSE,
   NULL,
   NULL,
   EINA_FALSE,
   NULL,
   NULL,
   EINA_FALSE,
   EINA_FALSE,
   NULL,
   {
      EINA_FALSE,
      NULL,
   }
};
#endif

#ifdef _F_E_WIN_AUX_HINT_
static Ecore_Job *aux_hint_job = NULL;
#endif /* end of _F_E_WIN_AUX_HINT_ */

EAPI int E_EVENT_BORDER_ADD = 0;
EAPI int E_EVENT_BORDER_REMOVE = 0;
EAPI int E_EVENT_BORDER_ZONE_SET = 0;
EAPI int E_EVENT_BORDER_DESK_SET = 0;
EAPI int E_EVENT_BORDER_RESIZE = 0;
EAPI int E_EVENT_BORDER_MOVE = 0;
EAPI int E_EVENT_BORDER_SHOW = 0;
EAPI int E_EVENT_BORDER_HIDE = 0;
EAPI int E_EVENT_BORDER_ICONIFY = 0;
EAPI int E_EVENT_BORDER_UNICONIFY = 0;
EAPI int E_EVENT_BORDER_STICK = 0;
EAPI int E_EVENT_BORDER_UNSTICK = 0;
EAPI int E_EVENT_BORDER_STACK = 0;
EAPI int E_EVENT_BORDER_ICON_CHANGE = 0;
EAPI int E_EVENT_BORDER_URGENT_CHANGE = 0;
EAPI int E_EVENT_BORDER_FOCUS_IN = 0;
EAPI int E_EVENT_BORDER_FOCUS_OUT = 0;
EAPI int E_EVENT_BORDER_PROPERTY = 0;
EAPI int E_EVENT_BORDER_FULLSCREEN = 0;
EAPI int E_EVENT_BORDER_UNFULLSCREEN = 0;
#ifdef _F_USE_BORDER_TRANSFORM_
EAPI int E_EVENT_BORDER_TRANSFORM_CHANGED = 0;
#endif /* end of _F_USE_BORDER_TRANSFORM_ */
#ifdef _F_ZONE_WINDOW_ROTATION_
EAPI int E_EVENT_BORDER_ROTATION = 0; /* deprecated */
EAPI int E_EVENT_BORDER_ROTATION_CHANGE_BEGIN = 0;
EAPI int E_EVENT_BORDER_ROTATION_CHANGE_CANCEL = 0;
EAPI int E_EVENT_BORDER_ROTATION_CHANGE_END = 0;
#endif
#ifdef _F_USE_TILED_DESK_LAYOUT_
EAPI int E_EVENT_BORDER_DESK_LAYOUT_SET_BEGIN = 0;
EAPI int E_EVENT_BORDER_DESK_LAYOUT_SET_CANCEL = 0;
EAPI int E_EVENT_BORDER_DESK_LAYOUT_SET_END = 0;
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
#ifdef _F_DEICONIFY_APPROVE_
EAPI int E_EVENT_BORDER_FORCE_RENDER_DONE = 0;
#endif /* end of _F_DEICONIFY_APPROVE_ */
EAPI int E_EVENT_BORDER_ANR = 0;
#define GRAV_SET(bd, grav)                                \
  ecore_x_window_gravity_set(bd->bg_win, grav);           \
  ecore_x_window_gravity_set(bd->client.shell_win, grav); \
  ecore_x_window_gravity_set(bd->client.win, grav);

static Eina_List *
_e_border_sub_borders_new(E_Border *bd)
{
   Eina_List *list = NULL, *l;
   E_Border *child;

   EINA_LIST_FOREACH(bd->transients, l, child)
     {
        if (!eina_list_data_find(list, child))
          list = eina_list_append(list, child);
     }
   return list;
}
#ifdef _F_USE_WM_ZONE_HOST_
static int tizen_zone_event_change_callback_listener(vsm_zone_h zone, vsm_zone_event_t event, void *user_data)
{
   unsigned int i;
   char* zone_name;
   E_Container *e_con = e_container_current_get(e_manager_current_get());
   E_Zone *e_zone = e_util_zone_current_get(e_manager_current_get());

   zone_name =  vsm_get_zone_name(zone);

   if(VSM_ZONE_EVENT_CREATED == event)
     {
        /* add desk dynamically */
        e_desk_row_add(e_zone);

        for ( i = 0 ; i < e_zone->desk_x_count ; i++)
          {
             e_desk_name_add(e_con->num, e_zone->num, i, e_zone->desk_y_count - 1, zone_name);
          }

        e_desk_name_update();
     }

   if(VSM_ZONE_EVENT_DESTROYED == event)
     {
        e_desk_row_remove(e_zone);

        e_desk_name_update();
     }

   if(VSM_ZONE_EVENT_SWITCHED == event)
     {
        /* search the desk name & change desk if zone_name matches deskname */
        for(i=0; i < (e_zone->desk_x_count * e_zone->desk_y_count); i++)
          {
             if(strcmp(e_zone->desks[i]->name, zone_name) == 0)
               {
                  /* TO DO : profile handling need to be add here. */
                  e_desk_show(e_zone->desks[i]);
                  break;
               }

             if(strcmp(zone_name, "personal") == 0)
               {
                  e_desk_show(e_zone->desks[0]);
                  break;
               }

          }
     }

   return 0;
}

static Eina_Bool _ecore_tizen_zone_cb_handle_data(void *data, Ecore_Fd_Handler *hdl)
{
   vsm_context_h ctx = data;
   vsm_enter_eventloop(ctx, 0, 0);

}
#endif

/* externally accessible functions */
EINTERN int
e_border_init(void)
{
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHOW_REQUEST, _e_border_cb_window_show_request, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_DESTROY, _e_border_cb_window_destroy, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_HIDE, _e_border_cb_window_hide, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_REPARENT, _e_border_cb_window_reparent, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CONFIGURE_REQUEST, _e_border_cb_window_configure_request, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_RESIZE_REQUEST, _e_border_cb_window_resize_request, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_GRAVITY, _e_border_cb_window_gravity, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_STACK_REQUEST, _e_border_cb_window_stack_request, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, _e_border_cb_window_property, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_COLORMAP, _e_border_cb_window_colormap, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHAPE, _e_border_cb_window_shape, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_IN, _e_border_cb_window_focus_in, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_OUT, _e_border_cb_window_focus_out, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, _e_border_cb_client_message, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_STATE_REQUEST, _e_border_cb_window_state_request, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_MOVE_RESIZE_REQUEST, _e_border_cb_window_move_resize_request, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_DESKTOP_CHANGE, _e_border_cb_desktop_change, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_SYNC_ALARM, _e_border_cb_sync_alarm, NULL));
#ifdef _F_ZONE_WINDOW_ROTATION_
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CONFIGURE, _e_border_cb_window_configure, NULL));
#endif

   ecore_x_passive_grab_replay_func_set(_e_border_cb_grab_replay, NULL);

   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_POINTER_WARP, _e_border_cb_pointer_warp, NULL));
#ifndef _F_DISABLE_E_EFREET_
   handlers = eina_list_append(handlers, ecore_event_handler_add(EFREET_EVENT_DESKTOP_CACHE_UPDATE, _e_border_cb_efreet_cache_update, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(EFREET_EVENT_ICON_CACHE_UPDATE, _e_border_cb_efreet_cache_update, NULL));
#endif
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_CONFIG_ICON_THEME, _e_border_cb_config_icon_theme, NULL));
#ifdef _F_USE_DESK_WINDOW_PROFILE_
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_DESK_WINDOW_PROFILE_CHANGE, _e_border_cb_desk_window_profile_change, NULL));
#endif
#ifdef _F_ZONE_WINDOW_ROTATION_
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_ZONE_ROTATION_CHANGE_BEGIN, _e_border_cb_zone_rotation_change_begin, NULL));
#endif
#ifdef _F_TEMPORARY_COMP_EVENT_
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_COMP_SOURCE_VISIBILITY, _e_border_cb_comp_src_visibility, NULL));
#endif

#ifdef _F_DEICONIFY_APPROVE_
   _msg_handler = e_msg_handler_add(_e_border_msg_handler, NULL);
#endif

   if (!borders_hash) borders_hash = eina_hash_string_superfast_new(NULL);

   E_EVENT_BORDER_ADD = ecore_event_type_new();
   E_EVENT_BORDER_REMOVE = ecore_event_type_new();
   E_EVENT_BORDER_DESK_SET = ecore_event_type_new();
   E_EVENT_BORDER_ZONE_SET = ecore_event_type_new();
   E_EVENT_BORDER_RESIZE = ecore_event_type_new();
   E_EVENT_BORDER_MOVE = ecore_event_type_new();
   E_EVENT_BORDER_SHOW = ecore_event_type_new();
   E_EVENT_BORDER_HIDE = ecore_event_type_new();
   E_EVENT_BORDER_ICONIFY = ecore_event_type_new();
   E_EVENT_BORDER_UNICONIFY = ecore_event_type_new();
   E_EVENT_BORDER_STICK = ecore_event_type_new();
   E_EVENT_BORDER_UNSTICK = ecore_event_type_new();
   E_EVENT_BORDER_STACK = ecore_event_type_new();
   E_EVENT_BORDER_ICON_CHANGE = ecore_event_type_new();
   E_EVENT_BORDER_URGENT_CHANGE = ecore_event_type_new();
   E_EVENT_BORDER_FOCUS_IN = ecore_event_type_new();
   E_EVENT_BORDER_FOCUS_OUT = ecore_event_type_new();
   E_EVENT_BORDER_PROPERTY = ecore_event_type_new();
   E_EVENT_BORDER_FULLSCREEN = ecore_event_type_new();
   E_EVENT_BORDER_UNFULLSCREEN = ecore_event_type_new();
#ifdef _F_USE_BORDER_TRANSFORM_
   E_EVENT_BORDER_TRANSFORM_CHANGED = ecore_event_type_new();
#endif /* end of _F_USE_BORDER_TRANSFORM_ */
#ifdef _F_ZONE_WINDOW_ROTATION_
   E_EVENT_BORDER_ROTATION = ecore_event_type_new(); /* deprecated */
   E_EVENT_BORDER_ROTATION_CHANGE_BEGIN = ecore_event_type_new();
   E_EVENT_BORDER_ROTATION_CHANGE_CANCEL = ecore_event_type_new();
   E_EVENT_BORDER_ROTATION_CHANGE_END = ecore_event_type_new();
#endif
#ifdef _F_USE_TILED_DESK_LAYOUT_
   E_EVENT_BORDER_DESK_LAYOUT_SET_BEGIN = ecore_event_type_new();
   E_EVENT_BORDER_DESK_LAYOUT_SET_CANCEL = ecore_event_type_new();
   E_EVENT_BORDER_DESK_LAYOUT_SET_END = ecore_event_type_new();
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
#ifdef _F_DEICONIFY_APPROVE_
   E_EVENT_BORDER_FORCE_RENDER_DONE = ecore_event_type_new();
#endif /* end of _F_DEICONIFY_APPROVE_ */

   E_EVENT_BORDER_ANR = ecore_event_type_new();

#ifdef _F_USE_WM_ZONE_HOST_
   if(e_config->tizen_zone_active)
     {
        vsm_context_h ctx;
        int vsm_fd;

        ctx = vsm_create_context();

        if (ctx == NULL) {
             fprintf(stderr, "Failed to initialize domain control context");
             return 0;
        }

        vsm_fd = vsm_get_poll_fd (ctx);

        ecore_main_fd_handler_add(vsm_fd,
                                  ECORE_FD_READ | ECORE_FD_WRITE | ECORE_FD_ERROR,
                                  _ecore_tizen_zone_cb_handle_data, ctx,
                                  NULL, NULL);

        vsm_add_event_callback(ctx, tizen_zone_event_change_callback_listener, NULL);

     }
#endif
   //   e_init_undone();

   return 1;
}

EINTERN int
e_border_shutdown(void)
{
   E_FREE_LIST(handlers, ecore_event_handler_del);

   if (borders_hash) eina_hash_free(borders_hash);
   borders_hash = NULL;

#ifndef _F_DISABLE_E_MENU
   e_int_border_menu_hooks_clear();
#endif

   return 1;
}

EAPI E_Border *
e_border_new(E_Container   *con,
             Ecore_X_Window win,
             int            first_map,
             int            internal)
{
   E_Border *bd, *bd2;
   Ecore_X_Window_Attributes *att;
   unsigned int managed, desk[2];
   int deskx, desky;

   E_OBJECT_CHECK_RETURN(con, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(con, E_CONTAINER_TYPE, NULL);

   bd = E_OBJECT_ALLOC(E_Border, E_BORDER_TYPE, _e_border_free);
   if (!bd) return NULL;
   ecore_x_window_shadow_tree_flush();
   e_object_del_func_set(E_OBJECT(bd), E_OBJECT_CLEANUP_FUNC(_e_border_del));

   bd->w = 1;
   bd->h = 1;
   /* FIXME: ewww - round trip */
   bd->client.argb = ecore_x_window_argb_get(win);
   if (bd->client.argb)
     bd->win = ecore_x_window_manager_argb_new(con->win, 0, 0, bd->w, bd->h);
   else
     {
        bd->win = ecore_x_window_override_new(con->win, 0, 0, bd->w, bd->h);
        ecore_x_window_shape_events_select(bd->win, 1);
     }
   e_bindings_mouse_grab(E_BINDING_CONTEXT_WINDOW, bd->win);
   e_bindings_wheel_grab(E_BINDING_CONTEXT_WINDOW, bd->win);
   e_focus_setup(bd);
   bd->bg_ecore_evas = e_canvas_new(bd->win,
                                    0, 0, bd->w, bd->h, 1, 0,
                                    &(bd->bg_win));
   ecore_evas_ignore_events_set(bd->bg_ecore_evas, EINA_TRUE);
   e_canvas_add(bd->bg_ecore_evas);
   bd->event_win = ecore_x_window_input_new(bd->win, 0, 0, bd->w, bd->h);
   bd->bg_evas = ecore_evas_get(bd->bg_ecore_evas);
   ecore_x_window_shape_events_select(bd->bg_win, 1);
   ecore_evas_name_class_set(bd->bg_ecore_evas, "E", "Frame_Window");
   ecore_evas_title_set(bd->bg_ecore_evas, "Enlightenment Frame");
   if (bd->client.argb)
     bd->client.shell_win = ecore_x_window_manager_argb_new(bd->win, 0, 0, 1, 1);
   else
     bd->client.shell_win = ecore_x_window_override_new(bd->win, 0, 0, 1, 1);
   ecore_x_window_container_manage(bd->client.shell_win);
   if (!internal) ecore_x_window_client_manage(win);
   /* FIXME: Round trip. XCB */
   /* fetch needed to avoid grabbing the server as window may vanish */
   att = &bd->client.initial_attributes;
   if ((!ecore_x_window_attributes_get(win, att)) || (att->input_only))
     {
        //	printf("##- ATTR FETCH FAILED/INPUT ONLY FOR 0x%x - ABORT MANAGE\n", win);
          e_canvas_del(bd->bg_ecore_evas);
          ecore_evas_free(bd->bg_ecore_evas);
          ecore_x_window_free(bd->client.shell_win);
          e_bindings_mouse_ungrab(E_BINDING_CONTEXT_WINDOW, bd->win);
          e_bindings_wheel_ungrab(E_BINDING_CONTEXT_WINDOW, bd->win);
          ecore_x_window_free(bd->win);
          free(bd);
	      bd = NULL;
          return NULL;
     }

   /* printf("##- ON MAP CLIENT 0x%x SIZE %ix%i %i:%i\n",
    *     bd->client.win, bd->client.w, bd->client.h, att->x, att->y); */

   /* FIXME: if first_map is 1 then we should ignore the first hide event
    * or ensure the window is already hidden and events flushed before we
    * create a border for it */
   if (first_map)
     {
        // printf("##- FIRST MAP\n");
          bd->x = att->x;
          bd->y = att->y;
          bd->changes.pos = 1;
          bd->re_manage = 1;
          // needed to be 1 for internal windw and on restart.
          // bd->ignore_first_unmap = 2;
     }

   bd->client.win = win;
   bd->zone = e_zone_current_get(con);

   _e_border_hook_call(E_BORDER_HOOK_NEW_BORDER, bd);

   bd->handlers = eina_list_append(bd->handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_IN, _e_border_cb_mouse_in, bd));
   bd->handlers = eina_list_append(bd->handlers, ecore_event_handler_add(ECORE_X_EVENT_MOUSE_OUT, _e_border_cb_mouse_out, bd));
   bd->handlers = eina_list_append(bd->handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, _e_border_cb_mouse_down, bd));
   bd->handlers = eina_list_append(bd->handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP, _e_border_cb_mouse_up, bd));
   bd->handlers = eina_list_append(bd->handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE, _e_border_cb_mouse_move, bd));
   bd->handlers = eina_list_append(bd->handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_WHEEL, _e_border_cb_mouse_wheel, bd));

   bd->client.icccm.title = NULL;
   bd->client.icccm.name = NULL;
   bd->client.icccm.class = NULL;
   bd->client.icccm.icon_name = NULL;
   bd->client.icccm.machine = NULL;
   bd->client.icccm.min_w = 1;
   bd->client.icccm.min_h = 1;
   bd->client.icccm.max_w = 32767;
   bd->client.icccm.max_h = 32767;
   bd->client.icccm.base_w = 0;
   bd->client.icccm.base_h = 0;
   bd->client.icccm.step_w = -1;
   bd->client.icccm.step_h = -1;
   bd->client.icccm.min_aspect = 0.0;
   bd->client.icccm.max_aspect = 0.0;
   bd->client.icccm.accepts_focus = 1;

   bd->client.netwm.pid = 0;
   bd->client.netwm.name = NULL;
   bd->client.netwm.icon_name = NULL;
   bd->client.netwm.desktop = 0;
   bd->client.netwm.state.modal = 0;
   bd->client.netwm.state.sticky = 0;
   bd->client.netwm.state.shaded = 0;
   bd->client.netwm.state.hidden = 0;
   bd->client.netwm.state.maximized_v = 0;
   bd->client.netwm.state.maximized_h = 0;
   bd->client.netwm.state.skip_taskbar = 0;
   bd->client.netwm.state.skip_pager = 0;
   bd->client.netwm.state.fullscreen = 0;
   bd->client.netwm.state.stacking = E_STACKING_NONE;
   bd->client.netwm.action.move = 0;
   bd->client.netwm.action.resize = 0;
   bd->client.netwm.action.minimize = 0;
   bd->client.netwm.action.shade = 0;
   bd->client.netwm.action.stick = 0;
   bd->client.netwm.action.maximized_h = 0;
   bd->client.netwm.action.maximized_v = 0;
   bd->client.netwm.action.fullscreen = 0;
   bd->client.netwm.action.change_desktop = 0;
   bd->client.netwm.action.close = 0;
   bd->client.netwm.type = ECORE_X_WINDOW_TYPE_UNKNOWN;

#ifdef _F_USE_BORDER_TRANSFORM_
   bd->transform.enable = 0;
   bd->transform.ratio_fit = 0;
   bd->transform.force_pending = 0;
   bd->transform.parent = 0;
   bd->transform.child_wins = 0;
   bd->transform.geometry = _e_border_transform_rect_create(0, 0, 0, 0, 0);
   bd->transform.border_geometry = _e_border_transform_rect_create(0, 0, 0, 0, 0);
   bd->transform.input_geometry = _e_border_transform_rect_create(0, 0, 0, 0, 0);
#endif /* end of _F_USE_BORDER_TRANSFORM_ */

#ifdef _F_USE_ICONIFY_RESIZE_
   if (e_config->use_iconify_resize)
     bd->iconify_resize.support = 1;
#endif /* end of _F_USE_ICONIFY_RESIZE_ */

   {
      int at_num = 0, i;
      Ecore_X_Atom *atoms;

      atoms = ecore_x_window_prop_list(bd->client.win, &at_num);
      bd->client.icccm.fetch.command = 1;
      if (atoms)
        {
           Eina_Bool video_parent = EINA_FALSE;
           Eina_Bool video_position = EINA_FALSE;

           /* icccm */
            for (i = 0; i < at_num; i++)
              {
                 if (atoms[i] == ECORE_X_ATOM_WM_NAME)
                   bd->client.icccm.fetch.title = 1;
                 else if (atoms[i] == ECORE_X_ATOM_WM_CLASS)
                   bd->client.icccm.fetch.name_class = 1;
                 else if (atoms[i] == ECORE_X_ATOM_WM_ICON_NAME)
                   bd->client.icccm.fetch.icon_name = 1;
                 else if (atoms[i] == ECORE_X_ATOM_WM_CLIENT_MACHINE)
                   bd->client.icccm.fetch.machine = 1;
                 else if (atoms[i] == ECORE_X_ATOM_WM_HINTS)
                   bd->client.icccm.fetch.hints = 1;
                 else if (atoms[i] == ECORE_X_ATOM_WM_NORMAL_HINTS)
                   bd->client.icccm.fetch.size_pos_hints = 1;
                 else if (atoms[i] == ECORE_X_ATOM_WM_PROTOCOLS)
                   bd->client.icccm.fetch.protocol = 1;
                 else if (atoms[i] == ECORE_X_ATOM_MOTIF_WM_HINTS)
                   bd->client.mwm.fetch.hints = 1;
                 else if (atoms[i] == ECORE_X_ATOM_WM_TRANSIENT_FOR)
                   {
                      bd->client.icccm.fetch.transient_for = 1;
                      bd->client.netwm.fetch.type = 1;
                   }
                 else if (atoms[i] == ECORE_X_ATOM_WM_CLIENT_LEADER)
                   bd->client.icccm.fetch.client_leader = 1;
                 else if (atoms[i] == ECORE_X_ATOM_WM_WINDOW_ROLE)
                   bd->client.icccm.fetch.window_role = 1;
                 else if (atoms[i] == ECORE_X_ATOM_WM_STATE)
                   bd->client.icccm.fetch.state = 1;
              }
            /* netwm, loop again, netwm will ignore some icccm, so we
             * have to be sure that netwm is checked after */
            for (i = 0; i < at_num; i++)
              {
                 if (atoms[i] == ECORE_X_ATOM_NET_WM_NAME)
                   {
     /* Ignore icccm */
                       bd->client.icccm.fetch.title = 0;
                       bd->client.netwm.fetch.name = 1;
                   }
                 else if (atoms[i] == ECORE_X_ATOM_NET_WM_ICON_NAME)
                   {
     /* Ignore icccm */
                       bd->client.icccm.fetch.icon_name = 0;
                       bd->client.netwm.fetch.icon_name = 1;
                   }
                 else if (atoms[i] == ECORE_X_ATOM_NET_WM_ICON)
                   {
                      bd->client.netwm.fetch.icon = 1;
                   }
                 else if (atoms[i] == ECORE_X_ATOM_NET_WM_USER_TIME)
                   {
                      bd->client.netwm.fetch.user_time = 1;
                   }
                 else if (atoms[i] == ECORE_X_ATOM_NET_WM_STRUT)
                   {
                      DBG("ECORE_X_ATOM_NET_WM_STRUT");
                      bd->client.netwm.fetch.strut = 1;
                   }
                 else if (atoms[i] == ECORE_X_ATOM_NET_WM_STRUT_PARTIAL)
                   {
                      DBG("ECORE_X_ATOM_NET_WM_STRUT_PARTIAL");
                      bd->client.netwm.fetch.strut = 1;
                   }
                 else if (atoms[i] == ECORE_X_ATOM_NET_WM_WINDOW_TYPE)
                   {
     /* Ignore mwm
        bd->client.mwm.fetch.hints = 0;
      */
                        bd->client.netwm.fetch.type = 1;
                   }
                 else if (atoms[i] == ECORE_X_ATOM_NET_WM_STATE)
                   {
                      bd->client.netwm.fetch.state = 1;
                   }
              }
            /* other misc atoms */
            for (i = 0; i < at_num; i++)
              {
     /* loop to check for own atoms */
                  if (atoms[i] == E_ATOM_WINDOW_STATE)
                    {
                       bd->client.e.fetch.state = 1;
                    }
     /* loop to check for qtopia atoms */
                  if (atoms[i] == ATM__QTOPIA_SOFT_MENU)
                    bd->client.qtopia.fetch.soft_menu = 1;
                  else if (atoms[i] == ATM__QTOPIA_SOFT_MENUS)
                    bd->client.qtopia.fetch.soft_menus = 1;
     /* loop to check for vkbd atoms */
                  else if (atoms[i] == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE)
                    bd->client.vkbd.fetch.state = 1;
                  else if (atoms[i] == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD)
                    bd->client.vkbd.fetch.vkbd = 1;
     /* loop to check for illume atoms */
                  else if (atoms[i] == ECORE_X_ATOM_E_ILLUME_CONFORMANT)
                    bd->client.illume.conformant.fetch.conformant = 1;
                  else if (atoms[i] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE)
                    bd->client.illume.quickpanel.fetch.state = 1;
                  else if (atoms[i] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL)
                    bd->client.illume.quickpanel.fetch.quickpanel = 1;
                  else if (atoms[i] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_PRIORITY_MAJOR)
                    bd->client.illume.quickpanel.fetch.priority.major = 1;
                  else if (atoms[i] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_PRIORITY_MINOR)
                    bd->client.illume.quickpanel.fetch.priority.minor = 1;
                  else if (atoms[i] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_ZONE)
                    bd->client.illume.quickpanel.fetch.zone = 1;
                  else if (atoms[i] == ECORE_X_ATOM_E_ILLUME_DRAG_LOCKED)
                    bd->client.illume.drag.fetch.locked = 1;
                  else if (atoms[i] == ECORE_X_ATOM_E_ILLUME_DRAG)
                    bd->client.illume.drag.fetch.drag = 1;
                  else if (atoms[i] == ECORE_X_ATOM_E_ILLUME_WINDOW_STATE)
                    bd->client.illume.win_state.fetch.state = 1;
                  else if (atoms[i] == ECORE_X_ATOM_E_VIDEO_PARENT)
                    video_parent = EINA_TRUE;
                  else if (atoms[i] == ECORE_X_ATOM_E_VIDEO_POSITION)
                    video_position = EINA_TRUE;
#ifdef _F_USE_DESK_WINDOW_PROFILE_
                   /* loop to check for window profile list atom */
                  else if (atoms[i] == ECORE_X_ATOM_E_WINDOW_PROFILE_SUPPORTED)
                    bd->client.e.fetch.profile = 1;
#endif
#ifdef _F_ZONE_WINDOW_ROTATION_
                  /* loop to check for wm rotation */
                  else if (atoms[i] == ECORE_X_ATOM_E_WINDOW_ROTATION_SUPPORTED)
                    {
                       if (e_config->wm_win_rotation)
                         bd->client.e.fetch.rot.support = 1;
                    }
                  else if ((atoms[i] == ECORE_X_ATOM_E_WINDOW_ROTATION_0_GEOMETRY) ||
                           (atoms[i] == ECORE_X_ATOM_E_WINDOW_ROTATION_90_GEOMETRY) ||
                           (atoms[i] == ECORE_X_ATOM_E_WINDOW_ROTATION_180_GEOMETRY) ||
                           (atoms[i] == ECORE_X_ATOM_E_WINDOW_ROTATION_270_GEOMETRY))
                    {
                       if (e_config->wm_win_rotation)
                         bd->client.e.fetch.rot.geom_hint = 1;
                    }
                  else if (atoms[i] == ECORE_X_ATOM_E_WINDOW_ROTATION_APP_SUPPORTED)
                    {
                       if (e_config->wm_win_rotation)
                         bd->client.e.fetch.rot.app_set = 1;
                    }
                  else if (atoms[i] == ECORE_X_ATOM_E_WINDOW_ROTATION_PREFERRED_ROTATION)
                    {
                       if (e_config->wm_win_rotation)
                         bd->client.e.fetch.rot.preferred_rot = 1;
                    }
                  else if (atoms[i] == ECORE_X_ATOM_E_WINDOW_ROTATION_AVAILABLE_LIST)
                    {
                       if (e_config->wm_win_rotation)
                         bd->client.e.fetch.rot.available_rots = 1;
                    }
#endif
#ifdef _F_DEICONIFY_APPROVE_
                  else if (atoms[i] == ECORE_X_ATOM_E_DEICONIFY_APPROVE)
                    {
                       bd->client.e.state.deiconify_approve.support = 1;
                    }
#endif
#ifdef _F_USE_TILED_DESK_LAYOUT_
                  else if (atoms[i] == E_ATOM_WINDOW_DESKTOP_LAYOUT_SUPPORTED)
                    {
                       if (e_config->use_tiled_desk_layout)
                         bd->client.e.fetch.ly.support = 1;
                    }
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
#ifdef _F_E_WIN_AUX_HINT_
                  else if (atoms[i] == ECORE_X_ATOM_E_WINDOW_AUX_HINT_SUPPORT)
                    {
                       if (e_config->win_aux_hint)
                         bd->client.e.fetch.aux_hint.support = 1;
                    }
                  else if (atoms[i] == ECORE_X_ATOM_E_WINDOW_AUX_HINT)
                    {
                       if (e_config->win_aux_hint)
                         bd->client.e.fetch.aux_hint.hints = 1;
                    }
#endif /* end of _F_E_WIN_AUX_HINT_ */
              }
            if (video_position && video_parent)
              {
                 bd->client.e.state.video = 1;
                 bd->client.e.fetch.video_parent = 1;
                 bd->client.e.fetch.video_position = 1;
                 ecore_x_window_lower(bd->win);
                 ecore_x_composite_window_events_disable(bd->win);
                 ecore_x_window_ignore_set(bd->win, EINA_TRUE);
                 fprintf(stderr, "We found a video window \\o/ %x\n", win);
              }
            free(atoms);
        }
   }
   bd->client.border.changed = 1;

   bd->client.w = att->w;
   bd->client.h = att->h;

#ifdef _F_FIX_POSITION_PROBLEM_
   bd->x = att->x;
   bd->y = att->y;
#endif
   bd->w = bd->client.w;
   bd->h = bd->client.h;

   bd->resize_mode = RESIZE_NONE;
   bd->layer = 100;
   bd->saved.layer = bd->layer;
   bd->changes.icon = 1;
   bd->changes.size = 1;
   bd->changes.shape = 1;
   bd->changes.shape_input = 1;

   bd->offer_resistance = 1;

   /* just to friggin make java happy - we're DELAYING the reparent until
    * eval time...
    */
/*   ecore_x_window_reparent(win, bd->client.shell_win, 0, 0); */
   bd->need_reparent = 1;

   ecore_x_window_border_width_set(win, 0);
   ecore_x_window_show(bd->event_win);
   ecore_x_window_show(bd->client.shell_win);
   bd->shape = e_container_shape_add(con);

   bd->take_focus = 1;
   bd->new_client = 1;
   bd->changed = 1;
#ifdef _F_ZONE_WINDOW_ROTATION_
   bd->client.e.state.rot.preferred_rot = -1;
   bd->client.e.state.rot.type = E_BORDER_ROTATION_TYPE_NORMAL;
   bd->client.e.state.rot.ang.next = -1;
   bd->client.e.state.rot.ang.reserve = -1;
   bd->client.e.state.rot.pending_show = 0;
   bd->client.e.state.rot.ang.curr = 0;
   bd->client.e.state.rot.ang.prev = 0;
#endif

//   bd->zone = e_zone_current_get(con);
#ifdef _F_USE_WM_ZONE_HOST_
   if(e_config->tizen_zone_active)
     {
        unsigned int i;
        char* zone_name;
        vsm_context_h * vsm_ctx;
        pid_t client_win_pid;
        vsm_zone_h vsm_zone;
        EAPI Ecore_X_Atom E_ATOM_X_WIN_PID= 0;

        vsm_ctx = vsm_create_context();

        E_ATOM_X_WIN_PID = ecore_x_atom_get("X_CLIENT_PID");

        ecore_x_window_prop_card32_get(bd->client.win, E_ATOM_X_WIN_PID, &client_win_pid, 1);

        vsm_zone = vsm_lookup_zone_by_pid(vsm_ctx, client_win_pid);

        zone_name =  vsm_get_zone_name(vsm_zone);

        /* search the desk name & change desk if zone_name matches deskname */
        for(i=0; i < (bd->zone->desk_x_count * bd->zone->desk_y_count); i++)
          {
             if(strcmp(bd->zone->desks[i]->name, zone_name) == 0)
               {
                  /* TO DO : profile handling need to be added here. */
                  bd->desk = bd->zone->desks[i];
                  break;
               }

             if(strcmp(zone_name, "personal") == 0)
               {
                  bd->desk = bd->zone->desks[0];
                  break;
               }

             if(strcmp(zone_name, "") == 0) /* for the case it's created in HOST */
               {
                  bd->desk = e_desk_current_get(bd->zone);
                  break;
               }
          }

     }
   else
     bd->desk = e_desk_current_get(bd->zone);
#else
   bd->desk = e_desk_current_get(bd->zone);
#endif
   e_container_border_add(bd);
   borders = eina_list_append(borders, bd);

   bd2 = eina_hash_find(borders_hash, e_util_winid_str_get(bd->client.win));
   if (bd2)
     {
#ifdef E_LOGGING
        WRN("EEEEK! 2 borders with same client window id in them! very bad!\n"
            "optimisations failing due to bizarre client behavior. will\n"
            "work around.\n"
            "bd=%p, bd->references=%i, bd->deleted=%i, bd->client.win=%x",
               bd2, bd2->e_obj_inherit.references, bd2->e_obj_inherit.deleted,
               bd2->client.win);

        ELBF(ELBT_BD, 0, bd->client.win,
             "ERR! 2 borders with same client win id in them! bd:%p ref:%i deleted:%i",
             bd2, bd2->e_obj_inherit.references, bd2->e_obj_inherit.deleted);
#else
        printf("EEEEK! 2 borders with same client window id in them! very bad!\n");
        printf("optimisations failing due to bizarre client behavior. will\n");
        printf("work around.\n");
        printf("bd=%p, bd->references=%i, bd->deleted=%i, bd->client.win=%x\n",
               bd2, bd2->e_obj_inherit.references, bd2->e_obj_inherit.deleted,
               bd2->client.win);

        ELBF(ELBT_BD, 0, bd->client.win,
             "ERR! 2 borders with same client win id in them! bd:%p ref:%i deleted:%i",
             bd2, bd2->e_obj_inherit.references, bd2->e_obj_inherit.deleted);
#endif

#ifdef _F_ZONE_WINDOW_ROTATION_
        if ((rot.vkbd) && (rot.vkbd == bd2))
          {
             int unref_count = 0;

             ELB(ELBT_BD, "UNSET OLD VKBD", rot.vkbd->client.win);
             ELBF(ELBT_BD, 1, rot.vkbd->client.win, "VKBD HIDE PREPARE_DONE:%d",
                  rot.vkbd_hide_prepare_done);

             if (rot.vkbd_hide_prepare_timer)
               {
                  ecore_timer_del(rot.vkbd_hide_prepare_timer);
                  rot.vkbd_hide_prepare_timer = NULL;
                  unref_count++;
               }

             if (rot.vkbd_hide_timer)
               {
                  ecore_timer_del(rot.vkbd_hide_timer);
                  unref_count++;
               }
             rot.vkbd_hide_timer = NULL;

             rot.vkbd_hide_prepare_done = EINA_TRUE;

             ELB(ELBT_BD, "HIDE OLD VKBD", bd2->client.win);
             e_border_hide(bd2, 0);

             if (!e_object_is_del(E_OBJECT(bd2)))
               {
                  ELB(ELBT_BD, "DEL OLD VKBD", bd2->client.win);
                  e_object_del(E_OBJECT(bd2));
               }

             if (rot.vkbd_ctrl_win)
               {
                  ELB(ELBT_BD, "SET KBD_OFF", 0);
                  ecore_x_e_virtual_keyboard_state_set
                     (rot.vkbd_ctrl_win, ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF);
               }
             rot.vkbd_hide_prepare_done = EINA_FALSE;

             rot.vkbd_show_prepare_done = EINA_FALSE;
             if (rot.vkbd_show_prepare_timer)
               ecore_timer_del(rot.vkbd_show_prepare_timer);
             rot.vkbd_show_prepare_timer = NULL;
             if (rot.vkbd_show_timer)
               ecore_timer_del(rot.vkbd_show_timer);
             rot.vkbd_show_timer = NULL;

             while (unref_count)
               {
                  e_object_unref(E_OBJECT(bd2));
                  unref_count--;
               }

             rot.vkbd = NULL;
          }
#endif
        eina_hash_del(borders_hash, e_util_winid_str_get(bd->client.win), bd2);
        eina_hash_del(borders_hash, e_util_winid_str_get(bd2->bg_win), bd2);
        eina_hash_del(borders_hash, e_util_winid_str_get(bd2->win), bd2);
     }
   eina_hash_add(borders_hash, e_util_winid_str_get(bd->client.win), bd);
   eina_hash_add(borders_hash, e_util_winid_str_get(bd->bg_win), bd);
   eina_hash_add(borders_hash, e_util_winid_str_get(bd->win), bd);
   managed = 1;
   ecore_x_window_prop_card32_set(win, E_ATOM_MANAGED, &managed, 1);
   ecore_x_window_prop_card32_set(win, E_ATOM_CONTAINER, &bd->zone->container->num, 1);
   ecore_x_window_prop_card32_set(win, E_ATOM_ZONE, &bd->zone->num, 1);
     {
        unsigned int zgeom[4];

        zgeom[0] = bd->zone->x;
        zgeom[1] = bd->zone->y;
        zgeom[2] = bd->zone->w;
        zgeom[3] = bd->zone->h;
        ecore_x_window_prop_card32_set(win, E_ATOM_ZONE_GEOMETRY, zgeom, 4);
     }
   e_desk_xy_get(bd->desk, &deskx, &desky);
   desk[0] = deskx;
   desk[1] = desky;
   ecore_x_window_prop_card32_set(win, E_ATOM_DESK, desk, 2);

   focus_stack = eina_list_append(focus_stack, bd);

   bd->pointer = e_pointer_window_new(bd->win, 0);

#ifdef _F_BORDER_HOOK_PATCH_
   _e_border_hook_call(E_BORDER_HOOK_POST_NEW_BORDER, bd);
#endif

   return bd;
}

EAPI void
e_border_res_change_geometry_save(E_Border *bd)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (bd->pre_res_change.valid) return;
   bd->pre_res_change.valid = 1;
   bd->pre_res_change.x = bd->x;
   bd->pre_res_change.y = bd->y;
   bd->pre_res_change.w = bd->w;
   bd->pre_res_change.h = bd->h;
   bd->pre_res_change.saved.x = bd->saved.x;
   bd->pre_res_change.saved.y = bd->saved.y;
   bd->pre_res_change.saved.w = bd->saved.w;
   bd->pre_res_change.saved.h = bd->saved.h;
}

EAPI void
e_border_res_change_geometry_restore(E_Border *bd)
{
   struct
   {
      unsigned char valid : 1;
      int           x, y, w, h;
      struct
      {
         int x, y, w, h;
      } saved;
   } pre_res_change;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (!bd->pre_res_change.valid) return;
   if (bd->new_client) return;

   ecore_x_window_shadow_tree_flush();
   memcpy(&pre_res_change, &bd->pre_res_change, sizeof(pre_res_change));

   if (bd->fullscreen)
     {
        e_border_unfullscreen(bd);
        e_border_fullscreen(bd, e_config->fullscreen_policy);
     }
   else if (bd->maximized != E_MAXIMIZE_NONE)
     {
        E_Maximize max;

        max = bd->maximized;
        e_border_unmaximize(bd, E_MAXIMIZE_BOTH);
        e_border_maximize(bd, max);
     }
   else
     {
        int x, y, w, h, zx, zy, zw, zh;

        bd->saved.x = bd->pre_res_change.saved.x;
        bd->saved.y = bd->pre_res_change.saved.y;
        bd->saved.w = bd->pre_res_change.saved.w;
        bd->saved.h = bd->pre_res_change.saved.h;

        e_zone_useful_geometry_get(bd->zone, &zx, &zy, &zw, &zh);

        if (bd->saved.w > zw)
          bd->saved.w = zw;
        if ((bd->saved.x + bd->saved.w) > (zx + zw))
          bd->saved.x = zx + zw - bd->saved.w;

        if (bd->saved.h > zh)
          bd->saved.h = zh;
        if ((bd->saved.y + bd->saved.h) > (zy + zh))
          bd->saved.y = zy + zh - bd->saved.h;

        x = bd->pre_res_change.x;
        y = bd->pre_res_change.y;
        w = bd->pre_res_change.w;
        h = bd->pre_res_change.h;
        if (w > zw)
          w = zw;
        if (h > zh)
          h = zh;
        if ((x + w) > (zx + zw))
          x = zx + zw - w;
        if ((y + h) > (zy + zh))
          y = zy + zh - h;

#ifdef _F_USE_VIRT_RESOLUTION_
        if (e_virtual_res_type_get() != E_VIRTUAL_RES_DOUBLE_SOURCE)
          {
             /* Dont resize windows if double stereo mode is enabled */
          }
        else
#endif /* end of _F_USE_VIRT_RESOLUTION_ */

          {
             e_border_move_resize(bd, x, y, w, h);
          }
     }
   memcpy(&bd->pre_res_change, &pre_res_change, sizeof(pre_res_change));
}

EAPI void
e_border_zone_set(E_Border *bd,
                  E_Zone   *zone)
{
   E_Event_Border_Zone_Set *ev;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);
   if (bd->zone == zone) return;

   /* if the window does not lie in the new zone, move it so that it does */
   if (!E_INTERSECTS(bd->x, bd->y, bd->w, bd->h, zone->x, zone->y, zone->w, zone->h))
     {
        int x, y;
        /* first guess -- get offset from old zone, and apply to new zone */
        x = zone->x + (bd->x - bd->zone->x);
        y = zone->y + (bd->y - bd->zone->y);

        /* keep window from hanging off bottom and left */
        if (x + bd->w > zone->x + zone->w) x += (zone->x + zone->w) - (x + bd->w);
        if (y + bd->h > zone->y + zone->h) y += (zone->y + zone->h) - (y + bd->h);

        /* make sure to and left are on screen (if the window is larger than the zone, it will hang off the bottom / right) */
        if (x < zone->x) x = zone->x;
        if (y < zone->y) y = zone->y;

        if (!E_INTERSECTS(x, y, bd->w, bd->h, zone->x, zone->y, zone->w, zone->h))
          {
             /* still not in zone at all, so just move it to closest edge */
              if (x < zone->x) x = zone->x;
              if (x >= zone->x + zone->w) x = zone->x + zone->w - bd->w;
              if (y < zone->y) y = zone->y;
              if (y >= zone->y + zone->h) y = zone->y + zone->h - bd->h;
          }
        e_border_move(bd, x, y);
     }

   bd->zone = zone;

   if (bd->desk->zone != bd->zone)
     e_border_desk_set(bd, e_desk_current_get(bd->zone));

   ev = E_NEW(E_Event_Border_Zone_Set, 1);
   if (ev)
     {
        ev->border = bd;
        e_object_ref(E_OBJECT(bd));
        //   e_object_breadcrumb_add(E_OBJECT(bd), "border_zone_set_event");
        ev->zone = zone;
        e_object_ref(E_OBJECT(zone));

        ecore_event_add(E_EVENT_BORDER_ZONE_SET, ev, _e_border_event_border_zone_set_free, NULL);
     }

   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_ZONE, &bd->zone->num, 1);
   // XXXXXXXXXXXXXXXXXXXXXXXXX
   // XXX ZZZZZZZZZZZZZZZZZZZzz
   // need to adjust this if zone pos/size changes
     {
        unsigned int zgeom[4];

        zgeom[0] = bd->zone->x;
        zgeom[1] = bd->zone->y;
        zgeom[2] = bd->zone->w;
        zgeom[3] = bd->zone->h;
        ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_ZONE_GEOMETRY, zgeom, 4);
     }
   e_remember_update(bd);
}

EAPI void
e_border_desk_set(E_Border *bd,
                  E_Desk   *desk)
{
   E_Event_Border_Desk_Set *ev;
   E_Desk *old_desk;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   E_OBJECT_CHECK(desk);
   E_OBJECT_TYPE_CHECK(desk, E_DESK_TYPE);
   if (bd->desk == desk) return;
#ifdef _F_USE_DESK_WINDOW_PROFILE_
   if ((e_config->use_desktop_window_profile) &&
       (bd->client.e.state.profile.use))
     {
        if (bd->client.e.state.profile.wait_for_done) return;
        if (strcmp(bd->client.e.state.profile.name, desk->window_profile) != 0)
          {
             ecore_x_e_window_profile_change_request_send(bd->client.win,
                                                          desk->window_profile);
             bd->client.e.state.profile.wait_for_done = 1;
             return;
          }
     }
#endif
   ecore_x_window_shadow_tree_flush();
   if (bd->fullscreen)
     {
        bd->desk->fullscreen_borders--;
        desk->fullscreen_borders++;
     }
   old_desk = bd->desk;
   bd->desk = desk;
   e_border_zone_set(bd, desk->zone);

   _e_border_hook_call(E_BORDER_HOOK_SET_DESK, bd);
   e_hints_window_desktop_set(bd);

   ev = E_NEW(E_Event_Border_Desk_Set, 1);
   if (ev)
     {
        ev->border = bd;
        e_object_ref(E_OBJECT(bd));
        //   e_object_breadcrumb_add(E_OBJECT(bd), "border_desk_set_event");
        ev->desk = old_desk;
        e_object_ref(E_OBJECT(old_desk));
        ecore_event_add(E_EVENT_BORDER_DESK_SET, ev, _e_border_event_border_desk_set_free, NULL);
     }

   if (bd->ignore_first_unmap != 1)
     {
        if ((bd->desk->visible) || (bd->sticky))
          e_border_show(bd);
        else
          e_border_hide(bd, 1);
     }

   if (e_config->transient.desktop)
     {
        Eina_List *l;
        E_Border *child;
        Eina_List *list = _e_border_sub_borders_new(bd);

        EINA_LIST_FOREACH(list, l, child)
          {
             e_border_desk_set(child, bd->desk);
          }
        eina_list_free(list);
     }
   e_remember_update(bd);
}

#ifdef _F_ZONE_WINDOW_ROTATION_
static Eina_Bool
_e_border_vkbd_state_check(E_Border *bd,
                           Eina_Bool show)
{
   Eina_Bool res = EINA_TRUE;
   if (!e_config->wm_win_rotation) return EINA_FALSE;
   if ((rot.vkbd) && (rot.vkbd == bd))
     {
        if (show)
          {
             if ((rot.vkbd_hide_prepare_done) ||
                 (rot.vkbd_hide_prepare_timer))
               res = EINA_FALSE;
          }
        else
          {
             if ((rot.vkbd_show_prepare_done) ||
                 (rot.vkbd_show_prepare_timer))
               res = EINA_FALSE;
          }
     }
   return res;
}

static Eina_Bool
_e_border_vkbd_show_timeout(void *data)
{
   E_Border *bd = data;
   if (!e_config->wm_win_rotation) return ECORE_CALLBACK_CANCEL;
   if ((bd) && ((E_OBJECT(bd)->type) == (E_BORDER_TYPE)))
     {
        if (_e_border_vkbd_state_check(bd, EINA_TRUE))
          {
             if (rot.vkbd_ctrl_win)
               {
                  ELB(ELBT_BD, "SET KBD_ON", 0);
                  ecore_x_e_virtual_keyboard_state_set
                    (rot.vkbd_ctrl_win, ECORE_X_VIRTUAL_KEYBOARD_STATE_ON);
               }
          }
     }

   rot.vkbd_show_prepare_done = EINA_FALSE;

   if (rot.vkbd_show_prepare_timer)
     ecore_timer_del(rot.vkbd_show_prepare_timer);
   rot.vkbd_show_prepare_timer = NULL;

   if (rot.vkbd_show_timer)
     ecore_timer_del(rot.vkbd_show_timer);
   rot.vkbd_show_timer = NULL;

   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_border_vkbd_hide_timeout(void *data)
{
   E_Border *bd = data;
   int unref_count = 0;

   if (!e_config->wm_win_rotation) return ECORE_CALLBACK_CANCEL;

   rot.vkbd_hide_timer = NULL;
   unref_count++;

   if (rot.vkbd_hide_prepare_timer)
     {
        ecore_timer_del(rot.vkbd_hide_prepare_timer);
        unref_count++;
     }
   rot.vkbd_hide_prepare_timer = NULL;

   if (_e_border_vkbd_state_check(bd, EINA_FALSE))
     {
        if (rot.vkbd_ctrl_win)
          {
             ELB(ELBT_BD, "SET KBD_OFF", 0);
             ecore_x_e_virtual_keyboard_state_set
                (rot.vkbd_ctrl_win, ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF);
          }
     }

   rot.vkbd_hide_prepare_done = EINA_FALSE;

   while (unref_count)
     {
        e_object_unref(E_OBJECT(bd));
        unref_count--;
     }

   return ECORE_CALLBACK_CANCEL;
}

static void
_e_border_vkbd_show(E_Border *bd)
{
   if (!e_config->wm_win_rotation) return;
   rot.vkbd_show_prepare_done = EINA_TRUE;
   if (rot.vkbd_show_prepare_timer)
     ecore_timer_del(rot.vkbd_show_prepare_timer);
   rot.vkbd_show_prepare_timer = NULL;
   if (rot.vkbd_show_timer)
     ecore_timer_del(rot.vkbd_show_timer);
   rot.vkbd_show_timer = NULL;
   if ((bd) && (!e_object_is_del(E_OBJECT(bd))))
     {
        e_border_show(bd);
        rot.vkbd_show_timer = ecore_timer_add(1.0f, _e_border_vkbd_show_timeout, bd);
     }
}

static void
_e_border_vkbd_hide(E_Border *bd, Eina_Bool clean)
{
   int unref_count = 0;

   if (!e_config->wm_win_rotation) return;
   rot.vkbd_hide_prepare_done = EINA_TRUE;

   if (clean)
     {
        if (rot.vkbd_hide_prepare_timer)
          {
             ecore_timer_del(rot.vkbd_hide_prepare_timer);
             unref_count++;
          }
        rot.vkbd_hide_prepare_timer = NULL;

        if (rot.vkbd_hide_timer)
          {
             ecore_timer_del(rot.vkbd_hide_timer);
             unref_count++;
          }
        rot.vkbd_hide_timer = NULL;

        if ((bd) && ((E_OBJECT(bd)->type) == (E_BORDER_TYPE)))
          {
             ELB(ELBT_BD, "HIDE VKBD", bd->client.win);
             e_border_hide(bd, 0);

             e_object_ref(E_OBJECT(bd));
             if (!e_object_is_del(E_OBJECT(bd)))
               {
                  ELB(ELBT_BD, "DEL VKBD", bd->client.win);
                  e_object_del(E_OBJECT(bd));
               }
             rot.vkbd_hide_timer = ecore_timer_add(0.03f, _e_border_vkbd_hide_timeout, bd);
          }
     }
   else
     {
        if ((bd) && ((E_OBJECT(bd)->type) == (E_BORDER_TYPE)))
          {
             ELB(ELBT_BD, "HIDE VKBD", bd->client.win);
             e_border_hide(bd, 0);
          }
     }

   while (unref_count)
     {
        e_object_unref(E_OBJECT(bd));
        unref_count--;
     }
}

static Eina_Bool
_e_border_vkbd_show_prepare_timeout(void *data)
{
   E_Border *bd = data;
   if (!e_config->wm_win_rotation) return ECORE_CALLBACK_CANCEL;
   if ((bd) && (!e_object_is_del(E_OBJECT(bd))))
     {
        ELB(ELBT_BD, "TIMEOUT KBD_SHOW_PREPARE", bd->client.win);
        _e_border_vkbd_show(bd);
     }
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_border_vkbd_hide_prepare_timeout(void *data)
{
   E_Border *bd = data;
   int unref_count = 0;
   if (!e_config->wm_win_rotation) return ECORE_CALLBACK_CANCEL;

   rot.vkbd_hide_prepare_timer = NULL;
   unref_count++;

   if (rot.vkbd_hide_timer)
     {
        ecore_timer_del(rot.vkbd_hide_timer);
        unref_count++;
     }
   rot.vkbd_hide_timer = NULL;

   ELB(ELBT_BD, "TIMEOUT KBD_HIDE_PREPARE", bd->client.win);
   _e_border_vkbd_hide(bd, EINA_TRUE);

   while (unref_count)
     {
        e_object_unref(E_OBJECT(bd));
        unref_count--;
     }

   return ECORE_CALLBACK_CANCEL;
}
#endif

#ifdef _F_TEMPORARY_COMP_EVENT_
static Eina_Bool
_e_border_cb_comp_src_visibility(void *data  __UNUSED__,
                                 int ev_type __UNUSED__,
                                 void       *event)
{
   E_Event_Comp_Source_Visibility *ev = event;
   if (!ev) return ECORE_CALLBACK_PASS_ON;

   E_Border *bd = e_border_find_all_by_client_window(ev->win);
   if (bd)
     {
        ELBF(ELBT_BD, 0, ev->win,
             "%15.15s| VISIVILITY vis:%d",
             "COMP_EVENT", ev->visible);
#ifdef _F_ZONE_WINDOW_ROTATION_
        if ((e_config->wm_win_rotation) &&
            (rot.vkbd == bd) && (ev->visible))
          {
             _e_border_vkbd_show_timeout(bd);
          }
#endif /* end of _F_ZONE_WINDOW_ROTATION_ */
     }

   return ECORE_CALLBACK_PASS_ON;
}
#endif /* end of _F_TEMPORARY_COMP_EVENT_ */

#ifdef _F_USE_TILED_DESK_LAYOUT_
static int
_e_border_desk_layout_launch_pos_get(E_Border *bd)
{
   if (!bd) return -1;

   e_border_aux_hint_info_update(bd);
   return bd->launch_info.launch_to;
}

static int
_e_border_desk_layout_launcher_get(E_Border *bd)
{
   if (!bd) return -1;

   e_border_aux_hint_info_update(bd);
   return bd->launch_info.launch_by;
}

static void _e_border_desk_layout_clear(E_Border *bd)
{
   if (!bd) return;

   bd->client.e.state.ly.curr_ly = NULL;
   bd->client.e.state.ly.prev_ly = NULL;
}

static void _e_border_desk_layout_set(E_Border *bd)
{
   E_Desk_Layout *desk_ly = NULL;
   int launched_by;
   int pos;

   if (!e_config->use_tiled_desk_layout) return;
   if (!bd->client.e.state.ly.support) return;
   if (!(bd->desk && bd->desk->ly.tiled)) return;

   launched_by = _e_border_desk_layout_launcher_get(bd);
   pos = _e_border_desk_layout_launch_pos_get(bd);
   if (bd->new_client)
     {
        switch (launched_by)
          {
           case 2: // split-launcher
              desk_ly = e_desk_layout_get(bd->desk, pos);
              break;

           default: // others
              desk_ly = NULL;
              break;
          }
     }
   else
     {
        switch (launched_by)
          {
           case 1: // task manager
              if (bd->client.e.state.ly.curr_ly)
                desk_ly = bd->client.e.state.ly.curr_ly;
              else
                desk_ly = NULL;
              break;

           case 2: // split-launcher
              desk_ly = e_desk_layout_get(bd->desk, pos);
              break;

           default: // others
              // clear split info
              _e_border_desk_layout_clear(bd);
              desk_ly = NULL;
              break;
          }
     }

   e_border_desk_layout_set(bd, desk_ly, EINA_TRUE, EINA_TRUE);
   return;
}
#endif

#ifdef _F_E_WIN_AUX_HINT_
static void
_e_border_aux_hint_job(void *data)
{
   aux_hint_job = NULL;

   Eina_List *ml, *cl;
   E_Manager *man;
   E_Container *con;

   if (!e_config->win_aux_hint)
     return;

   if (!borders)
     return;

   EINA_LIST_FOREACH(e_manager_list(), ml, man)
     {
        EINA_LIST_FOREACH(man->containers, cl, con)
          {
             E_Border_List *bl;
             E_Border *bd;

             bl = e_container_border_list_last(con);
             if (!bl) continue;

             while ((bd = e_container_border_list_prev(bl)))
               {
                  if (bd->changes.aux_hint)
                    {
                       ELBF(ELBT_BD, 0, bd->client.win, "AUX_HINT_EVAL hook");

                       _e_border_hook_call(E_BORDER_HOOK_AUX_HINT_EVAL, bd);

                       E_Border_Aux_Hint *hint;
                       Eina_List *l;
                       EINA_LIST_FOREACH(bd->client.e.state.aux_hint.hints, l, hint)
                         {
                            if (hint->deleted)
                              {
                                 bd->client.e.state.aux_hint.hints = eina_list_remove(bd->client.e.state.aux_hint.hints, hint);

                                 eina_stringshare_del(hint->hint);
                                 eina_stringshare_del(hint->val);
                                 E_FREE(hint);
                              }
                         }
                       bd->changes.aux_hint = 0;
                    }
               }
             e_container_border_list_free(bl);
          }
     }
}

static void
_e_border_aux_hint_handle(E_Border *bd)
{
   if (!bd) return;

   if (bd->client.e.state.aux_hint.support)
     {
        E_Border_Aux_Hint *hint;
        Eina_List *l;
        EINA_LIST_FOREACH(bd->client.e.state.aux_hint.hints, l, hint)
          {
             hint->deleted = 1;
          }

        unsigned char *data = NULL;
        int num = 0;
        int res = ecore_x_window_prop_property_get
           (bd->client.win, ECORE_X_ATOM_E_WINDOW_AUX_HINT,
            ECORE_X_ATOM_STRING, 0, &data, &num);

        if ((res == 8) && (num > 0))
          {
             char **result;
             unsigned int elements, i;
             result = eina_str_split_full((char *)data, ",", -1, &elements);

             for (i = 0; i < elements; i++)
               {
                  char **r = NULL;
                  unsigned int n;
                  r = eina_str_split_full(result[i], ":", -1, &n);

                  if (n == 3)
                    {
                       unsigned int id = (unsigned int)atoi(r[0]);
                       Eina_Bool found = EINA_FALSE;

                       ELBF(ELBT_BD, 0, bd->client.win,
                            "AUX_HINT id:%d %s %s", id, r[1], r[2]);

                       EINA_LIST_FOREACH(bd->client.e.state.aux_hint.hints, l, hint)
                         {
                            if (hint->id == id)
                              {
                                 if (strcmp(hint->val, r[2]) != 0)
                                   {
                                      eina_stringshare_del(hint->val);
                                      hint->val = eina_stringshare_add(r[2]);
                                      hint->changed = 1;
                                      hint->replied = 0;
                                      bd->changes.aux_hint = 1;
                                   }
                                 hint->deleted = 0;
                                 found = EINA_TRUE;
                                 break;
                              }
                         }

                       if (!found)
                         {
                            ELBF(ELBT_BD, 0, bd->client.win, "AUX_HINT ADD id:%d", id);

                            hint = E_NEW(E_Border_Aux_Hint, 1);
                            if (hint)
                              {
                                 hint->id = (unsigned int)atoi(r[0]);
                                 hint->hint = eina_stringshare_add(r[1]);
                                 hint->val = eina_stringshare_add(r[2]);
                                 hint->changed = 1;
                                 hint->replied = 0;
                                 hint->deleted = 0;
                                 bd->changes.aux_hint = 1;

                                 bd->client.e.state.aux_hint.hints = eina_list_append(bd->client.e.state.aux_hint.hints, hint);
                              }
                         }
                    }
                  if (r && r[0]) free(r[0]);
                  free(r);
               }
             if (result && result[0]) free(result[0]);
             free(result);
          }

        /* set bd->changes.aux_hint if deleted flag exists */
        EINA_LIST_FOREACH(bd->client.e.state.aux_hint.hints, l, hint)
          {
             if (hint->deleted)
               {
                  bd->changes.aux_hint = 1;
                  break;
               }
          }

        free(data);
     }
}
#endif /* end of _F_E_WIN_AUX_HINT_ */

EAPI void
e_border_show(E_Border *bd)
{
   E_Event_Border_Show *ev;
   unsigned int visible;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (bd->visible) return;
   if (bd->client.e.state.rot.pending_show) return;
   traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:BORDER:SHOW");
#ifdef _F_ZONE_WINDOW_ROTATION_
   // newly created window that has to be rotated will be show after rotation done.
   // so, skip at this time. it will be called again after GETTING ROT_DONE.
   if ((bd->new_client) &&
       (bd->client.e.state.rot.ang.next != -1))
     {
        bd->client.e.state.rot.pending_show = 1;
        traceEnd(TTRACE_TAG_WINDOW_MANAGER);
        return;
     }
   if ((e_config->wm_win_rotation) &&
       (rot.vkbd_ctrl_win) && (rot.vkbd) &&
       (bd == rot.vkbd) &&
       (!rot.vkbd_show_prepare_done))
     {
        ELB(ELBT_BD, "SEND KBD_ON_PREPARE", bd->client.win);
        ecore_x_e_virtual_keyboard_on_prepare_request_send(rot.vkbd_ctrl_win);
        if (rot.vkbd_show_prepare_timer)
          ecore_timer_del(rot.vkbd_show_prepare_timer);
        rot.vkbd_show_prepare_timer = ecore_timer_add(1.5f,
                                                      _e_border_vkbd_show_prepare_timeout,
                                                      bd);
        traceEnd(TTRACE_TAG_WINDOW_MANAGER);
        return;
     }
   ELB(ELBT_BD, "BD_SHOW", bd->client.win);
   LOGE("BD_SHOW(0x%08x)", bd->client.win);
#endif
   ecore_x_window_shadow_tree_flush();
   e_container_shape_show(bd->shape);
   if (!bd->need_reparent)
     ecore_x_window_show(bd->client.win);
   e_hints_window_visible_set(bd);
   bd->hidden = 0;
   bd->visible = 1;
   bd->changes.visible = 1;

   visible = 1;
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MAPPED, &visible, 1);
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MANAGED, &visible, 1);

   ev = E_NEW(E_Event_Border_Show, 1);
   if (ev)
     {
        ev->border = bd;
        e_object_ref(E_OBJECT(bd));
        //   e_object_breadcrumb_add(E_OBJECT(bd), "border_show_event");
        ecore_event_add(E_EVENT_BORDER_SHOW, ev, _e_border_event_border_show_free, NULL);
     }

#ifdef _F_ZONE_WINDOW_ROTATION_
   if ((e_config->wm_win_rotation) &&
       ((bd->client.e.state.rot.support) ||
        (bd->client.e.state.rot.app_set)))
     {
        if (bd->client.e.state.rot.ang.next == -1)
          {
             ELB(ELBT_ROT, "ROT_CHECK", bd->client.win);
             int rotation = e_border_rotation_recommend_angle_get(bd);
             if (rotation != -1) e_border_rotation_set(bd, rotation);
          }
     }
#endif
#ifdef _F_USE_TILED_DESK_LAYOUT_
   if (e_config->use_tiled_desk_layout)
     {
        if (!bd->iconic)
          {
             _e_border_desk_layout_set(bd);
          }
     }
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
   traceEnd(TTRACE_TAG_WINDOW_MANAGER);
}

EAPI void
e_border_hide(E_Border *bd,
              int       manage)
{
   unsigned int visible;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:BORDER:HIDE");

#ifdef _F_ZONE_WINDOW_ROTATION_
   if ((e_config->wm_win_rotation) &&
       (rot.vkbd_ctrl_win) && (rot.vkbd) &&
       (bd == rot.vkbd) &&
       (!rot.vkbd_hide_prepare_done) &&
       (!bd->iconic))
     {
        Eina_Bool need_prepare = EINA_TRUE;
        if (bd->parent)
          {
             if (e_object_is_del(E_OBJECT(bd->parent)))
               need_prepare = EINA_FALSE;
          }
        else
          need_prepare = EINA_FALSE;

        /* If E wait for VKBD_ON_PREPARE_DONE,
         * E doesn't need to wait for that no more,
         * VKBD will be destroyed soon. */
        if (rot.vkbd_show_prepare_timer)
          {
             ecore_timer_del(rot.vkbd_show_prepare_timer);
             rot.vkbd_show_prepare_timer = NULL;
             ELB(ELBT_BD, "DEL VKBD_ON_PREPARE_TIMER, NO NEED TO WAIT FOR THAT.", bd->client.win);
          }

        ELBF(ELBT_BD, 0, bd->client.win, "SEND KBD_OFF_PREPARE:%d", need_prepare);
        if (need_prepare)
          {
             e_object_ref(E_OBJECT(bd));
             ecore_x_e_virtual_keyboard_off_prepare_request_send(rot.vkbd_ctrl_win);

             if (rot.vkbd_hide_prepare_timer)
               {
                  ecore_timer_del(rot.vkbd_hide_prepare_timer);
                  e_object_unref(E_OBJECT(rot.vkbd));
               }
             rot.vkbd_hide_prepare_timer = NULL;

             if (rot.vkbd_hide_timer)
               {
                  ecore_timer_del(rot.vkbd_hide_timer);
                  e_object_unref(E_OBJECT(rot.vkbd));
               }
             rot.vkbd_hide_timer = NULL;

             rot.vkbd_hide_prepare_timer = ecore_timer_add(1.5f,
                                                           _e_border_vkbd_hide_prepare_timeout,
                                                           bd);
             traceEnd(TTRACE_TAG_WINDOW_MANAGER);
             return;
          }
        else
          {
             e_object_ref(E_OBJECT(bd));

             /* In order to clear conformant area properly, WM should send keyboard off prepare request event */
             ecore_x_e_virtual_keyboard_off_prepare_request_send(rot.vkbd_ctrl_win);

             /* cleanup code from _e_border_vkbd_hide() */
             rot.vkbd_hide_prepare_done = EINA_TRUE;

             if (rot.vkbd_hide_prepare_timer)
               {
                  ecore_timer_del(rot.vkbd_hide_prepare_timer);
                  e_object_unref(E_OBJECT(rot.vkbd));
               }
             rot.vkbd_hide_prepare_timer = NULL;

             if (rot.vkbd_hide_timer)
               {
                  ecore_timer_del(rot.vkbd_hide_timer);
                  e_object_unref(E_OBJECT(rot.vkbd));
               }
             rot.vkbd_hide_timer = NULL;

             rot.vkbd_hide_timer = ecore_timer_add(0.03f, _e_border_vkbd_hide_timeout, bd);
          }
     }
   /* if vkbd's parent has to be hidden,
    * vkbd and prediction widnow will be hidden with its parent.
    */
   else if ((bd == rot.vkbd_parent) && (!bd->iconic))
     {
        ELBF(ELBT_BD, 0, bd->client.win, "VKBD'S PARENT HIDE visible:%d", bd->visible);
        if ((rot.vkbd) && (rot.vkbd->visible))
          {
             ELB(ELBT_BD, "DO HIDE VKBD WITH ITS PARENT", rot.vkbd->client.win);
             _e_border_vkbd_hide(rot.vkbd, EINA_FALSE);
             ecore_x_window_hide(rot.vkbd->client.win);
          }
        if ((rot.vkbd_prediction) && (rot.vkbd_prediction->visible))
          {
             ELB(ELBT_BD, "PREDICTION HIDE WITH ITS PARENT", rot.vkbd_prediction->client.win);
             e_border_hide(rot.vkbd_prediction, 0);
             ecore_x_window_hide(rot.vkbd_prediction->client.win);
          }
     }
   // clear pending_show, because this window is hidden now.
   bd->client.e.state.rot.pending_show = 0;
   ELBF(ELBT_BD, 0, bd->client.win, "HIDE visible:%d", bd->visible);
   LOGE("BD_HIDE(0x%08x), visible:%d", bd->client.win, bd->visible);
#endif
   if (!bd->visible) goto send_event;
   ecore_x_window_shadow_tree_flush();
   if (bd->moving)
     _e_border_move_end(bd);
   if (bd->resize_mode != RESIZE_NONE)
     {
        _e_border_pointer_resize_end(bd);
        bd->resize_mode = RESIZE_NONE;
        _e_border_resize_end(bd);
     }

   e_container_shape_hide(bd->shape);
   if (!bd->iconic) e_hints_window_hidden_set(bd);

   bd->visible = 0;
   bd->changes.visible = 1;

   if (!bd->need_reparent)
     {
        if ((bd->focused) ||
            (e_grabinput_last_focus_win_get() == bd->client.win))
          {
             e_border_focus_set(bd, 0, 1);
             if (manage != 2)
               {
                  E_Border *pbd;
                  E_Container *con;
                  E_Zone *zone;
                  E_Desk *desk;

                  con = e_container_current_get(e_manager_current_get());
                  zone = e_zone_current_get(con);
                  desk = e_desk_current_get(zone);

                  if ((bd->parent) &&
                      (bd->parent->desk == desk) && (bd->parent->modal == bd))
                    {
                       if (manage == 0 && bd->parent->saved.event_mask)
                         {
                            ecore_x_event_mask_unset(bd->parent->client.win, ECORE_X_EVENT_MASK_WINDOW_DAMAGE | ECORE_X_EVENT_MASK_WINDOW_PROPERTY);
                            ecore_x_event_mask_set(bd->parent->client.win, bd->parent->saved.event_mask);
                         }
#ifdef _F_FOCUS_WINDOW_IF_TOP_STACK_
                       if (bd->parent->client.icccm.accepts_focus ||
                           bd->parent->client.icccm.take_focus)
                         {
                            _e_border_transient_next_focus_set(bd);
                         }
                       else
                         {
                            _e_border_latest_stacked_focus(bd);
                         }
#else
                       e_border_focus_set(bd->parent, 1, 1);
#endif
                    }
                  else if (e_config->focus_revert_on_hide_or_close)
                    {
                       /* When using pointer focus, the border under the
                        * pointer (if any) gets focused, in sloppy/click
                        * focus the last focused window on the current
                        * desk gets focus */
                       if (e_config->focus_policy == E_FOCUS_MOUSE)
                         {
                            pbd = e_border_under_pointer_get(desk, bd);
                            if (pbd)
                              e_border_focus_set(pbd, 1, 1);
                         }
#ifdef _F_FOCUS_WINDOW_IF_TOP_STACK_
                       else if ((e_config->focus_setting == E_FOCUS_NEW_WINDOW_IF_TOP_STACK) &&
                                (e_config->focus_policy == E_FOCUS_CLICK))
                         {
                            if ((bd->parent) &&
                                (bd->parent->desk == desk) &&
                                ((bd->parent->client.icccm.accepts_focus ||
                                  bd->parent->client.icccm.take_focus)))
                              {
                                 _e_border_transient_next_focus_set(bd);
                              }
                            else
                              {
                                 _e_border_latest_stacked_focus(bd);
                              }
                         }
#endif
                       else
                         e_desk_last_focused_focus(desk);
                    }
               }
          }
        switch (manage)
          {
           case 2: break;
           case 3:
             bd->hidden = 1;
           case 1:
             /* Make sure that this border isn't deleted */
             bd->await_hide_event++;
           default:
             if (!e_manager_comp_evas_get(bd->zone->container->manager))
               ecore_x_window_hide(bd->client.win);
          }
     }

   visible = 0;
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MAPPED, &visible, 1);
   if (!manage)
     ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MANAGED, &visible, 1);

   bd->post_show = 0;

#ifdef _F_USE_BORDER_TRANSFORM_
   //remove transform childs if exits
   if (bd->transform.parent)
     {
        E_Border *bd_parent = e_border_find_by_client_window(bd->transform.parent);
        if (bd_parent)
           e_border_transform_child_remove(bd, bd_parent);
     }

   if (bd->transform.child_wins)
     {
        Eina_List *l = NULL;
        E_Border *temp_bd = NULL;

        EINA_LIST_FOREACH(bd->transform.child_wins, l, temp_bd)
          {
             e_border_transform_child_remove(temp_bd, bd);
          }
        eina_list_free(bd->transform.child_wins);
        bd->transform.child_wins = NULL;
     }
#endif /* end of _F_USE_BORDER_TRANSFORM_ */
   traceEnd(TTRACE_TAG_WINDOW_MANAGER);

send_event:
   if (!stopping)
     {
        E_Event_Border_Hide *ev;

        ev = E_NEW(E_Event_Border_Hide, 1);
        if (ev)
          {
             ev->border = bd;
             e_object_ref(E_OBJECT(bd));
             //	e_object_breadcrumb_add(E_OBJECT(bd), "border_hide_event");
             ecore_event_add(E_EVENT_BORDER_HIDE, ev, _e_border_event_border_hide_free, NULL);
          }
     }
   traceEnd(TTRACE_TAG_WINDOW_MANAGER);
}

static void
_pri_adj(int pid, int set, int adj, Eina_Bool use_adj, Eina_Bool adj_children, Eina_Bool do_children)
{
   int newpri = set;

   if (use_adj) newpri = getpriority(PRIO_PROCESS, pid) + adj;
   setpriority(PRIO_PROCESS, pid, newpri);
// shouldnt need to do this as default ionice class is "none" (0), and
// this inherits io priority FROM nice level
//        ioprio_set(IOPRIO_WHO_PROCESS, pid,
//                   IOPRIO_PRIO_VALUE(2, 5));
   if (do_children)
     {
        Eina_List *files;
        char *file, buf[PATH_MAX];
        FILE *f;
        int pid2, ppid;

        // yes - this is /proc specific... so this may not work on some
        // os's - works on linux. too bad for others.
        files = ecore_file_ls("/proc");
        EINA_LIST_FREE(files, file)
          {
             if (isdigit(file[0]))
               {
                  snprintf(buf, sizeof(buf), "/proc/%s/stat", file);
                  f = fopen(buf, "r");
                  if (f)
                    {
                       pid2 = -1;
                       ppid = -1;
                       if (fscanf(f, "%i %*s %*s %i %*s", &pid2, &ppid) == 2)
                         {
                            fclose(f);
                            if (ppid == pid)
                              {
                                 if (adj_children)
                                    _pri_adj(pid2, set, adj, EINA_TRUE,
                                             adj_children, do_children);
                                 else
                                    _pri_adj(pid2, set, adj, use_adj,
                                             adj_children, do_children);
                              }
                         }
                       else fclose(f);
                    }
               }
             free(file);
          }
     }
}

static void
_e_border_pri_raise(E_Border *bd)
{
   if (!e_config->priority_raise_on_focus) return;

   if (bd->client.netwm.pid <= 0) return;
   if (bd->client.netwm.pid == getpid()) return;
   _pri_adj(bd->client.netwm.pid,
            e_config->priority - 1, -1, EINA_FALSE,
//            EINA_TRUE, EINA_TRUE);
            EINA_TRUE, EINA_FALSE);
//   printf("WIN: pid %i, title %s (HI!!!!!!!!!!!!!!!!!!)\n",
//          bd->client.netwm.pid, e_border_name_get(bd));
}

static void
_e_border_pri_norm(E_Border *bd)
{
   if (!e_config->priority_raise_on_focus) return;

   if (bd->client.netwm.pid <= 0) return;
   if (bd->client.netwm.pid == getpid()) return;
   _pri_adj(bd->client.netwm.pid,
            e_config->priority, 1, EINA_FALSE,
//            EINA_TRUE, EINA_TRUE);
            EINA_TRUE, EINA_FALSE);
//   printf("WIN: pid %i, title %s (NORMAL)\n",
//          bd->client.netwm.pid, e_border_name_get(bd));
}

static void
_e_border_frame_replace(E_Border *bd, Eina_Bool argb)
{
   Ecore_X_Window win;
   Ecore_Evas *bg_ecore_evas;
   char buf[4096];

   bd->argb = argb;

   win = bd->win;
   bg_ecore_evas = bd->bg_ecore_evas;

   /* unregister old frame window */
   eina_hash_del(borders_hash, e_util_winid_str_get(bd->bg_win), bd);
   eina_hash_del(borders_hash, e_util_winid_str_get(bd->win), bd);

   e_focus_setdown(bd);
   e_bindings_mouse_ungrab(E_BINDING_CONTEXT_WINDOW, bd->win);
   e_bindings_wheel_ungrab(E_BINDING_CONTEXT_WINDOW, bd->win);

   if (bd->icon_object)
     evas_object_del(bd->icon_object);

   evas_object_del(bd->bg_object);
   e_canvas_del(bg_ecore_evas);
   ecore_evas_free(bg_ecore_evas);

   if (bd->pointer)
     e_object_del(E_OBJECT(bd->pointer));

   /* create new frame */
   if (argb)
     bd->win = ecore_x_window_manager_argb_new(bd->zone->container->win,
                 bd->x, bd->y, bd->w, bd->h);
   else
     {
        bd->win = ecore_x_window_override_new(bd->zone->container->win,
                bd->x, bd->y, bd->w, bd->h);
        ecore_x_window_shape_events_select(bd->win, 1);
     }

   ecore_x_window_configure(bd->win,
          ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
          ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
          0, 0, 0, 0, 0,
          win, ECORE_X_WINDOW_STACK_BELOW);

   e_bindings_mouse_grab(E_BINDING_CONTEXT_WINDOW, bd->win);
   e_bindings_wheel_grab(E_BINDING_CONTEXT_WINDOW, bd->win);
   e_focus_setup(bd);

   bd->bg_ecore_evas = e_canvas_new(bd->win,
                                    0, 0, bd->w, bd->h, 1, 0,
                                    &(bd->bg_win));

   e_canvas_add(bd->bg_ecore_evas);
   ecore_x_window_reparent(bd->event_win, bd->win, 0, 0);

   bd->bg_evas = ecore_evas_get(bd->bg_ecore_evas);
   ecore_evas_name_class_set(bd->bg_ecore_evas, "E", "Frame_Window");
   ecore_evas_title_set(bd->bg_ecore_evas, "Enlightenment Frame");

   ecore_x_window_shape_events_select(bd->bg_win, 1);

   /* move client with shell win over to new frame */
   ecore_x_window_reparent(bd->client.shell_win, bd->win,
         bd->client_inset.l, bd->client_inset.t);

   bd->pointer = e_pointer_window_new(bd->win, 0);

   eina_hash_add(borders_hash, e_util_winid_str_get(bd->bg_win), bd);
   eina_hash_add(borders_hash, e_util_winid_str_get(bd->win), bd);

   if (bd->visible)
     {
        E_Border *tmp;
        Eina_List *l;

        ecore_evas_show(bd->bg_ecore_evas);
        ecore_x_window_show(bd->win);

        EINA_LIST_FOREACH(bd->client.e.state.video_child, l, tmp)
          ecore_x_window_show(tmp->win);
     }

   bd->bg_object = edje_object_add(bd->bg_evas);
   snprintf(buf, sizeof(buf), "e/widgets/border/%s/border", bd->client.border.name);
   e_theme_edje_object_set(bd->bg_object, "base/theme/borders", buf);

   bd->icon_object = e_border_icon_add(bd, bd->bg_evas);

   /* cleanup old frame */
   ecore_x_window_free(win);
}

static void
_e_border_client_move_resize_send(E_Border *bd)
{
   if (bd->internal_ecore_evas)
     ecore_evas_managed_move(bd->internal_ecore_evas,
                             bd->x + bd->fx.x + bd->client_inset.l,
                             bd->y + bd->fx.y + bd->client_inset.t);

#ifdef _F_MOVE_RESIZE_SEND_PATCH_
   bd->client.icccm.move_resize_send.x = bd->x + bd->fx.x + bd->client_inset.l;
   bd->client.icccm.move_resize_send.y = bd->y + bd->fx.y + bd->client_inset.t;
   bd->client.icccm.move_resize_send.w = bd->client.w;
   bd->client.icccm.move_resize_send.h = bd->client.h;
   bd->client.icccm.move_resize_send.need_send = 1;
   bd->changed = 1;
#else
   ecore_x_icccm_move_resize_send(bd->client.win,
                                  bd->x + bd->fx.x + bd->client_inset.l,
                                  bd->y + bd->fx.y + bd->client_inset.t,
                                  bd->client.w,
                                  bd->client.h);
#endif
}

static void
_e_border_pending_move_resize_add(E_Border    *bd,
                                  int          move,
                                  int          resize,
                                  int          x,
                                  int          y,
                                  int          w,
                                  int          h,
                                  Eina_Bool    without_border,
                                  unsigned int serial)
{
   E_Border_Pending_Move_Resize *pnd;

   pnd = E_NEW(E_Border_Pending_Move_Resize, 1);
   if (!pnd) return;
   pnd->resize = resize;
   pnd->move = move;
   pnd->without_border = without_border;
   pnd->x = x;
   pnd->y = y;
   pnd->w = w;
   pnd->h = h;
   pnd->serial = serial;
   bd->pending_move_resize = eina_list_append(bd->pending_move_resize, pnd);
}

static void
_e_border_move_internal(E_Border *bd,
                        int       x,
                        int       y,
                        Eina_Bool without_border)
{
   E_Event_Border_Move *ev;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   ecore_x_window_shadow_tree_flush();
   if (bd->new_client)
     {
        _e_border_pending_move_resize_add(bd, 1, 0, x, y, 0, 0, without_border, 0);
        return;
     }

   if (bd->maximized)
     {
       if ((bd->maximized & E_MAXIMIZE_DIRECTION) != E_MAXIMIZE_BOTH)
         {
            if (e_config->allow_manip)
              bd->maximized = 0;

            if ((bd->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_HORIZONTAL)
              {
                  x = bd->x;
              }
            else
            if ((bd->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_VERTICAL)
              {
                  y = bd->y;
              }
         }
       else if (e_config->allow_manip)
         bd->maximized = 0;
       else
         return;
     }

   if (without_border)
     {
        x -= bd->client_inset.l;
        y -= bd->client_inset.t;
     }
   if (bd->move_intercept_cb)
     {
        int px, py;
        px = bd->x, py = bd->y;
        bd->move_intercept_cb(bd, x, y);
        if ((bd->x == px) && (bd->y == py)) return;
     }
   else if ((x == bd->x) && (y == bd->y)) return;
   bd->pre_res_change.valid = 0;
   bd->x = x;
   bd->y = y;
   bd->changed = 1;
   bd->changes.pos = 1;
#if 0
   if (bd->client.netwm.sync.request)
     {
        bd->client.netwm.sync.wait++;
        ecore_x_netwm_sync_request_send(bd->client.win, bd->client.netwm.sync.serial++);
     }
#endif
   _e_border_client_move_resize_send(bd);
   _e_border_move_update(bd);
   ev = E_NEW(E_Event_Border_Move, 1);
   if (ev)
     {
        ev->border = bd;
        e_object_ref(E_OBJECT(bd));
        //  e_object_breadcrumb_add(E_OBJECT(bd), "border_move_event");
        ecore_event_add(E_EVENT_BORDER_MOVE, ev, _e_border_event_border_move_free, NULL);
     }
   _e_border_zone_update(bd);
}

/**
 * Move window to coordinates that already account border decorations.
 *
 * This call will consider given position already accounts border
 * decorations, so it will not be considered later. This will just
 * work properly with borders that have being evaluated and border
 * decorations are known (border->client_inset).
 *
 * @parm x horizontal position to place window.
 * @parm y vertical position to place window.
 *
 * @see e_border_move_without_border()
 */
EAPI void
e_border_move(E_Border *bd,
              int       x,
              int       y)
{
   if (bd->fullscreen)
     return;

   _e_border_move_internal(bd, x, y, 0);
}


/**
 * Set a callback which will be called just prior to updating the
 * move coordinates for a border
 */
EAPI void
e_border_move_intercept_cb_set(E_Border *bd, E_Border_Move_Intercept_Cb cb)
{
   bd->move_intercept_cb = cb;
}

/**
 * Move window to coordinates that do not account border decorations yet.
 *
 * This call will consider given position does not account border
 * decoration, so these values (border->client_inset) will be
 * accounted automatically. This is specially useful when it is a new
 * client and has not be evaluated yet, in this case
 * border->client_inset will be zeroed and no information is known. It
 * will mark pending requests so border will be accounted on
 * evalutation phase.
 *
 * @parm x horizontal position to place window.
 * @parm y vertical position to place window.
 *
 * @see e_border_move()
 */
EAPI void
e_border_move_without_border(E_Border *bd,
                             int       x,
                             int       y)
{
   if (bd->fullscreen)
     return;

   _e_border_move_internal(bd, x, y, 1);
}

EAPI void
e_border_center(E_Border *bd)
{
   int x, y, w, h;
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   e_zone_useful_geometry_get(bd->zone, &x, &y, &w, &h);
   e_border_move(bd, x + (w - bd->w) / 2, y + (h - bd->h) / 2);
}

EAPI void
e_border_center_pos_get(E_Border *bd,
                        int      *x,
                        int      *y)
{
   int zx, zy, zw, zh;
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   e_zone_useful_geometry_get(bd->zone, &zx, &zy, &zw, &zh);
   if (x) *x = zx + (zw - bd->w) / 2;
   if (y) *y = zy + (zh - bd->h) / 2;
}

EAPI void
e_border_fx_offset(E_Border *bd,
                   int       x,
                   int       y)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if ((x == bd->fx.x) && (y == bd->fx.y)) return;
   bd->fx.x = x;
   bd->fx.y = y;

   bd->changes.pos = 1;
   bd->changed = 1;

   if (bd->moving) _e_border_move_update(bd);
}

static void
_e_border_move_resize_internal(E_Border *bd,
                               int       x,
                               int       y,
                               int       w,
                               int       h,
                               Eina_Bool without_border,
                               Eina_Bool move)
{
   E_Event_Border_Move *mev;
   E_Event_Border_Resize *rev;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   ecore_x_window_shadow_tree_flush();

   if (bd->new_client)
     {
        _e_border_pending_move_resize_add(bd, move, 1, x, y, w, h, without_border, 0);
        return;
     }

   if (bd->maximized)
     {
       if ((bd->maximized & E_MAXIMIZE_DIRECTION) != E_MAXIMIZE_BOTH)
         {
            if (e_config->allow_manip)
              bd->maximized = 0;

            if ((bd->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_HORIZONTAL)
              {
                  x = bd->x;
                  w = bd->w;
              }
            else
            if ((bd->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_VERTICAL)
              {
                  y = bd->y;
                  h = bd->h;
              }
         }
       else
       if (e_config->allow_manip)
         bd->maximized = 0;
       else
         return;
     }

   if (without_border)
     {
        x -= bd->client_inset.l;
        y -= bd->client_inset.t;
        w += (bd->client_inset.l + bd->client_inset.r);
        h += (bd->client_inset.t + bd->client_inset.b);
     }

   if ((!move || ((x == bd->x) && (y == bd->y))) &&
       (w == bd->w) && (h == bd->h))
     return;

   bd->pre_res_change.valid = 0;
   if (move)
     {
        bd->changes.pos = 1;
        bd->x = x;
        bd->y = y;
     }
   bd->w = w;
   bd->h = h;
   bd->client.w = bd->w - (bd->client_inset.l + bd->client_inset.r);
   bd->client.h = bd->h - (bd->client_inset.t + bd->client_inset.b);

#ifdef _F_USE_ICONIFY_RESIZE_
   if (e_config->use_iconify_resize)
     {
        if (bd->iconify_resize.support)
          {
             if (bd->iconify_resize.set || bd->iconify_resize.need_draw)
               {
                  ELBF(ELBT_BD, 0, bd->client.win,"Iconify Resize update: Geo (%d,%d,%d,%d) -> (%d,%d,%d,%d)",
                       bd->iconify_resize.x, bd->iconify_resize.y, bd->iconify_resize.w, bd->iconify_resize.h, bd->x, bd->y, bd->w, bd->h);
                  bd->iconify_resize.x = bd->x;
                  bd->iconify_resize.y = bd->y;
                  bd->iconify_resize.w = bd->w;
                  bd->iconify_resize.h = bd->h;
               }
          }
     }
#endif

   if ((bd->shaped) || (bd->client.shaped))
     {
        bd->need_shape_merge = 1;
        bd->need_shape_export = 1;
     }
   if (bd->shaped_input)
     {
        bd->need_shape_merge = 1;
     }

   if (bd->internal_ecore_evas)
     {
        bd->changed = 1;
        bd->changes.size = 1;
     }
   else
     {
        if (bdresize && bd->client.netwm.sync.request)
          {
             bd->client.netwm.sync.wait++;
             /* Don't use x and y as supplied to this function, as it is called with 0, 0
              * when no move is intended.  The border geometry is set above anyways.
              */
             _e_border_pending_move_resize_add(bd, move, 1, bd->x, bd->y, bd->w, bd->h, without_border,
                                               bd->client.netwm.sync.serial);
             ecore_x_netwm_sync_request_send(bd->client.win,
                                             bd->client.netwm.sync.serial++);
          }
        else
          {
             bd->changed = 1;
             bd->changes.size = 1;
          }
     }

   _e_border_client_move_resize_send(bd);

   _e_border_resize_update(bd);
   if (move)
     {
        mev = E_NEW(E_Event_Border_Move, 1);
        if (mev)
          {
             mev->border = bd;
             e_object_ref(E_OBJECT(bd));
             //   e_object_breadcrumb_add(E_OBJECT(bd), "border_move_event");
             ecore_event_add(E_EVENT_BORDER_MOVE, mev, _e_border_event_border_move_free, NULL);
          }
     }

   rev = E_NEW(E_Event_Border_Resize, 1);
   if (rev)
     {
        rev->border = bd;
        e_object_ref(E_OBJECT(bd));
        //   e_object_breadcrumb_add(E_OBJECT(bd), "border_resize_event");
        ecore_event_add(E_EVENT_BORDER_RESIZE, rev, _e_border_event_border_resize_free, NULL);
     }
   _e_border_zone_update(bd);
}

/**
 * Move and resize window to values that already account border decorations.
 *
 * This call will consider given values already accounts border
 * decorations, so it will not be considered later. This will just
 * work properly with borders that have being evaluated and border
 * decorations are known (border->client_inset).
 *
 * @parm x horizontal position to place window.
 * @parm y vertical position to place window.
 * @parm w horizontal window size.
 * @parm h vertical window size.
 *
 * @see e_border_move_resize_without_border()
 */
EAPI void
e_border_move_resize(E_Border *bd,
                     int       x,
                     int       y,
                     int       w,
                     int       h)
{
   if (bd->fullscreen)
     return;

   _e_border_move_resize_internal(bd, x, y, w, h, 0, 1);
}

/**
 * Move and resize window to values that do not account border decorations yet.
 *
 * This call will consider given values already accounts border
 * decorations, so it will not be considered later. This will just
 * work properly with borders that have being evaluated and border
 * decorations are known (border->client_inset).
 *
 * @parm x horizontal position to place window.
 * @parm y vertical position to place window.
 * @parm w horizontal window size.
 * @parm h vertical window size.
 *
 * @see e_border_move_resize()
 */
EAPI void
e_border_move_resize_without_border(E_Border *bd,
                                    int       x,
                                    int       y,
                                    int       w,
                                    int       h)
{
   if (bd->fullscreen)
     return;

   _e_border_move_resize_internal(bd, x, y, w, h, 1, 1);
}

/**
 * Resize window to values that already account border decorations.
 *
 * This call will consider given size already accounts border
 * decorations, so it will not be considered later. This will just
 * work properly with borders that have being evaluated and border
 * decorations are known (border->client_inset).
 *
 * @parm w horizontal window size.
 * @parm h vertical window size.
 *
 * @see e_border_resize_without_border()
 */
EAPI void
e_border_resize(E_Border *bd,
                int       w,
                int       h)
{
   if (bd->fullscreen)
     return;

   _e_border_move_resize_internal(bd, 0, 0, w, h, 0, 0);
}

#ifdef _F_ZONE_WINDOW_ROTATION_
/**
 * @describe
 *  Get current rotoation state.
 * @param      bd             border
 * @return     EINA_FALSE     the state that does not rotating.
 *             EINA_TRUE      the state that rotating.
 */
EAPI Eina_Bool
e_border_rotation_is_progress(E_Border *bd)
{
   if (!bd) return EINA_FALSE;

   if (bd->client.e.state.rot.ang.next == -1)
     return EINA_FALSE;
   else
     return EINA_TRUE;
}

/**
 * @describe
 *  Check if this border is rotatable to given angle.
 * @param      bd             border
 * @param      ang            test angle.
 * @return     EINA_FALSE     can't be rotated.
 *             EINA_TRUE      can be rotated.
 */
EAPI Eina_Bool
e_border_rotation_is_available(E_Border *bd, int ang)
{
   if (!bd) return EINA_FALSE;

   ELBF(ELBT_ROT, 0, bd->client.win, "CHECK ROTATABLE rot:%d", ang);
   if (ang < 0) goto fail;
   if ((!bd->client.e.state.rot.support) && (!bd->client.e.state.rot.app_set)) goto fail;
   if (e_object_is_del(E_OBJECT(bd))) goto fail;

   if (bd->client.e.state.rot.preferred_rot == -1)
     {
        unsigned int i;

        if (bd->client.e.state.rot.app_set)
          {
             if (bd->client.e.state.rot.available_rots &&
                 bd->client.e.state.rot.count)
               {
                  Eina_Bool found = EINA_FALSE;
                  for (i = 0; i < bd->client.e.state.rot.count; i++)
                    {
                       if (bd->client.e.state.rot.available_rots[i] == ang)
                         {
                            found = EINA_TRUE;
                         }
                    }
                  if (found) goto success;
               }
          }
        else
          {
             goto success;
          }
     }
   else if (bd->client.e.state.rot.preferred_rot == ang) goto success;

fail:
   ELBF(ELBT_ROT, 0, bd->client.win, "CAN'T BE ROTATED rot:%d", ang);
   return EINA_FALSE;
success:
   ELBF(ELBT_ROT, 0, bd->client.win, "BE ROTATABLE rot:%d", ang);
   return EINA_TRUE;
}

/**
 * @describe
 *  Get the list of available rotation of border.
 * @param      bd             border
 * @return     Eina_List*     the list consist of integer value of available rotation angle.
 * @caution                   the caller is responsible for freeing the list after use.
 */
EAPI Eina_List*
e_border_rotation_available_list_get(E_Border *bd)
{
   Eina_List *list = NULL;
   int *element;
   unsigned int i;

   E_OBJECT_CHECK_RETURN(bd, EINA_FALSE);
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, EINA_FALSE);

   if ((!bd->client.e.state.rot.support) && (!bd->client.e.state.rot.app_set)) return list;

   if (bd->client.e.state.rot.preferred_rot != -1)
     {
        element = (int *)malloc(1 * sizeof(int));
        if (element)
          {
             *element = bd->client.e.state.rot.preferred_rot;
             list = eina_list_append(list, element);
          }
        else
          ELB(ELBT_BD, "Out of Memory: malloc(sizeof(int))", 0);
     }
   else if (bd->client.e.state.rot.count > 0)
     {
        if (bd->client.e.state.rot.available_rots)
          {
             for (i = 0; i < bd->client.e.state.rot.count; i++)
               {
                  element = (int *)malloc(1 * sizeof(int));
                  if (!element) continue;
                  *element = bd->client.e.state.rot.available_rots[i];
                  list = eina_list_append(list, element);
               }
          }
     }

   return list;
}

/**
 * @describe
 *  Get current rotation angle.
 * @param      bd             border
 * @return     int            current angle
 */
EAPI int
e_border_rotation_curr_angle_get(E_Border *bd)
{
   E_OBJECT_CHECK_RETURN(bd, -1);
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, -1);

   return bd->client.e.state.rot.ang.curr;
}

/**
 * @describe
 *  Get being replaced rotation angle.
 * @param      bd             border
 * @return     int            be replaced angle.
 */
EAPI int
e_border_rotation_next_angle_get(E_Border *bd)
{
   E_OBJECT_CHECK_RETURN(bd, -1);
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, -1);

   return bd->client.e.state.rot.ang.next;
}

/**
 * @describe
 *  Get previous angle get
 * @param      bd             border
 * @return     int            previous angle.
 */
EAPI int
e_border_rotation_prev_angle_get(E_Border *bd)
{
   E_OBJECT_CHECK_RETURN(bd, -1);
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, -1);

   return bd->client.e.state.rot.ang.prev;
}

/**
 * @describe
 *  Get the angle that this border should be rotated.
 * @param      bd             border
 * @return     Eina_Bool      -1: There is no need to be rotated.
 *                            != -1: The angle that this border should be rotated.
 */
EAPI int
e_border_rotation_recommend_angle_get(E_Border *bd)
{
   E_Zone *zone = bd->zone;
   int ret_ang = -1;

   E_OBJECT_CHECK_RETURN(bd, ret_ang);
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, ret_ang);

   if (!e_config->wm_win_rotation) goto end;
   if (bd->client.e.state.rot.type == E_BORDER_ROTATION_TYPE_DEPENDENT) goto end;
   if ((!bd->client.e.state.rot.app_set) && (!bd->client.e.state.rot.support)) goto end;

   ELB(ELBT_ROT, "GET RECOMMEND ANGLE", bd->client.win);

   if (bd->client.e.state.rot.preferred_rot != -1)
     {
        ret_ang = bd->client.e.state.rot.preferred_rot;
        ELBF(ELBT_ROT, 0, bd->client.win, "PREFERRED rot:%d", ret_ang);
     }
   else if ((bd->client.e.state.rot.available_rots) &&
            (bd->client.e.state.rot.count))
     {
        unsigned int i;
        int current_ang = _e_border_rotation_curr_next_get(bd);
        Eina_Bool found = EINA_FALSE;
        Eina_Bool found_curr_ang = EINA_FALSE;

        if (_e_border_rotation_is_dependent_parent(bd))
          {
             ret_ang = _e_border_rotation_curr_next_get(bd->parent);
             ELBF(ELBT_ROT, 0, bd->client.win, "FOLLOW PARENT rot:%d", ret_ang);
          }
        else
          {
             ret_ang = e_zone_rotation_get(zone);
             ELBF(ELBT_ROT, 0, bd->client.win, "FOLLOW ZONE rot:%d", ret_ang);
          }

        for (i = 0; i < bd->client.e.state.rot.count; i++)
          {
             if (bd->client.e.state.rot.available_rots[i] == ret_ang)
               {
                  found = EINA_TRUE;
                  break;
               }
             if (bd->client.e.state.rot.available_rots[i] == current_ang)
               found_curr_ang = EINA_TRUE;
          }

        if (!found)
          {
             if (found_curr_ang)
               ret_ang = current_ang;
             else
               ret_ang = bd->client.e.state.rot.available_rots[0];
          }
     }
   else
     ELB(ELBT_ROT, "PREFERRED / AVAILABLE LIST ISN'T SET", bd->client.win);

end:
   return ret_ang;
}

/**
 * @describe
 *  Set the rotation of the border given angle.
 * @param      bd             border
 * *param      rotation       angle
 * @return     EINA_TRUE      rotation starts or is already in progress.
 *             EINA_FALSE     fail
 */
EAPI Eina_Bool
e_border_rotation_set(E_Border *bd, int rotation)
{
   Eina_List *list, *l;
   E_Border *child;
   int curr_rot;

   if (!bd) return EINA_FALSE;

   ELB(ELBT_ROT, "TRY ROT_SET", bd->client.win);
   if (rotation < 0) return EINA_FALSE;
   if (!e_border_rotation_is_available(bd, rotation)) return EINA_FALSE;
   traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:BORDER:ROTATION SET");

   // in case same with current angle.
   curr_rot = e_border_rotation_curr_angle_get(bd);
   if (curr_rot == rotation)
     {
        ELBF(ELBT_ROT, 0, bd->client.win, "SAME WITH CURRENT ang:%d", rotation);
        if (e_border_rotation_is_progress(bd))
          {
             // cancel the changes in case only doesn't send request.
             if ((!bd->client.e.state.rot.pending_change_request) &&
                 (!bd->client.e.state.rot.wait_for_done))
               {
                  _e_border_rotation_list_remove(bd);
                  ELB(ELBT_ROT, "\tCANCEL ROTATION CHANGES", bd->client.win);
                  if (bd->client.e.state.rot.pending_show)
                    {
                       bd->client.e.state.rot.pending_show = 0;
                       if (!e_object_is_del(E_OBJECT(bd)))
                         e_border_show(bd);
                    }
                  if (bd->client.e.state.deiconify_approve.pending)
                    _e_border_deiconify_approve_send_pending_end(bd);

                  traceEnd(TTRACE_TAG_WINDOW_MANAGER);
                  return EINA_FALSE;
               }
             else
               ELB(ELBT_ROT, "\tALREADY REQ - CONTINUE... It will be reserved.", bd->client.win);
          }
        else
          {
             traceEnd(TTRACE_TAG_WINDOW_MANAGER);
             return EINA_FALSE;
          }
     }

   // in case same with next angle.
   curr_rot = e_border_rotation_next_angle_get(bd);
   if (curr_rot == rotation)
     {
        ELBF(ELBT_ROT, 0, bd->client.win, "SAME WITH NEXT ang:%d", rotation);
        // if there is reserve angle, remove it.
        if (bd->client.e.state.rot.ang.reserve != -1)
          {
             bd->client.e.state.rot.ang.reserve = -1;
             ELB(ELBT_ROT, "\tCANCEL RESERVED ROT", bd->client.win);
          }
        goto finish;
     }

   /* if this border is rotating now,
    * it will be rotated to this angle after rotation done.
    */
   if ((bd->client.e.state.rot.pending_change_request) ||
       (bd->client.e.state.rot.wait_for_done))
     {
        bd->client.e.state.rot.ang.reserve = rotation;
        ELBF(ELBT_ROT, 0, bd->client.win, "ROT_RESERVE(pending) NEXT:%d != RESERVE:%d",
             bd->client.e.state.rot.ang.next, rotation);
        goto finish;
     }

   /* search rotatable window in this window's child */
   list = _e_border_sub_borders_new(bd);
   EINA_LIST_FOREACH(list, l, child)
     {
        // the window which type is "ECORE_X_WINDOW_TYPE_NORMAL" will be rotated itself.
        // it shouldn't be rotated by rotation state of parent window.
        if (child->client.netwm.type == ECORE_X_WINDOW_TYPE_NORMAL) continue;
        if (e_border_rotation_set(child, rotation))
          {
             ELBF(ELBT_ROT, 0, child->client.win, "ROT_SET(child) curr:%d != TOBE:%d",
                  bd->client.e.state.rot.ang.curr, rotation);
          }
        else
          {
             ELBF(ELBT_ROT, 0, child->client.win, "FAILED ROT_SET (child) curr:%d != TOBE:%d",
                  bd->client.e.state.rot.ang.curr, rotation);
          }
     }
   eina_list_free(list);

   /* if there is vkbd window, send message to prepare rotation */
   if (_e_border_is_vkbd(bd))
     {
        ELB(ELBT_ROT, "PENDING ROT_REQ UNTIL GET PREP_DONE", rot.vkbd_ctrl_win);
        if (rot.prepare_timer)
          ecore_timer_del(rot.prepare_timer);
        rot.prepare_timer = NULL;

        ELB(ELBT_ROT, "SEND ROT_CHANGE_PREPARE", rot.vkbd_ctrl_win);
        ecore_x_e_window_rotation_change_prepare_send(rot.vkbd_ctrl_win,
                                                      rotation,
                                                      EINA_FALSE, 1, 1);
        rot.prepare_timer = ecore_timer_add(4.0f,
                                            _e_border_rotation_change_prepare_timeout,
                                            NULL);

        rot.wait_prepare_done = EINA_TRUE;
     }

   _e_border_event_border_rotation_change_begin_send(bd);

   ELBF(ELBT_ROT, 0, bd->client.win, "ROT_SET curr:%d != TOBE:%d", e_border_rotation_curr_angle_get(bd), rotation);

   bd->client.e.state.rot.pending_change_request = 0;
   bd->client.e.state.rot.ang.next = rotation;
   bd->changes.rotation = 1;
   bd->changed = 1;

finish:
   /* Now the WM has a rotatable window thus we unset variables about zone rotation cancel */
   if (rot.cancel.state)
     {
        rot.cancel.state = EINA_FALSE;
        rot.cancel.zone = NULL;
     }
   traceEnd(TTRACE_TAG_WINDOW_MANAGER);
   return EINA_TRUE;
}
#endif

#ifdef _F_USE_TILED_DESK_LAYOUT_
/* 1. check given desktop layout by policy module
 * 2. resize window fit in desktop layout
 * 3. send E_ATOM_WINDOW_DESKTOP_LAYOUT_CHANGE message to client
 * 4. set desktop layout id value to client window
 * 5. send internal E_EVENT
 */
EAPI void
e_border_desk_layout_set(E_Border      *bd,
                         E_Desk_Layout *desk_ly,
                         Eina_Bool      show,
                         Eina_Bool      hook)
{
   E_Desk_Layout *prev, *curr;
   int id, x, y, w, h;
   Eina_Bool move, resize;

   Eina_List *l;
   E_Border *child;
   Eina_List *list;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if ((!bd->client.e.state.ly.support) && (desk_ly))
      return;

   if (bd->client.illume.win_state.state != ECORE_X_ILLUME_WINDOW_STATE_NORMAL)
      return;

   if (desk_ly == NULL)
     {
        bd->client.e.state.ly.prev_ly = bd->client.e.state.ly.curr_ly;
     }
   bd->client.e.state.ly.curr_ly = desk_ly;
   bd->client.e.state.ly.changing = 1;

   /* policy module would change desktop layout placement of border */
   if (hook)
     {
        if (show)
          {
             int launched_by = bd->launch_info.launch_by;
             if (launched_by == 2 && !bd->iconic)
                e_border_focus_latest_set(bd);

             _e_border_hook_call(E_BORDER_HOOK_DESKTOP_LAYOUT_PRE_SHOW_SET, bd);
          }
        else
          _e_border_hook_call(E_BORDER_HOOK_DESKTOP_LAYOUT_PRE_HIDE_SET, bd);
     }

   /* in above hook, curr_ly could be changed.. */
   prev = bd->client.e.state.ly.prev_ly;
   curr = bd->client.e.state.ly.curr_ly;

   ELBF(ELBT_DESK_LY, 0, bd->client.win, "SET LY prev:%d -> curr:%d",
        prev ? prev->id : -1, curr ? curr->id : -1);

   bd->client.e.state.ly.changing = 0;
   bd->changed = 1;
   bd->changes.pos = 1;
   move = EINA_FALSE;
   resize = EINA_FALSE;

   id = curr ? curr->id : -1;

   if (show)
     e_border_desk_layout_border_update(bd);

   e_border_geometry_get(bd, &x, &y, &w, &h);

   /* reset info for launch info */
   bd->launch_info.launch_by = 1;
   bd->launch_info.launch_to = id;


   if (prev != curr)
     {
        /* send E_ATOM_WINDOW_DESKTOP_LAYOUT_CHANGE message to client */
        ecore_x_client_message32_send(bd->client.win,
                                      E_ATOM_WINDOW_DESKTOP_LAYOUT_CHANGE,
                                      ECORE_X_EVENT_MASK_NONE,
                                      id, x, y, w, h);
     }

   /* set desktop layout id value to client window */
   if (curr)
     {
        ecore_x_window_prop_card32_set(bd->client.win,
                                       E_ATOM_WINDOW_DESKTOP_LAYOUT,
                                       (unsigned int *)&id, 1);
     }
   else
     {
        ecore_x_window_prop_property_del(bd->client.win,
                                         E_ATOM_WINDOW_DESKTOP_LAYOUT);
     }

   if (e_object_is_del(E_OBJECT(bd)))
     {
        // if the bd is deleted, there is no need to send internal EVENT ?
        return;
     }

   /* send internal E_EVENT */
   E_Event_Border_Desk_Layout_Set_Begin *ev;
   ev = E_NEW(E_Event_Border_Desk_Layout_Set_Begin, 1);
   if (ev)
     {
        ev->border = bd;
        ev->show = show;
        e_object_ref(E_OBJECT(bd));
        if (prev) e_object_ref(E_OBJECT(prev));
        if (curr) e_object_ref(E_OBJECT(curr));

        ecore_event_add(E_EVENT_BORDER_DESK_LAYOUT_SET_BEGIN,
                        ev,
                        _e_border_event_border_desk_layout_set_begin_free,
                        NULL);
     }

   /* TODO: remove this code when client protocol is complete. */
   E_Event_Border_Desk_Layout_Set_End *ev1;
   ev1 = E_NEW(E_Event_Border_Desk_Layout_Set_End, 1);
   if (ev1)
     {
        ev1->border = bd;
        ev1->show = show;
        e_object_ref(E_OBJECT(bd));
        if (prev) e_object_ref(E_OBJECT(prev));
        if (curr) e_object_ref(E_OBJECT(curr));

        ecore_event_add(E_EVENT_BORDER_DESK_LAYOUT_SET_END,
                        ev1,
                        _e_border_event_border_desk_layout_set_end_free,
                        NULL);
     }

   //get the transient child and set their layouts as well
   list = _e_border_sub_borders_new(bd);

   EINA_LIST_FOREACH(list, l, child)
     {
        // check if virtual keyboard then don't set
        if (child->client.vkbd.vkbd) continue;
        // check if curr_ly is already there and same as bd, then don't set
        if (!child->client.e.state.ly.curr_ly && !desk_ly) continue;
        if (child->client.e.state.ly.curr_ly && desk_ly)
          {
             if (child->client.e.state.ly.curr_ly->id == desk_ly->id)
                continue;
          }
        if (_e_border_desk_layout_launcher_get(child) != 0)
           e_border_desk_layout_set(child, desk_ly, show, hook);
     }
   eina_list_free(list);
}


EAPI void
e_border_desk_layout_border_update(E_Border *bd)
{
   _e_border_hook_call(E_BORDER_HOOK_DESKTOP_LAYOUT_UPDATE, bd);
}

#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

#ifdef _F_E_WIN_AUX_HINT_
EAPI void
e_border_aux_hint_reply_send(E_Border    *bd,
                             unsigned int id)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (e_config->win_aux_hint)
     {
        if (!bd->client.e.state.aux_hint.support)
          return;

        E_Border_Aux_Hint *hint;
        Eina_List *l;
        EINA_LIST_FOREACH(bd->client.e.state.aux_hint.hints, l, hint)
          {
             if (hint->id == id)
               {
                  if (!hint->replied)
                    {
                       ELBF(ELBT_BD, 0, bd->client.win, "AUX_HINT_ALLOWED Send");

                       ecore_x_client_message32_send(bd->client.win,
                                                     ECORE_X_ATOM_E_WINDOW_AUX_HINT_ALLOWED,
                                                     ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                                     bd->client.win, id, 0, 0, 0);
                       hint->changed = 0;
                       hint->replied = 1;
                    }
                  break;
               }
          }
     }
}

EAPI void
e_border_aux_hint_info_update(E_Border *bd)
{
   if (!bd) return;

   if (bd->client.e.fetch.aux_hint.hints || aux_hint_job)
     {
        if (bd->client.e.state.aux_hint.support)
          {
             _e_border_aux_hint_handle(bd);

             if (bd->changes.aux_hint)
               {
                  if (aux_hint_job)
                     ecore_job_del(aux_hint_job);
                  _e_border_aux_hint_job(NULL);
               }
          }
        bd->client.e.fetch.aux_hint.hints = 0;
     }
}
#endif /* end of _F_E_WIN_AUX_HINT_ */

/**
 * Resize window to values that do not account border decorations yet.
 *
 * This call will consider given size does not account border
 * decoration, so these values (border->client_inset) will be
 * accounted automatically. This is specially useful when it is a new
 * client and has not be evaluated yet, in this case
 * border->client_inset will be zeroed and no information is known. It
 * will mark pending requests so border will be accounted on
 * evalutation phase.
 *
 * @parm w horizontal window size.
 * @parm h vertical window size.
 *
 * @see e_border_resize()
 */
EAPI void
e_border_resize_without_border(E_Border *bd,
                               int       w,
                               int       h)
{
   if (bd->fullscreen)
     return;

   _e_border_move_resize_internal(bd, 0, 0, w, h, 1, 0);
}

EAPI void
e_border_layer_set(E_Border *bd,
                   int       layer)
{
   int oldraise;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   ecore_x_window_shadow_tree_flush();

   oldraise = e_config->transient.raise;

   if (bd->fullscreen)
     {
        bd->saved.layer = layer;
        return;
     }
   bd->layer = layer;
   if (e_config->transient.layer)
     {
        Eina_List *l;
        E_Border *child;
        Eina_List *list = _e_border_sub_borders_new(bd);

        /* We need to set raise to one, else the child wont
         * follow to the new layer. It should be like this,
         * even if the user usually doesn't want to raise
         * the transients.
         */
        e_config->transient.raise = 1;
        EINA_LIST_FOREACH(list, l, child)
          {
             e_border_layer_set(child, layer);
          }
        eina_list_free(list);
     }
   e_border_raise(bd);
   e_config->transient.raise = oldraise;
}

EAPI void
e_border_raise(E_Border *bd)
{
   E_Event_Border_Stack *ev;
   E_Border *last = NULL, *child, *stack;
   E_Stacking type;
   Eina_List *l;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:BORDER:RAISE");

#ifdef _F_USE_VIRT_RESOLUTION_
   _e_mod_check_change_resolution(bd->client.win);
#endif /* end of _F_USE_VIRT_RESOLUTION_ */
   ecore_x_window_shadow_tree_flush();

   if (e_config->transient.raise)
     {
#ifdef _F_FOCUS_WINDOW_IF_TOP_STACK_
        if (e_config->focus_setting != E_FOCUS_NEW_WINDOW_IF_TOP_STACK)
          {
#endif
        Eina_List *list = _e_border_sub_borders_new(bd);

        EINA_LIST_REVERSE_FOREACH(list, l, child)
          {
             /* Don't stack iconic transients. If the user wants these shown,
              * thats another option.
              */
             if (!child->iconic)
               {
                  if (last)
                    e_border_stack_below(child, last);
                  else
                    {
                       E_Border *above;

                       /* First raise the border to find out which border we will end up above */
                       above = e_container_border_raise(child);

                       if (above)
                         {
                            /* We ended up above a border, now we must stack this border to
                             * generate the stacking event, and to check if this transient
                             * has other transients etc.
                             */
                            e_border_stack_above(child, above);
                         }
                       else
                         {
                            /* If we didn't end up above any border, we are on the bottom! */
                            e_border_lower(child);
                         }
                    }
                  last = child;
               }
          }
        eina_list_free(list);
#ifdef _F_FOCUS_WINDOW_IF_TOP_STACK_
          }
        else
          {
             EINA_LIST_FOREACH(bd->transients, l, child)
               {
                /* Don't stack iconic transients. If the user wants these shown,
                 * thats another option.
                 */
                  if (!child->iconic)
                    {
                       child->layer = bd->layer;
                       if (!last) last = child;
                       e_border_raise (child);
                    }
               }
          }
#endif
     }

   if (last)
     {
        e_container_border_stack_below(bd, last);
        stack = last;
        type = E_STACKING_BELOW;
     }
   else
     {
        E_Border *above;

        /* If we don't have any children, raise this border */
        above = e_container_border_raise(bd);
        e_border_raise_latest_set(bd);
        if (above)
          {
             /* We ended up above a border */
              stack = above;
              type = E_STACKING_ABOVE;
          }
        else
          {
             /* No border to raise above, same as a lower! */
              stack = NULL;
              type = E_STACKING_ABOVE;
          }
     }

   ev = E_NEW(E_Event_Border_Stack, 1);
   if (ev)
     {
        ev->border = bd;
        ev->stack = stack;
        ev->type = type;
        e_object_ref(E_OBJECT(bd));
        if (stack)
          e_object_ref(E_OBJECT(stack));
        ecore_event_add(E_EVENT_BORDER_STACK, ev, _e_border_event_border_stack_free, NULL);
     }

   e_remember_update(bd);
   traceEnd(TTRACE_TAG_WINDOW_MANAGER);
}

EAPI void
e_border_lower(E_Border *bd)
{
   E_Event_Border_Stack *ev;
   E_Border *last = NULL, *child, *stack;
   E_Stacking type;
   Eina_List *l;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:BORDER:LOWER");

   ecore_x_window_shadow_tree_flush();

   if (e_config->transient.lower)
     {
#ifdef _F_FOCUS_WINDOW_IF_TOP_STACK_
        if (e_config->focus_setting != E_FOCUS_NEW_WINDOW_IF_TOP_STACK)
          {
#endif
        Eina_List *list = _e_border_sub_borders_new(bd);

        EINA_LIST_REVERSE_FOREACH(list, l, child)
          {
             /* Don't stack iconic transients. If the user wants these shown,
              * thats another option.
              */
             if (!child->iconic)
               {
                  if (last)
                    e_border_stack_below(child, last);
                  else
                    {
                       E_Border *below;

                       /* First lower the border to find out which border we will end up below */
                       below = e_container_border_lower(child);

                       if (below)
                         {
                            /* We ended up below a border, now we must stack this border to
                             * generate the stacking event, and to check if this transient
                             * has other transients etc.
                             */
                            e_border_stack_below(child, below);
                         }
                       else
                         {
                            /* If we didn't end up below any border, we are on top! */
                            e_border_raise(child);
                         }
                    }
                  last = child;
               }
          }
        eina_list_free(list);

#ifdef _F_FOCUS_WINDOW_IF_TOP_STACK_
          }
        else
          {
             EINA_LIST_FOREACH(bd->transients, l, child)
               {
                /* Don't stack iconic transients. If the user wants these shown,
                 * thats another option.
                 */
                  if (!child->iconic)
                    {
                       child->layer = bd->layer;
                       e_border_lower (child);
                       last = child;
                    }
               }
          }
#endif
     }

   if (last)
     {
        e_container_border_stack_below(bd, last);
        stack = last;
        type = E_STACKING_BELOW;
     }
   else
     {
        E_Border *below;

        /* If we don't have any children, lower this border */
        below = e_container_border_lower(bd);
        if (below)
          {
             /* We ended up below a border */
              stack = below;
              type = E_STACKING_BELOW;
          }
        else
          {
             /* No border to hide under, same as a raise! */
              stack = NULL;
              type = E_STACKING_BELOW;
          }
     }

   ev = E_NEW(E_Event_Border_Stack, 1);
   if (ev)
     {
        ev->border = bd;
        ev->stack = stack;
        ev->type = type;
        e_object_ref(E_OBJECT(bd));
        if (stack)
          e_object_ref(E_OBJECT(stack));
        ecore_event_add(E_EVENT_BORDER_STACK, ev, _e_border_event_border_stack_free, NULL);
     }

   e_remember_update(bd);
   traceEnd(TTRACE_TAG_WINDOW_MANAGER);
}

EAPI void
e_border_stack_above(E_Border *bd,
                     E_Border *above)
{
   /* TODO: Should stack above allow the border to change level */
    E_Event_Border_Stack *ev;
    E_Border *last = NULL, *child, *stack;
    E_Stacking type;
    Eina_List *l;

    E_OBJECT_CHECK(bd);
    E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
    traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:BORDER:STACK ABOVE");

    ecore_x_window_shadow_tree_flush();

    if (e_config->transient.raise)
      {
         Eina_List *list = _e_border_sub_borders_new(bd);

         EINA_LIST_REVERSE_FOREACH(list, l, child)
           {
              /* Don't stack iconic transients. If the user wants these shown,
               * thats another option.
               */
              if (!child->iconic)
                {
                   if (last)
                     e_border_stack_below(child, last);
                   else
                     e_border_stack_above(child, above);
                   last = child;
                }
           }
         eina_list_free(list);
      }

    if (last)
      {
         e_container_border_stack_below(bd, last);
         stack = last;
         type = E_STACKING_BELOW;
      }
    else
      {
         e_container_border_stack_above(bd, above);
         stack = above;
         type = E_STACKING_ABOVE;
      }

    ev = E_NEW(E_Event_Border_Stack, 1);
    if (ev)
      {
         ev->border = bd;
         ev->stack = stack;
         ev->type = type;
         e_object_ref(E_OBJECT(bd));
         if (stack)
           e_object_ref(E_OBJECT(stack));
         ecore_event_add(E_EVENT_BORDER_STACK, ev, _e_border_event_border_stack_free, NULL);
      }
    e_remember_update(bd);
    traceEnd(TTRACE_TAG_WINDOW_MANAGER);
}

EAPI void
e_border_stack_below(E_Border *bd,
                     E_Border *below)
{
   /* TODO: Should stack below allow the border to change level */
    E_Event_Border_Stack *ev;
    E_Border *last = NULL, *child, *stack;
    E_Stacking type;
    Eina_List *l;

    E_OBJECT_CHECK(bd);
    E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
    traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:BORDER:STACK BELOW");

    ecore_x_window_shadow_tree_flush();

    if (e_config->transient.lower)
      {
         Eina_List *list = _e_border_sub_borders_new(bd);

         EINA_LIST_REVERSE_FOREACH(bd->transients, l, child)
           {
              /* Don't stack iconic transients. If the user wants these shown,
               * thats another option.
               */
              if (!child->iconic)
                {
                   if (last)
                     e_border_stack_below(child, last);
                   else
                     e_border_stack_below(child, below);
                   last = child;
                }
           }
         eina_list_free(list);
      }

    if (last)
      {
         e_container_border_stack_below(bd, last);
         stack = last;
         type = E_STACKING_BELOW;
      }
    else
      {
         e_container_border_stack_below(bd, below);
         stack = below;
         type = E_STACKING_BELOW;
      }

    ev = E_NEW(E_Event_Border_Stack, 1);
    if (ev)
      {
         ev->border = bd;
         ev->stack = stack;
         ev->type = type;
         e_object_ref(E_OBJECT(bd));
         if (stack)
           e_object_ref(E_OBJECT(stack));
         ecore_event_add(E_EVENT_BORDER_STACK, ev, _e_border_event_border_stack_free, NULL);
      }

    e_remember_update(bd);
    traceEnd(TTRACE_TAG_WINDOW_MANAGER);
}

EAPI void
e_border_focus_latest_set(E_Border *bd)
{
   focus_stack = eina_list_remove(focus_stack, bd);
   focus_stack = eina_list_prepend(focus_stack, bd);
}

EAPI void
e_border_raise_latest_set(E_Border *bd)
{
   raise_stack = eina_list_remove(raise_stack, bd);
   raise_stack = eina_list_prepend(raise_stack, bd);
}

/*
 * Sets the focus to the given border if necessary
 * There are 3 cases of different focus_policy-configurations:
 *
 * - E_FOCUS_CLICK: just set the focus, the most simple one
 *
 * - E_FOCUS_MOUSE: focus is where the mouse is, so try to
 *   warp the pointer to the window. If this fails (because
 *   the pointer is already in the window), just set the focus.
 *
 * - E_FOCUS_SLOPPY: focus is where the mouse is or on the
 *   last window which was focused, if the mouse is on the
 *   desktop. So, we need to look if there is another window
 *   under the pointer and warp to pointer to the right
 *   one if so (also, we set the focus afterwards). In case
 *   there is no window under pointer, the pointer is on the
 *   desktop and so we just set the focus.
 *
 *
 * This function is to be called when setting the focus was not
 * explicitly triggered by the user (by moving the mouse or
 * clicking for example), but implicitly (by closing a window,
 * the last focused window should get focus).
 *
 */
EAPI void
e_border_focus_set_with_pointer(E_Border *bd)
{
#ifdef PRINT_LOTS_OF_DEBUG
   E_PRINT_BORDER_INFO(bd);
#endif
   /* note: this is here as it seems there are enough apps that do not even
    * expect us to emulate a look of focus but not actually set x input
    * focus as we do - so simply abort any focuse set on such windows */
   /* be strict about accepting focus hint */
   if ((!bd->client.icccm.accepts_focus) &&
       (!bd->client.icccm.take_focus)) return;
   if (bd->lock_focus_out) return;

   e_border_focus_set(bd, 1, 1);

   if (e_config->focus_policy == E_FOCUS_CLICK) return;
   if (!bd->visible) return;

   if (e_config->focus_policy == E_FOCUS_SLOPPY)
     {
        if (!e_border_under_pointer_get(bd->desk, bd))
          {
             e_border_pointer_warp_to_center(bd);
          }
     }
   else
     {
        e_border_pointer_warp_to_center(bd);
     }
}

EAPI void
e_border_focus_set(E_Border *bd,
                   int       focus,
                   int       set)
{
   E_Border *bd_unfocus = NULL;
   Eina_Bool focus_changed = EINA_FALSE;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   /* note: this is here as it seems there are enough apps that do not even
    * expect us to emulate a look of focus but not actually set x input
    * focus as we do - so simply abort any focuse set on such windows */
   /* be strict about accepting focus hint */
   if ((!bd->client.icccm.accepts_focus) &&
       (!bd->client.icccm.take_focus))
     return;
   if ((set) && (focus) && (bd->lock_focus_out)) return;

   traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:BORDER:FOCUS SET");

   /* dont focus an iconified window. that's silly! */
   if (focus)
     {
        if (bd->iconic)
          {
#ifndef _F_USE_EXTENDED_ICONIFY_
             e_border_uniconify(bd);
             if (!focus_track_frozen)
               e_border_focus_latest_set(bd);
#endif
             traceEnd(TTRACE_TAG_WINDOW_MANAGER);
             return;
          }
        else if (!bd->visible)
          {
             traceEnd(TTRACE_TAG_WINDOW_MANAGER);
             return;
          }
        /* FIXME: hack for deskflip animation:
         * dont update focus when sliding previous desk */
        else if ((!bd->sticky) &&
                 (bd->desk != e_desk_current_get(bd->desk->zone)))
          {
             traceEnd(TTRACE_TAG_WINDOW_MANAGER);
             return;
          }
     }

   if ((bd->modal) && (bd->modal != bd) && (bd->modal->visible))
     {
        e_border_focus_set(bd->modal, focus, set);
        traceEnd(TTRACE_TAG_WINDOW_MANAGER);
        return;
     }
   else if ((bd->leader) && (bd->leader->modal) && (bd->leader->modal != bd))
     {
        e_border_focus_set(bd->leader->modal, focus, set);
        traceEnd(TTRACE_TAG_WINDOW_MANAGER);
        return;
     }

   if (focus)
     {
        if (set)
          {
             if (bd->visible && bd->changes.visible)
               {
                  bd->want_focus = 1;
                  bd->changed = 1;
               }
             else if ((!bd->focused) ||
                      (focus_next && (bd != eina_list_data_get(focus_next))))
               {
                  Eina_List *l;

                  if ((l = eina_list_data_find_list(focus_next, bd)))
                    focus_next = eina_list_promote_list(focus_next, l);
                  else
                    focus_next = eina_list_prepend(focus_next, bd);
               }
             if ((bd->client.icccm.take_focus) &&
                 (bd->client.icccm.accepts_focus))
               {
                  e_grabinput_focus(bd->client.win, E_FOCUS_METHOD_LOCALLY_ACTIVE);
                  /* TODO what if the client didn't take focus ? */
               }
             else if (!bd->client.icccm.accepts_focus)
               {
                  e_grabinput_focus(bd->client.win, E_FOCUS_METHOD_GLOBALLY_ACTIVE);
               }
             else if (!bd->client.icccm.take_focus)
               {
                  e_grabinput_focus(bd->client.win, E_FOCUS_METHOD_PASSIVE);
                  /* e_border_focus_set(bd, 1, 0); */
               }
            traceEnd(TTRACE_TAG_WINDOW_MANAGER);
            return;
          }

        if (!bd->focused)
          {
             Eina_List *l;
             E_Border *bd2;

             if (focused) bd_unfocus = focused;
             if (focusing == bd) focusing = NULL;
             bd->focused = 1;
             focused = bd;
             if (!bd_unfocus)
               {
                  EINA_LIST_FOREACH(e_border_client_list(), l, bd2)
                    {
                       if ((bd2->fullscreen) &&
                           (bd2 != bd) &&
                           (bd2->zone == bd->zone) &&
                           ((bd2->desk == bd->desk) ||
                               (bd2->sticky) || (bd->sticky)))
                         {
                            Eina_Bool unfocus_is_parent = EINA_FALSE;
                            E_Border *bd_parent;

                            bd_parent = bd->parent;
                            while (bd_parent)
                              {
                                 if (bd_parent == bd2)
                                   {
                                      unfocus_is_parent = EINA_TRUE;
                                      break;
                                   }
                                 bd_parent = bd_parent->parent;
                              }
                            if ((!unfocus_is_parent) &&
                                (!e_config->allow_above_fullscreen))
                              {
                                 e_border_iconify(bd2);
                              }
                         }
                    }
               }
             focus_changed = EINA_TRUE;
          }
     }
   else
     {
        bd->want_focus = 0;
        focus_next = eina_list_remove(focus_next, bd);
        if (bd == focusing) focusing = NULL;

        if ((bd->focused) &&
            ((bd->desk == e_desk_current_get(bd->zone)) || (bd->sticky)))
          {
             Eina_Bool wasfocused = EINA_FALSE;
             bd_unfocus = bd;

             /* should always be the case. anyway */
             if (bd == focused)
               {
                  focused = NULL;
                  wasfocused = EINA_TRUE;
               }

             if ((set) && (!focus_next) && (!focusing))
               {
                  e_grabinput_focus(bd->zone->container->bg_win,
                                    E_FOCUS_METHOD_PASSIVE);
               }
             if ((bd->fullscreen) && (wasfocused))
               {
                  Eina_Bool have_vis_child = EINA_FALSE;
                  Eina_List *l;
                  E_Border *bd2;

                  EINA_LIST_FOREACH(e_border_client_list(), l, bd2)
                    {
                       if ((bd2 != bd) &&
                           (bd2->zone == bd->zone) &&
                           ((bd2->desk == bd->desk) ||
                               (bd2->sticky) || (bd->sticky)))
                         {
                            if (bd2->parent == bd)
                              {
                                 have_vis_child = EINA_TRUE;
                                 break;
                              }
                         }
                    }
                  if ((!have_vis_child) &&
                      (!e_config->allow_above_fullscreen))
                    e_border_iconify(bd);
               }
          }
     }

   if ((bd_unfocus) &&
       (!e_object_is_del(E_OBJECT(bd_unfocus)) &&
        (e_object_ref_get(E_OBJECT(bd_unfocus)) > 0)))
     {
        E_Event_Border_Focus_Out *ev;

        bd_unfocus->focused = 0;
        e_focus_event_focus_out(bd_unfocus);

        if (bd_unfocus->raise_timer)
          ecore_timer_del(bd_unfocus->raise_timer);
        bd_unfocus->raise_timer = NULL;

        edje_object_signal_emit(bd_unfocus->bg_object, "e,state,unfocused", "e");
        if (bd_unfocus->icon_object)
          edje_object_signal_emit(bd_unfocus->icon_object, "e,state,unfocused", "e");

        ev = E_NEW(E_Event_Border_Focus_Out, 1);
        if (ev)
          {
             ev->border = bd_unfocus;
             e_object_ref(E_OBJECT(bd_unfocus));

             ecore_event_add(E_EVENT_BORDER_FOCUS_OUT, ev,
                             _e_border_event_border_focus_out_free, NULL);
          }

        if ((bd_unfocus->fullscreen) &&
            (bd != bd_unfocus) &&
            (bd->zone == bd_unfocus->zone) &&
            ((bd->desk == bd_unfocus->desk) ||
             (bd->sticky) || (bd_unfocus->sticky)))
          {
             Eina_Bool unfocus_is_parent = EINA_FALSE;
             E_Border *bd_parent;

             bd_parent = bd->parent;
             while (bd_parent)
               {
                  if (bd_parent == bd_unfocus)
                    {
                       unfocus_is_parent = EINA_TRUE;
                       break;
                    }
                  bd_parent = bd_parent->parent;
               }
             if ((!unfocus_is_parent) && (!e_config->allow_above_fullscreen))
               {
                  e_border_iconify(bd_unfocus);
               }
          }
     }

   if (focus_changed)
     {
        E_Event_Border_Focus_In *ev;

        e_focus_event_focus_in(bd);

        if (!focus_track_frozen)
          e_border_focus_latest_set(bd);

        e_hints_active_window_set(bd->zone->container->manager, bd);

        edje_object_signal_emit(bd->bg_object, "e,state,focused", "e");
        if (bd->icon_object)
          edje_object_signal_emit(bd->icon_object, "e,state,focused", "e");

        ev = E_NEW(E_Event_Border_Focus_In, 1);
        if (ev)
          {
             ev->border = bd;
             e_object_ref(E_OBJECT(bd));

             ecore_event_add(E_EVENT_BORDER_FOCUS_IN, ev,
                             _e_border_event_border_focus_in_free, NULL);
          }
     }
   traceEnd(TTRACE_TAG_WINDOW_MANAGER);
}

EAPI void
e_border_shade(E_Border   *bd,
               E_Direction dir)
{
   E_Event_Border_Resize *ev;
   Eina_List *l;
   E_Border *tmp;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if ((bd->shaded) || (bd->shading) || (bd->fullscreen) ||
       ((bd->maximized) && (!e_config->allow_manip))) return;
   if ((bd->client.border.name) &&
       (!strcmp("borderless", bd->client.border.name))) return;

   EINA_LIST_FOREACH(bd->client.e.state.video_child, l, tmp)
     ecore_x_window_hide(tmp->win);

   ecore_x_window_shadow_tree_flush();

   bd->shade.x = bd->x;
   bd->shade.y = bd->y;
   bd->shade.dir = dir;

   e_hints_window_shaded_set(bd, 1);
   e_hints_window_shade_direction_set(bd, dir);

   if (e_config->border_shade_animate)
     {
        bd->shade.start = ecore_loop_time_get();
        bd->shading = 1;
        bd->changes.shading = 1;
        bd->changed = 1;

        if (bd->shade.dir == E_DIRECTION_UP ||
            bd->shade.dir == E_DIRECTION_LEFT)
          ecore_x_window_gravity_set(bd->client.win, ECORE_X_GRAVITY_SW);
        else
          ecore_x_window_gravity_set(bd->client.win, ECORE_X_GRAVITY_NE);

        bd->shade.anim = ecore_animator_add(_e_border_shade_animator, bd);
        edje_object_signal_emit(bd->bg_object, "e,state,shading", "e");
     }
   else
     {
        if (bd->shade.dir == E_DIRECTION_UP)
          {
             bd->h = bd->client_inset.t + bd->client_inset.b;
          }
        else if (bd->shade.dir == E_DIRECTION_DOWN)
          {
             bd->h = bd->client_inset.t + bd->client_inset.b;
             bd->y = bd->y + bd->client.h;
             bd->changes.pos = 1;
          }
        else if (bd->shade.dir == E_DIRECTION_LEFT)
          {
             bd->w = bd->client_inset.l + bd->client_inset.r;
          }
        else if (bd->shade.dir == E_DIRECTION_RIGHT)
          {
             bd->w = bd->client_inset.l + bd->client_inset.r;
             bd->x = bd->x + bd->client.w;
             bd->changes.pos = 1;
          }

        if ((bd->shaped) || (bd->client.shaped))
          {
             bd->need_shape_merge = 1;
             bd->need_shape_export = 1;
          }
        if (bd->shaped_input)
          {
             bd->need_shape_merge = 1;
          }

        bd->changes.size = 1;
        bd->shaded = 1;
        bd->changes.shaded = 1;
        bd->changed = 1;
        edje_object_signal_emit(bd->bg_object, "e,state,shaded", "e");
        e_border_frame_recalc(bd);
        ev = E_NEW(E_Event_Border_Resize, 1);
        if (ev)
          {
             ev->border = bd;
             /* The resize is added in the animator when animation complete */
             /* For non-animated, we add it immediately with the new size */
             e_object_ref(E_OBJECT(bd));
             //	     e_object_breadcrumb_add(E_OBJECT(bd), "border_resize_event");
             ecore_event_add(E_EVENT_BORDER_RESIZE, ev, _e_border_event_border_resize_free, NULL);
          }
     }

   e_remember_update(bd);
}

EAPI void
e_border_unshade(E_Border   *bd,
                 E_Direction dir)
{
   E_Event_Border_Resize *ev;
   Eina_List *l;
   E_Border *tmp;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if ((!bd->shaded) || (bd->shading))
     return;

   EINA_LIST_FOREACH(bd->client.e.state.video_child, l, tmp)
     ecore_x_window_show(tmp->win);

   ecore_x_window_shadow_tree_flush();

   bd->shade.dir = dir;

   e_hints_window_shaded_set(bd, 0);
   e_hints_window_shade_direction_set(bd, dir);

   if (bd->shade.dir == E_DIRECTION_UP ||
       bd->shade.dir == E_DIRECTION_LEFT)
     {
        bd->shade.x = bd->x;
        bd->shade.y = bd->y;
     }
   else
     {
        bd->shade.x = bd->x - bd->client.w;
        bd->shade.y = bd->y - bd->client.h;
     }
   if (e_config->border_shade_animate)
     {
        bd->shade.start = ecore_loop_time_get();
        bd->shading = 1;
        bd->changes.shading = 1;
        bd->changed = 1;

        if (bd->shade.dir == E_DIRECTION_UP)
          {
             ecore_x_window_gravity_set(bd->client.win, ECORE_X_GRAVITY_SW);
             ecore_x_window_move_resize(bd->client.win, 0,
                                        bd->h - (bd->client_inset.t + bd->client_inset.b) -
                                        bd->client.h,
                                        bd->client.w, bd->client.h);
          }
        else if (bd->shade.dir == E_DIRECTION_LEFT)
          {
             ecore_x_window_gravity_set(bd->client.win, ECORE_X_GRAVITY_SW);
             ecore_x_window_move_resize(bd->client.win,
                                        bd->w - (bd->client_inset.l + bd->client_inset.r) -
                                        bd->client.h,
                                        0, bd->client.w, bd->client.h);
          }
        else
          ecore_x_window_gravity_set(bd->client.win, ECORE_X_GRAVITY_NE);

        bd->shade.anim = ecore_animator_add(_e_border_shade_animator, bd);
        edje_object_signal_emit(bd->bg_object, "e,state,unshading", "e");
     }
   else
     {
        if (bd->shade.dir == E_DIRECTION_UP)
          {
             bd->h = bd->client_inset.t + bd->client.h + bd->client_inset.b;
          }
        else if (bd->shade.dir == E_DIRECTION_DOWN)
          {
             bd->h = bd->client_inset.t + bd->client.h + bd->client_inset.b;
             bd->y = bd->y - bd->client.h;
             bd->changes.pos = 1;
          }
        else if (bd->shade.dir == E_DIRECTION_LEFT)
          {
             bd->w = bd->client_inset.l + bd->client.w + bd->client_inset.r;
          }
        else if (bd->shade.dir == E_DIRECTION_RIGHT)
          {
             bd->w = bd->client_inset.l + bd->client.w + bd->client_inset.r;
             bd->x = bd->x - bd->client.w;
             bd->changes.pos = 1;
          }
        if ((bd->shaped) || (bd->client.shaped))
          {
             bd->need_shape_merge = 1;
             bd->need_shape_export = 1;
          }
        if (bd->shaped_input)
          {
             bd->need_shape_merge = 1;
          }

        bd->changes.size = 1;
        bd->shaded = 0;
        bd->changes.shaded = 1;
        bd->changed = 1;
        edje_object_signal_emit(bd->bg_object, "e,state,unshaded", "e");
        e_border_frame_recalc(bd);
        ev = E_NEW(E_Event_Border_Resize, 1);
        if (ev)
          {
             ev->border = bd;
             /* The resize is added in the animator when animation complete */
             /* For non-animated, we add it immediately with the new size */
             e_object_ref(E_OBJECT(bd));
             //	     e_object_breadcrumb_add(E_OBJECT(bd), "border_resize_event");
             ecore_event_add(E_EVENT_BORDER_RESIZE, ev, _e_border_event_border_resize_free, NULL);
          }
     }

   e_remember_update(bd);
}

static void
_e_border_client_inset_calc(E_Border *bd)
{
   /* int w, h; */
   Evas_Coord cx, cy, cw, ch;

   if (bd->bg_object)
     {
        evas_object_resize(bd->bg_object, 1000, 1000);
        edje_object_message_signal_process(bd->bg_object);
        edje_object_calc_force(bd->bg_object);
        edje_object_part_geometry_get(bd->bg_object, "e.swallow.client", &cx, &cy, &cw, &ch);
        bd->client_inset.l = cx;
        bd->client_inset.r = 1000 - (cx + cw);
        bd->client_inset.t = cy;
        bd->client_inset.b = 1000 - (cy + ch);
     }
   else
     {
        bd->client_inset.l = 0;
        bd->client_inset.r = 0;
        bd->client_inset.t = 0;
        bd->client_inset.b = 0;
     }

   ecore_x_netwm_frame_size_set(bd->client.win,
                                bd->client_inset.l, bd->client_inset.r,
                                bd->client_inset.t, bd->client_inset.b);
   ecore_x_e_frame_size_set(bd->client.win,
                            bd->client_inset.l, bd->client_inset.r,
                            bd->client_inset.t, bd->client_inset.b);
}

static void
_e_border_maximize(E_Border *bd, E_Maximize max)
{
   int x1, yy1, x2, y2;
   int w, h, pw, ph;
   int zx, zy, zw, zh;
#ifdef _F_USE_BOTTOM_TOP_MAXIMIZE
   int cy = 0;
#endif

   zx = zy = zw = zh = 0;

   switch (max & E_MAXIMIZE_TYPE)
     {
      case E_MAXIMIZE_NONE:
         /* Ignore */
         break;

      case E_MAXIMIZE_FULLSCREEN:
         w = bd->zone->w;
         h = bd->zone->h;

         if (bd->bg_object)
           {
              edje_object_signal_emit(bd->bg_object, "e,action,maximize,fullscreen", "e");
              _e_border_client_inset_calc(bd);
           }
         e_border_resize_limit(bd, &w, &h);
         /* center x-direction */
         x1 = bd->zone->x + (bd->zone->w - w) / 2;
         /* center y-direction */
         yy1 = bd->zone->y + (bd->zone->h - h) / 2;

#ifdef _F_USE_BOTTOM_TOP_MAXIMIZE
         cy = bd->zone->y + (bd->zone->h / 2);
#endif

         switch (max & E_MAXIMIZE_DIRECTION)
           {
            case E_MAXIMIZE_BOTH:
              e_border_move_resize(bd, x1, yy1, w, h);
              break;

            case E_MAXIMIZE_VERTICAL:
              e_border_move_resize(bd, bd->x, yy1, bd->w, h);
              break;

            case E_MAXIMIZE_HORIZONTAL:
              e_border_move_resize(bd, x1, bd->y, w, bd->h);
              break;

            case E_MAXIMIZE_LEFT:
              e_border_move_resize(bd, bd->zone->x, bd->zone->y, w / 2, h);
              break;

            case E_MAXIMIZE_RIGHT:
              e_border_move_resize(bd, x1, bd->zone->y, w / 2, h);
              break;
#ifdef _F_USE_BOTTOM_TOP_MAXIMIZE
            case E_MAXIMIZE_TOP:
              e_border_move_resize(bd, bd->zone->x, bd->zone->y, w, h / 2);
              break;
            case E_MAXIMIZE_BOTTOM:
              e_border_move_resize(bd, bd->zone->x, cy, w, h / 2);
              break;
#endif
           }
         break;

      case E_MAXIMIZE_SMART:
      case E_MAXIMIZE_EXPAND:
         if (bd->zone)
           e_zone_useful_geometry_get(bd->zone, &zx, &zy, &zw, &zh);

         if (bd->w < zw)
           w = bd->w;
         else
           w = zw;

         if (bd->h < zh)
           h = bd->h;
         else
           h = zh;

         if (bd->x < zx) // window left not useful coordinates
           x1 = zx;
         else if (bd->x + bd->w > zx + zw) // window right not useful coordinates
           x1 = zx + zw - bd->w;
         else // window normal position
           x1 = bd->x;

         if (bd->y < zy) // window top not useful coordinates
           yy1 = zy;
         else if (bd->y + bd->h > zy + zh) // window bottom not useful coordinates
           yy1 = zy + zh - bd->h;
         else // window normal position
           yy1 = bd->y;

         switch (max & E_MAXIMIZE_DIRECTION)
           {
            case E_MAXIMIZE_BOTH:
              e_border_move_resize(bd, zx, zy, zw, zh);
              break;

            case E_MAXIMIZE_VERTICAL:
              e_border_move_resize(bd, x1, zy, w, zh);
              break;

            case E_MAXIMIZE_HORIZONTAL:
              e_border_move_resize(bd, zx, yy1, zw, h);
              break;

            case E_MAXIMIZE_LEFT:
              e_border_move_resize(bd, zx, zy, zw / 2, zh);
              break;

            case E_MAXIMIZE_RIGHT:
              e_border_move_resize(bd, zx + zw / 2, zy, zw / 2, zh);
              break;
           }

         edje_object_signal_emit(bd->bg_object, "e,action,maximize", "e");
         break;

      case E_MAXIMIZE_FILL:
         x1 = bd->zone->x;
         yy1 = bd->zone->y;
         x2 = bd->zone->x + bd->zone->w;
         y2 = bd->zone->y + bd->zone->h;

         /* walk through all shelves */
#ifndef _F_DISABLE_E_SHELF
         e_maximize_border_shelf_fill(bd, &x1, &yy1, &x2, &y2, max);
#endif
         /* walk through all windows */
         e_maximize_border_border_fill(bd, &x1, &yy1, &x2, &y2, max);

         w = x2 - x1;
         h = y2 - yy1;
         pw = w;
         ph = h;
         e_border_resize_limit(bd, &w, &h);
         /* center x-direction */
         x1 = x1 + (pw - w) / 2;
         /* center y-direction */
         yy1 = yy1 + (ph - h) / 2;

         switch (max & E_MAXIMIZE_DIRECTION)
           {
            case E_MAXIMIZE_BOTH:
              e_border_move_resize(bd, x1, yy1, w, h);
              break;

            case E_MAXIMIZE_VERTICAL:
              e_border_move_resize(bd, bd->x, yy1, bd->w, h);
              break;

            case E_MAXIMIZE_HORIZONTAL:
              e_border_move_resize(bd, x1, bd->y, w, bd->h);
              break;

            case E_MAXIMIZE_LEFT:
              e_border_move_resize(bd, bd->zone->x, bd->zone->y, w / 2, h);
              break;

            case E_MAXIMIZE_RIGHT:
              e_border_move_resize(bd, x1, bd->zone->y, w / 2, h);
              break;
           }
         break;
     }
}

EAPI void
e_border_maximize(E_Border  *bd,
                  E_Maximize max)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (!(max & E_MAXIMIZE_DIRECTION)) max |= E_MAXIMIZE_BOTH;

   if ((bd->shaded) || (bd->shading)) return;
   ecore_x_window_shadow_tree_flush();
   if (bd->fullscreen)
     e_border_unfullscreen(bd);
   /* Only allow changes in vertical/ horizontal maximization */
   if (((bd->maximized & E_MAXIMIZE_DIRECTION) == (max & E_MAXIMIZE_DIRECTION)) ||
       ((bd->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_BOTH)) return;
   if (bd->new_client)
     {
        bd->need_maximize = 1;
        bd->maximized &= ~E_MAXIMIZE_TYPE;
        bd->maximized |= max;
        return;
     }

   bd->pre_res_change.valid = 0;
   if (!(bd->maximized & E_MAXIMIZE_HORIZONTAL))
     {
        /* Horizontal hasn't been set */
        bd->saved.x = bd->x - bd->zone->x;
        bd->saved.w = bd->w;
     }
   if (!(bd->maximized & E_MAXIMIZE_VERTICAL))
     {
        /* Vertical hasn't been set */
        bd->saved.y = bd->y - bd->zone->y;
        bd->saved.h = bd->h;
     }

   bd->saved.zone = bd->zone->num;
   e_hints_window_size_set(bd);

   e_border_raise(bd);

   _e_border_maximize(bd, max);


   /* Remove previous type */
   bd->maximized &= ~E_MAXIMIZE_TYPE;
   /* Add new maximization. It must be added, so that VERTICAL + HORIZONTAL == BOTH */
   bd->maximized |= max;

   e_hints_window_maximized_set(bd, bd->maximized & E_MAXIMIZE_HORIZONTAL,
                                bd->maximized & E_MAXIMIZE_VERTICAL);
   e_remember_update(bd);
}

EAPI void
e_border_unmaximize(E_Border  *bd,
                    E_Maximize max)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (!(max & E_MAXIMIZE_DIRECTION))
     {
        CRI("BUG: Unmaximize call without direction!");
        return;
     }

   if ((bd->shaded) || (bd->shading)) return;
   ecore_x_window_shadow_tree_flush();
   /* Remove directions not used */
   max &= (bd->maximized & E_MAXIMIZE_DIRECTION);
   /* Can only remove existing maximization directions */
   if (!max) return;
   if (bd->maximized & E_MAXIMIZE_TYPE)
     {
        bd->pre_res_change.valid = 0;
        bd->need_maximize = 0;

        if ((bd->maximized & E_MAXIMIZE_TYPE) == E_MAXIMIZE_FULLSCREEN)
          {
             if (bd->bg_object)
               {
                  edje_object_signal_emit(bd->bg_object, "e,action,unmaximize,fullscreen", "e");
                  _e_border_client_inset_calc(bd);
               }

             bd->maximized = E_MAXIMIZE_NONE;
             _e_border_move_resize_internal(bd,
                                            bd->zone->x + bd->saved.x,
                                            bd->zone->y + bd->saved.y,
                                            bd->saved.w, bd->saved.h, 0, 1);
             bd->saved.x = bd->saved.y = bd->saved.w = bd->saved.h = 0;
             e_hints_window_size_unset(bd);
          }
        else
          {
             int w, h, x, y;

             w = bd->w;
             h = bd->h;
             x = bd->x;
             y = bd->y;

             if (max & E_MAXIMIZE_VERTICAL)
               {
                  /* Remove vertical */
                  h = bd->saved.h;
                  y = bd->saved.y + bd->zone->y;
                  bd->saved.h = bd->saved.y = 0;
                  bd->maximized &= ~E_MAXIMIZE_VERTICAL;
                  bd->maximized &= ~E_MAXIMIZE_LEFT;
                  bd->maximized &= ~E_MAXIMIZE_RIGHT;
               }
             if (max & E_MAXIMIZE_HORIZONTAL)
               {
                  /* Remove horizontal */
                  w = bd->saved.w;
                  x = bd->saved.x + bd->zone->x;
                  bd->saved.w = bd->saved.x = 0;
                  bd->maximized &= ~E_MAXIMIZE_HORIZONTAL;
               }

             e_border_resize_limit(bd, &w, &h);

             if (!(bd->maximized & E_MAXIMIZE_DIRECTION))
               {
                  bd->maximized = E_MAXIMIZE_NONE;
                  _e_border_move_resize_internal(bd, x, y, w, h, 0, 1);
                  e_hints_window_size_unset(bd);
                  edje_object_signal_emit(bd->bg_object, "e,action,unmaximize", "e");
               }
             else
               {
                  _e_border_move_resize_internal(bd, x, y, w, h, 0, 1);
                  e_hints_window_size_set(bd);
               }
          }
        e_hints_window_maximized_set(bd, bd->maximized & E_MAXIMIZE_HORIZONTAL,
                                     bd->maximized & E_MAXIMIZE_VERTICAL);
     }
   e_remember_update(bd);
}

EAPI void
e_border_fullscreen(E_Border    *bd,
                    E_Fullscreen policy)
{
   E_Event_Border_Fullscreen *ev;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if ((bd->shaded) || (bd->shading)) return;
   ecore_x_window_shadow_tree_flush();
   if (bd->new_client)
     {
        bd->need_fullscreen = 1;
        return;
     }
   if (!bd->fullscreen)
     {
        bd->pre_res_change.valid = 0;

        bd->saved.x = bd->x - bd->zone->x;
        bd->saved.y = bd->y - bd->zone->y;
        bd->saved.w = bd->client.w;
        bd->saved.h = bd->client.h;
        bd->saved.maximized = bd->maximized;
        bd->saved.zone = bd->zone->num;

        if (bd->maximized)
          e_border_unmaximize(bd, E_MAXIMIZE_BOTH);
        e_hints_window_size_set(bd);

        bd->client_inset.l = 0;
        bd->client_inset.r = 0;
        bd->client_inset.t = 0;
        bd->client_inset.b = 0;

        bd->desk->fullscreen_borders++;

        /* e_zone_fullscreen_set(bd->zone, 1); */
        bd->saved.layer = bd->layer;
        if (!e_config->allow_above_fullscreen)
          e_border_layer_set(bd, 250);

        if ((eina_list_count(bd->zone->container->zones) > 1) ||
            (policy == E_FULLSCREEN_RESIZE) || (!ecore_x_randr_query()))
          {
             e_border_move_resize(bd, bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h);
          }
        else if (policy == E_FULLSCREEN_ZOOM)
          {
             Ecore_X_Randr_Screen_Size_MM *sizes;
             int num_sizes, i, best_size_index = 0;

             ecore_x_randr_screen_primary_output_current_size_get(bd->zone->container->manager->root,
                                                                  &screen_size.width,
                                                                  &screen_size.height,
                                                                  NULL, NULL, NULL);
             sizes = ecore_x_randr_screen_primary_output_sizes_get(bd->zone->container->manager->root,
                                                                   &num_sizes);
             if (sizes)
               {
                  Ecore_X_Randr_Screen_Size best_size = { -1, -1 };
                  int best_dist = INT_MAX, dist;

                  for (i = 0; i < num_sizes; i++)
                    {
                       if ((sizes[i].width > bd->w) && (sizes[i].height > bd->h))
                         {
                            dist = (sizes[i].width * sizes[i].height) - (bd->w * bd->h);
                            if (dist < best_dist)
                              {
                                 best_size.width = sizes[i].width;
                                 best_size.height = sizes[i].height;
                                 best_dist = dist;
                                 best_size_index = i;
                              }
                         }
                    }
                  if (((best_size.width != -1) && (best_size.height != -1)) &&
                      ((best_size.width != screen_size.width) ||
                       (best_size.height != screen_size.height)))
                    {
                       if (ecore_x_randr_screen_primary_output_size_set(bd->zone->container->manager->root,
                                                                        best_size_index))
                         screen_size_index = best_size_index;
                       e_border_move_resize(bd, 0, 0, best_size.width, best_size.height);
                    }
                  else
                    {
                       screen_size.width = -1;
                       screen_size.height = -1;
                       e_border_move_resize(bd, 0, 0, bd->zone->w, bd->zone->h);
                    }
                  free(sizes);
               }
             else
               e_border_move_resize(bd, bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h);
          }
        bd->fullscreen = 1;

        e_hints_window_fullscreen_set(bd, 1);
        e_hints_window_size_unset(bd);
        bd->client.border.changed = 1;
        bd->changed = 1;
     }
   bd->fullscreen_policy = policy;

   ev = E_NEW(E_Event_Border_Fullscreen, 1);
   if (ev)
     {
        ev->border = bd;
        e_object_ref(E_OBJECT(bd));
        //   e_object_breadcrumb_add(E_OBJECT(bd), "border_fullscreen_event");
        ecore_event_add(E_EVENT_BORDER_FULLSCREEN, ev, _e_border_event_border_fullscreen_free, NULL);
     }

   e_remember_update(bd);
}

EAPI void
e_border_unfullscreen(E_Border *bd)
{
   E_Event_Border_Unfullscreen *ev;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if ((bd->shaded) || (bd->shading)) return;
   ecore_x_window_shadow_tree_flush();
   if (bd->fullscreen)
     {
        bd->pre_res_change.valid = 0;
        bd->fullscreen = 0;
        bd->need_fullscreen = 0;
        bd->desk->fullscreen_borders--;

        if ((screen_size.width != -1) && (screen_size.height != -1))
          {
             ecore_x_randr_screen_primary_output_size_set(bd->zone->container->manager->root,
                                                          screen_size_index);
             screen_size.width = -1;
             screen_size.height = -1;
          }
        e_border_move_resize(bd,
                             bd->saved.x + bd->zone->x,
                             bd->saved.y + bd->zone->y,
                             bd->saved.w, bd->saved.h);

        if (bd->saved.maximized)
          e_border_maximize(bd, (e_config->maximize_policy & E_MAXIMIZE_TYPE) |
                            bd->saved.maximized);

        e_border_layer_set(bd, bd->saved.layer);

        e_hints_window_fullscreen_set(bd, 0);
        bd->client.border.changed = 1;
        bd->changed = 1;
     }
   bd->fullscreen_policy = 0;

   ev = E_NEW(E_Event_Border_Unfullscreen, 1);
   if (ev)
     {
        ev->border = bd;
        e_object_ref(E_OBJECT(bd));
        //   e_object_breadcrumb_add(E_OBJECT(bd), "border_unfullscreen_event");
        ecore_event_add(E_EVENT_BORDER_UNFULLSCREEN, ev, _e_border_event_border_unfullscreen_free, NULL);
     }

   e_remember_update(bd);
}

EAPI void
e_border_iconify(E_Border *bd)
{
   E_Event_Border_Iconify *ev;
   unsigned int iconic;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:BORDER:ICONIFY");
   if (bd->shading) return;
   ecore_x_window_shadow_tree_flush();
#ifdef _F_BORDER_HOOK_PATCH_
   _e_border_hook_call(E_BORDER_HOOK_ICONIFY_BORDER, bd);
#endif
#ifdef _F_DEICONIFY_APPROVE_
   if (e_config->deiconify_approve)
     {
#ifdef _F_USE_ICONIFY_RESIZE_
        if (e_config->use_iconify_resize)
          {
             if (bd->iconify_resize.support)
               {
                  if (!bd->iconify_resize.set)
                    {
                       bd->iconify_resize.x = bd->x;
                       bd->iconify_resize.y = bd->y;
                       bd->iconify_resize.w = bd->w;
                       bd->iconify_resize.h = bd->h;
                       e_border_move_resize(bd, bd->x, bd->y, e_config->iconify_resize_w, e_config->iconify_resize_h);

                       bd->iconify_resize.set = 1;
                       bd->iconify_resize.need_draw = 0;
                       ELBF(ELBT_BD, 0, bd->client.win,"Iconify Resize: Geo (%d, %d, %d, %d)", bd->x, bd->y, bd->w, bd->h);
                    }
               }
#endif
          }
     }
#endif

   if (!bd->iconic)
     {
        bd->iconic = 1;
        e_border_hide(bd, 1);
        if (bd->fullscreen) bd->desk->fullscreen_borders--;
        edje_object_signal_emit(bd->bg_object, "e,action,iconify", "e");
     }
   iconic = 1;
   e_hints_window_iconic_set(bd);
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MAPPED, &iconic, 1);

   ev = E_NEW(E_Event_Border_Iconify, 1);
   if (ev)
     {
        ev->border = bd;
        e_object_ref(E_OBJECT(bd));
        //   e_object_breadcrumb_add(E_OBJECT(bd), "border_iconify_event");
        ecore_event_add(E_EVENT_BORDER_ICONIFY, ev, _e_border_event_border_iconify_free, NULL);
     }

   if (e_config->transient.iconify)
     {
        Eina_List *l;
        E_Border *child;
        Eina_List *list = _e_border_sub_borders_new(bd);

        EINA_LIST_FOREACH(list, l, child)
          {
             e_border_iconify(child);
          }
        eina_list_free(list);
     }
   e_remember_update(bd);
#ifdef _F_USE_VIRT_RESOLUTION_
   _e_mod_revert_resolution( bd->client.win);
#endif /* end of _F_USE_VIRT_RESOLUTION_ */
   traceEnd(TTRACE_TAG_WINDOW_MANAGER);
}

#ifdef _F_DEICONIFY_APPROVE_
static Eina_Bool
_e_border_uniconify_timeout(void *data)
{
   E_Border *bd;
   E_Border *child_bd;
   Eina_Bool retry = EINA_FALSE;

   bd = data;

   if (!e_object_is_del(E_OBJECT(bd)))
     {
        ELB(ELBT_BD, "TIMEOUT DEICONIFY_APPROVE", bd->client.win);
        bd->client.e.state.deiconify_approve.render_done = 1;

        if (bd->client.e.state.deiconify_approve.pending_list)
          {
             _e_border_window_pending_event_flush(bd);
          }

        E_Border *ancestor_bd;
        ancestor_bd = bd->client.e.state.deiconify_approve.ancestor;
        if ((ancestor_bd) &&
            (!e_object_is_del(E_OBJECT(ancestor_bd))))
          {
             ancestor_bd->client.e.state.deiconify_approve.req_list = eina_list_remove(ancestor_bd->client.e.state.deiconify_approve.req_list, bd);
             bd->client.e.state.deiconify_approve.ancestor = NULL;
          }
        else
          {
             ancestor_bd = bd;
          }

        ELBF(ELBT_BD, 0, bd->client.win,
             "TIMEOUT DEICONIFY_APPROVE.. ancestor:%x bd:%x", ancestor_bd->client.win, bd->client.win);

        if ((ancestor_bd->client.e.state.deiconify_approve.req_list == NULL) &&
            (ancestor_bd->client.e.state.deiconify_approve.render_done))
          {
             if (ancestor_bd->client.e.state.deiconify_approve.wait_timer)
               {
                  ecore_timer_del(ancestor_bd->client.e.state.deiconify_approve.wait_timer);
                  ancestor_bd->client.e.state.deiconify_approve.wait_timer = NULL;

                  if (bd->client.e.state.deiconify_approve.wait_timer)
                    {
                       ecore_timer_del(bd->client.e.state.deiconify_approve.wait_timer);
                       bd->client.e.state.deiconify_approve.wait_timer = NULL;
                    }

                  if (bd->client.e.state.deiconify_approve.render_only)
                    {
                       bd->client.e.state.deiconify_approve.render_only = 0;
                       e_border_force_render_request(bd,EINA_FALSE);
                       return ECORE_CALLBACK_CANCEL;
                    }
                  _e_border_uniconify_pending_list_remove(ancestor_bd);
                  _e_border_window_pending_event_flush(ancestor_bd);
               }
             else
               {
                  ELB(ELBT_BD, "Unset DEICONIFY_APPROVE render_done", ancestor_bd->client.win);
                  ancestor_bd->client.e.state.deiconify_approve.render_done = 0;
               }
          }
        else
          {
             if (bd == ancestor_bd)
               {
                  ELB(ELBT_BD, "Children are not handled. So re-run timer.", bd->client.win);
                  retry = EINA_TRUE;
               }
          }

        if (bd != ancestor_bd)
          {
             _e_border_uniconify_pending_list_remove(bd);
          }

        if (retry) return ECORE_CALLBACK_RENEW;

        bd->client.e.state.deiconify_approve.wait_timer = NULL;
     }

   return ECORE_CALLBACK_CANCEL;
}

static void
_e_border_deiconify_approve_send_pending_end(E_Border *bd)
{
   E_Border *bd_ancestor;

   if (e_config->deiconify_approve)
     {
        if (!e_object_is_del(E_OBJECT(bd)))
          {
             bd->client.e.state.deiconify_approve.pending = 0;

             ELBF(ELBT_BD, 0, bd->client.win,
                  "NEED to send DEICONIFY_APPROVE. (PENDING_END) ancestor:%x",
                  bd->client.win);

             bd->client.e.state.deiconify_approve.need_send = 1;
             bd->changed = 1;

             bd_ancestor = bd->client.e.state.deiconify_approve.ancestor;
             if (bd_ancestor)
               {
                  bd_ancestor->client.e.state.deiconify_approve.req_list = eina_list_append(bd_ancestor->client.e.state.deiconify_approve.req_list, bd);
               }
          }

        if (e_config->transient.iconify)
          {
             Eina_List *l;
             E_Border *child;
             Eina_List *list = _e_border_sub_borders_new(bd);
             EINA_LIST_FOREACH(list, l, child)
               {
                  // if rotation is in progress, skip.
                  // it will be called when send rotation request.
                  if ((!child->client.e.state.deiconify_approve.pending) ||
                      ((e_border_rotation_is_progress(child)) &&
                       (!child->client.e.state.rot.wait_for_done))) continue;

                  _e_border_deiconify_approve_send_pending_end(child);
               }
             eina_list_free(list);
          }
     }
}

static void
_e_border_deiconify_approve_send(E_Border *bd, E_Border *bd_ancestor, Eina_Bool pending_ancestor)
{
   if (!bd || !bd_ancestor) return;

   if (e_config->deiconify_approve)
     {
        if (e_config->transient.iconify)
          {
             Eina_List *l;
             E_Border *child;
             Eina_List *list = _e_border_sub_borders_new(bd);
             EINA_LIST_FOREACH(list, l, child)
               {
                  Eina_Bool pending = EINA_FALSE;
                  Eina_Bool p = EINA_FALSE;
#ifdef _F_ZONE_WINDOW_ROTATION_
                  if ((e_config->wm_win_rotation) &&
                      ((child->client.e.state.rot.support) ||
                       (child->client.e.state.rot.app_set)))
                    {
                       ELB(ELBT_ROT, "CHECK_DEICONIFY CHILD", child->client.win);
                       int rotation = e_border_rotation_recommend_angle_get(child);
                       if (rotation != -1)
                         {
                            if (e_border_rotation_set(child, rotation))
                              {
                                 // if rotation request is sent to application not yet,
                                 // pending to send deiconify approve message.
                                 // thus deiconify approve message will be sent in time to send rotation request.
                                 // but the message of rotation request is already sent to application,
                                 // there is no need to delay sending the message of deiconify approve.
                                 if (!child->client.e.state.rot.wait_for_done)
                                   pending = EINA_TRUE;
                              }
                         }
                    }
#endif
                  if ((pending_ancestor) || (pending)) p = EINA_TRUE;

                  _e_border_deiconify_approve_send(child, bd_ancestor, p);

                  if (!p)
                    {
                       ELBF(ELBT_BD, 0, child->client.win,
                            "NEED to send DEICONIFY_APPROVE. ancestor:%x pending(%d,%d)",
                            bd_ancestor->client.win,
                            pending_ancestor, pending);

                       child->client.e.state.deiconify_approve.need_send = 1;
                       child->changed = 1;

                       child->client.e.state.deiconify_approve.ancestor = bd_ancestor;
                       bd_ancestor->client.e.state.deiconify_approve.req_list = eina_list_append(bd_ancestor->client.e.state.deiconify_approve.req_list, child);
                    }
                  else
                    {
                       child->client.e.state.deiconify_approve.need_send = 0;

                       /* queue a deiconify send job to give the chance to other jobs */
                       ELBF(ELBT_BD, 0, child->client.win,
                            "SEND DEICONIFY_APPROVE. (PENDING) ancestor:%x",
                            bd_ancestor->client.win);
                       child->client.e.state.deiconify_approve.ancestor = bd_ancestor;
                       child->client.e.state.deiconify_approve.pending = 1;
                    }
               }
             eina_list_free(list);
          }
     }
}

static void
_e_border_deiconify_approve_send_all_transient(E_Border *bd)
{
   Eina_Bool pending = EINA_FALSE;

   if (e_config->deiconify_approve)
     {
#ifdef _F_ZONE_WINDOW_ROTATION_
        if ((e_config->wm_win_rotation) &&
            ((bd->client.e.state.rot.support) ||
             (bd->client.e.state.rot.app_set)))
          {
             ELB(ELBT_ROT, "CHECK_DEICONIFY", bd->client.win);
             int rotation = e_border_rotation_recommend_angle_get(bd);
             if (rotation != -1)
               {
                  if (e_border_rotation_set(bd, rotation))
                    {
                       // if rotation request is sent to application not yet,
                       // pending to send deiconify approve message.
                       // thus deiconify approve message will be sent in time to send rotation request.
                       // but the message of rotation request is already sent to application,
                       // there is no need to delay sending the message of deiconify approve.
                       if (!bd->client.e.state.rot.wait_for_done)
                         pending = EINA_TRUE;
                    }
               }
          }
#endif
        if (e_config->transient.iconify)
          {
             _e_border_deiconify_approve_send(bd, bd, pending);
          }

        if (!pending)
          {
             ELBF(ELBT_BD, 0, bd->client.win, "NEED to send DEICONIFY_APPROVE.");
             bd->client.e.state.deiconify_approve.need_send = 1;
             bd->changed = 1;
          }
        else
          {
             bd->client.e.state.deiconify_approve.need_send = 0;

             /* queue a deiconify send job to give the chance to other jobs */
             ELBF(ELBT_BD, 0, bd->client.win, "SEND DEICONIFY_APPROVE. (PENDING)");
             bd->client.e.state.deiconify_approve.pending = 1;
          }
     }
}

static void
_e_border_uniconify_pending_list_add(E_Border *bd)
{
   if (!bd) return;

   if (!eina_list_data_find(_uniconify_pending_list, bd))
     {
        ELB(ELBT_BD, "ADD to PENDING uniconify event", bd->client.win);
        _uniconify_pending_list = eina_list_append(_uniconify_pending_list, bd);
     }

   if (!eina_list_data_find(_uniconify_waiting_list, bd))
     {
        _uniconify_waiting_list = eina_list_append(_uniconify_waiting_list, bd);
     }
}

static void
_e_border_uniconify_pending_list_remove(E_Border *bd)
{
   Eina_List *l = NULL;
   E_Border *uniconify_bd = NULL;

   if (!bd) return;

   _uniconify_waiting_list = eina_list_remove(_uniconify_waiting_list, bd);
   ELB(ELBT_BD, "REMOVE from PENDING uniconify event", bd->client.win);

   if (!_uniconify_waiting_list)
     {
        if (_uniconify_pending_list)
          {
             Eina_List *flush_list = eina_list_clone(_uniconify_pending_list);

             eina_list_free(_uniconify_pending_list);
             _uniconify_pending_list = NULL;

             EINA_LIST_FOREACH(flush_list, l, uniconify_bd)
               {
                  if (!e_object_is_del(E_OBJECT(uniconify_bd)))
                    {
                       if (uniconify_bd->client.e.state.deiconify_approve.render_done)
                         {
                            ELB(ELBT_BD, "UNICONIFY Pended window", uniconify_bd->client.win);
                            e_border_uniconify(uniconify_bd);
                         }
                       else
                         {
                            ELB(ELBT_BD, "UNICONIFY Pended window, BUT not render_done.", uniconify_bd->client.win);
                         }
                    }
               }
             eina_list_free(flush_list);
          }
     }
}
#endif

EAPI void
e_border_uniconify(E_Border *bd)
{
   E_Desk *desk;
   E_Event_Border_Uniconify *ev;
   unsigned int iconic;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

#ifdef _F_BORDER_HOOK_PATCH_
   _e_border_hook_call(E_BORDER_HOOK_UNICONIFY_BORDER, bd);
#endif

#ifdef _F_USE_VIRT_RESOLUTION_
   if(EINA_FALSE == _e_mod_check_change_resolution(bd->client.win))
     {
        return ECORE_CALLBACK_CANCEL;
     }
#endif /* end of _F_USE_VIRT_RESOLUTION_ */
   if (bd->iconify_by_client == 1)
     {
        ELBF(ELBT_BD, 0, bd->client.win, "Iconified by client. Not to uniconify...");
        return;
     }

   traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:BORDER:UNICONIFY");

#ifdef _F_DEICONIFY_APPROVE_
   if (e_config->deiconify_approve)
     {
        if (bd->client.e.state.deiconify_approve.render_only)
          {
             bd->client.e.state.deiconify_approve.defer_deiconify = 1;
             ELB(ELBT_BD, "DEICONIFY_APPROVE FORCE_RENDER is already running", bd->client.win);
             traceEnd(TTRACE_TAG_WINDOW_MANAGER);
             return;
          }
        if (bd->client.e.state.deiconify_approve.wait_timer)
          {
             ELB(ELBT_BD, "DEICONIFY_APPROVE WAIT_TIMER is already running", bd->client.win);
             _e_border_uniconify_pending_list_add(bd);
             traceEnd(TTRACE_TAG_WINDOW_MANAGER);
             return;
          }

        if (bd->client.e.state.deiconify_approve.pending)
          {
             ELB(ELBT_BD, "DEICONIFY_APPROVE PENDING_JOB is already running", bd->client.win);
             traceEnd(TTRACE_TAG_WINDOW_MANAGER);
             return;
          }

        if (bd->client.e.state.deiconify_approve.render_done == 0)
          {
             if (!bd->client.e.state.deiconify_approve.need_send)
               {
                  bd->client.e.state.deiconify_approve.need_send = 1;

#ifdef _F_USE_TILED_DESK_LAYOUT_
                  if (e_config->use_tiled_desk_layout)
                    {
                       if (!bd->client.e.state.ly.changing)
                         _e_border_desk_layout_set(bd);
                    }
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

#ifdef _F_USE_ICONIFY_RESIZE_
                  if (e_config->use_iconify_resize)
                    {
                       if (bd->iconify_resize.support)
                         {
                            if (bd->iconify_resize.set)
                              {
                                 bd->iconify_resize.need_draw = 1;
                                 bd->iconify_resize.set = 0;
                                 e_border_move_resize(bd, bd->iconify_resize.x, bd->iconify_resize.y, bd->iconify_resize.w, bd->iconify_resize.h);

                                 ELBF(ELBT_BD, 0, bd->client.win,"UnIconify Resize: Geo (%d, %d, %d, %d)", bd->x, bd->y, bd->w, bd->h);
#ifdef _F_USE_BORDER_TRANSFORM_
                                 e_border_transform_update(bd);
#endif
                              }
                         }
                    }
#endif

                  _e_border_uniconify_pending_list_add(bd);

                  ELB(ELBT_BD, "DEICONIFY_APPROVE to all transient", bd->client.win);
                  _e_border_deiconify_approve_send_all_transient(bd);
               }

             traceEnd(TTRACE_TAG_WINDOW_MANAGER);
             return;
          }
        bd->client.e.state.deiconify_approve.render_done = 0;
     }
   else
#endif
     {
#ifdef _F_USE_TILED_DESK_LAYOUT_
        if (e_config->use_tiled_desk_layout)
          {
             if (!bd->client.e.state.ly.changing)
               {
                  _e_border_desk_layout_set(bd);
               }
          }
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
     }

#if _F_ZONE_WINDOW_ROTATION_
   if (!bd->client.win)
     {
        ELB(ELBT_DFT, "ERR! obj is already deleted", bd->client.win);
        traceEnd(TTRACE_TAG_WINDOW_MANAGER);
        return;
     }
#endif

   if (bd->shading)
     {
        traceEnd(TTRACE_TAG_WINDOW_MANAGER);
        return;
     }

#ifdef _F_USE_ICONIFY_RESIZE_
   if (e_config->use_iconify_resize)
     {
        if (bd->iconify_resize.support)
          {
             if (bd->iconify_resize.set)
               {
                  bd->iconify_resize.need_draw = 1;
                  bd->iconify_resize.set = 0;
                  e_border_move_resize(bd, bd->iconify_resize.x, bd->iconify_resize.y, bd->iconify_resize.w, bd->iconify_resize.h);

                  ELBF(ELBT_BD, 0, bd->client.win,"UnIconify Resize: Geo (%d, %d, %d, %d)", bd->x, bd->y, bd->w, bd->h);
#ifdef _F_USE_BORDER_TRANSFORM_
                  e_border_transform_update(bd);
#endif
               }
          }
     }
#endif

   ecore_x_window_shadow_tree_flush();
   e_border_show(bd);

#ifdef _F_DEICONIFY_APPROVE_
   Eina_Bool not_raise = EINA_FALSE;
   not_raise = bd->client.e.state.deiconify_approve.not_raise;
#endif

   if (bd->iconic)
     {
        bd->iconic = 0;
        if (bd->fullscreen) bd->desk->fullscreen_borders++;
        desk = e_desk_current_get(bd->desk->zone);
#ifdef _F_USE_EXTENDED_ICONIFY_
        if (e_manager_comp_evas_get(bd->zone->container->manager))
          {
             if (bd->await_hide_event > 0)
               bd->await_hide_event--;
          }
#endif
        e_border_desk_set(bd, desk);
#ifdef _F_DEICONIFY_APPROVE_
        if (!bd->client.e.state.deiconify_approve.pending_list)
          {
             if (!bd->client.e.state.deiconify_approve.not_raise)
               {
                  e_border_raise(bd);
               }
          }
#else
        e_border_raise(bd);
#endif
        edje_object_signal_emit(bd->bg_object, "e,action,uniconify", "e");
     }
   iconic = 0;
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MAPPED, &iconic, 1);
#ifdef _F_DEICONIFY_APPROVE_
   bd->client.e.state.deiconify_approve.not_raise = 0;
#endif

   ev = E_NEW(E_Event_Border_Uniconify, 1);
   if (ev)
     {
        ev->border = bd;
        e_object_ref(E_OBJECT(bd));
        //   e_object_breadcrumb_add(E_OBJECT(bd), "border_uniconify_event");
        ecore_event_add(E_EVENT_BORDER_UNICONIFY, ev, _e_border_event_border_uniconify_free, NULL);
     }

   if (e_config->transient.iconify)
     {
        Eina_List *l;
        E_Border *child;
        Eina_List *list = _e_border_sub_borders_new(bd);

        EINA_LIST_FOREACH(list, l, child)
          {
#ifdef _F_DEICONIFY_APPROVE_
             child->client.e.state.deiconify_approve.not_raise = not_raise;
#endif
             e_border_uniconify(child);
          }
        eina_list_free(list);
     }
   e_remember_update(bd);
   traceEnd(TTRACE_TAG_WINDOW_MANAGER);
}

EAPI void
e_border_stick(E_Border *bd)
{
   E_Event_Border_Stick *ev;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (bd->sticky) return;
   bd->sticky = 1;
   e_hints_window_sticky_set(bd, 1);
   e_border_show(bd);

   if (e_config->transient.desktop)
     {
        Eina_List *l;
        E_Border *child;
        Eina_List *list = _e_border_sub_borders_new(bd);

        EINA_LIST_FOREACH(list, l, child)
          {
             child->sticky = 1;
             e_hints_window_sticky_set(child, 1);
             e_border_show(child);
          }
        eina_list_free(list);
     }

   edje_object_signal_emit(bd->bg_object, "e,state,sticky", "e");
   ev = E_NEW(E_Event_Border_Stick, 1);
   if (ev)
     {
        ev->border = bd;
        e_object_ref(E_OBJECT(bd));
        //   e_object_breadcrumb_add(E_OBJECT(bd), "border_stick_event");
        ecore_event_add(E_EVENT_BORDER_STICK, ev, _e_border_event_border_stick_free, NULL);
     }

   e_remember_update(bd);
}

EAPI void
e_border_unstick(E_Border *bd)
{
   E_Event_Border_Unstick *ev;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   /* Set the desk before we unstick the border */
   if (!bd->sticky) return;
   bd->sticky = 0;
   e_hints_window_sticky_set(bd, 0);

   if (e_config->transient.desktop)
     {
        Eina_List *l;
        E_Border *child;
        Eina_List *list = _e_border_sub_borders_new(bd);

        EINA_LIST_FOREACH(list, l, child)
          {
             child->sticky = 0;
             e_hints_window_sticky_set(child, 0);
          }
        eina_list_free(list);
     }

   edje_object_signal_emit(bd->bg_object, "e,state,unsticky", "e");
   ev = E_NEW(E_Event_Border_Unstick, 1);
   if (ev)
     {
        ev->border = bd;
        e_object_ref(E_OBJECT(bd));
        //   e_object_breadcrumb_add(E_OBJECT(bd), "border_unstick_event");
        ecore_event_add(E_EVENT_BORDER_UNSTICK, ev, _e_border_event_border_unstick_free, NULL);
     }

   e_border_desk_set(bd, e_desk_current_get(bd->zone));
   e_remember_update(bd);
}

EAPI void
e_border_pinned_set(E_Border *bd,
                    int       set)
{
   int layer;
   int stacking;

   if (bd)
     {
        bd->borderless = set;
        bd->user_skip_winlist = set;
        if (set)
          {
             layer = 50;
             stacking = E_STACKING_BELOW;
          }
        else
          {
             layer = 100;
             stacking = E_STACKING_NONE;
          }

        e_border_layer_set(bd, layer);
        e_hints_window_stacking_set(bd, stacking);

        bd->client.border.changed = 1;
        bd->changed = 1;
     }
}

EAPI E_Border *
e_border_find_by_client_window(Ecore_X_Window win)
{
   E_Border *bd;

   bd = eina_hash_find(borders_hash, e_util_winid_str_get(win));
   if ((bd) && (!e_object_is_del(E_OBJECT(bd))) &&
       (bd->client.win == win))
     return bd;
   return NULL;
}

EAPI E_Border *
e_border_find_all_by_client_window(Ecore_X_Window win)
{
   E_Border *bd;

   bd = eina_hash_find(borders_hash, e_util_winid_str_get(win));
   if ((bd) && (bd->client.win == win))
     return bd;
   return NULL;
}

EAPI E_Border *
e_border_find_by_frame_window(Ecore_X_Window win)
{
   E_Border *bd;

   bd = eina_hash_find(borders_hash, e_util_winid_str_get(win));
   if ((bd) && (!e_object_is_del(E_OBJECT(bd))) &&
       (bd->bg_win == win))
     return bd;
   return NULL;
}

EAPI E_Border *
e_border_find_by_window(Ecore_X_Window win)
{
   E_Border *bd;

   bd = eina_hash_find(borders_hash, e_util_winid_str_get(win));
   if ((bd) && (!e_object_is_del(E_OBJECT(bd))) &&
       (bd->win == win))
     return bd;
   return NULL;
}

EAPI E_Border *
e_border_find_by_alarm(Ecore_X_Sync_Alarm al)
{
   Eina_List *l;
   E_Border *bd;

   EINA_LIST_FOREACH(borders, l, bd)
     {
        if ((bd) && (!e_object_is_del(E_OBJECT(bd))) &&
            (bd->client.netwm.sync.alarm == al))
          return bd;
     }
   return NULL;
}

EAPI E_Border *
e_border_focused_get(void)
{
   return focused;
}

static void
_e_border_shape_input_rectangle_set(E_Border* bd)
{
   if (!bd) return;

   if ((bd->visible) && (bd->shaped_input))
     {
#ifdef _F_SHAPE_INPUT_PATCH_
        Ecore_X_Rectangle *rects;
        Ecore_X_Rectangle *client_rects;
        Ecore_X_Window twin, twin2;
        int x, y;
        int num_client = 0;
        int num_rect = 0;

        client_rects = ecore_x_window_shape_input_rectangles_get(bd->client.win, &num_client);

        num_rect = num_client + 4;
        rects = E_NEW(Ecore_X_Rectangle, num_rect);
        if (!rects)
          {
             if (client_rects) free(client_rects);
             return;
          }

        twin = ecore_x_window_override_new(bd->zone->container->scratch_win,
                                           0, 0, bd->w, bd->h);
        rects[0].x = 0;
        rects[0].y = 0;
        rects[0].width = bd->w;
        rects[0].height = bd->client_inset.t;
        rects[1].x = 0;
        rects[1].y = bd->client_inset.t;
        rects[1].width = bd->client_inset.l;
        rects[1].height = bd->h - bd->client_inset.t - bd->client_inset.b;
        rects[2].x = bd->w - bd->client_inset.r;
        rects[2].y = bd->client_inset.t;
        rects[2].width = bd->client_inset.r;
        rects[2].height = bd->h - bd->client_inset.t - bd->client_inset.b;
        rects[3].x = 0;
        rects[3].y = bd->h - bd->client_inset.b;
        rects[3].width = bd->w;
        rects[3].height = bd->client_inset.b;

        if (client_rects)
          {
             int i;
             for (i=0; i<num_client; i++)
               {
                  rects[i+4].x = client_rects[i].x;
                  rects[i+4].y = client_rects[i].y;
                  rects[i+4].width = client_rects[i].width;
                  rects[i+4].height = client_rects[i].height;
               }
             free(client_rects);
          }

        ecore_x_window_shape_input_rectangles_set(twin, rects, num_rect);

        twin2 = ecore_x_window_override_new
           (bd->zone->container->scratch_win, 0, 0,
               bd->w - bd->client_inset.l - bd->client_inset.r,
               bd->h - bd->client_inset.t - bd->client_inset.b);
        x = 0;
        y = 0;
        if ((bd->shading) || (bd->shaded))
          {
             if (bd->shade.dir == E_DIRECTION_UP)
                y = bd->h - bd->client_inset.t - bd->client_inset.b -
                bd->client.h;
             else if (bd->shade.dir == E_DIRECTION_LEFT)
                x = bd->w - bd->client_inset.l - bd->client_inset.r -
                bd->client.w;
          }
        ecore_x_window_shape_input_window_set_xy(twin2, bd->client.win,
                                                 x, y);
        ecore_x_window_shape_input_rectangle_clip(twin2, 0, 0,
                                                  bd->w - bd->client_inset.l - bd->client_inset.r,
                                                  bd->h - bd->client_inset.t - bd->client_inset.b);
        ecore_x_window_shape_input_window_add_xy(twin, twin2,
                                                 bd->client_inset.l,
                                                 bd->client_inset.t);
        ecore_x_window_shape_input_window_set(bd->win, twin);
        ecore_x_window_free(twin2);
        ecore_x_window_free(twin);

        E_FREE(rects);

#else
        Ecore_X_Rectangle rects[4];
        Ecore_X_Window twin, twin2;
        int x, y;

        twin = ecore_x_window_override_new(bd->zone->container->scratch_win,
                                           0, 0, bd->w, bd->h);
        rects[0].x = 0;
        rects[0].y = 0;
        rects[0].width = bd->w;
        rects[0].height = bd->client_inset.t;
        rects[1].x = 0;
        rects[1].y = bd->client_inset.t;
        rects[1].width = bd->client_inset.l;
        rects[1].height = bd->h - bd->client_inset.t - bd->client_inset.b;
        rects[2].x = bd->w - bd->client_inset.r;
        rects[2].y = bd->client_inset.t;
        rects[2].width = bd->client_inset.r;
        rects[2].height = bd->h - bd->client_inset.t - bd->client_inset.b;
        rects[3].x = 0;
        rects[3].y = bd->h - bd->client_inset.b;
        rects[3].width = bd->w;
        rects[3].height = bd->client_inset.b;
        ecore_x_window_shape_input_rectangles_set(twin, rects, 4);

        twin2 = ecore_x_window_override_new
           (bd->zone->container->scratch_win, 0, 0,
               bd->w - bd->client_inset.l - bd->client_inset.r,
               bd->h - bd->client_inset.t - bd->client_inset.b);
        x = 0;
        y = 0;
        if ((bd->shading) || (bd->shaded))
          {
             if (bd->shade.dir == E_DIRECTION_UP)
                y = bd->h - bd->client_inset.t - bd->client_inset.b -
                bd->client.h;
             else if (bd->shade.dir == E_DIRECTION_LEFT)
                x = bd->w - bd->client_inset.l - bd->client_inset.r -
                bd->client.w;
          }
        ecore_x_window_shape_input_window_set_xy(twin2, bd->client.win,
                                                 x, y);
        ecore_x_window_shape_input_rectangle_clip(twin2, 0, 0,
                                                  bd->w - bd->client_inset.l - bd->client_inset.r,
                                                  bd->h - bd->client_inset.t - bd->client_inset.b);
        ecore_x_window_shape_input_window_add_xy(twin, twin2,
                                                 bd->client_inset.l,
                                                 bd->client_inset.t);
        ecore_x_window_shape_input_window_set(bd->win, twin);
        ecore_x_window_free(twin2);
        ecore_x_window_free(twin);
#endif
     }
   else
     {
        if (bd->visible) // not shaped input
          {
             if (!((bd->comp_hidden) || (bd->tmp_input_hidden > 0)))
                ecore_x_composite_window_events_enable(bd->win);
             else
                ecore_x_composite_window_events_disable(bd->win);
          }
        else
          {
             if (!e_manager_comp_evas_get(bd->zone->container->manager))
                ecore_x_composite_window_events_enable(bd->win);
             else
                ecore_x_composite_window_events_disable(bd->win);
          }
     }
}

EAPI void
e_border_idler_before(void)
{
   Eina_List *ml, *cl;
   E_Manager *man;
   E_Container *con;

   if (!borders)
     return;
   traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:BORDER:IDLE BEFORE");

#ifdef _F_ZONE_WINDOW_ROTATION_
   if (rot.cancel.state)
     {
        /* there is no border which supports window manager rotation */
        e_zone_rotation_update_cancel(rot.cancel.zone);
        rot.cancel.state = EINA_FALSE;
        rot.cancel.zone = NULL;
     }
#endif

   EINA_LIST_FOREACH(e_manager_list(), ml, man)
     {
        EINA_LIST_FOREACH(man->containers, cl, con)
          {
             E_Border_List *bl;
             E_Border *bd;

             // pass 1 - eval0. fetch properties on new or on change and
             // call hooks to decide what to do - maybe move/resize
             bl = e_container_border_list_last(con);
             if (bl)
               {
                  while ((bd = e_container_border_list_prev(bl)))
                    {
                       if (bd->changed) _e_border_eval0(bd);
                    }
                  e_container_border_list_free(bl);
               }

             // layout hook - this is where a hook gets to figure out what to
             // do if anything.
             _e_border_container_layout_hook(con);

             // pass 2 - show windows needing show
             bl = e_container_border_list_last(con);
             if (bl)
               {
                  while ((bd = e_container_border_list_prev(bl)))
                    {
                       if ((bd->changes.visible) && (bd->visible) &&
                           (!bd->new_client) && (!bd->changes.pos) &&
                           (!bd->changes.size))
                         {
                            _e_border_show(bd);
                            bd->changes.visible = 0;
                         }
                    }
                  e_container_border_list_free(bl);
               }

             // pass 3 - hide windows needing hide and eval (main eval)
             bl = e_container_border_list_first(con);
             if (bl)
               {
                  while ((bd = e_container_border_list_next(bl)))
                    {
                       if (e_object_is_del(E_OBJECT(bd))) continue;

                       if ((bd->changes.visible) && (!bd->visible))
                         {
                            _e_border_hide(bd);
                            bd->changes.visible = 0;
                         }

                       if (bd->changed) _e_border_eval(bd);

                       if ((bd->changes.visible) && (bd->visible))
                         {
                            _e_border_show(bd);
                            bd->changes.visible = 0;
                         }
                    }
                  e_container_border_list_free(bl);
               }
          }
     }

   if (focus_next)
     {
        E_Border *bd = NULL, *bd2;

        EINA_LIST_FREE(focus_next, bd2)
          if ((!bd) && (bd2->visible)) bd = bd2;

        if (!bd)
          {
             /* TODO revert focus when lost here ? */
             return;
          }
#if 0
        if (bd == focused)
          {
             /* already focused. but anyway dont be so strict, this
                fcks up illume setting focus on internal windows */
             return;
          }
#endif

        focus_time = ecore_x_current_time_get();

        focusing = bd;

        if ((bd->client.icccm.take_focus) &&
            (bd->client.icccm.accepts_focus))
          {
             e_grabinput_focus(bd->client.win, E_FOCUS_METHOD_LOCALLY_ACTIVE);
             /* TODO what if the client didn't take focus ? */
          }
        else if (!bd->client.icccm.accepts_focus)
          {
             e_grabinput_focus(bd->client.win, E_FOCUS_METHOD_GLOBALLY_ACTIVE);
          }
        else if (!bd->client.icccm.take_focus)
          {
             e_grabinput_focus(bd->client.win, E_FOCUS_METHOD_PASSIVE);
             /* e_border_focus_set(bd, 1, 0); */
          }
     }

#ifdef _F_ZONE_WINDOW_ROTATION_
   if ((e_config->wm_win_rotation) &&
       (rot.fetch))
     {
        Ecore_X_Event_Client_Message *msg = NULL;
        Ecore_X_Atom t = 0;
        EINA_LIST_FREE(rot.msgs, msg)
          {
             t = msg->message_type;
             if (t == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_ON_PREPARE_DONE)
               {
                  if ((rot.vkbd_ctrl_win) &&
                      ((Ecore_X_Window)msg->data.l[0] == rot.vkbd_ctrl_win) &&
                      (rot.vkbd))
                    {
                       ELB(ELBT_BD, "GET KBD_ON_PREPARE_DONE", rot.vkbd_ctrl_win);
                       if (rot.vkbd_show_prepare_timer)
                         _e_border_vkbd_show(rot.vkbd);
                       else
                         ELB(ELBT_BD, "GET KBD_ON_PREPARE_DONE but skip", rot.vkbd_ctrl_win);
                    }
               }
             else if (t == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_OFF_PREPARE_DONE)
               {
                  if ((rot.vkbd_ctrl_win) &&
                      ((Ecore_X_Window)msg->data.l[0] == rot.vkbd_ctrl_win) &&
                      (rot.vkbd))
                    {
                       ELB(ELBT_BD, "GET KBD_OFF_PREPARE_DONE", rot.vkbd_ctrl_win);
                       if (rot.vkbd_hide_prepare_timer)
                         {
                            _e_border_vkbd_hide(rot.vkbd, EINA_TRUE);
                         }
                       else
                         ELB(ELBT_BD, "GET KBD_OFF_PREPARE_DONE but skip", rot.vkbd_ctrl_win);
                    }
               }
             else if (t == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_CONTROL_WINDOW)
               {
                  rot.vkbd_ctrl_win = msg->data.l[0];
                  ELB(ELBT_BD, "SET KBD_CONTROL_WIN", rot.vkbd_ctrl_win);
               }
             else if (t == ECORE_X_ATOM_E_WINDOW_ROTATION_CHANGE_PREPARE_DONE)
               {
                  if ((rot.vkbd_ctrl_win) &&
                      (rot.vkbd_ctrl_win == (Ecore_X_Window)msg->data.l[0]))
                    {
                       ELB(ELBT_ROT, "GET ROT_PREPARE_DONE", rot.vkbd_ctrl_win);
                       E_Manager *m = e_manager_current_get();
                       E_Zone *zone = NULL;
                       if (m) zone = e_util_zone_current_get(m);
                       if ((zone) && (rot.wait_prepare_done))
                         {
                            if (rot.list)
                              _e_border_rotation_list_send();
                            else
                              _e_border_rotation_change_done();

                            if (rot.prepare_timer)
                              ecore_timer_del(rot.prepare_timer);
                            rot.prepare_timer = NULL;
                            rot.wait_prepare_done = EINA_FALSE;
                         }
                    }
               }
             E_FREE(msg);
          }
        // if there is windows over 2 that has to be rotated or is existed window needs resizing,
        // lock the screen.
        // but, DO NOT lock the screen when rotation block state
        if (eina_list_count(rot.list) > 1)
          {
             Eina_List *l;
             E_Border *bd;
             Eina_Bool rot_block = EINA_FALSE;

             EINA_LIST_FOREACH(rot.list, l, bd)
               {
                  if (!bd->zone) continue;
                  if (bd->zone->rot.block_count)
                    {
                       rot_block = EINA_TRUE;
                    }
               }
             if ((!rot.screen_lock) && (!rot_block))
               {
                  e_manager_comp_screen_lock(e_manager_current_get());
                  rot.screen_lock = EINA_TRUE;
               }
          }
        else if (eina_list_count(rot.list) == 0)
          {
             E_Border *bd;
             Eina_List *zlist = NULL;
             Eina_List *l = NULL;
             E_Zone *zone = NULL;

             if (rot.async_list)
               {
                  EINA_LIST_FREE(rot.async_list, bd)
                    {
                       if (!eina_list_data_find(zlist, bd->zone))
                         zlist = eina_list_append(zlist, bd->zone);
                       _e_border_rotation_change_message_send(bd);
                    }

                  EINA_LIST_FOREACH(zlist, l, zone)
                    e_zone_rotation_update_cancel(zone);
                  if (zlist)
                    eina_list_free(zlist);
               }
          }

        rot.msgs = NULL;
        rot.fetch = EINA_FALSE;
     }
#endif
   traceEnd(TTRACE_TAG_WINDOW_MANAGER);
}

EAPI Eina_List *
e_border_client_list(void)
{
   /* FIXME: This should be a somewhat ordered list */
   return borders;
}

static Ecore_X_Window action_input_win = 0;
static E_Border *action_border = NULL;
static Ecore_Event_Handler *action_handler_key = NULL;
static Ecore_Event_Handler *action_handler_mouse = NULL;
static Ecore_Timer *action_timer = NULL;
static Ecore_X_Rectangle action_orig;

static void
_e_border_show(E_Border *bd)
{
   Eina_List *l;
   E_Border *tmp;

   ecore_evas_show(bd->bg_ecore_evas);

   if (bd->post_job)
     {
        bd->post_show = 1;
        return;
     }

   if (!((bd->comp_hidden) || (bd->tmp_input_hidden > 0)))
     {
        _e_border_shape_input_rectangle_set(bd);
        // not anymore
        //	ecore_x_composite_window_events_enable(bd->win);
        ecore_x_window_ignore_set(bd->win, EINA_FALSE);
     }

   ecore_x_window_show(bd->win);

   EINA_LIST_FOREACH(bd->client.e.state.video_child, l, tmp)
     ecore_x_window_show(tmp->win);
}

static void
_e_border_hide(E_Border *bd)
{
   E_Border *tmp;
   Eina_List *l;

   if (!e_manager_comp_evas_get(bd->zone->container->manager))
     {
        ecore_x_window_hide(bd->win);
        ecore_evas_hide(bd->bg_ecore_evas);

        EINA_LIST_FOREACH(bd->client.e.state.video_child, l, tmp)
          ecore_x_window_hide(tmp->win);
     }
   else
     {
        ecore_x_composite_window_events_disable(bd->win);
        ecore_x_window_ignore_set(bd->win, EINA_TRUE);
     }
}

static int
_e_border_action_input_win_del(void)
{
   if (!action_input_win)
     return 0;

   e_grabinput_release(action_input_win, action_input_win);
   ecore_x_window_free(action_input_win);
   action_input_win = 0;
   return 1;
}

static int
_e_border_action_input_win_new(E_Border *bd)
{
   if (!action_input_win)
     {
        Ecore_X_Window parent = bd->zone->container->win;
        action_input_win = ecore_x_window_input_new(parent, 0, 0, 1, 1);
        if (!action_input_win)
          return 0;
     }

   ecore_x_window_show(action_input_win);
   if (e_grabinput_get(action_input_win, 0, action_input_win))
     return 1;

   _e_border_action_input_win_del();
   return 0;
}

static void
_e_border_action_finish(void)
{
   _e_border_action_input_win_del();

   if (action_timer)
     {
        ecore_timer_del(action_timer);
        action_timer = NULL;
     }

   if (action_handler_key)
     {
        ecore_event_handler_del(action_handler_key);
        action_handler_key = NULL;
     }

   if (action_handler_mouse)
     {
        ecore_event_handler_del(action_handler_mouse);
        action_handler_mouse = NULL;
     }

   action_border = NULL;
}

static void
_e_border_action_init(E_Border *bd)
{
   action_orig.x = bd->x;
   action_orig.y = bd->y;
   action_orig.width = bd->w;
   action_orig.height = bd->h;

   action_border = bd;
}

static void
_e_border_action_restore_orig(E_Border *bd)
{
   if (action_border != bd)
     return;

   e_border_move_resize(bd, action_orig.x, action_orig.y, action_orig.width, action_orig.height);
}

static int
_e_border_key_down_modifier_apply(int modifier,
                                  int value)
{
   if (modifier & ECORE_EVENT_MODIFIER_CTRL)
     return value * 2;
   else if (modifier & ECORE_EVENT_MODIFIER_ALT)
     {
        value /= 2;
        if (value)
          return value;
        else
          return 1;
     }

   return value;
}

static Eina_Bool
_e_border_action_move_timeout(void *data __UNUSED__)
{
   _e_border_move_end(action_border);
   _e_border_action_finish();
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_border_action_move_timeout_add(void)
{
   if (action_timer)
     ecore_timer_del(action_timer);
   action_timer = ecore_timer_add(e_config->border_keyboard.timeout, _e_border_action_move_timeout, NULL);
}

static Eina_Bool
_e_border_move_key_down(void *data __UNUSED__,
                        int type   __UNUSED__,
                        void      *event)
{
   Ecore_Event_Key *ev = event;
   int x, y;

   if (ev->event_window != action_input_win)
     return ECORE_CALLBACK_PASS_ON;
   if (!action_border)
     {
        fputs("ERROR: no action_border!\n", stderr);
        goto stop;
     }

   x = action_border->x;
   y = action_border->y;

   if ((strcmp(ev->key, "Up") == 0) || (strcmp(ev->key, "k") == 0))
     y -= _e_border_key_down_modifier_apply(ev->modifiers, e_config->border_keyboard.move.dy);
   else if ((strcmp(ev->key, "Down") == 0) || (strcmp(ev->key, "j") == 0))
     y += _e_border_key_down_modifier_apply(ev->modifiers, e_config->border_keyboard.move.dy);
   else if ((strcmp(ev->key, "Left") == 0) || (strcmp(ev->key, "h") == 0))
     x -= _e_border_key_down_modifier_apply(ev->modifiers, e_config->border_keyboard.move.dx);
   else if ((strcmp(ev->key, "Right") == 0) || (strcmp(ev->key, "l") == 0))
     x += _e_border_key_down_modifier_apply(ev->modifiers, e_config->border_keyboard.move.dx);
   else if (strcmp(ev->key, "Return") == 0)
     goto stop;
   else if (strcmp(ev->key, "Escape") == 0)
     {
        _e_border_action_restore_orig(action_border);
        goto stop;
     }
   else if ((strncmp(ev->key, "Control", sizeof("Control") - 1) != 0) &&
            (strncmp(ev->key, "Alt", sizeof("Alt") - 1) != 0))
     goto stop;

   e_border_move(action_border, x, y);
   _e_border_action_move_timeout_add();

   return ECORE_CALLBACK_PASS_ON;

stop:
   _e_border_move_end(action_border);
   _e_border_action_finish();
   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_e_border_move_mouse_down(void *data __UNUSED__,
                          int type   __UNUSED__,
                          void      *event)
{
   Ecore_Event_Mouse_Button *ev = event;

   if (ev->event_window != action_input_win)
     return ECORE_CALLBACK_PASS_ON;

   if (!action_border)
     fputs("ERROR: no action_border!\n", stderr);

   _e_border_move_end(action_border);
   _e_border_action_finish();
   return ECORE_CALLBACK_DONE;
}

EAPI void
e_border_act_move_keyboard(E_Border *bd)
{
   if (!bd)
     return;

   if (!_e_border_move_begin(bd))
     return;

   if (!_e_border_action_input_win_new(bd))
     {
        _e_border_move_end(bd);
        return;
     }

   _e_border_action_init(bd);
   _e_border_action_move_timeout_add();
   _e_border_move_update(bd);

   if (action_handler_key)
     ecore_event_handler_del(action_handler_key);
   action_handler_key = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, _e_border_move_key_down, NULL);

   if (action_handler_mouse)
     ecore_event_handler_del(action_handler_mouse);
   action_handler_mouse = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, _e_border_move_mouse_down, NULL);
}

static Eina_Bool
_e_border_action_resize_timeout(void *data __UNUSED__)
{
   _e_border_resize_end(action_border);
   _e_border_action_finish();
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_border_action_resize_timeout_add(void)
{
   if (action_timer)
     ecore_timer_del(action_timer);
   action_timer = ecore_timer_add(e_config->border_keyboard.timeout, _e_border_action_resize_timeout, NULL);
}

static Eina_Bool
_e_border_resize_key_down(void *data __UNUSED__,
                          int type   __UNUSED__,
                          void      *event)
{
   Ecore_Event_Key *ev = event;
   int w, h, dx, dy;

   if (ev->event_window != action_input_win)
     return ECORE_CALLBACK_PASS_ON;
   if (!action_border)
     {
        fputs("ERROR: no action_border!\n", stderr);
        goto stop;
     }

   w = action_border->w;
   h = action_border->h;

   dx = e_config->border_keyboard.resize.dx;
   if (dx < action_border->client.icccm.step_w)
     dx = action_border->client.icccm.step_w;
   dx = _e_border_key_down_modifier_apply(ev->modifiers, dx);
   if (dx < action_border->client.icccm.step_w)
     dx = action_border->client.icccm.step_w;

   dy = e_config->border_keyboard.resize.dy;
   if (dy < action_border->client.icccm.step_h)
     dy = action_border->client.icccm.step_h;
   dy = _e_border_key_down_modifier_apply(ev->modifiers, dy);
   if (dy < action_border->client.icccm.step_h)
     dy = action_border->client.icccm.step_h;

   if ((strcmp(ev->key, "Up") == 0) || (strcmp(ev->key, "k") == 0))
     h -= dy;
   else if ((strcmp(ev->key, "Down") == 0) || (strcmp(ev->key, "j") == 0))
     h += dy;
   else if ((strcmp(ev->key, "Left") == 0) || (strcmp(ev->key, "h") == 0))
     w -= dx;
   else if ((strcmp(ev->key, "Right") == 0) || (strcmp(ev->key, "l") == 0))
     w += dx;
   else if (strcmp(ev->key, "Return") == 0)
     goto stop;
   else if (strcmp(ev->key, "Escape") == 0)
     {
        _e_border_action_restore_orig(action_border);
        goto stop;
     }
   else if ((strncmp(ev->key, "Control", sizeof("Control") - 1) != 0) &&
            (strncmp(ev->key, "Alt", sizeof("Alt") - 1) != 0))
     goto stop;

   e_border_resize_limit(action_border, &w, &h);
   e_border_resize(action_border, w, h);
   _e_border_action_resize_timeout_add();

   return ECORE_CALLBACK_PASS_ON;

stop:
   _e_border_resize_end(action_border);
   _e_border_action_finish();
   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_e_border_resize_mouse_down(void *data __UNUSED__,
                            int type   __UNUSED__,
                            void      *event)
{
   Ecore_Event_Mouse_Button *ev = event;

   if (ev->event_window != action_input_win)
     return ECORE_CALLBACK_PASS_ON;

   if (!action_border)
     fputs("ERROR: no action_border!\n", stderr);

   _e_border_resize_end(action_border);
   _e_border_action_finish();
   return ECORE_CALLBACK_DONE;
}

EAPI void
e_border_act_resize_keyboard(E_Border *bd)
{
   if (!bd)
     return;

   if (!_e_border_resize_begin(bd))
     return;

   if (!_e_border_action_input_win_new(bd))
     {
        _e_border_resize_end(bd);
        return;
     }

   _e_border_action_init(bd);
   _e_border_action_resize_timeout_add();
   _e_border_resize_update(bd);

   if (action_handler_key)
     ecore_event_handler_del(action_handler_key);
   action_handler_key = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, _e_border_resize_key_down, NULL);

   if (action_handler_mouse)
     ecore_event_handler_del(action_handler_mouse);
   action_handler_mouse = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, _e_border_resize_mouse_down, NULL);
}

EAPI void
e_border_act_move_begin(E_Border                 *bd,
                        Ecore_Event_Mouse_Button *ev)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if ((bd->resize_mode != RESIZE_NONE) || (bd->moving)) return;
   if (!_e_border_move_begin(bd))
     return;

   e_zone_edge_disable();
   bd->moving = 1;
   _e_border_pointer_move_begin(bd);
   if (ev)
     {
        char source[256];

        snprintf(source, sizeof(source) - 1, "mouse,down,%i", ev->buttons);
        _e_border_moveinfo_gather(bd, source);
     }
}

EAPI void
e_border_act_move_end(E_Border                    *bd,
                      Ecore_Event_Mouse_Button *ev __UNUSED__)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (!bd->moving) return;
   bd->moving = 0;
   _e_border_pointer_move_end(bd);
   e_zone_edge_enable();
   _e_border_move_end(bd);
   e_zone_flip_coords_handle(bd->zone, -1, -1);
}

EAPI void
e_border_act_resize_begin(E_Border                 *bd,
                          Ecore_Event_Mouse_Button *ev)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (bd->lock_user_size) return;
   if ((bd->resize_mode != RESIZE_NONE) || (bd->moving)) return;
   if (!_e_border_resize_begin(bd))
     return;
   if (bd->mouse.current.mx < (bd->x + bd->w / 2))
     {
        if (bd->mouse.current.my < (bd->y + bd->h / 2))
          {
             bd->resize_mode = RESIZE_TL;
             GRAV_SET(bd, ECORE_X_GRAVITY_SE);
          }
        else
          {
             bd->resize_mode = RESIZE_BL;
             GRAV_SET(bd, ECORE_X_GRAVITY_NE);
          }
     }
   else
     {
        if (bd->mouse.current.my < (bd->y + bd->h / 2))
          {
             bd->resize_mode = RESIZE_TR;
             GRAV_SET(bd, ECORE_X_GRAVITY_SW);
          }
        else
          {
             bd->resize_mode = RESIZE_BR;
             GRAV_SET(bd, ECORE_X_GRAVITY_NW);
          }
     }
   _e_border_pointer_resize_begin(bd);
   if (ev)
     {
        char source[256];

        snprintf(source, sizeof(source) - 1, "mouse,down,%i", ev->buttons);
        _e_border_moveinfo_gather(bd, source);
     }
}

EAPI void
e_border_act_resize_end(E_Border                    *bd,
                        Ecore_Event_Mouse_Button *ev __UNUSED__)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (bd->resize_mode != RESIZE_NONE)
     {
        _e_border_pointer_resize_end(bd);
        bd->resize_mode = RESIZE_NONE;
        _e_border_resize_end(bd);
        bd->changes.reset_gravity = 1;
        bd->changed = 1;
     }
}

#ifndef _F_DISABLE_E_MENU
EAPI void
e_border_act_menu_begin(E_Border                 *bd,
                        Ecore_Event_Mouse_Button *ev,
                        int                       key)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (ev)
     {
        e_int_border_menu_show(bd,
                               bd->x + bd->fx.x + ev->x - bd->zone->container->x,
                               bd->y + bd->fx.y + ev->y - bd->zone->container->y, key,
                               ev->timestamp);
     }
   else
     {
        int x, y;

        ecore_x_pointer_xy_get(bd->zone->container->win, &x, &y);
        e_int_border_menu_show(bd, x, y, key, 0);
     }
}

#endif

EAPI void
e_border_act_close_begin(E_Border *bd)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (bd->lock_close) return;
   if (bd->client.icccm.delete_request)
     {
        bd->delete_requested = 1;
        ecore_x_window_delete_request_send(bd->client.win);
        if (bd->client.netwm.ping)
          e_border_ping(bd);
     }
   else if (e_config->kill_if_close_not_possible)
     {
        e_border_act_kill_begin(bd);
     }
}

EAPI void
e_border_act_kill_begin(E_Border *bd)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (bd->internal) return;
   if (bd->lock_close) return;
   if ((bd->client.netwm.pid > 1) && (e_config->kill_process))
     {
        kill(bd->client.netwm.pid, SIGINT);
        bd->kill_timer = ecore_timer_add(e_config->kill_timer_wait,
                                         _e_border_cb_kill_timer, bd);
     }
   else
     {
        if (!bd->internal) ecore_x_kill(bd->client.win);
     }
}

EAPI Evas_Object *
e_border_icon_add(E_Border *bd,
                  Evas     *evas)
{
   Evas_Object *o;

   E_OBJECT_CHECK_RETURN(bd, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, NULL);

   o = NULL;
   if (bd->internal)
     {
        if (!bd->internal_icon)
          {
             o = e_icon_add(evas);
             e_util_icon_theme_set(o, "enlightenment");
          }
        else
          {
             if (!bd->internal_icon_key)
               {
                  char *ext;

                  ext = strrchr(bd->internal_icon, '.');
                  if ((ext) && ((!strcmp(ext, ".edj"))))
                    {
                       o = edje_object_add(evas);
                       if (!edje_object_file_set(o, bd->internal_icon, "icon"))
                         e_util_icon_theme_set(o, "enlightenment");
                    }
                  else if (ext)
                    {
                       o = e_icon_add(evas);
                       e_icon_file_set(o, bd->internal_icon);
                    }
                  else
                    {
                       o = e_icon_add(evas);
                       if (!e_util_icon_theme_set(o, bd->internal_icon))
                         e_util_icon_theme_set(o, "enlightenment");
                    }
               }
             else
               {
                  o = edje_object_add(evas);
                  edje_object_file_set(o, bd->internal_icon,
                                       bd->internal_icon_key);
               }
          }
        return o;
     }
   if ((e_config->use_app_icon) && (bd->icon_preference != E_ICON_PREF_USER))
     {
        if (bd->client.netwm.icons)
          {
             o = e_icon_add(evas);
             e_icon_data_set(o, bd->client.netwm.icons[0].data,
                             bd->client.netwm.icons[0].width,
                             bd->client.netwm.icons[0].height);
             e_icon_alpha_set(o, 1);
             return o;
          }
     }
   if (!o)
     {
        if ((bd->desktop) && (bd->icon_preference != E_ICON_PREF_NETWM))
          {
             o = e_icon_add(evas);
             if (o)
               {
                  e_icon_fdo_icon_set(o, bd->desktop->icon);
                  return o;
               }
          }
        else if (bd->client.netwm.icons)
          {
             o = e_icon_add(evas);
             e_icon_data_set(o, bd->client.netwm.icons[0].data,
                             bd->client.netwm.icons[0].width,
                             bd->client.netwm.icons[0].height);
             e_icon_alpha_set(o, 1);
             return o;
          }
     }

   o = e_icon_add(evas);
   e_util_icon_theme_set(o, "unknown");
   return o;
}

EAPI void
e_border_button_bindings_ungrab_all(void)
{
   Eina_List *l;
   E_Border *bd;

   EINA_LIST_FOREACH(borders, l, bd)
     {
        e_focus_setdown(bd);
        e_bindings_mouse_ungrab(E_BINDING_CONTEXT_WINDOW, bd->win);
        e_bindings_wheel_ungrab(E_BINDING_CONTEXT_WINDOW, bd->win);
     }
}

EAPI void
e_border_button_bindings_grab_all(void)
{
   Eina_List *l;
   E_Border *bd;

   EINA_LIST_FOREACH(borders, l, bd)
     {
        e_bindings_mouse_grab(E_BINDING_CONTEXT_WINDOW, bd->win);
        e_bindings_wheel_grab(E_BINDING_CONTEXT_WINDOW, bd->win);
        e_focus_setup(bd);
     }
}

EAPI Eina_List *
e_border_focus_stack_get(void)
{
   return focus_stack;
}

EAPI Eina_List *
e_border_raise_stack_get(void)
{
   return raise_stack;
}

EAPI Eina_List *
e_border_lost_windows_get(E_Zone *zone)
{
   Eina_List *list = NULL, *l;
   E_Border *bd;
   int loss_overlap = 5;

   E_OBJECT_CHECK_RETURN(zone, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, NULL);
   EINA_LIST_FOREACH(borders, l, bd)
     {
        if (!bd->zone)
          continue;

        if ((bd->zone != zone) ||
            (bd->zone->container != zone->container))
          continue;

        if (!E_INTERSECTS(bd->zone->x + loss_overlap,
                          bd->zone->y + loss_overlap,
                          bd->zone->w - (2 * loss_overlap),
                          bd->zone->h - (2 * loss_overlap),
                          bd->x, bd->y, bd->w, bd->h))
          {
             list = eina_list_append(list, bd);
          }
        else if ((!E_CONTAINS(bd->zone->x, bd->zone->y,
                              bd->zone->w, bd->zone->h,
                              bd->x, bd->y, bd->w, bd->h)) &&
                 (bd->shaped))
          {
             Ecore_X_Rectangle *rect;
             int i, num;

             rect = ecore_x_window_shape_rectangles_get(bd->win, &num);
             if (rect)
               {
                  int ok;

                  ok = 0;
                  for (i = 0; i < num; i++)
                    {
                       if (E_INTERSECTS(bd->zone->x + loss_overlap,
                                        bd->zone->y + loss_overlap,
                                        bd->zone->w - (2 * loss_overlap),
                                        bd->zone->h - (2 * loss_overlap),
                                        rect[i].x, rect[i].y,
                                        (int)rect[i].width, (int)rect[i].height))
                         {
                            ok = 1;
                            break;
                         }
                    }
                  free(rect);
                  if (!ok)
                    list = eina_list_append(list, bd);
               }
          }
     }
   return list;
}

EAPI void
e_border_ping(E_Border *bd)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (!e_config->ping_clients) return;
   bd->ping_ok = 0;
   ecore_x_netwm_ping_send(bd->client.win);
   bd->ping = ecore_loop_time_get();
   if (bd->ping_poller) ecore_poller_del(bd->ping_poller);
   bd->ping_poller = ecore_poller_add(ECORE_POLLER_CORE,
                                      e_config->ping_clients_interval,
                                      _e_border_cb_ping_poller, bd);
}

EAPI void
e_border_move_cancel(void)
{
   if (bdmove)
     {
        if (bdmove->cur_mouse_action)
          {
             E_Border *bd;

             bd = bdmove;
             e_object_ref(E_OBJECT(bd));
             if (bd->cur_mouse_action->func.end_mouse)
               bd->cur_mouse_action->func.end_mouse(E_OBJECT(bd), "", NULL);
             else if (bd->cur_mouse_action->func.end)
               bd->cur_mouse_action->func.end(E_OBJECT(bd), "");
             e_object_unref(E_OBJECT(bd->cur_mouse_action));
             bd->cur_mouse_action = NULL;
             e_object_unref(E_OBJECT(bd));
          }
        else
          _e_border_move_end(bdmove);
     }
}

EAPI void
e_border_resize_cancel(void)
{
   if (bdresize)
     {
        if (bdresize->cur_mouse_action)
          {
             E_Border *bd;

             bd = bdresize;
             e_object_ref(E_OBJECT(bd));
             if (bd->cur_mouse_action->func.end_mouse)
               bd->cur_mouse_action->func.end_mouse(E_OBJECT(bd), "", NULL);
             else if (bd->cur_mouse_action->func.end)
               bd->cur_mouse_action->func.end(E_OBJECT(bd), "");
             e_object_unref(E_OBJECT(bd->cur_mouse_action));
             bd->cur_mouse_action = NULL;
             e_object_unref(E_OBJECT(bd));
          }
        else
          {
             bdresize->resize_mode = RESIZE_NONE;
             _e_border_resize_end(bdresize);
          }
     }
}

EAPI void
e_border_frame_recalc(E_Border *bd)
{
   if (!bd->bg_object) return;

   bd->w -= (bd->client_inset.l + bd->client_inset.r);
   bd->h -= (bd->client_inset.t + bd->client_inset.b);

   _e_border_client_inset_calc(bd);

   bd->w += (bd->client_inset.l + bd->client_inset.r);
   bd->h += (bd->client_inset.t + bd->client_inset.b);

   bd->changed = 1;
   bd->changes.size = 1;
   if ((bd->shaped) || (bd->client.shaped))
     {
        bd->need_shape_merge = 1;
        bd->need_shape_export = 1;
     }
   if (bd->shaped_input)
     {
        bd->need_shape_merge = 1;
     }
   _e_border_client_move_resize_send(bd);
}

EAPI Eina_List *
e_border_immortal_windows_get(void)
{
   Eina_List *list = NULL, *l;
   E_Border *bd;

   EINA_LIST_FOREACH(borders, l, bd)
     {
        if (bd->lock_life)
          list = eina_list_append(list, bd);
     }
   return list;
}

EAPI const char *
e_border_name_get(const E_Border *bd)
{
   E_OBJECT_CHECK_RETURN(bd, "");
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, "");
   if (bd->client.netwm.name)
     return bd->client.netwm.name;
   else if (bd->client.icccm.title)
     return bd->client.icccm.title;
   return "";
}

EAPI void
e_border_signal_move_begin(E_Border       *bd,
                           const char     *sig,
                           const char *src __UNUSED__)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if ((bd->resize_mode != RESIZE_NONE) || (bd->moving)) return;
   if (!_e_border_move_begin(bd)) return;
   bd->moving = 1;
   _e_border_pointer_move_begin(bd);
   e_zone_edge_disable();
   _e_border_moveinfo_gather(bd, sig);
   if (bd->cur_mouse_action)
     {
        if ((!bd->cur_mouse_action->func.end_mouse) &&
            (!bd->cur_mouse_action->func.end))
          bd->cur_mouse_action = NULL;
        else
          e_object_unref(E_OBJECT(bd->cur_mouse_action));
     }
   bd->cur_mouse_action = e_action_find("window_move");
   if (bd->cur_mouse_action)
     e_object_ref(E_OBJECT(bd->cur_mouse_action));
}

EAPI void
e_border_signal_move_end(E_Border       *bd,
                         const char *sig __UNUSED__,
                         const char *src __UNUSED__)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (!bd->moving) return;
   bd->moving = 0;
   _e_border_pointer_move_end(bd);
   e_zone_edge_enable();
   _e_border_move_end(bd);
   e_zone_flip_coords_handle(bd->zone, -1, -1);
}

EAPI int
e_border_resizing_get(E_Border *bd)
{
   E_OBJECT_CHECK_RETURN(bd, 0);
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, 0);
   if (bd->resize_mode == RESIZE_NONE) return 0;
   return 1;
}

EAPI void
e_border_signal_resize_begin(E_Border       *bd,
                             const char     *dir,
                             const char     *sig,
                             const char *src __UNUSED__)
{
   Ecore_X_Gravity grav = ECORE_X_GRAVITY_NW;
   int resize_mode = RESIZE_BR;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if ((bd->resize_mode != RESIZE_NONE) || (bd->moving)) return;
   if (!_e_border_resize_begin(bd))
     return;
   if (!strcmp(dir, "tl"))
     {
        resize_mode = RESIZE_TL;
        grav = ECORE_X_GRAVITY_SE;
     }
   else if (!strcmp(dir, "t"))
     {
        resize_mode = RESIZE_T;
        grav = ECORE_X_GRAVITY_S;
     }
   else if (!strcmp(dir, "tr"))
     {
        resize_mode = RESIZE_TR;
        grav = ECORE_X_GRAVITY_SW;
     }
   else if (!strcmp(dir, "r"))
     {
        resize_mode = RESIZE_R;
        grav = ECORE_X_GRAVITY_W;
     }
   else if (!strcmp(dir, "br"))
     {
        resize_mode = RESIZE_BR;
        grav = ECORE_X_GRAVITY_NW;
     }
   else if (!strcmp(dir, "b"))
     {
        resize_mode = RESIZE_B;
        grav = ECORE_X_GRAVITY_N;
     }
   else if (!strcmp(dir, "bl"))
     {
        resize_mode = RESIZE_BL;
        grav = ECORE_X_GRAVITY_NE;
     }
   else if (!strcmp(dir, "l"))
     {
        resize_mode = RESIZE_L;
        grav = ECORE_X_GRAVITY_E;
     }
   bd->resize_mode = resize_mode;
   _e_border_pointer_resize_begin(bd);
   _e_border_moveinfo_gather(bd, sig);
   GRAV_SET(bd, grav);
   if (bd->cur_mouse_action)
     {
        if ((!bd->cur_mouse_action->func.end_mouse) &&
            (!bd->cur_mouse_action->func.end))
          bd->cur_mouse_action = NULL;
        else
          e_object_unref(E_OBJECT(bd->cur_mouse_action));
     }
   bd->cur_mouse_action = e_action_find("window_resize");
   if (bd->cur_mouse_action)
     e_object_ref(E_OBJECT(bd->cur_mouse_action));
}

EAPI void
e_border_signal_resize_end(E_Border       *bd,
                           const char *dir __UNUSED__,
                           const char *sig __UNUSED__,
                           const char *src __UNUSED__)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (bd->resize_mode == RESIZE_NONE) return;
   _e_border_resize_handle(bd);
   _e_border_pointer_resize_end(bd);
   bd->resize_mode = RESIZE_NONE;
   _e_border_resize_end(bd);
   bd->changes.reset_gravity = 1;
   bd->changed = 1;
}

EAPI void
e_border_resize_limit(E_Border *bd,
                      int      *w,
                      int      *h)
{
   double a;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   *w -= bd->client_inset.l + bd->client_inset.r;
   *h -= bd->client_inset.t + bd->client_inset.b;
   if (*h < 1) *h = 1;
   if (*w < 1) *w = 1;
   if ((bd->client.icccm.base_w >= 0) &&
       (bd->client.icccm.base_h >= 0))
     {
        int tw, th;

        tw = *w - bd->client.icccm.base_w;
        th = *h - bd->client.icccm.base_h;
        if (tw < 1) tw = 1;
        if (th < 1) th = 1;
        a = (double)(tw) / (double)(th);
        if ((bd->client.icccm.min_aspect != 0.0) &&
            (a < bd->client.icccm.min_aspect))
          {
             th = tw / bd->client.icccm.max_aspect;
             *h = th + bd->client.icccm.base_h;
          }
        else if ((bd->client.icccm.max_aspect != 0.0) &&
                 (a > bd->client.icccm.max_aspect))
          {
             tw = th * bd->client.icccm.max_aspect;
             *w = tw + bd->client.icccm.base_w;
          }
     }
   else
     {
        a = (double)*w / (double)*h;
        if ((bd->client.icccm.min_aspect != 0.0) &&
            (a < bd->client.icccm.min_aspect))
          *h = *w / bd->client.icccm.min_aspect;
        else if ((bd->client.icccm.max_aspect != 0.0) &&
                 (a > bd->client.icccm.max_aspect))
          *w = *h * bd->client.icccm.max_aspect;
     }
   if (bd->client.icccm.step_w > 0)
     {
        if (bd->client.icccm.base_w >= 0)
          *w = bd->client.icccm.base_w +
            (((*w - bd->client.icccm.base_w) / bd->client.icccm.step_w) *
             bd->client.icccm.step_w);
        else
          *w = bd->client.icccm.min_w +
            (((*w - bd->client.icccm.min_w) / bd->client.icccm.step_w) *
             bd->client.icccm.step_w);
     }
   if (bd->client.icccm.step_h > 0)
     {
        if (bd->client.icccm.base_h >= 0)
          *h = bd->client.icccm.base_h +
            (((*h - bd->client.icccm.base_h) / bd->client.icccm.step_h) *
             bd->client.icccm.step_h);
        else
          *h = bd->client.icccm.min_h +
            (((*h - bd->client.icccm.min_h) / bd->client.icccm.step_h) *
             bd->client.icccm.step_h);
     }

   if (*h < 1) *h = 1;
   if (*w < 1) *w = 1;

   if      (*w > bd->client.icccm.max_w) *w = bd->client.icccm.max_w;
   else if (*w < bd->client.icccm.min_w)
     *w = bd->client.icccm.min_w;
   if      (*h > bd->client.icccm.max_h) *h = bd->client.icccm.max_h;
   else if (*h < bd->client.icccm.min_h)
     *h = bd->client.icccm.min_h;

   *w += bd->client_inset.l + bd->client_inset.r;
   *h += bd->client_inset.t + bd->client_inset.b;
}

/* local subsystem functions */
static void
_e_border_free(E_Border *bd)
{
#ifdef _F_USE_DESK_WINDOW_PROFILE_
   if (bd->client.e.state.profile.use)
     {
        if (bd->client.e.state.profile.available_list)
          {
             int i;
             for (i = 0; i < bd->client.e.state.profile.num; i++)
               {
                  if (bd->client.e.state.profile.available_list[i])
                    {
                       eina_stringshare_del(bd->client.e.state.profile.available_list[i]);
                       bd->client.e.state.profile.available_list[i] = NULL;
                    }
               }
             E_FREE(bd->client.e.state.profile.available_list);
             bd->client.e.state.profile.available_list = NULL;
          }

        bd->client.e.state.profile.num = 0;

        if (bd->client.e.state.profile.name)
          {
             eina_stringshare_del(bd->client.e.state.profile.name);
             bd->client.e.state.profile.name = NULL;
          }

        bd->client.e.state.profile.wait_for_done = 0;
        bd->client.e.state.profile.use = 0;
     }
#endif
   if (bd->client.e.state.video_parent && bd->client.e.state.video_parent_border)
     {
        bd->client.e.state.video_parent_border->client.e.state.video_child =
          eina_list_remove
          (bd->client.e.state.video_parent_border->client.e.state.video_child,
              bd);
     }
   if (bd->client.e.state.video_child)
     {
        E_Border *tmp;

        EINA_LIST_FREE(bd->client.e.state.video_child, tmp)
          {
             tmp->client.e.state.video_parent_border = NULL;
          }
     }
#ifndef _F_DISABLE_E_EFREET_
   if (bd->desktop)
     {
        efreet_desktop_free(bd->desktop);
        bd->desktop = NULL;
     }
#endif
   if (bd->post_job)
     {
        ecore_idle_enterer_del(bd->post_job);
        bd->post_job = NULL;
     }
   if (bd->pointer)
     {
        e_object_del(E_OBJECT(bd->pointer));
        bd->pointer = NULL;
     }
   if (bdresize == bd)
     _e_border_resize_end(bd);
   if (bdmove == bd)
     _e_border_move_end(bd);
   /* TODO: Other states to end before dying? */

   if (bd->cur_mouse_action)
     {
        e_object_unref(E_OBJECT(bd->cur_mouse_action));
        bd->cur_mouse_action = NULL;
     }

   E_FREE(bd->shape_rects);
   bd->shape_rects_num = 0;
/*
   if (bd->dangling_ref_check)
     {
        ecore_timer_del(bd->dangling_ref_check);
        bd->dangling_ref_check = NULL;
     }
 */
   if (bd->kill_timer)
     {
        ecore_timer_del(bd->kill_timer);
        bd->kill_timer = NULL;
     }
   if (bd->ping_poller)
     {
        ecore_poller_del(bd->ping_poller);
        bd->ping_poller = NULL;
     }
   E_FREE_LIST(bd->pending_move_resize, free);

   if (bd->shade.anim) ecore_animator_del(bd->shade.anim);
   if (bd->border_menu) e_menu_deactivate(bd->border_menu);

   if (bd->border_locks_dialog)
     {
        e_object_del(E_OBJECT(bd->border_locks_dialog));
        bd->border_locks_dialog = NULL;
     }
   if (bd->border_remember_dialog)
     {
        e_object_del(E_OBJECT(bd->border_remember_dialog));
        bd->border_remember_dialog = NULL;
     }
   if (bd->border_border_dialog)
     {
        e_object_del(E_OBJECT(bd->border_border_dialog));
        bd->border_border_dialog = NULL;
     }
   if (bd->border_prop_dialog)
     {
        e_object_del(E_OBJECT(bd->border_prop_dialog));
        bd->border_prop_dialog = NULL;
     }

#ifndef _F_DISABLE_E_MENU
   e_int_border_menu_del(bd);
#endif

   if (focusing == bd)
     focusing = NULL;

   focus_next = eina_list_remove(focus_next, bd);

   if ((focused == bd) ||
       (e_grabinput_last_focus_win_get() == bd->client.win))
     {
        if ((!focus_next) && (!focusing))
          {
             e_grabinput_focus(bd->zone->container->bg_win,
                               E_FOCUS_METHOD_PASSIVE);
             e_hints_active_window_set(bd->zone->container->manager, NULL);
          }

        focused = NULL;
     }
   E_FREE_LIST(bd->handlers, ecore_event_handler_del);
   if (bd->remember)
     {
        E_Remember *rem;

        rem = bd->remember;
        bd->remember = NULL;
        e_remember_unuse(rem);
     }
   if (!bd->already_unparented)
     {
        ecore_x_window_reparent(bd->client.win, bd->zone->container->manager->root,
                                bd->x + bd->client_inset.l, bd->y + bd->client_inset.t);
        ecore_x_window_save_set_del(bd->client.win);
        bd->already_unparented = 1;
     }
   if (bd->group) eina_list_free(bd->group);
   if (bd->transients) eina_list_free(bd->transients);
   if (bd->stick_desks) eina_list_free(bd->stick_desks);
   if (bd->client.netwm.icons)
     {
        int i;
        for (i = 0; i < bd->client.netwm.num_icons; i++)
          free(bd->client.netwm.icons[i].data);
        free(bd->client.netwm.icons);
     }
   if (bd->client.netwm.extra_types)
     free(bd->client.netwm.extra_types);
   if (bd->client.border.name)
     eina_stringshare_del(bd->client.border.name);
   if (bd->bordername)
     eina_stringshare_del(bd->bordername);
   if (bd->client.icccm.name)
     eina_stringshare_del(bd->client.icccm.name);
   if (bd->client.icccm.class)
     {
        if (!strcmp(bd->client.icccm.class, "Vmplayer"))
          e_bindings_mapping_change_enable(EINA_TRUE);
        eina_stringshare_del(bd->client.icccm.class);
     }
   if (bd->client.icccm.title)
     eina_stringshare_del(bd->client.icccm.title);
   if (bd->client.icccm.icon_name)
     eina_stringshare_del(bd->client.icccm.icon_name);
   if (bd->client.icccm.machine)
     eina_stringshare_del(bd->client.icccm.machine);
   if (bd->client.icccm.window_role)
     eina_stringshare_del(bd->client.icccm.window_role);

   if ((bd->client.icccm.command.argc > 0) && (bd->client.icccm.command.argv))
     {
        int i;

        for (i = 0; i < bd->client.icccm.command.argc; i++)
          free(bd->client.icccm.command.argv[i]);
        free(bd->client.icccm.command.argv);
     }
   if (bd->client.netwm.name)
     eina_stringshare_del(bd->client.netwm.name);
   if (bd->client.netwm.icon_name)
     eina_stringshare_del(bd->client.netwm.icon_name);
   e_object_del(E_OBJECT(bd->shape));
   if (bd->internal_icon) eina_stringshare_del(bd->internal_icon);
   if (bd->internal_icon_key) eina_stringshare_del(bd->internal_icon_key);
   if (bd->icon_object) evas_object_del(bd->icon_object);
#ifdef _F_ZONE_WINDOW_ROTATION_
   if (e_config->wm_win_rotation)
     {
        Eina_Bool rm_vkbd_parent = EINA_FALSE;

        bd->client.e.fetch.rot.app_set = 0;
        bd->client.e.state.rot.preferred_rot = -1;

        if (bd->client.e.state.rot.available_rots)
          E_FREE(bd->client.e.state.rot.available_rots);

        _e_border_rotation_list_remove(bd);
        if ((rot.vkbd) && (rot.vkbd == bd))
          {
             ELB(ELBT_BD, "UNSET VKBD", bd->client.win);
             rot.vkbd = NULL;
             if (rot.vkbd_ctrl_win)
               {
                  ELB(ELBT_BD, "SET KBD_OFF", 0);
                  ecore_x_e_virtual_keyboard_state_set
                    (rot.vkbd_ctrl_win, ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF);
               }

             rot.vkbd_hide_prepare_done = EINA_FALSE;
             if (rot.vkbd_hide_prepare_timer)
               ecore_timer_del(rot.vkbd_hide_prepare_timer);
             rot.vkbd_hide_prepare_timer = NULL;
             if (rot.vkbd_hide_timer)
               ecore_timer_del(rot.vkbd_hide_timer);
             rot.vkbd_hide_timer = NULL;

             rot.vkbd_show_prepare_done = EINA_FALSE;
             if (rot.vkbd_show_prepare_timer)
               ecore_timer_del(rot.vkbd_show_prepare_timer);
             rot.vkbd_show_prepare_timer = NULL;
             if (rot.vkbd_show_timer)
               ecore_timer_del(rot.vkbd_show_timer);
             rot.vkbd_show_timer = NULL;
             if ((rot.vkbd_parent) && (!rot.vkbd_prediction))
               rm_vkbd_parent = EINA_TRUE;
          }
        else if ((rot.vkbd_prediction) &&
                 (rot.vkbd_prediction == bd))
          {
             rot.vkbd_prediction = NULL;
             if ((rot.vkbd_parent) && (!rot.vkbd))
               rm_vkbd_parent = EINA_TRUE;
          }
        else if ((rot.vkbd_parent) &&
                 (rot.vkbd_parent == bd))
          rm_vkbd_parent = EINA_TRUE;

        if (rm_vkbd_parent)
          {
             ELB(ELBT_BD, "RESET VKBD_PARENT DATA", bd->client.win);
             rot.vkbd_parent = NULL;
          }
     }
#endif
#ifdef _F_E_WIN_AUX_HINT_
   if (e_config->win_aux_hint)
     {
        E_Border_Aux_Hint *hint;
        EINA_LIST_FREE(bd->client.e.state.aux_hint.hints, hint)
          {
             eina_stringshare_del(hint->hint);
             eina_stringshare_del(hint->val);
             E_FREE(hint);
          }
     }
#endif /* end of _F_E_WIN_AUX_HINT_ */
   evas_object_del(bd->bg_object);
   e_canvas_del(bd->bg_ecore_evas);
   ecore_evas_free(bd->bg_ecore_evas);
   ecore_x_window_free(bd->client.shell_win);
   e_focus_setdown(bd);
   e_bindings_mouse_ungrab(E_BINDING_CONTEXT_WINDOW, bd->win);
   e_bindings_wheel_ungrab(E_BINDING_CONTEXT_WINDOW, bd->win);
   ecore_x_window_free(bd->win);

   eina_hash_del(borders_hash, e_util_winid_str_get(bd->client.win), bd);
   eina_hash_del(borders_hash, e_util_winid_str_get(bd->bg_win), bd);
   eina_hash_del(borders_hash, e_util_winid_str_get(bd->win), bd);
   borders = eina_list_remove(borders, bd);
   focus_stack = eina_list_remove(focus_stack, bd);
   raise_stack = eina_list_remove(raise_stack, bd);

   e_container_border_remove(bd);
   free(bd);
   bd = NULL;
}

/*
   static int
   _e_border_del_dangling_ref_check(void *data)
   {
   E_Border *bd;

   bd = data;
   printf("---\n");
   printf("EEK EEK border still around 1 second after being deleted!\n");
   printf("%p, %i, \"%s\" [\"%s\" \"%s\"]\n",
          bd, e_object_ref_get(E_OBJECT(bd)), bd->client.icccm.title,
          bd->client.icccm.name, bd->client.icccm.class);
   //   e_object_breadcrumb_debug(E_OBJECT(bd));
   printf("---\n");
   return 1;
   }
 */

static void
_e_border_del(E_Border *bd)
{
   E_Event_Border_Remove *ev;
   E_Border *child;

#ifdef _F_BORDER_HOOK_PATCH_
   _e_border_hook_call(E_BORDER_HOOK_DEL_BORDER, bd);
#endif

#ifdef _F_ZONE_WINDOW_ROTATION_
   if (bd->changed)
     {
        if ((e_config->wm_win_rotation) &&
            (rot.vkbd) &&
            (bd == rot.vkbd))
          {
             bd->changed = 0;
          }
     }
#endif

#ifdef _F_MOVE_RESIZE_SEND_PATCH_
   if (bd->client.icccm.move_resize_send.need_send)
     {
        ecore_x_window_move_resize(bd->client.win,
                                   bd->client.icccm.move_resize_send.x,
                                   bd->client.icccm.move_resize_send.y,
                                   bd->client.icccm.move_resize_send.w,
                                   bd->client.icccm.move_resize_send.h);

        ecore_x_icccm_move_resize_send(bd->client.win,
                                       bd->client.icccm.move_resize_send.x,
                                       bd->client.icccm.move_resize_send.y,
                                       bd->client.icccm.move_resize_send.w,
                                       bd->client.icccm.move_resize_send.h);
        bd->client.icccm.move_resize_send.need_send = 0;
     }
#endif

   if (bd == focused)
     {
        focused = NULL;
     }

   if (bd == focusing)
     focusing = NULL;

   focus_next = eina_list_remove(focus_next, bd);

   if (bd->fullscreen) bd->desk->fullscreen_borders--;

   if ((drag_border) && (drag_border->data == bd))
     {
        e_object_del(E_OBJECT(drag_border));
        drag_border = NULL;
     }
   if (bd->border_menu) e_menu_deactivate(bd->border_menu);

   if (bd->border_locks_dialog)
     {
        e_object_del(E_OBJECT(bd->border_locks_dialog));
        bd->border_locks_dialog = NULL;
     }
   if (bd->border_remember_dialog)
     {
        e_object_del(E_OBJECT(bd->border_remember_dialog));
        bd->border_remember_dialog = NULL;
     }
   if (bd->border_border_dialog)
     {
        e_object_del(E_OBJECT(bd->border_border_dialog));
        bd->border_border_dialog = NULL;
     }
   if (bd->border_prop_dialog)
     {
        e_object_del(E_OBJECT(bd->border_prop_dialog));
        bd->border_prop_dialog = NULL;
     }

#ifndef _F_DISABLE_E_MENU
  e_int_border_menu_del(bd);
#endif
   if (bd->raise_timer)
     {
        ecore_timer_del(bd->raise_timer);
        bd->raise_timer = NULL;
     }
   if (!bd->already_unparented)
     {
        ecore_x_window_reparent(bd->client.win,
                                bd->zone->container->manager->root,
                                bd->x + bd->client_inset.l,
                                bd->y + bd->client_inset.t);
        ecore_x_window_save_set_del(bd->client.win);
        bd->already_unparented = 1;
//	bd->client.win = 0;
     }
   bd->already_unparented = 1;

   if ((!bd->new_client) && (!stopping))
     {
        ev = E_NEW(E_Event_Border_Remove, 1);
        if (ev)
          {
             ev->border = bd;
             e_object_ref(E_OBJECT(bd));
             // e_object_breadcrumb_add(E_OBJECT(bd), "border_remove_event");
             ecore_event_add(E_EVENT_BORDER_REMOVE, ev, _e_border_event_border_remove_free, NULL);
          }
     }

   if (bd->parent)
     {
        bd->parent->transients = eina_list_remove(bd->parent->transients, bd);
        if ((bd->parent->modal == bd) &&
            (bd->parent->saved.event_mask))
          {
             ecore_x_event_mask_unset(bd->parent->client.win, ECORE_X_EVENT_MASK_WINDOW_DAMAGE | ECORE_X_EVENT_MASK_WINDOW_PROPERTY);
             ecore_x_event_mask_set(bd->parent->client.win, bd->parent->saved.event_mask);
             bd->parent->lock_close = 0;
             bd->parent->saved.event_mask = 0;
             bd->parent->modal = NULL;
          }
        bd->parent = NULL;
     }
   EINA_LIST_FREE(bd->transients, child)
     {
        child->changes.stack = 1;
        child->parent = NULL;
     }

#ifdef _F_DEICONIFY_APPROVE_
   bd->client.e.state.deiconify_approve.render_done = 0;
   bd->client.e.state.deiconify_approve.render_only = 0;

   _e_border_uniconify_pending_list_remove(bd);

   E_Border *ancestor_bd;
   ancestor_bd = bd->client.e.state.deiconify_approve.ancestor;
   if ((ancestor_bd) &&
       (!e_object_is_del(E_OBJECT(ancestor_bd))))
     {
        ancestor_bd->client.e.state.deiconify_approve.req_list = eina_list_remove(ancestor_bd->client.e.state.deiconify_approve.req_list, bd);
        bd->client.e.state.deiconify_approve.ancestor = NULL;

        if ((ancestor_bd->client.e.state.deiconify_approve.req_list == NULL) &&
            (ancestor_bd->client.e.state.deiconify_approve.render_done))
          {
             if (ancestor_bd->client.e.state.deiconify_approve.wait_timer)
               {
                  ecore_timer_del(ancestor_bd->client.e.state.deiconify_approve.wait_timer);
                  ancestor_bd->client.e.state.deiconify_approve.wait_timer = NULL;
                  _e_border_uniconify_pending_list_remove(ancestor_bd);
               }
          }
     }

   if (bd->client.e.state.deiconify_approve.wait_timer)
     {
        ecore_timer_del(bd->client.e.state.deiconify_approve.wait_timer);
        bd->client.e.state.deiconify_approve.wait_timer = NULL;
     }

   if (bd->client.e.state.deiconify_approve.req_list)
     {
        EINA_LIST_FREE(bd->client.e.state.deiconify_approve.req_list, child)
          {
             child->client.e.state.deiconify_approve.render_done = 0;
             child->client.e.state.deiconify_approve.ancestor = NULL;
          }
     }

   if (bd->client.e.state.deiconify_approve.pending_list)
     {
        _e_border_window_pending_event_flush(bd);
     }
#endif

#ifdef _F_ZONE_WINDOW_ROTATION_
   _e_border_rotation_list_remove(bd);
   if (rot.async_list) rot.async_list = eina_list_remove(rot.async_list, bd);
#endif

   if (bd->leader)
     {
        bd->leader->group = eina_list_remove(bd->leader->group, bd);
        if (bd->leader->modal == bd)
          bd->leader->modal = NULL;
        bd->leader = NULL;
     }
   EINA_LIST_FREE(bd->group, child)
     {
        child->leader = NULL;
     }
}

#ifdef PRINT_LOTS_OF_DEBUG
static void
_e_border_print(E_Border   *bd,
                const char *func)
{
   if (!bd) return;

   DBG("*Window Info*"
          "\tPointer: %p\n"
          "\tName: %s\n"
          "\tTitle: %s\n"
          "\tBorderless: %s\n",
          bd, bd->client.icccm.name, bd->client.icccm.title,
          bd->borderless ? "TRUE" : "FALSE");
}

#endif

#ifdef _F_DEICONIFY_APPROVE_
static void
_e_border_window_show_request_event_free(void *data __UNUSED__,
                                         void *ev)
{
   Ecore_X_Event_Window_Show_Request *e;

   e = ev;
   E_FREE(e);
}
#endif

#ifdef _F_USE_VIRT_RESOLUTION_
static Eina_Bool _e_timer_unmute_plane()
{
   unsigned int prop;
   prop = 1;
   ecore_x_window_prop_card32_set(ecore_x_window_root_first_get(),
                                  E_ATOM_WM_PLANE_MUTE_REQUEST,&prop, 1);
   ecore_timer_del( mute_timer);
   mute_timer = NULL;
   ecore_x_sync();
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_mod_comp_resolution_change(int width, int height)
{
   int index = -1;
   int num_sizes = 0;
   int i = 0;

   int temp_w, temp_h;

   if (e_virtual_res_type_get() == E_VIRTUAL_RES_SINGLE_SOURCE)
   {
      E_Virtual_Res_Data* vir_res_data = e_virtual_res_data_get();
      if (vir_res_data && vir_res_data->preferred_resolution_get)
        vir_res_data->preferred_resolution_get(&temp_w, &temp_h);
      else
        ecore_x_screen_size_get(ecore_x_default_screen_get(), &temp_w, &temp_h);
   }
   else
     ecore_x_screen_size_get(ecore_x_default_screen_get(), &temp_w, &temp_h);

   if( (temp_w == width) && (temp_h == height))
     {
        return EINA_TRUE;
     }

   Ecore_X_Window root = ecore_x_window_root_first_get();
   Ecore_X_Randr_Screen_Size_MM *xrrs =
      ecore_x_randr_screen_primary_output_sizes_get(root,&num_sizes);

   if (xrrs)
     {
        for(i = 0; i < num_sizes; i++)
          if ( xrrs[i].width == width &&  xrrs[i].height == height)
            index = i;
        E_FREE(xrrs);
        if (index >= 0)
          {
             unsigned int prop;
             prop = 0;
             ecore_x_sync();
             ecore_x_window_prop_card32_set(ecore_x_window_root_first_get(),
                                            E_ATOM_WM_PLANE_MUTE_REQUEST,&prop, 1);
             ecore_x_sync();

             if(mute_timer)
               {
                  ecore_timer_delay(mute_timer,PLANE_MUTE_DELAY_TIMER);
               }
             else
               {
                  mute_timer = ecore_timer_add(PLANE_MUTE_TIMER, _e_timer_unmute_plane, NULL);
               }
             if (ecore_x_randr_screen_primary_output_size_set(root, index) == EINA_TRUE)
               {
                  ecore_x_sync();
                  return EINA_TRUE;
               }
          }
     }
   return EINA_FALSE;
}

static Eina_Bool
_e_mod_revert_resolution( Ecore_X_Window win)
{
   const Eina_List *l;
   void *list_data;
   unsigned int *del_data = NULL ;

   if (res_windows_list)
     {
        EINA_LIST_FOREACH(res_windows_list, l, list_data)
          {
             if ( win == *((unsigned int *)list_data))
               {
                  del_data  = ( unsigned int *)list_data ;
               }
          }
        if( del_data )
          {
             _e_mod_comp_resolution_change(pref_w, pref_h);
             res_windows_list = eina_list_remove(res_windows_list, del_data);
             if (del_data)
               {
                  free(del_data);
                  del_data = NULL;
               }
          }
     }
   return EINA_TRUE ;
}

static Eina_Bool
_e_mod_check_change_resolution( Ecore_X_Window win)
{
   unsigned int prop_data[2];
   int num, width, height;
   Eina_Bool ret = EINA_FALSE ;
   Ecore_X_Display *dpy;
   dpy = ecore_x_display_get();

   int temp_w = 0;
   int temp_h = 0;

   if (e_virtual_res_type_get() == E_VIRTUAL_RES_SINGLE_SOURCE)
     {
        E_Virtual_Res_Data* vir_res_data = e_virtual_res_data_get();
        if (vir_res_data && vir_res_data->preferred_resolution_get)
          vir_res_data->preferred_resolution_get(&temp_w, &temp_h);
        else
          ecore_x_screen_size_get(ecore_x_default_screen_get(), &temp_w, &temp_h);
     }

   else
     ecore_x_screen_size_get(ecore_x_default_screen_get(), &temp_w, &temp_h);

   if ( ecore_x_window_prop_card32_get(win, E_ATOM_WM_APP_RESOLUTION , prop_data, 2) > 0)
     {
        width = prop_data[0] ;
        height = prop_data[1] ;
        if( (temp_w == width) && (temp_h == height))
          {
             return EINA_TRUE ;
          }
        pref_w = temp_w;
        pref_h = temp_h;

        Ecore_X_Window highlight_win;
        int revert_to_return;
        unsigned int prop = 0;
        XGetInputFocus(dpy, &highlight_win, &revert_to_return);

        if (ecore_x_window_prop_card32_get(highlight_win, E_ATOM_WM_SUPPORT_EPOP , &prop, 1) > 0)
          {
             if(prop)
               {
                  ret = EINA_TRUE;
               }
          }
        if(EINA_FALSE == ret)
          {
             return EINA_FALSE;
          }

        if( EINA_TRUE == _e_mod_comp_resolution_change(width, height))
          {
             const Eina_List *l;
             void *list_data;
             if (res_windows_list)
               {
                  EINA_LIST_FOREACH(res_windows_list, l, list_data)
                    {
                       if ( win == *((unsigned int *)list_data))
                         {
                            return EINA_TRUE;
                         }
                    }
               }
             unsigned int *win_data;
             win_data  = (unsigned int *)malloc(sizeof(unsigned int));
             if( win_data )
               {
                  *win_data = win;
                  res_windows_list = eina_list_append(res_windows_list, win_data);
               }
          }
     }
   else
     {
        if( (temp_w != pref_w) || (temp_h != pref_h))
          {
             const Eina_List *l;
             void *list_data;
             if (res_windows_list)
               {
                  EINA_LIST_FOREACH(res_windows_list, l, list_data)
                    {
                       int win_temp = *((int*)list_data);
                       E_Border *bd = e_border_find_by_client_window(win_temp);
                       if(bd)
                         {
                            e_border_hide(bd,0);
                         }
                       ecore_x_window_hide(win_temp);
                       _e_mod_revert_resolution(win_temp);
                       ecore_x_sync();
                    }
               }
          }
     }
   return EINA_TRUE ;
}
#endif /* end of _F_USE_VIRT_RESOLUTION_ */

static Eina_Bool
_e_border_cb_window_show_request(void *data  __UNUSED__,
                                 int ev_type __UNUSED__,
                                 void       *ev)
{
   E_Border *bd;
   E_Container *con;
   Ecore_X_Event_Window_Show_Request *e;

   e = ev;
#ifdef _F_USE_VIRT_RESOLUTION_
   if ( EINA_FALSE == _e_mod_check_change_resolution(e->win))
     {
        return ECORE_CALLBACK_CANCEL;
     }
#endif /* end of _F_USE_VIRT_RESOLUTION_ */
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return ECORE_CALLBACK_PASS_ON;

#ifdef _F_DEICONIFY_APPROVE_
   if (bd->client.e.state.pending_event.pending)
     {
        Eina_List *l, *ln;
        E_Border *hold_bd = NULL;

        EINA_LIST_FOREACH_SAFE(bd->client.e.state.pending_event.wait_for_list, l, ln, hold_bd)
          {
             hold_bd->client.e.state.deiconify_approve.pending_list = eina_list_remove(hold_bd->client.e.state.deiconify_approve.pending_list, bd);
             ELBF(ELBT_BD, 0, hold_bd->client.win, "Pending win:0x%07x", bd->client.win);
             _e_border_window_pending_event_send(bd, hold_bd);
          }

        Ecore_X_Event_Window_Show_Request *new_e;
        new_e = E_NEW(Ecore_X_Event_Window_Show_Request, 1);
        if (new_e)
          {
             new_e->win = e->win;
             new_e->parent = e->parent;
             new_e->time = ecore_x_current_time_get();

             ecore_event_add(ECORE_X_EVENT_WINDOW_SHOW_REQUEST, new_e,
                             _e_border_window_show_request_event_free, NULL);

             ELB(ELBT_BD, "Send pended SHOW event", e->win);
          }

        return ECORE_CALLBACK_CANCEL;
     }
#endif

   if ((e_config->wm_win_rotation) &&
       (rot.vkbd_ctrl_win) && (rot.vkbd) &&
       (bd == rot.vkbd) &&
       (rot.vkbd_hide_prepare_timer))
     {
        con = bd->zone->container;
        bd = e_border_new(con, e->win, 0, 0);
     }

   if (bd)
     {
        if (bd->iconic)
          {
             if (!bd->lock_client_iconify)
               e_border_uniconify(bd);
          }
        else
          {
             /* FIXME: make border "urgent" for a bit - it wants attention */
             /*	e_border_show(bd); */
             if (!bd->lock_client_stacking)
               e_border_raise(bd);
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_window_destroy(void *data  __UNUSED__,
                            int ev_type __UNUSED__,
                            void       *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Destroy *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return ECORE_CALLBACK_PASS_ON;
   ELB(ELBT_BD, "X_WIN_DEL", bd->client.win);
#ifdef _F_ZONE_WINDOW_ROTATION_
   if (e_config->wm_win_rotation)
     {
        if (bd->client.vkbd.win_type == E_VIRTUAL_KEYBOARD_WINDOW_TYPE_KEYPAD)
          {
             ELB(ELBT_BD, "X_DEL_NOTIFY", bd->client.win);
             if (!rot.vkbd_hide_prepare_timer)
               {
                  ELB(ELBT_BD, "HIDE VKBD", bd->client.win);
                  e_border_hide(bd, 0);
                  if (!rot.vkbd_hide_prepare_timer)
                    {
                       ELB(ELBT_BD, "DEL VKBD", bd->client.win);
                       e_object_del(E_OBJECT(bd));
                    }
               }
             return ECORE_CALLBACK_PASS_ON;
          }
     }
#endif

#ifdef _F_DEICONIFY_APPROVE_
   if (e_config->deiconify_approve)
     {
        if (bd->client.e.state.pending_event.pending)
          {
             bd->client.e.state.pending_event.destroy.pending = 1;
             bd->client.e.state.pending_event.destroy.win = e->win;
             bd->client.e.state.pending_event.destroy.event_win = e->event_win;
             ELBF(ELBT_BD, 0, bd->client.win, "PENDING destroy event. win:%x, event_win:%x", e->win, e->event_win);
             return ECORE_CALLBACK_CANCEL;
          }
     }

   bd->client.e.state.pending_event.destroy.pending = 0;
   ELB(ELBT_BD, "Real Destroy", bd->client.win);
#endif

#ifdef _F_USE_TILED_DESK_LAYOUT_
   if (e_config->use_tiled_desk_layout)
     {
        if (bd->client.e.state.ly.curr_ly)
          {
             /* If bd was splited.. then reset curr_ly. */
             e_border_desk_layout_set(bd, NULL, EINA_FALSE, EINA_TRUE);
          }
     }
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

   e_border_hide(bd, 0);
   e_object_del(E_OBJECT(bd));
   return ECORE_CALLBACK_PASS_ON;
}

#ifdef _F_DEICONIFY_APPROVE_
static Eina_List *
_e_border_find_below_win_list(E_Border* bd)
{
   E_Border *temp_bd;
   Ecore_X_XRegion *visible_region = NULL;
   Ecore_X_XRegion *win_region = NULL;
   Ecore_X_Rectangle visible_rect, win_rect;

   Eina_Bool passed = EINA_FALSE;
   E_Border *parent = NULL;
   Eina_Bool skip;
   Eina_List *below_list = NULL;

   int temp_bd_x, temp_bd_y, temp_bd_w, temp_bd_h;

#ifdef _F_USE_TILED_DESK_LAYOUT_
   Eina_Bool splited;
   if (bd->client.e.state.ly.curr_ly)
      splited = EINA_TRUE;
   else
      splited = EINA_FALSE;
#endif

   // set the current zone's region
   visible_rect.x = bd->zone->x;
   visible_rect.y = bd->zone->y;
   visible_rect.width = bd->zone->w;
   visible_rect.height = bd->zone->h;

   visible_region = ecore_x_xregion_new();
   if (!visible_region) return NULL;

   ecore_x_xregion_union_rect(visible_region, visible_region, &visible_rect);

   E_Border_List *bl;
   bl = e_container_border_list_last(bd->zone->container);
   if (bl)
     {
        while ((temp_bd = e_container_border_list_prev(bl)))
          {
             skip = EINA_FALSE;
             /* skip if it's the same border */
             if (temp_bd == bd)
               {
                  passed = EINA_TRUE;
                  continue;
               }

             if (e_object_is_del(E_OBJECT(temp_bd))) continue;

             /* skip if it's not on this zone */
             if (temp_bd->zone != bd->zone) continue;

             e_border_geometry_get(temp_bd, &temp_bd_x, &temp_bd_y, &temp_bd_w, &temp_bd_h);

             if (E_CONTAINS(temp_bd_x, temp_bd_y, temp_bd_w, temp_bd_h, bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h))
               {
                  if (!temp_bd->client.argb && temp_bd->visible)
                    {
                       break;
                    }
               }

             win_rect.x = temp_bd_x;
             win_rect.y = temp_bd_y;
             win_rect.width = temp_bd_w;
             win_rect.height = temp_bd_h;

             // if it stick out or is bigger than the entire visible region,
             // clip it by the entire visible's geometry.
             E_RECTS_CLIP_TO_RECT(win_rect.x, win_rect.y,
                                  win_rect.width, win_rect.height,
                                  visible_rect.x, visible_rect.y,
                                  (int)(visible_rect.width), (int)(visible_rect.height));

             if (ecore_x_xregion_rect_contain(visible_region, &win_rect))
               {

                  if (temp_bd->client.illume.win_state.state !=
                      ECORE_X_ILLUME_WINDOW_STATE_NORMAL)
                    {
                       skip = EINA_TRUE;
                    }

                  if (temp_bd->internal) skip = EINA_TRUE;
                  if (temp_bd->iconify_by_client) skip = EINA_TRUE;

                  if (!skip &&
                      !temp_bd->visible &&
                      passed &&
                      ecore_x_window_visible_get(temp_bd->client.win))
                    {
#ifdef _F_USE_TILED_DESK_LAYOUT_
                       if (splited)
                         {
                            if (temp_bd->client.e.state.ly.curr_ly)
                              {
                                 if (!temp_bd->client.e.state.deiconify_approve.wait_timer)
                                   temp_bd->client.e.state.deiconify_approve.not_raise = 1;
                                 below_list = eina_list_append(below_list, temp_bd);
                              }
                         }
                       else
                         {
                            if (!temp_bd->client.e.state.deiconify_approve.wait_timer)
                              temp_bd->client.e.state.deiconify_approve.not_raise = 1;
                            below_list = eina_list_append(below_list, temp_bd);
                         }

#else
                       if (!temp_bd->client.e.state.deiconify_approve.wait_timer)
                         temp_bd->client.e.state.deiconify_approve.not_raise = 1;
                       below_list = eina_list_append(below_list, temp_bd);
#endif
                    }

                  if ((!skip) && (!temp_bd->client.argb))
                    {
                       win_region = ecore_x_xregion_new();
                       if (win_region)
                         {
                            ecore_x_xregion_union_rect(win_region, win_region, &win_rect);
                            ecore_x_xregion_subtract(visible_region, visible_region, win_region);
                            ecore_x_xregion_free(win_region);
                            win_region = NULL;

                            if (ecore_x_xregion_is_empty(visible_region))
                              {
                                 if (temp_bd->parent)
                                   {
                                      parent = temp_bd->parent;
                                      while (parent)
                                        {
                                           if (!parent->client.e.state.deiconify_approve.wait_timer)
                                             parent->client.e.state.deiconify_approve.not_raise = 1;
                                           below_list = eina_list_append(below_list, parent);

                                           parent = parent->parent;
                                        }
                                   }

                                 break;
                              }
                         }
                    }
               }
          }
        e_container_border_list_free(bl);
     }
   if (visible_region) ecore_x_xregion_free(visible_region);
   return below_list;
}
#endif

static Eina_Bool
_e_border_cb_window_hide(void *data  __UNUSED__,
                         int ev_type __UNUSED__,
                         void       *ev)
{
   E_Border *bd = NULL;
   Ecore_X_Event_Window_Hide *e;

   e = ev;
//   printf("HIDE: %x, event %x send: %i\n", e->win, e->event_win, e->send_event);
// not interested in hide events from windows other than the window in question
#ifdef _F_USE_VIRT_RESOLUTION_
   _e_mod_revert_resolution( e->win);
#endif /* end of _F_USE_VIRT_RESOLUTION_ */
   if (e->win != e->event_win)
     {
        bd = e_border_find_by_client_window(e->win);
        if (!bd) return ECORE_CALLBACK_PASS_ON;
        if (!e->send_event) return ECORE_CALLBACK_PASS_ON;
        else
          {
             if (!((bd->zone) &&
                   (bd->zone->container->manager->root == e->event_win)))
               return ECORE_CALLBACK_PASS_ON;
          }
     }
   if (!bd) bd = e_border_find_by_client_window(e->win);
//   printf("  bd = %p\n", bd);
   if (!bd)
     {
        if (ecore_x_window_visible_get(e->win))
          {
             ELB(ELBT_BD, "FORCE UNMAP client window", e->win);
             ecore_x_window_hide(e->win);
          }
        return ECORE_CALLBACK_PASS_ON;
     }

#ifdef _F_DEICONIFY_APPROVE_
   if (e_config->deiconify_approve)
     {
        int temp_bd_x, temp_bd_y, temp_bd_w, temp_bd_h;

        if (bd->client.e.state.pending_event.done)
           goto do_hide;

        if (bd->client.e.state.pending_event.hide.pending)
          {
             ELB(ELBT_BD, "Already hide PENDING.", bd->client.win);
             return ECORE_CALLBACK_CANCEL;
          }

        if (bd->client.e.state.pending_event.pending)
          {
             bd->client.e.state.pending_event.hide.pending = 1;
             bd->client.e.state.pending_event.hide.win = e->win;
             bd->client.e.state.pending_event.hide.event_win = e->event_win;
             bd->client.e.state.pending_event.hide.send_event = e->send_event;
             ELB(ELBT_BD, "PENDING hide event. Pending by another event.", bd->client.win);
             return ECORE_CALLBACK_CANCEL;
          }

        e_border_geometry_get(bd, &temp_bd_x, &temp_bd_y, &temp_bd_w, &temp_bd_h);

#ifdef _F_USE_TILED_DESK_LAYOUT_
        if (e_config->use_tiled_desk_layout)
          {
             if (!bd->client.e.state.ly.curr_ly)
               {
                  if (!E_CONTAINS(temp_bd_x, temp_bd_y, temp_bd_w, temp_bd_h, bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h))
                    goto do_hide;
               }
             else
               {
                  if (!E_CONTAINS(bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h, temp_bd_x, temp_bd_y, temp_bd_w, temp_bd_h))
                    goto do_hide;
               }
          }
        else
          {
             if (!E_CONTAINS(temp_bd_x, temp_bd_y, temp_bd_w, temp_bd_h, bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h))
               goto do_hide;
          }
#else
        if (!E_CONTAINS(temp_bd_x, temp_bd_y, temp_bd_w, temp_bd_h, bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h))
           goto do_hide;
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

        Eina_List *below_list = NULL;
        // 1. find below non-alpha full size win

        if (bd->visible)
           below_list = _e_border_find_below_win_list(bd);

        if (below_list)
          {
             bd->client.e.state.pending_event.pending = 1;

             Eina_List *l;
             E_Border *below_bd;

             bd->client.e.state.pending_event.hide.pending = 1;
             bd->client.e.state.pending_event.hide.win = e->win;
             bd->client.e.state.pending_event.hide.event_win = e->event_win;
             bd->client.e.state.pending_event.hide.send_event = e->send_event;

             EINA_LIST_REVERSE_FOREACH(below_list, l, below_bd)
               {
                  bd->client.e.state.pending_event.wait_for_list =
                     eina_list_append(bd->client.e.state.pending_event.wait_for_list, below_bd);

                  below_bd->client.e.state.deiconify_approve.pending_list = eina_list_append(below_bd->client.e.state.deiconify_approve.pending_list, bd);
                  ELBF(ELBT_ROT, 0, bd->client.win, "below_win:%x, below_win's wait_bd:%x", below_bd->client.win, bd->client.win);

                  // 3. if not same then uniconify
                  if ((!below_bd->client.e.state.deiconify_approve.wait_timer) &&
                      (!below_bd->client.e.state.deiconify_approve.pending))
                    {
                       below_bd->client.e.state.deiconify_approve.not_raise = 1;
                       e_border_uniconify(below_bd);
                    }
               }
             eina_list_free(below_list);

             ELB(ELBT_BD, "PENDING hide event", bd->client.win);
             return ECORE_CALLBACK_CANCEL;
          }
     }

do_hide:
   bd->client.e.state.pending_event.hide.pending = 0;

#ifdef _F_USE_ICONIFY_RESIZE_
   if (e_config->use_iconify_resize)
     {
        if (bd->iconify_resize.support)
          {
             if (bd->iconify_resize.set)
               {
                  bd->iconify_resize.set = 0;
                  e_border_move_resize(bd, bd->iconify_resize.x, bd->iconify_resize.y, bd->iconify_resize.w, bd->iconify_resize.h);
                  ELBF(ELBT_BD, 0, bd->client.win,"X Hide: Restore size");
#ifdef _F_USE_BORDER_TRANSFORM_
                  e_border_transform_update(bd);
#endif
               }
          }
     }
#endif

   if ((!bd->client.e.state.pending_event.hide.pending) &&
       (!bd->client.e.state.pending_event.lower.pending) &&
       (!bd->client.e.state.pending_event.iconify.pending))
     {
        bd->client.e.state.pending_event.done = 0;
        bd->client.e.state.pending_event.pending = 0;
     }

   ELB(ELBT_BD, "Real hide", bd->client.win);
#endif



//   printf("  bd->ignore_first_unmap = %i\n", bd->ignore_first_unmap);
   if (bd->ignore_first_unmap > 0)
     {
        bd->ignore_first_unmap--;
        return ECORE_CALLBACK_PASS_ON;
     }
   /* Don't delete hidden or iconified windows */
#ifdef _F_USE_EXTENDED_ICONIFY_
   if (bd->await_hide_event > 0)
#else
   if ((bd->iconic) || (bd->await_hide_event > 0))
#endif
     {
//        printf("  Don't delete hidden or iconified windows\n");
//        printf("  bd->iconic = %i, bd->visible = %i, bd->new_client = %i, bd->await_hide_event = %i\n",
//               bd->iconic, bd->visible, bd->new_client, bd->await_hide_event);
        if (bd->await_hide_event > 0)
          {
             bd->await_hide_event--;
          }
        else
          {
//             printf("  hide really\n");
             /* Only hide the border if it is visible */
             if (bd->visible)
               {
#ifdef _F_USE_TILED_DESK_LAYOUT_
                  if (e_config->use_tiled_desk_layout)
                    {
                       if (bd->client.e.state.ly.curr_ly)
                         {
                            /* If bd was splited.. then reset curr_ly. */
                            e_border_desk_layout_set(bd, NULL, EINA_FALSE, EINA_TRUE);
                         }
                    }
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
                  e_border_hide(bd, 1);
               }
          }
     }
   else
     {
//             printf("  hide2\n");
#ifdef _F_USE_EXTENDED_ICONIFY_
        if (bd->iconic)
          {
             bd->iconic = 0;
             bd->visible = 1;
          }
#endif

#ifdef _F_ZONE_WINDOW_ROTATION_
        if (e_config->wm_win_rotation)
          {
             if (bd->client.vkbd.win_type == E_VIRTUAL_KEYBOARD_WINDOW_TYPE_KEYPAD)
               {
                  ELB(ELBT_BD, "X_UNMAP_NOTIFY", bd->client.win);
                  if (!rot.vkbd_hide_prepare_timer)
                    {
                       ELB(ELBT_BD, "HIDE VKBD", bd->client.win);
                       e_border_hide(bd, 0);
                       if (!rot.vkbd_hide_prepare_timer)
                         {
                            ELB(ELBT_BD, "DEL VKBD", bd->client.win);
                            e_object_del(E_OBJECT(bd));
                         }
                    }
                  return ECORE_CALLBACK_PASS_ON;
               }
          }
#endif

#ifdef _F_USE_TILED_DESK_LAYOUT_
        if (e_config->use_tiled_desk_layout)
          {
             if (bd->client.e.state.ly.curr_ly)
               {
                  /* If bd was splited.. then reset curr_ly. */
                  e_border_desk_layout_set(bd, NULL, EINA_FALSE, EINA_TRUE);
               }
          }
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

        e_border_hide(bd, 0);
        e_object_del(E_OBJECT(bd));
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_window_reparent(void *data  __UNUSED__,
                             int ev_type __UNUSED__,
                             void *ev    __UNUSED__)
{
#if 0
   E_Border *bd;
   Ecore_X_Event_Window_Reparent *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return 1;
   if (e->parent == bd->client.shell_win) return 1;
   if (ecore_x_window_parent_get(e->win) == bd->client.shell_win)
     {
        return 1;
     }
   e_border_hide(bd, 0);
   e_object_del(E_OBJECT(bd));
#endif
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_window_configure_request(void *data  __UNUSED__,
                                      int ev_type __UNUSED__,
                                      void       *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Configure_Request *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd)
     {
        if (e_stolen_win_get(e->win)) return ECORE_CALLBACK_PASS_ON;
        if (!e_util_container_window_find(e->win))
          ecore_x_window_configure(e->win, e->value_mask,
                                   e->x, e->y, e->w, e->h, e->border,
                                   e->abovewin, e->detail);
        return ECORE_CALLBACK_PASS_ON;
     }
   traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:BORDER:WIN CONFIGURE REQ");

   SLOGI("CONFIG_REQ win:0x%08x, value_mask:%lu, x,y,w,h:(%d,%d,%d,%d), above:0x%08x, detail:%d",
             e->win, e->value_mask, e->x, e->y, e->w, e->h, e->border, e->abovewin, e->detail);

#ifdef _F_USE_VIRT_RESOLUTION_
   int temp_w = 0;
   int temp_h = 0;
   int prefer_w = 0;
   int prefer_h = 0;

   E_Virtual_Res_Data* vir_res_data = e_virtual_res_data_get();
   if (vir_res_data && vir_res_data->preferred_resolution_get)
     vir_res_data->preferred_resolution_get(&prefer_w, &prefer_h);

   if ((e_virtual_res_type_get() == E_VIRTUAL_RES_SINGLE_SOURCE) && ((e->w > prefer_w) || (e->h > prefer_h)))
     {
        if ((e->w > prefer_w) && (e->h > prefer_h))
          {
             temp_w = prefer_w ;
             temp_h = prefer_h ;
          }
        else if (e->w > prefer_w)
          {
             temp_w = prefer_w ;
             temp_h = e->h ;
          }
        else if (e->h > prefer_h)
          {
             temp_w = e->w ;
             temp_h = prefer_h ;
          }
     }
   else if ((e_virtual_res_type_get() != E_VIRTUAL_RES_SINGLE_SOURCE) && prefer_w && prefer_h)
     {
        int zoneX,zoneY,zoneW,zoneH ;
        e_zone_useful_geometry_get(bd->zone, &zoneX, &zoneY, &zoneW, &zoneH);
        if ((e->w > zoneW) && (e->h > zoneH))
          {
             temp_w = prefer_w ;
             temp_h = prefer_h ;
          }
        else if (e->w > zoneW)
          {
             temp_w = prefer_w ;
             temp_h = e->h ;
          }
        else if (e->h > zoneH)
          {
             temp_w = e->w ;
             temp_h = prefer_h ;
          }
        else
          {
             temp_w = e->w ;
             temp_h = e->h ;
          }
     }
   else
     {
        temp_w = e->w ;
        temp_h = e->h ;
     }
#endif /* end of _F_USE_VIRT_RESOLUTION_ */

   if ((e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_X) ||
       (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_Y))
     {
        int x, y;

        x = bd->x;
        y = bd->y;
        if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_X)
          x = e->x;
        if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_Y)
          y = e->y;
        if ((e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_W) ||
            (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_H))
          {
             int w, h;

             h = bd->h;
             w = bd->w;

#ifdef _F_USE_VIRT_RESOLUTION_
             if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_W)
               w = temp_w + bd->client_inset.l + bd->client_inset.r;
             if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_H)
               h = temp_h + bd->client_inset.t + bd->client_inset.b;
#else
             if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_W)
               w = e->w + bd->client_inset.l + bd->client_inset.r;
             if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_H)
               h = e->h + bd->client_inset.t + bd->client_inset.b;
#endif /* end of _F_USE_VIRT_RESOLUTION_ */

             if ((!bd->lock_client_location) && (!bd->lock_client_size))
               {
                  if ((bd->maximized & E_MAXIMIZE_TYPE) != E_MAXIMIZE_NONE)
                    {
                       bd->saved.x = x - bd->zone->x;
                       bd->saved.y = y - bd->zone->y;
                       bd->saved.w = w;
                       bd->saved.h = h;
                    }
                  else
                    e_border_move_resize(bd, x, y, w, h);
               }
             else if (!bd->lock_client_location)
               {
                  if ((bd->maximized & E_MAXIMIZE_TYPE) != E_MAXIMIZE_NONE)
                    {
                       bd->saved.x = x - bd->zone->x;
                       bd->saved.y = y - bd->zone->y;
                    }
                  else
                    e_border_move(bd, x, y);
               }
             else if (!bd->lock_client_size)
               {
                  if ((bd->shaded) || (bd->shading))
                    {
                       int pw, ph;

                       pw = bd->client.w;
                       ph = bd->client.h;
                       if ((bd->shade.dir == E_DIRECTION_UP) ||
                           (bd->shade.dir == E_DIRECTION_DOWN))
                         {
                            e_border_resize(bd, w, bd->h);
                            bd->client.h = ph;
                         }
                       else
                         {
                            e_border_resize(bd, bd->w, h);
                            bd->client.w = pw;
                         }
                    }
                  else
                    {
                       if ((bd->maximized & E_MAXIMIZE_TYPE) != E_MAXIMIZE_NONE)
                         {
                            bd->saved.w = w;
                            bd->saved.h = h;
                         }
                       else
                         e_border_resize(bd, w, h);
                    }
               }
          }
        else
          {
             if (!bd->lock_client_location)
               {
                  if ((bd->maximized & E_MAXIMIZE_TYPE) != E_MAXIMIZE_NONE)
                    {
                       bd->saved.x = x - bd->zone->x;
                       bd->saved.y = y - bd->zone->y;
                    }
                  else
                    e_border_move(bd, x, y);
               }
          }
     }
   else if ((e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_W) ||
            (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_H))
     {
        int w, h;

        h = bd->h;
        w = bd->w;
#ifdef _F_USE_TV_FEATURE_
        if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_W)
          w = temp_w + bd->client_inset.l + bd->client_inset.r;
        if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_H)
          h = temp_h + bd->client_inset.t + bd->client_inset.b;
#else
        if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_W)
          w = e->w + bd->client_inset.l + bd->client_inset.r;
        if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_H)
          h = e->h + bd->client_inset.t + bd->client_inset.b;
#endif /* end of _F_USE_TV_FEATURE_ */
        if (!bd->lock_client_size)
          {
             if ((bd->shaded) || (bd->shading))
               {
                  int pw, ph;

                  pw = bd->client.w;
                  ph = bd->client.h;
                  if ((bd->shade.dir == E_DIRECTION_UP) ||
                      (bd->shade.dir == E_DIRECTION_DOWN))
                    {
                       e_border_resize(bd, w, bd->h);
                       bd->client.h = ph;
                    }
                  else
                    {
                       e_border_resize(bd, bd->w, h);
                       bd->client.w = pw;
                    }
               }
             else
               {
                  if ((bd->maximized & E_MAXIMIZE_TYPE) == E_MAXIMIZE_NONE)
                    {
                       int zx, zy, zw, zh;
                       int rx = bd->x;
                       int ry = bd->y;
                       zx = zy = zw = zh = 0;

     /*
      * This code does resize and move a window on a
      * X configure request into an useful geometry.
      * This is really useful for size jumping file dialogs.
      */

                       if (bd->zone)
                         {
                            e_zone_useful_geometry_get(bd->zone, &zx, &zy, &zw, &zh);

                            if (e_config->geometry_auto_resize_limit == 1)
                              {
                                 if (w > zw)
                                   w = zw;

                                 if (h > zh)
                                   h = zh;
                              }
                         }
                       e_border_resize(bd, w, h);

                       if (e_config->geometry_auto_move == 1)
                         {
                             /* z{x,y,w,h} are only set here; FIXME! */
                             if (bd->zone)
                               {
                                  // move window horizontal if resize to not useful geometry
                                  if (bd->x + bd->w > zx + zw)
                                    rx = zx + zw - bd->w;
                                  else if (bd->x < zx)
                                    rx = zx;

                                  // move window vertical if resize to not useful geometry
                                  if (bd->y + bd->h > zy + zh)
                                    ry = zy + zh - bd->h;
                                  else if (bd->y < zy)
                                    ry = zy;
                               }
                             e_border_move(bd, rx, ry);
                         }
                    }
               }
          }
     }
   if (!bd->lock_client_stacking)
     {
        if ((e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE) &&
            (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING))
          {
             E_Border *obd;

             if (e->detail == ECORE_X_WINDOW_STACK_ABOVE)
               {
                  obd = e_border_find_by_client_window(e->abovewin);
                  if (obd)
                    {
                       e_border_stack_above(bd, obd);
                    }
                  else
                    {
                       ecore_x_window_configure(bd->win,
                                                ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
                                                ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
                                                0, 0, 0, 0, 0,
                                                e->abovewin, ECORE_X_WINDOW_STACK_ABOVE);
                       /* FIXME: need to rebuiuld border list from current stacking */
                    }
               }
             else if (e->detail == ECORE_X_WINDOW_STACK_BELOW)
               {
                  obd = e_border_find_by_client_window(e->abovewin);
                  if (obd)
                    {
                       e_border_stack_below(bd, obd);
                    }
                  else
                    {
                       ecore_x_window_configure(bd->win,
                                                ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
                                                ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
                                                0, 0, 0, 0, 0,
                                                e->abovewin, ECORE_X_WINDOW_STACK_BELOW);
                       /* FIXME: need to rebuiuld border list from current stacking */
                    }
               }
             else if (e->detail == ECORE_X_WINDOW_STACK_TOP_IF)
               {
     /* FIXME: do */
               }
             else if (e->detail == ECORE_X_WINDOW_STACK_BOTTOM_IF)
               {
     /* FIXME: do */
               }
             else if (e->detail == ECORE_X_WINDOW_STACK_OPPOSITE)
               {
     /* FIXME: do */
               }
          }
        else if (e->value_mask & ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE)
          {
             if (e->detail == ECORE_X_WINDOW_STACK_ABOVE)
               {
                  if (bd->new_client)
                    {
                       ELBF(ELBT_BD, 0, bd->client.win, "Set post_raise");
                       bd->post_raise = 1;
                       bd->post_lower = 0;
                    }
                  else
                    e_border_raise(bd);
               }
             else if (e->detail == ECORE_X_WINDOW_STACK_BELOW)
               {
#ifdef _F_DEICONIFY_APPROVE_
                  if (!e->abovewin)
                    {
                       if (e_config->deiconify_approve)
                         {
                            int bx, by, bw, bh;
                            e_border_geometry_get(bd, &bx, &by, &bw, &bh);

                            if (bd->client.e.state.pending_event.done)
                               goto do_lower;

                            if (bd->client.e.state.pending_event.lower.pending)
                              {
                                 ELB(ELBT_BD, "Already lower PENDING.", bd->client.win);
                                 traceEnd(TTRACE_TAG_WINDOW_MANAGER);
                                 return ECORE_CALLBACK_CANCEL;
                              }

                            if (bd->client.e.state.pending_event.pending)
                              {
                                 bd->client.e.state.pending_event.lower.pending = 1;
                                 bd->client.e.state.pending_event.lower.win = e->win;
                                 bd->client.e.state.pending_event.lower.parent_win = e->parent_win;
                                 bd->client.e.state.pending_event.lower.above_win = e->abovewin;
                                 bd->client.e.state.pending_event.lower.detail = e->detail;
                                 bd->client.e.state.pending_event.lower.value_mask = ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE;
                                 ELB(ELBT_BD, "PENDING lower event. Pending by another event", bd->client.win);
                                 traceEnd(TTRACE_TAG_WINDOW_MANAGER);
                                 return ECORE_CALLBACK_CANCEL;
                              }

#ifdef _F_USE_TILED_DESK_LAYOUT_
                            if (e_config->use_tiled_desk_layout)
                              {
                                 if (!bd->client.e.state.ly.curr_ly)
                                   {
                                      if (!E_CONTAINS(bx, by, bw, bh, bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h))
                                         goto do_lower;
                                   }
                                 else
                                   {
                                      if (!E_CONTAINS(bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h, bx, by, bw, bh))
                                         goto do_lower;

                                   }
                              }
                            else
                              {
                                 if (!E_CONTAINS(bx, by, bw, bh, bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h))
                                    goto do_lower;
                              }
#else
                            if (!E_CONTAINS(bx, by, bw, bh, bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h))
                               goto do_lower;
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

                            Eina_List *below_list;
                            // 1. find below non-alpha full size win
                            below_list = _e_border_find_below_win_list(bd);

                            if (below_list)
                              {
                                 bd->client.e.state.pending_event.pending = 1;

                                 Eina_List *l;
                                 E_Border *below_bd;

                                 bd->client.e.state.pending_event.lower.pending = 1;
                                 bd->client.e.state.pending_event.lower.win = e->win;
                                 bd->client.e.state.pending_event.lower.parent_win = e->parent_win;
                                 bd->client.e.state.pending_event.lower.above_win = e->abovewin;
                                 bd->client.e.state.pending_event.lower.detail = e->detail;
                                 bd->client.e.state.pending_event.lower.value_mask = ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE;

                                 EINA_LIST_REVERSE_FOREACH(below_list, l, below_bd)
                                   {
                                      bd->client.e.state.pending_event.wait_for_list =
                                         eina_list_append(bd->client.e.state.pending_event.wait_for_list, below_bd);

                                      below_bd->client.e.state.deiconify_approve.pending_list = eina_list_append(below_bd->client.e.state.deiconify_approve.pending_list, bd);
                                      ELBF(ELBT_ROT, 0, bd->client.win, "below_win:%x, below_win's wait_bd:%x", below_bd->client.win, bd->client.win);

                                      // 3. if not same then uniconify
                                      if (!below_bd->client.e.state.deiconify_approve.wait_timer)
                                        below_bd->client.e.state.deiconify_approve.not_raise = 1;
                                      e_border_uniconify(below_bd);
                                   }
                                 eina_list_free(below_list);

                                 ELB(ELBT_BD, "PENDING lower event", bd->client.win);
                                 traceEnd(TTRACE_TAG_WINDOW_MANAGER);
                                 return ECORE_CALLBACK_CANCEL;
                              }
                         }

do_lower:
                       bd->client.e.state.pending_event.lower.pending = 0;

                       if ((!bd->client.e.state.pending_event.hide.pending) &&
                           (!bd->client.e.state.pending_event.lower.pending) &&
                           (!bd->client.e.state.pending_event.iconify.pending))
                         {
                            bd->client.e.state.pending_event.done = 0;
                            bd->client.e.state.pending_event.pending = 0;
                         }
                    }
#endif

#ifdef _F_USE_TILED_DESK_LAYOUT_
                  if (e_config->use_tiled_desk_layout)
                    {
                       if (bd->client.e.state.ly.curr_ly)
                         {
                            /* If bd was splited.. then reset curr_ly. */
                            e_border_desk_layout_set(bd, NULL, EINA_FALSE, EINA_TRUE);
                         }
                    }
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

                  if (bd->new_client || bd->client.icccm.fetch.transient_for)
                    {
                       ELBF(ELBT_BD, 0, bd->client.win, "Set post_lower, new_client:%d, fetch transient_for:%d",
                            bd->new_client, bd->client.icccm.fetch.transient_for);
                       bd->post_lower = 1;
                       bd->post_raise = 0;
                    }
                  else
                    {
                       ELB(ELBT_BD, "Real lower", bd->client.win);
                       e_border_lower(bd);
                    }
               }
             else if (e->detail == ECORE_X_WINDOW_STACK_TOP_IF)
               {
     /* FIXME: do */
               }
             else if (e->detail == ECORE_X_WINDOW_STACK_BOTTOM_IF)
               {
     /* FIXME: do */
               }
             else if (e->detail == ECORE_X_WINDOW_STACK_OPPOSITE)
               {
     /* FIXME: do */
               }
          }
     }

   /* FIXME: need to send synthetic stacking event too as well as move/resize */
   _e_border_client_move_resize_send(bd);
   traceEnd(TTRACE_TAG_WINDOW_MANAGER);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_window_resize_request(void *data  __UNUSED__,
                                   int ev_type __UNUSED__,
                                   void       *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Resize_Request *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd)
     {
        if (e_stolen_win_get(e->win)) return ECORE_CALLBACK_PASS_ON;
        ecore_x_window_resize(e->win, e->w, e->h);
        return ECORE_CALLBACK_PASS_ON;
     }
   {
      int w, h;

      w = e->w + bd->client_inset.l + bd->client_inset.r;
      h = e->h + bd->client_inset.t + bd->client_inset.b;
      if ((bd->shaded) || (bd->shading))
        {
           int pw, ph;

           pw = bd->client.w;
           ph = bd->client.h;
           if ((bd->shade.dir == E_DIRECTION_UP) ||
               (bd->shade.dir == E_DIRECTION_DOWN))
             {
                e_border_resize(bd, w, bd->h);
                bd->client.h = ph;
             }
           else
             {
                e_border_resize(bd, bd->w, h);
                bd->client.w = pw;
             }
        }
      else
        e_border_resize(bd, w, h);
   }

   _e_border_client_move_resize_send(bd);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_window_gravity(void *data  __UNUSED__,
                            int ev_type __UNUSED__,
                            void *ev    __UNUSED__)
{
//   E_Border *bd;
//   Ecore_X_Event_Window_Gravity *e;

//   e = ev;
//   bd = e_border_find_by_client_window(e->win);
//   if (!bd) return 1;
             return 1;
}

static Eina_Bool
_e_border_cb_window_stack_request(void *data  __UNUSED__,
                                  int ev_type __UNUSED__,
                                  void       *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Stack_Request *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd)
     {
        if (e_stolen_win_get(e->win)) return ECORE_CALLBACK_PASS_ON;
        if (!e_util_container_window_find(e->win))
          {
             if (e->detail == ECORE_X_WINDOW_STACK_ABOVE)
               ecore_x_window_raise(e->win);
             else if (e->detail == ECORE_X_WINDOW_STACK_BELOW)
               ecore_x_window_lower(e->win);
          }
        return ECORE_CALLBACK_PASS_ON;
     }
   if (e->detail == ECORE_X_WINDOW_STACK_ABOVE)
     e_border_raise(bd);
   else if (e->detail == ECORE_X_WINDOW_STACK_BELOW)
     e_border_lower(bd);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_window_property(void *data  __UNUSED__,
                             int ev_type __UNUSED__,
                             void       *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Property *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return ECORE_CALLBACK_PASS_ON;
   if (e->atom == ECORE_X_ATOM_WM_NAME)
     {
        if ((!bd->client.netwm.name) &&
            (!bd->client.netwm.fetch.name))
          {
             bd->client.icccm.fetch.title = 1;
             bd->changed = 1;
          }
     }
   else if (e->atom == ECORE_X_ATOM_NET_WM_NAME)
     {
        bd->client.netwm.fetch.name = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_WM_CLASS)
     {
        bd->client.icccm.fetch.name_class = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_WM_ICON_NAME)
     {
        if ((!bd->client.netwm.icon_name) &&
            (!bd->client.netwm.fetch.icon_name))
          {
             bd->client.icccm.fetch.icon_name = 1;
             bd->changed = 1;
          }
     }
   else if (e->atom == ECORE_X_ATOM_NET_WM_ICON_NAME)
     {
        bd->client.netwm.fetch.icon_name = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_WM_CLIENT_MACHINE)
     {
        bd->client.icccm.fetch.machine = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_WM_PROTOCOLS)
     {
        bd->client.icccm.fetch.protocol = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_WM_HINTS)
     {
        bd->client.icccm.fetch.hints = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_WM_NORMAL_HINTS)
     {
        bd->client.icccm.fetch.size_pos_hints = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_MOTIF_WM_HINTS)
     {
        /*
           if ((bd->client.netwm.type == ECORE_X_WINDOW_TYPE_UNKNOWN) &&
            (!bd->client.netwm.fetch.type))
           {
         */
          bd->client.mwm.fetch.hints = 1;
          bd->changed = 1;
          /*
             }
           */
     }
   else if (e->atom == ECORE_X_ATOM_WM_TRANSIENT_FOR)
     {
        bd->client.icccm.fetch.transient_for = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_WM_CLIENT_LEADER)
     {
        bd->client.icccm.fetch.client_leader = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_WM_WINDOW_ROLE)
     {
        bd->client.icccm.fetch.window_role = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_NET_WM_ICON)
     {
        bd->client.netwm.fetch.icon = 1;
        bd->changed = 1;
     }
   else if (e->atom == ATM__QTOPIA_SOFT_MENU)
     {
        bd->client.qtopia.fetch.soft_menu = 1;
        bd->changed = 1;
     }
   else if (e->atom == ATM__QTOPIA_SOFT_MENUS)
     {
        bd->client.qtopia.fetch.soft_menus = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE)
     {
        bd->client.vkbd.fetch.state = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD)
     {
        bd->client.vkbd.fetch.vkbd = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_E_ILLUME_CONFORMANT)
     {
        bd->client.illume.conformant.fetch.conformant = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE)
     {
        bd->client.illume.quickpanel.fetch.state = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_E_ILLUME_QUICKPANEL)
     {
        bd->client.illume.quickpanel.fetch.quickpanel = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_PRIORITY_MAJOR)
     {
        bd->client.illume.quickpanel.fetch.priority.major = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_PRIORITY_MINOR)
     {
        bd->client.illume.quickpanel.fetch.priority.minor = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_ZONE)
     {
        bd->client.illume.quickpanel.fetch.zone = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_E_ILLUME_DRAG_LOCKED)
     {
        bd->client.illume.drag.fetch.locked = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_E_ILLUME_DRAG)
     {
        bd->client.illume.drag.fetch.drag = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_E_ILLUME_WINDOW_STATE)
     {
        bd->client.illume.win_state.fetch.state = 1;
        bd->changed = 1;
     }
   /*
      else if (e->atom == ECORE_X_ATOM_NET_WM_USER_TIME)
      {
        bd->client.netwm.fetch.user_time = 1;
        bd->changed = 1;
      }
      else if (e->atom == ECORE_X_ATOM_NET_WM_STRUT)
      {
        bd->client.netwm.fetch.strut = 1;
        bd->changed = 1;
      }
      else if (e->atom == ECORE_X_ATOM_NET_WM_STRUT_PARTIAL)
      {
        bd->client.netwm.fetch.strut = 1;
        bd->changed = 1;
      }
    */
   else if (e->atom == ECORE_X_ATOM_NET_WM_SYNC_REQUEST_COUNTER)
     {
        //printf("ECORE_X_ATOM_NET_WM_SYNC_REQUEST_COUNTER\n");
     }
   else if (e->atom == ECORE_X_ATOM_E_VIDEO_POSITION)
     {
        bd->client.e.fetch.video_position = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_E_VIDEO_PARENT)
     {
        bd->client.e.fetch.video_parent = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_NET_WM_STATE)
     {
        bd->client.netwm.fetch.state = 1;
        bd->changed = 1;
     }
#ifdef _F_USE_DESK_WINDOW_PROFILE_
   else if (e->atom == ECORE_X_ATOM_E_WINDOW_PROFILE_SUPPORTED)
     {
        bd->client.e.fetch.profile = 1;
        bd->changed = 1;
     }
   else if (e->atom == ECORE_X_ATOM_E_WINDOW_PROFILE_AVAILABLE_LIST)
     {
        bd->client.e.fetch.profile = 1;
        bd->changed = 1;
     }
#endif
#ifdef _F_ZONE_WINDOW_ROTATION_
   else if (e->atom == ECORE_X_ATOM_E_WINDOW_ROTATION_SUPPORTED)
     {
        if (e_config->wm_win_rotation)
          {
             bd->client.e.fetch.rot.support = 1;
             bd->changed = 1;
          }
     }
   else if ((e->atom == ECORE_X_ATOM_E_WINDOW_ROTATION_0_GEOMETRY) ||
            (e->atom == ECORE_X_ATOM_E_WINDOW_ROTATION_90_GEOMETRY) ||
            (e->atom == ECORE_X_ATOM_E_WINDOW_ROTATION_180_GEOMETRY) ||
            (e->atom == ECORE_X_ATOM_E_WINDOW_ROTATION_270_GEOMETRY))
     {
        if (e_config->wm_win_rotation)
          {
             bd->client.e.fetch.rot.geom_hint = 1;
             bd->changed = 1;
          }
     }
   else if (e->atom == ECORE_X_ATOM_E_WINDOW_ROTATION_APP_SUPPORTED)
     {
        if (e_config->wm_win_rotation)
          {
             bd->client.e.fetch.rot.app_set = 1;
             bd->changed = 1;
          }
     }
   else if (e->atom == ECORE_X_ATOM_E_WINDOW_ROTATION_PREFERRED_ROTATION)
     {
        if (e_config->wm_win_rotation)
          {
             bd->client.e.fetch.rot.preferred_rot = 1;
             bd->changed = 1;
          }
     }
   else if (e->atom == ECORE_X_ATOM_E_WINDOW_ROTATION_AVAILABLE_LIST)
     {
        if (e_config->wm_win_rotation)
          {
             bd->client.e.fetch.rot.available_rots = 1;
             bd->changed = 1;
          }
     }
#endif
#ifdef _F_USE_TILED_DESK_LAYOUT_
   else if (e->atom == E_ATOM_WINDOW_DESKTOP_LAYOUT_SUPPORTED)
     {
        if (e_config->use_tiled_desk_layout)
          {
             bd->client.e.fetch.ly.support = 1;
             bd->changed = 1;
          }
     }
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
#ifdef _F_E_WIN_AUX_HINT_
   else if (e->atom == ECORE_X_ATOM_E_WINDOW_AUX_HINT_SUPPORT)
     {
        if (e_config->win_aux_hint)
          {
             bd->client.e.fetch.aux_hint.support = 1;
             bd->changed = 1;
          }
     }
   else if (e->atom == ECORE_X_ATOM_E_WINDOW_AUX_HINT)
     {
        if (e_config->win_aux_hint)
          {
             bd->client.e.fetch.aux_hint.hints = 1;
             bd->changed = 1;
          }
     }
#endif /* end of _F_E_WIN_AUX_HINT_ */
#ifdef _F_DEICONIFY_APPROVE_
   else if (e->atom == ECORE_X_ATOM_E_DEICONIFY_APPROVE)
     {
        int ret;
        unsigned int val = 0;

        ret = ecore_x_window_prop_card32_get(bd->client.win,
                                             ECORE_X_ATOM_E_DEICONIFY_APPROVE,
                                             &val, 1);
        if (ret == 1)
          bd->client.e.state.deiconify_approve.support = val;
        else
          bd->client.e.state.deiconify_approve.support = 0;
     }
#endif

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_window_colormap(void *data  __UNUSED__,
                             int ev_type __UNUSED__,
                             void       *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Colormap *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return ECORE_CALLBACK_PASS_ON;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_window_shape(void *data  __UNUSED__,
                          int ev_type __UNUSED__,
                          void       *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Shape *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);

   if (e->type == ECORE_X_SHAPE_INPUT)
     {
        if (bd)
          {
             bd->need_shape_merge = 1;
// YYY             bd->shaped_input = 1;
             bd->changes.shape_input = 1;
             bd->changed = 1;
          }

        return ECORE_CALLBACK_PASS_ON;
     }

   if (bd)
     {
        bd->changes.shape = 1;
        bd->changed = 1;
        return ECORE_CALLBACK_PASS_ON;
     }
   bd = e_border_find_by_window(e->win);
   if (bd)
     {
        bd->need_shape_export = 1;
        bd->changed = 1;
        return ECORE_CALLBACK_PASS_ON;
     }
   bd = e_border_find_by_frame_window(e->win);
   if (bd)
     {
        bd->need_shape_merge = 1;
        bd->changed = 1;
        return ECORE_CALLBACK_PASS_ON;
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_window_focus_in(void *data  __UNUSED__,
                             int ev_type __UNUSED__,
                             void       *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Focus_In *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return ECORE_CALLBACK_PASS_ON;
#ifdef INOUTDEBUG_FOCUS
   {
      time_t t;
      char *ct;

      const char *modes[] = {
        "MODE_NORMAL",
        "MODE_WHILE_GRABBED",
        "MODE_GRAB",
        "MODE_UNGRAB"
      };
      const char *details[] = {
        "DETAIL_ANCESTOR",
        "DETAIL_VIRTUAL",
        "DETAIL_INFERIOR",
        "DETAIL_NON_LINEAR",
        "DETAIL_NON_LINEAR_VIRTUAL",
        "DETAIL_POINTER",
        "DETAIL_POINTER_ROOT",
        "DETAIL_DETAIL_NONE"
      };
      t = time(NULL);
      ct = ctime(&t);
      ct[strlen(ct) - 1] = 0;
      DBG("FF ->IN %i 0x%x %s md=%s dt=%s\n",
             e->time,
             e->win,
             ct,
             modes[e->mode],
             details[e->detail]);

      DBG("%s cb focus in %d %d\n",
             e_border_name_get(bd),
             bd->client.icccm.accepts_focus,
             bd->client.icccm.take_focus);
   }
#endif
   _e_border_pri_raise(bd);
   if (e->mode == ECORE_X_EVENT_MODE_GRAB)
     {
        if (e->detail == ECORE_X_EVENT_DETAIL_POINTER) return ECORE_CALLBACK_PASS_ON;
     }
   else if (e->mode == ECORE_X_EVENT_MODE_UNGRAB)
     {
        if (e->detail == ECORE_X_EVENT_DETAIL_POINTER) return ECORE_CALLBACK_PASS_ON;
     }

   /* ignore focus in from !take_focus windows, we just gave it em */
   /* if (!bd->client.icccm.take_focus)
    *   return ECORE_CALLBACK_PASS_ON; */

   /* should be equal, maybe some clients dont reply with the proper timestamp ? */
   if (e->time >= focus_time)
     e_border_focus_set(bd, 1, 0);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_window_focus_out(void *data  __UNUSED__,
                              int ev_type __UNUSED__,
                              void       *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Focus_Out *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return ECORE_CALLBACK_PASS_ON;
#ifdef INOUTDEBUG_FOCUS
   {
      time_t t;
      char *ct;

      const char *modes[] = {
        "MODE_NORMAL",
        "MODE_WHILE_GRABBED",
        "MODE_GRAB",
        "MODE_UNGRAB"
      };
      const char *details[] = {
        "DETAIL_ANCESTOR",
        "DETAIL_VIRTUAL",
        "DETAIL_INFERIOR",
        "DETAIL_NON_LINEAR",
        "DETAIL_NON_LINEAR_VIRTUAL",
        "DETAIL_POINTER",
        "DETAIL_POINTER_ROOT",
        "DETAIL_DETAIL_NONE"
      };
      t = time(NULL);
      ct = ctime(&t);
      ct[strlen(ct) - 1] = 0;
      DBG("FF <-OUT %i 0x%x %s md=%s dt=%s",
             e->time,
             e->win,
             ct,
             modes[e->mode],
             details[e->detail]);

      DBG("%s cb focus out %d %d",
             e_border_name_get(bd),
             bd->client.icccm.accepts_focus,
             bd->client.icccm.take_focus);
   }
#endif
   _e_border_pri_norm(bd);
   if (e->mode == ECORE_X_EVENT_MODE_NORMAL)
     {
        if (e->detail == ECORE_X_EVENT_DETAIL_INFERIOR) return ECORE_CALLBACK_PASS_ON;
        else if (e->detail == ECORE_X_EVENT_DETAIL_NON_LINEAR)
          return ECORE_CALLBACK_PASS_ON;
        else if (e->detail == ECORE_X_EVENT_DETAIL_NON_LINEAR_VIRTUAL)
          return ECORE_CALLBACK_PASS_ON;
     }
   else if (e->mode == ECORE_X_EVENT_MODE_GRAB)
     {
        if (e->detail == ECORE_X_EVENT_DETAIL_NON_LINEAR) return ECORE_CALLBACK_PASS_ON;
        else if (e->detail == ECORE_X_EVENT_DETAIL_INFERIOR)
          return ECORE_CALLBACK_PASS_ON;
        else if (e->detail == ECORE_X_EVENT_DETAIL_NON_LINEAR_VIRTUAL)
          return ECORE_CALLBACK_PASS_ON;
        else if (e->detail == ECORE_X_EVENT_DETAIL_ANCESTOR)
          return ECORE_CALLBACK_PASS_ON;
        else if (e->detail == ECORE_X_EVENT_DETAIL_VIRTUAL)
          return ECORE_CALLBACK_PASS_ON;
     }
   else if (e->mode == ECORE_X_EVENT_MODE_UNGRAB)
     {
        /* for firefox/thunderbird (xul) menu walking */
        /* NB: why did i disable this before? */
        if (e->detail == ECORE_X_EVENT_DETAIL_INFERIOR) return ECORE_CALLBACK_PASS_ON;
        else if (e->detail == ECORE_X_EVENT_DETAIL_POINTER)
          return ECORE_CALLBACK_PASS_ON;
     }
   else if (e->mode == ECORE_X_EVENT_MODE_WHILE_GRABBED)
     {
        if (e->detail == ECORE_X_EVENT_DETAIL_ANCESTOR) return ECORE_CALLBACK_PASS_ON;
        else if (e->detail == ECORE_X_EVENT_DETAIL_INFERIOR)
          return ECORE_CALLBACK_PASS_ON;
     }
   e_border_focus_set(bd, 0, 0);
   return ECORE_CALLBACK_PASS_ON;
}

#if _F_BORDER_CLIP_TO_ZONE_
static void
_e_border_shape_input_clip_to_zone(E_Border *bd)
{
   /* if (!(e_config->window_out_of_vscreen_limits_partly)) return; */
   if (!(bd->visible))
     return;

   if (!(E_CONTAINS(bd->zone->x, bd->zone->y,
                    bd->zone->w, bd->zone->h,
                    bd->x, bd->y, bd->w, bd->h)))
     {
        int x, y, w, h;
        x = bd->x; y = bd->y; w = bd->w; h = bd->h;
        E_RECTS_CLIP_TO_RECT(x, y, w, h,
                             bd->zone->x, bd->zone->y,
                             bd->zone->w, bd->zone->h);
        x -= bd->x;
        y -= bd->y;
        ecore_x_window_shape_input_rectangle_set(bd->bg_win, x, y, w, h);
        ecore_x_window_shape_input_rectangle_set(bd->win, x, y, w, h);
     }
   else
     {
        ecore_x_window_shape_input_rectangle_set(bd->bg_win, 0, 0, bd->w, bd->h);
        ecore_x_window_shape_input_rectangle_set(bd->win, 0, 0, bd->w, bd->h);
     }
}
#endif /* _F_BORDER_CLIP_TO_ZONE_ */

#ifdef _F_DEICONIFY_APPROVE_
static void
_e_border_deiconify_approve_done(Ecore_X_Window win, int done)
{
   E_Border *bd = NULL;
   E_Border *ancestor_bd = NULL;

   if (!e_config->deiconify_approve) return;
   if (done != 1) return;

   bd = e_border_find_by_client_window(win);
   if (!bd) return;

   bd->client.e.state.deiconify_approve.render_done = 1;

   ancestor_bd = bd->client.e.state.deiconify_approve.ancestor;
   if ((ancestor_bd) &&
       (!e_object_is_del(E_OBJECT(ancestor_bd))))
     {
        ancestor_bd->client.e.state.deiconify_approve.req_list =
           eina_list_remove(ancestor_bd->client.e.state.deiconify_approve.req_list, bd);
        bd->client.e.state.deiconify_approve.ancestor = NULL;
     }
   else
     {
        ancestor_bd = bd;
     }

   ELBF(ELBT_BD, 0, bd->client.win,
        "RECEIVE DEICONIFY_APPROVE.. ancestor:%x", ancestor_bd->client.win);

   if ((ancestor_bd->client.e.state.deiconify_approve.req_list == NULL) &&
       (ancestor_bd->client.e.state.deiconify_approve.render_done))
     {
        if (ancestor_bd->client.e.state.deiconify_approve.wait_timer)
          {
             ecore_timer_del(ancestor_bd->client.e.state.deiconify_approve.wait_timer);
             ancestor_bd->client.e.state.deiconify_approve.wait_timer = NULL;

             if (bd->client.e.state.deiconify_approve.wait_timer)
               {
                  ecore_timer_del(bd->client.e.state.deiconify_approve.wait_timer);
                  bd->client.e.state.deiconify_approve.wait_timer = NULL;
               }

             if (bd->client.e.state.deiconify_approve.render_only)
               {
                  ELBF(ELBT_BD, 0, bd->client.win, "Render only.. win:%x", bd->client.win);
                  bd->client.e.state.deiconify_approve.render_only = 0;
                  e_border_force_render_request(bd,EINA_FALSE);
                  return;
               }
             _e_border_uniconify_pending_list_remove(ancestor_bd);
             _e_border_window_pending_event_flush(ancestor_bd);
          }
        else
          {
             ELB(ELBT_BD, "Unset DEICONIFY_APPROVE render_done", ancestor_bd->client.win);
             ancestor_bd->client.e.state.deiconify_approve.render_done = 0;
          }
     }

   if (bd != ancestor_bd)
     {
        if (bd->client.e.state.deiconify_approve.wait_timer)
          {
             ecore_timer_del(bd->client.e.state.deiconify_approve.wait_timer);
             bd->client.e.state.deiconify_approve.wait_timer = NULL;
          }
        _e_border_uniconify_pending_list_remove(bd);
        _e_border_window_pending_event_flush(bd);
     }
}
#endif

static Eina_Bool
_e_border_cb_client_message(void *data  __UNUSED__,
                            int ev_type __UNUSED__,
                            void       *ev)
{
   Ecore_X_Event_Client_Message *e;
   E_Border *bd;
#ifdef _F_USE_DESK_WINDOW_PROFILE_
   char *profile = NULL;
#endif
   traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:BORDER:CLIENT MSG");

   e = ev;
#ifdef _F_USE_DESK_WINDOW_PROFILE_
   if (e->message_type == ECORE_X_ATOM_E_WINDOW_PROFILE_CHANGE)
     {
        bd = e_border_find_by_client_window(e->win);
        if (bd)
          {
             if (bd->client.e.state.profile.use)
               {
                  profile = ecore_x_atom_name_get(e->data.l[1]);
                  ecore_x_e_window_profile_change_request_send(bd->client.win,
                                                               profile);
                  bd->client.e.state.profile.wait_for_done = 1;
               }
          }
     }
   else if (e->message_type == ECORE_X_ATOM_E_WINDOW_PROFILE_CHANGE_DONE)
     {
        bd = e_border_find_by_client_window(e->win);
        if (bd)
          {
             if ((bd->client.e.state.profile.use) &&
                 (bd->client.e.state.profile.wait_for_done))
               {
                  E_Container *con = bd->zone->container;
                  E_Desk *desk = NULL;

                  profile = ecore_x_atom_name_get(e->data.l[1]);
                  if (profile)
                    {
                       if (bd->client.e.state.profile.name)
                         eina_stringshare_del(bd->client.e.state.profile.name);
                       bd->client.e.state.profile.name = eina_stringshare_add(profile);
                    }

                  bd->client.e.state.profile.wait_for_done = 0;

                  desk = e_container_desk_window_profile_get(con, profile);
                  if ((desk) && (bd->desk != desk))
                    {
                       e_border_desk_set(bd, desk);
                       bd->changes.pos = 1;
                       bd->changes.size = 1;
                       bd->changed = 1;
                    }
               }
          }
     }

   if (profile)
     free(profile);
#endif

#ifdef _F_DEICONIFY_APPROVE_
   if (e->message_type == ECORE_X_ATOM_E_DEICONIFY_APPROVE)
     {
        _e_border_deiconify_approve_done(e->win, e->data.l[1]);
        traceEnd(TTRACE_TAG_WINDOW_MANAGER);
        return ECORE_CALLBACK_PASS_ON;
     }
#endif

#ifdef _F_ZONE_WINDOW_ROTATION_
   if (!e_config->wm_win_rotation) return ECORE_CALLBACK_PASS_ON;

   bd = e_border_find_by_client_window(e->win);
   if (!bd)
     {
        if (e_config->wm_win_rotation)
          {
             Ecore_X_Event_Client_Message *msg = NULL;
             Ecore_X_Atom t = e->message_type;
             if ((t == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_ON_PREPARE_DONE)  ||
                 (t == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_OFF_PREPARE_DONE) ||
                 (t == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_CONTROL_WINDOW)   ||
                 (t == ECORE_X_ATOM_E_WINDOW_ROTATION_CHANGE_PREPARE_DONE))
               {
                  msg = E_NEW(Ecore_X_Event_Client_Message, 1);
                  if (!msg) return ECORE_CALLBACK_PASS_ON;

                  msg->win = e->win;
                  msg->message_type = e->message_type;
                  msg->data.l[0] = e->data.l[0];
                  msg->data.l[1] = e->data.l[1];
                  msg->data.l[2] = e->data.l[2];
                  msg->data.l[3] = e->data.l[3];
                  msg->data.l[4] = e->data.l[4];
                  rot.msgs = eina_list_append(rot.msgs, msg);

                  rot.fetch = EINA_TRUE;
               }
          }
        traceEnd(TTRACE_TAG_WINDOW_MANAGER);
        return ECORE_CALLBACK_PASS_ON;
     }

   if (e->message_type == ECORE_X_ATOM_E_WINDOW_ROTATION_CHANGE_DONE)
     {
        ELBF(ELBT_ROT, 0, e->data.l[0], "GET ROT_DONE a%d %dx%d zone_a:%d",
             e->data.l[1], e->data.l[2], e->data.l[3], bd->zone->rot.curr);

        if (e_config->wm_win_rotation)
          {
             bd->client.e.state.rot.ang.prev = bd->client.e.state.rot.ang.curr;
             bd->client.e.state.rot.ang.curr = (int)e->data.l[1];

             if ((int)e->data.l[1] == bd->client.e.state.rot.ang.next)
               {
                  bd->client.e.state.rot.ang.next = -1;

                  _e_border_rotation_list_remove(bd);

                  if (bd->client.e.state.rot.ang.reserve != -1)
                    {
                       ELBF(ELBT_ROT, 0, bd->client.win, "NEXT_ROT(pending) curr:%d != next:%d",
                            bd->client.e.state.rot.ang.curr, bd->client.e.state.rot.ang.reserve);
                       e_border_rotation_set(bd, bd->client.e.state.rot.ang.reserve);
                       bd->client.e.state.rot.ang.reserve = -1;
                    }
                  else if (bd->client.e.state.rot.pending_show)
                    {
                       ELB(ELBT_BD, "SHOW_BD (PEND)", bd->client.win);
                       bd->client.e.state.rot.pending_show = 0;
                       e_border_show(bd);
#ifdef _F_FOCUS_WINDOW_IF_TOP_STACK_
                       if (e_config->focus_setting == E_FOCUS_NEW_WINDOW_IF_TOP_STACK)
                         _e_border_check_stack(bd);
#endif
                    }
               }
             else
               {
                  int angle = e_border_rotation_recommend_angle_get(bd);
                  if (angle != -1) e_border_rotation_set(bd, angle);
               }
          }
     }
#endif
   traceEnd(TTRACE_TAG_WINDOW_MANAGER);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_window_state_request(void *data  __UNUSED__,
                                  int ev_type __UNUSED__,
                                  void       *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_State_Request *e;
   int i;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return ECORE_CALLBACK_PASS_ON;

   for (i = 0; i < 2; i++)
     e_hints_window_state_update(bd, e->state[i], e->action);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_window_move_resize_request(void *data  __UNUSED__,
                                        int ev_type __UNUSED__,
                                        void       *ev)
{
   E_Border *bd;
   Ecore_X_Event_Window_Move_Resize_Request *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (!bd) return ECORE_CALLBACK_PASS_ON;

   if ((bd->shaded) || (bd->shading) ||
       (bd->fullscreen) || (bd->moving) ||
       (bd->resize_mode != RESIZE_NONE))
     return ECORE_CALLBACK_PASS_ON;

   if ((e->button >= 1) && (e->button <= 3))
     {
        bd->mouse.last_down[e->button - 1].mx = e->x;
        bd->mouse.last_down[e->button - 1].my = e->y;
        bd->mouse.last_down[e->button - 1].x = bd->x;
        bd->mouse.last_down[e->button - 1].y = bd->y;
        bd->mouse.last_down[e->button - 1].w = bd->w;
        bd->mouse.last_down[e->button - 1].h = bd->h;
     }
   else
     {
        bd->moveinfo.down.x = bd->x;
        bd->moveinfo.down.y = bd->y;
        bd->moveinfo.down.w = bd->w;
        bd->moveinfo.down.h = bd->h;
     }
   bd->mouse.current.mx = e->x;
   bd->mouse.current.my = e->y;
   bd->moveinfo.down.button = e->button;
   bd->moveinfo.down.mx = e->x;
   bd->moveinfo.down.my = e->y;
   grabbed = 1;

   if (!bd->lock_user_stacking)
     e_border_raise(bd);

   if (e->direction == MOVE)
     {
        bd->cur_mouse_action = e_action_find("window_move");
        if (bd->cur_mouse_action)
          {
             if ((!bd->cur_mouse_action->func.end_mouse) &&
                 (!bd->cur_mouse_action->func.end))
               bd->cur_mouse_action = NULL;
             if (bd->cur_mouse_action)
               {
                  e_object_ref(E_OBJECT(bd->cur_mouse_action));
                  bd->cur_mouse_action->func.go(E_OBJECT(bd), NULL);
               }
          }
        return ECORE_CALLBACK_PASS_ON;
     }

   if (!_e_border_resize_begin(bd))
     return ECORE_CALLBACK_PASS_ON;

   switch (e->direction)
     {
      case RESIZE_TL:
        bd->resize_mode = RESIZE_TL;
        GRAV_SET(bd, ECORE_X_GRAVITY_SE);
        break;

      case RESIZE_T:
        bd->resize_mode = RESIZE_T;
        GRAV_SET(bd, ECORE_X_GRAVITY_S);
        break;

      case RESIZE_TR:
        bd->resize_mode = RESIZE_TR;
        GRAV_SET(bd, ECORE_X_GRAVITY_SW);
        break;

      case RESIZE_R:
        bd->resize_mode = RESIZE_R;
        GRAV_SET(bd, ECORE_X_GRAVITY_W);
        break;

      case RESIZE_BR:
        bd->resize_mode = RESIZE_BR;
        GRAV_SET(bd, ECORE_X_GRAVITY_NW);
        break;

      case RESIZE_B:
        bd->resize_mode = RESIZE_B;
        GRAV_SET(bd, ECORE_X_GRAVITY_N);
        break;

      case RESIZE_BL:
        bd->resize_mode = RESIZE_BL;
        GRAV_SET(bd, ECORE_X_GRAVITY_NE);
        break;

      case RESIZE_L:
        bd->resize_mode = RESIZE_L;
        GRAV_SET(bd, ECORE_X_GRAVITY_E);
        break;

      default:
        return ECORE_CALLBACK_PASS_ON;
     }

   bd->cur_mouse_action = e_action_find("window_resize");
   if (bd->cur_mouse_action)
     {
        if ((!bd->cur_mouse_action->func.end_mouse) &&
            (!bd->cur_mouse_action->func.end))
          bd->cur_mouse_action = NULL;
     }
   if (bd->cur_mouse_action)
     e_object_ref(E_OBJECT(bd->cur_mouse_action));

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_desktop_change(void *data  __UNUSED__,
                            int ev_type __UNUSED__,
                            void       *ev)
{
   E_Border *bd;
   Ecore_X_Event_Desktop_Change *e;

   e = ev;
   bd = e_border_find_by_client_window(e->win);
   if (bd)
     {
        if (e->desk == 0xffffffff)
          e_border_stick(bd);
        else if ((int)e->desk < (bd->zone->desk_x_count * bd->zone->desk_y_count))
          {
             E_Desk *desk;

             desk = e_desk_at_pos_get(bd->zone, e->desk);
             if (desk)
               e_border_desk_set(bd, desk);
          }
     }
   else
     {
        ecore_x_netwm_desktop_set(e->win, e->desk);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_sync_alarm(void *data  __UNUSED__,
                        int ev_type __UNUSED__,
                        void       *ev)
{
   E_Manager *man;
   E_Border *bd;
   Ecore_X_Event_Sync_Alarm *e;
   unsigned int serial;

   e = ev;
   bd = e_border_find_by_alarm(e->alarm);
   if (!bd) return ECORE_CALLBACK_PASS_ON;

   if (bd->client.netwm.sync.wait)
     bd->client.netwm.sync.wait--;

   if (ecore_x_sync_counter_query(bd->client.netwm.sync.counter, &serial))
     {
        E_Border_Pending_Move_Resize *pnd = NULL;

        /* skip pending for which we didn't get a reply */
        while (bd->pending_move_resize)
          {
             pnd = bd->pending_move_resize->data;
             bd->pending_move_resize = eina_list_remove(bd->pending_move_resize, pnd);

             if (serial == pnd->serial)
               break;

             E_FREE(pnd);
          }

        if (pnd)
          {
             bd->x = pnd->x;
             bd->y = pnd->y;
             bd->w = pnd->w;
             bd->h = pnd->h;
             bd->client.w = bd->w - (bd->client_inset.l + bd->client_inset.r);
             bd->client.h = bd->h - (bd->client_inset.t + bd->client_inset.b);
             E_FREE(pnd);
          }
     }

   bd->changes.size = 1;
   bd->changes.pos = 1;

   _e_border_eval(bd);
   evas_render(bd->bg_evas);

   man = e_manager_current_get();
   ecore_x_pointer_xy_get(man ? man->root : ecore_x_window_root_get(bd->win),
                          &bd->mouse.current.mx,
                          &bd->mouse.current.my);

   bd->client.netwm.sync.send_time = ecore_loop_time_get();
   _e_border_resize_handle(bd);

   return ECORE_CALLBACK_PASS_ON;
}

#ifndef _F_DISABLE_E_EFREET_
static Eina_Bool
_e_border_cb_efreet_cache_update(void *data  __UNUSED__,
                                 int ev_type __UNUSED__,
                                 void *ev    __UNUSED__)
{
   Eina_List *l;
   E_Border *bd;

   /* mark all borders for desktop/icon updates */
   EINA_LIST_FOREACH(borders, l, bd)
     {
        if (bd->desktop)
          {
             efreet_desktop_free(bd->desktop);
             bd->desktop = NULL;
          }
        bd->changes.icon = 1;
        bd->changed = 1;
     }
   /*
      e_init_status_set(_("Desktop files scan done"));
      e_init_done();
    */
   return ECORE_CALLBACK_PASS_ON;
}
#endif

static Eina_Bool
_e_border_cb_config_icon_theme(void *data  __UNUSED__,
                               int ev_type __UNUSED__,
                               void *ev    __UNUSED__)
{
   Eina_List *l;
   E_Border *bd;

   /* mark all borders for desktop/icon updates */
   EINA_LIST_FOREACH(borders, l, bd)
     {
        bd->changes.icon = 1;
        bd->changed = 1;
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_pointer_warp(void *data  __UNUSED__,
                          int ev_type __UNUSED__,
                          void       *ev)
{
   E_Event_Pointer_Warp *e;

   e = ev;
   if (!bdmove) return ECORE_CALLBACK_PASS_ON;
   e_border_move(bdmove, bdmove->x + (e->curr.x - e->prev.x), bdmove->y + (e->curr.y - e->prev.y));
   return ECORE_CALLBACK_PASS_ON;
}

static void
_e_border_cb_signal_bind(void            *data,
                         Evas_Object *obj __UNUSED__,
                         const char      *emission,
                         const char      *source)
{
   E_Border *bd;

   bd = data;
   if (e_dnd_active()) return;
   e_bindings_signal_handle(E_BINDING_CONTEXT_WINDOW, E_OBJECT(bd),
                            emission, source);
}

static Eina_Bool
_e_border_cb_mouse_in(void    *data,
                      int type __UNUSED__,
                      void    *event)
{
   Ecore_X_Event_Mouse_In *ev;
   E_Border *bd;

   ev = event;
   bd = data;
#ifdef INOUTDEBUG_MOUSE
   {
      time_t t;
      char *ct;

      const char *modes[] = {
         "MODE_NORMAL",
         "MODE_WHILE_GRABBED",
         "MODE_GRAB",
         "MODE_UNGRAB"
      };
      const char *details[] = {
         "DETAIL_ANCESTOR",
         "DETAIL_VIRTUAL",
         "DETAIL_INFERIOR",
         "DETAIL_NON_LINEAR",
         "DETAIL_NON_LINEAR_VIRTUAL",
         "DETAIL_POINTER",
         "DETAIL_POINTER_ROOT",
         "DETAIL_DETAIL_NONE"
      };
      t = time(NULL);
      ct = ctime(&t);
      ct[strlen(ct) - 1] = 0;
      DBG("@@ ->IN 0x%x 0x%x %s md=%s dt=%s",
             ev->win, ev->event_win,
             ct,
             modes[ev->mode],
             details[ev->detail]);
   }
#endif
   if (grabbed) return ECORE_CALLBACK_PASS_ON;
   if (ev->event_win == bd->win)
     {
        e_focus_event_mouse_in(bd);
     }
#if 0
   if ((ev->win != bd->win) &&
       (ev->win != bd->event_win) &&
       (ev->event_win != bd->win) &&
       (ev->event_win != bd->event_win))
     return ECORE_CALLBACK_PASS_ON;
#else
   if (ev->win != bd->event_win) return ECORE_CALLBACK_PASS_ON;
#endif
   bd->mouse.current.mx = ev->root.x;
   bd->mouse.current.my = ev->root.y;
   if (!bd->bg_evas_in)
     {
        evas_event_feed_mouse_in(bd->bg_evas, ev->time, NULL);
        bd->bg_evas_in = EINA_TRUE;
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_mouse_out(void    *data,
                       int type __UNUSED__,
                       void    *event)
{
   Ecore_X_Event_Mouse_Out *ev;
   E_Border *bd;

   ev = event;
   bd = data;
#ifdef INOUTDEBUG_MOUSE
   {
      time_t t;
      char *ct;

      const char *modes[] = {
         "MODE_NORMAL",
         "MODE_WHILE_GRABBED",
         "MODE_GRAB",
         "MODE_UNGRAB"
      };
      const char *details[] = {
         "DETAIL_ANCESTOR",
         "DETAIL_VIRTUAL",
         "DETAIL_INFERIOR",
         "DETAIL_NON_LINEAR",
         "DETAIL_NON_LINEAR_VIRTUAL",
         "DETAIL_POINTER",
         "DETAIL_POINTER_ROOT",
         "DETAIL_DETAIL_NONE"
      };
      t = time(NULL);
      ct = ctime(&t);
      ct[strlen(ct) - 1] = 0;
      DBG("@@ <-OUT 0x%x 0x%x %s md=%s dt=%s",
             ev->win, ev->event_win,
             ct,
             modes[ev->mode],
             details[ev->detail]);
   }
#endif
   if (grabbed) return ECORE_CALLBACK_PASS_ON;
   if (ev->event_win == bd->win)
     {
        if (bd->fullscreen)
          return ECORE_CALLBACK_PASS_ON;
        if ((ev->mode == ECORE_X_EVENT_MODE_UNGRAB) &&
            (ev->detail == ECORE_X_EVENT_DETAIL_INFERIOR))
          return ECORE_CALLBACK_PASS_ON;
        if (ev->mode == ECORE_X_EVENT_MODE_GRAB)
          return ECORE_CALLBACK_PASS_ON;
        if ((ev->mode == ECORE_X_EVENT_MODE_NORMAL) &&
            (ev->detail == ECORE_X_EVENT_DETAIL_INFERIOR))
          return ECORE_CALLBACK_PASS_ON;
        e_focus_event_mouse_out(bd);
     }
#if 0
   if ((ev->win != bd->win) &&
       (ev->win != bd->event_win) &&
       (ev->event_win != bd->win) &&
       (ev->event_win != bd->event_win))
     return ECORE_CALLBACK_PASS_ON;
#else
   if (ev->win != bd->event_win) return ECORE_CALLBACK_PASS_ON;
#endif
   bd->mouse.current.mx = ev->root.x;
   bd->mouse.current.my = ev->root.y;
   if (bd->bg_evas_in)
     {
        if (!((evas_event_down_count_get(bd->bg_evas) > 0) &&
              (!((ev->mode == ECORE_X_EVENT_MODE_GRAB) &&
                 (ev->detail == ECORE_X_EVENT_DETAIL_NON_LINEAR)))))
          {
             if (ev->mode == ECORE_X_EVENT_MODE_GRAB)
               evas_event_feed_mouse_cancel(bd->bg_evas, ev->time, NULL);
             evas_event_feed_mouse_out(bd->bg_evas, ev->time, NULL);
             bd->bg_evas_in = EINA_FALSE;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_mouse_wheel(void    *data,
                         int type __UNUSED__,
                         void    *event)
{
   Ecore_Event_Mouse_Wheel *ev;
   E_Border *bd;

   ev = event;
   bd = data;
   if ((ev->event_window == bd->win) ||
       (ev->event_window == bd->event_win))
     {
        bd->mouse.current.mx = ev->root.x;
        bd->mouse.current.my = ev->root.y;
        if (!bd->cur_mouse_action)
          e_bindings_wheel_event_handle(E_BINDING_CONTEXT_WINDOW,
                                        E_OBJECT(bd), ev);
     }
   evas_event_feed_mouse_wheel(bd->bg_evas, ev->direction, ev->z, ev->timestamp, NULL);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_mouse_down(void    *data,
                        int type __UNUSED__,
                        void    *event)
{
   Ecore_Event_Mouse_Button *ev;
   E_Border *bd;

   ev = event;
   bd = data;
   if ((ev->event_window == bd->win) ||
       (ev->event_window == bd->event_win))
     {
        if ((ev->buttons >= 1) && (ev->buttons <= 3))
          {
             bd->mouse.last_down[ev->buttons - 1].mx = ev->root.x;
             bd->mouse.last_down[ev->buttons - 1].my = ev->root.y;
             bd->mouse.last_down[ev->buttons - 1].x = bd->x + bd->fx.x;
             bd->mouse.last_down[ev->buttons - 1].y = bd->y + bd->fx.y;
             bd->mouse.last_down[ev->buttons - 1].w = bd->w;
             bd->mouse.last_down[ev->buttons - 1].h = bd->h;
          }
        else
          {
             bd->moveinfo.down.x = bd->x + bd->fx.x;
             bd->moveinfo.down.y = bd->y + bd->fx.y;
             bd->moveinfo.down.w = bd->w;
             bd->moveinfo.down.h = bd->h;
          }
        bd->mouse.current.mx = ev->root.x;
        bd->mouse.current.my = ev->root.y;
        if (!bd->cur_mouse_action)
          {
             bd->cur_mouse_action =
               e_bindings_mouse_down_event_handle(E_BINDING_CONTEXT_WINDOW,
                                                  E_OBJECT(bd), ev);
             if (bd->cur_mouse_action)
               {
                  if ((!bd->cur_mouse_action->func.end_mouse) &&
                      (!bd->cur_mouse_action->func.end))
                    bd->cur_mouse_action = NULL;
                  if (bd->cur_mouse_action)
                    e_object_ref(E_OBJECT(bd->cur_mouse_action));
               }
          }
        e_focus_event_mouse_down(bd);
     }
   if (ev->window != ev->event_window)
     {
        return 1;
     }
   if ((ev->window != bd->event_win) && (ev->event_window != bd->win))
     {
        return 1;
     }
   if ((ev->buttons >= 1) && (ev->buttons <= 3))
     {
        bd->mouse.last_down[ev->buttons - 1].mx = ev->root.x;
        bd->mouse.last_down[ev->buttons - 1].my = ev->root.y;
        bd->mouse.last_down[ev->buttons - 1].x = bd->x + bd->fx.x;
        bd->mouse.last_down[ev->buttons - 1].y = bd->y + bd->fx.y;
        bd->mouse.last_down[ev->buttons - 1].w = bd->w;
        bd->mouse.last_down[ev->buttons - 1].h = bd->h;
     }
   else
     {
        bd->moveinfo.down.x = bd->x + bd->fx.x;
        bd->moveinfo.down.y = bd->y + bd->fx.y;
        bd->moveinfo.down.w = bd->w;
        bd->moveinfo.down.h = bd->h;
     }
   bd->mouse.current.mx = ev->root.x;
   bd->mouse.current.my = ev->root.y;
/*
   if (bd->moving)
     {
     }
   else if (bd->resize_mode != RESIZE_NONE)
     {
     }
   else
 */
   {
      Evas_Button_Flags flags = EVAS_BUTTON_NONE;

      if (ev->double_click) flags |= EVAS_BUTTON_DOUBLE_CLICK;
      if (ev->triple_click) flags |= EVAS_BUTTON_TRIPLE_CLICK;
      evas_event_feed_mouse_down(bd->bg_evas, ev->buttons, flags, ev->timestamp, NULL);
   }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_mouse_up(void    *data,
                      int type __UNUSED__,
                      void    *event)
{
   Ecore_Event_Mouse_Button *ev;
   E_Border *bd;

   ev = event;
   bd = data;
   if ((ev->event_window == bd->win) ||
       (ev->event_window == bd->event_win))
     {
        if ((ev->buttons >= 1) && (ev->buttons <= 3))
          {
             bd->mouse.last_up[ev->buttons - 1].mx = ev->root.x;
             bd->mouse.last_up[ev->buttons - 1].my = ev->root.y;
             bd->mouse.last_up[ev->buttons - 1].x = bd->x + bd->fx.x;
             bd->mouse.last_up[ev->buttons - 1].y = bd->y + bd->fx.y;
          }
        bd->mouse.current.mx = ev->root.x;
        bd->mouse.current.my = ev->root.y;
        /* also we dont pass the same params that went in - then again that */
        /* should be ok as we are just ending the action if it has an end */
        if (bd->cur_mouse_action)
          {
             if (bd->cur_mouse_action->func.end_mouse)
               bd->cur_mouse_action->func.end_mouse(E_OBJECT(bd), "", ev);
             else if (bd->cur_mouse_action->func.end)
               bd->cur_mouse_action->func.end(E_OBJECT(bd), "");
             e_object_unref(E_OBJECT(bd->cur_mouse_action));
             bd->cur_mouse_action = NULL;
          }
        else
          {
             if (!e_bindings_mouse_up_event_handle(E_BINDING_CONTEXT_WINDOW, E_OBJECT(bd), ev))
               e_focus_event_mouse_up(bd);
          }
     }
   if (ev->window != bd->event_win) return ECORE_CALLBACK_PASS_ON;
   if ((ev->buttons >= 1) && (ev->buttons <= 3))
     {
        bd->mouse.last_up[ev->buttons - 1].mx = ev->root.x;
        bd->mouse.last_up[ev->buttons - 1].my = ev->root.y;
        bd->mouse.last_up[ev->buttons - 1].x = bd->x + bd->fx.x;
        bd->mouse.last_up[ev->buttons - 1].y = bd->y + bd->fx.y;
     }
   bd->mouse.current.mx = ev->root.x;
   bd->mouse.current.my = ev->root.y;

   bd->drag.start = 0;

   evas_event_feed_mouse_up(bd->bg_evas, ev->buttons, EVAS_BUTTON_NONE, ev->timestamp, NULL);
   return ECORE_CALLBACK_PASS_ON;
}

static void
_e_border_stay_within_container(E_Border *bd, int x, int y, int *new_x, int *new_y)
{
#ifdef _F_BORDER_CLIP_TO_ZONE_
   int new_x_max, new_y_max;
   int new_x_min, new_y_min;
   int margin_x=0, margin_y=0;

   if (!bd->internal)
     {
        margin_x = bd->w - e_config->border_drag_margin_x;
        margin_y = bd->h - e_config->border_drag_margin_y;
     }

   new_x_max = bd->zone->x + bd->zone->w - bd->w + margin_x;
   new_x_min = bd->zone->x - margin_x;
   new_y_max = bd->zone->y + bd->zone->h - bd->h + margin_y;
   new_y_min = bd->zone->y - margin_y;

   if (x >= new_x_max)      *new_x = new_x_max;
   else if (x <= new_x_min) *new_x = new_x_min;

   if (y >= new_y_max)      *new_y = new_y_max;
   else if (y <= new_y_min) *new_y = new_y_min;
#endif
}

static Eina_Bool
_e_border_cb_mouse_move(void    *data,
                        int type __UNUSED__,
                        void    *event)
{
   Ecore_Event_Mouse_Move *ev;
   E_Border *bd;

   ev = event;
   bd = data;
   if ((ev->window != bd->event_win) &&
       (ev->event_window != bd->win)) return ECORE_CALLBACK_PASS_ON;
   bd->mouse.current.mx = ev->root.x;
   bd->mouse.current.my = ev->root.y;
   if (bd->moving)
     {
        int x, y, new_x, new_y;
        int new_w, new_h;
        Eina_List *skiplist = NULL;
#if 0
        // FIXME: remove? sync what for when only moving?
        if ((ecore_loop_time_get() - bd->client.netwm.sync.time) > 0.5)
          bd->client.netwm.sync.wait = 0;
        if ((bd->client.netwm.sync.request) &&
            (bd->client.netwm.sync.alarm) &&
            (bd->client.netwm.sync.wait > 1)) return ECORE_CALLBACK_PASS_ON;
#endif
        if ((bd->moveinfo.down.button >= 1) && (bd->moveinfo.down.button <= 3))
          {
             x = bd->mouse.last_down[bd->moveinfo.down.button - 1].x +
               (bd->mouse.current.mx - bd->moveinfo.down.mx);
             y = bd->mouse.last_down[bd->moveinfo.down.button - 1].y +
               (bd->mouse.current.my - bd->moveinfo.down.my);
          }
        else
          {
             x = bd->moveinfo.down.x +
               (bd->mouse.current.mx - bd->moveinfo.down.mx);
             y = bd->moveinfo.down.y +
               (bd->mouse.current.my - bd->moveinfo.down.my);
          }
        new_x = x;
        new_y = y;

#ifdef _F_USE_RESIST_MAGNETIC_EFFECT_
        skiplist = eina_list_append(skiplist, bd);
        e_resist_container_border_position(bd->zone->container, skiplist,
                                           bd->x, bd->y, bd->w, bd->h,
                                           x, y, bd->w, bd->h,
                                           &new_x, &new_y, &new_w, &new_h);
        eina_list_free(skiplist);

        _e_border_stay_within_container(bd, x, y, &new_x, &new_y);
#else
        /* if (e_config->window_out_of_vscreen_limits_partly) */
        if (1)
          _e_border_stay_within_container(bd, x, y, &new_x, &new_y);
        else
          {
             skiplist = eina_list_append(skiplist, bd);
             e_resist_container_border_position(bd->zone->container, skiplist,
                                                bd->x, bd->y, bd->w, bd->h,
                                                x, y, bd->w, bd->h,
                                                &new_x, &new_y, &new_w, &new_h);
             eina_list_free(skiplist);
          }
#endif
        bd->shelf_fix.x = 0;
        bd->shelf_fix.y = 0;
        bd->shelf_fix.modified = 0;
        e_border_move(bd, new_x, new_y);
        e_zone_flip_coords_handle(bd->zone, ev->root.x, ev->root.y);
     }
   else if (bd->resize_mode != RESIZE_NONE)
     {
        if ((bd->client.netwm.sync.request) &&
            (bd->client.netwm.sync.alarm))
          {
             if ((ecore_loop_time_get() - bd->client.netwm.sync.send_time) > 0.5)
               {
                  E_Border_Pending_Move_Resize *pnd;

                  if (bd->pending_move_resize)
                    {
                       bd->changes.pos = 1;
                       bd->changes.size = 1;
                       bd->changed = 1;
                       _e_border_client_move_resize_send(bd);
                    }
                  EINA_LIST_FREE(bd->pending_move_resize, pnd)
                    E_FREE(pnd);

                  bd->client.netwm.sync.wait = 0;
               }
             /* sync.wait is incremented when resize_handle sends
              * sync-request and decremented by sync-alarm cb. so
              * we resize here either on initial resize, timeout or
              * when no new resize-request was added by sync-alarm cb.
              */
             if (!bd->client.netwm.sync.wait)
               _e_border_resize_handle(bd);
          }
        else
          _e_border_resize_handle(bd);
     }
   else
     {
        if (bd->drag.start)
          {
             if ((bd->drag.x == -1) && (bd->drag.y == -1))
               {
                  bd->drag.x = ev->root.x;
                  bd->drag.y = ev->root.y;
               }
             else
               {
                  int dx, dy;

                  dx = bd->drag.x - ev->root.x;
                  dy = bd->drag.y - ev->root.y;
                  if (((dx * dx) + (dy * dy)) >
                      (e_config->drag_resist * e_config->drag_resist))
                    {
     /* start drag! */
                        if (bd->icon_object)
                          {
                             Evas_Object *o = NULL;
                             Evas_Coord x, y, w, h;
                             const char *drag_types[] = { "enlightenment/border" };

                             e_object_ref(E_OBJECT(bd));
                             evas_object_geometry_get(bd->icon_object,
                                                      &x, &y, &w, &h);
                             drag_border = e_drag_new(bd->zone->container,
                                                      bd->x + bd->fx.x + x,
                                                      bd->y + bd->fx.y + y,
                                                      drag_types, 1, bd, -1,
                                                      NULL,
                                                      _e_border_cb_drag_finished);
                             o = e_border_icon_add(bd, drag_border->evas);
                             if (!o)
                               {
     /* FIXME: fallback icon for drag */
                                   o = evas_object_rectangle_add(drag_border->evas);
                                   evas_object_color_set(o, 255, 255, 255, 255);
                               }
                             e_drag_object_set(drag_border, o);

                             e_drag_resize(drag_border, w, h);
                             e_drag_start(drag_border, bd->drag.x, bd->drag.y);
                          }
                        bd->drag.start = 0;
                    }
               }
          }
        evas_event_feed_mouse_move(bd->bg_evas, ev->x, ev->y, ev->timestamp, NULL);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_cb_grab_replay(void *data __UNUSED__,
                         int        type,
                         void      *event)
{
   Ecore_Event_Mouse_Button *ev;

   if (type != ECORE_EVENT_MOUSE_BUTTON_DOWN) return ECORE_CALLBACK_DONE;
   ev = event;
   if ((e_config->pass_click_on)
       || (e_config->always_click_to_raise) // this works even if not on click-to-focus
       || (e_config->always_click_to_focus) // this works even if not on click-to-focus
      )
     {
        E_Border *bd;

        bd = e_border_find_by_window(ev->event_window);
        if (bd)
          {
             if (bd->cur_mouse_action)
               return ECORE_CALLBACK_DONE;
             if (ev->event_window == bd->win)
               {
                  if (!e_bindings_mouse_down_find(E_BINDING_CONTEXT_WINDOW,
                                                  E_OBJECT(bd), ev, NULL))
                    return ECORE_CALLBACK_PASS_ON;
               }
          }
     }
   return ECORE_CALLBACK_DONE;
}

static void
_e_border_cb_drag_finished(E_Drag     *drag,
                           int dropped __UNUSED__)
{
   E_Border *bd;

   bd = drag->data;
   e_object_unref(E_OBJECT(bd));
   drag_border = NULL;
}

#ifdef _F_USE_DESK_WINDOW_PROFILE_
static Eina_Bool
_e_border_cb_desk_window_profile_change(void *data  __UNUSED__,
                                        int ev_type __UNUSED__,
                                        void       *ev)
{
   E_Event_Desk_Window_Profile_Change *e;
   Eina_List *l = NULL;
   E_Border *bd;

   e = (E_Event_Desk_Window_Profile_Change *)ev;
   EINA_LIST_FOREACH(borders, l, bd)
     {
        if (!e_object_is_del(E_OBJECT(bd)))
          {
             bd->client.e.fetch.profile = 1;
             bd->changed = 1;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}
#endif

#ifdef _F_ZONE_WINDOW_ROTATION_

#define SIZE_EQUAL_TO_ZONE(a, z) \
   ((((a)->w) == ((z)->w)) &&    \
    (((a)->h) == ((z)->h)))

static int
_e_border_rotation_curr_next_get(E_Border *bd)
{
   if (!bd) return -1;

   return ((bd->client.e.state.rot.ang.next == -1) ?
           bd->client.e.state.rot.ang.curr : bd->client.e.state.rot.ang.next);
}

static int
_prev_angle_get(Ecore_X_Window win)
{
   int ret, count = 0, ang = -1;
   unsigned char* data = NULL;

   ret = ecore_x_window_prop_property_get
      (win, ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE,
      ECORE_X_ATOM_CARDINAL, 32, &data, &count);

   if ((ret) && (data) && (count))
     ang = ((int *)data)[0];
   if (data) free(data);
   return ang;
}

/* check whether virtual keyboard is visible on the zone */
static Eina_Bool
_e_border_is_vkbd(E_Border *bd)
{
   if (!e_config->wm_win_rotation) return EINA_FALSE;

   if ((rot.vkbd_ctrl_win) &&
       (rot.vkbd == bd) &&
       (!e_object_is_del(E_OBJECT(rot.vkbd))) &&
       (rot.vkbd->zone == bd->zone) &&
       (E_INTERSECTS(bd->zone->x, bd->zone->y,
                     bd->zone->w, bd->zone->h,
                     rot.vkbd->x, rot.vkbd->y,
                     rot.vkbd->w, rot.vkbd->h)))
     {
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

static Eina_Bool
_e_border_rotation_is_dependent_parent(E_Border *bd)
{
   if (!bd) return EINA_FALSE;
   if ((!bd->parent) || (!bd->parent->visible)) return EINA_FALSE;
   if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_NORMAL) return EINA_FALSE;
   if ((!bd->client.e.state.rot.support) &&
       (!bd->client.e.state.rot.app_set)) return EINA_FALSE;

   return EINA_TRUE;
}

static Eina_Bool
_e_border_cb_zone_rotation_change_begin(void *data __UNUSED__,
                                        int ev_type __UNUSED__,
                                        void       *ev)
{
   E_Event_Zone_Rotation_Change_Begin *e = ev;

   if (!e_config->wm_win_rotation) return ECORE_CALLBACK_PASS_ON;
   if ((!e) || (!e->zone)) return ECORE_CALLBACK_PASS_ON;

   if (!_e_border_rotation_zone_set(e->zone))
     {
        /* The WM will decide to cancel zone rotation at idle time.
         * Because, the policy module can make list of rotation windows
         */
        rot.cancel.state = EINA_TRUE;
        rot.cancel.zone = e->zone;
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_rotation_zone_set(E_Zone *zone)
{
   E_Border_List *l = NULL;
   E_Border *bd = NULL;
   Eina_Bool res = EINA_FALSE;
   Eina_Bool ret = EINA_FALSE;

   if (!e_config->wm_win_rotation) return EINA_FALSE;

   l = e_container_border_list_last(zone->container);
   if (!l) return ret;

   /* step 1. make the list needs to be rotated. */
   while ((bd = e_container_border_list_prev(l)))
     {
        if (bd->zone != zone) continue;

        // if this window has parent and window type isn't "ECORE_X_WINDOW_TYPE_NORMAL",
        // it will be rotated when parent do rotate itself.
        // so skip here.
        if ((bd->parent) &&
            (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_NORMAL)) continue;

        // default type is "E_BORDER_ROTATION_TYPE_NORMAL",
        // but it can be changed to "E_BORDER_ROTATION_TYPE_DEPENDENT" by illume according to its policy.
        // if it's not normal type window, will be rotated by illume.
        // so skip here.
        if (bd->client.e.state.rot.type != E_BORDER_ROTATION_TYPE_NORMAL) continue;

        if ((!bd->visible) ||
            (!E_INTERSECTS(bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h,
                           bd->x, bd->y, bd->w, bd->h))) continue;

        res = e_border_rotation_set(bd, zone->rot.curr);
        if (!res)
          {
             ELBF(ELBT_ROT, 0, bd->client.win, "FAILED ROT_SET(main) curr:%d != zone:%d",
                  bd->client.e.state.rot.ang.curr, zone->rot.curr);
          }
        else ret = EINA_TRUE;
     }
   if (l) e_container_border_list_free(l);

   return ret;
}

static void
_e_border_rotation_change_request(E_Border *bd, int rotation)
{
   if (!bd) return;
   if (rotation < 0) return;

   // if this window is in withdrawn state, change the state to NORMAL.
   // that's because the window in withdrawn state can't render its canvas.
   // eventually, this window will not send the message of rotation done,
   // even if e17 request to rotation this window.
   e_hints_window_visible_set(bd);

   ecore_x_e_window_rotation_change_request_send(bd->client.win, rotation);

   if (bd->client.e.state.deiconify_approve.pending)
     _e_border_deiconify_approve_send_pending_end(bd);

   bd->client.e.state.rot.wait_for_done = 1;

   if ((!rot.async_list) ||
       (!eina_list_data_find(rot.async_list, bd)))
     {
        if (rot.done_timer)
          ecore_timer_del(rot.done_timer);
        ELB(ELBT_ROT, "ADD TIMEOUT ROT_DONE", (unsigned int)NULL);
        rot.done_timer = ecore_timer_add(4.0f,
                                         _e_border_rotation_change_done_timeout,
                                         NULL);
     }
}

static void
_e_border_rotation_change_message_send(E_Border *bd)
{
   int rotation;
   Eina_Bool resize = EINA_FALSE;

   if (!bd) return;

   rotation = bd->client.e.state.rot.ang.next;
   if (rotation == -1) return;
   if (bd->client.e.state.rot.wait_for_done) return;

   resize = _e_border_rotation_prepare_send(bd, rotation);

   // new protocol
   if ((resize) && (!bd->client.e.state.rot.geom_hint))
     {
        ELBF(ELBT_ROT, 1, 0, "SEND ROT_CHANGE_REQUEST");
        SLOGI("[ROTATION] SEND REQ_ROT, win:0x%08x, rot:%d",
                     bd->client.win, rotation);
        _e_border_rotation_change_request(bd, rotation);
     }
   // original protocol
   else
     {
        bd->client.e.state.rot.pending_change_request = resize;

        if (!resize)
          {
             ELBF(ELBT_ROT, 1, 0, "SEND ROT_CHANGE_REQUEST");
             SLOGI("[ROTATION] SEND REQ_ROT, win:0x%08x, rot:%d",
                          bd->client.win, rotation);
             _e_border_rotation_change_request(bd, rotation);
          }
     }
}

static void
_e_border_rotation_list_send(void)
{
   Eina_List *l;
   E_Border *bd;

   if (!e_config->wm_win_rotation) return;
   if (!rot.list) return;
   if (eina_list_count(rot.list) <= 0) return;

   EINA_LIST_FOREACH(rot.list, l, bd)
     {
        if (bd->client.e.state.rot.pending_change_request) continue;
        if (bd->client.e.state.rot.wait_for_done) continue;

        _e_border_rotation_change_message_send(bd);
     }
}

static Eina_Bool
_e_border_rotation_change_prepare_timeout(void *data)
{
   E_Zone *zone = data;
   if (!e_config->wm_win_rotation) return ECORE_CALLBACK_CANCEL;

   ELB(ELBT_ROT, "TIMEOUT ROT_CHANGE_PREPARE(VKBD)", 0);

   if ((zone) && (rot.wait_prepare_done))
     {
        if (rot.list)
          {
             _e_border_rotation_list_send();
             if (rot.prepare_timer)
               ecore_timer_del(rot.prepare_timer);
             rot.prepare_timer = NULL;
             rot.wait_prepare_done = EINA_FALSE;
          }
     }
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_border_rotation_change_done_timeout(void *data __UNUSED__)
{
   if (!e_config->wm_win_rotation) return ECORE_CALLBACK_CANCEL;
   ELB(ELBT_ROT, "TIMEOUT ROT_CHANGE", 0);
   _e_border_rotation_change_done();
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_border_rotation_list_remove(E_Border *bd)
{
   E_Event_Border_Rotation_Change_End *ev = NULL;
   Eina_Bool found = EINA_FALSE;

   if (!e_config->wm_win_rotation) return;

   if (eina_list_data_find(rot.list, bd) == bd)
     {
        found = EINA_TRUE;
        rot.list = eina_list_remove(rot.list, bd);
     }

   if (bd->client.e.state.rot.wait_for_done)
     {
        bd->client.e.state.rot.wait_for_done = 0;

        /* if we make the border event in the _e_border_free function,
         * then we may meet a crash problem, only work this at least e_border_hide.
         */
        if (!e_object_is_del(E_OBJECT(bd)))
          {
             ev = E_NEW(E_Event_Border_Rotation_Change_End, 1);
             if (ev)
               {
                  ev->border = bd;
                  e_object_ref(E_OBJECT(bd));
                  ecore_event_add(E_EVENT_BORDER_ROTATION_CHANGE_END,
                                  ev,
                                  _e_border_event_border_rotation_change_end_free,
                                  NULL);
               }
          }

        if ((found) &&
            (eina_list_count(rot.list) == 0))
          {
             _e_border_rotation_change_done();
          }
     }
   bd->client.e.state.rot.ang.next = -1;
   bd->changes.rotation = 0;
}

static void
_e_border_rotation_change_done(void)
{
   E_Manager *m = NULL;
   E_Border *bd;

   if (!e_config->wm_win_rotation) return;

   if (rot.prepare_timer) ecore_timer_del(rot.prepare_timer);
   rot.prepare_timer = NULL;

   rot.wait_prepare_done = EINA_FALSE;

   if (rot.done_timer) ecore_timer_del(rot.done_timer);
   rot.done_timer = NULL;

   EINA_LIST_FREE(rot.list, bd)
     {
        if (bd->client.e.state.rot.pending_show)
          {
             ELB(ELBT_ROT, "SHOW PEND(TIMEOUT)", bd->client.win);
             bd->client.e.state.rot.pending_show = 0;
             e_border_show(bd);
          }
        bd->client.e.state.rot.ang.next = -1;
        bd->client.e.state.rot.wait_for_done = 0;
     }

   EINA_LIST_FREE(rot.async_list, bd)
     {
        ELB(ELBT_ROT, "ASYNC_LIST MESSAGE SEND", bd->client.win);
        _e_border_rotation_change_message_send(bd);
     }

   rot.list = NULL;
   rot.async_list = NULL;

   m = e_manager_current_get();
   if (rot.screen_lock)
     {
        e_manager_comp_screen_unlock(m);
        rot.screen_lock = EINA_FALSE;
     }
   e_zone_rotation_update_done(e_util_zone_current_get(m));
}

static Eina_Bool
_e_border_rotation_prepare_send(E_Border *bd, int rotation)
{
   E_Zone *zone = bd->zone;
   int x, y, w, h;
   Eina_Bool move = EINA_FALSE;
   Eina_Bool hint = EINA_FALSE;
   Eina_Bool resize = EINA_FALSE;

   x = bd->x;
   y = bd->y;
   w = bd->w;
   h = bd->h;

   /* Do not resize for the tiled window */
#ifdef _F_USE_TILED_DESK_LAYOUT_
   if (e_config->use_tiled_desk_layout)
     {
        if (bd->client.e.state.ly.support)
          {
             if (bd->client.e.state.ly.curr_ly)
               {
                  if ((bd->client.e.state.ly.curr_ly->x == bd->x) &&
                      (bd->client.e.state.ly.curr_ly->y == bd->y) &&
                      (bd->client.e.state.ly.curr_ly->w == bd->w) &&
                      (bd->client.e.state.ly.curr_ly->h == bd->h))
                    {
                       goto end;
                    }
               }
          }
     }
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
   if (SIZE_EQUAL_TO_ZONE(bd, zone)) goto end;

   ELB(ELBT_ROT, "SIZE DIFF WITH ZONE", 0);
   ELBF(ELBT_ROT, 0, bd->client.win, "ORIGIN_SIZE name:%s (%d,%d) %dx%d",
        bd->client.icccm.name, bd->x, bd->y, bd->w, bd->h);

   hint = _e_border_rotation_geom_get(bd, bd->zone, rotation,
                                      &x, &y, &w, &h, &move);

   if ((!hint) || (bd->w != w) || (bd->h != h)) resize = EINA_TRUE;

end:

   SLOGI("[ROTATION] SEND PREP_ROT, win:0x%08x a%d",
                bd->client.win, rotation);
   ELBF(ELBT_ROT, 1, bd->client.win,
        "SEND ROT_CHANGE_PREPARE a%d, resize: %d, (%dx%d)", rotation, resize, w, h);

   ecore_x_e_window_rotation_change_prepare_send
      (bd->client.win, rotation, resize, w, h);

   if (((move) && ((bd->x !=x) || (bd->y !=y))) ||
       ((resize) && ((bd->w != w) || (bd->h != h))))
     {
        _e_border_move_resize_internal(bd, x, y, w, h, EINA_TRUE, move);
        ELBF(ELBT_ROT, 0, bd->client.win, "RESIZE name:%s (%d,%d) %dx%d resize:%d",
             bd->client.icccm.name, x, y, w, h, resize);
     }

   return resize;
}

static Eina_Bool
_e_border_cb_window_configure(void *data    __UNUSED__,
                              int   ev_type __UNUSED__,
                              void         *ev)
{
   Ecore_X_Event_Window_Configure *e = ev;

   if (!e) return ECORE_CALLBACK_PASS_ON;
   if (!e_config->wm_win_rotation) return ECORE_CALLBACK_PASS_ON;

   E_Border *bd = e_border_find_by_client_window(e->win);
   if (!bd) return ECORE_CALLBACK_PASS_ON;

   if ((bd->client.e.state.rot.pending_change_request) &&
       (bd->client.e.state.rot.geom_hint))
     {
        if ((e->w == bd->w) && (e->h == bd->h))
          {
             ELB(ELBT_BD, "GET CONFIGURE_NOTI (ROTATION)", bd->client.win);
             bd->client.e.state.rot.pending_change_request = 0;

             if (bd->client.e.state.rot.wait_for_done)
               {
                  ELB(ELBT_BD, "BUT, Already wait for DONE", bd->client.win);
                  goto end;
               }

             ELBF(ELBT_ROT, 0, bd->client.win,
                  "SEND ROT_CHANGE_REQUEST a%d %dx%d",
                  bd->client.e.state.rot.ang.next,
                  bd->w, bd->h);
             SLOGI("[ROTATION] SEND REQ_ROT(CONFIGURE_NOTI), win:0x%08x, rot:%d",
                          bd->client.win, bd->client.e.state.rot.ang.next);
             _e_border_rotation_change_request(bd, bd->client.e.state.rot.ang.next);
          }
     }

end:
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_border_rotation_geom_get(E_Border  *bd,
                            E_Zone    *zone,
                            int        ang,
                            int       *x,
                            int       *y,
                            int       *w,
                            int       *h,
                            Eina_Bool *move)
{
   if (!e_config->wm_win_rotation) return EINA_FALSE;

   Eina_Bool res = EINA_FALSE;
   Eina_Bool _move = EINA_TRUE;
   int _x = bd->x;
   int _y = bd->y;
   int _w = bd->w;
   int _h = bd->h;

#ifdef _F_USE_VIRT_RESOLUTION_
   int prefer_w = 0;
   int prefer_h = 0;
   int temp_w = 0;
   int temp_h = 0;
#endif /* end of _F_USE_VIRT_RESOLUTION_ */

   if (x) *x = bd->x;
   if (y) *y = bd->y;
   if (w) *w = bd->w;
   if (h) *h = bd->h;
   if (move) *move = EINA_TRUE;

#ifdef _F_USE_VIRT_RESOLUTION_
   if (e_virtual_res_type_get() == E_VIRTUAL_RES_SINGLE_SOURCE)
     {
        E_Virtual_Res_Data* vir_res_data = e_virtual_res_data_get();
        if (vir_res_data && vir_res_data->preferred_resolution_get)
          vir_res_data->preferred_resolution_get(&prefer_w, &prefer_h);
        temp_w = prefer_w;
        temp_h = prefer_h;
     }
   else
     {
        temp_w = zone->w;
        temp_h = zone->h;
     }
#endif /* end of _F_USE_VIRT_RESOLUTION_ */

   if (bd->client.e.state.rot.geom_hint)
     {
        switch (ang)
          {
           case   0:
              _w = bd->client.e.state.rot.geom[0].w;
              _h = bd->client.e.state.rot.geom[0].h;
              if (_w == 0) _w = bd->w;
              if (_h == 0) _h = bd->h;

#ifdef _F_USE_VIRT_RESOLUTION_
              _x = 0; _y = temp_h - _h;
#else
              _x = 0; _y = zone->h - _h;
#endif /* end of _F_USE_VIRT_RESOLUTION_ */

              break;
           case  90:
              _w = bd->client.e.state.rot.geom[1].w;
              _h = bd->client.e.state.rot.geom[1].h;
              if (_w == 0) _w = bd->w;
              if (_h == 0) _h = bd->h;

#ifdef _F_USE_VIRT_RESOLUTION_
              _x = temp_w - _w; _y = 0;
#else
              _x = zone->w - _w; _y = 0;
#endif /* end of _F_USE_VIRT_RESOLUTION_ */

              break;
           case 180:
              _w = bd->client.e.state.rot.geom[2].w;
              _h = bd->client.e.state.rot.geom[2].h;
              if (_w == 0) _w = bd->w;
              if (_h == 0) _h = bd->h;
              _x = 0; _y = 0;
              break;
           case 270:
              _w = bd->client.e.state.rot.geom[3].w;
              _h = bd->client.e.state.rot.geom[3].h;
              if (_w == 0) _w = bd->w;
              if (_h == 0) _h = bd->h;
              _x = 0; _y = 0;
              break;
          }

        if (x) *x = _x;
        if (y) *y = _y;
        if (w) *w = _w;
        if (h) *h = _h;

        if (!((rot.vkbd) && (rot.vkbd == bd)))
          {
             if (x) *x = bd->x;
             if (y) *y = bd->y;
             if (move) *move = EINA_FALSE;
          }

        res = EINA_TRUE;
     }

   if (res)
     {
        _x = 0; _y = 0; _w = 0; _h = 0;
        if (x) _x = *x;
        if (y) _y = *y;
        if (w) _w = *w;
        if (h) _h = *h;
        if (move) _move = *move;

        ELBF(ELBT_ROT, 1, bd->client.win,
             "GET SIZE_HINT[%d] %d,%d %dx%d move:%d",
             ang, _x, _y, _w, _h, _move);
     }

   return res;
}
#endif

static Eina_Bool
_e_border_post_move_resize_job(void *data)
{
   E_Border *bd;

   bd = (E_Border *)data;
   if (bd->post_move)
     {
        E_Border *tmp;
        Eina_List *l;

        EINA_LIST_FOREACH(bd->client.e.state.video_child, l, tmp)
          ecore_x_window_move(tmp->win,
                              bd->x +
                              bd->client_inset.l +
                              bd->fx.x +
                              tmp->client.e.state.video_position.x,
                              bd->y +
                              bd->client_inset.t +
                              bd->fx.y +
                              tmp->client.e.state.video_position.y);
     }
   if (bd->client.e.state.video)
     {
        E_Border *parent;

        parent = bd->client.e.state.video_parent_border;
        ecore_x_window_move(bd->win,
                            parent->x +
                            parent->client_inset.l +
                            parent->fx.x +
                            bd->client.e.state.video_position.x,
                            parent->y +
                            parent->client_inset.t +
                            parent->fx.y +
                            bd->client.e.state.video_position.y);
     }
   else if ((bd->post_move) && (bd->post_resize))
     {
        ecore_x_window_move_resize(bd->win,
                                   bd->x + bd->fx.x,
                                   bd->y + bd->fx.y,
                                   bd->w, bd->h);
     }
   else if (bd->post_move)
     {
        ecore_x_window_move(bd->win, bd->x + bd->fx.x, bd->y + bd->fx.y);
     }
   else if (bd->post_resize)
     {
        ecore_x_window_resize(bd->win, bd->w, bd->h);
     }

   if (bd->client.e.state.video)
     {
        fprintf(stderr, "%x: [%i, %i] [%i, %i]\n",
                bd->win,
                bd->client.e.state.video_parent_border->x +
                bd->client.e.state.video_parent_border->client_inset.l +
                bd->client.e.state.video_parent_border->fx.x +
                bd->client.e.state.video_position.x,
                bd->client.e.state.video_parent_border->y +
                bd->client.e.state.video_parent_border->client_inset.t +
                bd->client.e.state.video_parent_border->fx.y +
                bd->client.e.state.video_position.y,
                bd->w, bd->h);
     }

   if (bd->post_show)
     {
        if (bd->visible)
          {
             bd->post_job = NULL;
             _e_border_show(bd);
          }
     }
   bd->post_show = 0;
   bd->post_move = 0;
   bd->post_resize = 0;
   bd->post_job = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_border_container_layout_hook(E_Container *con)
{
   _e_border_hook_call(E_BORDER_HOOK_CONTAINER_LAYOUT, con);
}

static void
_e_border_eval0(E_Border *bd)
{
   int change_urgent = 0;
   int rem_change = 0;
   traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:BORDER:EVAL 0");
#ifdef _F_USE_DESK_WINDOW_PROFILE_
   Eina_Bool need_desk_set = EINA_FALSE;
#endif
#ifdef _F_E_WIN_AUX_HINT_
   Eina_Bool need_aux_hint_update = EINA_FALSE;
#endif /* end of _F_E_WIN_AUX_HINT_ */
#ifdef _F_ZONE_WINDOW_ROTATION_
   /* workaround: since transient_for is fetched by illume in hook,
    * added below flag to know this state in this eval time
    */
   Eina_Bool fetch_transient_for = EINA_FALSE;

   if (bd->client.icccm.fetch.transient_for)
     {
        if (e_config->wm_win_rotation)
          {
             if (((rot.vkbd) && (rot.vkbd == bd)) ||
                 ((rot.vkbd_prediction) && (rot.vkbd_prediction == bd)))
               {
                  bd->client.e.fetch.rot.need_rotation = EINA_TRUE;
                  ELB(ELBT_BD, "UPDATE TRANSIENT_FOR", bd->client.win);
               }
          }
        fetch_transient_for = EINA_TRUE;
     }
#endif
#ifdef _F_USE_TILED_DESK_LAYOUT_
   Eina_Bool need_desk_ly_set = EINA_FALSE;
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

   if (e_object_is_del(E_OBJECT(bd)))
     {
        CRI("_e_border_eval(%p) with deleted border!\n", bd);
        bd->changed = 0;
        traceEnd(TTRACE_TAG_WINDOW_MANAGER);
        return;
     }

   _e_border_hook_call(E_BORDER_HOOK_EVAL_PRE_FETCH, bd);

   bd->changes.border = 0;

   /* fetch any info queued to be fetched */
   if (bd->client.netwm.fetch.state)
     {
        e_hints_window_state_get(bd);
        bd->client.netwm.fetch.state = 0;
        rem_change = 1;
     }
   if (bd->client.icccm.fetch.client_leader)
     {
        /* TODO: What do to if the client leader isn't mapped yet? */
        E_Border *bd_leader = NULL;

        bd->client.icccm.client_leader = ecore_x_icccm_client_leader_get(bd->client.win);
        if (bd->client.icccm.client_leader)
          bd_leader = e_border_find_by_client_window(bd->client.icccm.client_leader);
        if (bd->leader)
          {
             if (bd->leader != bd_leader)
               {
                  bd->leader->group = eina_list_remove(bd->leader->group, bd);
                  if (bd->leader->modal == bd) bd->leader->modal = NULL;
                  bd->leader = NULL;
               }
             else
               bd_leader = NULL;
          }
        /* If this border is the leader of the group, don't register itself */
        if ((bd_leader) && (bd_leader != bd))
          {
             bd_leader->group = eina_list_append(bd_leader->group, bd);
             bd->leader = bd_leader;
             /* Only set the window modal to the leader it there is no parent */
             if ((e_config->modal_windows) && (bd->client.netwm.state.modal) &&
                 ((!bd->parent) || (bd->parent->modal != bd)))
               {
                  bd->leader->modal = bd;
                  if (bd->leader->focused)
                    e_border_focus_set(bd, 1, 1);
                  else
                    {
                       Eina_List *l;
                       E_Border *child;

                       EINA_LIST_FOREACH(bd->leader->group, l, child)
                         {
                            if ((child != bd) && (child->focused))
                              e_border_focus_set(bd, 1, 1);
                         }
                    }
               }
          }
        bd->client.icccm.fetch.client_leader = 0;
        rem_change = 1;
     }
   if (bd->client.icccm.fetch.title)
     {
        char *title = ecore_x_icccm_title_get(bd->client.win);
        eina_stringshare_replace(&bd->client.icccm.title, title);
        if (title) free(title);

        if (bd->bg_object)
          edje_object_part_text_set(bd->bg_object, "e.text.title",
                                    bd->client.icccm.title);
        bd->client.icccm.fetch.title = 0;
        rem_change = 1;
     }
   if (bd->client.netwm.fetch.name)
     {
        char *name;
        ecore_x_netwm_name_get(bd->client.win, &name);
        eina_stringshare_replace(&bd->client.netwm.name, name);
        if (name) free(name);

        if (bd->bg_object)
          edje_object_part_text_set(bd->bg_object, "e.text.title",
                                    bd->client.netwm.name);
        bd->client.netwm.fetch.name = 0;
        rem_change = 1;
     }
   if (bd->client.icccm.fetch.name_class)
     {
        const char *pname, *pclass;
        char *nname, *nclass;

        ecore_x_icccm_name_class_get(bd->client.win, &nname, &nclass);
        pname = bd->client.icccm.name;
        pclass = bd->client.icccm.class;
        bd->client.icccm.name = eina_stringshare_add(nname);
        bd->client.icccm.class = eina_stringshare_add(nclass);
        if (bd->client.icccm.class && (!strcmp(bd->client.icccm.class, "Vmplayer")))
          e_bindings_mapping_change_enable(EINA_FALSE);
#ifdef _F_ZONE_WINDOW_ROTATION_
        if (e_config->wm_win_rotation)
          {
             if ((bd->client.icccm.name) && (bd->client.icccm.class))
               {
                  if ((!strcmp(bd->client.icccm.name, "Virtual Keyboard")) &&
                      (!strcmp(bd->client.icccm.class, "ISF")))
                    {
                       ELB(ELBT_BD, "SET VKBD", bd->client.win);
                       bd->client.vkbd.win_type = E_VIRTUAL_KEYBOARD_WINDOW_TYPE_KEYPAD;
                       rot.vkbd = bd;
                    }
                  else if ((!strcmp(bd->client.icccm.name, "Prediction Window")) &&
                           (!strcmp(bd->client.icccm.class, "ISF")))
                    {
                       ELB(ELBT_BD, "SET PREDICTION", bd->client.win);
                       bd->client.vkbd.win_type = E_VIRTUAL_KEYBOARD_WINDOW_TYPE_PREDICTION;
                       rot.vkbd_prediction = bd;
                    }
                  else if ((!strcmp(bd->client.icccm.name, "Key Magnifier")) &&
                           (!strcmp(bd->client.icccm.class, "ISF")))
                    {
                       ELB(ELBT_BD, "SET MAGNIFIER", bd->client.win);
                       bd->client.vkbd.win_type = E_VIRTUAL_KEYBOARD_WINDOW_TYPE_MAGNIFIER;
                    }
                  else if ((!strcmp(bd->client.icccm.name, "ISF Popup")) &&
                           (!strcmp(bd->client.icccm.class, "ISF")))
                    {
                       ELB(ELBT_BD, "SET VKBD_POPUP", bd->client.win);
                       bd->client.vkbd.win_type = E_VIRTUAL_KEYBOARD_WINDOW_TYPE_POPUP;
                    }
               }
          }
#endif
        if (nname) free(nname);
        if (nclass) free(nclass);

        if (!((bd->client.icccm.name == pname) &&
              (bd->client.icccm.class == pclass)))
          bd->changes.icon = 1;

        if (pname) eina_stringshare_del(pname);
        if (pclass) eina_stringshare_del(pclass);
        bd->client.icccm.fetch.name_class = 0;
        bd->changes.icon = 1;
        rem_change = 1;
     }
   if (bd->client.icccm.fetch.state)
     {
        bd->client.icccm.state = ecore_x_icccm_state_get(bd->client.win);
        bd->client.icccm.fetch.state = 0;
        rem_change = 1;
     }
   if (bd->client.e.fetch.state)
     {
        e_hints_window_e_state_get(bd);
        bd->client.e.fetch.state = 0;
        rem_change = 1;
     }
#ifdef _F_USE_DESK_WINDOW_PROFILE_
   if (bd->client.e.fetch.profile)
     {
        const char **list = NULL;
        int n, i, res;
        unsigned int use;

        if (bd->client.e.state.profile.name)
          {
             eina_stringshare_del(bd->client.e.state.profile.name);
             bd->client.e.state.profile.name = NULL;
          }

        if (bd->client.e.state.profile.available_list)
          {
             for (i = 0; i < bd->client.e.state.profile.num; i++)
               {
                  if (bd->client.e.state.profile.available_list[i])
                    {
                       eina_stringshare_del(bd->client.e.state.profile.available_list[i]);
                       bd->client.e.state.profile.available_list[i] = NULL;
                    }
               }
             E_FREE(bd->client.e.state.profile.available_list);
             bd->client.e.state.profile.available_list = NULL;
          }
        bd->client.e.state.profile.num = 0;

        res = ecore_x_window_prop_card32_get(bd->client.win,
                                             ECORE_X_ATOM_E_WINDOW_PROFILE_SUPPORTED,
                                             &use,
                                             1);
        if ((res == 1) && (use == 1))
          {
             Ecore_X_Atom val;
             res = ecore_x_window_prop_atom_get(bd->client.win,
                                                ECORE_X_ATOM_E_WINDOW_PROFILE_CHANGE,
                                                &val, 1);
             if (res == 1)
               {
                  char *name = ecore_x_atom_name_get(val);
                  if (name)
                    {
                       bd->client.e.state.profile.name = eina_stringshare_add(name);
                       free(name);
                    }
               }

             if (ecore_x_e_window_available_profiles_get(bd->client.win, &list, &n))
               {
                  bd->client.e.state.profile.available_list = E_NEW(const char *, n);
                  for (i = 0; i < n; i++)
                    {
                       bd->client.e.state.profile.available_list[i] = eina_stringshare_add(list[i]);

                       if (list[i])
                          free(list[i]);
                    }
                  bd->client.e.state.profile.num = n;
               }
             need_desk_set = EINA_TRUE;
             bd->client.e.state.profile.use = 1;

             if (list)
               free(list);
          }

        bd->client.e.fetch.profile = 0;
     }
#endif
#ifdef _F_ZONE_WINDOW_ROTATION_
   if ((e_config->wm_win_rotation) &&
       (bd->client.e.fetch.rot.support))
     {
        int ret = 0;
        unsigned int support = 0;

        ret = ecore_x_window_prop_card32_get
          (bd->client.win,
          ECORE_X_ATOM_E_WINDOW_ROTATION_SUPPORTED,
          &support, 1);

        bd->client.e.state.rot.support = 0;
        if ((ret == 1) && (support == 1))
          {
             int ang = -1;

             ang = _prev_angle_get(bd->client.win);
             if (ang != -1)
               {
                  bd->client.e.state.rot.ang.curr = ang;
                  ELBF(ELBT_ROT, 1, bd->client.win, "prev_ang:%d", ang);
               }
             bd->client.e.state.rot.support = 1;
          }

        if (bd->client.e.state.rot.support)
          bd->client.e.fetch.rot.need_rotation = EINA_TRUE;

        bd->client.e.fetch.rot.support = 0;
     }
   if ((e_config->wm_win_rotation) &&
       (bd->client.e.fetch.rot.geom_hint))
     {
        Eina_Rectangle r[4];
        int i, x, y, w, h;
        bd->client.e.state.rot.geom_hint = 0;
        for (i = 0; i < 4; i++)
          {
             r[i].x = bd->client.e.state.rot.geom[i].x;
             r[i].y = bd->client.e.state.rot.geom[i].y;
             r[i].w = bd->client.e.state.rot.geom[i].w;
             r[i].h = bd->client.e.state.rot.geom[i].h;

             bd->client.e.state.rot.geom[i].x = 0;
             bd->client.e.state.rot.geom[i].y = 0;
             bd->client.e.state.rot.geom[i].w = 0;
             bd->client.e.state.rot.geom[i].h = 0;
          }

        for (i = 0; i < 4; i++)
          {
             x = 0; y = 0; w = 0; h = 0;
             if (ecore_x_e_window_rotation_geometry_get(bd->client.win, i*90, &x, &y, &w, &h))
               {
                  bd->client.e.state.rot.geom_hint = 1;
                  bd->client.e.state.rot.geom[i].x = x;
                  bd->client.e.state.rot.geom[i].y = y;
                  bd->client.e.state.rot.geom[i].w = w;
                  bd->client.e.state.rot.geom[i].h = h;

                  if (!((r[i].x == x) && (r[i].y == y) &&
                        (r[i].w == w) && (r[i].h == h)))
                    {
                       bd->client.e.fetch.rot.need_rotation = EINA_TRUE;
                    }
               }
          }
        bd->client.e.fetch.rot.geom_hint = 0;
     }
   if ((e_config->wm_win_rotation) &&
       (bd->client.e.fetch.rot.app_set))
     {
        ELB(ELBT_ROT, "Fetch ROT_APP_SET", bd->client.win);
        unsigned char _prev_app_set = bd->client.e.state.rot.app_set;
        bd->client.e.state.rot.app_set = ecore_x_e_window_rotation_app_get(bd->client.win);

        if (_prev_app_set != bd->client.e.state.rot.app_set)
          {
             if (bd->client.e.state.rot.app_set)
               {
                  int ang = -1;

                  ang = _prev_angle_get(bd->client.win);
                  if (ang != -1)
                    {
                       bd->client.e.state.rot.ang.curr = ang;
                       ELBF(ELBT_ROT, 1, bd->client.win, "prev_ang:%d", ang);
                    }
               }
             bd->client.e.fetch.rot.need_rotation = EINA_TRUE;
          }

        bd->client.e.fetch.rot.app_set = 0;
     }
   if ((e_config->wm_win_rotation) &&
       (bd->client.e.fetch.rot.preferred_rot))
     {
        int r = 0, _prev_preferred_rot;
        _prev_preferred_rot = bd->client.e.state.rot.preferred_rot;
        bd->client.e.state.rot.preferred_rot = -1;
        if (ecore_x_e_window_rotation_preferred_rotation_get(bd->client.win, &r))
          {
             bd->client.e.state.rot.preferred_rot = r;
             ELBF(ELBT_ROT, 0, bd->client.win, "Fetch PREFERRED_ROT:%d", r);
          }
        else
          {
             ELB(ELBT_ROT, "Fetch PREFERRED_ROT Del..", bd->client.win);
          }

        if (_prev_preferred_rot != bd->client.e.state.rot.preferred_rot)
          bd->client.e.fetch.rot.need_rotation = EINA_TRUE;

        bd->client.e.fetch.rot.preferred_rot = 0;
     }
   if ((e_config->wm_win_rotation) &&
       (bd->client.e.fetch.rot.available_rots))
     {
        Eina_Bool res, diff = EINA_FALSE;
        int *rots = NULL;
        unsigned int count = 0, i = 0;
        int _prev_rots[4] = { -1, };

        if (bd->client.e.state.rot.available_rots)
          {
             memcpy(_prev_rots,
                    bd->client.e.state.rot.available_rots,
                    (sizeof(int) * bd->client.e.state.rot.count));

             E_FREE(bd->client.e.state.rot.available_rots);
          }

        bd->client.e.state.rot.count = 0;

        res = ecore_x_e_window_rotation_available_rotations_get(bd->client.win,
                                                                &rots, &count);
        if ((res) && (count > 0) && (rots))
          {
             bd->client.e.state.rot.available_rots = rots;
             bd->client.e.state.rot.count = count;

             for (i = 0; i < count; i++)
               {
                  ELBF(ELBT_ROT, 0, bd->client.win, "Fetch AVAILABLE_ROTS[%d]:%d", i, rots[i]);
                  if ((!diff) && (_prev_rots[i] != rots[i]))
                    {
                       ELBF(ELBT_ROT, 0, bd->client.win, "count:%d i:%d _prev:%d != rot:%d",
                            count, i, _prev_rots[i], rots[i]);
                       diff = EINA_TRUE;
                    }
               }
          }
        else
          {
             ELB(ELBT_ROT, "Fetch AVAILABLE_ROTS Del..", bd->client.win);
             diff = EINA_TRUE;
          }

        if (diff) bd->client.e.fetch.rot.need_rotation = EINA_TRUE;
        bd->client.e.fetch.rot.available_rots = 0;
     }
#endif
#ifdef _F_USE_TILED_DESK_LAYOUT_
   if (e_config->use_tiled_desk_layout)
     {
        if (bd->client.e.fetch.ly.support)
          {
             unsigned char prev = bd->client.e.state.ly.support;
             Eina_Bool res;
             int val = -1;

             res = _prop_card32_get(bd->client.win,
                                    E_ATOM_WINDOW_DESKTOP_LAYOUT_SUPPORTED,
                                    &val);
             if ((res) && (val != -1))
               bd->client.e.state.ly.support = (unsigned char)val;
             else
               bd->client.e.state.ly.support = 0;

             if (prev != bd->client.e.state.ly.support)
               need_desk_ly_set = EINA_TRUE;

             ELBF(ELBT_DESK_LY, 0, bd->client.win, "Fetch LY_SUPPORT support:%d",
                  bd->client.e.state.ly.support);

             bd->client.e.fetch.ly.support = 0;
          }
     }
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */
#ifdef _F_E_WIN_AUX_HINT_
   if (e_config->win_aux_hint)
     {
        if (bd->client.e.fetch.aux_hint.support)
          {
             unsigned char prev = bd->client.e.state.aux_hint.support;
             unsigned int support = 0;
             int ret = 0;

             ret = ecore_x_window_prop_card32_get
               (bd->client.win,
                ECORE_X_ATOM_E_WINDOW_AUX_HINT_SUPPORT,
                &support, 1);

             bd->client.e.state.aux_hint.support = 0;
             if ((ret == 1) && (support == 1))
               {
                  bd->client.e.state.aux_hint.support = 1;
               }

             if (prev != bd->client.e.state.aux_hint.support)
               {
                  need_aux_hint_update = EINA_TRUE;
               }

             bd->client.e.fetch.aux_hint.support = 0;
          }
        if (bd->client.e.fetch.aux_hint.hints)
          {
             need_aux_hint_update = EINA_TRUE;
          }
     }
#endif /* end of _F_E_WIN_AUX_HINT_ */
   if (bd->client.netwm.fetch.type)
     {
        e_hints_window_type_get(bd);
        if ((!bd->lock_border) || (!bd->client.border.name))
          bd->client.border.changed = 1;

        if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DOCK)
          {
             if (!bd->client.netwm.state.skip_pager)
               {
                  bd->client.netwm.state.skip_pager = 1;
                  bd->client.netwm.update.state = 1;
               }
             if (!bd->client.netwm.state.skip_taskbar)
               {
                  bd->client.netwm.state.skip_taskbar = 1;
                  bd->client.netwm.update.state = 1;
               }
          }
        bd->client.netwm.fetch.type = 0;
     }
   if (bd->client.icccm.fetch.machine)
     {
        char *machine = ecore_x_icccm_client_machine_get(bd->client.win);

        if ((!machine) && (bd->client.icccm.client_leader))
          machine = ecore_x_icccm_client_machine_get(bd->client.icccm.client_leader);

        eina_stringshare_replace(&bd->client.icccm.machine, machine);
        if (machine) free(machine);

        bd->client.icccm.fetch.machine = 0;
        rem_change = 1;
     }
   if (bd->client.icccm.fetch.command)
     {
        if ((bd->client.icccm.command.argc > 0) && (bd->client.icccm.command.argv))
          {
             int i;

             for (i = 0; i < bd->client.icccm.command.argc; i++)
               free(bd->client.icccm.command.argv[i]);
             free(bd->client.icccm.command.argv);
          }
        bd->client.icccm.command.argc = 0;
        bd->client.icccm.command.argv = NULL;
        ecore_x_icccm_command_get(bd->client.win,
                                  &(bd->client.icccm.command.argc),
                                  &(bd->client.icccm.command.argv));
        if ((bd->client.icccm.client_leader) &&
            (!bd->client.icccm.command.argv))
          ecore_x_icccm_command_get(bd->client.icccm.client_leader,
                                    &(bd->client.icccm.command.argc),
                                    &(bd->client.icccm.command.argv));
        bd->client.icccm.fetch.command = 0;
        rem_change = 1;
     }
   if (bd->client.icccm.fetch.hints)
     {
        Eina_Bool accepts_focus, is_urgent;

        accepts_focus = EINA_TRUE;
        is_urgent = EINA_FALSE;
        bd->client.icccm.initial_state = ECORE_X_WINDOW_STATE_HINT_NORMAL;
        if (ecore_x_icccm_hints_get(bd->client.win,
                                    &accepts_focus,
                                    &bd->client.icccm.initial_state,
                                    &bd->client.icccm.icon_pixmap,
                                    &bd->client.icccm.icon_mask,
                                    &bd->client.icccm.icon_window,
                                    &bd->client.icccm.window_group,
                                    &is_urgent))
          {
             bd->client.icccm.accepts_focus = accepts_focus;
#ifdef _F_FOCUS_WINDOW_IF_TOP_STACK_
             if (!accepts_focus)
               {
                  if (bd->focused)
                     _e_border_latest_stacked_focus(bd);
               }
#endif

             if ((bd->client.icccm.urgent != is_urgent) && ((!bd->focused) || (!is_urgent)))
               change_urgent = 1;
             bd->client.icccm.urgent = is_urgent;

             /* If this is a new window, set the state as requested. */
             if ((bd->new_client) &&
                 (bd->client.icccm.initial_state == ECORE_X_WINDOW_STATE_HINT_ICONIC))
               {
                  e_border_iconify(bd);
                  e_border_hide(bd, 1);
               }
          }
        bd->client.icccm.fetch.hints = 0;
        rem_change = 1;
     }
   if (bd->client.icccm.fetch.size_pos_hints)
     {
        Eina_Bool request_pos;

        request_pos = EINA_FALSE;
        if (ecore_x_icccm_size_pos_hints_get(bd->client.win,
                                             &request_pos,
                                             &bd->client.icccm.gravity,
                                             &bd->client.icccm.min_w,
                                             &bd->client.icccm.min_h,
                                             &bd->client.icccm.max_w,
                                             &bd->client.icccm.max_h,
                                             &bd->client.icccm.base_w,
                                             &bd->client.icccm.base_h,
                                             &bd->client.icccm.step_w,
                                             &bd->client.icccm.step_h,
                                             &bd->client.icccm.min_aspect,
                                             &bd->client.icccm.max_aspect))
          {
             bd->client.icccm.request_pos = request_pos;
          }
        else
          {
          }
        if (bd->client.icccm.min_w > 32767) bd->client.icccm.min_w = 32767;
        if (bd->client.icccm.min_h > 32767) bd->client.icccm.min_h = 32767;
        if (bd->client.icccm.max_w > 32767) bd->client.icccm.max_w = 32767;
        if (bd->client.icccm.max_h > 32767) bd->client.icccm.max_h = 32767;
        if (bd->client.icccm.base_w > 32767) bd->client.icccm.base_w = 32767;
        if (bd->client.icccm.base_h > 32767) bd->client.icccm.base_h = 32767;
        //	if (bd->client.icccm.step_w < 1) bd->client.icccm.step_w = 1;
        //	if (bd->client.icccm.step_h < 1) bd->client.icccm.step_h = 1;
        // if doing a resize, fix it up
        if (bd->resize_mode != RESIZE_NONE)
          {
             int x, y, w, h, new_w, new_h;

             x = bd->x;
             y = bd->y;
             w = bd->w;
             h = bd->h;
             new_w = w;
             new_h = h;
             e_border_resize_limit(bd, &new_w, &new_h);
             if ((bd->resize_mode == RESIZE_TL) ||
                 (bd->resize_mode == RESIZE_L) ||
                 (bd->resize_mode == RESIZE_BL))
               x += (w - new_w);
             if ((bd->resize_mode == RESIZE_TL) ||
                 (bd->resize_mode == RESIZE_T) ||
                 (bd->resize_mode == RESIZE_TR))
               y += (h - new_h);
             e_border_move_resize(bd, x, y, new_w, new_h);
          }
        bd->client.icccm.fetch.size_pos_hints = 0;
        rem_change = 1;
     }
   if (bd->client.icccm.fetch.protocol)
     {
        int i, num;
        Ecore_X_WM_Protocol *proto;

        proto = ecore_x_window_prop_protocol_list_get(bd->client.win, &num);
        if (proto)
          {
             for (i = 0; i < num; i++)
               {
                  if (proto[i] == ECORE_X_WM_PROTOCOL_DELETE_REQUEST)
                    bd->client.icccm.delete_request = 1;
                  else if (proto[i] == ECORE_X_WM_PROTOCOL_TAKE_FOCUS)
                    bd->client.icccm.take_focus = 1;
                  else if (proto[i] == ECORE_X_NET_WM_PROTOCOL_PING)
                    bd->client.netwm.ping = 1;
                  else if (proto[i] == ECORE_X_NET_WM_PROTOCOL_SYNC_REQUEST)
                    {
                       bd->client.netwm.sync.request = 1;
                       if (!ecore_x_netwm_sync_counter_get(bd->client.win,
                                                           &bd->client.netwm.sync.counter))
                         bd->client.netwm.sync.request = 0;
                    }
               }
             free(proto);
          }
        if (bd->client.netwm.ping)
          e_border_ping(bd);
        else
          {
             if (bd->ping_poller) ecore_poller_del(bd->ping_poller);
             bd->ping_poller = NULL;
          }
        bd->client.icccm.fetch.protocol = 0;
     }
   if (bd->client.icccm.fetch.transient_for)
     {
        /* TODO: What do to if the transient for isn't mapped yet? */
        E_Border *bd_parent = NULL;
#ifdef _F_DEICONIFY_APPROVE_
        Eina_Bool change_parent = EINA_FALSE;
#endif

        bd->client.icccm.transient_for = ecore_x_icccm_transient_for_get(bd->client.win);
        if (bd->client.icccm.transient_for)
          bd_parent = e_border_find_by_client_window(bd->client.icccm.transient_for);
        /* If we already have a parent, remove it */
        if (bd->parent)
          {
             if (bd_parent != bd->parent)
               {
                  bd->parent->transients = eina_list_remove(bd->parent->transients, bd);
                  if (bd->parent->modal == bd) bd->parent->modal = NULL;
                  bd->parent = NULL;
               }
             else
               bd_parent = NULL;
          }
        if ((bd_parent) && (bd_parent != bd) &&
            (eina_list_data_find(bd->transients, bd_parent) != bd_parent))
          {
             bd_parent->transients = eina_list_append(bd_parent->transients, bd);
             bd->parent = bd_parent;
#ifdef _F_DEICONIFY_APPROVE_
             change_parent = EINA_TRUE;
#endif
          }
        if (bd->parent)
          {
             e_border_layer_set(bd, bd->parent->layer);
             if ((e_config->modal_windows) && (bd->client.netwm.state.modal))
               {
                  Ecore_X_Window_Attributes attr;
                  bd->parent->modal = bd;
                  ecore_x_window_attributes_get(bd->parent->client.win, &attr);
                  if ((!bd->parent->saved.event_mask) ||
                      (attr.event_mask.mine != (ECORE_X_EVENT_MASK_WINDOW_DAMAGE | ECORE_X_EVENT_MASK_WINDOW_PROPERTY)))
                    {
                       bd->parent->saved.event_mask = attr.event_mask.mine;
                       bd->parent->lock_close = 1;
                       ecore_x_event_mask_unset(bd->parent->client.win, attr.event_mask.mine);
                       ecore_x_event_mask_set(bd->parent->client.win, ECORE_X_EVENT_MASK_WINDOW_DAMAGE | ECORE_X_EVENT_MASK_WINDOW_PROPERTY);
                    }
               }

             if (e_config->focus_setting == E_FOCUS_NEW_DIALOG ||
                 (bd->parent->focused && (e_config->focus_setting == E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED)))
               bd->take_focus = 1;
          }

#ifdef _F_DEICONIFY_APPROVE_
        if (change_parent)
          {
             bd->client.e.state.deiconify_approve.render_done = 0;

             E_Border *ancestor_bd;
             ancestor_bd = bd->client.e.state.deiconify_approve.ancestor;
             if ((ancestor_bd) &&
                 (!e_object_is_del(E_OBJECT(ancestor_bd))))
               {
                  ancestor_bd->client.e.state.deiconify_approve.req_list = eina_list_remove(ancestor_bd->client.e.state.deiconify_approve.req_list, bd);
                  bd->client.e.state.deiconify_approve.ancestor = NULL;

                  if ((ancestor_bd->client.e.state.deiconify_approve.req_list == NULL) &&
                      (ancestor_bd->client.e.state.deiconify_approve.render_done))
                    {
                       if (ancestor_bd->client.e.state.deiconify_approve.wait_timer)
                         {
                            ecore_timer_del(ancestor_bd->client.e.state.deiconify_approve.wait_timer);
                            ancestor_bd->client.e.state.deiconify_approve.wait_timer = NULL;
                            _e_border_uniconify_pending_list_remove(ancestor_bd);
                         }
                    }
               }
          }
#endif
        if ((bd_parent) ||
            ((!bd_parent) && (!bd->client.icccm.transient_for)))
          bd->client.icccm.fetch.transient_for = 0;
        rem_change = 1;
     }
   if (bd->client.icccm.fetch.window_role)
     {
        char *role = ecore_x_icccm_window_role_get(bd->client.win);
        eina_stringshare_replace(&bd->client.icccm.window_role, role);
        if (role) free(role);

        bd->client.icccm.fetch.window_role = 0;
        rem_change = 1;
     }
   if (bd->client.icccm.fetch.icon_name)
     {
        char *icon_name = ecore_x_icccm_icon_name_get(bd->client.win);
        eina_stringshare_replace(&bd->client.icccm.icon_name, icon_name);
        if (icon_name) free(icon_name);

        bd->client.icccm.fetch.icon_name = 0;
        rem_change = 1;
     }
   if (bd->client.netwm.fetch.icon_name)
     {
        char *icon_name;
        ecore_x_netwm_icon_name_get(bd->client.win, &icon_name);
        eina_stringshare_replace(&bd->client.netwm.icon_name, icon_name);
        if (icon_name) free(icon_name);

        bd->client.netwm.fetch.icon_name = 0;
        rem_change = 1;
     }
   if (bd->client.netwm.fetch.icon)
     {
        int i;
        if (bd->client.netwm.icons)
          {
             for (i = 0; i < bd->client.netwm.num_icons; i++)
               {
                  free(bd->client.netwm.icons[i].data);
                  bd->client.netwm.icons[i].data = NULL;
               }
             free(bd->client.netwm.icons);
          }
        bd->client.netwm.icons = NULL;
        bd->client.netwm.num_icons = 0;
        if (ecore_x_netwm_icons_get(bd->client.win,
                                     &bd->client.netwm.icons,
                                     &bd->client.netwm.num_icons))
          {
             // unless the rest of e17 uses border icons OTHER than icon #0
             // then free the rest that we don't need anymore.
             for (i = 1; i < bd->client.netwm.num_icons; i++)
               {
                  free(bd->client.netwm.icons[i].data);
                  bd->client.netwm.icons[i].data = NULL;
               }
             bd->client.netwm.num_icons = 1;
             bd->changes.icon = 1;
          }
        bd->client.netwm.fetch.icon = 0;
     }
   if (bd->client.netwm.fetch.user_time)
     {
        ecore_x_netwm_user_time_get(bd->client.win, &bd->client.netwm.user_time);
        bd->client.netwm.fetch.user_time = 0;
     }
   if (bd->client.netwm.fetch.strut)
     {
        if (!ecore_x_netwm_strut_partial_get(bd->client.win,
                                             &bd->client.netwm.strut.left,
                                             &bd->client.netwm.strut.right,
                                             &bd->client.netwm.strut.top,
                                             &bd->client.netwm.strut.bottom,
                                             &bd->client.netwm.strut.left_start_y,
                                             &bd->client.netwm.strut.left_end_y,
                                             &bd->client.netwm.strut.right_start_y,
                                             &bd->client.netwm.strut.right_end_y,
                                             &bd->client.netwm.strut.top_start_x,
                                             &bd->client.netwm.strut.top_end_x,
                                             &bd->client.netwm.strut.bottom_start_x,
                                             &bd->client.netwm.strut.bottom_end_x))
          {
             ecore_x_netwm_strut_get(bd->client.win,
                                     &bd->client.netwm.strut.left, &bd->client.netwm.strut.right,
                                     &bd->client.netwm.strut.top, &bd->client.netwm.strut.bottom);

             bd->client.netwm.strut.left_start_y = 0;
             bd->client.netwm.strut.left_end_y = 0;
             bd->client.netwm.strut.right_start_y = 0;
             bd->client.netwm.strut.right_end_y = 0;
             bd->client.netwm.strut.top_start_x = 0;
             bd->client.netwm.strut.top_end_x = 0;
             bd->client.netwm.strut.bottom_start_x = 0;
             bd->client.netwm.strut.bottom_end_x = 0;
          }
        bd->client.netwm.fetch.strut = 0;
     }
   if (bd->client.qtopia.fetch.soft_menu)
     {
        e_hints_window_qtopia_soft_menu_get(bd);
        bd->client.qtopia.fetch.soft_menu = 0;
        rem_change = 1;
     }
   if (bd->client.qtopia.fetch.soft_menus)
     {
        e_hints_window_qtopia_soft_menus_get(bd);
        bd->client.qtopia.fetch.soft_menus = 0;
        rem_change = 1;
     }
   if (bd->client.vkbd.fetch.state)
     {
        e_hints_window_virtual_keyboard_state_get(bd);
        bd->client.vkbd.fetch.state = 0;
        rem_change = 1;
     }
   if (bd->client.vkbd.fetch.vkbd)
     {
        e_hints_window_virtual_keyboard_get(bd);
        bd->client.vkbd.fetch.vkbd = 0;
        rem_change = 1;
     }
   if (bd->client.illume.conformant.fetch.conformant)
     {
        bd->client.illume.conformant.conformant =
          ecore_x_e_illume_conformant_get(bd->client.win);
        bd->client.illume.conformant.fetch.conformant = 0;
     }
   if (bd->client.illume.quickpanel.fetch.state)
     {
        bd->client.illume.quickpanel.state =
          ecore_x_e_illume_quickpanel_state_get(bd->client.win);
        bd->client.illume.quickpanel.fetch.state = 0;
     }
   if (bd->client.illume.quickpanel.fetch.quickpanel)
     {
        bd->client.illume.quickpanel.quickpanel =
          ecore_x_e_illume_quickpanel_get(bd->client.win);
        bd->client.illume.quickpanel.fetch.quickpanel = 0;
     }
   if (bd->client.illume.quickpanel.fetch.priority.major)
     {
        bd->client.illume.quickpanel.priority.major =
          ecore_x_e_illume_quickpanel_priority_major_get(bd->client.win);
        bd->client.illume.quickpanel.fetch.priority.major = 0;
     }
   if (bd->client.illume.quickpanel.fetch.priority.minor)
     {
        bd->client.illume.quickpanel.priority.minor =
          ecore_x_e_illume_quickpanel_priority_minor_get(bd->client.win);
        bd->client.illume.quickpanel.fetch.priority.minor = 0;
     }
   if (bd->client.illume.quickpanel.fetch.zone)
     {
        bd->client.illume.quickpanel.zone =
          ecore_x_e_illume_quickpanel_zone_get(bd->client.win);
        bd->client.illume.quickpanel.fetch.zone = 0;
     }
   if (bd->client.illume.drag.fetch.drag)
     {
        bd->client.illume.drag.drag =
          ecore_x_e_illume_drag_get(bd->client.win);
        bd->client.illume.drag.fetch.drag = 0;
     }
   if (bd->client.illume.drag.fetch.locked)
     {
        bd->client.illume.drag.locked =
          ecore_x_e_illume_drag_locked_get(bd->client.win);
        bd->client.illume.drag.fetch.locked = 0;
     }
   if (bd->client.illume.win_state.fetch.state)
     {
        bd->client.illume.win_state.state =
           ecore_x_e_illume_window_state_get(bd->client.win);
        bd->client.illume.win_state.fetch.state = 0;
     }
   if (bd->changes.shape)
     {
        Ecore_X_Rectangle *rects;
        int num;

        bd->changes.shape = 0;
        rects = ecore_x_window_shape_rectangles_get(bd->client.win, &num);
        if (rects)
          {
             int cw = 0, ch = 0;

             /* This doesn't fix the race, but makes it smaller. we detect
              * this and if cw and ch != client w/h then mark this as needing
              * a shape change again to fixup next event loop.
              */
             ecore_x_window_size_get(bd->client.win, &cw, &ch);
             if ((cw != bd->client.w) || (ch != bd->client.h))
               bd->changes.shape = 1;
             if ((num == 1) &&
                 (rects[0].x == 0) &&
                 (rects[0].y == 0) &&
                 ((int)rects[0].width == cw) &&
                 ((int)rects[0].height == ch))
               {
                  if (bd->client.shaped)
                    {
                       bd->client.shaped = 0;
                       if (!bd->bordername)
                         bd->client.border.changed = 1;
                    }
               }
             else
               {
                  if (!bd->client.shaped)
                    {
                       bd->client.shaped = 1;
                       if (!bd->bordername)
                         bd->client.border.changed = 1;
                    }
               }
             free(rects);
          }
        else
          {
             // FIXME: no rects i think can mean... totally empty window
             bd->client.shaped = 0;
             if (!bd->bordername)
               bd->client.border.changed = 1;
          }
        bd->need_shape_merge = 1;
     }
   if (bd->changes.shape_input)
     {
        Ecore_X_Rectangle *rects;
        int num;

        bd->changes.shape_input = 0;
        rects = ecore_x_window_shape_input_rectangles_get(bd->client.win, &num);
        if (rects)
          {
             int cw = 0, ch = 0;

             /* This doesn't fix the race, but makes it smaller. we detect
              * this and if cw and ch != client w/h then mark this as needing
              * a shape change again to fixup next event loop.
              */
             ecore_x_window_size_get(bd->client.win, &cw, &ch);
             if ((cw != bd->client.w) || (ch != bd->client.h))
               bd->changes.shape_input = 1;
             if ((num == 1) &&
                 (rects[0].x == 0) &&
                 (rects[0].y == 0) &&
                 ((int)rects[0].width == cw) &&
                 ((int)rects[0].height == ch))
               {
                  if (bd->shaped_input)
                    {
                       bd->shaped_input = 0;
                       if (!bd->bordername)
                         bd->client.border.changed = 1;
                    }
               }
             else
               {
                  if (!bd->shaped_input)
                    {
                       bd->shaped_input = 1;
                       if (!bd->bordername)
                         bd->client.border.changed = 1;
                    }
               }
             free(rects);
          }
        else
          {
             bd->shaped_input = 1;
             if (!bd->bordername)
               bd->client.border.changed = 1;
          }
        bd->need_shape_merge = 1;
     }
   if (bd->client.mwm.fetch.hints)
     {
        int pb;

        bd->client.mwm.exists =
          ecore_x_mwm_hints_get(bd->client.win,
                                &bd->client.mwm.func,
                                &bd->client.mwm.decor,
                                &bd->client.mwm.input);
        pb = bd->client.mwm.borderless;
        bd->client.mwm.borderless = 0;
        if (bd->client.mwm.exists)
          {
             if ((!(bd->client.mwm.decor & ECORE_X_MWM_HINT_DECOR_ALL)) &&
                 (!(bd->client.mwm.decor & ECORE_X_MWM_HINT_DECOR_TITLE)) &&
                 (!(bd->client.mwm.decor & ECORE_X_MWM_HINT_DECOR_BORDER)))
               bd->client.mwm.borderless = 1;
          }
        if (bd->client.mwm.borderless != pb)
          {
             if ((!bd->lock_border) || (!bd->client.border.name))
               bd->client.border.changed = 1;
          }
        bd->client.mwm.fetch.hints = 0;
        rem_change = 1;
     }
   if (bd->client.e.fetch.video_parent)
     {
        /* unlinking child/parent */
        if (bd->client.e.state.video_parent_border != NULL)
          {
             bd->client.e.state.video_parent_border->client.e.state.video_child =
               eina_list_remove
               (bd->client.e.state.video_parent_border->client.e.state.video_child,
                   bd);
          }

        ecore_x_window_prop_card32_get(bd->client.win,
                                       ECORE_X_ATOM_E_VIDEO_PARENT,
                                       &bd->client.e.state.video_parent,
                                       1);

        /* linking child/parent */
        if (bd->client.e.state.video_parent != 0)
          {
             E_Border *tmp;
             Eina_List *l;

             EINA_LIST_FOREACH(borders, l, tmp)
               if (tmp->client.win == bd->client.e.state.video_parent)
                 {
                    /* fprintf(stderr, "child added to parent \\o/\n"); */
                    bd->client.e.state.video_parent_border = tmp;
                    tmp->client.e.state.video_child = eina_list_append(tmp->client.e.state.video_child,
                                                                       bd);
		    if (bd->desk != tmp->desk)
		      e_border_desk_set(bd, tmp->desk);
                    break;
                 }
          }

        /* fprintf(stderr, "new parent %x => %p\n", bd->client.e.state.video_parent, bd->client.e.state.video_parent_border); */

        if (bd->client.e.state.video_parent_border) bd->client.e.fetch.video_parent = 0;
	rem_change = 1;
     }
   if (bd->client.e.fetch.video_position && bd->client.e.fetch.video_parent == 0)
     {
        unsigned int xy[2];

        ecore_x_window_prop_card32_get(bd->client.win,
                                       ECORE_X_ATOM_E_VIDEO_POSITION,
                                       xy,
                                       2);
        bd->client.e.state.video_position.x = xy[0];
        bd->client.e.state.video_position.y = xy[1];
        bd->client.e.state.video_position.updated = 1;
        bd->client.e.fetch.video_position = 0;
        bd->x = bd->client.e.state.video_position.x;
        bd->y = bd->client.e.state.video_position.y;

        fprintf(stderr, "internal position has been updated [%i, %i]\n", bd->client.e.state.video_position.x, bd->client.e.state.video_position.y);
     }
   if (bd->client.netwm.update.state)
     {
        e_hints_window_state_set(bd);
        /* Some stats might change the border, like modal */
        if (((!bd->lock_border) || (!bd->client.border.name)) &&
            (!(((bd->maximized & E_MAXIMIZE_TYPE) == E_MAXIMIZE_FULLSCREEN))))
          {
             bd->client.border.changed = 1;
          }
        if (bd->parent)
          {
             if ((e_config->modal_windows) && (bd->client.netwm.state.modal))
               {
                  Ecore_X_Window_Attributes attr;
                  ecore_x_window_attributes_get(bd->parent->client.win, &attr);
                  if ((!bd->parent->saved.event_mask) ||
                      (attr.event_mask.mine != (ECORE_X_EVENT_MASK_WINDOW_DAMAGE | ECORE_X_EVENT_MASK_WINDOW_PROPERTY)))
                    {
                       bd->parent->saved.event_mask = attr.event_mask.mine;
                       ecore_x_event_mask_unset(bd->parent->client.win, attr.event_mask.mine);
                       ecore_x_event_mask_set(bd->parent->client.win, ECORE_X_EVENT_MASK_WINDOW_DAMAGE | ECORE_X_EVENT_MASK_WINDOW_PROPERTY);
                    }
                  bd->parent->modal = bd;
                  if (bd->parent->focused)
                    e_border_focus_set(bd, 1, 1);
               }
          }
        else if (bd->leader)
          {
             if ((e_config->modal_windows) && (bd->client.netwm.state.modal))
               {
                  bd->leader->modal = bd;
                  if (bd->leader->focused)
                    e_border_focus_set(bd, 1, 1);
                  else
                    {
                       Eina_List *l;
                       E_Border *child;

                       EINA_LIST_FOREACH(bd->leader->group, l, child)
                         {
                            if ((child != bd) && (child->focused))
                              e_border_focus_set(bd, 1, 1);
                         }
                    }
               }
          }
        bd->client.netwm.update.state = 0;
     }

   if (bd->new_client)
     {
        E_Event_Border_Add *ev;
        E_Exec_Instance *inst;

        ev = E_NEW(E_Event_Border_Add, 1);
        if (ev)
          {
             ev->border = bd;
             e_object_ref(E_OBJECT(bd));
             //	e_object_breadcrumb_add(E_OBJECT(bd), "border_add_event");
             ecore_event_add(E_EVENT_BORDER_ADD, ev, _e_border_event_border_add_free, NULL);
          }

        if ((!bd->lock_border) || (!bd->client.border.name))
          bd->client.border.changed = 1;

          {
             char *str = NULL;

             if ((ecore_x_netwm_startup_id_get(bd->client.win, &str) && (str)) ||
                 ((bd->client.icccm.client_leader > 0) &&
                     ecore_x_netwm_startup_id_get(bd->client.icccm.client_leader, &str) && (str))
                )
               {
                  if (!strncmp(str, "E_START|", 8))
                    {
                       int id;

                       id = atoi(str + 8);
                       if (id > 0) bd->client.netwm.startup_id = id;
                    }
                  free(str);
               }
          }
        /* It's ok not to have fetch flag, should only be set on startup
         *     * and not changed. */
        if (!ecore_x_netwm_pid_get(bd->client.win, &bd->client.netwm.pid))
          {
             if (bd->client.icccm.client_leader)
               {
                  if (!ecore_x_netwm_pid_get(bd->client.icccm.client_leader, &bd->client.netwm.pid))
                    bd->client.netwm.pid = -1;
               }
             else
               bd->client.netwm.pid = -1;
          }

        if (!bd->re_manage)
          {
             inst = e_exec_startup_id_pid_instance_find(bd->client.netwm.startup_id,
                                                        bd->client.netwm.pid);
             if ((inst) && (inst->used == 0))
               {
                  E_Zone *zone;
                  E_Desk *desk;

                  inst->used++;
                  zone = e_container_zone_number_get(bd->zone->container,
                                                     inst->screen);
                  if (zone) e_border_zone_set(bd, zone);
                  desk = e_desk_at_xy_get(bd->zone, inst->desk_x,
                                          inst->desk_y);
                  if (desk) e_border_desk_set(bd, desk);
                  e_exec_instance_found(inst);
               }

             if (e_config->window_grouping) // FIXME: We may want to make the border "urgent" so that the user knows it appeared.
               {
                  E_Border *bdl = NULL;

                  bdl = bd->parent;
                  if (!bdl)
                    {
                       if (bd->leader) bdl = bd->leader;
                    }
                  if (!bdl)
                    {
                       E_Border *child;
                       E_Border_List *bl;

                       bl = e_container_border_list_first(bd->zone->container);
                       if (bl)
                         {
                            while ((child = e_container_border_list_next(bl)))
                              {
                                 if (child == bd) continue;
                                 if (e_object_is_del(E_OBJECT(child))) continue;
                                 if ((bd->client.icccm.client_leader) &&
                                     (child->client.icccm.client_leader ==
                                      bd->client.icccm.client_leader))
                                   {
                                      bdl = child;
                                      break;
                                   }
                              }
                            e_container_border_list_free(bl);
                         }
                    }
                  if (bdl)
                    {
                       if (bdl->zone)
                         e_border_zone_set(bd, bdl->zone);
                       if (bdl->desk)
                         e_border_desk_set(bd, bdl->desk);
                       else
                         e_border_stick(bd);
                    }
               }
          }
     }

#ifdef _F_USE_DESK_WINDOW_PROFILE_
   if ((e_config->use_desktop_window_profile) && (need_desk_set))
     {
        if (!(bd->client.e.state.profile.name) &&
            (bd->client.e.state.profile.num >= 1))
          {
             const char *p = NULL;
             int i;
             for (i = 0; i < bd->client.e.state.profile.num; i++)
               {
                  if (!bd->client.e.state.profile.available_list[i])
                    continue;
                  p = bd->client.e.state.profile.available_list[i];
                  if (strcmp(bd->desk->window_profile, p) == 0)
                    {
                       bd->client.e.state.profile.name = eina_stringshare_add(bd->desk->window_profile);
                       break;
                    }
               }

             if (!bd->client.e.state.profile.name)
               {
                  E_Container *con = bd->zone->container;
                  E_Desk *desk = NULL;

                  for (i = 0; i < bd->client.e.state.profile.num; i++)
                    {
                       if (!bd->client.e.state.profile.available_list[i])
                         continue;
                       p = bd->client.e.state.profile.available_list[i];
                       desk = e_container_desk_window_profile_get(con, p);
                       if ((desk) && (bd->desk != desk))
                         {
                            bd->client.e.state.profile.name = eina_stringshare_add(p);
                            break;
                         }
                    }
               }
          }

        if (!bd->client.e.state.profile.name)
          {
             bd->client.e.state.profile.name = eina_stringshare_add(bd->desk->window_profile);
          }

        if (bd->new_client)
          {
             E_Desk *desk;
             desk = e_container_desk_window_profile_get(bd->zone->container, bd->client.e.state.profile.name);
             if ((desk) && (bd->desk != desk))
               {
                  LOGE("win:%x, Set NEW profile:%s", bd->client.win, bd->client.e.state.profile.name);
                  e_border_desk_set(bd, desk);
                  bd->changes.pos = 1;
                  bd->changes.size = 1;
                  bd->changed = 1;
               }
          }
        else
          {
             ecore_x_e_window_profile_change_request_send(bd->client.win,
                                                          bd->client.e.state.profile.name);
             bd->client.e.state.profile.wait_for_done = 1;
          }
     }
#endif

#ifdef _F_E_WIN_AUX_HINT_
   if (need_aux_hint_update)
     {
        e_border_aux_hint_info_update(bd);
        bd->client.e.fetch.aux_hint.hints = 0;
     }
#endif /* end of _F_E_WIN_AUX_HINT_ */

   /* PRE_POST_FETCH calls e_remember apply for new client */
   _e_border_hook_call(E_BORDER_HOOK_EVAL_PRE_POST_FETCH, bd);
   _e_border_hook_call(E_BORDER_HOOK_EVAL_POST_FETCH, bd);
   _e_border_hook_call(E_BORDER_HOOK_EVAL_PRE_BORDER_ASSIGN, bd);

#ifdef _F_ZONE_WINDOW_ROTATION_
   if (fetch_transient_for)
     {
        Eina_Bool need_fetch = EINA_FALSE;

        if (rot.vkbd)
          {
             if (rot.vkbd == bd) need_fetch = EINA_TRUE;
          }
        else if ((rot.vkbd_prediction) && (rot.vkbd_prediction == bd))
          need_fetch = EINA_TRUE;

        if (need_fetch)
          {
             if (bd->parent != rot.vkbd_parent)
               {
                  ELBF(ELBT_ROT, 0, bd->client.win, "SET VKBD_PARENT: 0x%08x",
                       bd->parent ? bd->parent->client.win : (unsigned int)NULL);
                  rot.vkbd_parent = bd->parent;
               }
          }
     }
   if (e_config->wm_win_rotation)
     {
        if ((bd->client.e.fetch.rot.need_rotation) &&
            (bd->client.e.state.rot.type == E_BORDER_ROTATION_TYPE_NORMAL))
          {
             Eina_Bool hint = EINA_FALSE;
             int ang = 0;
             int x, y, w, h;
             Eina_Bool move;

             ang = e_border_rotation_recommend_angle_get(bd);
             if (e_border_rotation_set(bd, ang))
               {
                  ELB(ELBT_ROT, "NEED ROT", bd->client.win);
                  ang = bd->client.e.state.rot.ang.next;
                  bd->changed = 1;
               }
             else
               ang = bd->client.e.state.rot.ang.curr;

             hint = _e_border_rotation_geom_get(bd, bd->zone, ang, &x, &y, &w, &h, &move);
             if (hint)
               {
                  _e_border_move_resize_internal(bd, x, y, w, h, EINA_TRUE, move);
                  ELBF(ELBT_ROT, 0, bd->client.win, "RESIZE_BY_HINT name:%s (%d,%d) %dx%d",
                       bd->client.icccm.name, x, y, w, h);
               }
          }
        if (bd->client.e.fetch.rot.need_rotation)
          bd->client.e.fetch.rot.need_rotation = EINA_FALSE;
     }
#endif
#ifdef _F_USE_TILED_DESK_LAYOUT_
   if (need_desk_ly_set)
     {
        if (e_config->use_tiled_desk_layout)
          {
#if 0
             _e_border_desk_layout_set(bd);
#endif
          }
     }
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

   if (bd->need_reparent)
     {
        if (!bd->internal)
          ecore_x_window_save_set_add(bd->client.win);
        ecore_x_window_reparent(bd->client.win, bd->client.shell_win, 0, 0);
        if (bd->visible)
          {
             if ((bd->new_client) && (bd->internal) &&
                 (bd->internal_ecore_evas))
               ecore_evas_show(bd->internal_ecore_evas);
             ecore_x_window_show(bd->client.win);
          }
        bd->need_reparent = 0;
     }

   if ((bd->client.border.changed) && (!bd->shaded) &&
       (!(((bd->maximized & E_MAXIMIZE_TYPE) == E_MAXIMIZE_FULLSCREEN))))
     {
        const char *bordername;

        if (bd->fullscreen)
          bordername = "borderless";
        else if (bd->bordername)
          bordername = bd->bordername;
        else if ((bd->client.mwm.borderless) || (bd->borderless))
          bordername = "borderless";
        else if (((bd->client.icccm.transient_for != 0) ||
                  (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DIALOG)) &&
                 (bd->client.icccm.min_w == bd->client.icccm.max_w) &&
                 (bd->client.icccm.min_h == bd->client.icccm.max_h))
          bordername = "noresize_dialog";
        else if ((bd->client.icccm.min_w == bd->client.icccm.max_w) &&
                 (bd->client.icccm.min_h == bd->client.icccm.max_h))
          bordername = "noresize";
        else if (bd->client.shaped)
          bordername = "shaped";
        else if ((!bd->client.icccm.accepts_focus) &&
                 (!bd->client.icccm.take_focus))
          bordername = "nofocus";
        else if (bd->client.icccm.urgent)
          bordername = "urgent";
        else if ((bd->client.icccm.transient_for != 0) ||
                 (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DIALOG))
          bordername = "dialog";
        else if (bd->client.netwm.state.modal)
          bordername = "modal";
        else if ((bd->client.netwm.state.skip_taskbar) ||
                 (bd->client.netwm.state.skip_pager))
          bordername = "skipped";
        else if ((bd->internal) && (bd->client.icccm.class) &&
                 (!strncmp(bd->client.icccm.class, "e_fwin", 6)))
          bordername = "internal_fileman";
        else
          bordername = e_config->theme_default_border_style;
        if (!bordername) bordername = "default";

        if ((!bd->client.border.name) || (strcmp(bd->client.border.name, bordername)))
          {
             Evas_Object *o;
             char buf[4096];
             int ok;

             bd->changes.border = 1;
             eina_stringshare_replace(&bd->client.border.name, bordername);

             if (bd->bg_object)
               {
                  bd->w -= (bd->client_inset.l + bd->client_inset.r);
                  bd->h -= (bd->client_inset.t + bd->client_inset.b);
                  bd->changes.size = 1;
                  evas_object_del(bd->bg_object);
               }
             o = edje_object_add(bd->bg_evas);
             snprintf(buf, sizeof(buf), "e/widgets/border/%s/border", bd->client.border.name);
             ok = e_theme_edje_object_set(o, "base/theme/borders", buf);
             if ((!ok) && (strcmp(bd->client.border.name, "borderless")))
               {
                  if (bd->client.border.name != e_config->theme_default_border_style)
                    {
                       snprintf(buf, sizeof(buf), "e/widgets/border/%s/border", e_config->theme_default_border_style);
                       ok = e_theme_edje_object_set(o, "base/theme/borders", buf);
                    }
                  if (!ok)
                    {
                       ok = e_theme_edje_object_set(o, "base/theme/borders",
                                                    "e/widgets/border/default/border");
                       if (ok)
                         {
                            /* Reset default border style to default */
                            eina_stringshare_replace(&e_config->theme_default_border_style, "default");
                            e_config_save_queue();
                         }
                    }
               }

             bd->shaped = 0;
             if (ok)
               {
                  const char *shape_option, *argb_option;
                  int use_argb = 0;

                  bd->bg_object = o;

                  if ((e_config->use_composite) && (!bd->client.argb))
                    {
                       argb_option = edje_object_data_get(o, "argb");
                       if ((argb_option) && (!strcmp(argb_option, "1")))
                         use_argb = 1;

                       if (use_argb != bd->argb)
                         _e_border_frame_replace(bd, use_argb);

                       o = bd->bg_object;
                    }

                  if (!bd->argb)
                    {
                       shape_option = edje_object_data_get(o, "shaped");
                       if ((shape_option) && (!strcmp(shape_option, "1")))
                         bd->shaped = 1;
                    }

                  if (bd->client.netwm.name)
                    edje_object_part_text_set(o, "e.text.title",
                                              bd->client.netwm.name);
                  else if (bd->client.icccm.title)
                    edje_object_part_text_set(o, "e.text.title",
                                              bd->client.icccm.title);
               }
             else
               {
                  evas_object_del(o);
                  bd->bg_object = NULL;
               }

             _e_border_client_inset_calc(bd);

             bd->w += (bd->client_inset.l + bd->client_inset.r);
             bd->h += (bd->client_inset.t + bd->client_inset.b);
             ecore_evas_shaped_set(bd->bg_ecore_evas, bd->shaped);
             bd->changes.size = 1;
             /*  really needed ? */
             ecore_x_window_move(bd->client.shell_win,
                                 bd->client_inset.l,
                                 bd->client_inset.t);

             if (bd->maximized != E_MAXIMIZE_NONE)
               {
                  E_Maximize maximized = bd->maximized;

                  /* to force possible resizes */
                  bd->maximized = E_MAXIMIZE_NONE;

                  _e_border_maximize(bd, maximized);

                  /* restore maximized state */
                  bd->maximized = maximized;

                  e_hints_window_maximized_set(bd, bd->maximized & E_MAXIMIZE_HORIZONTAL,
                                               bd->maximized & E_MAXIMIZE_VERTICAL);
               }
             if (bd->bg_object)
               {
                  edje_object_signal_callback_add(bd->bg_object, "*", "*",
                                                  _e_border_cb_signal_bind, bd);
                  if (bd->focused)
                    {
                       edje_object_signal_emit(bd->bg_object, "e,state,focused", "e");
                       if (bd->icon_object)
                         edje_object_signal_emit(bd->icon_object, "e,state,focused", "e");
                    }
                  if (bd->shaded)
                    edje_object_signal_emit(bd->bg_object, "e,state,shaded", "e");
                  if (bd->sticky)
                    edje_object_signal_emit(bd->bg_object, "e,state,sticky", "e");
                  if (bd->hung)
                    edje_object_signal_emit(bd->bg_object, "e,state,hung", "e");
                  // FIXME: in eval -do differently
                  //	     edje_object_message_signal_process(bd->bg_object);
                  //	     e_border_frame_recalc(bd);

                  evas_object_move(bd->bg_object, 0, 0);
                  evas_object_resize(bd->bg_object, bd->w, bd->h);
                  evas_object_show(bd->bg_object);
               }
          }
        bd->client.border.changed = 0;

        if (bd->icon_object)
          {
             if (bd->bg_object)
               {
                  evas_object_show(bd->icon_object);
                  edje_object_part_swallow(bd->bg_object, "e.swallow.icon", bd->icon_object);
               }
             else
               evas_object_hide(bd->icon_object);
          }
     }

   if (rem_change) e_remember_update(bd);

   if (change_urgent)
     {
        E_Event_Border_Urgent_Change *ev;

        if (bd->client.icccm.urgent)
          edje_object_signal_emit(bd->bg_object, "e,state,urgent", "e");
        else
          edje_object_signal_emit(bd->bg_object, "e,state,not_urgent", "e");

        ev = E_NEW(E_Event_Border_Urgent_Change, 1);
        if (ev)
          {
             ev->border = bd;
             e_object_ref(E_OBJECT(bd));
             ecore_event_add(E_EVENT_BORDER_URGENT_CHANGE, ev,
                             _e_border_event_border_urgent_change_free, NULL);
          }
     }

   _e_border_hook_call(E_BORDER_HOOK_EVAL_POST_BORDER_ASSIGN, bd);
   traceEnd(TTRACE_TAG_WINDOW_MANAGER);
}

#ifdef _F_FOCUS_WINDOW_IF_TOP_STACK_
static void
_e_border_latest_stacked_focus_check_set(E_Border *bd)
{
   E_Border* temp_bd = NULL;
   E_Border* top_focusable_bd = NULL;
   Eina_Bool is_fully_obscured = EINA_FALSE;
   Ecore_X_XRegion *visible_region = NULL;
   Ecore_X_XRegion *win_region = NULL;
   Ecore_X_Rectangle visible_rect, win_rect;
   E_Border_List *bl;
   int temp_bd_x, temp_bd_y, temp_bd_w, temp_bd_h;

   // set the entire visible region as a root geometry
   visible_rect.x = bd->zone->x;
   visible_rect.y = bd->zone->y;
   visible_rect.width = bd->zone->w;
   visible_rect.height = bd->zone->h;

   visible_region = ecore_x_xregion_new();
   if (!visible_region) return;

   ecore_x_xregion_union_rect(visible_region, visible_region, &visible_rect);

   bl = e_container_border_list_last(bd->zone->container);
   if (bl)
     {
        while ((temp_bd = e_container_border_list_prev(bl)))
          {
             if (temp_bd == bd) break;
             if (temp_bd == focused) continue;

             e_border_geometry_get(temp_bd, &temp_bd_x, &temp_bd_y, &temp_bd_w, &temp_bd_h);

             if ((temp_bd_x >= bd->zone->w) || (temp_bd_y >= bd->zone->h)) continue;
             if (((temp_bd_x + temp_bd_w) <= 0) || ((temp_bd_y + temp_bd_h) <= 0)) continue;
             if ((!temp_bd->iconic) && (temp_bd->visible) && (temp_bd->desk == bd->desk) &&
                 (temp_bd->client.icccm.accepts_focus || temp_bd->client.icccm.take_focus) &&
                 (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DOCK) &&
                 (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_TOOLBAR) &&
                 (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_MENU) &&
                 (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_SPLASH) &&
                 (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DESKTOP))
               {
                  if (!top_focusable_bd)
                    {
                       top_focusable_bd = temp_bd;
                    }

                  win_rect.x = temp_bd_x;
                  win_rect.y = temp_bd_y;
                  win_rect.width = temp_bd_w;
                  win_rect.height = temp_bd_h;

                  // if it stick out or is bigger than the entire visible region,
                  // clip it by the entire visible's geometry.
                  E_RECTS_CLIP_TO_RECT(win_rect.x, win_rect.y,
                                       win_rect.width, win_rect.height,
                                       visible_rect.x, visible_rect.y,
                                       (int)(visible_rect.width), (int)(visible_rect.height));

                  if (ecore_x_xregion_rect_contain(visible_region, &win_rect))
                    {
                       win_region = ecore_x_xregion_new();
                       if (win_region)
                         {
                            ecore_x_xregion_union_rect(win_region, win_region, &win_rect);
                            ecore_x_xregion_subtract(visible_region, visible_region, win_region);
                            ecore_x_xregion_free(win_region);
                            win_region = NULL;

                            if (ecore_x_xregion_is_empty(visible_region))
                              {
                                 is_fully_obscured = EINA_TRUE;
                                 break;
                              }
                         }
                    }
               }
          }
        e_container_border_list_free(bl);
     }

   if (is_fully_obscured == EINA_TRUE)
     {
        e_border_focus_set(top_focusable_bd, 1, 1);
     }
   else
     {
        e_border_focus_set(bd, 1, 1);
     }

   if (visible_region) ecore_x_xregion_free(visible_region);
}

static void
_e_border_latest_stacked_focus(E_Border *bd)
{
   E_Border *temp_bd;
   int root_w, root_h;
   int temp_bd_x, temp_bd_y, temp_bd_w, temp_bd_h;

   root_w = bd->zone->w;
   root_h = bd->zone->h;

   Eina_List *l;
   EINA_LIST_FOREACH(focus_stack, l, temp_bd)
     {
        if (bd == temp_bd) continue;
        e_border_geometry_get(temp_bd, &temp_bd_x, &temp_bd_y, &temp_bd_w, &temp_bd_h);

        if ((temp_bd_x >= root_w) || (temp_bd_y >= root_h)) continue;
        if (((temp_bd_x + temp_bd_w) <= 0) || ((temp_bd_y + temp_bd_h) <= 0)) continue;

        if ((!temp_bd->iconic) && (temp_bd->visible) && (temp_bd->desk == bd->desk) &&
            (temp_bd->client.icccm.accepts_focus || temp_bd->client.icccm.take_focus) &&
            (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DOCK) &&
            (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_TOOLBAR) &&
            (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_MENU) &&
            (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_SPLASH) &&
            (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DESKTOP))
          {
             _e_border_latest_stacked_focus_check_set(temp_bd);
             break;
          }
     }
}

static void
_e_border_check_stack (E_Border *bd)
{
   E_Border* temp_bd = NULL;
   E_Border* top_bd = NULL;
   int passed_focus = 0;
   int passed = 0;
   int root_w = bd->zone->w;
   int root_h = bd->zone->h;
   int temp_bd_x, temp_bd_y, temp_bd_w, temp_bd_h;

   E_Border_List *bl;
   bl = e_container_border_list_last(bd->zone->container);
   if (!bl) return;

   while ((temp_bd = e_container_border_list_prev(bl)))
     {
        e_border_geometry_get(temp_bd, &temp_bd_x, &temp_bd_y, &temp_bd_w, &temp_bd_h);

        if ((temp_bd_x >= root_w) || (temp_bd_y >= root_h)) continue;
        if (((temp_bd_x + temp_bd_w) <= 0) || ((temp_bd_y + temp_bd_h) <= 0)) continue;

        if ((temp_bd != bd) &&
            (temp_bd->client.illume.win_state.state != ECORE_X_ILLUME_WINDOW_STATE_NORMAL)) continue;

        if ((!temp_bd->iconic) && (temp_bd->visible) && (temp_bd->desk == bd->desk) &&
           (temp_bd->client.icccm.accepts_focus || temp_bd->client.icccm.take_focus) &&
           (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DOCK) &&
           (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_TOOLBAR) &&
           (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_MENU) &&
           (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_SPLASH) &&
           (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DESKTOP))
          {
             if (bd == temp_bd)
               {
                  if (!passed)
                    {
                       e_border_focus_set_with_pointer(bd);
                       break;
                    }
                  else
                    {
                       if (!passed_focus)
                         {
                            e_border_focus_set_with_pointer(top_bd);
                         }
                       break;
                    }
               }
             else
               {
                  if (!passed)
                    {
                       if ((bd->client.icccm.accepts_focus) || (bd->client.icccm.take_focus))
                         {
                            if (!bd->lock_focus_out)
                              {
                                 e_border_focus_latest_set(bd);
                              }
                         }
                    }
               }

             if (!passed)
               {
                  passed = 1;
                  top_bd = temp_bd;
               }

             if (temp_bd == focused)
               {
                  passed_focus = 1;
               }
          }
     }
   e_container_border_list_free(bl);
}

static void
_e_border_focus_top_stack_set(E_Border* bd)
{
   E_Border *temp_bd;
   int root_w, root_h;
   int temp_bd_x, temp_bd_y, temp_bd_w, temp_bd_h;

   root_w = bd->zone->w;
   root_h = bd->zone->h;

   E_Border_List *bl;
   bl = e_container_border_list_last(bd->zone->container);
   if (!bl) return;

   while ((temp_bd = e_container_border_list_prev(bl)))
     {
        e_border_geometry_get(temp_bd, &temp_bd_x, &temp_bd_y, &temp_bd_w, &temp_bd_h);

        if ((temp_bd_x >= root_w) || (temp_bd_y >= root_h)) continue;
        if (((temp_bd_x + temp_bd_w) <= 0) || ((temp_bd_y + temp_bd_h) <= 0)) continue;
        if (temp_bd->client.illume.win_state.state != ECORE_X_ILLUME_WINDOW_STATE_NORMAL) continue;

        if ((!temp_bd->iconic) && (temp_bd->visible) && (temp_bd->desk == bd->desk) &&
            (temp_bd->client.icccm.accepts_focus || temp_bd->client.icccm.take_focus) &&
            (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DOCK) &&
            (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_TOOLBAR) &&
            (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_MENU) &&
            (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_SPLASH) &&
            (temp_bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DESKTOP))
          {
             if (!temp_bd->focused)
               {
                  /* this border is the top of the latest stack */
                  e_border_focus_set (temp_bd, 1, 1);
               }
             break;
          }
     }
   e_container_border_list_free(bl);
}

static void
_e_border_transient_next_focus_set(E_Border *bd)
{
   Eina_List *l, *list = NULL;
   E_Border *parent;
   E_Border *child;
   E_Border *next_focus = NULL;

   if (!bd) return;
   if (!bd->parent) return;
   if (e_object_is_del(E_OBJECT(bd->parent))) return;

   parent = bd->parent;
   list = _e_border_sub_borders_new(parent);
   EINA_LIST_FOREACH(list, l, child)
     {
        if (child == bd) continue;
        if (e_object_is_del(E_OBJECT(child))) continue;

        if (child->client.icccm.accepts_focus ||
            child->client.icccm.take_focus)
          {
             if (child->visible)
               {
                  next_focus = child;
                  break;
               }
          }
     }
   eina_list_free(list);

   if (next_focus)
     {
        e_border_focus_set(next_focus, 1, 1);
     }
   else
     {
        if (parent->client.icccm.accepts_focus ||
            parent->client.icccm.take_focus)
          {
             e_border_focus_set(parent, 1, 1);
          }
        else
          {
             _e_border_latest_stacked_focus(bd);
          }
     }
}
#endif

static void
_e_border_eval(E_Border *bd)
{
   E_Event_Border_Property *event;
   E_Border_Pending_Move_Resize *pnd;
   int rem_change = 0;
   int send_event = 1;
   traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:BORDER:EVAL");

   if (e_object_is_del(E_OBJECT(bd)))
     {
        CRI("_e_border_eval(%p) with deleted border! - %d\n", bd, bd->new_client);
        bd->changed = 0;
        traceEnd(TTRACE_TAG_WINDOW_MANAGER);
        return;
     }

   _e_border_hook_call(E_BORDER_HOOK_EVAL_PRE_NEW_BORDER, bd);

   if (bd->new_client)
     {
        int zx = 0, zy = 0, zw = 0, zh = 0;

        if (bd->zone)
          e_zone_useful_geometry_get(bd->zone, &zx, &zy, &zw, &zh);

        /*
         * Limit maximum size of windows to useful geometry
         */
        // TODO: temoporary limited maximize algorithm
        // ->
        /*if (bd->w > zw)
          rw = zw;
          else
          rw = bd->w;

          if (bd->h > zh)
          rh = zh;
          else
          rh = bd->h;

          if ((rw != bd->w) || (rh != bd->h))
          {
          bd->w = rw;
          bd->h = rh;
          e_border_resize (bd, bd->w, bd->h);
          }*/
        // <-

        if (bd->re_manage)
          {
             bd->x -= bd->client_inset.l;
             bd->y -= bd->client_inset.t;
             bd->changes.pos = 1;
             bd->placed = 1;
          }
        else if ((!bd->placed) && (bd->client.icccm.request_pos))
          {

             Ecore_X_Window_Attributes *att;
             int bw;

             att = &bd->client.initial_attributes;
             bw = att->border * 2;
             switch (bd->client.icccm.gravity)
               {
                case ECORE_X_GRAVITY_N:
                   bd->x = (att->x - (bw / 2)) - (bd->client_inset.l / 2);
                   bd->y = att->y;
                   break;

                case ECORE_X_GRAVITY_NE:
                   bd->x = (att->x - (bw)) - (bd->client_inset.l);
                   bd->y = att->y;
                   break;

                case ECORE_X_GRAVITY_E:
                   bd->x = (att->x - (bw)) - (bd->client_inset.l);
                   bd->y = (att->y - (bw / 2)) - (bd->client_inset.t / 2);
                   break;

                case ECORE_X_GRAVITY_SE:
                   bd->x = (att->x - (bw)) - (bd->client_inset.l);
                   bd->y = (att->y - (bw)) - (bd->client_inset.t);
                   break;

                case ECORE_X_GRAVITY_S:
                   bd->x = (att->x - (bw / 2)) - (bd->client_inset.l / 2);
                   bd->y = (att->y - (bw)) - (bd->client_inset.t);
                   break;

                case ECORE_X_GRAVITY_SW:
                   bd->x = att->x;
                   bd->y = (att->y - (bw)) - (bd->client_inset.t);
                   break;

                case ECORE_X_GRAVITY_W:
                   bd->x = att->x;
                   bd->y = (att->y - (bw)) - (bd->client_inset.t);
                   break;

                case ECORE_X_GRAVITY_CENTER:
                   bd->x = (att->x - (bw / 2)) - (bd->client_inset.l / 2);
                   bd->y = (att->y - (bw / 2)) - (bd->client_inset.t / 2);
                   break;

                case ECORE_X_GRAVITY_NW:
                default:
                   bd->x = att->x;
                   bd->y = att->y;
               }

             /*
              * This ensures that windows that like to open with a x/y
              * position smaller than returned by e_zone_useful_geometry_get()
              * are moved to useful positions.
              */
             // ->
             if (e_config->geometry_auto_move)
               {
                  if (bd->x < zx)
                    bd->x = zx;

                  if (bd->y < zy)
                    bd->y = zy;

                  if (bd->x + bd->w > zx + zw)
                    bd->x = zx + zw - bd->w;

                  if (bd->y + bd->h > zy + zh)
                    bd->y = zy + zh - bd->h;
                  // <--

                  if (bd->zone && e_container_zone_at_point_get(bd->zone->container, bd->x, bd->y))
                    {
                       bd->changes.pos = 1;
                       bd->placed = 1;
                    }
               }
             else
               {
                  bd->changes.pos = 1;
                  bd->placed = 1;
               }
          }
        if (!bd->placed)
          {
             /* FIXME: special placement for dialogs etc. etc. etc goes
              * here */
             /* FIXME: what if parent is not on this desktop - or zone? */
             if ((bd->parent) && (bd->parent->visible))
               {
                  bd->x = bd->parent->x + ((bd->parent->w - bd->w) / 2);
                  bd->y = bd->parent->y + ((bd->parent->h - bd->h) / 2);
                  bd->changes.pos = 1;
                  bd->placed = 1;
               }
#if 0
             else if ((bd->leader) && (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DIALOG))
               {
                  /* TODO: Place in center of group */
               }
#endif
             else if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DIALOG)
               {
                  bd->x = zx + ((zw - bd->w) / 2);
                  bd->y = zy + ((zh - bd->h) / 2);
                  bd->changes.pos = 1;
                  bd->placed = 1;
               }
          }
        if (!bd->placed)
          {
             Eina_List *skiplist = NULL;
             int new_x, new_y;
             unsigned int seedp1 = 1, seedp2 = 1;

             if (zw > bd->w)
               new_x = zx + (rand_r(&seedp1) % (zw - bd->w)); // Prevent Fix : CID 30774
             else
               new_x = zx;
             if (zh > bd->h)
               new_y = zy + (rand_r(&seedp2) % (zh - bd->h)); // Prevent Fix : CID 30774
             else
               new_y = zy;

             if ((e_config->window_placement_policy == E_WINDOW_PLACEMENT_SMART) || (e_config->window_placement_policy == E_WINDOW_PLACEMENT_ANTIGADGET))
               {
                  skiplist = eina_list_append(skiplist, bd);
                  if (bd->desk)
                    e_place_desk_region_smart(bd->desk, skiplist,
                                              bd->x, bd->y, bd->w, bd->h,
                                              &new_x, &new_y);
                  else
                    e_place_zone_region_smart(bd->zone, skiplist,
                                              bd->x, bd->y, bd->w, bd->h,
                                              &new_x, &new_y);
                  eina_list_free(skiplist);
               }
             else if (e_config->window_placement_policy == E_WINDOW_PLACEMENT_MANUAL)
               {
                  e_place_zone_manual(bd->zone, bd->w, bd->client_inset.t,
                                      &new_x, &new_y);
               }
             else
               {
                  e_place_zone_cursor(bd->zone, bd->x, bd->y, bd->w, bd->h,
                                      bd->client_inset.t, &new_x, &new_y);
               }
             bd->x = new_x;
             bd->y = new_y;
             bd->changes.pos = 1;
          }


        /* Recreate state */
        e_hints_window_init(bd);

        /* remove pending_move_resize */
        EINA_LIST_FREE(bd->pending_move_resize, pnd)
          {
             if ((!bd->lock_client_location) && (pnd->move))
               {
                  bd->x = pnd->x;
                  bd->y = pnd->y;
                  bd->changes.pos = 1;
                  bd->placed = 1;
                  if (pnd->without_border)
                    {
                       bd->x -= bd->client_inset.l;
                       bd->y -= bd->client_inset.t;
                    }
               }
             if ((!bd->lock_client_size) && (pnd->resize))
               {
                  bd->w = pnd->w + (bd->client_inset.l + bd->client_inset.r);
                  bd->h = pnd->h + (bd->client_inset.t + bd->client_inset.b);
                  bd->client.w = pnd->w;
                  bd->client.h = pnd->h;
                  bd->changes.size = 1;
               }

             free(pnd);
          }

        if ((bd->client.e.state.centered) &&
            ((!bd->remember) ||
             ((bd->remember) && (!(bd->remember->apply & E_REMEMBER_APPLY_POS)))))
          {
             bd->x = zx + (zw - bd->w) / 2;
             bd->y = zy + (zh - bd->h) / 2;
             bd->changes.pos = 1;
             bd->placed = 1;
          }

        _e_border_client_move_resize_send(bd);

        /* if the explicit geometry request asks for the app to be
         * in another zone - well move it there */
        {
           E_Zone *zone;

           zone = e_container_zone_at_point_get(bd->zone->container,
                                                bd->x + (bd->w / 2),
                                                bd->y + (bd->h / 2));
           if (!zone)
             zone = e_container_zone_at_point_get(bd->zone->container,
                                                  bd->x,
                                                  bd->y);
           if (!zone)
             zone = e_container_zone_at_point_get(bd->zone->container,
                                                  bd->x + bd->w - 1,
                                                  bd->y);
           if (!zone)
             zone = e_container_zone_at_point_get(bd->zone->container,
                                                  bd->x + bd->w - 1,
                                                  bd->y + bd->h - 1);
           if (!zone)
             zone = e_container_zone_at_point_get(bd->zone->container,
                                                  bd->x,
                                                  bd->y + bd->h - 1);
           if ((zone) && (zone != bd->zone))
	     e_border_zone_set(bd, zone);
        }
     }

   _e_border_hook_call(E_BORDER_HOOK_EVAL_POST_NEW_BORDER, bd);

   /* effect changes to the window border itself */
   if ((bd->changes.shading))
     {
        /*  show at start of unshade (but don't hide until end of shade) */
        if (bd->shaded)
          ecore_x_window_raise(bd->client.shell_win);
        bd->changes.shading = 0;
        rem_change = 1;
     }
   if ((bd->changes.shaded) && (bd->changes.pos) && (bd->changes.size))
     {
        if (bd->shaded)
          ecore_x_window_lower(bd->client.shell_win);
        else
          ecore_x_window_raise(bd->client.shell_win);
        bd->changes.shaded = 0;
        rem_change = 1;
     }
   else if ((bd->changes.shaded) && (bd->changes.pos))
     {
        if (bd->shaded)
          ecore_x_window_lower(bd->client.shell_win);
        else
          ecore_x_window_raise(bd->client.shell_win);
        bd->changes.size = 1;
        bd->changes.shaded = 0;
        rem_change = 1;
     }
   else if ((bd->changes.shaded) && (bd->changes.size))
     {
        if (bd->shaded)
          ecore_x_window_lower(bd->client.shell_win);
        else
          ecore_x_window_raise(bd->client.shell_win);
        bd->changes.shaded = 0;
        rem_change = 1;
     }
   else if (bd->changes.shaded)
     {
        if (bd->shaded)
          ecore_x_window_lower(bd->client.shell_win);
        else
          ecore_x_window_raise(bd->client.shell_win);
        bd->changes.size = 1;
        bd->changes.shaded = 0;
        rem_change = 1;
     }

   if (bd->changes.size)
     {
        int x = 0, y = 0, xx = 0, yy = 0;

        if ((bd->shaded) && (!bd->shading))
          {
             evas_obscured_clear(bd->bg_evas);
          }
        else
          {
             xx = bd->w - (bd->client_inset.l + bd->client_inset.r);
             yy = bd->h - (bd->client_inset.t + bd->client_inset.b);

             evas_obscured_clear(bd->bg_evas);
             evas_obscured_rectangle_add(bd->bg_evas,
                                         bd->client_inset.l, bd->client_inset.t, xx, yy);

             if (bd->shading)
               {
                  if (bd->shade.dir == E_DIRECTION_UP)
                    {
                       y = yy - bd->client.h;
                    }
                  else if (bd->shade.dir == E_DIRECTION_LEFT)
                    {
                       x = xx - bd->client.w;
                    }
               }
          }

        if (bd->client.e.state.video)
          {
             if (bd->client.e.state.video_position.updated)
               {
                  ecore_x_window_move(bd->win,
                                      bd->client.e.state.video_parent_border->x +
                                      bd->client.e.state.video_parent_border->client_inset.l +
                                      bd->client.e.state.video_parent_border->fx.x +
                                      bd->client.e.state.video_position.x,
                                      bd->client.e.state.video_parent_border->y +
                                      bd->client.e.state.video_parent_border->client_inset.t +
                                      bd->client.e.state.video_parent_border->fx.y +
                                      bd->client.e.state.video_position.y);
                  bd->client.e.state.video_position.updated = 0;
               }
          }
        else if (!bd->changes.pos)
          {
             if (bd->post_job) ecore_idle_enterer_del(bd->post_job);
             bd->post_job = ecore_idle_enterer_add(_e_border_post_move_resize_job, bd);
             bd->post_resize = 1;
          }
        else
          {
             E_Border *tmp;
             Eina_List *l;

             ecore_x_window_move_resize(bd->win,
                                        bd->x + bd->fx.x,
                                        bd->y + bd->fx.y,
                                        bd->w, bd->h);

             EINA_LIST_FOREACH(bd->client.e.state.video_child, l, tmp)
               ecore_x_window_move(tmp->win,
                                   bd->x + bd->fx.x + bd->client_inset.l + tmp->client.e.state.video_position.x,
                                   bd->y + bd->fx.y + bd->client_inset.t + tmp->client.e.state.video_position.y);
          }

        ecore_x_window_move_resize(bd->event_win, 0, 0, bd->w, bd->h);

        if ((!bd->shaded) || (bd->shading))
          ecore_x_window_move_resize(bd->client.shell_win,
                                     bd->client_inset.l, bd->client_inset.t, xx, yy);

        if (bd->internal_ecore_evas)
          ecore_evas_move_resize(bd->internal_ecore_evas, x, y, bd->client.w, bd->client.h);
        else if (!bd->client.e.state.video)
          ecore_x_window_move_resize(bd->client.win, x, y, bd->client.w, bd->client.h);

        ecore_evas_move_resize(bd->bg_ecore_evas, 0, 0, bd->w, bd->h);
        evas_object_resize(bd->bg_object, bd->w, bd->h);
        e_container_shape_resize(bd->shape, bd->w, bd->h);
        if (bd->changes.pos)
          e_container_shape_move(bd->shape, bd->x + bd->fx.x, bd->y + bd->fx.y);

        _e_border_client_move_resize_send(bd);

        bd->changes.pos = 0;
        bd->changes.size = 0;
        rem_change = 1;
     }
   else if (bd->changes.pos)
     {
        if (bd->post_job) ecore_idle_enterer_del(bd->post_job);
        bd->post_job = ecore_idle_enterer_add(_e_border_post_move_resize_job, bd);
        bd->post_move = 1;

        e_container_shape_move(bd->shape, bd->x + bd->fx.x, bd->y + bd->fx.y);

        _e_border_client_move_resize_send(bd);

        bd->changes.pos = 0;
        rem_change = 1;
     }

   if (bd->changes.reset_gravity)
     {
        GRAV_SET(bd, ECORE_X_GRAVITY_NW);
        bd->changes.reset_gravity = 0;
        rem_change = 1;
     }

   if (bd->need_shape_merge)
     {
        _e_border_shape_input_rectangle_set(bd);
        if ((bd->shaped) || (bd->client.shaped))
          {
             Ecore_X_Window twin, twin2;
             int x, y;

             twin = ecore_x_window_override_new
               (bd->zone->container->scratch_win, 0, 0, bd->w, bd->h);
             if (bd->shaped)
               ecore_x_window_shape_window_set(twin, bd->bg_win);
             else
               {
                  Ecore_X_Rectangle rects[4];

                  rects[0].x = 0;
                  rects[0].y = 0;
                  rects[0].width = bd->w;
                  rects[0].height = bd->client_inset.t;
                  rects[1].x = 0;
                  rects[1].y = bd->client_inset.t;
                  rects[1].width = bd->client_inset.l;
                  rects[1].height = bd->h - bd->client_inset.t - bd->client_inset.b;
                  rects[2].x = bd->w - bd->client_inset.r;
                  rects[2].y = bd->client_inset.t;
                  rects[2].width = bd->client_inset.r;
                  rects[2].height = bd->h - bd->client_inset.t - bd->client_inset.b;
                  rects[3].x = 0;
                  rects[3].y = bd->h - bd->client_inset.b;
                  rects[3].width = bd->w;
                  rects[3].height = bd->client_inset.b;
                  ecore_x_window_shape_rectangles_set(twin, rects, 4);
               }
             twin2 = ecore_x_window_override_new
               (bd->zone->container->scratch_win, 0, 0,
                bd->w - bd->client_inset.l - bd->client_inset.r,
                bd->h - bd->client_inset.t - bd->client_inset.b);
             x = 0;
             y = 0;
             if ((bd->shading) || (bd->shaded))
               {
                  if (bd->shade.dir == E_DIRECTION_UP)
                    y = bd->h - bd->client_inset.t - bd->client_inset.b - bd->client.h;
                  else if (bd->shade.dir == E_DIRECTION_LEFT)
                    x = bd->w - bd->client_inset.l - bd->client_inset.r - bd->client.w;
               }
             ecore_x_window_shape_window_set_xy(twin2, bd->client.win,
                                                x, y);
             ecore_x_window_shape_rectangle_clip(twin2, 0, 0,
                                                 bd->w - bd->client_inset.l - bd->client_inset.r,
                                                 bd->h - bd->client_inset.t - bd->client_inset.b);
             ecore_x_window_shape_window_add_xy(twin, twin2,
                                                bd->client_inset.l,
                                                bd->client_inset.t);
             ecore_x_window_free(twin2);
             ecore_x_window_shape_window_set(bd->win, twin);
             ecore_x_window_free(twin);
          }
        else
          ecore_x_window_shape_mask_set(bd->win, 0);
        //	bd->need_shape_export = 1;
        bd->need_shape_merge = 0;
     }

   if (bd->need_shape_export)
     {
        Ecore_X_Rectangle *rects, *orects;
        int num;

        rects = ecore_x_window_shape_rectangles_get(bd->win, &num);
        if (rects)
          {
             int changed;

             changed = 1;
             if ((num == bd->shape_rects_num) && (bd->shape_rects))
               {
                  int i;

                  orects = bd->shape_rects;
                  changed = 0;
                  for (i = 0; i < num; i++)
                    {
                       if (rects[i].x < 0)
                         {
                            rects[i].width -= rects[i].x;
                            rects[i].x = 0;
                         }
                       if ((rects[i].x + (int)rects[i].width) > bd->w)
                         rects[i].width = rects[i].width - rects[i].x;
                       if (rects[i].y < 0)
                         {
                            rects[i].height -= rects[i].y;
                            rects[i].y = 0;
                         }
                       if ((rects[i].y + (int)rects[i].height) > bd->h)
                         rects[i].height = rects[i].height - rects[i].y;

                       if ((orects[i].x != rects[i].x) ||
                           (orects[i].y != rects[i].y) ||
                           (orects[i].width != rects[i].width) ||
                           (orects[i].height != rects[i].height))
                         {
                            changed = 1;
                            break;
                         }
                    }
               }
             if (changed)
               {
                  if (bd->client.shaped)
                    e_container_shape_solid_rect_set(bd->shape, 0, 0, 0, 0);
                  else
                    e_container_shape_solid_rect_set(bd->shape, bd->client_inset.l, bd->client_inset.t, bd->client.w, bd->client.h);
                  E_FREE(bd->shape_rects);
                  bd->shape_rects = rects;
                  bd->shape_rects_num = num;
                  e_container_shape_rects_set(bd->shape, rects, num);
               }
             else
               free(rects);
          }
        else
          {
             E_FREE(bd->shape_rects);
             bd->shape_rects = NULL;
             bd->shape_rects_num = 0;
             e_container_shape_rects_set(bd->shape, NULL, 0);
          }
        bd->need_shape_export = 0;
     }

   if ((bd->changes.visible) && (bd->visible) && (bd->new_client))
     {
        int x, y;

        ecore_x_pointer_xy_get(bd->zone->container->win, &x, &y);
        if ((!bd->placed) && (!bd->re_manage) &&
            (e_config->window_placement_policy == E_WINDOW_PLACEMENT_MANUAL) &&
            (!((bd->client.icccm.transient_for != 0) ||
               (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DIALOG))) &&
            (!bdmove) && (!bdresize))
          {
             /* Set this window into moving state */

             bd->cur_mouse_action = e_action_find("window_move");
             if (bd->cur_mouse_action)
               {
                  if ((!bd->cur_mouse_action->func.end_mouse) &&
                      (!bd->cur_mouse_action->func.end))
                    bd->cur_mouse_action = NULL;
                  if (bd->cur_mouse_action)
                    {
                       bd->x = x - (bd->w >> 1);
                       bd->y = y - (bd->client_inset.t >> 1);
                       bd->changed = 1;
                       bd->changes.pos = 1;

                       _e_border_client_move_resize_send(bd);
                    }
               }
          }

        _e_border_show(bd);

        if (bd->cur_mouse_action)
          {
             bd->moveinfo.down.x = bd->x + bd->fx.x;
             bd->moveinfo.down.y = bd->y + bd->fx.y;
             bd->moveinfo.down.w = bd->w;
             bd->moveinfo.down.h = bd->h;
             bd->mouse.current.mx = x;
             bd->mouse.current.my = y;
             bd->moveinfo.down.button = 0;
             bd->moveinfo.down.mx = x;
             bd->moveinfo.down.my = y;

             grabbed = 1;
             e_object_ref(E_OBJECT(bd->cur_mouse_action));
             bd->cur_mouse_action->func.go(E_OBJECT(bd), NULL);
             if (e_config->border_raise_on_mouse_action)
               e_border_raise(bd);
             e_border_focus_set(bd, 1, 1);
          }
        bd->changes.visible = 0;
        rem_change = 1;
     }

   if (bd->changes.icon)
     {
#ifndef _F_DISABLE_E_EFREET_
        if (bd->desktop)
          {
             efreet_desktop_free(bd->desktop);
             bd->desktop = NULL;
          }
#endif
        if (bd->icon_object)
          {
             evas_object_del(bd->icon_object);
             bd->icon_object = NULL;
          }
#ifndef _F_DISABLE_E_EFREET_
        if (bd->remember && bd->remember->prop.desktop_file)
          {
             const char *desktop = bd->remember->prop.desktop_file;

             bd->desktop = efreet_desktop_get(desktop);
             if (!bd->desktop)
               bd->desktop = efreet_util_desktop_name_find(desktop);
          }
        if (!bd->desktop)
          {
             if ((bd->client.icccm.name) && (bd->client.icccm.class))
               bd->desktop = efreet_util_desktop_wm_class_find(bd->client.icccm.name,
                                                               bd->client.icccm.class);
          }
        if (!bd->desktop)
          {
             /* libreoffice and maybe others match window class
                with .desktop file name */
             if (bd->client.icccm.class)
               {
                  char buf[128];
                  snprintf(buf, sizeof(buf), "%s.desktop", bd->client.icccm.class);
                  bd->desktop = efreet_util_desktop_file_id_find(buf);
               }
          }
        if (!bd->desktop)
          {
             bd->desktop = e_exec_startup_id_pid_find(bd->client.netwm.startup_id,
                                                      bd->client.netwm.pid);
             if (bd->desktop) efreet_desktop_ref(bd->desktop);
          }
        if (!bd->desktop && bd->client.icccm.name)
          {
             /* this works for most cases as fallback. useful when app is
                run from a shell  */
             bd->desktop = efreet_util_desktop_exec_find(bd->client.icccm.name);
          }
        if (!bd->desktop && bd->client.icccm.transient_for)
          {
             E_Border *bd2 = e_border_find_by_client_window(bd->client.icccm.transient_for);
             if (bd2 && bd2->desktop)
               {
                  efreet_desktop_ref(bd2->desktop);
                  bd->desktop = bd2->desktop;
               }
          }
        if (bd->desktop)
          {
             ecore_x_window_prop_string_set(bd->client.win, E_ATOM_DESKTOP_FILE,
                                            bd->desktop->orig_path);
          }
#endif

        bd->icon_object = e_border_icon_add(bd, bd->bg_evas);
        if ((bd->focused) && (bd->icon_object))
          edje_object_signal_emit(bd->icon_object, "e,state,focused", "e");
        if (bd->bg_object)
          {
             evas_object_show(bd->icon_object);
             edje_object_part_swallow(bd->bg_object, "e.swallow.icon", bd->icon_object);
          }
        else
          evas_object_hide(bd->icon_object);

        {
           E_Event_Border_Icon_Change *ev;

           ev = E_NEW(E_Event_Border_Icon_Change, 1);
           if (ev)
             {
                ev->border = bd;
                e_object_ref(E_OBJECT(bd));
                //	     e_object_breadcrumb_add(E_OBJECT(bd), "border_icon_change_event");
                ecore_event_add(E_EVENT_BORDER_ICON_CHANGE, ev,
                                _e_border_event_border_icon_change_free, NULL);
             }
        }
        bd->changes.icon = 0;
     }

   /* If this is a new window, set the state as requested. */
   Eina_Bool post_iconify = EINA_FALSE;

   if (bd->new_client &&
       bd->client.icccm.initial_state == ECORE_X_WINDOW_STATE_HINT_ICONIC)
     {
        post_iconify = EINA_TRUE;
     }

   bd->new_client = 0;
   bd->changed = 0;
   bd->changes.stack = 0;
   bd->changes.prop = 0;

   if (post_iconify)
     {
        // iconified window requested by client
        LOGE("Set ICONIFY BY CLIENT [%x]", bd->client.win);
        ELBF(ELBT_BD, 0, bd->client.win, "Set ICONIFY BY CLIENT");
        bd->iconify_by_client = 1;
        e_border_iconify(bd);
        e_border_hide(bd, 1);
     }

   if (bd->post_lower)
     {
        ELBF(ELBT_BD, 0, bd->client.win, "Lower by post_lower");
        bd->post_lower = 0;
        e_border_lower(bd);
     }

   if (bd->post_raise)
     {
        ELBF(ELBT_BD, 0, bd->client.win, "Raise by post_raise");
        bd->post_raise = 0;
        e_border_raise(bd);
     }

   if ((bd->take_focus) || (bd->want_focus))
     {
        bd->take_focus = 0;
#ifdef _F_FOCUS_WINDOW_IF_TOP_STACK_
        if ((e_config->focus_setting == E_FOCUS_NEW_WINDOW) ||
            (e_config->focus_setting == E_FOCUS_NEW_WINDOW_IF_TOP_STACK) ||
            (bd->want_focus))
#else // original
        if ((e_config->focus_setting == E_FOCUS_NEW_WINDOW) || (bd->want_focus))
#endif
          {
	     bd->want_focus = 0;
#ifdef _F_FOCUS_WINDOW_IF_TOP_STACK_
             if (e_config->focus_setting == E_FOCUS_NEW_WINDOW_IF_TOP_STACK)
                _e_border_check_stack(bd);
             else
#endif
             e_border_focus_set_with_pointer(bd);
          }
        else if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DIALOG)
          {
             if ((e_config->focus_setting == E_FOCUS_NEW_DIALOG) ||
                 ((e_config->focus_setting == E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED) &&
                  (e_border_find_by_client_window(bd->client.icccm.transient_for) ==
                   e_border_focused_get())))
               {
                  e_border_focus_set_with_pointer(bd);
               }
          }
        else
          {
             /* focus window by default when it is the only one on desk */
             E_Border *bd2 = NULL;
             Eina_List *l;
             EINA_LIST_FOREACH(focus_stack, l, bd2)
               {
                  if (bd == bd2) continue;
                  if ((!bd2->iconic) && (bd2->visible) &&
                      ((bd->desk == bd2->desk) || bd2->sticky))
                    break;
               }

             if (!bd2)
               {
                  e_border_focus_set_with_pointer(bd);
               }
          }
     }

   if (bd->need_maximize)
     {
        E_Maximize max;
        max = bd->maximized;
        bd->maximized = E_MAXIMIZE_NONE;
        e_border_maximize(bd, max);
        bd->need_maximize = 0;
     }

   if (bd->need_fullscreen)
     {
        e_border_fullscreen(bd, e_config->fullscreen_policy);
        bd->need_fullscreen = 0;
     }

   if (rem_change)
     e_remember_update(bd);

   if (send_event) // FIXME: send only if a property changed - above need to
     { // check on that. for now - always send.
        event = E_NEW(E_Event_Border_Property, 1);
        if (event)
          {
             event->border = bd;
             e_object_ref(E_OBJECT(bd));
             ecore_event_add(E_EVENT_BORDER_PROPERTY, event, _e_border_event_border_property_free, NULL);
          }
        else
          ELB(ELBT_BD, "Out of Memory: E_NEW(E_Event_Border_Property)", 0);
     }

#ifdef _F_MOVE_RESIZE_SEND_PATCH_
   if (bd->client.icccm.move_resize_send.need_send)
     {
        ecore_x_icccm_move_resize_send(bd->client.win,
                                       bd->client.icccm.move_resize_send.x,
                                       bd->client.icccm.move_resize_send.y,
                                       bd->client.icccm.move_resize_send.w,
                                       bd->client.icccm.move_resize_send.h);
        bd->client.icccm.move_resize_send.need_send = 0;
     }
#endif

   /* wm rotation uses size information of client window for now.
    * thus, the message of rotation should be send after ConfigureWindow
    * for synchronization of information with client.
    * that's why this code is placed in here.
    * NOTE: it's mostly well working, but maybe exists exception case by timing.
    */
#ifdef _F_ZONE_WINDOW_ROTATION_
   if (bd->changes.rotation)
     {
        E_Zone *zone = bd->zone;

        if (bd->moving) e_border_act_move_end(bd, NULL);

        if ((!zone->rot.block_count) &&
            (!bd->client.e.state.deiconify_approve.pending_list) &&
            ((!bd->visible) ||
             (!E_INTERSECTS(bd->x, bd->y, bd->w, bd->h, zone->x, zone->y, zone->w, zone->h))))
          {
             // async list add
             ELB(ELBT_ROT, "ADD ASYNC LIST", bd->client.win);
             rot.async_list = eina_list_append(rot.async_list, bd);
          }
        else
          {
             // sync list add
             ELB(ELBT_ROT, "ADD SYNC LIST", bd->client.win);
             rot.list = eina_list_append(rot.list, bd);
             if (!rot.wait_prepare_done)
               _e_border_rotation_change_message_send(bd);
          }
        rot.fetch = EINA_TRUE;
        bd->changes.rotation = 0;
     }
#endif

#ifdef _F_DEICONIFY_APPROVE_
   if (bd->client.e.state.deiconify_approve.need_send)
     {
        bd->client.e.state.deiconify_approve.need_send = 0;
        bd->client.e.state.deiconify_approve.wait_timer =
           ecore_timer_add(e_config->deiconify_timeout, _e_border_uniconify_timeout, bd);

        if (bd->client.e.state.deiconify_approve.support)
          {
             long render_only = bd->client.e.state.deiconify_approve.render_only;
             ELBF(ELBT_BD, 0, bd->client.win, "SEND DEICONIFY_APPROVE.");
             ecore_x_client_message32_send(bd->client.win,
                                           ECORE_X_ATOM_E_DEICONIFY_APPROVE,
                                           ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                                           bd->client.win, 0, render_only, 0, 0);
          }
        else
          {
             ELBF(ELBT_BD, 0, bd->client.win, "DO DEICONIFY_APPROVE.");
             _e_border_deiconify_approve_done(bd->client.win, 1);
          }
     }
#endif

   _e_border_hook_call(E_BORDER_HOOK_EVAL_END, bd);
   traceEnd(TTRACE_TAG_WINDOW_MANAGER);
}

static void
_e_border_moveinfo_gather(E_Border   *bd,
                          const char *source)
{
   if (e_util_glob_match(source, "mouse,*,1")) bd->moveinfo.down.button = 1;
   else if (e_util_glob_match(source, "mouse,*,2"))
     bd->moveinfo.down.button = 2;
   else if (e_util_glob_match(source, "mouse,*,3"))
     bd->moveinfo.down.button = 3;
   else bd->moveinfo.down.button = 0;
   if ((bd->moveinfo.down.button >= 1) && (bd->moveinfo.down.button <= 3))
     {
        bd->moveinfo.down.mx = bd->mouse.last_down[bd->moveinfo.down.button - 1].mx;
        bd->moveinfo.down.my = bd->mouse.last_down[bd->moveinfo.down.button - 1].my;
     }
   else
     {
        bd->moveinfo.down.mx = bd->mouse.current.mx;
        bd->moveinfo.down.my = bd->mouse.current.my;
     }
}

static void
_e_border_resize_handle(E_Border *bd)
{
   int x, y, w, h;
   int new_x, new_y, new_w, new_h;
   int tw, th;
   Eina_List *skiplist = NULL;

   x = bd->x;
   y = bd->y;
   w = bd->w;
   h = bd->h;

   if ((bd->resize_mode == RESIZE_TR) ||
       (bd->resize_mode == RESIZE_R) ||
       (bd->resize_mode == RESIZE_BR))
     {
        if ((bd->moveinfo.down.button >= 1) &&
            (bd->moveinfo.down.button <= 3))
          w = bd->mouse.last_down[bd->moveinfo.down.button - 1].w +
            (bd->mouse.current.mx - bd->moveinfo.down.mx);
        else
          w = bd->moveinfo.down.w + (bd->mouse.current.mx - bd->moveinfo.down.mx);
     }
   else if ((bd->resize_mode == RESIZE_TL) ||
            (bd->resize_mode == RESIZE_L) ||
            (bd->resize_mode == RESIZE_BL))
     {
        if ((bd->moveinfo.down.button >= 1) &&
            (bd->moveinfo.down.button <= 3))
          w = bd->mouse.last_down[bd->moveinfo.down.button - 1].w -
            (bd->mouse.current.mx - bd->moveinfo.down.mx);
        else
          w = bd->moveinfo.down.w - (bd->mouse.current.mx - bd->moveinfo.down.mx);
     }

   if ((bd->resize_mode == RESIZE_TL) ||
       (bd->resize_mode == RESIZE_T) ||
       (bd->resize_mode == RESIZE_TR))
     {
        if ((bd->moveinfo.down.button >= 1) &&
            (bd->moveinfo.down.button <= 3))
          h = bd->mouse.last_down[bd->moveinfo.down.button - 1].h -
            (bd->mouse.current.my - bd->moveinfo.down.my);
        else
          h = bd->moveinfo.down.h - (bd->mouse.current.my - bd->moveinfo.down.my);
     }
   else if ((bd->resize_mode == RESIZE_BL) ||
            (bd->resize_mode == RESIZE_B) ||
            (bd->resize_mode == RESIZE_BR))
     {
        if ((bd->moveinfo.down.button >= 1) &&
            (bd->moveinfo.down.button <= 3))
          h = bd->mouse.last_down[bd->moveinfo.down.button - 1].h +
            (bd->mouse.current.my - bd->moveinfo.down.my);
        else
          h = bd->moveinfo.down.h + (bd->mouse.current.my - bd->moveinfo.down.my);
     }

   tw = bd->w;
   th = bd->h;

   if ((bd->resize_mode == RESIZE_TL) ||
       (bd->resize_mode == RESIZE_L) ||
       (bd->resize_mode == RESIZE_BL))
     x += (tw - w);
   if ((bd->resize_mode == RESIZE_TL) ||
       (bd->resize_mode == RESIZE_T) ||
       (bd->resize_mode == RESIZE_TR))
     y += (th - h);

   skiplist = eina_list_append(skiplist, bd);
   e_resist_container_border_position(bd->zone->container, skiplist,
                                      bd->x, bd->y, bd->w, bd->h,
                                      x, y, w, h,
                                      &new_x, &new_y, &new_w, &new_h);
   eina_list_free(skiplist);

   w = new_w;
   h = new_h;
   e_border_resize_limit(bd, &new_w, &new_h);
   if ((bd->resize_mode == RESIZE_TL) ||
       (bd->resize_mode == RESIZE_L) ||
       (bd->resize_mode == RESIZE_BL))
     new_x += (w - new_w);
   if ((bd->resize_mode == RESIZE_TL) ||
       (bd->resize_mode == RESIZE_T) ||
       (bd->resize_mode == RESIZE_TR))
     new_y += (h - new_h);

   e_border_move_resize(bd, new_x, new_y, new_w, new_h);
}

static Eina_Bool
_e_border_shade_animator(void *data)
{
   E_Border *bd = data;
   double dt, val;
   double dur = bd->client.h / e_config->border_shade_speed;

   dt = ecore_loop_time_get() - bd->shade.start;
   val = dt / dur;

   if (val < 0.0) val = 0.0;
   else if (val > 1.0) val = 1.0;

   if (e_config->border_shade_transition == E_TRANSITION_SINUSOIDAL)
     {
        bd->shade.val =
           ecore_animator_pos_map(val, ECORE_POS_MAP_SINUSOIDAL, 0.0, 0.0);
        if (!bd->shaded) bd->shade.val = 1.0 - bd->shade.val;
     }
   else if (e_config->border_shade_transition == E_TRANSITION_DECELERATE)
     {
        bd->shade.val =
           ecore_animator_pos_map(val, ECORE_POS_MAP_DECELERATE, 0.0, 0.0);
        if (!bd->shaded) bd->shade.val = 1.0 - bd->shade.val;
     }
   else if (e_config->border_shade_transition == E_TRANSITION_ACCELERATE)
     {
        bd->shade.val =
           ecore_animator_pos_map(val, ECORE_POS_MAP_ACCELERATE, 0.0, 0.0);
        if (!bd->shaded) bd->shade.val = 1.0 - bd->shade.val;
     }
   else if (e_config->border_shade_transition == E_TRANSITION_LINEAR)
     {
        bd->shade.val =
           ecore_animator_pos_map(val, ECORE_POS_MAP_LINEAR, 0.0, 0.0);
        if (!bd->shaded) bd->shade.val = 1.0 - bd->shade.val;
     }
   else if (e_config->border_shade_transition == E_TRANSITION_ACCELERATE_LOTS)
     {
        bd->shade.val =
           ecore_animator_pos_map(val, ECORE_POS_MAP_ACCELERATE_FACTOR, 1.7, 0.0);
        if (!bd->shaded) bd->shade.val = 1.0 - bd->shade.val;
     }
   else if (e_config->border_shade_transition == E_TRANSITION_DECELERATE_LOTS)
     {
        bd->shade.val =
           ecore_animator_pos_map(val, ECORE_POS_MAP_DECELERATE_FACTOR, 1.7, 0.0);
        if (!bd->shaded) bd->shade.val = 1.0 - bd->shade.val;
     }
   else if (e_config->border_shade_transition == E_TRANSITION_SINUSOIDAL_LOTS)
     {
        bd->shade.val =
           ecore_animator_pos_map(val, ECORE_POS_MAP_SINUSOIDAL_FACTOR, 1.7, 0.0);
        if (!bd->shaded) bd->shade.val = 1.0 - bd->shade.val;
     }
   else if (e_config->border_shade_transition == E_TRANSITION_BOUNCE)
     {
        bd->shade.val =
           ecore_animator_pos_map(val, ECORE_POS_MAP_BOUNCE, 1.2, 3.0);
        if (!bd->shaded) bd->shade.val = 1.0 - bd->shade.val;
     }
   else if (e_config->border_shade_transition == E_TRANSITION_BOUNCE_LOTS)
     {
        bd->shade.val =
           ecore_animator_pos_map(val, ECORE_POS_MAP_BOUNCE, 1.2, 5.0);
        if (!bd->shaded) bd->shade.val = 1.0 - bd->shade.val;
     }
   else
     {
        bd->shade.val =
           ecore_animator_pos_map(val, ECORE_POS_MAP_LINEAR, 0.0, 0.0);
        if (!bd->shaded) bd->shade.val = 1.0 - bd->shade.val;
     }

   /* due to M_PI's innacuracy, cos(M_PI/2) != 0.0, so we need this */
   if (bd->shade.val < 0.001) bd->shade.val = 0.0;
   else if (bd->shade.val > .999)
     bd->shade.val = 1.0;

   if (bd->shade.dir == E_DIRECTION_UP)
     bd->h = bd->client_inset.t + bd->client_inset.b + bd->client.h * bd->shade.val;
   else if (bd->shade.dir == E_DIRECTION_DOWN)
     {
        bd->h = bd->client_inset.t + bd->client_inset.b + bd->client.h * bd->shade.val;
        bd->y = bd->shade.y + bd->client.h * (1 - bd->shade.val);
        bd->changes.pos = 1;
     }
   else if (bd->shade.dir == E_DIRECTION_LEFT)
     bd->w = bd->client_inset.l + bd->client_inset.r + bd->client.w * bd->shade.val;
   else if (bd->shade.dir == E_DIRECTION_RIGHT)
     {
        bd->w = bd->client_inset.l + bd->client_inset.r + bd->client.w * bd->shade.val;
        bd->x = bd->shade.x + bd->client.w * (1 - bd->shade.val);
        bd->changes.pos = 1;
     }

   if ((bd->shaped) || (bd->client.shaped))
     {
        bd->need_shape_merge = 1;
        bd->need_shape_export = 1;
     }
   if (bd->shaped_input)
     {
        bd->need_shape_merge = 1;
     }
   bd->changes.size = 1;
   bd->changed = 1;

   /* we're done */
   if (val == 1)
     {
        E_Event_Border_Resize *ev;

        bd->shading = 0;
        bd->shaded = !(bd->shaded);
        bd->changes.size = 1;
        bd->changes.shaded = 1;
        bd->changes.shading = 1;
        bd->changed = 1;
        bd->shade.anim = NULL;

        if (bd->shaded)
          edje_object_signal_emit(bd->bg_object, "e,state,shaded", "e");
        else
          edje_object_signal_emit(bd->bg_object, "e,state,unshaded", "e");
        edje_object_message_signal_process(bd->bg_object);
        e_border_frame_recalc(bd);

        ecore_x_window_gravity_set(bd->client.win, ECORE_X_GRAVITY_NW);
        ev = E_NEW(E_Event_Border_Resize, 1);
        if (ev)
          {
             ev->border = bd;
             e_object_ref(E_OBJECT(bd));
             //	e_object_breadcrumb_add(E_OBJECT(bd), "border_resize_event");
             ecore_event_add(E_EVENT_BORDER_RESIZE, ev, _e_border_event_border_resize_free, NULL);
          }
        return ECORE_CALLBACK_CANCEL;
     }
   return ECORE_CALLBACK_RENEW;
}

static void
_e_border_event_border_resize_free(void *data __UNUSED__,
                                   void      *ev)
{
   E_Event_Border_Resize *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_resize_event");
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_event_border_move_free(void *data __UNUSED__,
                                 void      *ev)
{
   E_Event_Border_Move *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_move_event");
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_event_border_add_free(void *data __UNUSED__,
                                void      *ev)
{
   E_Event_Border_Add *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_add_event");
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_event_border_remove_free(void *data __UNUSED__,
                                   void      *ev)
{
   E_Event_Border_Remove *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_remove_event");
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_event_border_show_free(void *data __UNUSED__,
                                 void      *ev)
{
   E_Event_Border_Show *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_show_event");
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_event_border_hide_free(void *data __UNUSED__,
                                 void      *ev)
{
   E_Event_Border_Hide *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_hide_event");
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_event_border_iconify_free(void *data __UNUSED__,
                                    void      *ev)
{
   E_Event_Border_Iconify *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_iconify_event");
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_event_border_uniconify_free(void *data __UNUSED__,
                                      void      *ev)
{
   E_Event_Border_Uniconify *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_uniconify_event");
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

#ifdef _F_DEICONIFY_APPROVE_
static void
_e_border_event_border_force_render_free(void *data __UNUSED__,
                                                  void      *ev)
{
   E_Event_Border_Force_Render_Done *e;
   e = ev;
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}
#endif

static void
_e_border_event_border_stick_free(void *data __UNUSED__,
                                  void      *ev)
{
   E_Event_Border_Stick *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_stick_event");
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_event_border_unstick_free(void *data __UNUSED__,
                                    void      *ev)
{
   E_Event_Border_Unstick *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_unstick_event");
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_event_border_zone_set_free(void *data __UNUSED__,
                                     void      *ev)
{
   E_Event_Border_Zone_Set *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_zone_set_event");
   e_object_unref(E_OBJECT(e->border));
   e_object_unref(E_OBJECT(e->zone));
   E_FREE(e);
}

static void
_e_border_event_border_desk_set_free(void *data __UNUSED__,
                                     void      *ev)
{
   E_Event_Border_Desk_Set *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_desk_set_event");
   e_object_unref(E_OBJECT(e->border));
   e_object_unref(E_OBJECT(e->desk));
   E_FREE(e);
}

static void
_e_border_event_border_stack_free(void *data __UNUSED__,
                                  void      *ev)
{
   E_Event_Border_Stack *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_raise_event");
   e_object_unref(E_OBJECT(e->border));
   if (e->stack)
     {
//	e_object_breadcrumb_del(E_OBJECT(e->above), "border_raise_event.above");
          e_object_unref(E_OBJECT(e->stack));
     }
   E_FREE(e);
}

static void
_e_border_event_border_icon_change_free(void *data __UNUSED__,
                                        void      *ev)
{
   E_Event_Border_Icon_Change *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_icon_change_event");
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_event_border_urgent_change_free(void *data __UNUSED__,
                                          void      *ev)
{
   E_Event_Border_Urgent_Change *e;

   e = ev;
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_event_border_focus_in_free(void *data __UNUSED__,
                                     void      *ev)
{
   E_Event_Border_Focus_In *e;

   e = ev;
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_event_border_focus_out_free(void *data __UNUSED__,
                                      void      *ev)
{
   E_Event_Border_Focus_Out *e;

   e = ev;
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_event_border_property_free(void *data __UNUSED__,
                                     void      *ev)
{
   E_Event_Border_Property *e;

   e = ev;
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_event_border_fullscreen_free(void *data __UNUSED__,
                                       void      *ev)
{
   E_Event_Border_Fullscreen *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_fullscreen_event");
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_event_border_unfullscreen_free(void *data __UNUSED__,
                                         void      *ev)
{
   E_Event_Border_Unfullscreen *e;

   e = ev;
//   e_object_breadcrumb_del(E_OBJECT(e->border), "border_unfullscreen_event");
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

#ifdef _F_ZONE_WINDOW_ROTATION_
static void
_e_border_event_border_rotation_change_begin_free(void *data __UNUSED__,
                                                  void      *ev)
{
   E_Event_Border_Rotation_Change_Begin *e;
   e = ev;
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_event_border_rotation_change_cancel_free(void *data __UNUSED__,
                                                   void      *ev)
{
   E_Event_Border_Rotation_Change_Cancel *e;
   e = ev;
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_event_border_rotation_change_end_free(void *data __UNUSED__,
                                                void      *ev)
{
   E_Event_Border_Rotation_Change_End *e;
   e = ev;
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_event_border_rotation_change_begin_send(E_Border *bd)
{
   E_Event_Border_Rotation_Change_Begin *ev = NULL;
   ev = E_NEW(E_Event_Border_Rotation_Change_End, 1);
   if (ev)
     {
        ev->border = bd;
        e_object_ref(E_OBJECT(bd));
        ecore_event_add(E_EVENT_BORDER_ROTATION_CHANGE_BEGIN,
                        ev,
                        _e_border_event_border_rotation_change_begin_free,
                        NULL);
     }
}
#endif

#ifdef _F_USE_TILED_DESK_LAYOUT_
static void
_e_border_event_border_desk_layout_set_begin_free(void *data __UNUSED__,
                                                  void      *ev)
{
   E_Event_Border_Desk_Layout_Set_Begin *e = ev;
   e_object_unref(E_OBJECT(e->border));
   if (e->border->client.e.state.ly.prev_ly)
     e_object_unref(E_OBJECT(e->border->client.e.state.ly.prev_ly));
   if (e->border->client.e.state.ly.curr_ly)
     e_object_unref(E_OBJECT(e->border->client.e.state.ly.curr_ly));
   E_FREE(e);
}

static void
_e_border_event_border_desk_layout_set_end_free(void *data __UNUSED__,
                                                void      *ev)
{
   E_Event_Border_Desk_Layout_Set_End *e = ev;
   e->border->client.e.state.ly.changing = 0;
   e_object_unref(E_OBJECT(e->border));
   /* TODO: fix sigabort problem */
   /*
   if (e->border->client.e.state.ly.prev_ly)
     e_object_unref(E_OBJECT(e->border->client.e.state.ly.prev_ly));
   if (e->border->client.e.state.ly.curr_ly)
     e_object_unref(E_OBJECT(e->border->client.e.state.ly.curr_ly));
   */
   E_FREE(e);
}

/* TODO: remove this function when client protocol is complete. */
static Eina_Bool
_prop_card32_get(Ecore_X_Window win,
                 Ecore_X_Atom   atom,
                 int           *ret_val)
{
   unsigned int val = 0;
   if (ret_val) *ret_val = -1;
   int ret = ecore_x_window_prop_card32_get(win, atom, &val, 1);
   if (ret == 1)
     {
        if (ret_val) *ret_val = (int)val;
        return EINA_TRUE;
     }
   return EINA_FALSE;
}
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

static void
_e_border_event_border_anr_free(void *data __UNUSED__,
                                void      *ev)
{
   E_Event_Border_Anr *e;

   e = ev;
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}

static void
_e_border_zone_update(E_Border *bd)
{
   E_Container *con;
   Eina_List *l;
   E_Zone *zone;

   /* still within old zone - leave it there */
   if (E_INTERSECTS(bd->x, bd->y, bd->w, bd->h,
                    bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h))
#if _F_BORDER_CLIP_TO_ZONE_
     {
        _e_border_shape_input_clip_to_zone(bd);
        return;
     }
#else
     return;
#endif /* _F_BORDER_CLIP_TO_ZONE_ */
   /* find a new zone */
   con = bd->zone->container;
   EINA_LIST_FOREACH(con->zones, l, zone)
     {
        if (E_INTERSECTS(bd->x, bd->y, bd->w, bd->h,
                         zone->x, zone->y, zone->w, zone->h))
          {
             e_border_zone_set(bd, zone);
#if _F_BORDER_CLIP_TO_ZONE_
             _e_border_shape_input_clip_to_zone(bd);
#endif /* _F_BORDER_CLIP_TO_ZONE_ */
             return;
          }
     }
}

static int
_e_border_resize_begin(E_Border *bd)
{
   int ret;

   if (!bd->lock_user_stacking)
     {
        if (e_config->border_raise_on_mouse_action)
          e_border_raise(bd);
     }
   if ((bd->shaded) || (bd->shading) ||
       (bd->fullscreen) || (bd->lock_user_size))
     return 0;

   if (bd->client.icccm.accepts_focus || bd->client.icccm.take_focus)
     ret = e_grabinput_get(bd->win, 0, bd->win);
   else
     ret = e_grabinput_get(bd->win, 0, 0);

   if (grabbed && !ret)
     {
        grabbed = 0;
        return 0;
     }

   if (bd->client.netwm.sync.request)
     {
        bd->client.netwm.sync.alarm = ecore_x_sync_alarm_new(bd->client.netwm.sync.counter);
        bd->client.netwm.sync.serial = 1;
        bd->client.netwm.sync.wait = 0;
        bd->client.netwm.sync.send_time = ecore_loop_time_get();
     }

   _e_border_hook_call(E_BORDER_HOOK_RESIZE_BEGIN, bd);

   bdresize = bd;
   return 1;
}

static int
_e_border_resize_end(E_Border *bd)
{
   if (grabbed)
     {
        e_grabinput_release(bd->win, bd->win);
        grabbed = 0;
     }
   if (bd->client.netwm.sync.alarm)
     {
        E_Border_Pending_Move_Resize *pnd;

        ecore_x_sync_alarm_free(bd->client.netwm.sync.alarm);
        bd->client.netwm.sync.alarm = 0;
        /* resize to last geometry if sync alarm for it was not yet handled */
        if (bd->pending_move_resize)
          {
             bd->changed = 1;
             bd->changes.pos = 1;
             bd->changes.size = 1;
             _e_border_client_move_resize_send(bd);
          }

        EINA_LIST_FREE(bd->pending_move_resize, pnd)
          E_FREE(pnd);
     }

   _e_border_hook_call(E_BORDER_HOOK_RESIZE_END, bd);

   bdresize = NULL;

   /* If this border was maximized, we need to unset Maximized state or
    * on restart, E still thinks it's maximized */
   if (bd->maximized != E_MAXIMIZE_NONE)
     e_hints_window_maximized_set(bd, bd->maximized & E_MAXIMIZE_NONE,
                                  bd->maximized & E_MAXIMIZE_NONE);
   return 1;
}

static void
_e_border_resize_update(E_Border *bd)
{
   _e_border_hook_call(E_BORDER_HOOK_RESIZE_UPDATE, bd);
}

static int
_e_border_move_begin(E_Border *bd)
{
   int ret;
   if (!bd->lock_user_stacking)
     {
        if (e_config->border_raise_on_mouse_action)
          e_border_raise(bd);
     }
   if ((bd->fullscreen) || (bd->lock_user_location))
     return 0;

   if (bd->client.icccm.accepts_focus || bd->client.icccm.take_focus)
     {
#ifdef _F_MOVE_KEEPING_FOCUS_
        if (bd->client.illume.win_state.state != ECORE_X_ILLUME_WINDOW_STATE_NORMAL)
           ret = e_grabinput_get(bd->win, 0, 0);
        else
           ret = e_grabinput_get(bd->win, 0, bd->win);
#else
        ret = e_grabinput_get(bd->win, 0, bd->win);
#endif
     }
   else
     ret = e_grabinput_get(bd->win, 0, 0);

   if (grabbed && !ret)
     {
        grabbed = 0;
        return 0;
     }
#if 0
   if (bd->client.netwm.sync.request)
     {
        bd->client.netwm.sync.alarm = ecore_x_sync_alarm_new(bd->client.netwm.sync.counter);
        bd->client.netwm.sync.serial = 0;
        bd->client.netwm.sync.wait = 0;
        bd->client.netwm.sync.time = ecore_loop_time_get();
     }
#endif
   _e_border_hook_call(E_BORDER_HOOK_MOVE_BEGIN, bd);

   bdmove = bd;
   return 1;
}

static int
_e_border_move_end(E_Border *bd)
{
   if (grabbed)
     {
        e_grabinput_release(bd->win, bd->win);
        grabbed = 0;
     }
#if 0
   if (bd->client.netwm.sync.alarm)
     {
        ecore_x_sync_alarm_free(bd->client.netwm.sync.alarm);
        bd->client.netwm.sync.alarm = 0;
     }
#endif
   _e_border_hook_call(E_BORDER_HOOK_MOVE_END, bd);

   bdmove = NULL;
   return 1;
}

static void
_e_border_move_update(E_Border *bd)
{
   _e_border_hook_call(E_BORDER_HOOK_MOVE_UPDATE, bd);
}

static Eina_Bool
_e_border_cb_ping_poller(void *data)
{
   E_Border *bd;

   bd = data;
   if (bd->ping_ok)
     {
        if (bd->hung)
          {
             bd->hung = 0;
             edje_object_signal_emit(bd->bg_object, "e,state,unhung", "e");
             if (bd->kill_timer)
               {
                  ecore_timer_del(bd->kill_timer);
                  bd->kill_timer = NULL;
               }
          }
        bd->hung_count = 0;
     }
   else
     {
        int ping_check = 1;
        if (e_config->ping_only_visible)
          {
             if (!bd->visible)
               ping_check = 0;
          }

        if (ping_check)
          {
             /* if time between last ping and now is greater
              * than half the ping interval... */
             if ((ecore_loop_time_get() - bd->ping) >
                 ((e_config->ping_clients_interval *
                   ecore_poller_poll_interval_get(ECORE_POLLER_CORE)) / 2.0))
               {
                  bd->hung_count++;

                  LOGE("[ANR] - %d time no response, %s(%d,0x%x)",
                       bd->hung_count, bd->client.netwm.name, bd->client.netwm.pid, bd->client.win);

                  if (bd->hung_count >= e_config->max_hung_count)
                    {
                       if (!bd->hung)
                         {
                            bd->hung = 1;
                            edje_object_signal_emit(bd->bg_object, "e,state,hung", "e");

                            /* FIXME: if below dialog is up - hide it now */
                         }

                       if (e_config->kill_hung_process_by_module)
                         {
                            ELBF(ELBT_BD, 0, bd->client.win, "Not response %d times. PID:%d will be killed", e_config->max_hung_count, bd->client.netwm.pid);

                            E_Event_Border_Anr *ev = NULL;
                            ev = E_NEW(E_Event_Border_Anr, 1);
                            if (ev)
                              {
                                 ev->border = bd;
                                 e_object_ref(E_OBJECT(bd));
                                 ecore_event_add(E_EVENT_BORDER_ANR, ev, _e_border_event_border_anr_free, NULL);
                              }
                         }
                       else
                         {
                            if (bd->delete_requested)
                              {
                                 /* FIXME: pop up dialog saying app is hung - kill client, or pid */
                                 e_border_act_kill_begin(bd);
                              }
                         }
                    }
               }
          }
     }
   bd->ping_poller = NULL;

   if (e_config->ping_only_visible)
     {
        if (bd->visible)
          e_border_ping(bd);
     }
   else
     e_border_ping(bd);

   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_border_cb_kill_timer(void *data)
{
   E_Border *bd;

   bd = data;
// dont wait until it's hung -
//   if (bd->hung)
//     {
        if (bd->client.netwm.pid > 1)
          kill(bd->client.netwm.pid, SIGKILL);
//     }
   bd->kill_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_border_pointer_resize_begin(E_Border *bd)
{
   switch (bd->resize_mode)
     {
      case RESIZE_TL:
        e_pointer_type_push(bd->pointer, bd, "resize_tl");
        break;

      case RESIZE_T:
        e_pointer_type_push(bd->pointer, bd, "resize_t");
        break;

      case RESIZE_TR:
        e_pointer_type_push(bd->pointer, bd, "resize_tr");
        break;

      case RESIZE_R:
        e_pointer_type_push(bd->pointer, bd, "resize_r");
        break;

      case RESIZE_BR:
        e_pointer_type_push(bd->pointer, bd, "resize_br");
        break;

      case RESIZE_B:
        e_pointer_type_push(bd->pointer, bd, "resize_b");
        break;

      case RESIZE_BL:
        e_pointer_type_push(bd->pointer, bd, "resize_bl");
        break;

      case RESIZE_L:
        e_pointer_type_push(bd->pointer, bd, "resize_l");
        break;
     }
}

static void
_e_border_pointer_resize_end(E_Border *bd)
{
   switch (bd->resize_mode)
     {
      case RESIZE_TL:
        e_pointer_type_pop(bd->pointer, bd, "resize_tl");
        break;

      case RESIZE_T:
        e_pointer_type_pop(bd->pointer, bd, "resize_t");
        break;

      case RESIZE_TR:
        e_pointer_type_pop(bd->pointer, bd, "resize_tr");
        break;

      case RESIZE_R:
        e_pointer_type_pop(bd->pointer, bd, "resize_r");
        break;

      case RESIZE_BR:
        e_pointer_type_pop(bd->pointer, bd, "resize_br");
        break;

      case RESIZE_B:
        e_pointer_type_pop(bd->pointer, bd, "resize_b");
        break;

      case RESIZE_BL:
        e_pointer_type_pop(bd->pointer, bd, "resize_bl");
        break;

      case RESIZE_L:
        e_pointer_type_pop(bd->pointer, bd, "resize_l");
        break;
     }
}

static void
_e_border_pointer_move_begin(E_Border *bd)
{
   e_pointer_type_push(bd->pointer, bd, "move");
}

static void
_e_border_pointer_move_end(E_Border *bd)
{
   e_pointer_type_pop(bd->pointer, bd, "move");
}

static Eina_List *_e_border_hooks = NULL;
static int _e_border_hooks_delete = 0;
static int _e_border_hooks_walking = 0;

static void
_e_border_hooks_clean(void)
{
   Eina_List *l, *ln;
   E_Border_Hook *bh;

   EINA_LIST_FOREACH_SAFE(_e_border_hooks, l, ln, bh)
     {
        if (bh->delete_me)
          {
             _e_border_hooks = eina_list_remove_list(_e_border_hooks, l);
             free(bh);
          }
     }
}

static void
_e_border_hook_call(E_Border_Hook_Point hookpoint,
                    void               *bd)
{
   Eina_List *l;
   E_Border_Hook *bh;

   _e_border_hooks_walking++;
   EINA_LIST_FOREACH(_e_border_hooks, l, bh)
     {
        if (bh->delete_me) continue;
        if (bh->hookpoint == hookpoint) bh->func(bh->data, bd);
     }
   _e_border_hooks_walking--;
   if ((_e_border_hooks_walking == 0) && (_e_border_hooks_delete > 0))
     _e_border_hooks_clean();
}

EAPI E_Border_Hook *
e_border_hook_add(E_Border_Hook_Point               hookpoint,
                  void                              (*func)(void *data,
                                              void *bd),
                  void                             *data)
{
   E_Border_Hook *bh;

   bh = E_NEW(E_Border_Hook, 1);
   if (!bh) return NULL;
   bh->hookpoint = hookpoint;
   bh->func = func;
   bh->data = data;
   _e_border_hooks = eina_list_append(_e_border_hooks, bh);
   return bh;
}

EAPI void
e_border_hook_del(E_Border_Hook *bh)
{
   bh->delete_me = 1;
   if (_e_border_hooks_walking == 0)
     {
        _e_border_hooks = eina_list_remove(_e_border_hooks, bh);
        free(bh);
     }
   else
     _e_border_hooks_delete++;
}

EAPI void
e_border_focus_track_freeze(void)
{
   focus_track_frozen++;
}

EAPI void
e_border_focus_track_thaw(void)
{
   focus_track_frozen--;
}

EAPI E_Border *
e_border_under_pointer_get(E_Desk   *desk,
                           E_Border *exclude)
{
   E_Border *bd = NULL, *cbd;
   Eina_List *l;
   int x, y;

   /* We need to ensure that we can get the container window for the
    * zone of either the given desk or the desk of the excluded
    * window, so return if neither is given */
   if (desk)
     ecore_x_pointer_xy_get(desk->zone->container->win, &x, &y);
   else if (exclude)
     ecore_x_pointer_xy_get(exclude->desk->zone->container->win, &x, &y);
   else
     return NULL;

   EINA_LIST_FOREACH(e_border_raise_stack_get(), l, cbd)
     {
        if (!cbd) continue;
        /* If a border was specified which should be excluded from the list
         * (because it will be closed shortly for example), skip */
        if ((exclude) && (cbd == exclude)) continue;
        if ((desk) && (cbd->desk != desk)) continue;
        if (!E_INSIDE(x, y, cbd->x, cbd->y, cbd->w, cbd->h))
          continue;
        /* If the layer is higher, the position of the window is higher
         * (always on top vs always below) */
        if (!bd || (cbd->layer > bd->layer))
          bd = cbd;
     }
   return bd;
}

static Eina_Bool
_e_border_pointer_warp_to_center_timer(void *data __UNUSED__)
{
   if (warp_to)
     {
        int x, y;
        double spd;

        ecore_x_pointer_xy_get(warp_to_win, &x, &y);
        if ((x - warp_x) > 5 || (x - warp_x) < -5 ||
            (y - warp_y) > 5 || (y - warp_y) < -5)
          {
             /* User moved the mouse, so stop warping */
             warp_to = 0;
             goto cleanup;
          }

        /* We just use the same warp speed as configured
         * for the windowlist */
        spd = e_config->winlist_warp_speed;
        x = warp_x;
        y = warp_y;
        warp_x = (x * (1.0 - spd)) + (warp_to_x * spd);
        warp_y = (y * (1.0 - spd)) + (warp_to_y * spd);
        if (warp_x == x && warp_y == y)
          {
             warp_x = warp_to_x;
             warp_y = warp_to_y;
             warp_to = 0;
             goto cleanup;
          }
        ecore_x_pointer_warp(warp_to_win, warp_x, warp_y);
        return ECORE_CALLBACK_RENEW;
     }
 cleanup:
   ecore_timer_del(warp_timer);
   warp_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

EAPI int
e_border_pointer_warp_to_center(E_Border *bd)
{
   int x, y;

   /* Do not slide pointer when disabled (probably breaks focus
    * on sloppy/mouse focus but requested by users). */
   if (!e_config->pointer_slide) return 0;
   /* Only warp the pointer if it is not already in the area of
    * the given border */
   ecore_x_pointer_xy_get(bd->zone->container->win, &x, &y);
   if ((x >= bd->x) && (x <= (bd->x + bd->w)) &&
       (y >= bd->y) && (y <= (bd->y + bd->h)))
     return 0;

   warp_to_x = bd->x + (bd->w / 2);
   if (warp_to_x < (bd->zone->x + 1))
     warp_to_x = bd->zone->x + ((bd->x + bd->w - bd->zone->x) / 2);
   else if (warp_to_x > (bd->zone->x + bd->zone->w))
     warp_to_x = (bd->zone->x + bd->zone->w + bd->x) / 2;

   warp_to_y = bd->y + (bd->h / 2);
   if (warp_to_y < (bd->zone->y + 1))
     warp_to_y = bd->zone->y + ((bd->y + bd->h - bd->zone->y) / 2);
   else if (warp_to_y > (bd->zone->y + bd->zone->h))
     warp_to_y = (bd->zone->y + bd->zone->h + bd->y) / 2;

   warp_to = 1;
   warp_to_win = bd->zone->container->win;
   ecore_x_pointer_xy_get(bd->zone->container->win, &warp_x, &warp_y);
   if (!warp_timer)
     warp_timer = ecore_timer_add(0.01, _e_border_pointer_warp_to_center_timer, (const void *)bd);
   return 1;
}

EAPI void
e_border_comp_hidden_set(E_Border *bd,
                         Eina_Bool hidden)
{
   E_Border *tmp;
   Eina_List *l;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   EINA_LIST_FOREACH(bd->client.e.state.video_child, l, tmp)
     {
       if (hidden)
	 ecore_x_window_hide(tmp->win);
       else
	 ecore_x_window_show(tmp->win);
     }

   if (bd->comp_hidden == hidden) return;

   bd->comp_hidden = hidden;

   if ((bd->comp_hidden) || (bd->tmp_input_hidden > 0))
     {
        ecore_x_composite_window_events_disable(bd->win);
        ecore_x_window_ignore_set(bd->win, EINA_TRUE);
     }
   else
     {
        _e_border_shape_input_rectangle_set(bd);
        ecore_x_window_ignore_set(bd->win, EINA_FALSE);
     }
}

EAPI void
e_border_tmp_input_hidden_push(E_Border *bd)
{
   E_Border *tmp;
   Eina_List *l;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   EINA_LIST_FOREACH(bd->client.e.state.video_child, l, tmp)
     e_border_tmp_input_hidden_push(tmp);

   bd->tmp_input_hidden++;
   if (bd->tmp_input_hidden != 1) return;

   if ((bd->comp_hidden) || (bd->tmp_input_hidden > 0))
     {
        ecore_x_composite_window_events_disable(bd->win);
        ecore_x_window_ignore_set(bd->win, EINA_TRUE);
     }
   else
     {
        _e_border_shape_input_rectangle_set(bd);
        ecore_x_window_ignore_set(bd->win, EINA_FALSE);
     }
}

EAPI void
e_border_tmp_input_hidden_pop(E_Border *bd)
{
   E_Border *tmp;
   Eina_List *l;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   EINA_LIST_FOREACH(bd->client.e.state.video_child, l, tmp)
     e_border_tmp_input_hidden_pop(tmp);

   bd->tmp_input_hidden--;
   if (bd->tmp_input_hidden != 0) return;

   if ((bd->comp_hidden) || (bd->tmp_input_hidden > 0))
     {
        ecore_x_composite_window_events_disable(bd->win);
        ecore_x_window_ignore_set(bd->win, EINA_TRUE);
     }
   else
     {
        _e_border_shape_input_rectangle_set(bd);
        ecore_x_window_ignore_set(bd->win, EINA_FALSE);
     }
}

EAPI void
e_border_activate(E_Border *bd, Eina_Bool just_do_it)
{
   if ((e_config->focus_setting == E_FOCUS_NEW_WINDOW) ||
       ((bd->parent) &&
        ((e_config->focus_setting == E_FOCUS_NEW_DIALOG) ||
         ((bd->parent->focused) &&
          (e_config->focus_setting == E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED)))) ||
       (just_do_it))
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
        if (!bd->lock_user_stacking) e_border_raise(bd);
        if (!bd->lock_focus_out)
          {
             /* XXX ooffice does send this request for
              config dialogs when the main window gets focus.
              causing the pointer to jump back and forth.  */
             if ((e_config->focus_policy != E_FOCUS_CLICK) &&
                 !(bd->client.icccm.name && !strcmp(bd->client.icccm.name, "VCLSalFrame")))
               ecore_x_pointer_warp(bd->zone->container->win,
                                    bd->x + (bd->w / 2), bd->y + (bd->h / 2));
             e_border_focus_set(bd, 1, 1);
          }
     }
}

EAPI void
e_border_geometry_get(E_Border *bd, int *x, int *y, int *w, int *h)
{
   E_OBJECT_CHECK(bd);
   int bx = 0, by = 0, bw = 0, bh = 0;

#ifdef _F_USE_BORDER_TRANSFORM_
   if (e_border_transform_enable_get(bd))
     {
        e_border_transform_border_get(bd, &bx, &by, &bw, &bh, NULL);
     }
   else
#endif
     {
#ifdef _F_USE_ICONIFY_RESIZE_
        if ((e_config->use_iconify_resize) &&
            (bd->iconify_resize.support) &&
            (bd->iconify_resize.set))
          {

             bx = bd->iconify_resize.x;
             by = bd->iconify_resize.y;
             bw = bd->iconify_resize.w;
             bh = bd->iconify_resize.h;

          }
        else
#endif
          {
             bx = bd->x;
             by = bd->y;
             bw = bd->w;
             bh = bd->h;

          }
     }

   if (x) *x = bx;
   if (y) *y = by;
   if (w) *w = bw;
   if (h) *h = bh;
}

#ifdef _F_DEICONIFY_APPROVE_
EAPI void
e_border_force_render_request(E_Border *bd, Eina_Bool uniconify)
{
   E_Event_Border_Force_Render_Done *ev;
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:BORDER:FORCE RENDER REQ");

   if (e_config->deiconify_approve)
     {
        if (bd->client.e.state.deiconify_approve.render_only)
          {
             ELB(ELBT_BD, "DEICONIFY_APPROVE FORCE_RENDER is already running", bd->client.win);
             traceEnd(TTRACE_TAG_WINDOW_MANAGER);
             return;
          }
        if (bd->client.e.state.deiconify_approve.wait_timer)
          {
             ELB(ELBT_BD, "DEICONIFY_APPROVE WAIT_TIMER is already running", bd->client.win);
             traceEnd(TTRACE_TAG_WINDOW_MANAGER);
             return;
          }
        if (bd->client.e.state.deiconify_approve.pending)
          {
             ELB(ELBT_BD, "DEICONIFY_APPROVE PENDING_JOB is already running", bd->client.win);
             traceEnd(TTRACE_TAG_WINDOW_MANAGER);
             return;
          }

        if (bd->client.e.state.deiconify_approve.render_done == 0)
          {
             if (!bd->client.e.state.deiconify_approve.need_send)
               {
                  ELB(ELBT_BD, "Set DEICONIFY_APPROVE need_send, render_only", bd->client.win);
                  bd->client.e.state.deiconify_approve.need_send = 1;
                  bd->client.e.state.deiconify_approve.render_only = 1;
                  bd->changed = 1;
               }
             traceEnd(TTRACE_TAG_WINDOW_MANAGER);
             return;
          }

        bd->client.e.state.deiconify_approve.render_only = 0;

        ev = E_NEW(E_Event_Border_Force_Render_Done, 1);
        if (ev)
          {
             ev->border = bd;
             e_object_ref(E_OBJECT(bd));
             ecore_event_add(E_EVENT_BORDER_FORCE_RENDER_DONE, ev, _e_border_event_border_force_render_free, NULL);
          }

        if (bd->client.e.state.deiconify_approve.defer_deiconify)
          {
             bd->client.e.state.deiconify_approve.defer_deiconify = 0;

             if (!eina_list_data_find(_uniconify_pending_list, bd))
               {
                  ELB(ELBT_BD, "Not in pending list. Do uniconify.", bd->client.win);
                  e_border_uniconify(bd);
               }
             else
               _e_border_uniconify_pending_list_remove(bd);
          }

        bd->client.e.state.deiconify_approve.render_done = 0;
     }
   traceEnd(TTRACE_TAG_WINDOW_MANAGER);
}

EAPI Eina_List *
e_border_uniconify_pending_list_get(void)
{
   return _uniconify_pending_list;
}

EAPI Eina_List *
e_border_uniconify_waiting_list_get(void)
{
   return _uniconify_waiting_list;
}

EAPI void
e_border_pending_iconify(E_Border *bd)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (e_config->deiconify_approve)
     {
        int bx, by, bw, bh;
        e_border_geometry_get(bd, &bx, &by, &bw, &bh);

        if (bd->client.e.state.pending_event.iconify.pending)
          {
             ELB(ELBT_BD, "Already iconify PENDING.", bd->client.win);
             return;
          }

        if (bd->client.e.state.pending_event.pending)
          {
             bd->client.e.state.pending_event.iconify.pending = 1;
             bd->client.e.state.pending_event.iconify.win = bd->client.win;
             ELB(ELBT_BD, "PENDING iconify. Pending by another event", bd->client.win);
             return;
          }

#ifdef _F_USE_TILED_DESK_LAYOUT_
        if (e_config->use_tiled_desk_layout)
          {
             if (!bd->client.e.state.ly.curr_ly)
               {
                  if (!E_CONTAINS(bx, by, bw, bh, bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h))
                     goto do_iconify;
               }
             else
               {
                  if (!E_CONTAINS(bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h, bx, by, bw, bh))
                    goto do_iconify;
               }
          }
        else
          {
             if (!E_CONTAINS(bx, by, bw, bh, bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h))
               goto do_iconify;
          }
#else
        if (!E_CONTAINS(bx, by, bw, bh, bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h))
           goto do_iconify;
#endif /* end of _F_USE_TILED_DESK_LAYOUT_ */

        Eina_List *below_list;
        // 1. find below non-alpha full size win
        below_list = _e_border_find_below_win_list(bd);

        if (below_list)
          {
             bd->client.e.state.pending_event.pending = 1;

             Eina_List *l;
             E_Border *below_bd;

             bd->client.e.state.pending_event.iconify.pending = 1;
             bd->client.e.state.pending_event.iconify.win = bd->client.win;

             EINA_LIST_REVERSE_FOREACH(below_list, l, below_bd)
               {
                  bd->client.e.state.pending_event.wait_for_list =
                     eina_list_append(bd->client.e.state.pending_event.wait_for_list, below_bd);

                  below_bd->client.e.state.deiconify_approve.pending_list = eina_list_append(below_bd->client.e.state.deiconify_approve.pending_list, bd);
                  ELBF(ELBT_ROT, 0, bd->client.win, "below_win:%x, below_win's wait_bd:%x", below_bd->client.win, bd->client.win);

                  // 3. if not same then uniconify
                  if ((!below_bd->client.e.state.deiconify_approve.wait_timer) &&
                      (!below_bd->client.e.state.deiconify_approve.pending))
                    {
                       below_bd->client.e.state.deiconify_approve.not_raise = 1;
                       e_border_uniconify(below_bd);
                    }
               }
             eina_list_free(below_list);

             ELB(ELBT_BD, "PENDING iconify event", bd->client.win);
             return;
          }
     }

do_iconify:
   // iconified window requested by client
   LOGE("Set ICONIFY BY CLIENT [%x]", bd->client.win);
   ELBF(ELBT_BD, 0, bd->client.win, "Set ICONIFY BY CLIENT");
   bd->iconify_by_client = 1;
   e_border_iconify(bd);
}

static void
_e_border_window_pending_hide_event_free(void *data __UNUSED__,
                                         void *ev)
{
   Ecore_X_Event_Window_Hide *e;

   e = ev;
   E_FREE(e);
}

static void
_e_border_window_pending_destroy_event_free(void *data __UNUSED__,
                                            void *ev)
{
   Ecore_X_Event_Window_Destroy *e;

   e = ev;
   E_FREE(e);
}

static void
_e_border_window_pending_configure_request_event_free(void *data __UNUSED__,
                                            void *ev)
{
   Ecore_X_Event_Window_Configure_Request *e;

   e = ev;
   E_FREE(e);
}

static void
_e_border_window_pending_hide_event_send(E_Border *bd)
{
   Ecore_X_Event_Window_Hide *e;

   if (!bd) return;

   e = E_NEW(Ecore_X_Event_Window_Hide, 1);
   if (e)
     {
        e->win = bd->client.e.state.pending_event.hide.win;
        e->event_win = bd->client.e.state.pending_event.hide.event_win;
        e->time = ecore_x_current_time_get();
        e->send_event = bd->client.e.state.pending_event.hide.send_event;

        ecore_event_add(ECORE_X_EVENT_WINDOW_HIDE, e,
                        _e_border_window_pending_hide_event_free, NULL);

        ELB(ELBT_BD, "Send pended HIDE event", e->win);
     }
}

static void
_e_border_window_pending_destroy_event_send(E_Border *bd)
{
   Ecore_X_Event_Window_Destroy *e;

   if (!bd) return;

   e = E_NEW(Ecore_X_Event_Window_Destroy, 1);
   if (e)
     {
        e->win = bd->client.e.state.pending_event.destroy.win;
        e->event_win = bd->client.e.state.pending_event.destroy.event_win;
        e->time = ecore_x_current_time_get();

        ecore_event_add(ECORE_X_EVENT_WINDOW_DESTROY, e,
                        _e_border_window_pending_destroy_event_free, NULL);

        ELB(ELBT_BD, "Send pended DESTROY event", e->win);
     }
}

static void
_e_border_window_pending_configure_request_event_send(E_Border *bd)
{
   Ecore_X_Event_Window_Configure_Request *e;

   if (!bd) return;

   e = E_NEW(Ecore_X_Event_Window_Configure_Request, 1);
   if (e)
     {
        e->win = bd->client.e.state.pending_event.lower.win;
        e->parent_win = bd->client.e.state.pending_event.lower.parent_win;
        e->abovewin = bd->client.e.state.pending_event.lower.above_win;
        e->detail = bd->client.e.state.pending_event.lower.detail;
        e->value_mask = bd->client.e.state.pending_event.lower.value_mask;
        e->time = ecore_x_current_time_get();

        ecore_event_add(ECORE_X_EVENT_WINDOW_CONFIGURE_REQUEST, e,
                        _e_border_window_pending_configure_request_event_free, NULL);

        ELB(ELBT_BD, "Send pended CONFIGURE_REQUEST event", e->win);
     }
}

static void
_e_border_window_pending_event_send(E_Border *bd, E_Border *hold_bd)
{
   if (!bd) return;

   if (!bd->client.e.state.pending_event.pending)
     {
        ELBF(ELBT_BD, 0, bd->client.win, "No pending event");
        return;
     }

   bd->client.e.state.pending_event.wait_for_list = eina_list_remove(bd->client.e.state.pending_event.wait_for_list, hold_bd);
   if (bd->client.e.state.pending_event.wait_for_list)
     {
        ELBF(ELBT_BD, 0, bd->client.win, "hold_bd:%x. But list is NOT empty.", hold_bd->client.win);
        return;
     }

   ELBF(ELBT_BD, 0, bd->client.win, "hold_bd:%x. list is empty. DO flush event.", hold_bd->client.win);
   bd->client.e.state.pending_event.done = 1;

   ELBF(ELBT_BD, 0, bd->client.win, "ref count:%d", e_object_ref_get(E_OBJECT(bd)));

   if (bd->client.e.state.pending_event.iconify.pending)
     {
        bd->client.e.state.pending_event.iconify.pending = 0;

        // iconified window requested by client
        LOGE("Set ICONIFY BY CLIENT [%x]", bd->client.win);
        ELBF(ELBT_BD, 0, bd->client.win, "Set ICONIFY BY CLIENT");
        bd->iconify_by_client = 1;
        e_border_iconify(bd);

        // clear iconify event data
        bd->client.e.state.pending_event.iconify.win = 0;

        if ((!bd->client.e.state.pending_event.hide.pending) &&
            (!bd->client.e.state.pending_event.lower.pending) &&
            (!bd->client.e.state.pending_event.iconify.pending))
          {
             bd->client.e.state.pending_event.done = 0;
             bd->client.e.state.pending_event.pending = 0;
          }
     }

   if (bd->client.e.state.pending_event.hide.pending)
     {
        _e_border_window_pending_hide_event_send(bd);

        // clear hide event data
        bd->client.e.state.pending_event.hide.win = 0;
        bd->client.e.state.pending_event.hide.event_win = 0;
        bd->client.e.state.pending_event.hide.send_event = 0;
     }

   if (bd->client.e.state.pending_event.lower.pending)
     {
        _e_border_window_pending_configure_request_event_send(bd);

        // clear destroy event data
        bd->client.e.state.pending_event.lower.win = 0;
        bd->client.e.state.pending_event.lower.parent_win = 0;
        bd->client.e.state.pending_event.lower.above_win = 0;
        bd->client.e.state.pending_event.lower.detail = 0;
        bd->client.e.state.pending_event.lower.value_mask = 0;
     }

   if (bd->client.e.state.pending_event.destroy.pending)
     {
        _e_border_window_pending_destroy_event_send(bd);

        // clear destroy event data
        bd->client.e.state.pending_event.destroy.win = 0;
        bd->client.e.state.pending_event.destroy.event_win = 0;
     }
}

static void
_e_border_window_pending_event_flush(E_Border *bd)
{
   E_Border *pending_bd = NULL;

   if (!bd) return;
   if (!bd->client.e.state.deiconify_approve.pending_list) return;
   traceBegin(TTRACE_TAG_WINDOW_MANAGER,"WM:BORDER:WIN PENDING EVENT FLUSH");

   EINA_LIST_FREE(bd->client.e.state.deiconify_approve.pending_list, pending_bd)
     {
        ELBF(ELBT_BD, 0, bd->client.win, "Pending win:0x%07x", pending_bd->client.win);
        _e_border_window_pending_event_send(pending_bd, bd);
     }

   bd->client.e.state.deiconify_approve.pending_list = NULL;
   traceEnd(TTRACE_TAG_WINDOW_MANAGER);
}

static void
_e_border_msg_handler(void       *data,
                      const char *name,
                      const char *info,
                      int         val,
                      E_Object   *obj,
                      void       *msgdata)
{
   E_Manager *man = (E_Manager *)obj;
   E_Manager_Comp_Source *src = (E_Manager_Comp_Source *)msgdata;

   // handle only comp.manager msg
   if (strncmp(name, "comp.manager", sizeof("comp.manager"))) return;

   if (!strncmp(info, "visibility.src", sizeof("visibility.src")))
     {
        Ecore_X_Window win;
        E_Border *bd;
        Eina_Bool visible;

        win = e_manager_comp_src_window_get(man, src);
        bd = e_border_find_by_window(win);

        if (!bd) return;
        if (!bd->client.e.state.deiconify_approve.pending_list) return;

        visible = e_manager_comp_src_visible_get(man, src);
        if (visible)
          {
             _e_border_window_pending_event_flush(bd);
          }
     }
}
#endif


#ifdef _F_USE_BORDER_TRANSFORM_
EAPI void
e_border_transform_enable_set(E_Border *bd, Eina_Bool enable)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (!e_config->use_border_transform) return;

   ELBF(ELBT_BD, 0, bd->client.win, "Transform Enable:0x%07x %d -> %d", bd->client.win, bd->transform.enable, enable);

   if (bd->transform.enable && !enable)
     {
        int i, j;
        pixman_transform_t matrix;
        pixman_transform_init_identity(&matrix);

        _e_border_transform_change_event_send(bd);
        _e_border_transform_matrix_set(bd, &matrix);
        _e_border_transform_input_redirect_send(bd, &matrix);
     }

   bd->transform.enable = enable;
}

EAPI Eina_Bool
e_border_transform_enable_get(E_Border *bd)
{
   E_OBJECT_CHECK_RETURN(bd, EINA_FALSE);
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, EINA_FALSE);

   return bd->transform.enable;
}

EAPI void
e_border_transform_set(E_Border *bd, int x, int y, int w, int h, int angle)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   E_Border_Transform_Rect source_rect;
   E_Border_Transform_Rect dest_rect;
   E_Border_Transform_Rect window_dest_rect;
   pixman_transform_t dest_matrix;
   pixman_transform_t window_dest_matrix;
   E_Border_Transform_Vertex dest_vertex;
   E_Border_Transform_Vertex window_dest_vertex;

   if (!bd->transform.enable) return;

   source_rect = _e_border_transform_rect_client_rect_get(bd);
   dest_rect = _e_border_transform_rect_create(x, y, w, h, angle);
   bd->transform.input_geometry = dest_rect;

   dest_matrix = _e_border_transform_matrix_calculate(&dest_rect, &source_rect);

#ifdef _F_USE_VIRT_RESOLUTION_
/*In this API only scaling should be allowed when Stereo mode is set. Input transformation will be taken by comp_tizen.*/
   if (e_virtual_res_type_get() == E_VIRTUAL_RES_SINGLE_SOURCE)
     {
       _e_border_transform_change_event_send(bd);
       return;
     }
#endif  /* end of _F_USE_VIRT_RESOLUTION_ */

   if (bd->transform.parent)
     {
        pixman_transform_t parent_matrix;
        pixman_transform_t relative_matrix;

        E_Border *parent = NULL;
        parent = e_border_find_by_client_window(bd->transform.parent);

        if (parent)
          {
             parent_matrix = _e_border_transform_matrix_get(parent);
             pixman_transform_multiply(&relative_matrix, &parent_matrix, &dest_matrix);
          }

        dest_matrix = relative_matrix;
     }

   if (bd->transform.ratio_fit)
     {
        window_dest_rect = _e_border_transform_rect_ratio_rect_get(&dest_rect, &source_rect);
        window_dest_matrix = _e_border_transform_matrix_calculate(&window_dest_rect, &source_rect);
     }
   else
     {
        window_dest_rect = dest_rect;
        window_dest_matrix = dest_matrix;
     }

   dest_vertex = _e_border_transform_matrix_apply(&source_rect, &dest_matrix);
   window_dest_vertex = _e_border_transform_matrix_apply(&source_rect, &window_dest_matrix);
   bd->transform.vertex = window_dest_vertex;

   bd->transform.border_geometry = _e_border_transform_vertex_to_rect(&dest_vertex);
   bd->transform.geometry = _e_border_transform_vertex_to_rect(&window_dest_vertex);

   ELBF(ELBT_BD, 0, bd->client.win, "Transform Origin Size:0x%07x - [%dx%d  %dx%d]",
        bd->client.win, source_rect.x, source_rect.y, source_rect.w, source_rect.h);
   ELBF(ELBT_BD, 0, bd->client.win, "Transform Value:0x%07x -> [%d %d %d %d %d]", bd->client.win, bd->transform.geometry.x,
        bd->transform.geometry.y, bd->transform.geometry.w, bd->transform.geometry.h, bd->transform.geometry.angle);
   ELBF(ELBT_BD, 0, bd->client.win, "Transform Border Value:0x%07x -> [%d %d %d %d %d]", bd->client.win,  bd->transform.border_geometry.x,
        bd->transform.border_geometry.y, bd->transform.border_geometry.w, bd->transform.border_geometry.h,
        bd->transform.border_geometry.angle);

   _e_border_transform_change_event_send(bd);
   _e_border_transform_matrix_set(bd, &window_dest_matrix);
#ifdef _F_USE_VIRT_RESOLUTION_
   if (e_virtual_res_type_get() != E_VIRTUAL_RES_SINGLE_SOURCE)
     _e_border_transform_input_redirect_send(bd, &window_dest_matrix);
#else
   _e_border_transform_input_redirect_send(bd, &window_dest_matrix);
#endif

   if (bd->transform.child_wins)
     {
        ELBF(ELBT_BD, 0, bd->client.win, "Transform CHILD Exist!!");

        Eina_List *l = NULL;
        E_Border *temp_bd = NULL;

        EINA_LIST_FOREACH(bd->transform.child_wins, l, temp_bd)
          {
             ELBF(ELBT_BD, 0, bd->client.win, "Transform CHILD Exist!!  Update!! [%x]", temp_bd->client.win);
             e_border_transform_update(temp_bd);
          }
     }
}

EAPI void
e_border_transform_get(E_Border *bd, int *x, int *y, int *w, int *h, int *angle)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (bd->transform.enable)
     {
        if (x) *x = bd->transform.geometry.x;
        if (y) *y = bd->transform.geometry.y;
        if (w) *w = bd->transform.geometry.w;
        if (h) *h = bd->transform.geometry.h;
        if (angle) *angle = bd->transform.geometry.angle;
     }
}

EAPI void
e_border_transform_border_get(E_Border *bd, int *x, int *y, int *w, int *h, int *angle)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (bd->transform.enable)
     {
        if (x) *x = bd->transform.border_geometry.x;
        if (y) *y = bd->transform.border_geometry.y;
        if (w) *w = bd->transform.border_geometry.w;
        if (h) *h = bd->transform.border_geometry.h;
        if (angle) *angle = bd->transform.border_geometry.angle;
     }
}

EAPI void
e_border_transform_child_add(E_Border* bd, E_Border* parent)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   E_OBJECT_CHECK(parent);
   E_OBJECT_TYPE_CHECK(parent, E_BORDER_TYPE);

   // ??
   if (bd->client.w == 1 && bd->client.h == 1) return;

   if (bd->transform.parent != parent->client.win)
     {
        E_Border *bd_parent = e_border_find_by_client_window(bd->transform.parent);
        if (bd_parent)
           e_border_transform_child_remove(bd, bd_parent);
     }

   if (!eina_list_data_find(parent->transform.child_wins, bd))
     {
        ELBF(ELBT_BD, 0, bd->client.win, "RELATIVE | CHILD ADD [%x] -> [%x]", bd->client.win, parent->client.win);
        parent->transform.child_wins = eina_list_append(parent->transform.child_wins, bd);
        bd->transform.parent = parent->client.win;
     }
}

EAPI void
e_border_transform_child_remove(E_Border* bd, E_Border* parent)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   E_OBJECT_CHECK(parent);
   E_OBJECT_TYPE_CHECK(parent, E_BORDER_TYPE);

   if (eina_list_data_find(parent->transform.child_wins, bd))
      parent->transform.child_wins = eina_list_remove(parent->transform.child_wins, bd);

   bd->transform.parent = NULL;
}


EAPI void
e_border_transform_update(E_Border* bd)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);
   if (bd->transform.enable)
     {
        ELBF(ELBT_BD, 0, bd->client.win, "e_border_transform_update | [%d %d %d %d]",
             bd->transform.input_geometry.x, bd->transform.input_geometry.y, bd->transform.input_geometry.w, bd->transform.input_geometry.h);

        e_border_transform_set(bd, bd->transform.input_geometry.x,
                               bd->transform.input_geometry.y,
                               bd->transform.input_geometry.w,
                               bd->transform.input_geometry.h,
                               bd->transform.input_geometry.angle);

        if (bd->transform.child_wins)
          {
             Eina_List *l = NULL;
             E_Border *temp_bd = NULL;

             EINA_LIST_FOREACH(bd->transform.child_wins, l, temp_bd)
               {
                  e_border_transform_update(temp_bd);
               }
          }
     }
}

EAPI void
e_border_transform_force_pending_set(E_Border *bd, Eina_Bool force_pending)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (bd->transform.force_pending != force_pending)
     {
        bd->transform.force_pending = force_pending;

        ELBF(ELBT_BD, 0, bd->client.win, "FORCE_PENDING_POS | %s", force_pending ? "SET" : "UNSET");
        if (!force_pending)
          {
             e_border_transform_enable_set(bd, bd->transform.enable);
             e_border_transform_update(bd);
          }
     }
}

EAPI Eina_Bool
e_border_transform_force_pending_get(E_Border *bd)
{
   E_OBJECT_CHECK_RETURN(bd, EINA_FALSE);
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, EINA_FALSE);

   return bd->transform.force_pending;
}

EAPI void
e_border_transform_ratio_fit_set(E_Border *bd, Eina_Bool ratio_fit)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   bd->transform.ratio_fit = ratio_fit;
}

EAPI Eina_Bool
e_border_transform_ratio_fit_get(E_Border *bd)
{
   E_OBJECT_CHECK_RETURN(bd, EINA_FALSE);
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, EINA_FALSE);

   return bd->transform.ratio_fit;
}

EAPI E_Border_Transform_Vertex e_border_transform_vertex_get(E_Border *bd)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   return bd->transform.vertex;
}

static E_Border_Transform_Rect
_e_border_transform_rect_client_rect_get(E_Border *bd)
{
   E_Border_Transform_Rect result = _e_border_transform_rect_create(0, 0, 0, 0, 0);

   if (bd)
     {
        result.x = bd->x;
        result.y = bd->y;
        result.w = bd->client.w;
        result.h = bd->client.h;
        result.angle = 0;

#ifdef _F_USE_VIRT_RESOLUTION_
        if ((e_virtual_res_type_get() == E_VIRTUAL_RES_SINGLE_SOURCE) || (e_virtual_res_type_get() == E_VIRTUAL_RES_DOUBLE_SOURCE))
          {
             if ((result.w == 0) || (result.h == 0))
               {
                  E_Manager* man = e_manager_current_get();
                  if (!man) return result;
                  result.w = (double)man->w;
                  result.h = (double)man->h;
               }
          }
        else
#endif /* end of _F_USE_VIRT_RESOLUTION_ */
          {
             if (result.w == 0) result.w = 1;
             if (result.h == 0) result.h = 1;
          }
     }
   return result;
}

static E_Border_Transform_Rect
_e_border_transform_rect_create(int x, int y, int w, int h, int angle)
{
   E_Border_Transform_Rect result;

   result.x = x;
   result.y = y;
   result.w = w;
   result.h = h;
   result.angle = angle;

   return result;

}

static E_Border_Transform_Rect
_e_border_transform_vertex_to_rect(E_Border_Transform_Vertex* vertex)
{
   E_Border_Transform_Rect rect = _e_border_transform_rect_create(0, 0, 0, 0, 0);

   if (vertex)
     {
        rect.x = vertex->vertexPosition[0][0] + 0.5;
        rect.y = vertex->vertexPosition[0][1] + 0.5;
        rect.w = vertex->vertexPosition[2][0] - vertex->vertexPosition[0][0] + 0.5;
        rect.h = vertex->vertexPosition[2][1] - vertex->vertexPosition[0][1] + 0.5;
     }

   return rect;

}

static E_Border_Transform_Rect
_e_border_transform_rect_ratio_rect_get(E_Border_Transform_Rect *dest,
                                        E_Border_Transform_Rect *source)
{
   E_Border_Transform_Rect rect = _e_border_transform_rect_create(0, 0, 0, 0, 0);

   if (dest && source)
     {
        float sw = ( source->w == 0 ? 1 :  source->w);
        float sh = ( source->h == 0 ? 1 :  source->h);
        float xRatio = (float)dest->w / sw;
        float new_w, new_h;

        new_w = xRatio * source->w;
        new_h = xRatio * source->h;

        if (new_h > dest->h + 1)
          {
             float yRatio = (float)dest->h / sh;

             new_w = yRatio * source->w;
             new_h = yRatio * source->h;
          }
        rect.x = dest->x + (dest->w - new_w) / 2.0 + 0.5;
        rect.y = dest->y + (dest->h - new_h) / 2.0 + 0.5;
        rect.w = new_w + 0.5;
        rect.h = new_h + 0.5;
        rect.angle = 0;
     }

   return rect;
}

static pixman_transform_t
_e_border_transform_matrix_calculate(E_Border_Transform_Rect *dest, E_Border_Transform_Rect *source)
{
   pixman_transform_t matrix;
   pixman_transform_init_identity(&matrix);

   if (dest && source)
     {
        float sw = ( source->w == 0 ? 1 :  source->w);
        float sh = ( source->h == 0 ? 1 :  source->h);
        double radian = (3.141592 * (dest->angle - source->angle)) / 180.0;

        pixman_transform_translate(&matrix, NULL, pixman_double_to_fixed(-source->x), pixman_double_to_fixed(-source->y));
        pixman_transform_scale( &matrix, NULL, pixman_double_to_fixed((float)dest->w / sw), pixman_double_to_fixed((float)dest->h / sh));
        pixman_transform_translate(&matrix, NULL, pixman_double_to_fixed(-dest->w/2.0), pixman_double_to_fixed(-dest->h/2.0));
        pixman_transform_rotate(&matrix, NULL, pixman_double_to_fixed(cos(radian)), pixman_double_to_fixed(sin(radian)));
        pixman_transform_translate(&matrix, NULL, pixman_double_to_fixed((dest->w/2.0) + dest->x), pixman_double_to_fixed((dest->h/2.0) + dest->y));
     }

   return matrix;
}

static E_Border_Transform_Vertex
_e_border_transform_matrix_apply(E_Border_Transform_Rect *source,
                                 pixman_transform_t *matrix)
{
   int i;
   double z = 1.0;
   E_Border_Transform_Vertex result;
   pixman_vector_t pos[4];

   if (source && matrix)
     {
        pos[0].vector[0] = pixman_double_to_fixed(source->x);
        pos[0].vector[1] = pixman_double_to_fixed(source->y);
        pos[0].vector[2] = pixman_double_to_fixed(z);

        pos[1].vector[0] = pixman_double_to_fixed(source->x + source->w);
        pos[1].vector[1] = pixman_double_to_fixed(source->y);
        pos[1].vector[2] = pixman_double_to_fixed(z);

        pos[2].vector[0] = pixman_double_to_fixed(source->x + source->w);
        pos[2].vector[1] = pixman_double_to_fixed(source->y + source->h);
        pos[2].vector[2] = pixman_double_to_fixed(z);

        pos[3].vector[0] = pixman_double_to_fixed(source->x);
        pos[3].vector[1] = pixman_double_to_fixed(source->y + source->h);
        pos[3].vector[2] = pixman_double_to_fixed(z);

        // transformation
        for (i = 0 ; i < 4 ; ++i)
          {
             pixman_transform_point_3d(matrix, &pos[i]);

             result.vertexPosition[i][0] = pixman_fixed_to_double(pos[i].vector[0]);
             result.vertexPosition[i][1] = pixman_fixed_to_double(pos[i].vector[1]);
             result.vertexPosition[i][2] = pixman_fixed_to_double(pos[i].vector[2]);
          }
     }
   return result;
}

static void _e_border_transform_input_redirect_send(E_Border *bd, pixman_transform_t *matrix)
{
   XTransform transform;
   Ecore_X_Display *disp = ecore_x_display_get();
   Ecore_X_Window border_window = 0;
   int i, j;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

#ifdef _F_USE_VIRT_RESOLUTION_
   if (!((e_virtual_res_type_get() == E_VIRTUAL_RES_SINGLE_SOURCE) || (e_virtual_res_type_get() == E_VIRTUAL_RES_DOUBLE_SOURCE)))
#endif
   if (bd->transform.force_pending) return;

   if (!matrix) return;

   border_window = bd->win;

   for (i = 0 ; i < 3 ; ++i)
     {
        for (j = 0 ; j < 3 ; ++j)
          {
             transform.matrix[i][j] = matrix->matrix[i][j];
          }
     }

   ELBF(ELBT_BD, 0, bd->client.win, "Transform Maxtix %.2f %.2f %.2f", pixman_fixed_to_double(matrix->matrix[0][0]),
        pixman_fixed_to_double(matrix->matrix[0][1]), pixman_fixed_to_double(matrix->matrix[0][2]));
   ELBF(ELBT_BD, 0, bd->client.win, "Transform Maxtix %.2f %.2f %.2f", pixman_fixed_to_double(matrix->matrix[1][0]),
        pixman_fixed_to_double(matrix->matrix[1][1]), pixman_fixed_to_double(matrix->matrix[1][2]));
   ELBF(ELBT_BD, 0, bd->client.win, "Transform Maxtix %.2f %.2f %.2f", pixman_fixed_to_double(matrix->matrix[2][0]),
        pixman_fixed_to_double(matrix->matrix[2][1]), pixman_fixed_to_double(matrix->matrix[2][2]));

   XCompositeSetCoordinateTransform(disp, border_window, &transform);

#ifdef _F_USE_VIRT_RESOLUTION_
   if (!((e_virtual_res_type_get() == E_VIRTUAL_RES_SINGLE_SOURCE) || (e_virtual_res_type_get() == E_VIRTUAL_RES_DOUBLE_SOURCE)))
#endif
   _e_border_hook_call(E_BORDER_HOOK_TRANSFORM_UPDATE, bd);
}

static void _e_border_transform_change_event_send(E_Border *bd)
{
   E_Event_Border_Transform_Changed *ev = NULL;

   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (bd->transform.force_pending) return;

   ev = E_NEW(E_Event_Border_Transform_Changed, 1);
   if (ev)
     ev->border = bd;
   e_object_ref(E_OBJECT(bd));
   ecore_event_add(E_EVENT_BORDER_TRANSFORM_CHANGED, ev,
                   _e_border_event_border_transform_changed_free, NULL);
}

static void _e_border_event_border_transform_changed_free(void *data,
                                                          void *ev)
{
   E_Event_Border_Transform_Changed *e;
   e = ev;
   e_object_unref(E_OBJECT(e->border));
   E_FREE(e);
}


static pixman_transform_t
_e_border_transform_matrix_get(E_Border *bd)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   pixman_transform_t matrix;
   pixman_transform_init_identity(&matrix);

   int i = 0;
   int j = 0;

   for (i = 0; i < 3; i++)
      for (j = 0; j < 3; j++)
         matrix.matrix[i][j] = pixman_double_to_fixed(bd->transform.matrix[i][j]);

   return matrix;
}


static void
_e_border_transform_matrix_set(E_Border* bd, pixman_transform_t* matrix)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   if (matrix)
     {
        int i = 0;
        int j = 0;

        for (i = 0; i < 3; i++)
           for (j = 0; j < 3; j++)
              bd->transform.matrix[i][j] = pixman_fixed_to_double(matrix->matrix[i][j]);
     }
}
#endif /* end of _F_USE_BORDER_TRANSFORM_ */

#ifdef _F_USE_VIRT_RESOLUTION_
EAPI void
e_border_input_transform_enable_set(E_Border *bd, Eina_Bool enable)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   ELBF(ELBT_BD, 0, bd->client.win, "Transform Enable:0x%07x %d -> %d", bd->client.win, bd->transform.enable, enable);

   if (bd->transform.enable && !enable)
     {
        pixman_transform_t matrix;
        pixman_transform_init_identity(&matrix);

        _e_border_transform_input_redirect_send(bd, &matrix);
     }

   bd->transform.enable = enable;
}

EAPI void
e_border_input_transform_set(E_Border *bd, int x, int y, int w, int h, int angle)
{
   E_OBJECT_CHECK(bd);
   E_OBJECT_TYPE_CHECK(bd, E_BORDER_TYPE);

   E_Border_Transform_Rect source_rect;
   E_Border_Transform_Rect dest_rect;
   pixman_transform_t dest_matrix;
   E_Border_Transform_Vertex dest_vertex;
   
   if (!bd->transform.enable) return;


   dest_rect = _e_border_transform_rect_create(x, y, w, h, angle);
   bd->transform.input_geometry = dest_rect;

   source_rect = _e_border_transform_rect_client_rect_get(bd);

   dest_matrix = _e_border_transform_matrix_calculate(&dest_rect, &source_rect);


   dest_vertex = _e_border_transform_matrix_apply(&source_rect, &dest_matrix);
   
   _e_border_transform_input_redirect_send(bd, &dest_matrix);
}
#endif /* end of _F_USE_VIRT_RESOLUTION_ */

/*vim:ts=8 sw=3 sts=3 expandtab cino=>5n-3f0^-2{2(0W1st0*/

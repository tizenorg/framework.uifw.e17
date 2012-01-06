#include "e.h"

typedef struct _E_Smart_Data E_Smart_Data;

struct _E_Smart_Data
{
   Evas_Coord x, y, w, h;
   Evas_Object *obj;
   Evas_Object *eventarea;
   int size;
   const char *fdo;
   unsigned char fill_inside : 1;
   unsigned char scale_up : 1;
   unsigned char preload : 1;
   unsigned char loading : 1;
};

/* local subsystem functions */
static void _e_icon_smart_reconfigure(E_Smart_Data *sd);
static void _e_icon_smart_init(void);
static void _e_icon_smart_add(Evas_Object *obj);
static void _e_icon_smart_del(Evas_Object *obj);
static void _e_icon_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_icon_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_icon_smart_show(Evas_Object *obj);
static void _e_icon_smart_hide(Evas_Object *obj);
static void _e_icon_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_icon_smart_clip_set(Evas_Object *obj, Evas_Object * clip);
static void _e_icon_smart_clip_unset(Evas_Object *obj);
static void _e_icon_obj_prepare(Evas_Object *obj, E_Smart_Data *sd);
static void _e_icon_preloaded(void *data, Evas *e, Evas_Object *obj, void *event_info);

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

/* externally accessible functions */
EAPI Evas_Object *
e_icon_add(Evas *evas)
{
   _e_icon_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

static void
_e_icon_obj_prepare(Evas_Object *obj, E_Smart_Data *sd)
{
   if (!sd->obj) return;

   if (!strcmp(evas_object_type_get(sd->obj), "edje"))
     {
        Evas_Object *pclip;

        pclip = evas_object_clip_get(sd->obj);
        evas_object_del(sd->obj);
        sd->obj = evas_object_image_add(evas_object_evas_get(obj));
        evas_object_image_scale_hint_set(sd->obj, EVAS_IMAGE_SCALE_HINT_STATIC);
        evas_object_smart_member_add(sd->obj, obj);
        evas_object_event_callback_add(sd->obj, EVAS_CALLBACK_IMAGE_PRELOADED,
                                       _e_icon_preloaded, obj);
        evas_object_clip_set(sd->obj, pclip);
     }
}

EAPI Eina_Bool
e_icon_file_set(Evas_Object *obj, const char *file)
{
   E_Smart_Data *sd;
   int len;

   if (!file) return EINA_FALSE;
   if (!(sd = evas_object_smart_data_get(obj))) 
     return EINA_FALSE;

   len = strlen(file);
   if ((len > 4) && (!strcasecmp(file + len - 4, ".edj")))
      return e_icon_file_edje_set(obj, file, "icon");
   
   /* smart code here */
   _e_icon_obj_prepare(obj, sd);
   /* FIXME: 64x64 - unhappy about this. use icon size */
   sd->loading = 0;
   if (sd->fdo)
     {
        eina_stringshare_del(sd->fdo);
        sd->fdo = NULL;
     }

   if (sd->size != 0)
     evas_object_image_load_size_set(sd->obj, sd->size, sd->size);
   if (sd->preload) evas_object_hide(sd->obj);
   evas_object_image_file_set(sd->obj, file, NULL);
   if (evas_object_image_load_error_get(sd->obj) != EVAS_LOAD_ERROR_NONE)
     return EINA_FALSE;
   if (sd->preload)
     {
        sd->loading = 1;
        evas_object_image_preload(sd->obj, EINA_FALSE);
     }
   else if (evas_object_visible_get(obj))
     evas_object_show(sd->obj);

   _e_icon_smart_reconfigure(sd);
   return EINA_TRUE;
}

EAPI Eina_Bool
e_icon_file_key_set(Evas_Object *obj, const char *file, const char *key)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) 
     return EINA_FALSE;

   /* smart code here */
   sd->loading = 0;
   if (sd->fdo)
     {
        eina_stringshare_del(sd->fdo);
        sd->fdo = NULL;
     }
   _e_icon_obj_prepare(obj, sd);
   if (sd->size != 0)
     evas_object_image_load_size_set(sd->obj, sd->size, sd->size);
   if (sd->preload) evas_object_hide(sd->obj);
   evas_object_image_file_set(sd->obj, file, key);
   if (evas_object_image_load_error_get(sd->obj) != EVAS_LOAD_ERROR_NONE)
     return EINA_FALSE;
   if (sd->preload)
     {
        sd->loading = 1;
        evas_object_image_preload(sd->obj, 0);
     }
   else if (evas_object_visible_get(obj))
     evas_object_show(sd->obj);
   _e_icon_smart_reconfigure(sd);
   return EINA_TRUE;
}

EAPI Eina_Bool
e_icon_file_edje_set(Evas_Object *obj, const char *file, const char *part)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) 
     return EINA_FALSE;

   /* smart code here */
   if (sd->obj) evas_object_del(sd->obj);
   sd->loading = 0;
   if (sd->fdo)
     {
        eina_stringshare_del(sd->fdo);
        sd->fdo = NULL;
     }
   sd->obj = edje_object_add(evas_object_evas_get(obj));
   edje_object_file_set(sd->obj, file, part);
   if (evas_object_image_load_error_get(sd->obj) != EVAS_LOAD_ERROR_NONE)
     return EINA_FALSE;
   if (evas_object_visible_get(obj)) evas_object_show(sd->obj);
   evas_object_smart_member_add(sd->obj, obj);
   _e_icon_smart_reconfigure(sd);
   return EINA_TRUE;
}

EAPI Eina_Bool
e_icon_fdo_icon_set(Evas_Object *obj, const char *icon)
{
   E_Smart_Data *sd;
   const char *path;
   int len;
   
   if (!icon) return EINA_TRUE;
   if (icon[0] == '/') return e_icon_file_set(obj, icon);

   if (!(sd = evas_object_smart_data_get(obj))) 
     return EINA_FALSE;

   eina_stringshare_replace(&sd->fdo, icon);
   if (!sd->fdo) return EINA_FALSE;

   path = efreet_icon_path_find(e_config->icon_theme, sd->fdo, sd->size);
   if (!path) return EINA_TRUE;

   len = strlen(icon);
   if ((len > 4) && (!strcasecmp(icon + len - 4, ".edj")))
      return e_icon_file_edje_set(obj, path, "icon");
   
   /* smart code here */
   _e_icon_obj_prepare(obj, sd);
   sd->loading = 0;
   if (sd->size != 0)
     evas_object_image_load_size_set(sd->obj, sd->size, sd->size);
   if (sd->preload) evas_object_hide(sd->obj);
   evas_object_image_file_set(sd->obj, path, NULL);
   if (evas_object_image_load_error_get(sd->obj) != EVAS_LOAD_ERROR_NONE)
     return EINA_FALSE;
   if (sd->preload)
     {
        sd->loading = 1;
        evas_object_image_preload(sd->obj, 0);
     }
   else if (evas_object_visible_get(obj))
     evas_object_show(sd->obj);
   _e_icon_smart_reconfigure(sd);
   return EINA_TRUE;
}

EAPI void
e_icon_object_set(Evas_Object *obj, Evas_Object *o)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* smart code here */
   if (sd->obj) evas_object_del(sd->obj);
   sd->loading = 0;
   sd->obj = o;
   evas_object_smart_member_add(sd->obj, obj);
   if (evas_object_visible_get(obj)) evas_object_show(sd->obj);
   _e_icon_smart_reconfigure(sd);
}

EAPI const char *
e_icon_file_get(const Evas_Object *obj)
{
   E_Smart_Data *sd;
   const char *file;

   if (!(sd = evas_object_smart_data_get(obj))) return NULL;
   if (!strcmp(evas_object_type_get(sd->obj), "edje"))
     {
	edje_object_file_get(sd->obj, &file, NULL);
	return file;
     }
   evas_object_image_file_get(sd->obj, &file, NULL);
   return file;
}

EAPI void
e_icon_smooth_scale_set(Evas_Object *obj, Eina_Bool smooth)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   if (!strcmp(evas_object_type_get(sd->obj), "edje")) return;
   evas_object_image_smooth_scale_set(sd->obj, smooth);
}

EAPI Eina_Bool
e_icon_smooth_scale_get(const Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return EINA_FALSE;
   if (!strcmp(evas_object_type_get(sd->obj), "edje"))
     return EINA_FALSE;
   return evas_object_image_smooth_scale_get(sd->obj);
}

EAPI void
e_icon_alpha_set(Evas_Object *obj, Eina_Bool alpha)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   if (!strcmp(evas_object_type_get(sd->obj), "edje")) return;
   evas_object_image_alpha_set(sd->obj, alpha);
}

EAPI Eina_Bool
e_icon_alpha_get(const Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return EINA_FALSE;
   if (!strcmp(evas_object_type_get(sd->obj), "edje")) return EINA_FALSE;
   return evas_object_image_alpha_get(sd->obj);
}

EAPI void
e_icon_preload_set(Evas_Object *obj, Eina_Bool preload)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   sd->preload = preload;
}

EAPI Eina_Bool
e_icon_preload_get(const Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return EINA_FALSE;
   return sd->preload;
}

EAPI void
e_icon_size_get(const Evas_Object *obj, int *w, int *h)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) 
     {
	if (w) *w = 0;
	if (h) *h = 0;
	return;
     }
   evas_object_image_size_get(sd->obj, w, h);
}

EAPI Eina_Bool
e_icon_fill_inside_get(const Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return EINA_FALSE;
   return sd->fill_inside;
}

EAPI void
e_icon_fill_inside_set(Evas_Object *obj, Eina_Bool fill_inside)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   fill_inside = !!fill_inside;
   if (sd->fill_inside == fill_inside) return;
   sd->fill_inside = fill_inside;
   _e_icon_smart_reconfigure(sd);
}

EAPI Eina_Bool
e_icon_scale_up_get(const Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return EINA_FALSE;
   return sd->scale_up;
}

EAPI void
e_icon_scale_up_set(Evas_Object *obj, Eina_Bool scale_up)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   scale_up = !!scale_up;
   if (sd->scale_up == scale_up) return;
   sd->scale_up = scale_up;
   _e_icon_smart_reconfigure(sd);
}

EAPI void
e_icon_data_set(Evas_Object *obj, void *data, int w, int h)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   if (!strcmp(evas_object_type_get(sd->obj), "edje")) return;
   evas_object_image_size_set(sd->obj, w, h);
   evas_object_image_data_copy_set(sd->obj, data);
}

EAPI void *
e_icon_data_get(const Evas_Object *obj, int *w, int *h)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return NULL;
   if (!strcmp(evas_object_type_get(sd->obj), "edje")) return NULL;
   evas_object_image_size_get(sd->obj, w, h);
   return evas_object_image_data_get(sd->obj, 0);
}

EAPI void
e_icon_scale_size_set(Evas_Object *obj, int size)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   sd->size = size;
   if (!strcmp(evas_object_type_get(sd->obj), "edje"))
     return;
   evas_object_image_load_size_set(sd->obj, sd->size, sd->size);
}

EAPI int
e_icon_scale_size_get(const Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return 0;
   return sd->size;
}

EAPI void
e_icon_selected_set(const Evas_Object *obj, Eina_Bool selected)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   if (strcmp(evas_object_type_get(sd->obj), "edje")) return;
   if (selected)
     edje_object_signal_emit(sd->obj, "e,state,selected", "e");
   else
     edje_object_signal_emit(sd->obj, "e,state,unselected", "e");
}

/* local subsystem globals */
static void
_e_icon_smart_reconfigure(E_Smart_Data *sd)
{
   int iw, ih;
   Evas_Coord x, y, w, h;

   if (!sd->obj) return;
   if (!strcmp(evas_object_type_get(sd->obj), "edje"))
     {
	w = sd->w;
	h = sd->h;
	x = sd->x;
	y = sd->y;
	evas_object_move(sd->obj, x, y);
	evas_object_resize(sd->obj, w, h);
	evas_object_move(sd->eventarea, x, y);
	evas_object_resize(sd->eventarea, w, h);
     }
   else
     {
	ih = 0;
	ih = 0;
	evas_object_image_size_get(sd->obj, &iw, &ih);
	if (iw < 1) iw = 1;
	if (ih < 1) ih = 1;

	if (sd->fill_inside)
	  {
	     w = sd->w;
	     h = ((double)ih * w) / (double)iw;
	     if (h > sd->h)
	       {
		  h = sd->h;
		  w = ((double)iw * h) / (double)ih;
	       }
	  }
	else
	  {
	     w = sd->w;
	     h = ((double)ih * w) / (double)iw;
	     if (h < sd->h)
	       {
		  h = sd->h;
		  w = ((double)iw * h) / (double)ih;
	       }
	  }
	if (!sd->scale_up)
	  {
	     if ((w > iw) || (h > ih))
	       {
		  w = iw;
		  h = ih;
	       }
	  }
	x = sd->x + ((sd->w - w) / 2);
	y = sd->y + ((sd->h - h) / 2);
	evas_object_move(sd->obj, x, y);
	evas_object_image_fill_set(sd->obj, 0, 0, w, h);
	evas_object_resize(sd->obj, w, h);
	evas_object_move(sd->eventarea, x, y);
	evas_object_resize(sd->eventarea, w, h);
     }
}

static void
_e_icon_smart_init(void)
{
   if (_e_smart) return;
     {
	static Evas_Smart_Class sc = EVAS_SMART_CLASS_INIT_NAME_VERSION("e_icon");
	if (!sc.add)
	  {
	     sc.add = _e_icon_smart_add;
	     sc.del = _e_icon_smart_del;
	     sc.move = _e_icon_smart_move;
	     sc.resize = _e_icon_smart_resize;
	     sc.show = _e_icon_smart_show;
	     sc.hide = _e_icon_smart_hide;
	     sc.color_set = _e_icon_smart_color_set;
	     sc.clip_set = _e_icon_smart_clip_set;
	     sc.clip_unset = _e_icon_smart_clip_unset;
	  }
	_e_smart = evas_smart_class_new(&sc);
     }
}

static void
_e_icon_preloaded(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(data))) return;
   evas_object_smart_callback_call(data, "preloaded", NULL);
   evas_object_show(sd->obj);
   sd->loading = 0;
}

static void
_e_icon_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = calloc(1, sizeof(E_Smart_Data)))) return;
   sd->eventarea = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_color_set(sd->eventarea, 0, 0, 0, 0);
   evas_object_smart_member_add(sd->eventarea, obj);

   sd->obj = evas_object_image_add(evas_object_evas_get(obj));
   evas_object_event_callback_add(sd->obj, EVAS_CALLBACK_IMAGE_PRELOADED,
                                  _e_icon_preloaded, obj);
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   sd->fill_inside = 1;
   sd->scale_up = 1;
   sd->size = 64;
   evas_object_smart_member_add(sd->obj, obj);
   evas_object_smart_data_set(obj, sd);
}

static void
_e_icon_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   evas_object_del(sd->obj);
   evas_object_del(sd->eventarea);
   if (sd->fdo) eina_stringshare_del(sd->fdo);
   free(sd);
}

static void
_e_icon_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   if ((sd->x == x) && (sd->y == y)) return;
   sd->x = x;
   sd->y = y;
   _e_icon_smart_reconfigure(sd);
}

static void
_e_icon_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   if ((sd->w == w) && (sd->h == h)) return;
   sd->w = w;
   sd->h = h;
   if (sd->fdo)
     {
        const char *path;

        sd->size = MAX(w, h);
        path = efreet_icon_path_find(e_config->icon_theme, sd->fdo, sd->size);
        if (!path) return;

        /* smart code here */
        evas_object_image_load_size_set(sd->obj, sd->size, sd->size);
        evas_object_image_file_set(sd->obj, path, NULL);
        if (sd->preload)
          {
             sd->loading = 1;
             evas_object_image_preload(sd->obj, 0);
          }
     }

   _e_icon_smart_reconfigure(sd);
}

static void
_e_icon_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   if (!((sd->preload) && (sd->loading)))
     evas_object_show(sd->obj);
   evas_object_show(sd->eventarea);
}

static void
_e_icon_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   evas_object_hide(sd->obj);
   evas_object_hide(sd->eventarea);
}

static void
_e_icon_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   evas_object_color_set(sd->obj, r, g, b, a);
}

static void
_e_icon_smart_clip_set(Evas_Object *obj, Evas_Object * clip)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   evas_object_clip_set(sd->obj, clip);
   evas_object_clip_set(sd->eventarea, clip);
}

static void
_e_icon_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   evas_object_clip_unset(sd->obj);
   evas_object_clip_unset(sd->eventarea);
}

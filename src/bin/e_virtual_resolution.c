#include "e.h"

#ifdef _F_USE_VIRT_RESOLUTION_

/* local variables */
static Ecore_X_Window e_doubleWid  = 0 ;
static Ecore_X_Window e_popWid  = 0 ;
static int width_l = 0;
static int height_l = 0;
static int preferred_w = 0;
static int preferred_h = 0;
static int fb_w = 0;     /*current resolution width*/
static int fb_h = 0;     /*current resolution height*/

static E_Virtual_Res_Type virtual_res_type = E_VIRTUAL_RES_NONE;
static E_Virtual_Res_Data virtual_res_data;

/* local function prototypes */
void e_double_source_win_id_set(Ecore_X_Window wid);
Ecore_X_Window e_double_source_win_id_get(void);
void e_epop_source_win_id_set(Ecore_X_Window wid);
Ecore_X_Window e_epop_source_win_id_get(void);
void e_left_buffer_set(int w, int h);
void e_left_buffer_get(int *const w, int *const h);
void e_frame_buffer_set(int w, int h);
void e_frame_buffer_get(int *const w, int *const h);
void e_preferred_resolution_set(int w, int h);
void e_preferred_resolution_get(int *const w, int *const h);

void
e_virtual_res_data_clear(void)
{
   e_doubleWid  = 0 ;
   e_popWid = 0 ;
   width_l = 0;
   height_l = 0;
   preferred_w = 0;
   preferred_h = 0;
   fb_w = 0;     /*current resolution width*/
   fb_h = 0;     /*current resolution height*/

   virtual_res_type = E_VIRTUAL_RES_NONE;

   virtual_res_data.left_buffer_set = NULL;
   virtual_res_data.left_buffer_get = NULL;
   virtual_res_data.virtual_res_win_id_set = NULL;
   virtual_res_data.virtual_res_win_id_get = NULL;
   virtual_res_data.frame_buffer_set = NULL;
   virtual_res_data.frame_buffer_get = NULL;
   virtual_res_data.preferred_resolution_set = NULL;
   virtual_res_data.preferred_resolution_get = NULL;

}


EAPI void
e_virtual_res_enable_set(E_Virtual_Res_Type type)
{
   e_virtual_res_data_clear();
   virtual_res_type = type;

   switch(type)
     {
      case E_VIRTUAL_RES_SINGLE_SOURCE:
        virtual_res_data.left_buffer_set = e_left_buffer_set;
        virtual_res_data.left_buffer_get = e_left_buffer_get;
        virtual_res_data.virtual_res_win_id_set = NULL;
        virtual_res_data.virtual_res_win_id_get = NULL;
        virtual_res_data.frame_buffer_set = e_frame_buffer_set;
        virtual_res_data.frame_buffer_get = e_frame_buffer_get;
        virtual_res_data.preferred_resolution_set = e_preferred_resolution_set;
        virtual_res_data.preferred_resolution_get = e_preferred_resolution_get;
        break;

      case E_VIRTUAL_RES_DOUBLE_SOURCE:
        virtual_res_data.left_buffer_set = NULL;
        virtual_res_data.left_buffer_get = NULL;
        virtual_res_data.virtual_res_win_id_set = e_double_source_win_id_set;
        virtual_res_data.virtual_res_win_id_get = e_double_source_win_id_get;
        virtual_res_data.frame_buffer_set = e_frame_buffer_set;
        virtual_res_data.frame_buffer_get = e_frame_buffer_get;
        virtual_res_data.preferred_resolution_set = e_preferred_resolution_set;
        virtual_res_data.preferred_resolution_get = e_preferred_resolution_get;
        break;

      case E_VIRTUAL_RES_EPOP:
        virtual_res_data.left_buffer_set = NULL;
        virtual_res_data.left_buffer_get = NULL;
        virtual_res_data.virtual_res_win_id_set = e_epop_source_win_id_set;
        virtual_res_data.virtual_res_win_id_get = e_epop_source_win_id_get;
        virtual_res_data.frame_buffer_set = e_frame_buffer_set;
        virtual_res_data.frame_buffer_get = e_frame_buffer_get;
        virtual_res_data.preferred_resolution_set = e_preferred_resolution_set;
        virtual_res_data.preferred_resolution_get = e_preferred_resolution_get;
        break;

      case E_VIRTUAL_RES_NONE:
        break;
     }
}

EAPI E_Virtual_Res_Data*
e_virtual_res_data_get(void)
{
   if (virtual_res_type != E_VIRTUAL_RES_NONE)
     return &virtual_res_data;
   else
     return NULL;
}

EAPI E_Virtual_Res_Type
e_virtual_res_type_get(void)
{
   return virtual_res_type;
}

void
e_double_source_win_id_set(Ecore_X_Window wid)
{
   e_doubleWid = wid;
}

Ecore_X_Window
e_double_source_win_id_get(void)
{
   return e_doubleWid;
}

void
e_epop_source_win_id_set(Ecore_X_Window wid)
{
   e_popWid = wid;
}

Ecore_X_Window
e_epop_source_win_id_get(void)
{
   return e_popWid;
}


void
e_left_buffer_set(int w, int h)
{
   width_l = w;
   height_l = h;
}

void
e_left_buffer_get(int *const w, int *const h)
{
   if (w) *w = width_l;
   if (h) *h = height_l;
}

void
e_frame_buffer_set(int w, int h)
{
   fb_w= w;
   fb_h = h;
}

void
e_frame_buffer_get(int *const w, int *const h)
{
   if (w) *w = fb_w;
   if (h) *h = fb_h;
}

void
e_preferred_resolution_set(int w, int h)
{
   preferred_w = w;
   preferred_h = h;
}

void
e_preferred_resolution_get(int *const w, int *const h)
{
   if (w) *w = preferred_w;
   if (h) *h = preferred_h;
}

#endif /* end of _F_USE_VIRT_RESOLUTION_ */


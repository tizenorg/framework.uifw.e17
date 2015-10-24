#ifdef _F_USE_VIRT_RESOLUTION_

#ifndef E_VIRTUAL_RESOLUTION_H
# define E_VIRTUAL_RESOLUTION_H

typedef enum _E_Virtual_Res_Type
{
   E_VIRTUAL_RES_SINGLE_SOURCE,
   E_VIRTUAL_RES_DOUBLE_SOURCE,
   E_VIRTUAL_RES_EPOP,
   E_VIRTUAL_RES_NONE
} E_Virtual_Res_Type;

typedef struct _E_Virtual_Res_Data
{
   void (*virtual_res_win_id_set) (Ecore_X_Window wid);
   Ecore_X_Window (*virtual_res_win_id_get) (void);
   void (*left_buffer_set) (int w, int h);
   void (*left_buffer_get) (int *const w, int *const h);
   void (*frame_buffer_set) (int w, int h);
   void (*frame_buffer_get) (int *const w, int *const h);
   void (*preferred_resolution_set) (int w, int h);
   void (*preferred_resolution_get) (int *const w, int *const h);
} E_Virtual_Res_Data;


EAPI void e_virtual_res_enable_set(E_Virtual_Res_Type vir_res_type);
EAPI E_Virtual_Res_Data* e_virtual_res_data_get(void);
EAPI E_Virtual_Res_Type e_virtual_res_type_get(void);

#endif

#endif /* end of _F_USE_VIRT_RESOLUTION_*/

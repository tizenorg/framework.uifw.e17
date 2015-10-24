#ifndef E_WIDGET_CSLIDER_H
#define E_WIDGET_CSLIDER_H

#ifndef _F_DISABLE_E_WIDGET

Evas_Object * e_widget_cslider_add(Evas *e, E_Color_Component mode, E_Color *color, int vertical, int fixed);
void e_widget_cslider_color_value_set(Evas_Object *obj, E_Color *ec);
void e_widget_cslider_update(Evas_Object *obj);
void e_widget_cslider_mode_set(Evas_Object *obj, E_Color_Component mode);

#endif
#endif

#ifndef E_SPECTRUM_H
#define E_SPECTRUM_H

#ifndef _F_DISABLE_E_COLOR_CLASS

Evas_Object *e_spectrum_add(Evas *e);
void e_spectrum_color_value_set(Evas_Object *o, E_Color *cv);
void e_spectrum_mode_set(Evas_Object *o, E_Color_Component mode);
void e_spectrum_update(Evas_Object *o);

#endif
#endif

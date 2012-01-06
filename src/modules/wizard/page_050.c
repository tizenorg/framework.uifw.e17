#include "e.h"
#include "e_mod_main.h"

static int desktop_files = 1;

EAPI int
wizard_page_init(E_Wizard_Page *pg)
{
   return 1;
}
EAPI int
wizard_page_shutdown(E_Wizard_Page *pg)
{
   return 1;
}
EAPI int
wizard_page_show(E_Wizard_Page *pg)
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;
   
   o = e_widget_list_add(pg->evas, 1, 0);
   e_wizard_title_set(_("File Manager"));
   
   of = e_widget_framelist_add(pg->evas, _("Desktop Files"), 0);

   rg = e_widget_radio_group_new(&desktop_files);
   
   ob = e_widget_radio_add(pg->evas, _("No icons on desktop"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   evas_object_show(ob);
   ob = e_widget_radio_add(pg->evas, _("Enable desktop icons"), 1, rg);
   e_widget_framelist_object_append(of, ob);
   evas_object_show(ob);
   
   e_widget_list_object_append(o, of, 0, 0, 0.5);
   evas_object_show(ob);
   evas_object_show(of);

   e_wizard_page_show(o);
   pg->data = of;
   return 1; /* 1 == show ui, and wait for user, 0 == just continue */
}
EAPI int
wizard_page_hide(E_Wizard_Page *pg)
{
   evas_object_del(pg->data);
   return 1;
}
EAPI int
wizard_page_apply(E_Wizard_Page *pg)
{
   if (!desktop_files)
     {
	// FIXME: disable fileman
     }
   else
     {
	// FIXME: enable fileman
	// FIXME: populate ~/Desktop
     }
   return 1;
}

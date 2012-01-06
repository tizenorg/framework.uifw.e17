/* Quick launch chooser */
#include "e.h"
#include "e_mod_main.h"

static Eina_List *desktops = NULL;
static int *desktops_add = NULL;
static int desktops_num = 0;

static int
_cb_sort_desks(Efreet_Desktop *d1, Efreet_Desktop *d2)
{
   if (!d1->name) return 1;
   if (!d2->name) return -1;
   return strcmp(d1->name, d2->name);
}

EAPI int
wizard_page_init(E_Wizard_Page *pg __UNUSED__)
{
   Eina_List *desks = NULL;
   Efreet_Desktop *desk;
   
   desks = efreet_util_desktop_name_glob_list("*");
   desks = eina_list_sort(desks, 0, (Eina_Compare_Cb)_cb_sort_desks);
   EINA_LIST_FREE(desks, desk)
     {
        if (!desk->exec)
          {
             efreet_desktop_free(desk);
             continue;
          }
        desktops = eina_list_append(desktops, desk);
     }
   if (desktops)
     {
        desktops_num = eina_list_count(desktops);
        desktops_add = calloc(sizeof(int), desktops_num);
     }
   return 1;
}

EAPI int
wizard_page_shutdown(E_Wizard_Page *pg __UNUSED__)
{
   return 1;
}

EAPI int
wizard_page_show(E_Wizard_Page *pg __UNUSED__)
{
   Evas_Object *o, *of, *ob, *li, *ck;
   Evas_Coord mw, mh;
   Eina_List *l;
   int i;

   if (!desktops) return 0;
   
   o = e_widget_list_add(pg->evas, 1, 0);
   e_wizard_title_set(_("Quick Launch"));
   
   of = e_widget_framelist_add(pg->evas, _("Select Applications"), 0);

   li = e_widget_list_add(pg->evas, 1, 0);
   ob = e_widget_scrollframe_simple_add(pg->evas, li);
   e_widget_size_min_set(ob, 140 * e_scale, 140 * e_scale);

   for (i = 0, l = desktops; l ; l = l->next, i++)
     {
        Efreet_Desktop *desk;
        const char *icon;
        
        desk = l->data;
        icon = NULL;
        if (desk->icon)
          icon = efreet_icon_path_find(e_config->icon_theme, 
                                       desk->icon, 48);
        ck = e_widget_check_icon_add(pg->evas, desk->name, 
                                     icon, 32 * e_scale, 32 * e_scale,
                                     &(desktops_add[i]));
        e_widget_list_object_append(li, ck, 1, 1, 0.0);
        evas_object_show(ck);
     }

   e_widget_size_min_get(li, &mw, &mh);
   evas_object_resize(li, mw, mh);
   
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   evas_object_show(ob);
   evas_object_show(of);
   evas_object_show(li);
   
   e_wizard_page_show(o);

   return 1; /* 1 == show ui, and wait for user, 0 == just continue */
}

EAPI int
wizard_page_hide(E_Wizard_Page *pg)
{
   evas_object_del(pg->data);
   return 1;
}

EAPI int
wizard_page_apply(E_Wizard_Page *pg __UNUSED__)
{
   Efreet_Desktop *desk;
   Eina_List *l;
   int i;
   FILE *f;
   char buf[PATH_MAX];

   e_user_dir_concat_static(buf, "applications/bar/default");
   ecore_file_mkpath(buf);
   e_user_dir_concat_static(buf, "applications/bar/default/.order");
   f = fopen(buf, "w");
   if (f)
     {
        for (i = 0, l = desktops; l ; l = l->next, i++)
          {
             if (desktops_add[i])
               {
                  char *p;
                  
                  desk = l->data;
                  p = strrchr(desk->orig_path, '/');
                  if (!p) p = desk->orig_path;
                  else p++;
                  fprintf(f, "%s\n", p);
               }
          }
        fclose(f);
     }
   EINA_LIST_FREE(desktops, desk)
     efreet_desktop_free(desk);

   if (desktops_add)
     {
        free(desktops_add);
        desktops_add = NULL;
     }
   desktops_num = 0;
   return 1;
}

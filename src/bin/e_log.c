#include "e.h"

#ifndef E_LOG_LEVEL
# define E_LOG_LEVEL EINA_LOG_LEVEL_INFO
#endif

EINTERN int e_log_dom = -1;

static const char *_names[] = {
   "CRI",
   "ERR",
   "WRN",
   "INF",
   "DBG",
};

static void
_e_log_cb(const Eina_Log_Domain *d, Eina_Log_Level level, const char *file, const char *fnc __UNUSED__, int line, const char *fmt, void *data __UNUSED__, va_list args)
{
   const char *color;

   color = eina_log_level_color_get(level);
   fprintf(stdout,
           "%s%s<" EINA_COLOR_RESET "%s%s>" EINA_COLOR_RESET "%s:%d" EINA_COLOR_RESET " ",
           color, _names[level > EINA_LOG_LEVEL_DBG ? EINA_LOG_LEVEL_DBG : level],
           d->domain_str, color, file, line);
   vfprintf(stdout, fmt, args);
   putc('\n', stdout);
}

EINTERN int
e_log_init(void)
{
   e_log_dom = eina_log_domain_register("e", EINA_COLOR_WHITE);
   eina_log_domain_level_set("e", E_LOG_LEVEL);
   eina_log_print_cb_set(_e_log_cb, NULL);
   return (e_log_dom != -1);
}

EINTERN int
e_log_shutdown(void)
{
   eina_log_domain_unregister(e_log_dom);
   e_log_dom = -1;
   return 0;
}

#ifdef _F_E_LOGBUF_
EAPI unsigned int e_logbuf_type = ELBT_NONE;

#define MAX_LOGS 1024
#define MAX_LOG_FUNC_STR_SIZE 50
#define MAX_LOG_STR_SIZE 256

typedef struct _E_Log
{
   unsigned int      num;
   unsigned int      type;
   unsigned int      blank;
   char              func[MAX_LOG_FUNC_STR_SIZE];
   int               line;
   unsigned int      id; // Ecore_X_Window or Zone ID
   char              str[MAX_LOG_STR_SIZE];
} E_Log;

typedef struct _E_Logbuf
{
   E_DBus_Interface    *iface;
   int                  cur;
   int                  num;
   Eina_Bool            pts;
   FILE                *fp;
   Eina_List           *list; // list of E_Log
   Ecore_Event_Handler *h;
   Ecore_X_Atom         a;
} E_Logbuf;

static E_Logbuf *buf = NULL;

static void _log_disp(FILE *fp, E_Log *m);

EAPI void
e_logbuf_add(unsigned int type,
             const char  *func,
             int          line,
             const char  *str,
             unsigned int id)
{
   E_Log *msg = NULL;
   if (!buf) return;

   if (eina_list_count(buf->list) < MAX_LOGS)
     {
        msg = E_NEW(E_Log, 1);
        if (msg)
          buf->list = eina_list_append(buf->list, msg);
     }
   else
     {
        buf->cur = (buf->cur) % MAX_LOGS;
        msg = eina_list_nth(buf->list, buf->cur);
     }

   if (msg)
     {
        unsigned int n = 0;
        msg->num = buf->num++;
        msg->type = type;
        msg->blank = 0;

        n = sizeof(msg->func);
        if (n >= MAX_LOG_FUNC_STR_SIZE)
          {
             strncpy(msg->func, func, MAX_LOG_FUNC_STR_SIZE-1);
             msg->func[MAX_LOG_FUNC_STR_SIZE-1] = '\0';
          }
        else
          {
             strncpy(msg->func, func, n);
          }

        msg->line = line;
        msg->id = id;

        n = sizeof(msg->str);
        if (n >= MAX_LOG_STR_SIZE)
          {
             strncpy(msg->str, str, MAX_LOG_STR_SIZE-1);
             msg->str[MAX_LOG_STR_SIZE-1] = '\0';
          }
        else
          {
             strncpy(msg->str, str, n);
          }

        buf->cur = (buf->num) % MAX_LOGS;

        if ((buf->pts) && (buf->fp))
          _log_disp(buf->fp, msg);
     }
}

EAPI void
e_logbuf_fmt_add(unsigned int type,
                 unsigned int blank,
                 const char  *func,
                 int          line,
                 unsigned int id,
                 const char  *fmt,
                 ...)
{
   E_Log *msg = NULL;
   va_list args;
   if (!buf) return;

   if (eina_list_count(buf->list) < MAX_LOGS)
     {
        msg = E_NEW(E_Log, 1);
        if (msg)
          buf->list = eina_list_append(buf->list, msg);
     }
   else
     {
        buf->cur = (buf->cur) % MAX_LOGS;
        msg = eina_list_nth(buf->list, buf->cur);
     }

   if (msg)
     {
        unsigned int n = 0;
        msg->num = buf->num++;
        msg->type = type;
        msg->blank = blank;

        n = sizeof(msg->func);
        if (n >= MAX_LOG_FUNC_STR_SIZE)
          {
             strncpy(msg->func, func, MAX_LOG_FUNC_STR_SIZE-1);
             msg->func[MAX_LOG_FUNC_STR_SIZE-1] = '\0';
          }
        else
          {
             strncpy(msg->func, func, n);
          }

        msg->line = line;
        msg->id = id;

        va_start(args, fmt);
        vsnprintf(msg->str, sizeof(msg->str), fmt, args);
        va_end(args);

        buf->cur = (buf->num) % MAX_LOGS;

        if ((buf->pts) && (buf->fp))
          _log_disp(buf->fp, msg);
     }
}

static void
_e_logbuf_shutdown(void)
{
   e_logbuf_type = ELBT_NONE;

   if (buf)
     {
        E_Log *msg = NULL;
        EINA_LIST_FREE(buf->list, msg) E_FREE(msg);
        buf->list = NULL;
        if (buf->iface)
          {
             e_msgbus_interface_detach(buf->iface);
             e_dbus_interface_unref(buf->iface);
             buf->iface = NULL;
          }
        if (buf->h)
          ecore_event_handler_del(buf->h);
        memset(buf, 0, sizeof(E_Logbuf));
        E_FREE(buf);
        buf = NULL;
     }
}

static const char *
_type_name_get(unsigned int t)
{
   switch (t)
     {
      case ELBT_NONE:   return "NONE";   break;
      case ELBT_DFT:    return "DFT";    break;
      case ELBT_MNG:    return "MNG";    break;
      case ELBT_BD:     return "BD";     break;
      case ELBT_ROT:    return "ROT";    break;
      case ELBT_ILLUME: return "ILLUME"; break;
      case ELBT_COMP:   return "COMP";   break;
      case ELBT_MOVE:   return "MOVE";   break;
      case ELBT_ALL:    return "ALL";    break;
      default: break;
     }
   return NULL;
}

static void
_log_disp(FILE  *fp,
          E_Log *m)
{
   if (m->blank)
     {
        if (m->id)
          fprintf(fp, "%38.38s|0x%08x|%s\n",
                  "", m->id, m->str);
        else
          fprintf(fp, "%49.49s|%s\n",
                  "", m->str);
     }
   else
     {
        if (m->id)
          fprintf(fp, "%5d|%20.20s|%5d|%5.5s|0x%08x|%s\n",
                  m->num, m->func, m->line,
                  _type_name_get(m->type), m->id, m->str);
        else
          fprintf(fp, "%5d|%20.20s|%5d|%5.5s|%10.10s|%s\n",
                  m->num, m->func, m->line,
                  _type_name_get(m->type), "", m->str);
     }
}

static void
_e_log_dump(FILE *fp)
{
   Eina_List *l = NULL, *ll = NULL;
   E_Log *msg = NULL;
   unsigned int idx = 0;
   if ((!buf) || (!fp)) return;

   ll = eina_list_nth_list(buf->list, buf->cur);
   EINA_LIST_FOREACH(ll, l, msg)
     {
        _log_disp(fp, msg);
        idx = msg->num;
     }
   EINA_LIST_FOREACH(buf->list, l, msg)
     {
        if (msg->num < idx) break;
        _log_disp(fp, msg);
        idx = msg->num;
     }
   fprintf(fp, "Total:%d Num:%d\n", eina_list_count(buf->list), buf->num);
   fflush(fp);
}

static DBusMessage *
_e_log_cb_enable(E_DBus_Object *obj __UNUSED__,
                 DBusMessage       *msg)
{
   e_logbuf_type = ELBT_ALL;
   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_log_cb_disable(E_DBus_Object *obj __UNUSED__,
                  DBusMessage       *msg)
{
   e_logbuf_type = ELBT_NONE;
   return dbus_message_new_method_return(msg);
}

static DBusMessage *
_e_log_cb_dump(E_DBus_Object *obj __UNUSED__,
               DBusMessage       *msg)
{
   DBusError err;
   const char *path = NULL;
   FILE *fp = NULL;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err,
                              DBUS_TYPE_STRING, &path,
                              DBUS_TYPE_INVALID))
     {
        fprintf(stderr,
                "could not get Log.Dump arguments: %s: %s",
                err.name, err.message);
        dbus_error_free(&err);
     }
   else if (path)
     {
        if (buf)
          {
             Eina_Bool pts = EINA_FALSE;
             if (strstr(path, "/dev/pts/"))
               pts = EINA_TRUE;

             fp = fopen(path, "w");
             if (fp)
               {
                  _e_log_dump(fp);

                  if (buf->fp)
                    {
                       fclose(buf->fp);
                       buf->fp = NULL;
                    }

                  if (pts)
                    {
                       buf->pts = EINA_TRUE;
                       buf->fp = fp;
                    }
                  else
                    fclose(fp);

                  fprintf(stdout,
                          "dump log messages to file:%s\n",
                          path);
               }
          }
     }

   return dbus_message_new_method_return(msg);
}

static Eina_Bool
_e_log_cb_prop(void *data __UNUSED__,
               int type   __UNUSED__,
               void      *event)
{
   Ecore_X_Event_Window_Property *ev = event;
   char *path = NULL;
   FILE *fp = NULL;

   if (!ev) return ECORE_CALLBACK_PASS_ON;
   if (!ev->atom) return ECORE_CALLBACK_PASS_ON;
   if (!buf) return ECORE_CALLBACK_PASS_ON;
   if (ev->atom != buf->a) return ECORE_CALLBACK_PASS_ON;

   path = ecore_x_window_prop_string_get(ev->win, ev->atom);
   if (!path) return ECORE_CALLBACK_PASS_ON;

   fp = fopen(path, "w");
   if (!fp) return ECORE_CALLBACK_PASS_ON;

   _e_log_dump(fp);
   if (buf->fp)
     {
        fclose(buf->fp);
        buf->pts = EINA_FALSE;
        buf->fp = NULL;
     }
   fprintf(stdout,
           "dump log messages to file:%s\n",
           path);

   fclose(fp);
   return ECORE_CALLBACK_PASS_ON;
}

EINTERN int
e_logbuf_init(void)
{
   _e_logbuf_shutdown();

   buf = E_NEW(E_Logbuf, 1);
   if (!buf) return 0;

   buf->iface = e_dbus_interface_new("org.enlightenment.wm.Log");
   if (!buf->iface)
     {
        fprintf(stderr, "Cannot add org.enlightenment.wm.Log interface\n");
        memset(buf, 0, sizeof(E_Logbuf));
        E_FREE(buf);
        return 0;
     }

   e_dbus_interface_method_add(buf->iface, "Enable", "", "",   _e_log_cb_enable);
   e_dbus_interface_method_add(buf->iface, "Disable", "", "",  _e_log_cb_disable);
   e_dbus_interface_method_add(buf->iface, "Dump", "s", "",    _e_log_cb_dump);

   e_msgbus_interface_attach(buf->iface);

   buf->h = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, _e_log_cb_prop, NULL);
   buf->a = ecore_x_atom_get("_E_LOG");

   e_logbuf_type = ELBT_ALL;

   return 1;
}

EINTERN int
e_logbuf_shutdown(void)
{
   _e_logbuf_shutdown();
   return 1;
}
#endif

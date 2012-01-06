#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "e.h"

#ifdef HAVE_ECORE_IMF
#include <Ecore_IMF.h>
#endif

#include <fcntl.h>
#include <setjmp.h>

/*
 * i need to make more use of these when i'm baffled as to when something is
 * up. other hooks:
 * 
 *      void *(*__malloc_hook)(size_t size, const void *caller);
 * 
 *      void *(*__realloc_hook)(void *ptr, size_t size, const void *caller);
 *
 *      void *(*__memalign_hook)(size_t alignment, size_t size,
 *                               const void *caller);
 *
 *      void (*__free_hook)(void *ptr, const void *caller);
 *
 *      void (*__malloc_initialize_hook)(void);
 *
 *      void (*__after_morecore_hook)(void);
 *

static void my_init_hook(void);
static void my_free_hook(void *p, const void *caller);

static void (*old_free_hook)(void *ptr, const void *caller) = NULL;
void (*__free_hook)(void *ptr, const void *caller);

void (*__malloc_initialize_hook) (void) = my_init_hook;
static void
my_init_hook(void)
{
   old_free_hook = __free_hook;
   __free_hook = my_free_hook;
}

//void *magicfree = NULL;

static void 
my_free_hook(void *p, const void *caller)
{
   __free_hook = old_free_hook;
//   if ((p) && (p == magicfree))
//     {
//	printf("CAUGHT!!!!! %p ...\n", p);
//	abort();
//     }
   free(p);
   __free_hook = my_free_hook;
}
*/

EAPI int e_precache_end = 0;
EAPI Eina_Bool x_fatal = EINA_FALSE;

static int really_know = 0;

/* local subsystem functions */
static void _e_main_shutdown_push(int (*func)(void));
static void _e_main_shutdown(int errorcode);

static int  _e_main_x_shutdown(void);
static int  _e_main_dirs_init(void);
static int  _e_main_dirs_shutdown(void);
static int  _e_main_screens_init(void);
static int  _e_main_screens_shutdown(void);
static void _e_main_manage_all(void);
static int  _e_main_path_init(void);
static int  _e_main_path_shutdown(void);

static void _e_main_cb_x_fatal(void *data __UNUSED__);
static Eina_Bool _e_main_cb_signal_exit(void *data __UNUSED__, int ev_type __UNUSED__, void *ev __UNUSED__);
static Eina_Bool _e_main_cb_signal_hup(void *data __UNUSED__, int ev_type __UNUSED__, void *ev __UNUSED__);
static Eina_Bool _e_main_cb_signal_user(void *data __UNUSED__, int ev_type __UNUSED__, void *ev);
static Eina_Bool _e_main_cb_x_flusher(void *data __UNUSED__);
static Eina_Bool _e_main_cb_idler_before(void *data __UNUSED__);
static Eina_Bool _e_main_cb_idler_after(void *data __UNUSED__);
static Eina_Bool _e_main_cb_eet_cacheburst_end(void *data __UNUSED__);
static Eina_Bool _e_main_cb_startup_fake_end(void *data __UNUSED__);
static void _e_main_desk_save(void);
static void _e_main_desk_restore(E_Manager *man, E_Container *con);

/* local subsystem globals */
#define MAX_LEVEL 64
static int (*_e_main_shutdown_func[MAX_LEVEL]) (void);
static int _e_main_level = 0;
static int _e_cacheburst = 0;
static Eina_Bool locked = EINA_FALSE;

static jmp_buf x_fatal_buf;
static Eina_Bool inloop = EINA_FALSE;

static Eina_List *_e_main_idler_before_list = NULL;

static Ecore_Idle_Enterer *_e_main_idle_enterer_before  = NULL;
static Ecore_Idle_Enterer *_e_main_idle_enterer_after   = NULL;
static Ecore_Idle_Enterer *_e_main_idle_enterer_flusher = NULL;

#define TS_DO

#ifdef TS_DO   
#define TS(x) \
   { \
      t1 = ecore_time_unix_get(); \
      printf("ESTART: %1.5f [%1.5f] - %s\n", t1 - t0, t1 - t2, x); \
      t2 = t1; \
   }
static double t0, t1, t2;
#else
#define TS(x)
#endif

#undef DBG_EINA_STRINGSHARE

#ifdef DBG_EINA_STRINGSHARE
static Eina_Bool
stdbg(void *data __UNUSED__)
{
// enable to debug eina stringshare usage
   eina_stringshare_dump();
   return ECORE_CALLBACK_CANCEL;
}
#endif

/* externally accessible functions */
int
main(int argc, char **argv)
{
   int i;
   int nostartup = 0;
   int after_restart = 0; 
   int safe_mode = 0;
   char buf[PATH_MAX];
   char *s;
   struct sigaction action;
   double t, tstart;

#ifdef TS_DO
   t0 = t1 = t2 = ecore_time_unix_get();
#endif
   TS("begin");
   
#if 0
     { 
	stack_t ss;

	ss.ss_sp = malloc(8 * 1024);
	ss.ss_size = 8 * 1024;
	ss.ss_flags = 0;
	sigaltstack(&ss, NULL);
     }
#endif

   /* If _F_DO_NOT_USE_SIG_HANDLER_ is defined, enlighentment doesn't show error dialog */
   /* and Signals will be handled by another signal handler */
#ifndef _F_DO_NOT_USE_SIG_HANDLER_
   /* trap deadly bug signals and allow some form of sane recovery */
   /* or ability to gdb attach and debug at this point - better than your */
   /* wm/desktop vanishing and not knowing what happened */

   action.sa_sigaction = e_sigseg_act;
   action.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
   sigemptyset(&action.sa_mask);
   sigaction(SIGSEGV, &action, NULL);

   action.sa_sigaction = e_sigill_act;
   action.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
   sigemptyset(&action.sa_mask);
   sigaction(SIGILL, &action, NULL);

   action.sa_sigaction = e_sigfpe_act;
   action.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO; 
   sigemptyset(&action.sa_mask);
   sigaction(SIGFPE, &action, NULL);

   action.sa_sigaction = e_sigbus_act;
   action.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
   sigemptyset(&action.sa_mask);
   sigaction(SIGBUS, &action, NULL);

   action.sa_sigaction = e_sigabrt_act;
   action.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
   sigemptyset(&action.sa_mask);
   sigaction(SIGABRT, &action, NULL);

   TS("signals done");
#endif

   t = ecore_time_unix_get();
   s = getenv("E_START_TIME");
   if ((s) && (!getenv("E_RESTART_OK")))
     {
	tstart = atof(s);
	if ((t - tstart) < 5.0) safe_mode = 1;
     }

   tstart = t;
   snprintf(buf, sizeof(buf), "%1.1f", tstart);
   e_util_env_set("E_START_TIME", buf);

   /* FIXME: this is the init code for letting e be relocatable. right now
    * its not used - so i want to see if it can reliably determine its exe
    * prefix
    */
   TS("determine prefix");
   if (!e_prefix_determine(argv[0]))
     {
	fprintf(stderr,
		"ERROR: Enlightenment cannot determine its installed\n"
		"       prefix from the system or argv[0].\n"
		"       This is because it is not on Linux AND has been\n"
		"       Executed strangely. This is unusual.\n"
		);
     }
   TS("prefix done");

   /* for debugging by redirecting stdout of e to a log file to tail */
   setvbuf(stdout, NULL, _IONBF, 0);

   if (getenv("E_RESTART")) after_restart = 1;

   if (getenv("DESKTOP_STARTUP_ID")) 
     e_util_env_set("DESKTOP_STARTUP_ID", NULL);

   e_util_env_set("E_RESTART_OK", NULL);
   e_util_env_set("E_RESTART", "1");

   /* envrionment varabiles so you know E is running/launched you */
   e_util_env_set("PANTS", "ON");
   e_util_env_set("DESKTOP", "Enlightenment-0.17.0");

   TS("eina init");
   eina_init();
   _e_main_shutdown_push(eina_shutdown);

   TS("intl init");
   e_intl_init();
   _e_main_shutdown_push(e_intl_shutdown);

   TS("parse args");
   /* handle some command-line parameters */
   for (i = 1; i < argc; i++)
     {
	if ((!strcmp(argv[i], "-display")) && (i < (argc - 1)))
	  {
	     i++;
	     e_util_env_set("DISPLAY", argv[i]);
	  }
	else if ((!strcmp(argv[i], "-fake-xinerama-screen")) && (i < (argc - 1)))
	  {
	     int x, y, w, h;

	     i++;
	     /* WWxHH+XX+YY */
	     if (sscanf(argv[i], "%ix%i+%i+%i", &w, &h, &x, &y) == 4)
	       e_xinerama_fake_screen_add(x, y, w, h);
	  }
	else if (!strcmp(argv[i], "-good"))
	  {
	     good = 1;
	     evil = 0;
	     printf("LA LA LA\n");
	  }
	else if (!strcmp(argv[i], "-evil"))
	  {
	     good = 0;
	     evil = 1;
	     printf("MUHAHAHAHHAHAHAHAHA\n");
	  }
	else if (!strcmp(argv[i], "-psychotic"))
	  {
	     good = 1;
	     evil = 1;
	     printf("MUHAHALALALALALALALA\n");
	  }
	else if ((!strcmp(argv[i], "-profile")) && (i < (argc - 1)))
	  {
	     i++;
	     if (!getenv("E_CONF_PROFILE"))
	       e_util_env_set("E_CONF_PROFILE", argv[i]);
	  }
	else if (!strcmp(argv[i], "-i-really-know-what-i-am-doing-and-accept-full-responsibility-for-it"))
	  {
             really_know = 1;
	  }
	else if (!strcmp(argv[i], "-locked"))
	  {
	     locked = EINA_TRUE;
	     puts("enlightenment will start with desklock on.");
	  }
	else if ((!strcmp(argv[i], "-h")) ||
		 (!strcmp(argv[i], "-help")) ||
		 (!strcmp(argv[i], "--help")))
	  {
	     printf
	       (_(
		  "Options:\n"
		  "\t-display DISPLAY\n"
		  "\t\tConnect to display named DISPLAY.\n"
		  "\t\tEG: -display :1.0\n"
		  "\t-fake-xinerama-screen WxH+X+Y\n"
		  "\t\tAdd a FAKE xinerama screen (instead of the real ones)\n"
		  "\t\tgiven the geometry. Add as many as you like. They all\n"
		  "\t\treplace the real xinerama screens, if any. This can\n"
		  "\t\tbe used to simulate xinerama.\n"
		  "\t\tEG: -fake-xinerama-screen 800x600+0+0 -fake-xinerama-screen 800x600+800+0\n"
		  "\t-profile CONF_PROFILE\n"
		  "\t\tUse the configuration profile CONF_PROFILE instead of the user selected default or just \"default\".\n"
		  "\t-good\n"
		  "\t\tBe good.\n"
		  "\t-evil\n"
		  "\t\tBe evil.\n"
		  "\t-psychotic\n"
		  "\t\tBe psychotic.\n"
		  "\t-locked\n"
		  "\t\tStart with desklock on, so password will be asked.\n"
		  "\t-i-really-know-what-i-am-doing-and-accept-full-responsibility-for-it\n"
		  "\t\tIf you need this help, you don't need this option.\n"
		  )
		);
	     _e_main_shutdown(-1);
	  }
     }

   /* fix up DISPLAY to be :N.0 if no .screen is in it */
   s = getenv("DISPLAY");
   if (s)
     {
	char *p;

	p = strrchr(s, ':');
	if (!p)
	  {
	     snprintf(buf, sizeof(buf), "%s:0.0", s);
	     e_util_env_set("DISPLAY", buf);
	  }
	else
	  {
	     p = strrchr(p, '.');
	     if (!p)
	       {
		  snprintf(buf, sizeof(buf), "%s.0", s);
		  e_util_env_set("DISPLAY", buf);
	       }
	  }
     }

   TS("arg parse done");
   /* fixes for FOOLS that keep cp'ing default.edj into ~/.e/e/themes */
     {
	e_user_dir_concat_static(buf, "themes/default.edj");
	if (ecore_file_exists(buf)) ecore_file_unlink(buf);
     }

   TS("ecore init");
   /* basic ecore init */
   if (!ecore_init())
     {
	e_error_message_show(_("Enlightenment cannot Initialize Ecore!\n"
			       "Perhaps you are out of memory?"));
	_e_main_shutdown(-1);
     }
#ifdef HAVE_ECORE_IMF
   ecore_imf_init();
   _e_main_shutdown_push(ecore_imf_shutdown);
#endif
// FIXME: SEGV's on shutdown if fm2 windows up - disable for now. => is it history ?
  _e_main_shutdown_push(ecore_shutdown);

   /* init edje and set it up in frozen mode */
   edje_init();
   edje_freeze();
   _e_main_shutdown_push(edje_shutdown);

   _e_cacheburst++;
/* eet_cacheburst(_e_cacheburst); */
   ecore_timer_add(5.0, _e_main_cb_eet_cacheburst_end, NULL);

   TS("ecore_file init");
   /* init the file system */
   if (!ecore_file_init())
     {
	e_error_message_show(_("Enlightenment cannot initialize the File system.\n"
			       "Perhaps you are out of memory?"));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(ecore_file_shutdown);   

   TS("more ecore");
   /* setup my args */
   ecore_app_args_set(argc, (const char **)argv);

   /* setup a handler for when e is asked to exit via a system signal */
   if (!ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, _e_main_cb_signal_exit, NULL))
     {
	e_error_message_show(_("Enlightenment cannot set up an exit signal handler.\n"
			       "Perhaps you are out of memory?"));
	_e_main_shutdown(-1);
     }
   if (!ecore_event_handler_add(ECORE_EVENT_SIGNAL_HUP, _e_main_cb_signal_hup, NULL))
     {
	e_error_message_show(_("Enlightenment cannot set up a HUP signal handler.\n"
			       "Perhaps you are out of memory?"));
	_e_main_shutdown(-1);
     }
   if (!ecore_event_handler_add(ECORE_EVENT_SIGNAL_USER, _e_main_cb_signal_user, NULL))
     {
	e_error_message_show(_("Enlightenment cannot set up a USER signal handler.\n"
			       "Perhaps you are out of memory?"));
	_e_main_shutdown(-1);
     }

   /* an idle enterer to be called before all others */
   _e_main_idle_enterer_before = ecore_idle_enterer_before_add(_e_main_cb_idler_before, NULL);

   TS("x connect");
   /* init x */
   if (!ecore_x_init(NULL))
     {
	e_error_message_show(_("Enlightenment cannot initialize its X connection.\n"
			       "Have you set your DISPLAY variable?"));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(_e_main_x_shutdown);
   /* init white box of death alert */
   if (!e_alert_init(NULL))
     {
	e_error_message_show(_("Enlightenment cannot initialize its emergency alert system.\n"
			       "Have you set your DISPLAY variable?"));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_alert_shutdown);

   /* we want to have been launched by enlightenment_start. there is a very */
   /* good reason we want to have been launched this way, thus check */
   if (!getenv("E_START"))
     {
	e_alert_show("You are executing enlightenment directly. This is\n"
		     "bad. Please do not execute the \"enlightenment\"\n"
		     "binary. Use the \"enlightenment_start\" launcher. It\n"
		     "will handle setting up environment variables, paths,\n"
		     "and launching any other required services etc.\n"
		     "before enlightenment itself begins running.\n");
	_e_main_shutdown(-1);
     }

   TS("ecore_con");
   /* init generic communications */
   if (!ecore_con_init())
     {
	e_error_message_show(_("Enlightenment cannot initialize the connections system.\n"
			       "Perhaps you are out of memory?"));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(ecore_con_shutdown);
   /* init ipc */
   if (!ecore_ipc_init())
     {
	e_error_message_show(_("Enlightenment cannot initialize the IPC system.\n"
			       "Perhaps you are out of memory?"));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(ecore_ipc_shutdown);

   TS("xinerama");
   if (!e_xinerama_init())
     {
	e_error_message_show(_("Enlightenment cannot setup xinerama wrapping.\n"
			       "This should not happen."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_xinerama_shutdown);

   TS("randr");
   if (!e_randr_init())
     {
	e_error_message_show(_("Enlightenment cannot setup randr wrapping.\n"
			       "This should not happen."));
     }
   else
     {
       _e_main_shutdown_push(e_randr_shutdown);
     }

/* ecore_x_grab(); */

   ecore_x_io_error_handler_set(_e_main_cb_x_fatal, NULL);

   TS("x hints");
   /* Init window manager hints */
   e_hints_init();
   TS("x hints done");

   TS("ecore_evas init");
   /* init the evas wrapper */
   if (!ecore_evas_init())
     { 
	e_error_message_show(_("Enlightenment cannot initialize the Evas system.\n"
			       "Perhaps you are out of memory?"));
	_e_main_shutdown(-1);
    }
   ecore_evas_app_comp_sync_set(0); /* e doesn't sync to compositor - it should be one */
   if (!ecore_evas_engine_type_supported_get(ECORE_EVAS_ENGINE_SOFTWARE_XLIB))
     {
	e_error_message_show(_("Enlightenment found ecore_evas doesn't support the Software X11\n"
			       "rendering in Evas. Please check your installation of Evas and\n"
			       "Ecore and check they support the Software X11 rendering engine."));
	_e_main_shutdown(-1);
     }
   if (!ecore_evas_engine_type_supported_get(ECORE_EVAS_ENGINE_SOFTWARE_BUFFER))
     {
	e_error_message_show(_("Enlightenment found ecore_evas doesn't support the Software Buffer\n"
			       "rendering in Evas. Please check your installation of Evas and\n"
			       "Ecore and check they support the Software Buffer rendering engine."));
	_e_main_shutdown(-1);
     }
// ecore_evas closes evas - deletes objs - deletes fm widgets which tries to
// ipc to slave to stop monitoring - but ipc has been shut down. dont shut
// down.   
//   _e_main_shutdown_push(ecore_evas_shutdown);        
   TS("test done");

   /*** Finished loading subsystems, Loading WM Specifics ***/
   TS("configure");
   e_configure_init();

   TS("dirs");
   /* setup directories we will be using for configurations storage etc. */
   if (!_e_main_dirs_init())
     {
	e_error_message_show(_("Enlightenment cannot create directories in your home directory.\n"
			       "Perhaps you have no home directory or the disk is full?"));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(_e_main_dirs_shutdown);

   TS("filereg");
   /* setup file registry */
   if (!e_filereg_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its file registry system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_filereg_shutdown);

   TS("config");
   /* init config system */
   if (!e_config_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its config system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_config_shutdown);

   TS("env");
   /* init config system */
   if (!e_env_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its environment."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_env_shutdown);

   e_util_env_set("E_ICON_THEME", e_config->icon_theme);

   locked |= e_config->desklock_start_locked;

   /* set all execced stuff to configured priority */
   ecore_exe_run_priority_set(e_config->priority);

   TS("scale");
   /* init config system */
   if (!e_scale_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its scale system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_scale_shutdown);

   TS("pointer");
   if (!e_pointer_init())
     {
	e_error_message_show(_("Enlightenment cannot set up the pointer system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_pointer_shutdown);

   TS("path");
   /* setup paths for finding things */
   if (!_e_main_path_init())
     {
	e_error_message_show(_("Enlightenment cannot set up paths for finding files.\n"
			       "Perhaps you are out of memory?"));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(_e_main_path_shutdown);

   TS("ipc");
   /* setup e ipc service */
   if (e_ipc_init())
     _e_main_shutdown_push(e_ipc_shutdown);

   /* setup edje to animate @ e_config->framerate frames per sec. */
   edje_frametime_set(1.0 / e_config->framerate);

   TS("font");
   /* init font system */
   if (!e_font_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its font system."));
        _e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_font_shutdown);
   e_font_apply();
   e_canvas_recache();

   TS("theme");
   /* init theme system */
   if (!e_theme_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its theme system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_theme_shutdown);

   TS("move/resize info");
   /* init move resize popup */
   e_moveresize_init();
   _e_main_shutdown_push(e_moveresize_shutdown);
   
   TS("splash");
   /* setup init status window/screen */
   if (!e_init_init())
     {
        e_error_message_show(_("Enlightenment cannot set up init screen.\n"
                               "Perhaps you are out of memory?"));
        _e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_init_shutdown);
   if (!((!e_config->show_splash) || (after_restart)))
     {
	e_init_title_set(_("Enlightenment"));
	e_init_version_set(VERSION);
	e_init_show();
	pause();
     }

   e_init_status_set(_("Starting International Support"));
   TS("intl post");
   /* init intl system */
   if (!e_intl_post_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its intl system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_intl_post_shutdown);

   TS("efreet");
   /* init FDO desktop */
   if (!efreet_init())
     {
        e_error_message_show(_("Enlightenment cannot initialize the FDO desktop system.\n"
                               "Perhaps you are out of memory?"));
        _e_main_shutdown(-1);
     }
   _e_main_shutdown_push(efreet_shutdown);
   TS("efreet done");

   if (!really_know)
     {
        e_init_status_set(_("Testing Format Support"));
        TS("test file format support");   
          {
             Ecore_Evas *ee;
             Evas_Object *im, *txt;
             char buf[PATH_MAX];
             Evas_Coord tw, th;

             ee = ecore_evas_buffer_new(1, 1);
             if (!ee)
               {
                  e_error_message_show(_("Enlightenment found Evas can't create a buffer canvas. Please check\n"
                                         "Evas has Software Buffer engine support.\n"));
                  _e_main_shutdown(-1);
               }
             e_canvas_add(ee);
             im = evas_object_image_add(ecore_evas_get(ee));

             e_prefix_data_concat_static(buf, "data/images/test.png");
             evas_object_image_file_set(im, buf, NULL);
             if (evas_object_image_load_error_get(im) != EVAS_LOAD_ERROR_NONE)
               {
                  e_error_message_show(_("Enlightenment found Evas can't load PNG files. Check Evas has PNG\n"
                                         "loader support.\n"));
                  _e_main_shutdown(-1);
               }

	     e_prefix_data_concat_static(buf, "data/images/test.jpg");
             evas_object_image_file_set(im, buf, NULL);
             if (evas_object_image_load_error_get(im) != EVAS_LOAD_ERROR_NONE)
               {
                  e_error_message_show(_("Enlightenment found Evas can't load JPEG files. Check Evas has JPEG\n"
                                         "loader support.\n"));
                  _e_main_shutdown(-1);
               }
             
             e_prefix_data_concat_static(buf, "data/images/test.edj");
             evas_object_image_file_set(im, buf, "images/0");
             if (evas_object_image_load_error_get(im) != EVAS_LOAD_ERROR_NONE)
               {
                  e_error_message_show(_("Enlightenment found Evas can't load EET files. Check Evas has EET\n"
                                         "loader support.\n"));
                  _e_main_shutdown(-1);
               }
             evas_object_del(im);
             txt = evas_object_text_add(ecore_evas_get(ee));
             evas_object_text_font_set(txt, "Sans", 10);
             evas_object_text_text_set(txt, "Hello");
             evas_object_geometry_get(txt, NULL, NULL, &tw, &th);
             if ((tw <= 0) && (th <= 0))
               {
                  e_error_message_show(_("Enlightenment found Evas can't load the 'Sans' font. Check Evas has fontconfig\n"
                                         "support and system fontconfig defines a 'Sans' font.\n"));
                  _e_main_shutdown(-1);
               }
             evas_object_del(txt);
             e_canvas_del(ee);
             ecore_evas_free(ee);
          }
     }

   e_init_status_set(_("Check SVG Support"));
   TS("svg");
     {
        Ecore_Evas *ee;
        Evas_Object *im;
        char buf[PATH_MAX];

        ee = ecore_evas_buffer_new(1, 1);
        if (!ee)
          {
             e_error_message_show(_("Enlightenment found Evas can't create a buffer canvas. Please check\n"
                                    "Evas has Software Buffer engine support.\n"));
             _e_main_shutdown(-1);
          }
        e_canvas_add(ee);
        im = evas_object_image_add(ecore_evas_get(ee));

        e_prefix_data_concat_static(buf, "data/images/test.svg");
        evas_object_image_file_set(im, buf, NULL);
        if (evas_object_image_load_error_get(im) == EVAS_LOAD_ERROR_NONE)
          {
             efreet_icon_extension_add(".svg");
             /* prefer png over svg */
             efreet_icon_extension_add(".png");
          }
        evas_object_del(im);
        e_canvas_del(ee);
        ecore_evas_free(ee);
     }

   e_init_status_set(_("Setup Screens"));
   TS("screens");
   /* manage the root window */
   if (!_e_main_screens_init())
     {
	e_error_message_show(_("Enlightenment set up window management for all the screens on your system\n"
			       "failed. Perhaps another window manager is running?\n"));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(_e_main_screens_shutdown);

   e_init_status_set(_("Setup Screensaver"));
   TS("screensaver");
   /* setup screensaver */
   if (!e_screensaver_init())
     {
	e_error_message_show(_("Enlightenment cannot configure the X screensaver."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_screensaver_shutdown);

   e_init_status_set(_("Setup Desklock"));
   TS("desklock");
   /* setup desklock */
   if (!e_desklock_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its desk locking system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_desklock_shutdown);

   e_init_status_set(_("Setup Popups"));
   TS("popup");
   /* init popup system */
   if (!e_popup_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its popup system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_popup_shutdown);

   if ((locked) && ((!e_config->show_splash) && (!after_restart)))
     e_desklock_show();

   TS("msgbus");
   /* setup e msgbus (DBUS) service */
   if (e_msgbus_init())
     _e_main_shutdown_push(e_msgbus_shutdown);

   e_init_status_set(_("Setup Paths"));
   TS("efreet paths");
     {
	Eina_List **list;

	list = efreet_icon_extra_list_get();
	if (list)
	  {
	     e_user_dir_concat_static(buf, "icons");
	     *list = eina_list_prepend(*list, (void *)eina_stringshare_add(buf));
	     e_prefix_data_concat_static(buf, "data/icons");
	     *list = eina_list_prepend(*list, (void *)eina_stringshare_add(buf));
	  }
     }
   efreet_icon_extension_add(".edj");
   TS("efreet paths done");

   e_init_status_set(_("Setup System Controls"));
   TS("sys init");
   /* init the enlightenment sys command system */
   if (!e_sys_init())
    {
       e_error_message_show(_("Enlightenment cannot initialize the System Command system.\n"));
       _e_main_shutdown(-1);
    }
   _e_main_shutdown_push(e_sys_shutdown);

   e_init_status_set(_("Setup Actions"));
   TS("actions");
   /* init actions system */
   if (!e_actions_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its actions system."));
	_e_main_shutdown(-1);
     }

   e_init_status_set(_("Setup Execution System"));
   TS("exec");
   /* init app system */
   if (!e_exec_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its exec system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_exec_shutdown);

   TS("container freeze");
   e_container_all_freeze();

   e_init_status_set(_("Setup FM"));
   TS("fm2");
   /* init the enlightenment file manager */
   if (!e_fm2_init())
    {
       e_error_message_show(_("Enlightenment cannot initialize the File manager.\n"));
       _e_main_shutdown(-1);
    }
   _e_main_shutdown_push(e_fm2_shutdown);

   /*
   TS("fwin");
   if (!e_fwin_init())
    {
       e_error_message_show(_("Enlightenment cannot initialize the File manager.\n"));
       _e_main_shutdown(-1);
    }
   _e_main_shutdown_push(e_fwin_shutdown);
    */

   e_init_status_set(_("Setup Message System"));
   TS("msg");
   /* setup generic msg handling etc */
   if (!e_msg_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its msg system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_msg_shutdown);

   e_init_status_set(_("Setup DND"));
   TS("dnd");
   /* setup dnd */
   if (!e_dnd_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its dnd system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_dnd_shutdown);

   e_init_status_set(_("Setup Grab Input Handling"));
   TS("grabinput");
   /* setup input grabbing co-operation system */
   if (!e_grabinput_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its input grab handling system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_grabinput_shutdown);

   e_init_status_set(_("Setup Modules"));
   TS("modules");
   /* setup module loading etc */
   if (!e_module_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its module system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_module_shutdown);

   e_init_status_set(_("Setup Remembers"));
   TS("remember");
   /* do remember stuff */
   if (!e_remember_init(after_restart ? E_STARTUP_RESTART: E_STARTUP_START))
     {
       e_error_message_show(_("Enlightenment cannot setup remember settings."));
       _e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_remember_shutdown);

   e_init_status_set(_("Setup Color Classes"));
   TS("colorclasses");
   /* setup color_class */
   if (!e_color_class_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its color class system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_color_class_shutdown);

   e_init_status_set(_("Setup Gadcon"));
   TS("gadcon");
   /* setup gadcon */
   if (!e_gadcon_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its gadget control system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_gadcon_shutdown);

   e_init_status_set(_("Setup ACPI"));
   TS("acpi");
   /* acpi init will never fail. It always returns one even if acpid 
    * is not running, so no need to trap the return */
   e_acpi_init();
   _e_main_shutdown_push(e_acpi_shutdown);

   e_init_status_set(_("Setup DPMS"));
   TS("dpms");     
   /* setup dpms */
   if (!e_dpms_init())
     {
	e_error_message_show(_("Enlightenment cannot configure the DPMS settings."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_dpms_shutdown);

   e_init_status_set(_("Setup Powersave modes"));
   TS("powersave");
   if (!e_powersave_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its powersave modes."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_powersave_shutdown);

   e_init_status_set(_("Setup Wallpaper"));
   TS("bg");
   /* init desktop background system */
   if (!e_bg_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its desktop background system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_bg_shutdown);

   e_init_status_set(_("Setup Mouse"));
   TS("mouse");     
   /* setup mouse accel */
   if (!e_mouse_update())
     {
	e_error_message_show(_("Enlightenment cannot configure the mouse settings."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_actions_shutdown);

   e_init_status_set(_("Setup Bindings"));
   TS("bindings");
   /* init bindings system */
   if (!e_bindings_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its bindings system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_bindings_shutdown);

   e_init_status_set(_("Setup Shelves"));
   TS("shelves");
   /* setup shelves */
   if (!e_shelf_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its shelf system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_shelf_shutdown);

   e_init_status_set(_("Setup Thumbnailer"));
   TS("thumb init");
   /* init the enlightenment thumbnailing system */
   if (!e_thumb_init())
    {
       e_error_message_show(_("Enlightenment cannot initialize the Thumbnailing system.\n"));
       _e_main_shutdown(-1);
    }
   _e_main_shutdown_push(e_thumb_shutdown);

   e_init_status_set(_("Setup File Ordering"));
   TS("order");
   if (!e_order_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its order file system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_order_shutdown);

   TS("add idle enterers");
   /* add in a handler that just before we go idle we flush x - will happen after ecore_evas's idle rendering as it's after ecore_evas_init() */
   _e_main_idle_enterer_flusher = 
     ecore_idle_enterer_add(_e_main_cb_x_flusher, NULL);

   e_managers_keys_grab();

   /* ecore_x_ungrab(); */

   /* load modules */
   e_init_status_set(_("Load Modules"));
   TS("load modules");
   if (safe_mode)
     {
	E_Module *m; 
	char *crashmodule;

	crashmodule = getenv("E_MODULE_LOAD");
	if (crashmodule) m = e_module_new(crashmodule);

	if ((crashmodule) && (m))
	  {
	     e_module_disable(m);
	     e_object_del(E_OBJECT(m));

	     e_int_config_modules(e_container_current_get(e_manager_current_get()), NULL);
	     e_error_message_show
	       (_("Enlightenment crashed early on start and has<br>"
		  "been restarted. There was an error loading<br>"
		  "module named: %s. This module has been disabled<br>"
		  "and will not be loaded."), crashmodule);
	     e_util_dialog_show
	       (_("Enlightenment crashed early on start and has been restarted"),
		   _("Enlightenment crashed early on start and has been restarted.<br>"
		     "There was an error loading module named: %s<br><br>"
		     "This module has been disabled and will not be loaded."), crashmodule);
	     e_module_all_load();
	  }
	else
	  {	
	     e_int_config_modules(e_container_current_get(e_manager_current_get()), NULL);
	     e_error_message_show
	       (_("Enlightenment crashed early on start and has<br>"
		  "been restarted. All modules have been disabled<br>"
		  "and will not be loaded to help remove any problem<br>"
		  "modules from your configuration. The module<br>"
		  "configuration dialog should let you select your<br>"
		  "modules again."));
	     e_util_dialog_show
	       (_("Enlightenment crashed early on start and has been restarted"),
		   _("Enlightenment crashed early on start and has been restarted.<br>"
		     "All modules have been disabled and will not be loaded to help<br>"
		     "remove any problem modules from your configuration.<br><br>"
		     "The module configuration dialog should let you select your<br>"
		     "modules again."));
	  }
     }
   else
     e_module_all_load();

   TS("init properites");
   if (!nostartup)
     {
	if (after_restart) e_startup(E_STARTUP_RESTART);
	else e_startup(E_STARTUP_START);
     }

   if (!((!e_config->show_splash) || (after_restart)))
     {
	ecore_timer_add(16.0, _e_main_cb_startup_fake_end, NULL);
	if (locked) e_desklock_show();
     }

   e_container_all_thaw();

   TS("test code");
   /* run any testing code now we are set up */
   e_test();

   e_init_status_set(_("Configure Shelves"));
   TS("shelf config update");
   e_shelf_config_update();

   TS("manage all windows");
   _e_main_manage_all();

   /* an idle enterer to be called after all others */
   _e_main_idle_enterer_after = 
     ecore_idle_enterer_add(_e_main_cb_idler_after, NULL);

   e_init_status_set(_("Almost Done"));
   TS("MAIN LOOP AT LAST");
   /* no longer starting up */
   starting = 0;
   /* start our main loop */

#ifdef DBG_EINA_STRINGSHARE
   // enable to debug eina stringshare usage
   ecore_timer_add(5.0, stdbg, NULL);
#endif

   inloop = EINA_TRUE;
   if (!setjmp(x_fatal_buf))
      ecore_main_loop_begin();
   else
      printf("FATAL: X died. Connection gone. abbreviated shutdown\n");
   inloop = EINA_FALSE;
   
   stopping = 1;

   if (!x_fatal) e_canvas_idle_flush();

   /* ask all modules to save their config and then shutdown */
   /* NB: no need to do this as config shutdown will flush any saves */
   /*     and all changed config was already saved before */
   e_config_save_flush();

   /* Store current selected desktops */
   _e_main_desk_save();

   e_remember_internal_save();
   
   /* unroll our stack of shutdown functions with exit code of 0 */
   _e_main_shutdown(0);

   /* if we were flagged to restart, then  restart. */
   if (restart)
     {
	e_util_env_set("E_RESTART_OK", "1");
	ecore_app_restart();
     }

   e_prefix_shutdown();

   /* just return 0 to keep the compiler quiet */
   return 0;
}

/* FIXME: make safe to delete within a callback */
EAPI E_Before_Idler *
e_main_idler_before_add(int (*func) (void *data), void *data, int once)
{
   E_Before_Idler *eb;

   eb = calloc(1, sizeof(E_Before_Idler));
   eb->func = func;
   eb->data = data;
   eb->once = once;
   _e_main_idler_before_list = eina_list_append(_e_main_idler_before_list, eb);
   return eb;
}

EAPI void
e_main_idler_before_del(E_Before_Idler *eb)
{
   eb->delete_me = 1;
}

/* local subsystem functions */
static void
_e_main_shutdown_push(int (*func) (void))
{
   _e_main_level++;
   if (_e_main_level > MAX_LEVEL)
     {
	_e_main_level--;
	e_error_message_show("WARNING: too many init levels. MAX = %i", MAX_LEVEL);
	return;
     }
   _e_main_shutdown_func[_e_main_level - 1] = func;
}

static void
_e_main_shutdown(int errorcode)
{
   int i;

   printf("E17: Begin shutdown procedure!\n");
   if (_e_main_idle_enterer_before)
     {
	ecore_idle_enterer_del(_e_main_idle_enterer_before);
	_e_main_idle_enterer_before = NULL;
     }
   if (_e_main_idle_enterer_after)
     {
	ecore_idle_enterer_del(_e_main_idle_enterer_after);
	_e_main_idle_enterer_after = NULL;
     }
   if (_e_main_idle_enterer_flusher)
     {
	ecore_idle_enterer_del(_e_main_idle_enterer_flusher);
	_e_main_idle_enterer_flusher = NULL;
     }
   for (i = _e_main_level - 1; i >= 0; i--)
     (*_e_main_shutdown_func[i])();
   if (errorcode < 0) exit(errorcode); 
}

static int
_e_main_x_shutdown(void)
{
   if (x_fatal) return 1;
/* ecore_x_ungrab(); */
   ecore_x_focus_reset();
   ecore_x_events_allow_all();
   ecore_x_shutdown();
   return 1;
}

static int
_e_main_dirs_init(void)
{
   const char *base;
   const char *dirs[] = {
     "images",
     "fonts",
     "themes",
     "icons",
     "backgrounds",
     "applications",
     "applications/menu",
     "applications/menu/favorite",
     "applications/menu/all",
     "applications/bar",
     "applications/bar/default",
     "applications/startup",
     "applications/restart",
     "applications/trash",
     "modules",
     "config",
     "locale",
     "input_methods",
     NULL
   };

   base = e_user_dir_get();
   if (ecore_file_mksubdirs(base, dirs) != sizeof(dirs)/sizeof(dirs[0]) - 1)
     {
	e_error_message_show
	  ("Could not create one of the required subdirectories of '%s'", base);
	return 0;
     }

   return 1;
}

static int
_e_main_dirs_shutdown(void)
{
   return 1;
}

static int
_e_main_screens_init(void)
{
   Ecore_X_Window *roots;
   int num, i;

   TS("screens: atoms");
   if (!e_atoms_init()) return 0;
   TS("screens: manager");
   if (!e_manager_init()) return 0;
   TS("screens: container");
   if (!e_container_init()) return 0;
   TS("screens: zone");
   if (!e_zone_init()) return 0;
   TS("screens: desk");
   if (!e_desk_init()) return 0;
   TS("screens: menu");
   if (!e_menu_init()) return 0;
   TS("screens: exehist");
   if (!e_exehist_init()) return 0;

   TS("screens: get roots");
   num = 0;
   roots = ecore_x_window_root_list(&num);
   if ((!roots) || (num <= 0))
     {
	e_error_message_show("X reports there are no root windows and %i screens!\n", 
			     num);
	return 0;
     }
   TS("screens: focus");
   if (!e_focus_init()) return 0;
   TS("screens: border");
   if (!e_border_init()) return 0;
   TS("screens: win");
   if (!e_win_init()) return 0;
   TS("screens: manage roots");
   for (i = 0; i < num; i++)
     {
	E_Manager *man;
	E_Container *con;

	man = e_manager_new(roots[i], i);
	if (man)
	  e_manager_show(man);
	else
	  {
	     e_error_message_show("Cannot create manager object for screen %i\n", 
				  i);
	     free(roots);
	     return 0;
	  }
	con = e_container_new(man);
	if (con)
	  {
	     e_container_show(con);
	     e_grabinput_focus(con->bg_win, E_FOCUS_METHOD_PASSIVE);
	     e_hints_manager_init(man);
	     _e_main_desk_restore(man, con);
//	     e_manager_manage_windows(man);
	  }
	else
	  {
	     e_error_message_show("Cannot create desktop object for manager on screen %i\n", 
				  i);
	     free(roots);
	     return 0;
	  }
     }
   TS("screens: sync");

   free(roots);
   ecore_x_sync();
   return 1;
}

static int
_e_main_screens_shutdown(void)
{
   e_win_shutdown();
   e_border_shutdown();
   e_focus_shutdown();
   e_exehist_shutdown();
   e_menu_shutdown();
// ecore_evas closes evas - deletes objs - deletes fm widgets which tries to
// ipc to slave to stop monitoring - but ipc has been shut down. dont shut
// down.   
//   e_desk_shutdown();
//   e_zone_shutdown();
//   e_container_shutdown();
//   e_manager_shutdown();
   e_atoms_shutdown();
   return 1;
}

static void
_e_main_manage_all(void)
{
   Eina_List *l;
   E_Manager *man;

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     e_manager_manage_windows(man);
}

static int
_e_main_path_init(void)
{
   char buf[PATH_MAX];

   /* setup data paths */
   path_data = e_path_new();
   if (!path_data)
     {
	e_error_message_show("Cannot allocate path for path_data\n");
	return 0;
     }
   e_prefix_data_concat_static(buf, "data");
   e_path_default_path_append(path_data, buf);
   e_path_user_path_set(path_data, &(e_config->path_append_data));

   /* setup image paths */
   path_images = e_path_new();
   if (!path_images)
     {
	e_error_message_show("Cannot allocate path for path_images\n");
	return 0;
     }
   e_user_dir_concat_static(buf, "/images");
   e_path_default_path_append(path_images, buf);
   e_prefix_data_concat_static(buf, "data/images");
   e_path_default_path_append(path_images, buf);
   e_path_user_path_set(path_images, &(e_config->path_append_images));

   /* setup font paths */
   path_fonts = e_path_new();
   if (!path_fonts)
     {
	e_error_message_show("Cannot allocate path for path_fonts\n");
	return 0;
     }
   e_user_dir_concat_static(buf, "/fonts");
   e_path_default_path_append(path_fonts, buf);
   e_prefix_data_concat_static(buf, "data/fonts");
   e_path_default_path_append(path_fonts, buf);
   e_path_user_path_set(path_fonts, &(e_config->path_append_fonts));

   /* setup theme paths */
   path_themes = e_path_new();
   if (!path_themes)
     {
	e_error_message_show("Cannot allocate path for path_themes\n");
	return 0;
     }
   e_user_dir_concat_static(buf, "/themes");
   e_path_default_path_append(path_themes, buf);
   e_prefix_data_concat_static(buf, "data/themes");
   e_path_default_path_append(path_themes, buf);
   e_path_user_path_set(path_themes, &(e_config->path_append_themes));

   /* setup icon paths */
   path_icons = e_path_new();
   if (!path_icons)
     {
	e_error_message_show("Cannot allocate path for path_icons\n");
	return 0;
     }
   e_user_dir_concat_static(buf, "/icons");
   e_path_default_path_append(path_icons, buf);
   e_prefix_data_concat_static(buf, "data/icons");
   e_path_default_path_append(path_icons, buf);
   e_path_user_path_set(path_icons, &(e_config->path_append_icons));

   /* setup module paths */
   path_modules = e_path_new();
   if (!path_modules) 
     {
	e_error_message_show("Cannot allocate path for path_modules\n");
	return 0;
     }
   e_user_dir_concat_static(buf, "/modules");
   e_path_default_path_append(path_modules, buf);
   snprintf(buf, sizeof(buf), "%s/enlightenment/modules", e_prefix_lib_get());
   e_path_default_path_append(path_modules, buf);
   /* FIXME: eventually this has to go - moduels should have installers that
    * add appropriate install paths (if not installed to user homedir) to
    * e's module search dirs
    */
   snprintf(buf, sizeof(buf), "%s/enlightenment/modules_extra", e_prefix_lib_get());
   e_path_default_path_append(path_modules, buf);
   e_path_user_path_set(path_modules, &(e_config->path_append_modules));    

   /* setup background paths */
   path_backgrounds = e_path_new();
   if (!path_backgrounds) 
     {
	e_error_message_show("Cannot allocate path for path_backgrounds\n");
	return 0;
     }
   e_user_dir_concat_static(buf, "/backgrounds");
   e_path_default_path_append(path_backgrounds, buf);
   e_prefix_data_concat_static(buf, "data/backgrounds");
   e_path_default_path_append(path_backgrounds, buf);
   e_path_user_path_set(path_backgrounds, &(e_config->path_append_backgrounds));

   path_messages = e_path_new();
   if (!path_messages) 
     {
	e_error_message_show("Cannot allocate path for path_messages\n");
	return 0;
     }
   e_user_dir_concat_static(buf, "/locale");
   e_path_default_path_append(path_messages, buf);   
   e_path_default_path_append(path_messages, e_prefix_locale_get());
   e_path_user_path_set(path_messages, &(e_config->path_append_messages));

   return 1;
}

static int
_e_main_path_shutdown(void)
{
   if (path_data)
     {
	e_object_del(E_OBJECT(path_data));
	path_data = NULL;
     }
   if (path_images)
     {
	e_object_del(E_OBJECT(path_images));
	path_images = NULL;
     }
   if (path_fonts)
     {
	e_object_del(E_OBJECT(path_fonts));
	path_fonts = NULL;
     }
   if (path_themes)
     {
	e_object_del(E_OBJECT(path_themes));
	path_themes = NULL;
     }
   if (path_icons)
     {
	e_object_del(E_OBJECT(path_icons));
	path_icons = NULL;
     }
   if (path_modules)
     {
        e_object_del(E_OBJECT(path_modules));
        path_modules = NULL;
     }
   if (path_backgrounds)
     {
	e_object_del(E_OBJECT(path_backgrounds));
        path_backgrounds = NULL;
     }
   if (path_messages)
     {
	e_object_del(E_OBJECT(path_messages));
	path_messages = NULL;			          
     }
   return 1;
}

static void
_e_main_cb_x_fatal(void *data __UNUSED__)
{
   e_error_message_show("Lost X connection.");
   ecore_main_loop_quit();
   // x is foobared - best to get the hell out of the mainloop
   if (!x_fatal)
     {
        x_fatal = EINA_TRUE;
        if (inloop) longjmp(x_fatal_buf, -99);
     }
}

static Eina_Bool
_e_main_cb_signal_exit(void *data __UNUSED__, int ev_type __UNUSED__, void *ev __UNUSED__)
{
   /* called on ctrl-c, kill (pid) (also SIGINT, SIGTERM and SIGQIT) */
   e_sys_action_do(E_SYS_EXIT, NULL);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_main_cb_signal_hup(void *data __UNUSED__, int ev_type __UNUSED__, void *ev __UNUSED__)
{
   e_sys_action_do(E_SYS_RESTART, NULL);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_main_cb_signal_user(void *data __UNUSED__, int ev_type __UNUSED__, void *ev)
{
   Ecore_Event_Signal_User *e = ev;
   
   if (e->number == 1)
     {
        E_Action *a = e_action_find("configuration");
        if ((a) && (a->func.go)) a->func.go(NULL, NULL);
     }
   else if (e->number == 2)
     {
        // comp module has its own handler for this for enabling/disabling fps debug
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_main_cb_x_flusher(void *data __UNUSED__)
{
   eet_clearcache();
   ecore_x_flush();
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_main_cb_idler_before(void *data __UNUSED__)
{
   Eina_List *l, *pl;
   E_Before_Idler *eb;

   e_menu_idler_before();
   e_focus_idler_before();
   e_border_idler_before();
   e_popup_idler_before();
   e_drag_idler_before();
   e_pointer_idler_before();
   EINA_LIST_FOREACH(_e_main_idler_before_list, l, eb)
     {
	if (!eb->delete_me)
	  {
	     if (!eb->func(eb->data)) eb->delete_me = 1;
	  }
     }
   EINA_LIST_FOREACH_SAFE(_e_main_idler_before_list, pl, l, eb)
     {
	if ((eb->once) || (eb->delete_me))
	  {
	     _e_main_idler_before_list =
	       eina_list_remove_list(_e_main_idler_before_list, pl);
	     free(eb);
	  }
     }
   _e_cacheburst--;
/* eet_cacheburst(_e_cacheburst); */
   edje_thaw();
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_main_cb_idler_after(void *data __UNUSED__)
{
   edje_freeze();
   _e_cacheburst++;
/* eet_cacheburst(_e_cacheburst); */
     {
	static int first_idle = 1;

	if (first_idle)
	  {
	     TS("SLEEP");
	     first_idle = 0;
	     e_precache_end = 1;
	  }
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_main_cb_eet_cacheburst_end(void *data __UNUSED__)
{
   _e_cacheburst--;
/* eet_cacheburst(_e_cacheburst); */
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_main_cb_startup_fake_end(void *data __UNUSED__)
{
   e_init_hide();
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_main_desk_save(void)
{
   Eina_List *ml;
   E_Manager *man;
   char env[1024], name[1024];

   EINA_LIST_FOREACH(e_manager_list(), ml, man)
     {
	Eina_List *cl;
	E_Container *con;

	EINA_LIST_FOREACH(man->containers, cl, con)
	  {
	     Eina_List *zl;
	     E_Zone *zone;

	     EINA_LIST_FOREACH(con->zones, zl, zone)
	       {
		  snprintf(name, sizeof(name), "DESK_%d_%d_%d", man->num, con->num, zone->num);
		  snprintf(env, sizeof(env), "%d,%d", zone->desk_x_current, zone->desk_y_current);
		  e_util_env_set(name, env);
	       }
	  }
     }
}

static void
_e_main_desk_restore(E_Manager *man, E_Container *con)
{
   Eina_List *zl;
   E_Zone *zone;
   char *env;
   char name[1024];

   EINA_LIST_FOREACH(con->zones, zl, zone)
     {
	E_Desk *desk;
	int desk_x, desk_y;

	snprintf(name, sizeof(name), "DESK_%d_%d_%d", man->num, con->num, zone->num);
	env = getenv(name);
	if (!env) continue;
	if (!sscanf(env, "%d,%d", &desk_x, &desk_y)) continue;
	desk = e_desk_at_xy_get(zone, desk_x, desk_y);
	if (!desk) continue;
	e_desk_show(desk);
     }
}

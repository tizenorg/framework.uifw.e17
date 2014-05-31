/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */


#ifndef EFL_CONFIG_H__
#define EFL_CONFIG_H__


/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* enable Files menu item */
#define ENABLE_FILES 1

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
#define ENABLE_NLS 1

/* "This define can be used to wrap internal E stuff */
#define E_INTERNAL 1

/* Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
#define HAVE_ALLOCA_H 1

/* Define if the ALSA output plugin should be built */
/* #undef HAVE_ALSA */

/* Package BATTERY ( ecore >= 1.2.0 ecore-file >= 1.2.0 ecore-con >= 1.2.0
   eina >= 1.2.0 ) found. */
/* #undef HAVE_BATTERY */

/* Define to 1 if you have the <CFBase.h> header file. */
/* #undef HAVE_CFBASE_H */

/* Define to 1 if you have the MacOS X function CFLocaleCopyCurrent in the
   CoreFoundation framework. */
/* #undef HAVE_CFLOCALECOPYCURRENT */

/* Define to 1 if you have the MacOS X function CFPreferencesCopyAppValue in
   the CoreFoundation framework. */
/* #undef HAVE_CFPREFERENCESCOPYAPPVALUE */

/* Define if the GNU dcgettext() function is already present or preinstalled.
   */
#define HAVE_DCGETTEXT 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Package EBLUEZ ( edbus >= 1.2.0 ebluez >= 1.2.0 ) found. */
/* #undef HAVE_EBLUEZ */

/* Package ECONNMAN ( edbus >= 1.2.0 ) found. */
/* #undef HAVE_ECONNMAN */

/* Package ECORE_IMF ( ecore-imf >= 1.2.0 ecore-imf-evas >= 1.2.0 ) found. */
#define HAVE_ECORE_IMF 1

/* enable udev support */
/* #undef HAVE_EEZE */

/* enable eeze mounting */
/* #undef HAVE_EEZE_MOUNT */

/* "Have Elementary support" */
#define HAVE_ELEMENTARY 1

/* Package ENOTIFY ( edbus >= 1.2.0 enotify >= 1.2.0 ) found. */
#define HAVE_ENOTIFY 1

/* Package EOFONO ( edbus >= 1.2.0 eofono >= 1.2.0 ) found. */
/* #undef HAVE_EOFONO */

/* Package EPHYSICS ( ephysics ) found. */
/* #undef HAVE_EPHYSICS */

/* Package EXCHANGE (exchange) found. */
/* #undef HAVE_EXCHANGE */

/* Define to 1 if you have the <execinfo.h> header file. */
#define HAVE_EXECINFO_H 1

/* Define to 1 if you have the `fnmatch' function. */
#define HAVE_FNMATCH 1

/* Define to 1 if you have the <fnmatch.h> header file. */
#define HAVE_FNMATCH_H 1

/* Define if the GNU gettext() function is already present or preinstalled. */
#define HAVE_GETTEXT 1

/* enable HAL support */
#define HAVE_HAL 1

/* enable HAL mounting */
/* #undef HAVE_HAL_MOUNT */

/* Define if you have the iconv() function and it works. */
/* #undef HAVE_ICONV */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* PAM Authentication Support */
/* #undef HAVE_PAM */

/* Define to 1 if you have the <security/pam_appl.h> header file. */
/* #undef HAVE_SECURITY_PAM_APPL_H */

/* Define to 1 if you have the `setenv' function. */
#define HAVE_SETENV 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Package TEMPERATURE ( ecore >= 1.2.0 ecore-file >= 1.2.0 eina >= 1.2.0 )
   found. */
/* #undef HAVE_TEMPERATURE */

/* enable Udisks mounting */
#define HAVE_UDISKS_MOUNT 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `unsetenv' function. */
#define HAVE_UNSETENV 1

/* enable wayland client support */
/* #undef HAVE_WAYLAND_CLIENTS */

/* Define to 1 if your compiler has __attribute__ */
#define HAVE___ATTRIBUTE__ 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* "Module architecture" */
#define MODULE_ARCH "linux-gnueabi-armv7l-ver-pre-svn-08"

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
#define PACKAGE "enlightenment"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "enlightenment-devel@lists.sourceforge.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "enlightenment"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "enlightenment 0.16.999.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "enlightenment"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.16.999.0"

/* default value since PATH_MAX is not defined */
/* #undef PATH_MAX */

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at runtime.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Use module access */
/* #undef USE_MODULE_ACCESS */

/* Use module backlight */
/* #undef USE_MODULE_BACKLIGHT */

/* Use module battery */
/* #undef USE_MODULE_BATTERY */

/* Use module bluez */
/* #undef USE_MODULE_BLUEZ */

/* Use module clock */
#define USE_MODULE_CLOCK 1

/* Use module comp */
/* #undef USE_MODULE_COMP */

/* Use module conf */
/* #undef USE_MODULE_CONF */

/* Use module conf-applications */
#define USE_MODULE_CONF_APPLICATIONS 1

/* Use module conf-dialogs */
/* #undef USE_MODULE_CONF_DIALOGS */

/* Use module conf-display */
/* #undef USE_MODULE_CONF_DISPLAY */

/* Use module conf-edgebindings */
/* #undef USE_MODULE_CONF_EDGEBINDINGS */

/* Use module conf-interaction */
/* #undef USE_MODULE_CONF_INTERACTION */

/* Use module conf-intl */
/* #undef USE_MODULE_CONF_INTL */

/* Use module conf-keybindings */
/* #undef USE_MODULE_CONF_KEYBINDINGS */

/* Use module conf-menus */
/* #undef USE_MODULE_CONF_MENUS */

/* Use module conf-paths */
/* #undef USE_MODULE_CONF_PATHS */

/* Use module conf-performance */
/* #undef USE_MODULE_CONF_PERFORMANCE */

/* Use module conf-randr */
/* #undef USE_MODULE_CONF_RANDR */

/* Use module conf-shelves */
/* #undef USE_MODULE_CONF_SHELVES */

/* Use module conf-theme */
#define USE_MODULE_CONF_THEME 1

/* Use module conf-wallpaper2 */
/* #undef USE_MODULE_CONF_WALLPAPER2 */

/* Use module conf-window-manipulation */
/* #undef USE_MODULE_CONF_WINDOW_MANIPULATION */

/* Use module conf-window-remembers */
/* #undef USE_MODULE_CONF_WINDOW_REMEMBERS */

/* Use module connman */
/* #undef USE_MODULE_CONNMAN */

/* Use module cpufreq */
/* #undef USE_MODULE_CPUFREQ */

/* Use module dropshadow */
/* #undef USE_MODULE_DROPSHADOW */

/* Use module everything */
/* #undef USE_MODULE_EVERYTHING */

/* Use module fileman */
/* #undef USE_MODULE_FILEMAN */

/* Use module fileman-opinfo */
/* #undef USE_MODULE_FILEMAN_OPINFO */

/* Use module gadman */
#define USE_MODULE_GADMAN 1

/* Use module ibar */
#define USE_MODULE_IBAR 1

/* Use module ibox */
#define USE_MODULE_IBOX 1

/* Use module illume2 */
/* #undef USE_MODULE_ILLUME2 */

/* Use module mixer */
/* #undef USE_MODULE_MIXER */

/* Use module msgbus */
/* #undef USE_MODULE_MSGBUS */

/* Use module notification */
/* #undef USE_MODULE_NOTIFICATION */

/* Use module ofono */
/* #undef USE_MODULE_OFONO */

/* Use module pager */
/* #undef USE_MODULE_PAGER */

/* Use module physics */
/* #undef USE_MODULE_PHYSICS */

/* Use module quickaccess */
/* #undef USE_MODULE_QUICKACCESS */

/* Use module shot */
/* #undef USE_MODULE_SHOT */

/* Use module start */
/* #undef USE_MODULE_START */

/* Use module syscon */
/* #undef USE_MODULE_SYSCON */

/* Use module systray */
/* #undef USE_MODULE_SYSTRAY */

/* Use module tasks */
/* #undef USE_MODULE_TASKS */

/* Use module temperature */
/* #undef USE_MODULE_TEMPERATURE */

/* Use module tiling */
/* #undef USE_MODULE_TILING */

/* Use module winlist */
/* #undef USE_MODULE_WINLIST */

/* Use module wizard */
/* #undef USE_MODULE_WIZARD */

/* Use module xkbswitch */
/* #undef USE_MODULE_XKBSWITCH */

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif


/* Version number of package */
#define VERSION "0.16.999.0"

/* Major version */
#define VMAJ 0

/* Micro version */
#define VMIC 999

/* Minor version */
#define VMIN 16

/* Revison */
#define VREV 0

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define to 1 if the X Window System is missing or not being used. */
/* #undef X_DISPLAY_MISSING */

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */

/* Macro declaring a function argument to be unused */
#define __UNUSED__ __attribute__((unused))

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */


#endif /* EFL_CONFIG_H__ */


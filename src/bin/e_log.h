#ifdef E_TYPEDEFS

#else
#ifndef E_LOG_H
#define E_LOG_H


#ifdef E_LOGGING
#undef DBG
#undef INF
#undef WRN
#undef ERR
#undef CRI
#define DBG(...)            EINA_LOG_DOM_DBG(e_log_dom, __VA_ARGS__)
#define INF(...)            EINA_LOG_DOM_INFO(e_log_dom, __VA_ARGS__)
#define WRN(...)            EINA_LOG_DOM_WARN(e_log_dom, __VA_ARGS__)
#define ERR(...)            EINA_LOG_DOM_ERR(e_log_dom, __VA_ARGS__)
#define CRI(...)            EINA_LOG_DOM_CRIT(e_log_dom, __VA_ARGS__)

EINTERN extern int e_log_dom;

EINTERN int e_log_init(void);
EINTERN int e_log_shutdown(void);
#else
#undef DBG
#undef INF
#undef WRN
#undef ERR
#undef CRI
#define DBG(...)            printf(__VA_ARGS__); putc('\n', stdout)
#define INF(...)            printf(__VA_ARGS__); putc('\n', stdout)
#define WRN(...)            printf(__VA_ARGS__); putc('\n', stdout)
#define ERR(...)            printf(__VA_ARGS__); putc('\n', stdout)
#define CRI(...)            printf(__VA_ARGS__); putc('\n', stdout)
#endif

#ifdef _F_E_LOGBUF_
#define ELBT_NONE   0x00000000
#define ELBT_DFT    0x00000001
#define ELBT_MNG    0x00000002
#define ELBT_BD     0x00000004
#define ELBT_ROT    0x00000008
#define ELBT_ILLUME 0x00000010
#define ELBT_COMP   0x00000020
#define ELBT_MOVE   0x00000040
#define ELBT_ALL    0xFFFFFFFF

extern EAPI unsigned int e_logbuf_type;

EINTERN int  e_logbuf_init(void);
EINTERN int  e_logbuf_shutdown(void);
EAPI    void e_logbuf_add(unsigned int type, const char *func, int line, const char *str, unsigned int id);
EAPI    void e_logbuf_fmt_add(unsigned int type, unsigned int blank, const char *func, int line, unsigned int id, const char *fmt, ...);

#define ELB(t, s, i)           do { if ((e_logbuf_type) & (t)) { e_logbuf_add((t), __FUNCTION__, __LINE__, (s), (i));             } } while (0);
#define ELBF(t, b, i, f, x...) do { if ((e_logbuf_type) & (t)) { e_logbuf_fmt_add((t), (b), __FUNCTION__, __LINE__, (i), f, ##x); } } while (0);
#else
#define ELB(...)            ;
#define ELBF(...)           ;
#endif

#endif
#endif

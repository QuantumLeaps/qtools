/* The configuration-specific name of the link-time configuration check
   (QP_VERSION + encoded settings, which include Q_SPY) differs between the
   builds by design; all such names are compared as one name, so the inline
   initialization calling it and the kernel function defining it are
   identical in both builds. */
/* EXPECT: PASS */
/* Self-contained mock of the QP/Spy pattern (no QP headers needed) */
typedef unsigned char uint8_t;
extern struct { uint8_t flags; } QS_priv_;
void QF_crit_entry_(void);
void QF_crit_exit_(void);
void QS_beginRec_(uint8_t rec);
void QS_u8_raw_(uint8_t d);
void QS_endRec_(void);
int  QS_fltCheck_(uint8_t rec);
#ifdef Q_SPY
#define QS_REC_(rec_, d_) do { QF_crit_entry_(); \
    if (QS_fltCheck_(rec_)) { QS_beginRec_(rec_); QS_u8_raw_(d_); QS_endRec_(); } \
    QF_crit_exit_(); } while (0)
#else
#define QS_REC_(rec_, d_) ((void)0)
#endif
void action(int x);
int  other(int x);
int  g_counter;
#ifdef Q_SPY
#define QF_INIT_CFGCHK_ QF_816U_a08t01p01e4q1m2s42x
#else
#define QF_INIT_CFGCHK_ QF_816U_a08t01p01e4q1m2s00
#endif
void QF_INIT_CFGCHK_(void);
static inline void QF_init(void) { QF_INIT_CFGCHK_(); }

void QF_INIT_CFGCHK_(void) {
    action(1);
    QS_REC_(1U, 0U);
}
void f(int x) {
    QF_init();
    action(x);
}

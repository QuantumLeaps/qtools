/* The configuration-specific names are compared as one name, but a call of
   such a name that exists only in the production build is still found (R1). */
/* EXPECT: FAIL */
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

void f(int x) {
#ifndef Q_SPY
    QF_INIT_CFGCHK_();
#endif
    action(x);
}

/* Production trace macros expand to dead 'if (0) {no-ops}' blocks and casted no-ops (QS_BEGIN_ID style). */
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
#define QS_BEGIN_X(rec_) if (QS_fltCheck_(rec_)) { QS_beginRec_(rec_);
#define QS_END_X()       QS_endRec_(); }
#define QS_ARG_(p_)      (p_)
#define QS_U8_X(d_)      QS_u8_raw_(d_)
#else
#define QS_BEGIN_X(rec_) if (0) { (void)0;
#define QS_END_X()       }
#define QS_ARG_(p_)      ((void)(p_))
#define QS_U8_X(d_)      ((void)0)
#endif
int f(int x, void const *sender) {
#ifdef Q_SPY
    QS_BEGIN_X(4U)
        QS_U8_X((uint8_t)(sender != (void const *)0));
    QS_END_X()
#else
    QS_ARG_((void const *)sender);
#endif
    QS_BEGIN_X(5U)
        QS_U8_X((uint8_t)x);
    QS_END_X()
    action(x);
    if (x > 1) {
        QS_REC_(6U, 0U);
    }
    return x;
}

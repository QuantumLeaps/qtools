/* Instrumentation is purely additive (trace records, QS-private data, test-only local). */
/* EXPECT: PASS */
/* Self-contained mock of the QP/Spy pattern (no QP headers needed) */
typedef unsigned char uint8_t;
struct QS_Attr { uint8_t flags; };
extern struct QS_Attr QS_priv_;
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

int f(int x) {
    int r = 0;
#ifdef Q_SPY
    int seen = 0;
    if ((QS_priv_.flags & 1U) == 0U) { QS_priv_.flags |= 1U; seen = 1; }
    if (seen) { QS_REC_(1U, 0U); }
#endif
    for (int i = 0; i < x; ++i) {
        QS_REC_(2U, (uint8_t)i);
        action(i);
        if (x > 3) { r += i; }
    }
    QS_REC_(3U, (uint8_t)r);
    return r;
}

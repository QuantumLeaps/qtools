/* Production code with a label of its own and an additive trace record
   (the dump lists the label as "void L = <<< error >>>;"; not an error). */
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

int f(int x) {
    int r = other(x);
    if (r < 0) {
        goto done;
    }
    QS_REC_(2U, (uint8_t)r);
    r = other(r);
done:
    action(r);
    return r;
}

// C++: class temporaries (TARGET_EXPR), templates, and operator calls in production code around a trace record
// EXPECT: PASS
// Self-contained mock of the SafeQP/C++ QP/Spy pattern (no QP headers needed)
#include <cstdint>
#include <array>
namespace QP {
using QCritStatus = std::uint32_t;
namespace QF {
    QCritStatus critEntry();
    void critExit(QCritStatus stat);
}
class QS {
public:
    struct Attr { std::uint8_t flags; };
    static Attr priv_;
    static bool fltCheck_(std::uint_fast8_t idx, std::uint_fast32_t bit,
                          std::uint_fast8_t qsId);
    static void beginRec_(std::uint_fast8_t rec);
    static void u8_raw_(std::uint8_t d);
    static void u32_raw_(std::uint32_t d);
    static void obj_raw_(void const * obj);
    static void endRec_();
    template<typename T_OUT, typename T_IN>
    static T_OUT force_cast(T_IN in) { return reinterpret_cast<T_OUT>(in); }
};
struct QSpyId {
    std::uint8_t m_prio;
    std::uint_fast8_t getPrio() const noexcept { return m_prio; }
};
} // namespace QP
#ifdef Q_SPY
#define QS_CRIT_STAT    QP::QCritStatus critStat_;
#define QS_CRIT_ENTRY() (critStat_ = QP::QF::critEntry())
#define QS_CRIT_EXIT()  (QP::QF::critExit(critStat_))
#define QS_BEGIN_PRE(rec_, qsId_) \
    if (QP::QS::fltCheck_(static_cast<std::uint32_t>(rec_) >> 5U, \
        1U << (static_cast<std::uint32_t>(rec_) & 0x1FU), (qsId_))) { \
        QP::QS::beginRec_(static_cast<std::uint_fast8_t>(rec_));
#define QS_END_PRE()    QP::QS::endRec_(); }
#define QS_U8_PRE(d_)   (QP::QS::u8_raw_(static_cast<std::uint8_t>(d_)))
#define QS_OBJ_PRE(o_)  (QP::QS::obj_raw_(o_))
#define QS_FUN_PRE(f_)  (QP::QS::u32_raw_(static_cast<std::uint32_t>( \
    reinterpret_cast<std::uintptr_t>(f_))))
#else
#define QS_CRIT_STAT
#define QS_CRIT_ENTRY() static_cast<void>(0)
#define QS_CRIT_EXIT()  static_cast<void>(0)
#define QS_BEGIN_PRE(rec_, qsId_) if (false) {
#define QS_END_PRE()    }
#define QS_U8_PRE(d_)   static_cast<void>(0)
#define QS_OBJ_PRE(o_)  static_cast<void>(0)
#define QS_FUN_PRE(f_)  static_cast<void>(0)
#endif
namespace QP {
struct Pair { std::uint8_t a; std::uint8_t b; };
inline Pair mk(std::uint8_t a) { return Pair{a, static_cast<std::uint8_t>(a >> 1)}; }
std::uint8_t f(std::array<Pair, 3> &arr, std::uint8_t a) {
    QS_CRIT_STAT
    arr[1] = mk(a);
    QS_CRIT_ENTRY();
    QS_BEGIN_PRE(5, 0U)
        QS_U8_PRE(arr[1].b);
    QS_END_PRE()
    QS_CRIT_EXIT();
    Pair const p = mk(arr[1].b);
    return static_cast<std::uint8_t>((p.a > p.b) ? (p.a >> 1) : p.b);
}
} // namespace QP

// C++: member functions with QS trace records, critical sections, and QS-only helpers (class QS, QSpyId)
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
class Hsm {
public:
    using Handler = std::uint8_t (*)(void *me, int sig);
    Handler m_state;
    std::array<Handler, 4> m_path;
    std::uint8_t m_prio;
    std::size_t dispatch(int sig);
};
std::size_t Hsm::dispatch(int sig) {
    QS_CRIT_STAT
    std::size_t ip = 0U;
    Handler s = m_state;
    QS_CRIT_ENTRY();
    QS_BEGIN_PRE(8, m_prio)
        QS_OBJ_PRE(this);
        QS_FUN_PRE((QS::force_cast<void (*)(), Handler>(s)));
        QS_U8_PRE(QSpyId{m_prio}.getPrio());
    QS_END_PRE()
    QS_CRIT_EXIT();
    for (; ip < m_path.size(); ++ip) {
        m_path[ip] = s;
        if ((sig >> 2) > static_cast<int>(ip)) {   // shift, not a wrapper
            break;
        }
    }
    return ip;
}
} // namespace QP

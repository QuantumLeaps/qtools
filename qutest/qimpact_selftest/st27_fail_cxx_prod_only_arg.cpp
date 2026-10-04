// C++: production passes a different argument (sender 0 vs sender, as in qf_ps)
// EXPECT: FAIL
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
class Active {
public:
    bool post_(int sig, void const * sender);
};
void multicast(Active *a, int sig, void const * const sender) {
#ifdef Q_SPY
    static_cast<void>(a->post_(sig, sender));
#else
    static_cast<void>(sender);
    static_cast<void>(a->post_(sig, nullptr));
#endif
}
} // namespace QP

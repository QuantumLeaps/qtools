enum UserSpyRecords {
    . . .
    PHILO_STAT = QS_USER, /* define a user QS record types */
    . . .
};

void displyPhilStat(uint8_t n, char const *stat) {
    . . .

    /* application-specific record */
    QS_BEGIN_ID(PHILO_STAT, AO_Philo[n]->prio);
        QS_U8(0, n);    /* Philosopher number */
        QS_STR(stat);   /* Philosopher status */
        QS_U8(QS_HEX_FMT,  0xABU);                /* test */
        QS_U16(QS_HEX_FMT, 0xDEADU);              /* test */
        QS_U32(QS_HEX_FMT, 0xDEADBEEFU);          /* test */
        QS_U64(QS_HEX_FMT, 0xDEADBEEF12345678LL); /* test */
    QS_END();
}

QSPY ouptut produced:

4294954639 PHILO_STAT 3 thinking 0xAB 0xDEAD 0xDEADBEEF 0xDEADBEEF12345678


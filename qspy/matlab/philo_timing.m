t=Q_STATE(:,2)>3 & ~isnan(Q_STATE(:,5));
o=Q_STATE(:,3) == l_philo_0_;
subplot(5,1,1); stairs(Q_STATE(o & t,1),Q_STATE(o & t,5),'r'); ylabel('philo[0]')
o=Q_STATE(:,3) == l_philo_1_;
subplot(5,1,2); stairs(Q_STATE(o & t,1),Q_STATE(o & t,5),'b'); ylabel('philo[1]')
o=Q_STATE(:,3) == l_philo_2_;
subplot(5,1,3); stairs(Q_STATE(o & t,1),Q_STATE(o & t,5),'k'); ylabel('philo[2]')
o=Q_STATE(:,3) == l_philo_3_;
subplot(5,1,4); stairs(Q_STATE(o & t,1),Q_STATE(o & t,5),'g'); ylabel('philo[3]')
o=Q_STATE(:,3) == l_philo_4_;
subplot(5,1,5); stairs(Q_STATE(o & t,1),Q_STATE(o & t,5),'m'); ylabel('philo[4]')
xlabel('time stamp'); zoom on

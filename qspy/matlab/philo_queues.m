o=Q_EQUEUE(:,2) == l_philo_0_;
subplot(5,1,1); stairs(Q_EQUEUE(o,1),cumsum(Q_EQUEUE(o,10)),'r'); ylabel('philo[0]')
axis([min(Q_EQUEUE(:,1)) max(Q_EQUEUE(:,1)) 0 4])
o=Q_EQUEUE(:,2) == l_philo_1_;
subplot(5,1,2); stairs(Q_EQUEUE(o,1),cumsum(Q_EQUEUE(o,10)),'b'); ylabel('philo[1]')
axis([min(Q_EQUEUE(:,1)) max(Q_EQUEUE(:,1)) 0 4])
o=Q_EQUEUE(:,2) == l_philo_2_;
subplot(5,1,3); stairs(Q_EQUEUE(o,1),cumsum(Q_EQUEUE(o,10)),'k'); ylabel('philo[2]')
axis([min(Q_EQUEUE(:,1)) max(Q_EQUEUE(:,1)) 0 4])
o=Q_EQUEUE(:,2) == l_philo_3_;
subplot(5,1,4); stairs(Q_EQUEUE(o,1),cumsum(Q_EQUEUE(o,10)),'g'); ylabel('philo[3]')
axis([min(Q_EQUEUE(:,1)) max(Q_EQUEUE(:,1)) 0 4])
o=Q_EQUEUE(:,2) == l_philo_4_;
subplot(5,1,5); stairs(Q_EQUEUE(o,1),cumsum(Q_EQUEUE(o,10)),'m'); ylabel('philo[4]')
axis([min(Q_EQUEUE(:,1)) max(Q_EQUEUE(:,1)) 0 4])
xlabel('time stamp'); zoom on

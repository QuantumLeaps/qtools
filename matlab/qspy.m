%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Product:  QSPY Matlab interface.
% Last Updated for Version: 4.5.01
% Date of the Last Update:  Jun 13, 2012
%
%                    Q u a n t u m     L e a P s
%                    ---------------------------
%                    innovating embedded systems
%
% Copyright (C) 2002-2012 Quantum Leaps, LLC. All rights reserved.
%
% This program is open source software: you can redistribute it and/or
% modify it under the terms of the GNU General Public License as published
% by the Free Software Foundation, either version 2 of the License, or
% (at your option) any later version.
%
% Alternatively, this program may be distributed and modified under the
% terms of Quantum Leaps commercial licenses, which expressly supersede
% the GNU General Public License and are specifically designed for
% licensees interested in retaining the proprietary status of their code.
%
% This program is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
% GNU General Public License for more details.
%
% You should have received a copy of the GNU General Public License
% along with this program. If not, see <http://www.gnu.org/licenses/>.
%
% Contact information:
% Quantum Leaps Web sites: http://www.quantum-leaps.com
%                          http://www.state-machine.com
% e-mail:                  info@quantum-leaps.com
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% the string Q_FILE must be defined
fid = fopen(Q_FILE, 'r');
if fid == -1
    error('file not found')
end

Q_STATE  = [];  % sate entry/exit, init, tran, internal tran, ignored
Q_EQUEUE = [];  % QEQueue
Q_MPOOL  = [];  % QMPool
Q_NEW    = [];  % new/gc
Q_ACTIVE = [];  % active add/remove, subscribe/unsubscribe
Q_PUB    = [];  % publish/publish attempt
Q_TIME   = [];  % time event arm/disarm/rearm, clock tick
Q_INT    = [];  % interrupt locking/unlocking
Q_ISR    = [];  % ISR entry/exit
Q_MUTEX  = [];  % QK mutex locking/unlocking
Q_SCHED  = [];  % QK scheduler activity

Q_TOT        = 0;   % total number of records processed

while feof(fid) == 0
    line = fgetl(fid);
    Q_TOT = Q_TOT+1;

    rec = sscanf(line, '%d', 1);    % extract the record number
    switch rec    % discriminate based on the record number

        % QEP records
        case  0   %  QS_QEP_STATE_EMPTY

        case  1   %  QS_QEP_STATE_ENTRY
            Q_STATE(size(Q_STATE,1)+1,:) = ...
                [NaN 1 sscanf(line, '%*u %u %u')' NaN 1];

        case  2   %  QS_QEP_STATE_EXIT
            Q_STATE(size(Q_STATE,1)+1,:) = ...
                [NaN 2 sscanf(line, '%*u %u %u')' NaN 1];

        case  3   %  QS_QEP_STATE_INIT
            Q_STATE(size(Q_STATE,1)+1,:) = ...
                [NaN 3 sscanf(line, '%*u %u %u %u')' 1];

        case  4   %  QS_QEP_INIT_TRAN
            tmp = sscanf(line, '%*u %u %u %u')';
            Q_STATE(size(Q_STATE,1)+1,:) = ...
                [tmp(1) 3 tmp(2) NaN tmp(3) 1];

        case  5   %  QS_QEP_INTERN_TRAN
            Q_STATE(size(Q_STATE,1)+1,:) = ...
                [sscanf(line, '%*u %u %u %u %u')' NaN 1];

        case  6   %  QS_QEP_TRAN
            Q_STATE(size(Q_STATE,1)+1,:) = ...
                [sscanf(line, '%*u %u %u %u %u %u')' 1];

        case  7   %  QS_QEP_IGNORED
            Q_STATE(size(Q_STATE,1)+1,:) = ...
                [sscanf(line, '%*u %u %u %u %u')' NaN 0];

        case  8   %  QS_QEP_DISPATCH
            Q_STATE(size(Q_STATE,1)+1,:) = ...
                [sscanf(line, '%*u %u %u %u %u')' NaN 0];

        case  9   %  QS_QEP_UNHANDLED
            Q_STATE(size(Q_STATE,1)+1,:) = ...
                [NaN sscanf(line, '%*u %u %u %u')' NaN 0];


        % QF records
        case 10   %  QS_QF_ACTIVE_ADD
            tmp = sscanf(line, '%*u %u %u %u')';
            Q_ACTIVE(size(Q_ACTIVE,1)+1,:) = [tmp(1) NaN tmp(2) tmp(3) 1];

        case 11   %  QS_QF_ACTIVE_REMOVE
            tmp = sscanf(line, '%*u %u %u %u')';
            Q_ACTIVE(size(Q_ACTIVE,1)+1,:) = [tmp(1) NaN tmp(2) tmp(3) -1];

        case 12   %  QS_QF_ACTIVE_SUBSCRIBE
            Q_ACTIVE(size(Q_ACTIVE,1)+1,:) = ...
                [sscanf(line, '%*u %u %u %u')' NaN 1];

        case 13   %  QS_QF_ACTIVE_UNSUBSCRIBE
            Q_ACTIVE(size(Q_ACTIVE,1)+1,:) = ...
                [sscanf(line, '%*u %u %u %u')' NaN -1];

        case 14   %  QS_QF_ACTIVE_POST_FIFO
            tmp = sscanf(line, '%*u %u %u %u %u %u %u %u %u')';
            Q_EQUEUE(size(Q_EQUEUE,1)+1,:) = ...
               [tmp(1) tmp(2) tmp(4) (tmp(7)+1) (tmp(8)+1) tmp(3) tmp(5) tmp(6) 0 1];

        case 15   %  QS_QF_ACTIVE_POST_LIFO
            tmp = sscanf(line, '%*u %u %u %u %u %u %u %u')';
            Q_EQUEUE(size(Q_EQUEUE,1)+1,:) = ...
               [tmp(1) tmp(3) tmp(3) (tmp(6)+1) (tmp(7)+1) tmp(2) tmp(4) tmp(5) 1 1];

        case 16   %  QS_QF_ACTIVE_GET
            tmp = sscanf(line, '%*u %u %u %u %u %u %u')';
            Q_EQUEUE(size(Q_EQUEUE,1)+1,:) = ...
                [tmp(1) NaN tmp(3) (tmp(6)+1) NaN tmp(2) tmp(4) tmp(5) 0 -1];

        case 17   %  QS_QF_ACTIVE_GET_LAST
            tmp = sscanf(line, '%*u %u %u %u %u %u')';
            Q_EQUEUE(size(Q_EQUEUE,1)+1,:) = ...
                [tmp(1) NaN tmp(3) NaN NaN tmp(2) tmp(4) tmp(5) 0 -1];

        case 18   %  QS_QF_EQUEUE_INIT
            tmp = sscanf(line, '%*u %u %u')';
            Q_EQUEUE(size(Q_EQUEUE,1)+1,:) = ...
                [NaN NaN tmp(1) NaN (tmp(2)+1) NaN NaN NaN 0 0];

        case 19   %  QS_QF_EQUEUE_POST_FIFO
            tmp = sscanf(line, '%*u %u %u %u %u %u %u %u')';
            Q_EQUEUE(size(Q_EQUEUE,1)+1,:) = ...
               [tmp(1) NaN tmp(3) (tmp(6)+1) (tmp(7)+1) tmp(2) tmp(4) tmp(5) 0 1];

        case 20   %  QS_QF_EQUEUE_POST_LIFO
            tmp = sscanf(line, '%*u %u %u %u %u %u %u %u')';
            Q_EQUEUE(size(Q_EQUEUE,1)+1,:) = ...
               [tmp(1) NaN tmp(3) (tmp(6)+1) (tmp(7)+1) tmp(2) tmp(4) tmp(5) 1 1];

        case 21   %  QS_QF_EQUEUE_GET
            tmp = sscanf(line, '%*u %u %u %u %u %u %u')';
            Q_EQUEUE(size(Q_EQUEUE,1)+1,:) = ...
                [tmp(1) NaN tmp(3) (tmp(6)+1) 0 tmp(2) tmp(4) tmp(5) 0 -1];

        case 22   %  QS_QF_EQUEUE_GET_LAST
            tmp = sscanf(line, '%*u %u %u %u %u %u')';
            Q_EQUEUE(size(Q_EQUEUE,1)+1,:) = ...
                [tmp(1) NaN tmp(3) 0 0 tmp(2) tmp(4) tmp(5) 0 -1];

        case 23   %  QS_QF_MPOOL_INIT
            Q_MPOOL(size(Q_MPOOL,1)+1,:) = ...
                [NaN sscanf(line, '%*u %u %u')' NaN 0];

        case 24   %  QS_QF_MPOOL_GET
            Q_MPOOL(size(Q_MPOOL,1)+1,:) = ...
                [sscanf(line, '%*u %u %u %u %u')' -1];

        case 25   %  QS_QF_MPOOL_PUT
            Q_MPOOL(size(Q_MPOOL,1)+1,:) = ...
                [sscanf(line, '%*u %u %u %u')' NaN +1];

        case 26   %  QS_QF_PUBLISH_ATTEMPT
            Q_PUB(size(Q_PUB,1)+1,:) = [sscanf(line, '%*u %u %u %u %u %u')' 0];

        case 27   %  QS_QF_PUBLISH
            Q_PUB(size(Q_PUB,1)+1,:) = sscanf(line, '%*u %u %u %u %u %u %u')';

        case 28   %  QS_QF_NEW
            tmp = sscanf(line, '%*u %u %u %u')';
            Q_NEW(size(Q_NEW,1)+1,:) = [tmp(1) tmp(3) NaN NaN tmp(2) 1];

        case 29   %  QS_QF_GC_ATTEMPT
            Q_NEW(size(Q_NEW,1)+1,:) = ...
                [sscanf(line, '%*u %u %u %u %u')' NaN 0];

        case 30   %  QS_QF_GC
            Q_NEW(size(Q_NEW,1)+1,:) = ...
                [sscanf(line, '%*u %u %u %u %u')' NaN -1];

        case 31   %  QS_QF_TICK
            Q_TIME(size(Q_TIME,1)+1,:) = ...
                [NaN NaN NaN NaN sscanf(line, '%*u %u') NaN 0];

        case 32   %  QS_QF_TIMEEVT_ARM
            tmp = sscanf(line, '%*u %u %u %u %u %u')';
            Q_TIME(size(Q_TIME,1)+1,:) = ...
                [tmp(1) tmp(2) NaN tmp(3) tmp(4) tmp(5) 1];

        case 33   %  QS_QF_TIMEEVT_AUTO_DISARM
            tmp = sscanf(line, '%*u %u %u')';
            Q_TIME(size(Q_TIME,1)+1,:) = [NaN tmp(1) NaN tmp(2) NaN NaN -1];

        case 34   %  QS_QF_TIMEEVT_DISARM_ATTEMPT
            tmp = sscanf(line, '%*u %u %u %u')';
            Q_TIME(size(Q_TIME,1)+1,:) = [tmp(1) tmp(2) NaN tmp(3) NaN NaN 0];

        case 35   %  QS_QF_TIMEEVT_DISARM
            tmp = sscanf(line, '%*u %u %u %u %u %u')';
            Q_TIME(size(Q_TIME,1)+1,:) = ...
                [tmp(1) tmp(2) NaN tmp(3) tmp(4) tmp(5) -1];

        case 36   %  QS_QF_TIMEEVT_REARM
            tmp = sscanf(line, '%*u %u %u %u %u %u %u')';
            Q_TIME(size(Q_TIME,1)+1,:) = ...
                [tmp(1) tmp(2) NaN tmp(3) tmp(4) tmp(5) (1-tmp(6))];

        case 37   %  QS_QF_TIMEEVT_POST
            Q_TIME(size(Q_TIME,1)+1,:) = ...
                [sscanf(line, '%*u %u %u %u %u')' NaN NaN 0];

        case 38   %  QS_QF_TIMEEVT_CTR
            Q_TIME(size(Q_TIME,1)+1,:) = ...
                [tmp(1) tmp(2) NaN tmp(3) tmp(4) tmp(5) 0];

        case 39   %  QS_QF_INT_LOCK
            Q_INT(size(Q_INT,1)+1,:) = [sscanf(line, '%*u %u %u')' 1];

        case 40   %  QS_QF_INT_UNLOCK
            Q_INT(size(Q_INT,1)+1,:) = [sscanf(line, '%*u %u %u')' -1];

        case 41   %  QS_QF_ISR_ENTRY
            Q_ISR(size(Q_ISR,1)+1,:) = [sscanf(line, '%*u %u %u %u')' 1];

        case 42   %  QS_QF_ISR_EXIT
            Q_ISR(size(Q_ISR,1)+1,:) = [sscanf(line, '%*u %u %u %u')' -1];

        case 43   %  QS_QF_RESERVED6

        case 44   %  QS_QF_RESERVED5

        case 45   %  QS_QF_RESERVED4

        case 46   %  QS_QF_RESERVED3

        case 47   %  QS_QF_RESERVED2

        case 48   %  QS_QF_RESERVED1

        case 49   %  QS_QF_RESERVED0


        % QK records
        case 50   %  QS_QK_MUTEX_LOCK
            Q_MUTEX(size(Q_MUTEX,1)+1,:) = [sscanf(line, '%*u %u %u %u')' 1];

        case 51   %  QS_QK_MUTEX_UNLOCK
            Q_MUTEX(size(Q_MUTEX,1)+1,:) = [sscanf(line, '%*u %u %u %u')' -1];

        case 52   %  QS_QK_SCHEDULE
            Q_SCHED(size(Q_SCHED,1)+1,:) = sscanf(line, '%*u %u %u %u')';

        case 53   %  QS_QK_RESERVED6

        case 54   %  QS_QK_RESERVED5

        case 55   %  QS_QK_RESERVED4

        case 56   %  QS_QK_RESERVED3

        case 57   %  QS_QK_RESERVED2

        case 58   %  QS_QK_RESERVED1

        case 59   %  QS_QK_RESERVED0


        % Miscallaneous QS records
        case 60   %  QS_SIG_DICTIONARY
            eval(line(5:end));

        case 61   %  QS_OBJ_DICTIONARY
            eval(line(5:end));

        case 62   %  QS_FUN_DICTIONARY
            eval(line(5:end));

        case 63   %  QS_ASSERT,

        case 64   %  QS_RESERVED5,

        case 65   %  QS_RESERVED4,

        case 66   %  QS_RESERVED3,

        case 67   %  QS_RESERVED2,

        case 68   %  QS_RESERVED1,

        case 69   %  QS_RESERVED0,


        % User records
        % . . .

    end
end

% cleanup ...
fclose(fid);
clear fid;
clear line;
clear rec;
clear tmp;

% display status information ...
Q_TOT
whos

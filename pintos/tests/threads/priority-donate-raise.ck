# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(priority-donate-raise) begin
(priority-donate-raise) Main thread should have priority 41.  Actual priority: 41.
(priority-donate-raise) Main thread should have priority 50.  Actual priority: 50.
(priority-donate-raise) Main thread should have priority 50.  Actual priority: 50.
(priority-donate-raise) Thread A acquired lock.
(priority-donate-raise) Thread A finished.
(priority-donate-raise) end
EOF
pass;
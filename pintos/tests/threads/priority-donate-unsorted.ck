# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(priority-donate-unsorted) begin
(priority-donate-unsorted) Main thread should have priority 30.  Actual priority: 30.
(priority-donate-unsorted) Main thread should have priority 30.  Actual priority: 30.
(priority-donate-unsorted) T_low acquired lock_a.
(priority-donate-unsorted) VIP acquired lock_b.
(priority-donate-unsorted) T_mid acquired lock_a.
(priority-donate-unsorted) end
EOF
pass;
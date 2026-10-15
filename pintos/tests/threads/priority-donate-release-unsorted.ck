# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(priority-donate-release-unsorted) begin
(priority-donate-release-unsorted) Main thread should have priority 30.  Actual priority: 30.
(priority-donate-release-unsorted) Main thread should have priority 30.  Actual priority: 30.
(priority-donate-release-unsorted) T1 acquired lock_b.
(priority-donate-release-unsorted) VIP acquired lock_c.
(priority-donate-release-unsorted) T2 acquired lock_b.
(priority-donate-release-unsorted) T3 acquired lock_a.
(priority-donate-release-unsorted) end
EOF
pass;
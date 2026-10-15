# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(priority-donate-ready) begin
(priority-donate-ready) Thread B acquired lock.
(priority-donate-ready) Thread C is running.
(priority-donate-ready) Thread A will acquire lock.
(priority-donate-ready) Thread B finished.
(priority-donate-ready) Thread A acquired lock.
(priority-donate-ready) Thread A finished.
(priority-donate-ready) Thread C finished.
(priority-donate-ready) end
EOF
pass;
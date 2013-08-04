#/bin/sh
valgrind --leak-check=full --show-reachable=yes --track-origins=yes src/core/webcontrol_helper -t 2 -c webcontrol.conf -p "txtfile"

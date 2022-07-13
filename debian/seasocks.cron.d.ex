#
# Regular cron jobs for the seasocks package
#
0 4	* * *	root	[ -x /usr/bin/seasocks_maintenance ] && /usr/bin/seasocks_maintenance

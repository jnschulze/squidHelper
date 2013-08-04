#!/usr/bin/env python

import subprocess, os, time, signal, sys
print os.getpid()
#raw_input()

#path = "./test.sh"
path = ["./src/core/webcontrol_helper", "-t", "1", "-c", "webcontrol.conf", "-p", "memcached,activedirectory"]

#path = ["./test.sh"]

#stdout_file = open('test_stdout.log', 'w')
#stdout_file = sys.stdout
stderr_file = open('test_stderr.log', 'w')
#stderr_file = sys.stderr

#proc = subprocess.Popen(path, shell=False, stdin=subprocess.PIPE, stderr=stderr_file, stdout=stdout_file)

proc = subprocess.Popen(path, shell=False, stdout=sys.stdout, stdin=subprocess.PIPE, stderr=stderr_file)

print "subprocess pid is %d" % proc.pid
raw_input()

# Some time to start everything up
#time.sleep(2)

print("Starting Tests")

for i in range(0,250000):
	channel = i+1
	#print("Write Line %d", channel)
	if (channel) % 50 == 0:
		print("Completed %d requests" % channel)
		time.sleep(0.005)
	try:
		if (channel) % 2 == 0:
			proc.stdin.write("%d 192.168.222.23\n" % channel)
		else:
			proc.stdin.write("%d 192.168.111.25\n" % channel)
	except:
		pass
	#time.sleep(0.005)

print("Finished Tests")

#stdout_file.flush()
stderr_file.flush()

#proc.stdout.flush()
#proc.stderr.flush()

#print proc.stdout.read()
#print proc.stderr.read()

time.sleep(2)

proc.send_signal(signal.SIGINT)

returncode = proc.wait()
print "Returncode: %d" % returncode

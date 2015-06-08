Project: pibuttonpower
Modifications by: Scott Edlin

This is a project based on Sanjeev Sharma's buttonshutdown-daemon as referenced below.  I made trivial modifications to the code to support a two-pin hardware interface rather than the original three-pin interface.


Original:

Project: buttonshutdown-daemon
Written by: Sanjeev Sharma
More information at http://sanje2v.wordpress.com/

This daemon is used to monitor wiringPi GPIO pin 0 on whether pushbutton has been depressed. If the pushbutton is depressed for less than 2 secs, the daemon initiates a shutdown while if depressed for 2 or more seconds, the daemon restarts the system.

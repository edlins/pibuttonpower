/*
buttonshutdown-daemon - When the push button connected to wiringPi GPIO pin 0 is pushed
   and released this daemon initiates a system shutdown. If the button is held down
   for 2 secs or longer, a restart is initiated.

Written By: Sanjeev Sharma (http://sanje2v.wordpress.com/)
License: Freeware
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
// We want a success/failure return value from 'wiringPiSetup()'
//#define WIRINGPI_CODES 1
// #include <wiringPi.h>
#include <pigpio.h>

#define DAEMON_NAME "pibuttonpower"
#define PID_FILE "/var/run/" DAEMON_NAME ".pid"
// #define PIN			26	/* This is wiringPi pin 0 which is physical pin 8 */
// #define PIN_STR			"26"
char *pinstr;
int pinnum;

/* Function prototypes */
void Daemon_Stop(int signum);
void Button_Pressed(int gpio, int level, uint32_t tick);

/* ------------------ Start Here ----------------------- */
int main(int argc, char *argv[])
{
   /* command line arg */
   int c;
   while ((c = getopt(argc, argv, "p:")) != -1)
   {
      int this_optind = optind ? optind : 1;
      switch (c)
      {
      case 'p':
         pinstr = optarg;
         pinnum = strtol(pinstr, NULL, 10);
         break;
      case '?':
         if (optopt == 'c')
            fprintf(stderr, "Option -%c requires an argument.\n", optopt);
         return 1;
      default:
         abort();
      } // switch
   }    // while
   if (pinstr == NULL)
   {
      fprintf(stderr, "Error: pin must be specified with -p <wiringPiPin>\n");
      return 1;
   } // unless
   if ((pinnum < 1) || (pinnum > 31))
   {
      fprintf(stderr, "Error: pin %d is not between 1 and 20\n", pinnum);
      return 1;
   }
   // printf("Starting daemon on pin %d\n", pinnum);

   /* Logging */
   setlogmask(LOG_UPTO(LOG_INFO));
   openlog(DAEMON_NAME, LOG_CONS | LOG_PERROR, LOG_USER);
   syslog(LOG_INFO, "Daemon starting up on pin %d", pinnum);

   /* This daemon can only run as root. So make sure that it is. */
   if (geteuid() != 0)
   {
      syslog(LOG_ERR, "This daemon can only be run by root user, exiting");
      exit(EXIT_FAILURE);
   }

   /* Make sure the file '/usr/local/bin/gpio' exists */
   //	struct stat filestat;
   //	if (stat("/usr/local/bin/gpio", &filestat) == -1)
   //	{
   //	    syslog(LOG_ERR, "The program '/usr/local/bin/gpio' is missing, exiting");
   //	    exit(EXIT_FAILURE);
   //	}

   /* Ensure only one copy */
   /*
   Common user should be able to read the pid file so that they
   need not use 'sudo' with 'service buttonshutdown-daemon status'
   to read daemon status. The correct PID file permission should be:
      1. Read & Write permission for owner
      2. Read permission for group and others
   */
   syslog(LOG_INFO, "Handle PID file\n");
   const int PIDFILE_PERMISSION = 0644;
   int pidFilehandle = open(PID_FILE, O_RDWR | O_CREAT, PIDFILE_PERMISSION);
   if (pidFilehandle == -1)
   {
      /* Couldn't open lock file */
      syslog(LOG_ERR, "Could not open PID lock file %s, exiting", PID_FILE);
      exit(EXIT_FAILURE);
   }

   /* Try to lock file */
   if (lockf(pidFilehandle, F_TLOCK, 0) == -1)
   {
      /* Couldn't get lock on lock file */
      syslog(LOG_ERR, "Could not lock PID lock file %s, exiting", PID_FILE);
      exit(EXIT_FAILURE);
   }

   /* Our process ID and Session ID */
   pid_t pid, sid;

   /* Fork off the parent process */
   pid = fork();
   if (pid < 0)
      exit(EXIT_FAILURE);

   /* If we got a good PID, then
      we can exit the parent process. */
   if (pid > 0)
      exit(EXIT_SUCCESS);

   syslog(LOG_INFO, "Get and format PID\n");
   /* Get and format PID */
   char szPID[16];
   sprintf(szPID, "%d\n", getpid()); // Call 'getpid()', don't use 'pid' variable

   /* write pid to lockfile */
   write(pidFilehandle, szPID, strlen(szPID));

   /* Change the file mode mask */
   umask(0);

   /* Create a new SID for the child process */
   syslog(LOG_INFO, "setsid()\n");
   sid = setsid();
   if (sid < 0) /* Log the failure */
   {
      syslog(LOG_ERR, "setsid() failed with %d\n", sid);
      exit(EXIT_FAILURE);
   }
   syslog(LOG_INFO, "setsid() succeeded with %d\n", sid);

   /* Change the current working directory */
   syslog(LOG_INFO, "chdir()\n");
   if (chdir("/") < 0) /* Log the failure */
   {
      syslog(LOG_ERR, "chdir() failed with a negative value");
      exit(EXIT_FAILURE);
   }
   syslog(LOG_INFO, "chdir() succeeded\n");

   /* Close out the standard file descriptors */
   close(STDIN_FILENO);
   close(STDOUT_FILENO);
   close(STDERR_FILENO);

   /* Daemon-specific initializations */
   syslog(LOG_INFO, "Register SIGTERM handler\n");
   /* Add a process termination handler for
      handling daemon stop requests */
   signal(SIGTERM, &Daemon_Stop);

   syslog(LOG_INFO, "Initializing pigpio\n");
   int init = gpioInitialise();
   if (init < 0)
   {
      syslog(LOG_ERR, "gpioInitialise failed with %d\n", init);
      exit(init);
   }
   else
   {
      syslog(LOG_INFO, "gpioInitialise succeeded with %d\n", init);
   }

   /* Setup pin mode and interrupt handler */
   //	pinMode(pinnum, INPUT);
   gpioSetMode(pinnum, PI_INPUT);
   syslog(LOG_INFO, "setting pinMode on %d\n", pinnum);
   //	pullUpDnControl(pinnum, PUD_UP);
   gpioSetPullUpDown(pinnum, PI_PUD_UP);
   //	if (wiringPiISR(pinnum, INT_EDGE_FALLING, &Button_Pressed) == -1)
   if (gpioSetAlertFunc(pinnum, Button_Pressed) != 0)
   {
      syslog(LOG_ERR, "Unable to set interrupt handler for specified pin, exiting");
      exit(EXIT_FAILURE);
   }

   /*
The Big Loop
 1. When pressed for less than 2 secs, shutdown system
 2. When pressed for 2 secs or more, restart system
*/

   syslog(LOG_INFO, "Start the big loop\n");
   while (true)
   {
      /*
      Daemon hearbeat
         Just wait until there's an interrupt or system shutdown
      */
      syslog(LOG_INFO, "sleep\n");
      sleep(60);
      syslog(LOG_INFO, "wake\n");
   }
}

void Daemon_Stop(int signum)
{
   /* 'SIGTERM' was issued, system is telling this daemon to stop */
   syslog(LOG_INFO, "Stopping daemon");
   exit(EXIT_SUCCESS);
}

void Button_Pressed(int gpio, int level, uint32_t tick)
{
   /* Handle button pressed interrupts */

   /* Disable further interrupts */
   /* NOTE: Unfortunately, 'wiringPi' library doesn't support
      unhooking an existing interrupt handler, so we need
      to use 'gpio' binary to do this according to the author */
   //char command[128];
   //strcpy(command, "/usr/local/bin/gpio edge ");
   //strcat(command, pinstr);
   //strcat(command, " none");
   //syslog(LOG_INFO, "command = %s\n", command);
   // syslog(LOG_INFO, command);
   //	system(command);
   gpioSetAlertFunc(pinnum, NULL);

   /* Just wait for user to press the button */
   sleep(2);

   //	switch (digitalRead(pinnum))
   switch (gpioRead(pinnum)) {
      case PI_HIGH:
         syslog(LOG_INFO, "Shutting down system");
         break;
   //		    if (execl("/sbin/poweroff", "poweroff", NULL) == -1)
   //			syslog(LOG_ERR, "'poweroff' program failed to run with error: %d", errno);
   // NOTE: Execution will not reach here if 'execl()' succeeds
          //break;

   //		case LOW:	// Restart requested
      case PI_LOW:
         syslog(LOG_INFO, "Restarting system");
         break;

   //		    if (execl("/sbin/shutdown", "shutdown", "-r", "now", NULL) == -1)
   //			syslog(LOG_ERR, "'shutdown' program failed to run with error: %d", errno);

   // NOTE: Execution will not reach here if 'execl()' succeeds
   }

   //exit(EXIT_SUCCESS);
}

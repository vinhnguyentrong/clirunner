/* Virtual terminal interface shell.
 * Copyright (C) 2000 Kunihiro Ishiguro
 *
 * This file is part of GNU Zebra.
 *
 * GNU Zebra is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * GNU Zebra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Zebra; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <zebra.h>

#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <pwd.h>
#include <pthread.h>
#include <time.h>
#include <termios.h>
#include <sys/epoll.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <lib/version.h>
#include "getopt.h"
#include "command.h"
#include "memory.h"

#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "if.h"
#include "lib/conn.h"

#define SERVER_ADDR "192.168.2.67"
#define PORT 8086

char history_file[MAXPATHLEN];
/* VTY shell program name. */
char *progname;

/* Configuration file name and directory. */
char config_default[] = SYSCONFDIR VTYSH_DEFAULT_CONFIG;


/* Flag for indicate executing child command. */
int execute_flag = 0;

/* For sigsetjmp() & siglongjmp(). */
static sigjmp_buf jmpbuf;

/* Flag for avoid recursive siglongjmp() call. */
static int jmpflag = 0;

/* A static variable for holding the line. */
static char *line_read;

/* Master of threads. */
struct thread_master *master;

/* Command logging */
FILE *logfile;

char *system_intfs[256];

int IDLE_TIME_OUT = 300;

time_t timeout_start;
struct termios tp;

struct conn *conn;


/* SIGTSTP handler.  This function care user's ^Z input. */
void
sigtstp (int sig)
{
  /* Execute "end" command. */
  vtysh_execute ("end");

  /* Initialize readline. */
  rl_initialize ();
  printf ("\n");

  /* Check jmpflag for duplicate siglongjmp(). */
  if (! jmpflag)
    return;

  jmpflag = 0;

  /* Back to main command loop. */
  siglongjmp (jmpbuf, 1);
}

/* SIGINT handler.  This function care user's ^Z input.  */
void
sigint (int sig)
{
  /* Check this process is not child process. */
  if (! execute_flag)
    {
      rl_initialize ();
      printf ("\n");
      rl_forced_update_display ();
    }
}

/* Signale wrapper for vtysh. We don't use sigevent because
 * vtysh doesn't use threads. TODO */
RETSIGTYPE *
vtysh_signal_set (int signo, void (*func)(int))
{
  int ret;
  struct sigaction sig;
  struct sigaction osig;

  sig.sa_handler = func;
  sigemptyset (&sig.sa_mask);
  sig.sa_flags = 0;
#ifdef SA_RESTART
  sig.sa_flags |= SA_RESTART;
#endif /* SA_RESTART */

  ret = sigaction (signo, &sig, &osig);

  if (ret < 0)
    return (SIG_ERR);
  else
    return (osig.sa_handler);
}

/* Initialization of signal handles. */
void
vtysh_signal_init ()
{
  vtysh_signal_set (SIGINT, sigint);
  vtysh_signal_set (SIGTSTP, sigtstp);
  vtysh_signal_set (SIGPIPE, SIG_IGN);
}

/* Read a string, and return a pointer to it.  Returns NULL on EOF. */
char *
vtysh_rl_gets ()
{
  HIST_ENTRY *last;
  /* If the buffer has already been allocated, return the memory
   * to the free pool. */
  if (line_read)
    {
      free (line_read);
      line_read = NULL;
    }

  /* Timer start for idle session timeout. */
  time(&timeout_start);
  /* Copying terminal settings to global variable. */
  tcgetattr(STDIN_FILENO, &tp);

  /* Get a line from the user.  Change prompt according to node.  XXX. */
  line_read = readline (vtysh_prompt ());

  /* If the line has any text in it, save it on the history. But only if
   * last command in history isn't the same one. */
  if (line_read && *line_read)
    {
      using_history();
      last = previous_history();
      if (!last || strcmp (last->line, line_read) != 0) {
	add_history (line_read);
	append_history(1,history_file);
      }
    }

  return (line_read);
}

void *vtysh_timeout_detect_thread(void *arg) {
  time_t now;
  char *value;
  if ((value = getenv("TMOUT")) != NULL)
    IDLE_TIME_OUT = atoi(value);

  pthread_detach(pthread_self());
  time(&timeout_start);
  while(true)
  {
    time(&now);

    if ((IDLE_TIME_OUT > 0) && ((now - timeout_start) > IDLE_TIME_OUT)) {
      tcsetattr(STDIN_FILENO, TCSANOW, &tp);
      vty_out(vty, "%s%s", VTY_NEWLINE, VTY_NEWLINE);
      vty_out(vty, "Idle session timeout reached, logging out.%s",VTY_NEWLINE);
      vty_out(vty, "%s%s", VTY_NEWLINE, VTY_NEWLINE);
      exit(0);
    }
    sleep(5);
  }
}

void *check_conn() {
  struct epoll_event event;
  int even_count;

  event.events = EPOLLRDHUP | EPOLLERR | EPOLLHUP;
  event.data.fd = conn->fd_client;

  epoll_ctl(conn->fd_server_group, EPOLL_CTL_ADD, conn->fd_client, &event);

  while(1) {
    if (conn == NULL) {
      return 0;
    }
    even_count = epoll_wait(conn->fd_server_group, &event, 1, 0);
    if (even_count > 0) {
      if ((event.events &  (EPOLLERR | EPOLLHUP | EPOLLRDHUP))) {
        printf("Connection is interrupted. Press Enter\n");
        epoll_ctl(conn->fd_server_group, EPOLL_CTL_DEL, conn->fd_client, NULL);
        conn_free(conn);
        conn = NULL;
        return 0;
      }
    }
  }
}

/* VTY shell main routine. */
int
main (int argc, char **argv, char **env)
{
  char *p;
  int ret;
  pthread_t conn_handling;

  pthread_t vtysh_timeout_thread;


  /* Preserve name of myself. */
  progname = ((p = strrchr (argv[0], '/')) ? ++p : argv[0]);

  /* if logging open now */
  if ((p = getenv("VTYSH_LOG")) != NULL)
      logfile = fopen(p, "a");

  /* Initialize user input buffer. */
  line_read = NULL;
  setlinebuf(stdout);

  if_init ();
  //get_system_intfs(system_intfs);
  /* Signal and others. */
  vtysh_signal_init ();

  /* Make sure we pass authentication before proceeding. */
  vtysh_auth ();

  /* Make vty structure and register commands. */
  vtysh_init_vty ();
  vtysh_init_cmd ();
  vtysh_show_ip_init();
  vtysh_linux_cmd_init();
  vtysh_user_init ();
  vtysh_config_init ();
  vtysh_if_init();
  vtysh_dhcp_init();
  vtysh_upgrade_init();
  vtysh_arp_init();
  vtysh_bgp_init();
  vtysh_ippp_init();

  vty_init_vtysh ();

  ret = pthread_create(&vtysh_timeout_thread,
                       (pthread_attr_t *)NULL,
                        vtysh_timeout_detect_thread,
                        NULL);
  if (ret)
  {
    printf("Failed to create the idle timeout thread %d",ret);
    exit(-ret);
  }

  vtysh_pager_init ();

  vtysh_readline_init ();

  vty_hello (vty);

  //vtysh_execute("enable");
  //vtysh_execute("configure terminal");
  conn = conn_init(SERVER_ADDR, PORT);
  if (conn == NULL) {
    printf("Cannot establish connection!\n");
  }

  if (conn != NULL) 
    pthread_create(&conn_handling, NULL, check_conn, NULL);
  
  /* Preparation for longjmp() in sigtstp(). */
  sigsetjmp (jmpbuf, 1);
  jmpflag = 1;

  snprintf(history_file, sizeof(history_file), "%s/.history_quagga", getenv("HOME"));
  read_history(history_file);
  /* Main command loop. */
  while (vtysh_rl_gets ())
    vtysh_execute (line_read);

  if_terminate();
  history_truncate_file(history_file,1000);
  printf ("\n");

  /* Rest in peace. */
  exit (0);
}

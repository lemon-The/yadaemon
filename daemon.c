#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <syslog.h>
const char p_name[] = "I'm your daemon"; /*daemon's name*/
const char p_pid[] = "Pid: ";            /*daemon's pid*/
const char p_sigcount[] = "SIGUSR1: ";   /*count of SIGUSR1 calls*/
const char p_time[] = "Time: ";          /*process running time*/
const char separator[] = "|";
volatile static sig_atomic_t fd = -1;   /*"/tmp/file_daemon" fd*/ 
volatile static sig_atomic_t sigusr1_calls_amount = 0;
volatile static sig_atomic_t start_time = 0;    /*start time of the*/
                                                /*process*/

/*converts positive int to string. returns len of the number*/
int itostr(char *str, int len, int num)
{
    char *result = str;
    char tmp_str[len];
    int i, j;
    for(i = 0; num && i < len-1; i++){
        tmp_str[i] = num%10 + '0';
        num /= 10;
    }
    for(j = 0, i--; i >= 0; j++, i--)
        result[j] = tmp_str[i];
    result[j] = 0;
    if(!j){ /*put '0' to result if it is empty*/
        result[j] = '0';
        j = 1;
    }
    return j;
}

/*prints daemon info to /tmp/file_daemon*/
void print_info_to_file()
{
    char sigusr_str[100];
    char elapsed_str[1000000];
    char pid[100];
    int len;
    clock_t elapsed = time(NULL) - start_time;
    write(fd, p_name, sizeof(p_name));                  /*print name*/
    write(fd, separator, sizeof(separator));

    write(fd, p_pid, sizeof(p_pid));                    /*print pid*/
    len = itostr(pid, 100, getpid());
    write(fd, pid, sizeof(char)*len);
    write(fd, separator, sizeof(separator));

    write(fd, p_sigcount, sizeof(p_sigcount));          /*print sigcount*/
    len = itostr(sigusr_str, 100, sigusr1_calls_amount);/*amount*/
    write(fd, sigusr_str, sizeof(char)*len);
    write(fd, separator, sizeof(separator));

    write(fd, p_time, sizeof(p_time));                  /*print process*/
    len = itostr(elapsed_str, 1000000, elapsed);        /*running time*/
    write(fd, elapsed_str, sizeof(char)*len);
    write(fd, separator, sizeof(separator));

    write(fd, "\n", sizeof(char));
}

/*prints daemon info to the journal*/
void print_info_to_log()
{
    clock_t elapsed = time(NULL) - start_time;
    syslog(LOG_INFO, "%s|%s%d|%s%d|%s%d\n", p_name, p_pid,
        getpid(), p_sigcount, sigusr1_calls_amount, p_time, (int)elapsed);
}

/*increases sigusr1_calls_amount and prints info to the file and journal*/
void sigusr1_handler(int s)
{
    int save_errno = errno;
    signal(SIGUSR1, sigusr1_handler);
    sigusr1_calls_amount++;
    print_info_to_file();
    print_info_to_log();
    errno = save_errno;
}

/*prints info to the file and journal*/
void sigalrm_handler(int s)
{
    int save_errno = errno;
    signal(SIGALRM, sigalrm_handler);
    print_info_to_file();
    print_info_to_log();
    errno = save_errno;
}

int main()
{
    int pid;
    close(0);       /*"daemonisation" of the process*/
    close(1);
    close(2);
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
    chdir("/");
    pid = fork();
    if(pid > 0)
        exit(0);
    setsid();
    pid = fork();
    if(pid > 0)
        exit(0);
    fd = open("/tmp/file_daemon", O_WRONLY | O_CREAT | O_TRUNC, 0666); 
    openlog("Yet another daemon", 0, LOG_USER);
    start_time = time(NULL);
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGALRM, sigalrm_handler);
    while(1){
        alarm(5);
        pause();
    }
    close(fd);
    closelog();
    return 0;
}

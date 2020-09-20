/* 
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file 
 * you will need to modify Makefile to compile
 * your additional functions.
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Submit the entire lab1 folder as a tar archive (.tgz).
 * Command to create submission archive: 
      $> tar cvf lab1.tgz lab1/
 *
 * All the best 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include "parse.h"

#define TRUE 1
#define FALSE 0
#define READ 0
#define WRITE 1
#define STDIN 0
#define STDOUT 1

void SignalHandler(int);
void RunCommandPipe(Command *, int, int);
void RunCommand(int, Command *);
void DebugPrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);

int main(void)
{
  Command cmd;
  int parse_result;
  signal(SIGINT, SignalHandler);
  signal(SIGCHLD, SignalHandler);
  while (TRUE)
  {
    char *line;
    line = readline("> ");

    /* If EOF encountered, exit shell */
    if (!line)
    {
      break;
    }
    /* Remove leading and trailing whitespace from the line */
    stripwhite(line);
    /* If stripped line not blank */
    if (*line)
    {
      add_history(line);
      parse_result = parse(line, &cmd);
      RunCommand(parse_result, &cmd);
    }

    /* Clear memory */
    free(line);
  }
  return 0;
}

void SignalHandler(int sigNo)
{
  // TODO: Remove debug prints
  if (sigNo == SIGINT) {
    printf("CTRL C Pressed\n> ");
  }
  if (sigNo == SIGCHLD) {
    wait(NULL);
  }
}

/* Execute the given command(s).

 * Note: The function currently only prints the command(s).
 * 
 * TODO: 
 * 1. Implement this function so that it executes the given command(s).
 * 2. Remove the debug printing before the final submission.
 */
void RunCommandPipe(Command *cmd, int readPipe, int writePipe) {
  printf("readPipe: %d\n", readPipe);
  printf("writePipe: %d\n", writePipe);
  printf("pipe: %s\n", cmd->pgm->pgmlist[0]);
  Pgm *currentPgm = cmd->pgm;
  int hasPipe = FALSE;
  int result;
  char *rstdin = cmd->rstdin;
  if(currentPgm->next != NULL) {
    hasPipe = TRUE;
  }
  pid_t pid = fork();
  if(pid < 0){
    printf("Error creating process\n");
  } else if (pid == 0) { //CHILD
    dup2(writePipe, STDOUT);
    close(writePipe);
    if(hasPipe == TRUE) {
      dup2(readPipe, STDIN);
      cmd->pgm = currentPgm->next;
      //TODO error check
      RunCommandPipe(cmd, readPipe, writePipe);
    } else {
      if(rstdin != NULL) {
        int fd = open(rstdin, O_RDONLY);
        if (fd < 0) {
          printf("Failed to open: %s\n", rstdin);
        } else {
          result = dup2(fd, STDIN);
          if (result < 0) {
            printf("Failed to dup: %d\n", fd);
          }
          result = close(fd);
          if (result < 0) {
            printf("Failed to close %d\n", fd);
          }
        }
      }
    }
    close(readPipe);
    result = execvp(currentPgm->pgmlist[0], currentPgm->pgmlist);
    if (result < 0){
      printf("Command '%s' not found.\n", *currentPgm->pgmlist);
    }
    exit(1);
  } else {
    printf("Waiting2\n");
    wait(NULL);
    printf("No longer waiting2\n");
  }
}

void RunCommand(int parse_result, Command *cmd) {
  int result;
  if(parse_result < 0){
    printf("Parse ERROR\n");
    return;
  }
  printf("runCommand: %s\n", cmd->pgm->pgmlist[0]);
  // Exit
  if(strcmp(cmd->pgm->pgmlist[0], "exit") == 0){
    exit(EXIT_SUCCESS);
  }
  // CD
  if(strcmp(cmd->pgm->pgmlist[0], "cd") == 0){
      if(chdir(*++cmd->pgm->pgmlist) != 0) {
        printf("No such directory: %s\n", *cmd->pgm->pgmlist);
      }
      return;
  }
  Pgm *currentPgm = cmd->pgm;
  char *rstdout = cmd->rstdout;
  char *rstdin = cmd->rstdin;
  int bg = cmd->background;
  int hasPipe = FALSE;
  int p[2];
  if(currentPgm->next != NULL){
    hasPipe = TRUE;
    result = pipe(p);
  }
  pid_t pid = fork();
  if(pid < 0){
    printf("Error creating process\n");
  }
  else if(pid == 0){ // CHILD
    if(hasPipe == TRUE) {
      cmd->pgm = currentPgm->next;
      //TODO error check
      printf("p[READ]: %d\n", p[READ]);
      printf("p[WRITE]: %d\n", p[WRITE]);
      RunCommandPipe(cmd, p[READ], p[WRITE]);
      printf("p2[READ]: %d\n", p[READ]);
      printf("p2[WRITE]: %d\n", p[WRITE]);
    } else {
      if (bg == TRUE) {
        setpgid(0,0);
      }
      //STDOUT
      if(rstdout != NULL) {
        int fd = open(rstdout, O_CREAT|O_WRONLY|O_TRUNC, 0644);
        if (fd < 0) {
          printf("Failed to open: %s\n", rstdout);
        } else {
          result = dup2(fd, STDOUT);
          if (result < 0) {
              printf("Failed to dup: %d\n", fd);
            }
          result = close(fd);
          if (result < 0) {
            printf("Failed to close %d\n", fd);
          }
        }
      }
      //STDIN
      if (rstdin != NULL) {
        int fd = open(rstdin, O_RDONLY);
        if (fd < 0) {
          printf("Failed to open: %s\n", rstdin);
        } else {
          result = dup2(fd, STDIN);
          if (result < 0) {
            printf("Failed to dup: %d\n", fd);
          }
          result = close(fd);
          if (result < 0) {
            printf("Failed to close %d\n", fd);
          }
        }
      }
    }
    printf("Executing in runCommand: %s\n", currentPgm->pgmlist[0]);
    result = execvp(currentPgm->pgmlist[0], currentPgm->pgmlist);
    if (result < 0){
      printf("Command '%s' not found.\n", *currentPgm->pgmlist);
    }
    exit(1);
  } else { // PARENT
    if (bg == FALSE) {
      printf("Waiting\n");
      wait(NULL);
      printf("no longer waiting\n");
    } else {
      printf("[+] %d\n", pid);
    }
  }
}
/* 
 * Print a Command structure as returned by parse on stdout. 
 * 
 * Helper function, no need to change. Might be useful to study as inpsiration.
 */
void DebugPrintCommand(int parse_result, Command *cmd)
{
  if (parse_result != 1) {
    printf("Parse ERROR\n");
    return;
  }
  printf("------------------------------\n");
  printf("Parse OK\n");
  printf("stdin:      %s\n", cmd->rstdin ? cmd->rstdin : "<none>");
  printf("stdout:     %s\n", cmd->rstdout ? cmd->rstdout : "<none>");
  printf("background: %s\n", cmd->background ? "true" : "false");
  printf("Pgms:\n");
  PrintPgm(cmd->pgm);
  printf("------------------------------\n");
}


/* Print a (linked) list of Pgm:s.
 * 
 * Helper function, no need to change. Might be useful to study as inpsiration.
 */
void PrintPgm(Pgm *p)
{
  if (p == NULL)
  {
    return;
  }
  else
  {
    char **pl = p->pgmlist;
    /* The list is in reversed order so print
     * it reversed to get right
     */
    PrintPgm(p->next);
    printf("            * [ ");
    while (*pl)
    {
      printf("%s ", *pl++);
    }
    printf("]\n");
  }
}


/* Strip whitespace from the start and end of a string. 
 *
 * Helper function, no need to change.
 */
void stripwhite(char *string)
{
  register int i = 0;

  while (isspace(string[i]))
  {
    i++;
  }

  if (i)
  {
    strcpy(string, string + i);
  }

  i = strlen(string) - 1;
  while (i > 0 && isspace(string[i]))
  {
    i--;
  }

  string[++i] = '\0';
}

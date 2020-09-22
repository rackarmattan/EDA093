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
void RunCommand(int, Command *);
void ExecuteCommand(Command *, int);
void CheckAndSetStdin(char *);
void CheckAndSetStdout(char *);
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
  if (sigNo == SIGINT)
  {
    printf("CTRL C Pressed\n> ");
  }
  if (sigNo == SIGCHLD)
  {
    wait(NULL);
  }
}

void CheckAndSetStdin(char *rstdin)
{
  if (rstdin != NULL)
  {
    int fd = open(rstdin, O_RDONLY);
    if (fd < 0)
    {
      perror("Failed to open input file\n");
      return;
    }
    int result = dup2(fd, STDIN);
    if (result < 0)
    {
      perror("Failed to dup file descriptor to STDIN\n");
      return;
    }
    result = close(fd);
    if (result < 0)
    {
      perror("Failed to close file descriptor\n");
    }
  }
}
void CheckAndSetStdout(char *rstdout)
{
  if (rstdout != NULL)
  {
    int fd = open(rstdout, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0)
    {
      perror("Failed to open input file\n");
      return;
    }
    int result = dup2(fd, STDOUT);
    if (result < 0)
    {
      perror("Failed to dup file descriptor to STDOUT\n");
      return;
    }
    result = close(fd);
    if (result < 0)
    {
      perror("Failed to close file descriptor\n");
    }
  }
}

void ExecuteCommand(Command *cmd, int writePipeFd)
{
  Pgm *currentPgm = cmd->pgm;
  int p[2];
  int result;
  int hasPipe = FALSE;
  if (currentPgm->next != NULL)
  {
    pipe(p);
    hasPipe = TRUE;
  }
  pid_t pid = fork();
  if (pid < 0)
  {
    perror("Error forking\n");
  }
  else if (pid == 0)
  { // CHILD
    if (writePipeFd >= 0)
    {
      dup2(writePipeFd, STDOUT);
    }
    else
    {
      CheckAndSetStdout(cmd->rstdout);
    }
    if (hasPipe)
    {
      dup2(p[READ], STDIN);
      cmd->pgm = cmd->pgm->next;
      ExecuteCommand(cmd, p[WRITE]);
      close(p[WRITE]);
      result = execvp(currentPgm->pgmlist[0], currentPgm->pgmlist);
      if (result < 0)
      {
        perror("Command not found.\n");
      }
      close(p[READ]);
    }
    else
    {
      CheckAndSetStdin(cmd->rstdin);
      if (cmd->background == TRUE)
      {
        setpgid(0, 0);
      }
      result = execvp(currentPgm->pgmlist[0], currentPgm->pgmlist);
      if (result < 0)
      {
        perror("Command not found.\n");
      }
    }
  }
  else
  { // PARENT
    if (hasPipe)
    {
      close(p[READ]);
      close(p[WRITE]);
    }
    if (cmd->background == FALSE)
    {
      wait(NULL);
    }
  }
}

void RunCommand(int parse_result, Command *cmd)
{
  //int result;
  if (parse_result < 0)
  {
    perror("Parse ERROR\n");
    return;
  }
  // Check for built-in functions

  // exit
  if (strcmp(cmd->pgm->pgmlist[0], "exit") == 0)
  {
    exit(EXIT_SUCCESS);
  }
  // cd
  if (strcmp(cmd->pgm->pgmlist[0], "cd") == 0)
  {
    if (chdir(*++cmd->pgm->pgmlist) != 0)
    {
      printf("No such directory: %s\n", *cmd->pgm->pgmlist);
    }
    return;
  }

  // If not a built-in execute command
  ExecuteCommand(cmd, -1);
}
/* 
 * Print a Command structure as returned by parse on stdout. 
 * 
 * Helper function, no need to change. Might be useful to study as inpsiration.
 */
void DebugPrintCommand(int parse_result, Command *cmd)
{
  if (parse_result != 1)
  {
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

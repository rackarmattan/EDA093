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
#include "parse.h"

#define TRUE 1
#define FALSE 0
//TODO: Change before hand-in
#define WUHAN WNOHANG

void SignalHandler(int);
void RunCommand(int, Command *);
void DebugPrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);

int main(void)
{
  Command cmd;
  int parse_result;
  signal(SIGINT, SignalHandler);
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
  printf("Received signal: %d\n", sigNo);
  if (sigNo == SIGINT) {
    printf("CTRL C Pressed\n> ");
  }
}

/* Execute the given command(s).

 * Note: The function currently only prints the command(s).
 * 
 * TODO: 
 * 1. Implement this function so that it executes the given command(s).
 * 2. Remove the debug printing before the final submission.
 */
void RunCommand(int parse_result, Command *cmd)
{
  if(parse_result < 0){
    printf("Parse ERROR");
    return;
  }
  DebugPrintCommand(parse_result, cmd);
  //TODO: Add checks for stdin, stdout, pipe, etc

  //exit
  if(strcmp(cmd->pgm->pgmlist[0], "exit") == 0){
    exit(EXIT_SUCCESS);
  }

  int bg = cmd->background;
  if(strcmp(cmd->pgm->pgmlist[0], "cd") == 0)
  {
    int result = chdir(*++cmd->pgm->pgmlist);
    if(result != 0) {
      printf("No such directory: %s\n", *cmd->pgm->pgmlist);
    }
    return;
  }
  pid_t pid = fork();
  if(pid < 0){
    printf("Error creating process");
  }
  else if(pid == 0){ // CHILD
    if (bg == TRUE) {
      //signal(SIGINT, SIG_IGN);
      setpgid(0,0);
    }
    int result = execvp(cmd->pgm->pgmlist[0], cmd->pgm->pgmlist);
    if(result < 0){
      printf("Command '%s' not found.\n", *cmd->pgm->pgmlist);
    }
    
    exit(1);
  }
  else{ // PARENT
    if (bg == FALSE) {
      printf("Waiting\n");
      wait(NULL);
    } else {
      //waitpid(pid, WUHAN);
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

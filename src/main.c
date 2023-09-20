/***************************************************************************//**

  @file         main.c

  @author       Stephen Brennan

  @date         Thursday,  8 January 2015

  @brief        LSH (Libstephen SHell)

*******************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
  Global variable for the prompt display.
  Easiest way I could think of for using this variable.
 */
#define HISTORY_SIZE 256
char *history[HISTORY_SIZE];
int history_index = 0;
char *prompt;

/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_prompt(char **args);
int lsh_history(char **args);
int lsh_execute(char **args);
char **lsh_split_line(char *line);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "quit",
  "prompt",
  "history"
};

char *builtin_help[] = {
    "changes directory",
    "displays this help text",
    "exits the shell",
    "alias of exit",
    "changes the prompt",
    "displays, or runs specified index"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit,
  &lsh_exit,
  &lsh_prompt,
  &lsh_history
};

int lsh_num_builtins(void) {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

/**
   @brief Builtin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

/**
   @brief Builtin command: print help.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int lsh_help(char **args)
{
  int i;
  printf("Stephen Brennan's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("\t%s: %s\n", builtin_str[i], builtin_help[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int lsh_exit(char **args)
{
  return 0;
}

/**
  @brief Built-in command to change the prompt.
  @param args List of args, args[1] is what we want.
  @return Always return 1, to continue executing.
 */
int lsh_prompt(char **args) {
  if(args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument for prompt\n");
  }
  else {
    prompt = realloc(prompt, strlen(args[1])+1);
    if(prompt == NULL) {
      fprintf(stderr, "lsh: allocation error\n");
      exit(EXIT_FAILURE);
    }
    else {
      prompt = memset(prompt, 0, strlen(args[1])+1);
      prompt = memcpy(prompt, args[1], strlen(args[1]));
      prompt[strlen(args[1])+1] = '\0';
    }
  }
  return 1;
}

/**
  @brief display history, or reexec that command
  @param args List of args, we only care about args[1]
  @return return 1, to continue execution.
 */
int lsh_history(char **args) {
    if(args[1] == NULL) {
        // just `history`, displays history
        for(int i = 0; i < history_index; i++) {
            printf("%d: %s\n", i, history[i]);
        }
    }
    else if(args[2] != NULL) {
        // more than `history 3`, complain
        fprintf(stderr, "lsh: too many arguments\n");
    }
    else {
        // DONE: execute that command again
        // TODO: ideally, have that command appear in the input buffer
        char *line = history[atoi(args[1])];
        char *line2;
        char **args2;
        int status;
        line2 = malloc(sizeof(char) * strlen(line));
        line2 = memset(line2, 0, strlen(line)+1);
        line2 = memcpy(line2, line, strlen(line)+1);
        args2 = lsh_split_line(line2);
        status = lsh_execute(args2);
        free(line2);
        return status;
    }
    return 1;
}

/**
  @brief Launch a program and wait for it to terminate.
  @param args Null terminated list of arguments (including program).
  @return Always returns 1, to continue execution.
 */
int lsh_launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("lsh");
  } else {
    // Parent process
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
 */
char *lsh_read_line(void)
{
#ifdef LSH_USE_STD_GETLINE
  char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us
  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      printf("\n");
      exit(EXIT_SUCCESS);  // We received an EOF
    } else  {
      perror("lsh: getline\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
#else
#define LSH_RL_BUFSIZE 1024
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    if (c == EOF) {
      printf("\n");
      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
#endif
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
   @brief Split a line into tokens (very naively).
   @param line The line.
   @return Null-terminated array of tokens.
 */
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		free(tokens_backup);
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

/**
   @brief Loop getting input and executing it.
 */
void lsh_loop(void)
{
  char *line;
  char **args;
  int status;
  prompt = malloc(sizeof(char) * 5);
  memcpy(prompt, "lsh>", 5);
  do {
    printf("%s ", prompt);
    line = lsh_read_line();

    history[history_index] = malloc(sizeof(char) * strlen(line)+1);
    history[history_index] = memset(history[history_index], 0, strlen(line)+1);
    history[history_index] = memcpy(history[history_index], line, strlen(line)+1);
    history_index++;

    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
  free(prompt);
  for(int i = 0; i < history_index; i++) {
      free(history[i]);
  }
}

/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
 */
int main(int argc, char **argv)
{
  // Load config files, if any.

  // Run command loop.
  lsh_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}


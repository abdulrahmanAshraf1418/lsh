/******************************************************************************
 *   Simple Unix Shell Project
 *   Custom Implementation for Operating Systems Project
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024
#define TOKEN_BUFFER 64
#define TOKEN_DELIMITERS " \t\r\n\a"
#define HISTORY_SIZE 50

/* ========================= FUNCTION DECLARATIONS ========================= */

int shell_cd(char **args);
int shell_help(char **args);
int shell_exit(char **args);
int shell_pwd(char **args);
int shell_echo(char **args);
int shell_history(char **args);
int shell_export(char **args);

char *read_command_line(void);
char **split_command(char *line);
int execute_command(char **args);
int launch_program(char **args);
void shell_loop(void);

/* ========================= HISTORY STORAGE ========================= */

char *history_list[HISTORY_SIZE];
int history_counter = 0;

/* ========================= BUILTIN STRUCT ========================= */

typedef struct {
  char *command;
  int (*function)(char **args);
} builtin_command;

/* ========================= BUILTIN COMMANDS ========================= */

builtin_command commands[] = {
  {"cd", shell_cd},
  {"help", shell_help},
  {"exit", shell_exit},
  {"pwd", shell_pwd},
  {"echo", shell_echo},
  {"history", shell_history},
  {"export", shell_export}
};

int number_of_commands() {
  return sizeof(commands) / sizeof(builtin_command);
}

/* ========================= BUILTIN IMPLEMENTATIONS ========================= */

/* Change Directory */
int shell_cd(char **args) {

  if (args[1] == NULL) {
    fprintf(stderr, "shell: expected directory path\n");
  }
  else {
    if (chdir(args[1]) != 0) {
      perror("shell");
    }
  }

  return 1;
}

/* Help Command */
int shell_help(char **args) {

  int i;

  printf("========== Simple Unix Shell ==========\n");
  printf("Built-in Commands:\n");

  for (i = 0; i < number_of_commands(); i++) {
    printf(" - %s\n", commands[i].command);
  }

  printf("You can also run Linux system commands.\n");

  return 1;
}

/* Exit Command */
int shell_exit(char **args) {
  return 0;
}

/* Print Current Directory */
int shell_pwd(char **args) {

  char current_directory[BUFFER_SIZE];

  if (getcwd(current_directory, sizeof(current_directory)) != NULL) {
    printf("%s\n", current_directory);
  }
  else {
    perror("pwd error");
  }

  return 1;
}

/* Echo Command */
int shell_echo(char **args) {

  int i = 1;

  if (args[1] == NULL) {
    printf("\n");
    return 1;
  }

  while (args[i] != NULL) {

    printf("%s", args[i]);

    if (args[i + 1] != NULL) {
      printf(" ");
    }

    i++;
  }

  printf("\n");

  return 1;
}

/* Command History */
int shell_history(char **args) {

  int start = 0;

  if (history_counter > HISTORY_SIZE) {
    start = history_counter - HISTORY_SIZE;
  }

  for (int i = start; i < history_counter; i++) {

    printf("%d  %s\n",
           i + 1,
           history_list[i % HISTORY_SIZE]);
  }

  return 1;
}

/* Export Environment Variable */
int shell_export(char **args) {

  if (args[1] == NULL) {
    fprintf(stderr, "shell: expected VARIABLE=VALUE\n");
    return 1;
  }

  char *variable = strtok(args[1], "=");
  char *value = strtok(NULL, "=");

  if (variable == NULL || value == NULL) {
    fprintf(stderr, "shell: invalid format\n");
    return 1;
  }

  if (setenv(variable, value, 1) != 0) {
    perror("shell");
  }

  return 1;
}

/* ========================= PROCESS LAUNCHER ========================= */

int launch_program(char **args) {

  pid_t pid;
  int status;

  pid = fork();

  if (pid == 0) {

    /* Child Process */

    if (execvp(args[0], args) == -1) {
      perror("shell");
    }

    exit(EXIT_FAILURE);
  }

  else if (pid < 0) {

    /* Fork Failed */

    perror("shell");
  }

  else {

    /* Parent Process */

    do {
      waitpid(pid, &status, WUNTRACED);
    }
    while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/* ========================= EXECUTE COMMAND ========================= */

int execute_command(char **args) {

  int i;

  if (args[0] == NULL) {
    return 1;
  }

  for (i = 0; i < number_of_commands(); i++) {

    if (strcmp(args[0], commands[i].command) == 0) {
      return commands[i].function(args);
    }
  }

  return launch_program(args);
}

/* ========================= READ INPUT ========================= */

char *read_command_line(void) {

  int position = 0;
  int buffer_size = BUFFER_SIZE;

  char *buffer = malloc(sizeof(char) * buffer_size);

  int character;

  if (!buffer) {
    fprintf(stderr, "allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {

    character = getchar();

    if (character == EOF) {
      exit(EXIT_SUCCESS);
    }

    else if (character == '\n') {
      buffer[position] = '\0';
      return buffer;
    }

    else {
      buffer[position] = character;
    }

    position++;

    if (position >= buffer_size) {

      buffer_size += BUFFER_SIZE;

      buffer = realloc(buffer, buffer_size);

      if (!buffer) {
        fprintf(stderr, "allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

/* ========================= TOKENIZE INPUT ========================= */

char **split_command(char *line) {

  int buffer_size = TOKEN_BUFFER;
  int position = 0;

  char **tokens = malloc(buffer_size * sizeof(char*));

  char *token;

  if (!tokens) {
    fprintf(stderr, "allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, TOKEN_DELIMITERS);

  while (token != NULL) {

    tokens[position] = token;
    position++;

    if (position >= buffer_size) {

      buffer_size += TOKEN_BUFFER;

      tokens = realloc(tokens, buffer_size * sizeof(char*));

      if (!tokens) {
        fprintf(stderr, "allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, TOKEN_DELIMITERS);
  }

  tokens[position] = NULL;

  return tokens;
}

/* ========================= MAIN SHELL LOOP ========================= */

void shell_loop(void) {

  char *line;
  char **args;

  int status;

  do {

    printf("myshell$ ");

    line = read_command_line();

    /* Save History */

    if (strlen(line) > 0) {

      history_list[history_counter % HISTORY_SIZE] = strdup(line);

      history_counter++;
    }

    args = split_command(line);

    status = execute_command(args);

    free(line);
    free(args);

  } while (status);
}

/* ========================= MAIN FUNCTION ========================= */

int main(int argc, char **argv) {

  printf("Starting Simple Unix Shell...\n");

  shell_loop();

  return EXIT_SUCCESS;
}

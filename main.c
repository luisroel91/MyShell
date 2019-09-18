#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define BUFSIZE 1024
#define MYSHELL_TOKEN_BUFFERSIZE 64
// Space, Tab, CR, Newline, BEL
#define MYSHELL_TOKEN_DELIMITER " \t\r\n\a"

char *myshell_read_line(void)
{
	int buffersize = BUFSIZE;
	int position = 0;
	char *buffer = malloc(sizeof(char) * buffersize);
	// We need to store the character as an int because
	// EOF is an int, not a char
	int character;

	if (!buffer)
	{
		fprintf(stderr, "myshell: buffer allocation error\n");
		exit(EXIT_FAILURE);
	}

	while (1)
	{
		// Read character
		character = getchar();

		// If we hit the end or newline,
		// replace with nullterm and return
		// Otherwise, current position in buffer == character
		// meaning, add the character to the buffer
		if (character == EOF || character == '\n')
		{
			buffer[position] = '\0';
			return (buffer);
		}
		else
		{
			buffer[position] = character;
		}
		position++;

		// If our buffer gets to be too small
		// (position is greater than buffersize),
		// allocate more memory
		if (position >= buffersize)
		{
			// Add another 1024 to the buffersize
			buffersize += BUFSIZE;
			buffer = realloc(buffer, buffersize);
			// If there's an error expanding the buffer then
			// print the error to stderr and exit with EXIT_FAILURE
			if (!buffer)
			{
				fprintf(stderr, "myshell: buffer re-allocation error \n");
				exit(EXIT_FAILURE);
			}
		}
	}
}

char **myshell_split_line(char *line)
{
	int buffersize = MYSHELL_TOKEN_BUFFERSIZE, position = 0;
	char **tokens = malloc(buffersize * sizeof(char *));
	char *token;

	if (!tokens)
	{
		fprintf(stderr, "myshell: tokens allocation error\n");
		exit(EXIT_FAILURE);
	}
	// We use strtok to split the string by the delimiters we've
	// defined at the beginning of the file. Strtok returns a pointer
	// to the first token it finds and replaces the delimeter with
	// a null terminator.
	// We then use a loop to get the rest of the null termed pointers
	token = strtok(line, MYSHELL_TOKEN_DELIMITER);
	while (token != NULL)
	{
		// At the current index, add the token pointer to our tokens
		// array and increase the index
		tokens[position] = token;
		position++;
		// Expand the buffer just like in read_line
		if (position >= buffersize)
		{
			buffersize += MYSHELL_TOKEN_BUFFERSIZE;
			tokens = realloc(tokens, buffersize * sizeof(char *));
			if (!tokens)
			{
				fprintf(stderr, "myshell: token buffer re-allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
		// When we have all the tokens, add a null term to the end
		// of the list
		token = strtok(NULL, MYSHELL_TOKEN_DELIMITER);
	}
	tokens[position] = NULL;
	return (tokens);
}

// In unix there are only two real ways to start a process, exec()
// and fork(), fork() returns 0 to the child process and the PID
// of the child to the parent. Exec() on the other hand replaces the
// the current running process with a completely new one. A common
// pattern is to fork the program, and then use exec() in the child
// to replace itself with a new program.

int myshell_launch(char **arguments)
{
	// Notice we declare with type pid_t
	pid_t processid, awaitprocessid;
	int status;
	// Fork our process, once fork returns we have 2 procs
	// Fork gives our child the processid of zero, which is
	// why we use the conditional below
	processid = fork();
	if (processid == 0)
	{
		// Execvp's first arg is the program we want to run
		// and then the arguments we want to run it with.
		// If it returns -1 (well, technically, it returning anything
		// would mean that there was an error) print the system error
		if (execvp(arguments[0], arguments) == -1)
		{
			perror("myshell");
		}
		exit(EXIT_FAILURE);
		// Detect/Handle forking error
	}
	else if (processid < 0)
	{
		perror("myshell");
	}
	// Continue running parent proc
	else
	{
		// WIFEXITED is a macro that returns a non-zero value if the
		// child proc is terminated normally with exit
		// WIFSIGNALED is a macro that returns a non-zero value
		// if the process was terminated because it received a POSIX
		// signal that wasn't handled properly
		do
		{
			// Until the child is termed, the parent waits for a state
			// change using waitpid. Waitpid takes in the pid (processid)
			// a pointer to the status and options. WUNTRACED in this
			// case is an option macro that tells waitpid to return
			// the status of terminated or stopped children
			awaitprocessid = waitpid(processid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	// After the child is termed, we then return one to tell the
	// program we're ready to accept more input
	return (1);
}

// Prototype built in shell functions
int myshell_cd(char **arguments);
int myshell_help(char **arguments);
int myshell_exit(char **arguments);

// Array that holds a list of the built-in commands
char *builtin_commands_array[] =
	{
		"cd",
		"help",
		"exit"};

// Array of function pointers to the built-in commands
// that take an array of strings and return an int
int (*builtin_commands_functions[])(char **) = {
	&myshell_cd,
	&myshell_help,
	&myshell_exit};

// Calculate number of built-in commands
// To do that, we get the size of commands array
// and divide that by the size of a pointer to char
int myshell_number_of_builtins()
{
	return sizeof(builtin_commands_array) / sizeof(char *);
};

// Here we implement cd (change directory), if there is no argument
// (path), then we print the system error. Otherwise, we use chdir
// check for errors and then return
int myshell_cd(char **arguments)
{
	if (arguments[1] == NULL)
	{
		fprintf(stderr, "MyShell: no argument to cd\n");
	}
	else
	{
		if (chdir(arguments[1]) != 0)
		{
			perror("MyShell");
		}
	}
	return (1);
}

int myshell_help(char **arguments)
{
	int counter;
	printf("MyShell is your shell\n\n");
	printf("Type what you'd like to run and press enter.\n\n");
	printf("These are the built-in commands:\n");

	// Iterate through our builtin_commands_array and list them out
	// Notice how we use myshell_number_of_builtins() so our conditional
	// knows where to stop
	for (counter = 0; counter < myshell_number_of_builtins(); counter++)
	{
		printf("  %s\n", builtin_commands_array[counter]);
	}
	printf("\nCommon UNIX commands such as ls, rm, etc also work.\n\n");
	printf("Piping, redirection, autocompletion and globbing do not.\n");
	printf("Only whitespace seperated arguments please.\n\n");
	return (1);
}

// Just like in most C programs, return 0 typically means exit.
// Ours is no different. This ends our event loop.
int myshell_exit(char **arguments)
{
	return 0;
}

int myshell_execute(char **arguments)
{
	int counter;
	// If there are no arguments, return 1
	if (arguments[0] == NULL)
	{
		return 1;
	}
	// If there are arguments, loop through the built-ins
	// and then use strcmp (strcmp returns 0 if the comparison finds no difference)
	// to see if anything the user entered matches,
	for (counter = 0; counter < myshell_number_of_builtins(); counter++)
	{
		if (strcmp(arguments[0], builtin_commands_array[counter]) == 0)
		{
			return ((*builtin_commands_functions[counter])(arguments));
		}
	}
	return (myshell_launch(arguments));
}

void myshell_loop(void)
{
	char *line;
	char **arguments;
	int status;

	do
	{
		printf("MyShell> ");
		line = myshell_read_line();
		arguments = myshell_split_line(line);
		status = myshell_execute(arguments);

		free(line);
		free(arguments);
	} while (status);
}

int main(int argc, char **argv)
{
	myshell_loop();

	return EXIT_SUCCESS;
}

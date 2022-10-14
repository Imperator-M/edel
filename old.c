#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define CMDLINE_MAX 512
#define TOKEN_LENGTH 32
#define OUTPUT 1
#define INPUT 2

struct userInput{
    char* command;
    char *arguments[16];
    int index;

    /// the name of file being written into when redirection happens, its possible
    /// that no redirection happens so we set it to NULL
    char* redirectionFile;
};

/// initialize char pointer array to all NULL values
void initializeArrayArguments(struct userInput * obj){
    for(int i = 0; i < 16; i++){
        obj->arguments[i] = NULL;
    }
}
///inititalize redirection to NULL in case user does no redirection
void initializeRedirectFile(struct userInput * obj){
    obj->redirectionFile = NULL;

}

/// use to parse user input with no piping or redirection
struct userInput *normalParse(char cmd[CMDLINE_MAX]){
    /// create new object and initialize its array and redirection file
    struct userInput *newUserCommand = (struct userInput*)malloc(sizeof(struct userInput));
    initializeArrayArguments(newUserCommand);
    initializeRedirectFile(newUserCommand);
    newUserCommand->index = 0;

    /// we will use spaces to split the string
    char breakString[2] = " ";
    char* individualString;

    /// iterate over each string using strtok
    individualString = strtok(cmd, breakString);

    while(individualString != NULL){
        /// the first string will always be the command and first
        /// argument
        if(newUserCommand->index == 0){
            newUserCommand->command = individualString;
            newUserCommand->arguments[newUserCommand->index] = individualString;
            newUserCommand->index++;
        }
        /// the other strings are the arguments
        else{
            newUserCommand->arguments[newUserCommand->index] = individualString;
            newUserCommand->index++;
        }
        individualString = strtok(NULL, breakString);
    }
    /// return new object with its command and arguments
    return newUserCommand;
}

/// check if an outputRedirection exist
int checkForRedirection(char cmd[CMDLINE_MAX]){
    /// if we see a redirection in input then return true
    for(size_t i = 0; i < strlen(cmd); i++){
        if(cmd[i] == '>'){
            return OUTPUT;
        }
        else if(cmd[i] == '<'){
            return INPUT;
        }
    }
    /// else return false;
    return 0;
}

void parse1(struct userInput* obj, char* string){
    char s[2] = " ";
    char* token;
    token = strtok(string, s);

    while(token != NULL){
        if(obj->index == 0){
            obj->command = token;
            obj->arguments[obj->index] = token;
            obj->index++;
        }else{
            obj->arguments[obj->index] = token;
            obj->index++;
        }
        token = strtok(NULL, s);
    }
}

void parse2(struct userInput* obj, char* string){
    int count = 0;
    char s[2] = " ";
    char* token;
    token = strtok(string, s);

    while(token != NULL){
        if(count == 0){
            obj->redirectionFile = token;
        }else{
            obj->arguments[obj->index] = token;
            obj->index++;
        }
        token = strtok(NULL, s);
        count++;
    }
}

int checkIfFileExist(char* redirectionFile){
    struct stat sb;
    if(stat(redirectionFile, &sb) == 0){
        return 1;
    }
    return 0;
}

void forkRedirection(struct userInput* newUserCommand, char cmd[CMDLINE_MAX], int redirection){
    int inputRedirectionError = 0;
    pid_t pid;
    pid = fork();

    /// parent
    if(pid != 0){
        int status;
        waitpid(pid, &status, 0);
        ///could not open file
        if(inputRedirectionError == 1 && WEXITSTATUS(status)){
            fprintf(stderr, "Error: cannot open input file\n");
        }
        if(WEXITSTATUS(status) == 1){
            fprintf(stderr, "Error: cannot open input file\n");
        }else{
            fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(status));
        }

    }
    else{
        if(redirection == OUTPUT){
            int fd;

            /// create a file by the name given by the user
            fd = open(newUserCommand->redirectionFile, O_WRONLY| O_CREAT, 0664| O_TRUNC);

            /// dup2 makes it so what ever is printed instead goes to the file
            /// so we basically made the file descriptor that prints to the screen point
            /// to the file
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        else if(redirection == INPUT){
            int fd2;
            
            if(checkIfFileExist(newUserCommand->redirectionFile) == 0){
                inputRedirectionError = 1;
                exit(EXIT_FAILURE);
            }

            fd2 = open(newUserCommand->redirectionFile, O_RDONLY);
            dup2(fd2, STDIN_FILENO);
            close(fd2);
        }
        /// we then run the command and instead of printing to the terminal we write to the file
        /// so for echo hello > file , hello will not be printed, instead will be written to file
        execvp(newUserCommand->command, newUserCommand->arguments);
        exit(0);
    }
}

void forkProcessRedirection(char originalInput[CMDLINE_MAX], char cmd[CMDLINE_MAX], int redirection){
    char holdStringToMakeWork[CMDLINE_MAX];     //C throws error if variable not used
    strcpy(holdStringToMakeWork, originalInput);//So I made this so it was used###################################################################

    if(redirection == OUTPUT){
        char *copyCommand = calloc(strlen(cmd)+1, sizeof(char));

        strcpy(copyCommand, cmd);

        char s[2] = ">";
        char* individualString;
        individualString = strtok(copyCommand, s);

        char *part1Process = calloc(strlen(individualString)+1, sizeof(char));
        strcpy(part1Process, individualString);

        individualString = strtok(NULL, s);

        /// when we have a redirection we expect there two be two parts
        /// but since we got null on this token we have a missing command
        if(individualString == NULL){
            fprintf(stderr, "Error: missing command\n");
            return;
        }

        char *part2Process = calloc(strlen(individualString)+1, sizeof(char));
        strcpy(part2Process, individualString);

        struct userInput* newUserCommand = (struct userInput*)malloc(sizeof(struct userInput));
        initializeArrayArguments(newUserCommand);
        initializeRedirectFile(newUserCommand);
        newUserCommand->index = 0;

        parse1(newUserCommand, part1Process);

        parse2(newUserCommand, part2Process);

        forkRedirection(newUserCommand, cmd, redirection);
    }
    else if(redirection == INPUT){
        char *copyCommand = calloc(strlen(cmd)+1, sizeof(char));

        strcpy(copyCommand, cmd);

        char s[2] = "<";
        char* individualString;
        individualString = strtok(copyCommand, s);

        char *part1Process = calloc(strlen(individualString)+1, sizeof(char));
        strcpy(part1Process, individualString);

        individualString = strtok(NULL, s);

        /// when we have a redirection we expect there two be two parts
        /// but since we got null on this token we have a missing command
        if(individualString == NULL){
            fprintf(stderr, "Error: missing command\n");
            return;
        }

        char *part2Process = calloc(strlen(individualString)+1, sizeof(char));
        strcpy(part2Process, individualString);

        struct userInput* newUserCommand = (struct userInput*)malloc(sizeof(struct userInput));
        initializeArrayArguments(newUserCommand);
        initializeRedirectFile(newUserCommand);
        newUserCommand->index = 0;

        parse1(newUserCommand, part1Process);

        parse2(newUserCommand, part2Process);

        forkRedirection(newUserCommand, cmd, redirection);
    }
}

void forkProcess(char originalInput[CMDLINE_MAX], struct userInput* newUserCommand){
    pid_t pid;
    pid = fork();

    if(pid != 0){
        int status;
        waitpid(pid, &status, 0);
        if(WEXITSTATUS(status) == 1){
            fprintf(stderr, "Error: command not found\n");
        }
        fprintf(stderr, "+ completed '%s' [%d]\n", originalInput, WEXITSTATUS(status));
    }
    else{
        if(execvp(newUserCommand->command, newUserCommand->arguments) == -1){
            exit(EXIT_FAILURE);
        }
        exit(0);
    }
}

/// this will only take care of the input with no piping
void runCommand(char cmd[CMDLINE_MAX]){
    char copyCmdForPrintingMessage[CMDLINE_MAX];
    strncpy(copyCmdForPrintingMessage, cmd, CMDLINE_MAX);

    /// here will will need to check for any errors

    /// first lets go through the string and detect if there is a redirection
    if(checkForRedirection(cmd) == 1){
        /// if it does contain redirection then use strtok to break into 2 parts
        forkProcessRedirection(copyCmdForPrintingMessage, cmd, 1);
    }
    else if(checkForRedirection(cmd) == 2){
        forkProcessRedirection(copyCmdForPrintingMessage, cmd, 2);
    }else{
        struct userInput *newUserCommand = normalParse(cmd);
        forkProcess(copyCmdForPrintingMessage, newUserCommand);
    }
}

int checkForPiping(char cmd[CMDLINE_MAX], int* numberOfPipes){
    int i;
    int containsPipe = 0;
    for(i = 0; cmd[i]; i++){
        if(cmd[i] == '|'){
            (*numberOfPipes)++;
            containsPipe = 1;
        }
    }
    return containsPipe;
}

/// forking for one pipe
void forkIntoPipe(char cmd[CMDLINE_MAX], struct userInput* process1, struct userInput* process2,
        int inputRedirection, int outputRedirection){
    int fd[2];
    pid_t pid;
    pid_t pid2;
    /// set up pipe
    pipe(fd);
    pid = fork();


    if(pid == 0){
        if(inputRedirection == OUTPUT && process1->redirectionFile){
            exit(EXIT_FAILURE);
        }
        else if(inputRedirection == INPUT && process1->redirectionFile){
            int file;
            file = open(process1->redirectionFile, O_RDONLY);
            dup2(file, STDIN_FILENO);
            close(file);
        }
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);

        execvp(process1->command, process1->arguments);
        /// if for some reason the function that calls this one
        /// fails to detect an output redirection in first process
        /// then we make an exit failure and stop everything
    }
    pid2 = fork();
    if(pid2 == 0){
        /// if process 1 has a redirection file we return exit failure
        /// because it is an error
        if(inputRedirection == OUTPUT && process1->redirectionFile){
            exit(EXIT_FAILURE);
        }else{
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);

            /// the case where there is redirection in last process
            /// we will need to have STDOUT point to the file Name
            /// where the we had it which was int file
            if(outputRedirection == OUTPUT && process2->redirectionFile){
                int file;
                /// create a file by the name given by the user
                file = open(process2->redirectionFile, O_WRONLY| O_CREAT, 0664| O_TRUNC);

                /// dup2 makes it so what ever is printed instead goes to the file
                /// so we basically made the file descriptor that prints to the screen point
                /// to the file
                dup2(file, STDOUT_FILENO);
                close(file);
            }
            execvp(process2->command, process2->arguments);
        }
    }
    close(fd[0]);
    close(fd[1]);

    int status1;
    waitpid(pid, &status1, 0);

    int status2;
    waitpid(pid2, &status2, 0);

    /// if we get an exit failure it has to do with the
    /// output redirection failure so we return an error message
    if(WEXITSTATUS(status2) == 1){
        fprintf(stderr, "Error: mislocated output redirection\n");
    }
    /// otherwise return the completed message and executed pipe
    else{
        fprintf(stderr, "+ completed '%s' [%d] [%d]\n", cmd, WEXITSTATUS(status1), WEXITSTATUS(status2));
    }
}

void parseRedirectionForPipe(struct userInput *process, char* input){
    char *copyInput = calloc(strlen(input)+1, sizeof(char));

    strcpy(copyInput, input);

    char s[2] = ">";
    char* individualString;
    individualString = strtok(copyInput, s);

    char *part1 = calloc(strlen(individualString)+1, sizeof(char));
    strcpy(part1, individualString);

    individualString = strtok(NULL, s);

    char *part2 = calloc(strlen(individualString)+1, sizeof(char));
    strcpy(part2, individualString);

    parse1(process, part1);
    parse2(process, part2);

    /// this function just set all the commands and arguments for object
}

void parseInputRedirectionForPipe(struct userInput *process, char* input){
    char *copyInput = calloc(strlen(input)+1, sizeof(char));

    strcpy(copyInput, input);

    char s[2] = "<";
    char* individualString;
    individualString = strtok(copyInput, s);

    char *part1 = calloc(strlen(individualString)+1, sizeof(char));
    strcpy(part1, individualString);

    individualString = strtok(NULL, s);

    char *part2 = calloc(strlen(individualString)+1, sizeof(char));
    strcpy(part2, individualString);

    parse1(process, part1);
    parse2(process, part2);
}

/// take care of input for one pipe
void onePipingCommand(char cmd[CMDLINE_MAX]){
    /// first we will need to parse and get two parts
    /// the break point is "|" symbol so we will have two processes

    /// make a copy of original
    char *copyInput = calloc(strlen(cmd)+1, sizeof(char));
    strcpy(copyInput, cmd);

    /// now we will break the process into two parts
    char breakString[2] = "|";
    char* individualString;
    individualString = strtok(copyInput, breakString);

    /// need to create copies to not modify when using strtok
    char *part1Process = calloc(strlen(individualString)+1, sizeof(char));
    strcpy(part1Process, individualString);

    individualString = strtok(NULL, breakString);

    char *part2Process = calloc(strlen(individualString)+1, sizeof(char));
    strcpy(part2Process, individualString);

    /// since we know there is one pipe we will create two objects
    /// representing process 1 and process 2
    /// we also initialize their command,arguments, and file
    struct userInput* process1 = (struct userInput*)malloc(sizeof(struct userInput));
    initializeArrayArguments(process1);
    initializeRedirectFile(process1);
    process1->index = 0;

    struct userInput* process2 = (struct userInput*)malloc(sizeof(struct userInput));
    initializeArrayArguments(process2);
    initializeRedirectFile(process2);
    process2->index = 0;

    int process1Redirection = checkForRedirection(part1Process);
    int process2Redirection = checkForRedirection(part2Process);
    /// if there is a redirection at the beginning we return an error and return
    if( process1Redirection == OUTPUT){
        /// so we will use same method as one of our functions
        //parseRedirectionForPipe(process1, part1Process);
        fprintf(stderr, "Error: mislocated output redirection\n");
        return;
    }
    else if(process1Redirection == INPUT){
        parseInputRedirectionForPipe(process1, part1Process);
    }
    /// otherwise parse the string and set up the commands and arguments
    /// of the first object which is process 2
    else{
        parse1(process1, part1Process);
    }
    /// if there is a redirection in the second part meaning
    /// in the second process we parse the string in order
    /// to set the file for object 2 which is process 2
    if(process2Redirection == OUTPUT){
        parseRedirectionForPipe(process2, part2Process);
    }
    else if(process2Redirection == INPUT){
        fprintf(stderr, "Error: mislocated input redirection\n");
        return;
    }
    /// otherwise do a normal parse and set the command and arguments
    /// of process2
    else{
        parse1(process2, part2Process);
    }
    /// after all object have their commands/arguments we fork into pipe
    forkIntoPipe(cmd, process1, process2, process1Redirection, process2Redirection);
}

void forkIntoTwoPipes(char cmd[CMDLINE_MAX], struct userInput* process1, struct userInput* process2,
        struct userInput* process3, int process1Redirection, int process2Redirection,
                int process3Redirection){
    
    int tester = process2Redirection;
    tester += 1;

    int fd[2];
    int fd2[2];
    pipe(fd);
    pipe(fd2);

    pid_t pid;
    pid = fork();

    if(pid == 0){
        if(process1Redirection == INPUT && process1->redirectionFile){
            int file;
            file = open(process1->redirectionFile, O_RDONLY);
            dup2(file, STDIN_FILENO);
            close(file);
        }

        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        execvp(process1->command, process1->arguments);
    }
    pid_t pid2;
    pid2 = fork();
    if(pid2 == 0){
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);

        close(fd2[0]);
        dup2(fd2[1], STDOUT_FILENO);
        close(fd2[1]);
        execvp(process2->command, process2->arguments);
    }
    pid_t pid3;
    pid3 = fork();
    if(pid3 == 0){
        close(fd[1]);
        close(fd[0]);

        close(fd2[1]);
        dup2(fd2[0], STDIN_FILENO);
        close(fd2[0]);

        if(process3Redirection == OUTPUT && process3->redirectionFile){
            int file;

            /// create a file by the name given by the user
            file = open(process3->redirectionFile, O_WRONLY| O_CREAT, 0664| O_TRUNC);

            /// dup2 makes it so what ever is printed instead goes to the file
            /// so we basically made the file descriptor that prints to the screen point
            /// to the file
            dup2(file, STDOUT_FILENO);
            close(file);
        }
        execvp(process3->command, process3->arguments);
    }

    close(fd[0]);
    close(fd[1]);

    close(fd2[0]);
    close(fd2[1]);

    int status1;
    waitpid(pid, &status1, 0);

    int status2;
    waitpid(pid2, &status2, 0);

    int status3;
    waitpid(pid3, &status3, 0);

    fprintf(stderr, "+ completed '%s' [%d] [%d] [%d]\n", cmd, WEXITSTATUS(status1), WEXITSTATUS(status2), WEXITSTATUS(status3));
}

void twoPiping(char cmd[CMDLINE_MAX]){
    char *copyInput = calloc(strlen(cmd)+1, sizeof(char));
    strcpy(copyInput, cmd);

    /// now we will break into three processes
    char s[2] = "|";
    char* individualString;
    individualString = strtok(copyInput, s);

    char *part1Process = calloc(strlen(individualString)+1, sizeof(char));

    strcpy(part1Process, individualString);

    individualString = strtok(NULL, s);

    char *part2Process = calloc(strlen(individualString)+1, sizeof(char));
    strcpy(part2Process, individualString);

    individualString = strtok(NULL, s);

    char *part3Process = calloc(strlen(individualString)+1, sizeof(char));
    strcpy(part3Process, individualString);

    struct userInput* process1 = (struct userInput*)malloc(sizeof(struct userInput));
    initializeArrayArguments(process1);
    initializeRedirectFile(process1);
    process1->index = 0;

    struct userInput* process2 = (struct userInput*)malloc(sizeof(struct userInput));
    initializeArrayArguments(process2);
    initializeRedirectFile(process2);
    process2->index = 0;

    struct userInput* process3 = (struct userInput*)malloc(sizeof(struct userInput));
    initializeArrayArguments(process3);
    initializeRedirectFile(process3);
    process3->index = 0;

    int process1Redirection = checkForRedirection(part1Process);
    int process2Redirection = checkForRedirection(part2Process);
    int process3Redirection = checkForRedirection(part3Process);
    /// input redirection <
    if(process1Redirection == INPUT){
        parseInputRedirectionForPipe(process1, part1Process);
    }
    else if(process1Redirection == OUTPUT){
        fprintf(stderr, "Error: mislocated output redirection\n");
        return;
    }
    else{
        parse1(process1, part1Process);
    }
    if(process2Redirection == INPUT){
        fprintf(stderr, "Error: mislocated input redirection\n");
        return;
    }
    //parse1(process1, part1Process);
    else if(process2Redirection == OUTPUT){
        fprintf(stderr, "Error: mislocated output redirection\n");
        return;
    }
    else{
        parse1(process2, part2Process);
    }
    //parse1(process2, part2Process);
    if(process3Redirection == INPUT){
        fprintf(stderr, "Error: mislocated input redirection\n");
        return;
    }
    else if(process3Redirection == OUTPUT){
        parseRedirectionForPipe(process3, part3Process);
    }
    else{
        parse1(process3, part3Process);
    }
    //parse1(process3, part3Process);

    forkIntoTwoPipes(cmd, process1, process2, process3, process1Redirection, process2Redirection,
                     process3Redirection);
}

void forkIntoThreePipes(char cmd[CMDLINE_MAX], struct userInput* process1, struct userInput* process2,
                        struct userInput* process3, struct userInput* process4,
                                int process1Redirection, int process4Redirection){
    int fd[2];
    int fd2[2];
    int fd3[2];
    pipe(fd);
    pipe(fd2);
    pipe(fd3);

    pid_t pid;
    pid = fork();

    if(pid == 0){
        if(process1Redirection == INPUT && process1->redirectionFile){
            int file;
            file = open(process1->redirectionFile, O_RDONLY);
            dup2(file, STDIN_FILENO);
            close(file);
        }
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        execvp(process1->command, process1->arguments);
    }
    pid_t pid2;
    pid2 = fork();
    if(pid2 == 0){
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);

        close(fd2[0]);
        dup2(fd2[1], STDOUT_FILENO);
        close(fd2[1]);
        execvp(process2->command, process2->arguments);
    }
    pid_t pid3;
    pid3 = fork();
    if(pid3 == 0){
        close(fd[1]);
        close(fd[0]);

        close(fd2[1]);
        dup2(fd2[0], STDIN_FILENO);
        close(fd2[0]);

        close(fd3[0]);
        dup2(fd3[1], STDOUT_FILENO);
        close(fd3[1]);

        execvp(process3->command, process3->arguments);
    }

    pid_t pid4;
    pid4 = fork();
    if(pid4 == 0){
        close(fd[0]);
        close(fd[1]);

        close(fd2[0]);
        close(fd2[1]);

        close(fd3[1]);
        dup2(fd3[0], STDIN_FILENO);
        close(fd3[0]);

        if(process4Redirection == OUTPUT && process4->redirectionFile){
            int file;
            /// create a file by the name given by the user
            file = open(process4->redirectionFile, O_WRONLY| O_CREAT, 0664| O_TRUNC);

            /// dup2 makes it so what ever is printed instead goes to the file
            /// so we basically made the file descriptor that prints to the screen point
            /// to the file
            dup2(file, STDOUT_FILENO);
            close(file);
        }
        execvp(process4->command, process4->arguments);
    }
    close(fd[0]);
    close(fd[1]);

    close(fd2[0]);
    close(fd2[1]);

    close(fd3[0]);
    close(fd3[1]);

    int status1;
    waitpid(pid, &status1, 0);

    int status2;
    waitpid(pid2, &status2, 0);

    int status3;
    waitpid(pid3, &status3, 0);

    int status4;
    waitpid(pid4, &status4, 0);

    fprintf(stderr, "+ completed '%s' [%d] [%d] [%d] [%d]\n", cmd, WEXITSTATUS(status1),
            WEXITSTATUS(status2), WEXITSTATUS(status3), WEXITSTATUS(status4));
}

void threePiping(char cmd[CMDLINE_MAX]){
    char *copyInput = calloc(strlen(cmd)+1, sizeof(char));
    strcpy(copyInput, cmd);

    /// now we will break the process into two parts
    char s[2] = "|";
    char* individualString;
    individualString = strtok(copyInput, s);

    char *part1Process = calloc(strlen(individualString)+1, sizeof(char));

    strcpy(part1Process, individualString);

    individualString = strtok(NULL, s);

    char *part2Process = calloc(strlen(individualString)+1, sizeof(char));
    strcpy(part2Process, individualString);

    individualString = strtok(NULL, s);

    char *part3Process = calloc(strlen(individualString)+1, sizeof(char));
    strcpy(part3Process, individualString);

    individualString = strtok(NULL, s);

    char *part4Process = calloc(strlen(individualString)+1, sizeof(char));
    strcpy(part4Process, individualString);

    struct userInput* process1 = (struct userInput*)malloc(sizeof(struct userInput));
    initializeArrayArguments(process1);
    initializeRedirectFile(process1);
    process1->index = 0;

    struct userInput* process2 = (struct userInput*)malloc(sizeof(struct userInput));
    initializeArrayArguments(process2);
    initializeRedirectFile(process2);
    process2->index = 0;

    struct userInput* process3 = (struct userInput*)malloc(sizeof(struct userInput));
    initializeArrayArguments(process3);
    initializeRedirectFile(process3);
    process3->index = 0;

    struct userInput* process4 = (struct userInput*)malloc(sizeof(struct userInput));
    initializeArrayArguments(process4);
    initializeRedirectFile(process4);
    process4->index = 0;

    int process1Redirection = checkForRedirection(part1Process);
    int process2Redirection = checkForRedirection(part2Process);
    int process3Redirection = checkForRedirection(part3Process);
    int process4Redirection = checkForRedirection(part4Process);

    if(process1Redirection == INPUT){
        parseInputRedirectionForPipe(process1, part1Process);
    }
    else if(process1Redirection == OUTPUT){
        fprintf(stderr, "Error: mislocated output redirection\n");
        return;
    }
    else{
        parse1(process1, part1Process);
    }
    if(process2Redirection == INPUT){
        fprintf(stderr, "Error: mislocated input redirection\n");
        return;
    }
    //parse1(process1, part1Process);
    else if(process2Redirection == OUTPUT){
        fprintf(stderr, "Error: mislocated output redirection\n");
        return;
    }
    else{
        parse1(process2, part2Process);
    }
    if(process3Redirection == INPUT){
        fprintf(stderr, "Error: mislocated input redirection\n");
        return;
    }
    //parse1(process2, part2Process);
    else if(process3Redirection == OUTPUT){
        fprintf(stderr, "Error: mislocated output redirection\n");
        return;
    }
    else{
        parse1(process3, part3Process);
    }
    if(process4Redirection == INPUT){
        fprintf(stderr, "Error: mislocated input redirection\n");
        return;
    }
    //parse1(process3, part3Process);
    else if(process4Redirection == OUTPUT){
        parseRedirectionForPipe(process4, part4Process);
    }
    else{
        parse1(process4, part4Process);
    }
    //parse1(process4, part4Process);

    forkIntoThreePipes(cmd, process1, process2, process3, process4, process1Redirection,
                       process4Redirection);

    free(part1Process);
    free(part2Process);
    free(part3Process);
    free(part4Process);
    free(process1);
    free(process2);
    free(process3);
    free(process4);
}

void removeAllWhiteSpaces(char* copyOfUserInput){
    size_t j = 0;
    size_t i = 0;

    while(copyOfUserInput[i]){
        if(copyOfUserInput[i] != ' '){
            copyOfUserInput[j] = copyOfUserInput[i];
            j++;
        }
        i++;
    }
    copyOfUserInput[j] = '\0';
}

int checkForMissingCommandAtBeginning(char* copyOfUserInput){
    /// first case where
    /// remove all spaces from userInout
    removeAllWhiteSpaces(copyOfUserInput);
    size_t size = strlen(copyOfUserInput);
    for(size_t i = 0; i < size; i++){
        /// ex: > file
        /// ex: | grep hi
        if((copyOfUserInput[i] == '>' || copyOfUserInput[i] == '|') && i == 0){
            return 1;
        }
        /// ex: ls |  | grep hello
        if(copyOfUserInput[i] == '|' && copyOfUserInput[i+1] == '|'){
            return 1;
        }
        /// ex: ls | grep hello |
        if(copyOfUserInput[i] == '|' && i + 1 == size){
            return 1;
        }
    }
    return 0;
}

int checkForMissingOutputFile(char* copyOfUserInput){
    removeAllWhiteSpaces(copyOfUserInput);
    size_t size = strlen(copyOfUserInput);
    for(size_t i = 0; i < size; i++){
        if(copyOfUserInput[i] == '>' && i+1 == size){
            return 1;
        }
    }
    return 0;
}

int checkForMissingInputFile(char *copyOfUserInput){
    removeAllWhiteSpaces(copyOfUserInput);
    size_t size = strlen(copyOfUserInput);
    for(size_t i = 0; i < size; i++){
        if(copyOfUserInput[i] == '<' && i+1 == size)
            return 1;
    }
    return 0;
}

int checkForTooManyArguments(char* copyOfUserInput){
    int count = 0;
    char s[2] = " ";

    char* arguments;
    arguments = strtok(copyOfUserInput, s);

    while(arguments != NULL){
        count++;
        arguments = strtok(NULL, s);
    }
    if(count > 16)
        return 1;
    else
        return 0;
}

/* struct to hold the linked list data including a pointer to the next item in
the stack. Data holds the directory character array and top holds the top of 
the stack */
struct node
{
    char data[CMDLINE_MAX];
    struct node *next;
};
//initialize top to NULL as is standard of pointers
struct node *top = NULL; 

void push(){
    //Declare character array and a character new_top to hold the current
    //directory from getcwd. Then allocate memory for
    //Then use strcpy to store the new_top to the array in our stack
    //using getcwd so we create a copy of the cmd to be used instead
    char cwd[CMDLINE_MAX];
    char *cwd2;
    struct node *new_top;
    new_top = (struct node*)malloc(sizeof(struct node));

    cwd2 = getcwd(cwd, CMDLINE_MAX);
    strcpy(new_top -> data, cwd2);

    //if top == NULL then first item in stack, set next to null
    //else set top of stack to new item and set new_top back to top
    if(top == NULL){
        new_top -> next = NULL;
        }
    else{
        new_top -> next = top;
        }
    top = new_top;
}

int pop(){
    //initilize array to hold top of array we will pop from the stack
    //We need this so we can change directory with chdir()
    char item[CMDLINE_MAX];
    struct node *remove_item;
    int status = 0;
    //set remove_item equal to top of stack.
    //Set new top tb be the next item in the stack.
    //Free the stacks memory
    if (top == NULL){
        fprintf(stderr, "Error: directory stack empty\n");
        status = 1;
    }
    else{
        remove_item = top;
        strncpy(item, top -> data, CMDLINE_MAX);
        top = top -> next;
        free(remove_item);
        chdir(item);
    }
    return (status);
}

int dirs(){
    //Set current_item to top of the stack and use a while loop to traverse
    //each node.
    struct node *current_item;
    int status = 0;
    current_item = top;

    while(current_item != NULL)
    {
        printf("%s\n", current_item -> data);
        current_item = current_item -> next;

    }
    return (status);
}

void runCd(char cmd[CMDLINE_MAX]) {
    //initilize array to hold path for chdir()
    //use parser to parse user input
    //*command for outputting what command was used
    //status of command successful by default change to 1 if invalid
    char cmd2[CMDLINE_MAX];
    strcpy(cmd2, cmd);
    char *command = cmd2;
    int status = 0;

    char cwd[CMDLINE_MAX];
    getcwd(cwd, sizeof(cwd)); //gets current directory
    struct userInput *newUserCommand = normalParse(cmd);

    //If the users first command is cd then run cd command
    //look at the second argument to decide command.
    //if neither "." or ".." then check if its a valid command
    if (!strcmp(newUserCommand->command, "cd")){
        if (chdir(newUserCommand->arguments[1]) != 0){
            fprintf(stderr, "Error: cannot cd into directory\n");
            status = 1;
        }
        else{
            if (!strcmp(newUserCommand->arguments[1], ".")){
                chdir(cwd);
            }
            else if (!strcmp(newUserCommand->arguments[1], ".."))
            {
                chdir(cwd);
                chdir("..");
            }
        }
    }
        //If not cd check if its a stack command. If pushd, then error check
        //and activate push and then change directory as the user has inputed.
        //checking for valid command changes directory, so i set it back to
        //current and then switch it back to new directory. Not elegent.
    else{
        if (!strcmp(newUserCommand->command, "dirs")){
            printf("%s\n",cwd);
            status = dirs();
        }
        else if (!strcmp(newUserCommand->command, "pushd")){
            if (chdir(newUserCommand->arguments[1]) != 0){
                fprintf(stderr, "Error: no such directory\n");
                status = 1;
            }
            else{
                chdir(cwd);
                push();
                chdir(newUserCommand->arguments[1]);
            }
        }
        else if (!strcmp(newUserCommand->command, "popd")){
            status = pop();
        }
    }
    fprintf(stderr, "+ completed '%s' [%d]\n", command, status);
}

int main(void){
    /// the command line is just a string of possible length 512
    char cmd[CMDLINE_MAX];

    /// while will keep reprompting the user for other commands once one is executed
    while (1) {
        char *nl;
        //int retval;

        /* Print prompt */
        printf("sshell$ ");
        fflush(stdout);

        /* Get command line */
        /// fgets(char* str, int n, FILE* stream)
        /// 1. str is the array of chars where the string read is stored
        /// 2. n = the maximum numbers to be read
        /// 3. FILE* stream = in this case identify where the string is read from which is from stdin
        /// aka the user input
        fgets(cmd, CMDLINE_MAX, stdin);

        /* Print command line if stdin is not provided by terminal */
        if (!isatty(STDIN_FILENO)) {
            printf("%s", cmd);
            fflush(stdout);
        }

        /* Remove trailing newline from command line */
        nl = strchr(cmd, '\n');
        if (nl)
            *nl = '\0';

        /* Builtin command */
        if (!strcmp(cmd, "exit")) {
            fprintf(stderr, "Bye...\n");
            fprintf(stderr, "+ completed '%s' [0]\n", cmd);
            break;
        }

        /// before running any commands we will need to detect
        /// error from the left most side

        /// create a copy of original user input so we don't modify it
        char *copyOfUserInput = calloc(strlen(cmd)+1, sizeof(char));
        strcpy(copyOfUserInput, cmd);

        /// missing command error
        if(checkForMissingCommandAtBeginning(copyOfUserInput)){
            fprintf(stderr, "Error: missing command\n");
            free(copyOfUserInput);
            continue;
        }

        /// missing output file error
        if(checkForMissingOutputFile(copyOfUserInput)){
            fprintf(stderr, "Error: no output file\n");
            free(copyOfUserInput);
            continue;
        }
        if(checkForMissingInputFile(copyOfUserInput)){
            fprintf(stderr, "Error: no input file\n");
            free(copyOfUserInput);
            continue;
        }
        /// create other copy of user input
        char* anotherCopy = calloc(strlen(cmd)+1, sizeof(char));
        strcpy(anotherCopy, cmd);
        
        /// too many arguments error
        if(checkForTooManyArguments(anotherCopy)){
            fprintf(stderr, "Error: too many process arguments\n");
            free(anotherCopy);
            continue;
        }
        /// if no errors free pointer created by calloc
        free(copyOfUserInput);
        free(anotherCopy);

        /*create copies of the cmd char array, this is required since getcwd()
        changes the variable passed into it. We still will need cmd for later
        functions, so this is our solution*/
        char cmd2[CMDLINE_MAX];
        strncpy(cmd2, cmd, CMDLINE_MAX);
        char cmd3[CMDLINE_MAX];
        strcpy(cmd3, cmd);

        //newCmd used to eliminate any whitespace and grab command
        struct userInput *newCmd = normalParse(cmd2);
        
        /// before running normal commands or redirection
        /// we will check for any piping commands
        int numberOfPipes = 0;
        if(checkForPiping(cmd, &numberOfPipes)){
            if(numberOfPipes == 1)
                onePipingCommand(cmd);
            if(numberOfPipes == 2){
                twoPiping(cmd);
            }
            if(numberOfPipes == 3){
                threePiping(cmd);
            }
        }
        else if (!strcmp(cmd3, "pwd")){
            getcwd(cmd3, sizeof(cmd3));
            printf("%s\n", cmd3);
            fprintf(stderr, "+ completed '%s' [0]\n", newCmd->command);

        }
        else if (!strcmp(newCmd->command, "cd")
                 || !strcmp(newCmd->command, "pushd")
                 || !strcmp(newCmd->command, "popd")
                 || !strcmp(newCmd->command, "dirs")){
            runCd(cmd);
        }
        else{
            runCommand(cmd);
        }
    }
    return EXIT_SUCCESS;
}

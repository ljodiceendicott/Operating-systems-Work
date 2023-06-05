/**
**  NAME: Luke Jodice
**
**  What is it?: Template for csc380
**               Programming assignment #2 (v1.6)
**
**  Description:
**               Gets a single line from the user and
**               runs whatever that line is via execlp()
**               (unless the line is 'quit')
**
**  Your task:
**            Modify this program so it will run multiple
**            commands, in succession, until the user enters
**            quit.  (i.e., turn this into a *simple* shell)
**
**             It should also support infile <filename> <cmd>
**               to run <cmd> with the <filename> as stdin
**             It should also support outfile <filename> <cmd>
**               to run <cmd> with the <filename> as stdout
**             It should also support both infile and outfile
**               TOGETHER e.g.,
**               outfile <file1> infile <file2> <cmd>
**               to run <cmd> with the <fil1> as stdout
**               and <file2> as stdin
**
**
**  compile with:
**               g++ pa2_shell.cpp -o my_shell
**
**  run with:
**               ./my_shell
**
**  DO NOT DELETE MY COMMENTS ; if you do, you will lose credit
**
******************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_CMD_LENGTH 60

using namespace std;

main()
{
  // you might want to use these
  pid_t pid; // a process id
  int retv;  // a return value

  char *cmd;                         // our current command
  char *cmdWithArgs[MAX_CMD_LENGTH]; // current command with args

  string strLine; // this holds the whole input line
  string strCmd;  // this is just the command part

  while (strCmd != "quit")
  {
    // TODO #0: Prompt the user?
    pid = getpid();
    cout << "<LUKE CMD>$";

    strLine = "";
    strCmd = "";
    // TODO #1: keep the program running for multiple commands
    // getline gets the full line of input from cin (the user)
    getline(cin, strLine);

    // the following lines split the input into
    // the parts needed for a call to execlp
    // one c string (cmd) containing just the command
    // and one c string (cmdWithArgs) containing everything
    // -------------------
    istringstream ss; // declare a stream sourced from a string
    ss.str(strLine);  // initialize it with our input

    ss >> strCmd;                 // read the first 'word' of the input
    cmd = strdup(strCmd.c_str()); // convert it into c-string

    // note: this program leaks memory,

    int i = 0;
    cmdWithArgs[i++] = strdup(strCmd.c_str()); // convert it to exec compat

    while (ss >> strCmd)
    {
      cmdWithArgs[i++] = strdup(strCmd.c_str());
    }
    cmdWithArgs[i] = NULL; // make sure the last entry in the array is null.

    // if the user enters quit, then quit
    if (strCmd.compare("quit") == 0)
    {
      cout << "bye!" << endl;
      exit(0);
    }
    else
    {
      // TODO: do not lose this process to running execlp
      //          so that the program can keep running
      //          - ensure the new process runs the command
      //            but the existing process still takes commands
      //
      // TODO: this needs to be changed because it undoes what the above
      // works hard to do. The following always runs the command ls
      // with the arguments -ltr
      // instead run the user's input (variables are set up above)
      int rc = fork();
      if (rc < 0)
      {
        fprintf(stderr, "fork failed\n");
        return 1;
      }
      else if (rc == 0) // child Process(the one doing the exec)
      {

        string outFileName = " ";
        string inFileName = " ";
        bool isInFile = false;
        bool isOutFile = false;
        string input = "<";
        string output = ">";

        // if(strcmp(cmd, "outfile")== 0)
        // {
        //   //location of the output filw
        //   outFileName =cmdWithArgs[1];
        //   cmd = cmdWithArgs[2];
        //   if(strcmp(cmd,"infile")==0)
        //     {
        //     inFileName= cmdWithArgs[3];
        //     }

        // }
        // //infile present and not outfile
        // else if(strcmp(cmd,"infile")==0)
        // {
        //   inFileName = cmdWithArgs[1];
        // }
        int i = 0;
        string str(cmdWithArgs[0]);
        if (str == "outfile")
        {
          string next(cmdWithArgs[1]);
          outFileName = next;
          isOutFile = true;
          string in(cmdWithArgs[2]);
          if (in == "infile")
          {
            string inNext(cmdWithArgs[3]);
            isInFile = true;
            inFileName = inNext;
          }
        }
        else if (str == "infile")
        {
          string next(cmdWithArgs[1]);
          inFileName = next;
          isInFile = true;
        }
        if (isOutFile)
        {
          i = i + 2;
        }
        if (isInFile)
        {
          i = i + 2;
        }
        int iter = 0;
        for (int j = 0; j < MAX_CMD_LENGTH; j++)
        {
          // convert to allow for concatination
          if (cmdWithArgs[j] == NULL)
          {
            break;
          }
          else
          {
            cmdWithArgs[j] = cmdWithArgs[j + i];
            iter++;
          }
        }

        if (isInFile)
        {
          iter++;
          // adds to the argument list input file symbol
          cmdWithArgs[iter] = strdup(input.c_str());
          iter++;
          // add the input location
          cmdWithArgs[iter] = strdup(inFileName.c_str());

        }
        if (isOutFile)
        {
          iter++;
          // adds to the argument list output file symbol
          cmdWithArgs[iter] = strdup(output.c_str());
          iter++;
          // add the output location
          cmdWithArgs[iter] = strdup(outFileName.c_str());
        }
        iter++;
        cmdWithArgs[iter] = NULL;

        if(isInFile && isOutFile)
        {
         close(STDOUT_FILENO);
         outFileName = "./"+outFileName;
         open(outFileName.c_str(), O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
         
         int end = 0;
          for(int i =0; i< MAX_CMD_LENGTH; i++){
            if(cmdWithArgs[i]==NULL){
              break;
            }
            else{
              end++;
            }
          }
          cmdWithArgs[end] = strdup(inFileName.c_str());
          //add the outfile
          cmdWithArgs[end+1] = NULL;
        }
        else if(isOutFile)
        {
         close(STDOUT_FILENO);
         outFileName = "./"+outFileName;
         open(outFileName.c_str(), O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
        }
        else if(isInFile)
        {
          int end = 0;
          for(int i =0; i< MAX_CMD_LENGTH; i++){
            if(cmdWithArgs[i]==NULL){
              break;
            }
            else{
              end++;
            }
          }
          cmdWithArgs[end] = strdup(inFileName.c_str());
          cmdWithArgs[end+1] = NULL;
        }

        // Run command
        if (getpid() != pid)
        {
          execvp(cmdWithArgs[0], cmdWithArgs);
          string s = cmdWithArgs[0];
          if (s == "")
          {
            cout << "";
          }
          else
          {
            cout << "Invalid Command, Make sure that the command is correct and that it is spelled correctly" << endl;
          }
        }
      }

      else // parent waiting for child process
      {
        int rc_wait = wait(NULL);
      }
      //~~~~~~~~~~//EX. of what needs to be done~~~~//
      // cmdWithArgs[0] = strdup("ls");
      // cmdWithArgs[1] = strdup("-ltr");
      // cmdWithArgs[2] = NULL;

      // execvp(cmdWithArgs[0],cmdWithArgs);
      //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

      // remember: the OSTEP.org chapter on the CPU-API is your friend!

      // TODO: before the exec, you probably need to check for the
      // infile/outfile commands and adjust accordingly.

      // TODO: should also check the return value or kill the process if
      //          the exec fails, no extra processes should remain when done

      // TODO: make sure the parent waits for the child to finish
      //      so that the prompt appears after its output, not before
      // TODO: test to make sure if you hit enter a second time,
      //          it does NOT just re-run the last run command
    }
  }
}

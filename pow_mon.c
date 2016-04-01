/*
 * pow_mon.c
 *
 *  Created on: 30/mar/2016
 *      Author: timmy
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define die_err(e) do { fprintf(stderr, "%s\n", e); exit(EXIT_FAILURE); } while (0);
#define die_out(e) do { fprintf(stdout, "%s\n", e); exit(EXIT_FAILURE); } while (0);


//int main() {
//  int link[2];
//  pid_t pid;
//  char foo[4096];
//
//  if (pipe(link)==-1)
//    die_err("pipe");
//
//  if ((pid = fork()) == -1)
//    die_err("fork");
//
//  if(pid == 0) {
//
//	      dup2 (link[1], STDOUT_FILENO);
//    close(link[0]);
//    close(link[1]);
//    execl("/usr/bin/java", "/usr/bin/java", "-jar", "/home/timmy/Scrivania/data-generator.jar", "3", NULL);
//    sleep(10);
//        //execl("/bin/ls", "ls", "-1", (char *)0);
//    die_err("execl");
//
//  } else {
//
//    close(link[1]);
//    int nbytes = read(link[0], foo, sizeof(foo));
//    printf("Output: (%.*s)\n", nbytes, foo);
//    wait(NULL);
//
//  }
//  return 0;
//}

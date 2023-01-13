#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include "my_sock.h"

#define SERVER_ADDR "0.0.0.0"
#define STRING_MAX (1000 * 100)
#define ALL_ACCEPTED_ENTRIES (1000 * 1000)
#define ONCE_SEND_ENTRIES (1000L * 10)

struct append_entries
{
    char entry[STRING_MAX];
};

struct AppendEntriesRPC_Argument
{
    int term;
    int leaderID;
    int prevLogIndex;
    int prevLogTerm;
    int leaderCommit;
    struct append_entries entries[ONCE_SEND_ENTRIES];
};

int main(void)
{

    char *str = malloc(sizeof(char) * STRING_MAX);

    char c;
    int fdo;
    fdo = open("syscall.txt", (O_CREAT | O_RDWR), 0644);
    if (fdo == -1)
    {
        printf("error0\n");
        exit(1);
    }

    struct AppendEntriesRPC_Argument *AERPC_A = malloc(sizeof(struct AppendEntriesRPC_Argument));
    printf("Input -> ");
    scanf("%s", str);

    AERPC_A->term = 1;
    AERPC_A->leaderID = 2;
    AERPC_A->prevLogIndex = 3;
    AERPC_A->prevLogTerm = 4;
    AERPC_A->leaderCommit = 5;
    strcpy(AERPC_A->entries[0].entry, str);

    write(fdo, AERPC_A, sizeof(struct AppendEntriesRPC_Argument));

    char *str1 = malloc(sizeof(char) * STRING_MAX);
    printf("Input -> ");
    scanf("%s", str1);

    AERPC_A->term = 10;
    AERPC_A->leaderID = 20;
    AERPC_A->prevLogIndex = 30;
    AERPC_A->prevLogTerm = 40;
    AERPC_A->leaderCommit = 50;
    strcpy(AERPC_A->entries[0].entry, str1);

    write(fdo, AERPC_A, sizeof(struct AppendEntriesRPC_Argument));

    close(fdo);
    fdo = open("syscall.txt", O_RDONLY);
    if (fdo == -1)
    {
        printf("error\n");
        exit(1);
    }
    lseek(fdo, -sizeof(struct AppendEntriesRPC_Argument), SEEK_END);

    read(fdo, AERPC_A, sizeof(struct AppendEntriesRPC_Argument));
    printf("term: %d\n", AERPC_A->term);
    printf("leaderID: %d\n", AERPC_A->leaderID);
    printf("prevLogIndex: %d\n", AERPC_A->prevLogIndex);
    printf("prevLogTerm: %d\n", AERPC_A->prevLogTerm);
    printf("LeaderCommitIndex: %d\n", AERPC_A->leaderCommit);
    printf("entry: %s\n", AERPC_A->entries[0].entry);

    close(fdo);
    exit(0);
}
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
#define STRING_MAX (1000L * 100)
#define ALL_ACCEPTED_ENTRIES (1000L * 1000)
#define ONCE_SEND_ENTRIES (1000L * 10)

uint64_t c1,
    c2;
struct timespec ts1, ts2;
double t;

static uint64_t rdtscp()
{
    uint64_t rax;
    uint64_t rdx;
    uint32_t aux;
    asm volatile("rdtscp"
                 : "=a"(rax), "=d"(rdx), "=c"(aux)::);
    return (rdx << 32) | rax;
}

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

struct AppendEntriesRPC_Result
{
    int term;
    bool success;
};

struct LOG
{
    char entry[STRING_MAX];
    int term;
};

struct AllServer_PersistentState
{
    int currentTerm;
    int voteFor;
    struct LOG log[ALL_ACCEPTED_ENTRIES];
};

struct AllServer_VolatileState
{
    int commitIndex;
    int LastAppliedIndex;
};

struct Leader_VolatileState
{
    int nextIndex[5];
    int matchIndex[5];
};

// char filename[20];
char *filename;
char *logfilename;

// // logfile初期化
void make_logfile(char *name)
{
    FILE *logfile;

    filename = name;

    // sprintf(filename, sizeof(filename), "%s.dat", name);
    logfile = fopen(filename, "wb+");
    fclose(logfile);

    return;
}

void write_log(
    // char filename[],
    int i,
    struct AllServer_PersistentState *AS_PS)
{
    FILE *logfile;

    logfile = fopen(filename, "ab+"); // 追加読み書き
    if (logfile == NULL)
    {
        printf("cannot write log\n");
        exit(1);
    }
    for (int num = 1; num < ONCE_SEND_ENTRIES; num++)
    {
        fwrite(&AS_PS->currentTerm, sizeof(int), 1, logfile);
        fwrite(&AS_PS->voteFor, sizeof(int), 1, logfile);
        fwrite(&AS_PS->log[(i - 1) * (ONCE_SEND_ENTRIES - 1) + num].term, sizeof(int), 1, logfile);

        fwrite(&AS_PS->log[(i - 1) * (ONCE_SEND_ENTRIES - 1) + num].entry, sizeof(char), STRING_MAX, logfile);
    }
    fclose(logfile);
    return;
}

void read_log(
    // char filename[],
    int i)
{
    FILE *logfile;
    struct AllServer_PersistentState *AS_PS = malloc(sizeof(struct AllServer_PersistentState));

    logfile = fopen(filename, "rb");
    fseek(logfile, 0L, SEEK_SET);
    for (int j = 1; j < i; j++)
    {
        fseek(logfile, ((STRING_MAX + 12) * (ONCE_SEND_ENTRIES - 1)), SEEK_CUR);
    }
    for (int num = 1; num < ONCE_SEND_ENTRIES; num++)
    {
        fread(&(AS_PS->currentTerm), sizeof(int), 1, logfile);
        fread(&(AS_PS->voteFor), sizeof(int), 1, logfile);
        fread(&(AS_PS->log[(i - 1) * (ONCE_SEND_ENTRIES - 1) + num].term), sizeof(int), 1, logfile);

        fread(&(AS_PS->log[(i - 1) * (ONCE_SEND_ENTRIES - 1) + num].entry), sizeof(char), STRING_MAX, logfile);
    }
    for (int num = 1; num < ONCE_SEND_ENTRIES; num++)
    {
        printf("[logfile] AS_PS->currentTerm = %d\n", AS_PS->currentTerm);
        printf("[logfile] AS_PS->voteFor = %d\n", AS_PS->voteFor);
        printf("[logfile] AS_PS->log[%ld].term = %d\n", (i - 1) * (ONCE_SEND_ENTRIES - 1) + num, AS_PS->log[(i - 1) * (ONCE_SEND_ENTRIES - 1) + num].term);

        printf("[logfile] AS_PS->log[%ld].entry = %s\n\n", (i - 1) * (ONCE_SEND_ENTRIES - 1) + num, AS_PS->log[(i - 1) * (ONCE_SEND_ENTRIES - 1) + num].entry);
    }

    fclose(logfile);
    return;
}

void output_AERPC_A(struct AppendEntriesRPC_Argument *p)
{
    printf("---appendEntriesRPC---\n");
    printf("term: %d\n", p->term);
    for (int i = 1; i < ONCE_SEND_ENTRIES; i++)
    {
        printf("entry: %s\n", p->entries[i - 1].entry);
    }
    printf("prevLogIndex: %d\n", p->prevLogIndex);
    printf("prevLogTerm: %d\n", p->prevLogTerm);
    printf("LeaderCommitIndex: %d\n", p->leaderCommit);
    printf("----------------------\n");
    return;
}

void output_AERPC_R(struct AppendEntriesRPC_Result *p)
{
    printf("**AERPC_R**\n");
    printf("term: %d\n", p->term);
    printf("bool: %d\n", p->success);
    printf("***********\n");
}
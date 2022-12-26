#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define SERVER_ADDR "0.0.0.0"
#define MAX (10000 * 10000)
#define ENTRY_NUM 100

uint64_t c1, c2;
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

struct AppendEntriesRPC_Argument
{
    int term;
    int leaderID;
    int prevLogIndex;
    int prevLogTerm;
    char *entries[ENTRY_NUM];
    // struct appended_entry entries[ENTRY_NUM];
    int leaderCommit;
};
//*entries[ENTRY_NUM]なのは、効率のために複数のentryを同時に送ることがあるから。

struct AppendEntriesRPC_Result
{
    int term;
    bool success;
};

struct LOG
{
    char entry[MAX];
    int term;
};

// entryは20個まで可能
struct AllServer_PersistentState
{
    int currentTerm;
    int voteFor;
    struct LOG log[ENTRY_NUM];
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

void entries_box(
    struct AppendEntriesRPC_Argument *AERPC_A)
{
    for (int i = 0; i < ENTRY_NUM; i++)
    {
        AERPC_A->entries[i] = malloc(sizeof(char) * MAX);
    }
}

// char filename[20];
char *filename;
char *logfilename;

// logfile初期化
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

    fwrite(&AS_PS->currentTerm, sizeof(int), 1, logfile);
    fwrite(&AS_PS->voteFor, sizeof(int), 1, logfile);
    fwrite(&AS_PS->log[i].term, sizeof(int), 1, logfile);
    fwrite(&AS_PS->log[i].entry, sizeof(char), MAX, logfile);

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
        fseek(logfile, (MAX + 3), SEEK_CUR);
    }
    fread(&(AS_PS->currentTerm), sizeof(int), 1, logfile);
    fread(&(AS_PS->voteFor), sizeof(int), 1, logfile);
    fread(&(AS_PS->log[i].term), sizeof(int), 1, logfile);
    fread(&(AS_PS->log[i].entry), sizeof(char), MAX, logfile);

    printf("[logfile]AS_PS->currentTerm = %d\n", AS_PS->currentTerm);
    printf("[logfile] AS_PS->voteFor = %d\n", AS_PS->voteFor);
    printf("[logfile] AS_PS->log[%d].term = %d\n", i, AS_PS->log[i].term);
    printf("[logfile] AS_PS->log[%d].entry = %s\n", i, AS_PS->log[i].entry);

    fclose(logfile);
    return;
}

void output_AERPC_A(struct AppendEntriesRPC_Argument *p)
{
    printf("term: %d\n", p->term);
    printf("entry: %s\n", p->entries[0]); // 今は一度に一つのentryしか送らない場合のみ考える
    printf("prevLogIndex: %d\n", p->prevLogIndex);
    printf("prevLogTerm: %d\n", p->prevLogTerm);
    printf("LeaderCommitIndex: %d\n", p->leaderCommit);

    return;
}

void output_AERPC_R(struct AppendEntriesRPC_Result *p)
{
    printf("term: %d\n", p->term);
    printf("bool: %d\n", p->success);
}

// void timecheck()
// {
//     clock_gettime(CLOCK_MONOTONIC, &ts1);
//     c1 = rdtscp();
//     sleep(1);
//     c2 = rdtscp();
//     clock_gettime(CLOCK_MONOTONIC, &ts2);

//     t = ts2.tv_sec - ts1.tv_sec + (ts2.tv_nsec - ts1.tv_nsec) / 1e9;

//     printf("time=%.4fs clock=%.0fMHz\n", t, (c2 - c1) / t / 1e6);
// }
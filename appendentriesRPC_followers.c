#include "appendentries.h"
#include "debug.h"

int consistency_check(
    struct AppendEntriesRPC_Argument *rpc,
    struct AllServer_PersistentState *as_ps,
    struct AllServer_VolatileState *as_vs)
{

    // 1. Reply false if term<currentTerm
    if (rpc->term < as_ps->currentTerm)
    {
        printf("reject1,%d,%d\n", rpc->term, as_ps->currentTerm);
        return false;
    }

    // 2. Reply false if log doesn’t contain an entry at prevLogIndex whose term matches prevLogTerm
    if (rpc->prevLogTerm != as_ps->log[rpc->prevLogIndex].term)
    {
        printf("reject2,%d,%d\n", rpc->prevLogTerm, as_ps->log[rpc->prevLogIndex].term);
        return false;
    }

    // 3. If an existing entry conflicts with a new one(same index but different terms), delete the existing entry and all that follow it
    if (strcmp(as_ps->log[rpc->prevLogIndex + 1].entry, as_ps->log[0].entry) == -1 && as_ps->log[rpc->prevLogIndex + 1].term != rpc->term)
    {
        printf("reject3.0\n");
        int j = strcmp(as_ps->log[rpc->prevLogIndex + 1].entry, as_ps->log[0].entry);

        while (j == 0)
        {
            for (int i = rpc->prevLogIndex + 1; i < ENTRY_NUM; i++)
            {
                memset(as_ps->log[rpc->prevLogIndex + 1].entry, 0, sizeof(char) * MAX);
            }
        }
        printf("reject3\n");
        return false;
    }

    printf("rpc->prevLogIndex = %d", rpc->prevLogIndex);

    // 4. Append any new entries not already in the log
    for (int num = 1; num < NUM; num++)
    {
        // as_ps->log[rpc->prevLogIndex + 1].term = rpc->term;
        as_ps->log[rpc->prevLogIndex + num].term = rpc->term;
        strcpy(as_ps->log[rpc->prevLogIndex + num].entry, rpc->entries[num - 1]);
    }

    // ここらへん変えてる途中。
    as_vs->LastAppliedIndex = rpc->prevLogIndex + NUM;
    /* log記述 */
    write_log(rpc->prevLogIndex / (NUM - 1) + 1, as_ps);
    read_log(rpc->prevLogIndex / (NUM - 1) + 1);

    // 5. If leaderCmakeommit> commitIndex, set commitIndex = min(leaderCommit, index of last new entry)
    if (rpc->leaderCommit > as_vs->commitIndex)
    {
        as_vs->commitIndex = rpc->leaderCommit;
    }
    printf("write log\n");

    return true;
};

int transfer(
    int sock,
    struct AppendEntriesRPC_Argument *AERPC_A,
    struct AppendEntriesRPC_Result *AERPC_R,
    struct AllServer_PersistentState *AS_PS,
    struct AllServer_VolatileState *AS_VS)
{

    /* クライアントから文字列を受信 */
    // ここ変える
    recv(sock, AERPC_A, sizeof(struct AppendEntriesRPC_Argument), MSG_WAITALL);
    for (int num = 1; num < NUM; num++)
    {
        recv(sock, AERPC_A->entries[num - 1], sizeof(char) * MAX, MSG_WAITALL);
    }
    output_AERPC_A(AERPC_A);

    // consistency check
    AERPC_R->success = consistency_check(AERPC_A, AS_PS, AS_VS);
    AERPC_R->term = AS_PS->currentTerm;
    if (AERPC_R->success == false)
    {
        printf("AERPC_R->success == false\n");
        exit(1);
    }

    // lerderに返答
    send(sock, AERPC_R, sizeof(struct AppendEntriesRPC_Result), 0);

    /* 受信した文字列を表示 */
    // printf("replied\n");
    // output_AERPC_A(AERPC_A);
    return 0;
}

int main(int argc, char *argv[])
{
    int w_addr, c_sock;
    struct sockaddr_in a_addr;

    /* ソケットを作成 */
    int SERVER_PORT = atoi(argv[1]);

    w_addr = socket(AF_INET, SOCK_STREAM, 0);

    /* 構造体を全て0にセット */
    memset(&a_addr, 0, sizeof(struct sockaddr_in));

    /* サーバーのIPアドレスとポートの情報を設定 */
    a_addr.sin_family = AF_INET;
    a_addr.sin_port = htons((unsigned short)SERVER_PORT);
    a_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    /* ソケットに情報を設定 */
    bind(w_addr, (const struct sockaddr *)&a_addr, sizeof(a_addr));

    /* ソケットを接続待ちに設定 */
    listen(w_addr, 3);

    struct AppendEntriesRPC_Argument *AERPC_A = malloc(sizeof(struct AppendEntriesRPC_Argument));
    struct AppendEntriesRPC_Result *AERPC_R = malloc(sizeof(struct AppendEntriesRPC_Result));
    struct AllServer_PersistentState *AS_PS = malloc(sizeof(struct AllServer_PersistentState));
    struct AllServer_VolatileState *AS_VS = malloc(sizeof(struct AllServer_VolatileState));
    entries_box(AERPC_A);
    printf("makefile\n");

    make_logfile(argv[2]);

    AS_PS->currentTerm = 0;
    AS_PS->log[0].term = 0;
    for (int i = 0; i < 20; i++)
    {
        memset(AS_PS->log[i].entry, 0, sizeof(char) * MAX);
    }
    AS_VS->commitIndex = 0;
    AS_VS->LastAppliedIndex = 0;

    int k = 1;
    printf("Waiting connect...\n");
    c_sock = accept(w_addr, NULL, NULL);
    send(c_sock, &k, sizeof(int) * 1, 0);
    printf("Connected!!\n");

    AS_PS->currentTerm += 1;

    while (1)
    {
        transfer(c_sock, AERPC_A, AERPC_R, AS_PS, AS_VS);
    }

    exit(1);

    close(c_sock);

    /* 次の接続要求の受け付けに移る */

    /* 接続待ちソケットをクローズ */
    close(w_addr);

    return 0;
}
#include "appendentries.h"
#include "debug.h"
// #define NULL __DARWIN_NULL

int AppendEntriesRPC(
    int connectserver_num,
    int sock[],
    struct AppendEntriesRPC_Argument *AERPC_A,
    struct AppendEntriesRPC_Result *AERPC_R,
    struct Leader_VolatileState *L_VS,
    struct AllServer_VolatileState *AS_VS,
    struct AllServer_PersistentState *AS_PS)
{
    int replicatelog_num = 0;

    /* AERPC_Aの設定 */
    // このentryを1つしか送れないのを変更する。argvで受け取った数だけ送れるようにするのが理想。

    AERPC_A->term = AS_PS->log[L_VS->nextIndex[0]].term;
    AERPC_A->prevLogIndex = L_VS->nextIndex[0] - 1;
    AERPC_A->prevLogTerm = AS_PS->log[AERPC_A->prevLogIndex].term;
    for (int i = 1; i < NUM; i++)
    {
        strcpy(AERPC_A->entries[i - 1], AS_PS->log[L_VS->nextIndex[0] + (i - 1)].entry);
    }
    AERPC_A->leaderCommit = AS_VS->commitIndex;

    output_AERPC_A(AERPC_A);

    for (int i = 0; i < connectserver_num; i++)
    {
        // send(sock[i], AERPC_A, sizeof(struct AppendEntriesRPC_Argument), 0);
        my_send(sock[i], AERPC_A, sizeof(struct AppendEntriesRPC_Argument));
        for (int num = 1; num < NUM; num++)
        {
            // send(sock[i], AERPC_A->entries[num - 1], sizeof(char) * MAX, 0);
            my_send(sock[i], AERPC_A->entries[num - 1], sizeof(char) * MAX);
        }
        // recv(sock[i], AERPC_R, sizeof(struct AppendEntriesRPC_Result), MSG_WAITALL);
        my_recv(sock[i], AERPC_R, sizeof(struct AppendEntriesRPC_Result));

        printf("finish sending\n\n");
    }

    for (int i = 0; i < connectserver_num; i++)
    {
        output_AERPC_R(AERPC_R);
        printf("send to server %d\n", i);

        // • If successful: update nextIndex and matchIndex for follower.
        if (AERPC_R->success == 1)
        {
            L_VS->nextIndex[i] += (NUM - 1);
            L_VS->matchIndex[i] += (NUM - 1);

            replicatelog_num += (NUM - 1);

            printf("Success:%d\n", i);
        }
        // • If AppendEntries fails because of log inconsistency: decrement nextIndex and retry.
        else
        {
            printf("failure0\n");
            L_VS->nextIndex[i] -= (NUM - 1);
            AERPC_A->prevLogIndex -= (NUM - 1);
            AppendEntriesRPC(connectserver_num, sock, AERPC_A, AERPC_R, L_VS, AS_VS, AS_PS);
            printf("failure1\n");
            exit(1);
        }
    }
    // AS_VS->commitIndex += 1;
    return replicatelog_num;
}

int main(int argc, char *argv[])
{
    int SERVER_PORT[5];
    SERVER_PORT[0] = 1234;
    SERVER_PORT[1] = 2345;
    SERVER_PORT[2] = 3456;
    SERVER_PORT[3] = 4567;
    SERVER_PORT[4] = 5678;

    int sock[5];
    struct sockaddr_in addr[5];

    /* ソケットを作成 */
    for (int i = 0; i < 5; i++)
    {
        sock[i] = socket(AF_INET, SOCK_STREAM, 0);
    }

    /* 構造体を全て0にセット */
    for (int i = 0; i < 5; i++)
    {
        memset(&addr[i], 0, sizeof(struct sockaddr_in));
    }

    /* サーバーのIPアドレスとポートの情報を設定 */
    for (int i = 0; i < 5; i++)
    {
        addr[i].sin_family = AF_INET;
        addr[i].sin_port = htons((unsigned short)SERVER_PORT[i]);
        addr[i].sin_addr.s_addr = inet_addr(SERVER_ADDR);
    }

    /* followerとconnect */
    int connectserver_num = 0;
    printf("Start connect...\n");
    for (int i = 0; i < 5; i++)
    {
        int k = 0;
        connect(sock[i], (struct sockaddr *)&addr[i], sizeof(struct sockaddr_in));
        recv(sock[i], &k, sizeof(int) * 1, MSG_WAITALL);
        if (k == 1)
        {
            connectserver_num += k;
        }
    }
    printf("Finish connect with %dservers!\n", connectserver_num);

    struct AppendEntriesRPC_Argument *AERPC_A = malloc(sizeof(struct AppendEntriesRPC_Argument));
    struct AppendEntriesRPC_Result *AERPC_R = malloc(sizeof(struct AppendEntriesRPC_Result));
    struct AllServer_PersistentState *AS_PS = malloc(sizeof(struct AllServer_PersistentState));
    struct AllServer_VolatileState *AS_VS = malloc(sizeof(struct AllServer_VolatileState));
    struct Leader_VolatileState *L_VS = malloc(sizeof(struct Leader_VolatileState));
    entries_box(AERPC_A);

    // 初期設定
    AERPC_A->term = 1;
    AERPC_A->leaderID = 1;
    AERPC_A->prevLogIndex = 0;
    AERPC_A->prevLogTerm = 0;
    AERPC_A->leaderCommit = 0;

    AS_PS->currentTerm = 1;
    AS_PS->log[0].term = 0;
    for (int i = 0; i < 20; i++)
    {
        memset(AS_PS->log[i].entry, 0, sizeof(char) * MAX);
    }
    AS_PS->voteFor = 0;

    AS_VS->commitIndex = 0;
    AS_VS->LastAppliedIndex = 0;

    for (int i = 0; i < 5; i++)
    {
        L_VS->nextIndex[i] = 1; // (AERPC_A->leaderCommit=0) + 1
        L_VS->matchIndex[i] = 0;
    }

    char *str = malloc(sizeof(char) * MAX);
    int replicatelog_num;

    /* log記述用のファイル名 */
    make_logfile(argv[1]);

    /* 接続済のソケットでデータのやり取り */
    // 今は受け取れるentryが有限
    for (int i = 1; i < ENTRY_NUM; i++)
    {
        /* followerに送る */
        /* AS_PSの更新 */
        for (int num = 1; num < NUM; num++)
        {
            printf("Input -> ");
            scanf("%s", str);

            /* log[0]には入れない。log[1]から始める。　first index is 1*/
            // AERPC_A->entries[0] = str;
            strcpy(AS_PS->log[(i - 1) * (NUM - 1) + num].entry, str);
            AS_PS->log[(i - 1) * (NUM - 1) + num].term = AS_PS->currentTerm;

            printf("AS_PS->log[%d].term = %d\n", (i - 1) * (NUM - 1) + num, AS_PS->log[(i - 1) * (NUM - 1) + num].term);
            printf("AS_PS->log[%d].entry = %s\n\n", (i - 1) * (NUM - 1) + num, AS_PS->log[(i - 1) * (NUM - 1) + num].entry);
        }
        clock_gettime(CLOCK_MONOTONIC, &ts1);

        /* logを書き込み */

        write_log(i, AS_PS);
        read_log(i);

        /* AS_VSの更新 */
        // AS_VS->commitIndex = ; ここの段階での変更は起きない
        AS_VS->LastAppliedIndex += (NUM - 1); // = i

        replicatelog_num = AppendEntriesRPC(connectserver_num, sock, AERPC_A, AERPC_R, L_VS, AS_VS, AS_PS);

        if (replicatelog_num > (connectserver_num + 1) / 2)
        {
            printf("majority of servers replicated\n");
        }

        clock_gettime(CLOCK_MONOTONIC, &ts2);
        t = ts2.tv_sec - ts1.tv_sec + (ts2.tv_nsec - ts1.tv_nsec) / 1e9;

        printf("time=%.4fs\n", t);
    }

    /* ソケット通信をクローズ */
    // for (int i = 0; i < 5; i++)
    // {
    //     close(sock[i]);
    // }

    return 0;
}

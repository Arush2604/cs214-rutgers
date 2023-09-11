#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#define QUEUE_SIZE 8
#define BUFSIZE 2048
#define THREADSIZE 200

#define PLAYER_ONE_VAL 1
#define PLAYER_TWO_VAL 2

#define PLAYER_ONE_CHAR "X"
#define PLAYER_TWO_CHAR "O"

// game state
#define STATE_WAITING_P1 0
#define STATE_WAITING_P2 1
#define STATE_WAITING_FOR_P1_MOVE 2
#define STATE_WAITING_FOR_P2_MOVE 3
#define STATE_WAITING_FOR_P1_DRAW_CONF 4
#define STATE_WAITING_FOR_P2_DRAW_CONF 5
#define STATE_GAME_OVER_P1_WIN 6
#define STATE_GAME_OVER_P2_WIN 7
#define STATE_GAME_OVER_DRAW 8
#define STATE_GAME_OVER_P1_RESIGN 9
#define STATE_GAME_OVER_P2_RESIGN 10
#define STATE_GAME_OVER_DRAW_AGG 11
#define STATE_BEGN 12
#define STATE_DRAW_REJ 13
#define STATE_OVER_READY_FOR_NEW 14

// protocol
#define PLAY_PROT 1
#define PLAY_PROT_NAME "PLAY"

#define WAIT_PROT 2
#define WAIT_PROT_NAME "WAIT"

#define BEGN_PROT 3
#define BEGN_PROT_NAME "BEGN"

#define MOVE_PROT 4
#define MOVE_PROT_NAME "MOVE"

#define MOVD_PROT 5
#define MOVD_PROT_NAME "MOVD"

#define INVL_PROT 6
#define INVL_PROT_NAME "INVL"

#define RSGN_PROT 7
#define RSGN_PROT_NAME "RSGN"

#define DRAW_PROT 8
#define DRAW_PROT_NAME "DRAW"

#define OVER_PROT 9
#define OVER_PROT_NAME "OVER"


typedef struct ProtInfo {
    char name[100];
    char role[100];
    char position[100];
    char board[100];
    char reason[100];
    char message[100];
    char outcome[100];
} ProtInfo;

typedef struct Game {
    char playerOneName[100];
    char playerTwoName[100];

    int gameState;

    int playerUnhandled;
    ProtInfo lastProtInfo;

    char board[3][3];
} Game;

struct Game games[100];
int hasLatestGameBegun = 0;

pthread_mutex_t games_lock;

pthread_t threads[THREADSIZE];
int active_threads[THREADSIZE];

int open_listener(char *service) {
    struct addrinfo hint, *info_list, *info;
    int sock;

    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family   = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags    = AI_PASSIVE;

    if (getaddrinfo(NULL, service, &hint, &info_list)) {
        return -1;
    }
    
    for (info = info_list; info != NULL; info = info->ai_next) {
        sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        if (sock == -1) continue;

        if (bind(sock, info->ai_addr, info->ai_addrlen)) {
            close(sock);
            continue;
        }

        if (listen(sock, QUEUE_SIZE)) {
            close(sock);
            continue;
        }

        break;
    }

    freeaddrinfo(info_list);

    if (info == NULL) {
        fprintf(stderr, "Could not bind\n");
        return -1;
    }

    return sock;
}

int findOpenGame() {
    for(int i = 0; i < 100; i++) {
        if(games[i].gameState == STATE_WAITING_P1 || games[i].gameState == STATE_WAITING_P2 || games[i].gameState == STATE_OVER_READY_FOR_NEW) {
            return i;
        }
    }

    return -1;
}

int tokenize(char* str, ProtInfo** fmtMsg, int msglen) {

    //printf("msg len: %d", msglen);
    //printf("strlen %d", strlen(str));
    int wasdeformed = 0;
    if(msglen < 10) {
        if (strlen(str) > 8 + msglen) {
            wasdeformed = 1;
            str[msglen+7] = '\0';
            //printf("cleaned string :%s\n", str);
        }
        else {
            //printf("Good Length\n");

        }
    }
    else{
         if (strlen(str) > 9 + msglen) {
            wasdeformed = 1;
            str[msglen+8] = '\0';
            //printf("cleaned string :%s\n", str);
        }
        else {
            printf("Good Length\n");

        }

    }

    char delim[] = "|";
    char *token, *saveptr;

    printf("Original string: %s\n", str);

    token = strtok_r(str, delim, &saveptr);
    int index = 0;
    int expectedLast = 0;

    *fmtMsg = malloc(sizeof(ProtInfo));
    ProtInfo* fmtMsgTyped = *fmtMsg;
    char* tokens[5];

    int msgType;

    while (token != NULL) {
        tokens[index] = token;
        token = strtok_r(NULL, "|", &saveptr);
    
        index++;
    }

    if(index == 0) {
        return -1;
    }

    if(strcmp(tokens[0], PLAY_PROT_NAME) == 0) {
        expectedLast = 3;
        msgType = PLAY_PROT;
    } else if(strcmp(tokens[0], WAIT_PROT_NAME) == 0) {
        expectedLast = 2;
        msgType = WAIT_PROT;
    } else if(strcmp(tokens[0], BEGN_PROT_NAME) == 0) {
        expectedLast = 4;
        msgType = BEGN_PROT;
    } else if(strcmp(tokens[0], MOVE_PROT_NAME) == 0) {
        expectedLast = 4;
        msgType = MOVE_PROT;
    } else if(strcmp(tokens[0], MOVD_PROT_NAME) == 0) {
        expectedLast = 5;
        msgType = MOVD_PROT;
    } else if(strcmp(tokens[0], INVL_PROT_NAME) == 0) {
        expectedLast = 3;
        msgType = INVL_PROT;
    } else if(strcmp(tokens[0], RSGN_PROT_NAME) == 0) {
        expectedLast = 2;
        msgType = RSGN_PROT;
    } else if(strcmp(tokens[0], DRAW_PROT_NAME) == 0) {
        expectedLast = 3;
        msgType = DRAW_PROT;
    } else if(strcmp(tokens[0], OVER_PROT_NAME) == 0) {
        expectedLast = 4;
        msgType = OVER_PROT;
    } else {
        return -1;
    }

    if(wasdeformed == 0) {
        if(index != (expectedLast+1)) {
        printf("FAILED HERE: %d %d", index, expectedLast+1);
        return -1;
        }
    }
    else {
        if(index != (expectedLast)) {
        printf("FAILED HERE: %d %d", index, expectedLast);
        return -1;
        }


    }

    if(msgType == PLAY_PROT) {
        strcpy(fmtMsgTyped->name, tokens[2]);
    } else if(msgType == BEGN_PROT) {
        strcpy(fmtMsgTyped->role, tokens[2]);
        strcpy(fmtMsgTyped->name, tokens[3]);
    } else if(msgType == MOVE_PROT) {
        strcpy(fmtMsgTyped->role, tokens[2]);
        strcpy(fmtMsgTyped->position, tokens[3]);
    } else if(msgType == MOVD_PROT) {
        strcpy(fmtMsgTyped->role, tokens[2]);
        strcpy(fmtMsgTyped->position, tokens[3]);
        strcpy(fmtMsgTyped->board, tokens[4]);
    } else if(msgType == INVL_PROT) {
        strcpy(fmtMsgTyped->reason, tokens[2]);
    } else if(msgType == DRAW_PROT) {
        strcpy(fmtMsgTyped->message, tokens[2]);
    } else if(msgType == OVER_PROT) {
        strcpy(fmtMsgTyped->outcome, tokens[2]);
        strcpy(fmtMsgTyped->reason, tokens[3]);
    }

    return msgType;
}

void send_resp(int sock, char buf[BUFSIZE + 1], int msgType, ProtInfo* fmtMsg) {
    if(msgType == WAIT_PROT) {
        snprintf(buf, BUFSIZE, "%s|0|", WAIT_PROT_NAME);
    } else if(msgType == BEGN_PROT) {
        snprintf(buf, BUFSIZE, "%s|%ld|%s|%s|", BEGN_PROT_NAME, 1 + strlen(fmtMsg->role) + strlen(fmtMsg->name), fmtMsg->role, fmtMsg->name);
    } else if(msgType == MOVD_PROT) {
        snprintf(buf, BUFSIZE, "%s|%ld|%s|%s|%s|", MOVD_PROT_NAME, 2 + strlen(fmtMsg->role) + strlen(fmtMsg->position) + strlen(fmtMsg->board), fmtMsg->role, fmtMsg->position, fmtMsg->board);
    } else if(msgType == INVL_PROT) {
        snprintf(buf, BUFSIZE, "%s|%ld|%s|", INVL_PROT_NAME, strlen(fmtMsg->reason), fmtMsg->reason);
    } else if(msgType == DRAW_PROT) {
        snprintf(buf, BUFSIZE, "%s|%ld|%s|", DRAW_PROT_NAME, strlen(fmtMsg->message), fmtMsg->message);
    } else if(msgType == OVER_PROT) {
        snprintf(buf, BUFSIZE, "%s|%ld|%s|%s|", OVER_PROT_NAME, 1 + strlen(fmtMsg->outcome) + strlen(fmtMsg->reason), fmtMsg->outcome, fmtMsg->reason);
    }

    printf("SENDING %s\n", buf);

    int send_size = send(sock, buf, strlen(buf), 0);

    if (send_size == -1) {
        perror("Error sending data");
        exit(1);
    }
}

int maybeCommitBoardMove(char* position, int currentGame, int player) {
    int row, col;

    // Extract two integers from the string
    int result = sscanf(position, "%d,%d", &row, &col);

    if (result != 2) {
        return -1;
    }

    row--;
    col--;

    if(row < 0 || row > 2 || col < 0 || col > 2) {
        return -1;
    }

    if(games[currentGame].board[row][col] != 0) {
        return -1;
    } else {
        if(player == 1) {
            games[currentGame].board[row][col] = 'X';
        } else {
            games[currentGame].board[row][col] = 'O';
        }
        
    }

    return 0;    
}

int checkWin(char player, char board[3][3]) {
    for (int i = 0; i < 3; i++) {
        if (board[i][0] == player && board[i][1] == player && board[i][2] == player) {
            return 1;
        }
    }

    for (int i = 0; i < 3; i++) {
        if (board[0][i] == player && board[1][i] == player && board[2][i] == player) {
            return 1;
        }
    }

    if ((board[0][0] == player && board[1][1] == player && board[2][2] == player) || (board[0][2] == player && board[1][1] == player && board[2][0] == player)) {
        return 1;
    }

    return 0;
}

// -1 if not over, 0 is a draw, 1 is player 1 winning, 2 is player 2 winning
int isGameOver(char board[3][3]) {
    if(checkWin('X', board)) {
        return 1;
    } else if(checkWin('O', board)) {
        return 2;
    } else {
        for(int i = 0; i < 3; i++) {
            for(int y = 0; y < 3; y++) {
                if(board[i][y] == 0) {
                    return -1;
                }
            }
        }

        return 0;
    }
}

int findmsglen(char * msg) {

    //printf("function proc");

    char * str = strdup(msg);
    char* token;
    char* rest = str;
    int callcount = 0;
    char * size;

    char* tokens[5];
    token = strtok_r(rest, "|", &rest);
 
    while (token != NULL) {
        //printf("%s\n", token);
        tokens[callcount] = token;
        token = strtok_r(NULL, "|", &rest);
        callcount++;
    
    }

    //printf("strlen %s\n", tokens[1]);
    int len = atoi(tokens[1]);
    //printf("msg len: %d\n", len);
    free(str);
    return len;

}




void proc_msg(char* msg, int* currentGame, int* player, int sock, char buf[BUFSIZE + 1]) {
    ProtInfo* fmtMsg;
    int msglen = findmsglen(msg);
    int msgType = tokenize(msg, &fmtMsg, msglen);

    if(msgType == -1) {
        strcpy(fmtMsg->reason, "!Error! invalid message type.");
        send_resp(sock, buf, INVL_PROT, fmtMsg);
        return;
    }

    printf("GOT A REQ: %d %d %d\n", games[*currentGame].gameState, msgType, *player);

    if(games[*currentGame].gameState == STATE_WAITING_P1 && msgType == PLAY_PROT && *player == 0) {
        strcpy(games[*currentGame].playerOneName, fmtMsg->name);
        games[*currentGame].gameState = STATE_WAITING_P2;
        *player = 1;

        send_resp(sock, buf, WAIT_PROT, fmtMsg);
    } else if(games[*currentGame].gameState == STATE_WAITING_P2 && msgType == PLAY_PROT && *player == 0) {
        strcpy(games[*currentGame].playerTwoName, fmtMsg->name);
        games[*currentGame].gameState = STATE_BEGN;
        *player = 2;

        strcpy(fmtMsg->role, "O");

        games[*currentGame].playerUnhandled = 1;


        send_resp(sock, buf, WAIT_PROT, fmtMsg);
        send_resp(sock, buf, BEGN_PROT, fmtMsg);

    } else if((games[*currentGame].gameState == STATE_WAITING_FOR_P1_MOVE || games[*currentGame].gameState == STATE_BEGN || games[*currentGame].gameState == STATE_DRAW_REJ) && msgType == MOVE_PROT && *player == 1) {
        if((strcmp(fmtMsg->role, "X") == 0 && *player != 1) || (strcmp(fmtMsg->role, "O") == 0 && *player != 2)) {
            strcpy(fmtMsg->reason, "!Error! invalid role type.");
            send_resp(sock, buf, INVL_PROT, fmtMsg);
            return;
        }
        
        if(maybeCommitBoardMove(fmtMsg->position, *currentGame, *player) == -1) {
            strcpy(fmtMsg->reason, "Error! invalid board move.");
            send_resp(sock, buf, INVL_PROT, fmtMsg);
        } else {
            int overState = isGameOver(games[*currentGame].board);

            if(overState == 1) {
                games[*currentGame].gameState = STATE_GAME_OVER_P1_WIN;

                strcpy(fmtMsg->outcome, "W");
                strcpy(fmtMsg->reason, "Player 1 has completed a line.");
                send_resp(sock, buf, OVER_PROT, fmtMsg);
            } else if(overState == 0) {
                games[*currentGame].gameState = STATE_GAME_OVER_DRAW;

                strcpy(fmtMsg->outcome, "D");
                strcpy(fmtMsg->reason, "The grid is full.");

                strcpy(games[*currentGame].lastProtInfo.outcome, fmtMsg->outcome);
                strcpy(games[*currentGame].lastProtInfo.reason, fmtMsg->reason);
                
                send_resp(sock, buf, OVER_PROT, fmtMsg);
            } else {
                games[*currentGame].gameState = STATE_WAITING_FOR_P2_MOVE;
                
                char newBoard[10];

                for(int row = 0; row < 3; row++) {
                    for(int col = 0; col < 3; col++) {
                        if(games[*currentGame].board[row][col] == 0) {
                            newBoard[(row * 3) + col] = '.';
                        } else if(games[*currentGame].board[row][col] == 'X') {
                            newBoard[(row * 3) + col] = 'X';
                        } else if(games[*currentGame].board[row][col] == 'O') {
                            newBoard[(row * 3) + col] = 'O';
                        }
                    }
                }

                newBoard[9] = '\0';
                
                strcpy(fmtMsg->board, newBoard);
                strcpy(games[*currentGame].lastProtInfo.board, fmtMsg->board);
                strcpy(games[*currentGame].lastProtInfo.role, fmtMsg->role);
                strcpy(games[*currentGame].lastProtInfo.position, fmtMsg->position);

                send_resp(sock, buf, MOVD_PROT, fmtMsg);
            }

            games[*currentGame].playerUnhandled = 2;
        }
    } else if(games[*currentGame].gameState == STATE_WAITING_FOR_P2_MOVE && msgType == MOVE_PROT && *player == 2) {
        if((strcmp(fmtMsg->role, "X") == 0 && *player != 1) || (strcmp(fmtMsg->role, "O") == 0 && *player != 2)) {
            strcpy(fmtMsg->reason, "!Error! invalid role type.");
            send_resp(sock, buf, INVL_PROT, fmtMsg);
            return;
        }

        if(maybeCommitBoardMove(fmtMsg->position, *currentGame, *player) == -1) {
            strcpy(fmtMsg->reason, "Error! invalid board move.");
            send_resp(sock, buf, INVL_PROT, fmtMsg);
        } else {
            int overState = isGameOver(games[*currentGame].board);

            if(overState == 2) {
                games[*currentGame].gameState = STATE_GAME_OVER_P2_WIN;

                strcpy(fmtMsg->outcome, "W");
                strcpy(fmtMsg->reason, "Player 2 has completed a line.");
                send_resp(sock, buf, OVER_PROT, fmtMsg);
            } else if(overState == 0) {
                games[*currentGame].gameState = STATE_GAME_OVER_DRAW;

                strcpy(fmtMsg->outcome, "D");
                strcpy(fmtMsg->reason, "The grid is full.");
                strcpy(games[*currentGame].lastProtInfo.outcome, fmtMsg->outcome);
                strcpy(games[*currentGame].lastProtInfo.reason, fmtMsg->reason);

                send_resp(sock, buf, OVER_PROT, fmtMsg);
            } else {
                games[*currentGame].gameState = STATE_WAITING_FOR_P1_MOVE;

                char newBoard[10];

                for(int row = 0; row < 3; row++) {
                    for(int col = 0; col < 3; col++) {
                        if(games[*currentGame].board[row][col] == 0) {
                            newBoard[(row * 3) + col] = '.';
                        } else if(games[*currentGame].board[row][col] == 'X') {
                            newBoard[(row * 3) + col] = 'X';
                        } else if(games[*currentGame].board[row][col] == 'O') {
                            newBoard[(row * 3) + col] = 'O';
                        }
                    }
                }

                newBoard[9] = '\0';
                
                strcpy(fmtMsg->board, newBoard);
                strcpy(games[*currentGame].lastProtInfo.board, fmtMsg->board);
                strcpy(games[*currentGame].lastProtInfo.role, fmtMsg->role);
                strcpy(games[*currentGame].lastProtInfo.position, fmtMsg->position);

                send_resp(sock, buf, MOVD_PROT, fmtMsg);
            }

            games[*currentGame].playerUnhandled = 1;
        }
    } else if((games[*currentGame].gameState == STATE_WAITING_FOR_P1_MOVE || games[*currentGame].gameState == STATE_BEGN || games[*currentGame].gameState == STATE_DRAW_REJ) && msgType == DRAW_PROT && *player == 1) {
        if(fmtMsg->message[0] == 'S') {
            games[*currentGame].gameState = STATE_WAITING_FOR_P2_DRAW_CONF;
            games[*currentGame].playerUnhandled = 2;
        } else {
            strcpy(fmtMsg->reason, "!Error! invalid draw initiation.");
            send_resp(sock, buf, INVL_PROT, fmtMsg);
        }
    } else if(games[*currentGame].gameState == STATE_WAITING_FOR_P2_MOVE && msgType == DRAW_PROT && *player == 2) {
        if(fmtMsg->message[0] == 'S') {
            games[*currentGame].gameState = STATE_WAITING_FOR_P1_DRAW_CONF;
            games[*currentGame].playerUnhandled = 1;
        } else {
            strcpy(fmtMsg->reason, "!Error! invalid draw initiation.");
            send_resp(sock, buf, INVL_PROT, fmtMsg);
        }

    } else if(games[*currentGame].gameState == STATE_WAITING_FOR_P1_DRAW_CONF && msgType == DRAW_PROT  && *player == 1) {
        if(fmtMsg->message[0] == 'A') {
            games[*currentGame].gameState = STATE_GAME_OVER_DRAW_AGG;
            
            strcpy(fmtMsg->outcome, "D");
            strcpy(fmtMsg->reason, "Both players have agreed to a draw. The game is over.");
            strcpy(games[*currentGame].lastProtInfo.outcome, fmtMsg->outcome);
            strcpy(games[*currentGame].lastProtInfo.reason, fmtMsg->reason);

            send_resp(sock, buf, OVER_PROT, fmtMsg);
            games[*currentGame].playerUnhandled = 2;

        } else if(fmtMsg->message[0] == 'R') {
            games[*currentGame].gameState = STATE_DRAW_REJ;
            games[*currentGame].playerUnhandled = 2;
        } else {
            strcpy(fmtMsg->reason, "!Error! invalid draw response.");
            send_resp(sock, buf, INVL_PROT, fmtMsg);
        }
    } else if(games[*currentGame].gameState == STATE_WAITING_FOR_P2_DRAW_CONF && msgType == DRAW_PROT  && *player == 2) {
        if(fmtMsg->message[0] == 'A') {
            games[*currentGame].gameState = STATE_GAME_OVER_DRAW_AGG;

            strcpy(fmtMsg->outcome, "D");
            strcpy(fmtMsg->reason, "Both players have agreed to a draw. The game is over.");
            strcpy(games[*currentGame].lastProtInfo.outcome, fmtMsg->outcome);
            strcpy(games[*currentGame].lastProtInfo.reason, fmtMsg->reason);

            send_resp(sock, buf, OVER_PROT, fmtMsg);

            games[*currentGame].playerUnhandled = 1;
        } else if(fmtMsg->message[0] == 'R') {
            games[*currentGame].gameState = STATE_DRAW_REJ;

            games[*currentGame].playerUnhandled = 1;
        } else {
            strcpy(fmtMsg->reason, "!Error! invalid draw response.");
            send_resp(sock, buf, INVL_PROT, fmtMsg);
        }
    } else {
        strcpy(fmtMsg->reason, "!Error! invalid protocol for current game state.");
        send_resp(sock, buf, INVL_PROT, fmtMsg);
    }

    free(fmtMsg);
}

void check_gs(int* currentGame) {
    printf("CURRENT STATE: %d\n", games[*currentGame].gameState);
}

void* thread_function(void* arg) {
    int sock = *((int *) arg);

    char buf[BUFSIZE + 1];

    pthread_mutex_lock(&games_lock);

    int currentGame = findOpenGame();
    if(currentGame == -1) {
        printf("No more free game space!");
    }

    pthread_mutex_unlock(&games_lock);

    int player = 0;

    while (1) {
        pthread_mutex_lock(&games_lock);

        if((games[currentGame].gameState == STATE_WAITING_FOR_P2_MOVE && games[currentGame].playerUnhandled == player && player == 2) || (games[currentGame].gameState == STATE_WAITING_FOR_P1_MOVE && games[currentGame].playerUnhandled == player && player == 1)) {
            send_resp(sock, buf, MOVD_PROT, &games[currentGame].lastProtInfo);

            games[currentGame].playerUnhandled = 0;
        } else if(games[currentGame].gameState == STATE_WAITING_FOR_P1_DRAW_CONF && games[currentGame].playerUnhandled == player && player == 1) {
            strcpy(games[currentGame].lastProtInfo.message, "S");
            send_resp(sock, buf, DRAW_PROT, &games[currentGame].lastProtInfo);

            games[currentGame].playerUnhandled = 0;
        } else if(games[currentGame].gameState == STATE_WAITING_FOR_P2_DRAW_CONF && games[currentGame].playerUnhandled == player && player == 2) {
            strcpy(games[currentGame].lastProtInfo.message, "S");
            send_resp(sock, buf, DRAW_PROT, &games[currentGame].lastProtInfo);

            games[currentGame].playerUnhandled = 0;
        } else if(games[currentGame].gameState == STATE_GAME_OVER_DRAW && games[currentGame].playerUnhandled == player) {
            strcpy(games[currentGame].lastProtInfo.outcome, "D");
            strcpy(games[currentGame].lastProtInfo.reason, "The grid is full.");

            send_resp(sock, buf, DRAW_PROT, &games[currentGame].lastProtInfo);
            games[currentGame].playerUnhandled = 0;
            break;
        } else if(games[currentGame].gameState == STATE_GAME_OVER_DRAW_AGG && games[currentGame].playerUnhandled == player) {
            strcpy(games[currentGame].lastProtInfo.outcome, "D");
            strcpy(games[currentGame].lastProtInfo.reason, "Both players have agreed to a draw. The game is over.");

            send_resp(sock, buf, OVER_PROT, &games[currentGame].lastProtInfo);
            games[currentGame].playerUnhandled = 0;
            break;
        } else if(games[currentGame].gameState == STATE_GAME_OVER_P1_WIN && games[currentGame].playerUnhandled == player) {
            strcpy(games[currentGame].lastProtInfo.outcome, "L");
            strcpy(games[currentGame].lastProtInfo.reason, "Player 1 has completed a line.");

            send_resp(sock, buf, OVER_PROT, &games[currentGame].lastProtInfo);
            games[currentGame].playerUnhandled = 0;
            break;
        } else if(games[currentGame].gameState == STATE_GAME_OVER_P2_WIN && games[currentGame].playerUnhandled == player) {
            strcpy(games[currentGame].lastProtInfo.outcome, "L");
            strcpy(games[currentGame].lastProtInfo.reason, "Player 2 has completed a line.");

            send_resp(sock, buf, OVER_PROT, &games[currentGame].lastProtInfo);
            games[currentGame].playerUnhandled = 0;
            break;
        } else if(games[currentGame].gameState == STATE_GAME_OVER_P1_RESIGN && games[currentGame].playerUnhandled == player) {
            strcpy(games[currentGame].lastProtInfo.outcome, "W");
            strcpy(games[currentGame].lastProtInfo.reason, "Player 1 has resigned.");

            send_resp(sock, buf, OVER_PROT, &games[currentGame].lastProtInfo);
            games[currentGame].playerUnhandled = 0;
            break;
        } else if(games[currentGame].gameState == STATE_GAME_OVER_P2_RESIGN && games[currentGame].playerUnhandled == player) {
            strcpy(games[currentGame].lastProtInfo.outcome, "W");
            strcpy(games[currentGame].lastProtInfo.reason, "Player 2 has resigned.");

            send_resp(sock, buf, OVER_PROT, &games[currentGame].lastProtInfo);
            games[currentGame].playerUnhandled = 0;
            break;
        } else if(games[currentGame].gameState == STATE_BEGN && games[currentGame].playerUnhandled == player) {
            strcpy(games[currentGame].lastProtInfo.name, games[currentGame].playerOneName);
            strcpy(games[currentGame].lastProtInfo.role, "X");
            send_resp(sock, buf, BEGN_PROT, &games[currentGame].lastProtInfo);
            games[currentGame].playerUnhandled = 0;
        } else if(games[currentGame].gameState == STATE_DRAW_REJ && games[currentGame].playerUnhandled == player) {
            strcpy(games[currentGame].lastProtInfo.message, "R");
            send_resp(sock, buf, DRAW_PROT, &games[currentGame].lastProtInfo);
            games[currentGame].playerUnhandled = 0;
        }

        if((games[currentGame].gameState == STATE_GAME_OVER_DRAW || games[currentGame].gameState == STATE_GAME_OVER_DRAW_AGG || games[currentGame].gameState == STATE_GAME_OVER_P1_RESIGN || games[currentGame].gameState == STATE_GAME_OVER_P2_RESIGN || games[currentGame].gameState == STATE_GAME_OVER_P1_WIN || games[currentGame].gameState == STATE_GAME_OVER_P2_WIN) && games[currentGame].playerUnhandled == 0) {
            games[currentGame].gameState = STATE_OVER_READY_FOR_NEW;
            break;
        }

        pthread_mutex_unlock(&games_lock);

        int recv_size = recv(sock, buf, BUFSIZE, MSG_DONTWAIT);
        if(recv_size <= 0) {        
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }

            pthread_mutex_lock(&games_lock);

            if(games[currentGame].gameState == STATE_WAITING_P2) {
                games[currentGame].gameState = STATE_OVER_READY_FOR_NEW;
            } else {
                if(player == 1) {
                    games[currentGame].gameState = STATE_GAME_OVER_P1_RESIGN;
                    games[currentGame].playerUnhandled = 2;
                } else {
                    games[currentGame].gameState = STATE_GAME_OVER_P2_RESIGN;
                    games[currentGame].playerUnhandled = 1;
                }


            }


            pthread_mutex_unlock(&games_lock);

            break;
        }

        buf[recv_size] = '\0';

        printf("Received request: %s\n", buf);

        pthread_mutex_lock(&games_lock);

        proc_msg(buf, &currentGame, &player, sock, buf);
        check_gs(&currentGame);

        pthread_mutex_unlock(&games_lock);

    }

    close(sock);
    pthread_exit(NULL);

    return NULL;
}

int main(int argc, char **argv)
{
    struct sockaddr_storage remote_host;
    socklen_t remote_host_len;
    
    int listener = open_listener(argv[1]);
    if (listener < 0) exit(EXIT_FAILURE);

    pthread_mutex_init(&games_lock, NULL);

    while(1) {
        remote_host_len = sizeof(remote_host);
        int sock = accept(listener, (struct sockaddr *)&remote_host, &remote_host_len);
        
        if (sock < 0) {
            perror("accept");
            continue;
        }

        for(int i = 0; i < THREADSIZE; i++) {
            printf("THREADS: %d %d %d %d\n", active_threads[0], active_threads[1], active_threads[2], active_threads[3]);

            if(active_threads[i]) {
                int result = pthread_tryjoin_np(threads[i], NULL);

                if (result == 0) {
                    active_threads[i] = 0;
                    close(sock);
                    printf("CLOSED THREAD\n");
                }
            }

            if(!active_threads[i]) {
                active_threads[i] = 1;

                printf("CREATING THREAD\n");

                pthread_create(&threads[i], NULL, thread_function, &sock);
                break;
            }
        }
    }

    fprintf(stderr, "Shutting down\n");
    close(listener);

    return EXIT_SUCCESS;
}

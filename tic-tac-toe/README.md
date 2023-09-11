# tic-tac-toe-online

## Group Members

- Arush Verma (aav87)
- Smail Barkouch (sb2108)

## Server Implementation

The server is a multithreaded program. Everytime a new request is made, a new thread is spinned up to account for the request.

In each thread, an event loop is running. This event loop will track different state changes, as well as sending requests back to the client. Everytime the server `recv`'s a message, it will process the message into an protocol type, and determine the protocol type's effect on the state.

The server keeps track of state using an array of `Game`s. This `Game` struct will contain information, that being: 
```
char playerOneName[100];
char playerTwoName[100];

int gameState;

int playerUnhandled;
ProtInfo lastProtInfo;

char board[3][3];
```

There are 1 of 14 states:
```
STATE_WAITING_P1=0
STATE_WAITING_P2=1
STATE_WAITING_FOR_P1_MOVE=2
STATE_WAITING_FOR_P2_MOVE=3
STATE_WAITING_FOR_P1_DRAW_CONF=4
STATE_WAITING_FOR_P2_DRAW_CONF=5
STATE_GAME_OVER_P1_WIN=6
STATE_GAME_OVER_P2_WIN=7
STATE_GAME_OVER_DRAW=8
STATE_GAME_OVER_P1_RESIGN=9
STATE_GAME_OVER_P2_RESIGN=10
STATE_GAME_OVER_DRAW_AGG=11
STATE_BEGN=12
STATE_DRAW_REJ=13
STATE_OVER_READY_FOR_NEW=14
```

There will always be 1 or 2 players for each game, no more. Game state is kept intact between threads using a `mutex`. Any changes to the game state will only be done with the lock is had.

The `playerUnhandled` field is used to alert one of the players of a state change, and to talk to its client appropriately.

If a player disconnects, they automatically forfeit (since the error from `recv` tells the thread they disconnected).

The server functions entirely, and keeps track of all possible states of the game. It handles disconnects, forefits, request and rejected draws, and wins by players.

## Testing Methods
For the purpose of this program, we have designed 3 test matchups to test game functions, server responses and catching malformed/deformed messages.

To run these programs, run ./tttx.o [server_name] [port] [fd] where fd is one of the text files with pre-determined instructions (fd is an integer 1-6 relating to the respective files).

Once the client file is connected, pressing the 'ENTER' key will run the line from the file. Some instructions are purposefully invalid (placing a token in an occupied grid cell, extra lines after the specified message length, etc). Therefore, a sequence to run the tests will also be provided below, to account for instances where the same client must re-send an instruction. The 'name of the client' specifies that 'ENTER' should be pressed in that order on the respective client. It is followed by the expected effect, for convenience.

The matchups are:

1. c1 vs c2
- Instruction Order

c1 - PLAY
c2 - PLAY
c1 - MOVES X 1,1
c2 - MOVES O 2,2
c1 - MOVES X 1,3
c2 - MOVES O 1,2
c1 - MOVES X 3,2
c2 - MOVES O 2,3
c1 - MOVES X 2,1
c2 - MOVES O 3,3
c1 - MOVES X 3,1

--- C1 WINS ---

2. c3 vs c4
- Instruction Order

c3 - PLAY 
c4 - PLAY
c3 - MOVES O 1,1 (invalid role, must re-do)
c3 - MOVES X 1,1 
c4- MOVES O 3,3
c3 - MOVES X 1,3
c4- MOVES O 3,1
c3 - SUGGESTS DRAW
c4- REJECTS DRAW
c3 - MOVES X 2,2
c4- MOVES O 3,2

 --- C4 WINS ---

3. c5 vs c6
- Instruction Order

c5 - PLAY (command has more chars than specified for testing --but program registers instruction and accepts the message.)
c6 - PLAY
c5 - MOVES X 2,2
c6 - MOVES O 3,2
c5 - MOVES X 2,2 (command has more chars than specified but program registers the right instruction and accepts the message. Proof: move is invalid as grid is already occupied, c5 must re-do) 
c5 - MOVES X 1,3
c6 - MOVES O 3,1
c5 - MOVES X 3,3
c6 - MOVES O 2,3
c5 - MOVES X 1,2
c6 - MOVES O 1,1
c5 - SUGGESTS DRAW
c6 - ACCEPTS DRAW

----GAME DRAWS----
 



Expected Server Response Sequence:
1. c1 vs c2

2. c3 vs c4

3. c5 vs c6


#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BOARD_SIZE 8
#define EMPTY 0
#define MY_FLAG 2
#define MY_KING 4
#define ENEMY_FLAG 1
#define ENEMY_KING 3
#define TRUE 1
#define FALSE 0
#define MAX_STEP 15
#define MAX_WAYS 50
#define MAX_LAYERS 32
#define START "START"
#define PLACE "PLACE"
#define TURN "TURN"
#define END "END"
#define RIVAL_BEST 0x7fffffff
#define MY_WORST -0x7fffffff
#define SCORE_PAWN 100
#define SCORE_KING 200
#define COLUMN_SCORE 1
#define TRIANGLE_SCORE 1
#define BREAK_TIME 1400


typedef int BOOL;
typedef const int kint;
typedef struct
{
	int x[MAX_STEP];
	int y[MAX_STEP];
	int numStep;
}Command;

typedef struct
{
	Command step[MAX_LAYERS];
	int numMove;
}Line;

typedef Command NodeMoves[MAX_WAYS];
typedef const char kBoard[BOARD_SIZE][BOARD_SIZE];	
typedef char Board[BOARD_SIZE][BOARD_SIZE];	

Board board = { EMPTY };
int myFlag;
clock_t startTime;
BOOL timeUp = FALSE;

//position value estimation
const char myPawnScore[BOARD_SIZE][BOARD_SIZE] =
{
	{0, 1, 0, 9, 0, 1, 0, 1},
	{1, 0, 3, 0, 3, 0, 3, 0},
	{0, 4, 0, 6, 0, 6, 0, 3},
	{6, 0, 7, 0, 7, 0, 5, 0},
	{0, 8, 0, 8, 0, 8, 0, 9},
	{7, 0, 8, 0, 8, 0, 8, 0},
	{0, 8, 0, 8, 0, 8, 0, 7},
	{0, 0, 0, 0, 0, 0, 0, 0}
};
const char myKingScore[BOARD_SIZE][BOARD_SIZE] =
{
	{0, 10, 0, 10, 0, 10, 0, 10},
	{20, 0, 20, 0, 20, 0, 20, 0},
	{0, 30, 0, 30, 0, 30, 0, 30},
	{40, 0, 40, 0, 40, 0, 40, 0},
	{0, 40, 0, 40, 0, 40, 0, 40},
	{30, 0, 30, 0, 30, 0, 30, 0},
	{0, 20, 0, 20, 0, 20, 0, 20},
	{0, 0, 0, 0, 0, 0, 0, 0}
};
const char rivalPawnScore[BOARD_SIZE][BOARD_SIZE] =
{
	{0, 0, 0, 0, 0, 0, 0, 0},
	{7, 0, 8, 0, 8, 0, 8, 0},
	{0, 8, 0, 8, 0, 8, 0, 7},
	{9, 0, 8, 0, 8, 0, 8, 0},
	{0, 5, 0, 7, 0, 7, 0, 6},
	{3, 0, 6, 0, 6, 0, 4, 0},
	{0, 3, 0, 3, 0, 3, 0, 1},
	{1, 0, 1, 0, 9, 0, 1, 0}
};
const char rivalKingScore[BOARD_SIZE][BOARD_SIZE] =
{
	{0, 0, 0, 0, 0, 0, 0, 0},
	{20, 0, 20, 0, 20, 0, 20, 0},
	{0, 30, 0, 30, 0, 30, 0, 30},
	{40, 0, 40, 0, 40, 0, 40, 0},
	{0, 40, 0, 40, 0, 40, 0, 40},
	{30, 0, 30, 0, 30, 0, 30, 0},
	{0, 20, 0, 20, 0, 20, 0, 20},
	{10, 0, 10, 0, 10, 0, 10, 0}
};

BOOL isInBound(kint x, kint y)
{
	return x >= 0 && x < BOARD_SIZE&& y >= 0 && y < BOARD_SIZE;
}

void rotateCommand(Command* cmd)
{
	if (myFlag == ENEMY_FLAG)
	{
		for (int i = 0; i < cmd->numStep; i++)
		{
			cmd->x[i] = BOARD_SIZE - 1 - cmd->x[i];
			cmd->y[i] = BOARD_SIZE - 1 - cmd->y[i];
		}
	}
}

//Find all moves with no capture.
void tryToMove(kint x, kint y, kBoard boardCpy, NodeMoves allMoves, int* pNumMove)
{
	int newX, newY, moveDir[4][2];
	int moveDirMy[4][2] = { {1, -1}, {1, 1}, {-1, -1}, {-1, 1} };
	int moveDirEnemy[4][2] = { {-1, -1}, {-1, 1}, {1, -1}, {1, 1} };
	if (boardCpy[x][y] == ENEMY_FLAG)
	{
		memcpy(moveDir, moveDirEnemy, sizeof(moveDir));
	}
	else
	{
		memcpy(moveDir, moveDirMy, sizeof(moveDir));
	}
	for (int i = 0; i < (boardCpy[x][y] + 1) / 2 + 1; i++)
	{
		newX = x + moveDir[i][0];
		newY = y + moveDir[i][1];
		if (isInBound(newX, newY) && boardCpy[newX][newY] == EMPTY)
		{
			(allMoves + *pNumMove)->numStep = 2;
			(allMoves + *pNumMove)->x[0] = x;
			(allMoves + *pNumMove)->y[0] = y;
			(allMoves + *pNumMove)->x[1] = newX;
			(allMoves + *pNumMove)->y[1] = newY;
			(*pNumMove)++;
		}
	}
}

//Find all captures
void tryToJump(kint x, kint y, kint currentStep, Board boardCpy, NodeMoves allMoves, Command* jumpCmd, int* pNumJump, kint flag)
{
	int jumpDir[4][2] = { {2, -2}, {2, 2}, {-2, -2}, {-2, 2} };
	int newX, newY, midX, midY;
	char tmpFlag;
	jumpCmd->x[currentStep] = x;
	jumpCmd->y[currentStep] = y;
	jumpCmd->numStep++;
	for (int i = 0; i < 4; i++)
	{
		newX = x + jumpDir[i][0];
		newY = y + jumpDir[i][1];
		midX = (x + newX) / 2;
		midY = (y + newY) / 2;
		if (isInBound(newX, newY) && (boardCpy[midX][midY] == 3 - flag || boardCpy[midX][midY] == 5 - flag) && (boardCpy[newX][newY] == EMPTY))
		{
			boardCpy[newX][newY] = boardCpy[x][y];
			boardCpy[x][y] = EMPTY;
			tmpFlag = boardCpy[midX][midY];
			boardCpy[midX][midY] = EMPTY;
			tryToJump(newX, newY, currentStep + 1, boardCpy, allMoves, jumpCmd, pNumJump, flag);
			boardCpy[x][y] = boardCpy[newX][newY];
			boardCpy[newX][newY] = EMPTY;
			boardCpy[midX][midY] = tmpFlag;
		}
	}
	//Only those that captures the most will be saved.
	if (jumpCmd->numStep > allMoves->numStep)
	{
		*pNumJump = 1;
		memset(allMoves, 0, sizeof(Command) * MAX_WAYS);
		memcpy(allMoves, jumpCmd, sizeof(Command));
	}
	else if (jumpCmd->numStep == allMoves->numStep && jumpCmd->numStep != 1)
	{
		memcpy(allMoves + *pNumJump, jumpCmd, sizeof(Command));
		(*pNumJump)++;
	}
	jumpCmd->numStep--;
}

void place(Command cmd)
{
	int midX, midY, curFlag;
	curFlag = board[cmd.x[0]][cmd.y[0]];

	for (int i = 0; i < cmd.numStep - 1; i++)
	{
		board[cmd.x[i]][cmd.y[i]] = EMPTY;
		board[cmd.x[i + 1]][cmd.y[i + 1]] = curFlag;
		if (abs(cmd.x[i] - cmd.x[i + 1]) == 2)
		{
			midX = (cmd.x[i] + cmd.x[i + 1]) / 2;
			midY = (cmd.y[i] + cmd.y[i + 1]) / 2;
			board[midX][midY] = EMPTY;
		}
	}

	for (int i = 0; i < BOARD_SIZE; i++)
	{
		if (board[0][i] == ENEMY_FLAG)
		{
			board[0][i] = ENEMY_KING;
		}
		if (board[BOARD_SIZE - 1][i] == MY_FLAG)
		{
			board[BOARD_SIZE - 1][i] = MY_KING;
		}
	}
}

//Chesspieces standing in a line are more defensive.
int columnScore(kint x, kint y, kBoard boardEv, kint flag)
{
	int score = 0;
	if (x == 0 || x == BOARD_SIZE - 1 || y == 0 || y == BOARD_SIZE - 1)
	{
		return score;
	}
	if (boardEv[x - 1][y + 1] == flag && boardEv[x + 1][y - 1] == flag && boardEv[x + 1][y + 1] == EMPTY && boardEv[x - 1][y + 1] == EMPTY)
	{
		score += COLUMN_SCORE;
	}
	else if (boardEv[x + 1][y + 1] == flag && boardEv[x - 1][y + 1] == flag && boardEv[x - 1][y + 1] == EMPTY && boardEv[x + 1][y - 1] == EMPTY)
	{
		score += COLUMN_SCORE;
	}
	return score;
}

//Chesspieces standing in a triangle formation are more aggressive.
int triangleScore(kint x, kint y, kBoard boardEv, kint flag)
{
	int score = 0;
	if (x == 0 || x == BOARD_SIZE - 1 || y == 0 || y == BOARD_SIZE - 1)
	{
		return score;
	}
	if (flag == MY_FLAG)
	{
		if (boardEv[x - 1][y + 1] == flag && boardEv[x - 1][y - 1] == flag && boardEv[x + 1][y + 1] == EMPTY && boardEv[x + 1][y - 1] == EMPTY)
		{
			score += TRIANGLE_SCORE;
		}
	}
	else if (flag == ENEMY_FLAG)
	{
		if (boardEv[x + 1][y + 1] == flag && boardEv[x + 1][y - 1] == flag && boardEv[x - 1][y + 1] == EMPTY && boardEv[x - 1][y - 1] == EMPTY)
		{
			score += TRIANGLE_SCORE;
		}
	}
	return score;
}

//Return estimation of a specific situation.
int evaluate(kBoard boardEv)
{
	int meScore = 0;
	int rivalScore = 0;
	for (int x = 0; x < BOARD_SIZE; x++)
	{
		for (int y = 0; y < BOARD_SIZE; y++)
		{
			switch (boardEv[x][y])
			{
				case ENEMY_FLAG:
					rivalScore += SCORE_PAWN + rivalPawnScore[x][y] + columnScore(x, y, boardEv, ENEMY_FLAG) + triangleScore(x, y, boardEv, ENEMY_FLAG);
					break;
				case MY_FLAG:
					meScore += SCORE_PAWN + myPawnScore[x][y] + columnScore(x, y, boardEv, MY_FLAG) + triangleScore(x, y, boardEv, MY_FLAG);
					break;
				case ENEMY_KING:
					rivalScore += SCORE_KING + rivalKingScore[x][y];
					break;
				case MY_KING:
					meScore += SCORE_KING + myKingScore[x][y];
					break;
				default:
					break;
			}
		}
	}

	clock_t curTime = clock();
	if (curTime - startTime > BREAK_TIME)
	{
		timeUp = TRUE;
	}
	return meScore - rivalScore;
}

//Find all legal moves(or captures) in a specific situation.
int getAllMoves(kBoard boardCurNode, NodeMoves allMoves, BOOL* canJump, kint flag)
{
	Command tmp;
	memset(&tmp, 0, sizeof(tmp));
	int numJump = 0, numMove = 0;
	allMoves->numStep = 1;
	char boardFind[BOARD_SIZE][BOARD_SIZE];
	memcpy(boardFind, boardCurNode, sizeof(boardFind));

	for (int x = 0; x < BOARD_SIZE; x++)
	{
		for (int y = 0; y < BOARD_SIZE; y++)
		{
			if (boardFind[x][y] > 0 && (boardFind[x][y] == flag || boardFind[x][y] == flag + 2)) {
				tryToJump(x, y, 0, boardFind, allMoves, &tmp, &numJump, flag);
			}
		}
	}

	if (numJump == 0)
	{
		*canJump = FALSE;
		for (int x = 0; x < BOARD_SIZE; x++)
		{
			for (int y = 0; y < BOARD_SIZE; y++)
			{
				if (boardFind[x][y] > 0 && (boardFind[x][y] == flag || boardFind[x][y] == flag + 2))
				{
					tryToMove(x, y, boardFind, allMoves, &numMove);
				}
			}
		}
	}
	return (numJump == 0 ? numMove : numJump);
}

//Rearrange the moves that have been found to make sure the principal variable is at the very first position.
void getPrincipalVar(NodeMoves allMoves, const Line* pPrincipleVar, kint maxLayer, kint currentLayer, kint numMove)
{
	for (int i = 0; i < numMove; i++)
	{
		if ((allMoves + i)->numStep == pPrincipleVar->step[maxLayer - currentLayer].numStep
			&& memcmp((allMoves + i)->x, pPrincipleVar->step[maxLayer - currentLayer].x, (allMoves + i)->numStep) == 0
			&& memcmp((allMoves + i)->y, pPrincipleVar->step[maxLayer - currentLayer].y, (allMoves + i)->numStep) == 0)
		{
			Command temp = *allMoves;
			*allMoves = *(allMoves + i);
			*(allMoves + i) = temp;
			break;
		}
	}
}

void simulateMove(Board boardCpy, BOOL canJump, Command* pCmd)
{
	int startX = pCmd->x[0], startY = pCmd->y[0];
	int endX = pCmd->x[pCmd->numStep - 1], endY = pCmd->y[pCmd->numStep - 1];
	boardCpy[endX][endY] = boardCpy[startX][startY];
	boardCpy[startX][startY] = EMPTY;
	if (canJump)
	{
		int midX, midY;
		for (int i = 0; i < pCmd->numStep - 1; i++)
		{
			midX = (pCmd->x[i] + pCmd->x[i + 1]) / 2;
			midY = (pCmd->y[i] + pCmd->y[i + 1]) / 2;
			boardCpy[midX][midY] = EMPTY;
		}
	}
	//promotion
	if (boardCpy[endX][endY] == MY_FLAG && endX == BOARD_SIZE - 1)
	{
		boardCpy[endX][endY] = MY_KING;	
	}
	else if (boardCpy[endX][endY] == ENEMY_FLAG && endX == 0)
	{
		boardCpy[endX][endY] = ENEMY_KING;
	}
}

int alphaBeta(kint maxLayer, kint currentlayer, int alpha, int beta, kBoard boardLast, const Line* pPrincipalVar, Line* principalVarLast, kint flag)
{
	Line principalVarNext;
	NodeMoves allMoves;
	BOOL canJump = TRUE;
	int score;
	char boardNext[BOARD_SIZE][BOARD_SIZE];
	memcpy(boardNext, boardLast, sizeof(boardNext));

	if (currentlayer == 0)
	{
		principalVarLast->numMove = 0;
		return evaluate(boardNext);
	}


	if (getAllMoves(boardNext, allMoves, &canJump, flag) > 0)
	{
		getPrincipalVar(allMoves, pPrincipalVar, maxLayer, currentlayer, numWays);
		for (int i = 0; i < numWays; i++)
		{
			memcpy(boardNext, boardLast, sizeof(boardNext));
			simulateMove(boardNext, canJump, allMoves + i);
			score = -alphaBeta(maxLayer, currentlayer - 1, -beta, -alpha, boardNext, pPrincipalVar, &principalVarNext, 3 - flag);

			if (timeUp)
			{
				return 0;
			}

			if (score >= beta)
			{
				return beta;
			}
			if (score > alpha)
			{
				principalVarLast->step[0] = allMoves[i];
				memcpy(principalVarLast->step + 1, principalVarNext.step, principalVarNext.numMove * sizeof(Command));
				principalVarLast->numMove = principalVarNext.numMove + 1;
				alpha = score;
			}
		}
		return alpha;
	}
	else
	{
		return MY_WORST + 1;
	}
}

void aiTurn(kBoard boardTurn, Command* command)
{
	startTime = clock();

	int curScore;
	Line newPrincipalVar;
	Line PrincipalVar;
	for (int maxLayer = 2; maxLayer <= MAX_LAYERS; maxLayer += 2)
	{
		memset(&newPrincipalVar, 0, sizeof(Line));
		curScore = alphaBeta(maxLayer, maxLayer, MY_WORST, RIVAL_BEST, boardTurn, &PrincipalVar, &newPrincipalVar, MY_FLAG);
		if (timeUp)
		{
			break;
		}
		memcpy(&PrincipalVar, &newPrincipalVar, sizeof(Line));
		if (curScore == RIVAL_BEST - 1)
		{
			break;
		}
	}
	*command = PrincipalVar.step[0];
	memset(&PrincipalVar, 0, sizeof(PrincipalVar));
	timeUp = FALSE;
}

void start()
{
	memset(board, 0, sizeof(board));
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 8; j += 2)
		{
			board[i][j + (i + 1) % 2] = MY_FLAG;
		}
	}
	for (int i = 5; i < 8; i++)
	{
		for (int j = 0; j < 8; j += 2)
		{
			board[i][j + (i + 1) % 2] = ENEMY_FLAG;
		}
	}
}

void turn()
{
	Command command =
	{
		.x = {0},
		.y = {0},
		.numStep = 0
	};
	aiTurn(board, &command);
	place(command);
	rotateCommand(&command);
	printf("%d", command.numStep);
	for (int i = 0; i < command.numStep; i++)
	{
		printf(" %d,%d", command.x[i], command.y[i]);
	}
	printf("\n");
	fflush(stdout);
}

void end(int x)
{
	exit(0);
}

//debug
void printBoard()
{
	char visualBoard[BOARD_SIZE][BOARD_SIZE + 1] = { 0 };
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		for (int j = 0; j < BOARD_SIZE; j++)
		{
			switch (board[i][j])
			{
				case EMPTY:
					visualBoard[i][j] = '=';
					break;
				case ENEMY_FLAG:
					visualBoard[i][j] = 'O';
					break;
				case MY_FLAG:
					visualBoard[i][j] = 'X';
					break;
				case ENEMY_KING:
					visualBoard[i][j] = '@';
					break;
				case MY_KING:
					visualBoard[i][j] = '*';
					break;
				default:
					break;
			}
		}
		printf("%s\n", visualBoard[i]);
	}
}

void loop()
{
	char tag[10] = { 0 };
	Command command =
	{
		.x = {0},
		.y = {0},
		.numStep = 0
	};
	int status;
	while (TRUE)
	{
		memset(tag, 0, sizeof(tag));
		scanf("%s", tag);
		if (strcmp(tag, START) == 0)
		{
			scanf("%d", &myFlag);
			start();
			printf("OK\n");
			fflush(stdout);
		}
		else if (strcmp(tag, PLACE) == 0)
		{
			scanf("%d", &command.numStep);
			for (int i = 0; i < command.numStep; i++)
			{
				scanf("%d,%d", &command.x[i], &command.y[i]);
			}
			rotateCommand(&command);
			place(command);
		}
		else if (strcmp(tag, TURN) == 0)
		{
			turn();
		}
		else if (strcmp(tag, END) == 0)
		{
			scanf("%d", &status);
			end(status);
		}
		//printBoard();
		//printf("\n");
	}
}

int main(int argc, char* argv[])
{
	loop();
	return 0;
}
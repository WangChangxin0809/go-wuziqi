
/* ===== rapfi_prelude.inc ===== */
/*
 * Rapfi 2018 single-file port, with rule adapter for the contest in 要求.txt.
 * Upstream: https://github.com/dhbloo/Rapfi-gomocup
 *
 * MIT License
 * Copyright (c) 2018 Haobin Duan
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <algorithm>
#include <cstring>
#include <cstdio>
#define sscanf_s std::sscanf
using WORD = unsigned short;
inline int GetStdHandle(int) { return 0; }
inline void SetConsoleTextAttribute(int, WORD) {}
constexpr int STD_OUTPUT_HANDLE = 0;
constexpr int BACKGROUND_INTENSITY = 0;
constexpr int FOREGROUND_RED = 0;
constexpr int FOREGROUND_GREEN = 0;
using std::reverse;


/* ===== Define.h ===== */
#define RapFi_Version "2018.02"
///#define VERSION_YIXIN_BOARD

#ifndef _DEBUG
#define NDEBUG
#endif

#include <assert.h>
#include <string>
#include <sstream>
#include <list>
#include <vector>
#include <array>
#include <set>
#include <algorithm>
#include <iterator>
#include <functional>
#include <random>
#include <ctime>
#include <iostream>

using std::string;
using std::vector;
using std::array;
using std::ostream;
using std::list;
using std::set;
using std::cout;
using std::endl;
using std::swap;

typedef char Int8;
typedef unsigned char UInt8;
typedef unsigned short UShort;
typedef unsigned int UInt;
typedef unsigned long ULong;
typedef unsigned long long U64;

template<class T1, class T2>
inline auto _max(const T1 a, const T2 b) {
	return (a > b) ? a : b;
}

template<class T1, class T2>
inline auto _min(const T1 a, const T2 b) {
	return (a < b) ? a : b;
}

template<class T>
inline auto _abs(const T a) {
	return (a < 0) ? -a : a;
}

#define MAX(a,b) _max(a,b)
#define MIN(a,b) _min(a,b)
#define ABS(a) _abs(a)

static std::mt19937_64 random(time(NULL));

inline void toupper(string & str) {
	for (size_t i = 0; i < str.size(); i++) {
		char &c = str[i];
		if (c >= 'a' && c <= 'z') {
			c += 'A' - 'a';
		}
	}
}

// ���ص�ǰʱ��(��λ:ms)
inline long getTime() {
	return clock() * 1000L / CLOCKS_PER_SEC;
}

#ifdef _DEBUG
#define DEBUGL(message) cout << message << endl
#define MESSAGEL(message) cout << message << endl
#define MESSAGES_BEGIN 
#define MESSAGES(message) cout << message
#define MESSAGES_END cout << endl
#define ANALYSIS(type, pos) ((void) 0)
#else
#define DEBUGL(message)
#define MESSAGEL(message) cout << "MESSAGE " << message << endl
#define MESSAGES_BEGIN cout << "MESSAGE "
#define MESSAGES(message) cout << message
#define MESSAGES_END cout << endl

#ifdef VERSION_YIXIN_BOARD
#define ANALYSIS(type, pos) if (depth >= 5) cout << "MESSAGE REALTIME " << type << ' ' << (int)CoordX(pos) << ',' << (int)CoordY(pos) << endl
#else
#define ANALYSIS(type, pos) ((void) 0)
#endif
#endif


/* ===== Board.h ===== */
#define MAX_BOARD_SIZE_BIT 5
#define BOARD_BOUNDARY 4

enum Piece : UInt8 {
	Black = 0, // 00(Bin)
	White = 1, // 01(Bin)
	Empty = 3, // 11(Bin)
	Wrong = 2  // 10(Bin)
};

inline ostream & operator<<(ostream & out, const Piece piece) {
	switch (piece) {
	case Empty: return out << "Empty";
	case Black: return out << "Black";
	case White: return out << "White";
	default: return out << "Wrong";
	}
}

inline Piece Opponent(const Piece piece) { return static_cast<Piece>(piece ^ (UInt8)1); }

typedef short Delta;
typedef UShort Pos;

const Pos NullPos = UShort(0);

//�ĸ�����ı仯
const Int8 D[4] = {
	1,                             //{ 0, 1 } = 0 * 32 + 1
	(1 << MAX_BOARD_SIZE_BIT) - 1, //{ 1,-1 } = 1 * 32 - 1
	(1 << MAX_BOARD_SIZE_BIT),     //{ 1, 0 } = 1 * 32 + 0
	(1 << MAX_BOARD_SIZE_BIT) + 1, //{ 1, 1 } = 1 * 32 + 1
};

const Int8 RANGE_NEIGHBOR[8] = {
	-D[3], -D[2], -D[1], -D[0],
	 D[0],  D[1],  D[2],  D[3],
};

const Int8 RANGE_MIN[16] = {
	-D[3] - D[3], -D[2] - D[2], -D[1] - D[1], 
	-D[3]       , -D[2]       , -D[1]       ,
	-D[0] - D[0], -D[0]       ,  D[0]       ,  D[0] + D[0],
	 D[1]       ,  D[2]       ,  D[3]       ,
	 D[1] + D[1],  D[2] + D[2],  D[3] + D[3],
};

const Int8 RANGE_MIDDLE[24] = {
	-D[3] - D[3], -D[3] - D[2], -D[2] - D[2], -D[2] - D[1], -D[1] - D[1],
	-D[3] - D[0], -D[3]       , -D[2]       , -D[1]       , -D[1] + D[0],
	-D[0] - D[0], -D[0]       ,                D[0]       ,  D[0] + D[0],
	 D[1] - D[0],  D[1]       ,  D[2]       ,  D[3]       ,  D[3] + D[0],
	 D[1] + D[1],  D[1] + D[2],  D[2] + D[2],  D[3] + D[2],  D[3] + D[3],
};

const Int8 RANGE_LARGE[32] = {
	-D[3] - D[3] - D[3], -D[2] - D[2] - D[2], -D[1] - D[1] - D[1],
	-D[3] - D[3], -D[3] - D[2], -D[2] - D[2], -D[2] - D[1], -D[1] - D[1],
	-D[3] - D[0], -D[3]       , -D[2]       , -D[1]       , -D[1] + D[0],
	-D[0] - D[0] - D[0], -D[0] - D[0], -D[0],                
	 D[0],  D[0] + D[0],  D[0] + D[0] + D[0],
	D[1] - D[0],  D[1]       ,  D[2]       ,  D[3]       ,  D[3] + D[0],
	D[1] + D[1],  D[1] + D[2],  D[2] + D[2],  D[3] + D[2],  D[3] + D[3],
	D[1] + D[1] + D[1], D[2] + D[2] + D[2], D[3] + D[3] + D[3]
};

const short RANGE_LINE_4[32] = {
	-D[3] - D[3] - D[3] - D[3], -D[2] - D[2] - D[2] - D[2], -D[1] - D[1] - D[1] - D[1],
		  - D[3] - D[3] - D[3],	      - D[2] - D[2] - D[2],		  - D[1] - D[1] - D[1],
		          -D[3] - D[3],	             - D[2] - D[2],              - D[1] - D[1],
	                    - D[3],                     - D[2],                     - D[1],
	-D[0] - D[0] - D[0] - D[0],       - D[0] - D[0] - D[0],              - D[0] - D[0], -D[0],
	                      D[0],                D[0] + D[0],         D[0] + D[0] + D[0],  D[0] + D[0] + D[0] + D[0],
						  D[1],                       D[2],                       D[3],
				   D[1] + D[1],                D[2] + D[2],                D[3] + D[3],
			D[1] + D[1] + D[1],         D[2] + D[2] + D[2],         D[3] + D[3] + D[3],
	 D[1] + D[1] + D[1] + D[1],  D[2] + D[2] + D[2] + D[2],  D[3] + D[3] + D[3] + D[3],
};

const short RANGE_4[40] = {
	-D[3] - D[3] - D[3] - D[3], -D[2] - D[2] - D[2] - D[2], -D[1] - D[1] - D[1] - D[1],
	       -D[3] - D[3] - D[3],        -D[2] - D[2] - D[2],        -D[1] - D[1] - D[1],
	-D[3] - D[3], -D[3] - D[2], -D[2] - D[2], -D[2] - D[1], -D[1] - D[1],
	-D[3] - D[0], -D[3]       , -D[2]       , -D[1]       , -D[1] + D[0],

	-D[0] - D[0] - D[0] - D[0], -D[0] - D[0] - D[0], -D[0] - D[0], -D[0],
	 D[0],  D[0] + D[0],  D[0] + D[0] + D[0],  D[0] + D[0] + D[0] + D[0],

	D[1] - D[0],  D[1]       ,  D[2]       ,  D[3]       ,  D[3] + D[0],
	D[1] + D[1],  D[1] + D[2],  D[2] + D[2],  D[3] + D[2],  D[3] + D[3],
	       D[1] + D[1] + D[1],         D[2] + D[2] + D[2],         D[3] + D[3] + D[3],
	D[1] + D[1] + D[1] + D[1],  D[2] + D[2] + D[2] + D[2],  D[3] + D[3] + D[3] + D[3],
};

inline Pos POS_R(UInt8 x = 0, UInt8 y = 0) { return (x << MAX_BOARD_SIZE_BIT) + y; }
inline Pos POS(UInt8 x = 0, UInt8 y = 0) { return POS_R(x + BOARD_BOUNDARY, y + BOARD_BOUNDARY); }
inline UInt8 CoordX(Pos p) { return (p >> MAX_BOARD_SIZE_BIT) - BOARD_BOUNDARY; }
inline UInt8 CoordY(Pos p) { return (p & ((1 << MAX_BOARD_SIZE_BIT) - 1)) - BOARD_BOUNDARY; }

struct PosStr {
private:
	Pos p;
public:
	PosStr(Pos p) : p(p) {}
	inline Pos operator()() { return p; }
};

inline ostream & operator<<(ostream & out, PosStr pos) {
#ifdef _DEBUG
	return out << "[" << char(CoordY(pos()) + 65) << "," << int(CoordX(pos()) + 1) << "]";
#else
	return out << "[" << int(CoordX(pos()) + 1) << "," << int(CoordY(pos()) + 1) << "]";
#endif
}

inline string YXPos(Pos pos, UInt8 boardSize) {
	std::ostringstream s;
	s << "[" << char(CoordY(pos) + 65) << "," << int(boardSize - CoordX(pos)) << "]";
	return s.str();
}

inline int distance(Pos p1, Pos p2) {
	return MAX(ABS(static_cast<int>(CoordX(p1)) - CoordX(p2)), ABS(static_cast<int>(CoordY(p1)) - CoordY(p2)));
}

inline bool isInLine(const Pos & p1, const Pos & p2) {
	//To-do: �Ż�
	if (CoordX(p1) == CoordX(p2) || CoordY(p1) == CoordY(p2)) return true;
	return ABS(static_cast<int>(CoordX(p1)) - CoordX(p2)) == ABS(static_cast<int>(CoordY(p1)) - CoordY(p2));
}

struct Move {
	Pos pos;
	int value;

	Move() : pos(NullPos), value(SHRT_MIN) {}
	Move(int x, int y, int value) : pos(POS(x, y)), value(value) {}
	Move(Pos pos, int value) : pos(pos), value(value) {}
	inline bool operator == (const Move & move) { return pos == move.pos; }
	inline bool operator < (const Move & move) const { return value < move.value; }
	inline bool operator > (const Move & move) const { return value > move.value; }
};

struct CandArea {
	UInt8 x0, y0, x1, y1;

	CandArea() : x0(255), y0(255), x1(0), y1(0) {}
	CandArea(UInt8 x0, UInt8 y0, UInt8 x1, UInt8 y1) : x0(x0), y0(y0), x1(x1), y1(y1) {}

	void expend(Pos p, UInt8 boardSize);
};

class Board {
public:
	static const UInt8 MaxBoardSize = 1 << MAX_BOARD_SIZE_BIT;
	static const int MaxBoardSizeSqr = MaxBoardSize * MaxBoardSize;
	static const UInt8 RealBoardSize = MaxBoardSize - 2 * BOARD_BOUNDARY;

private:
	Piece board[MaxBoardSizeSqr];
	UInt8 boardSize, center;
	Pos boardStartPos, boardEndPos;
	int boardSizeSqr;
	int moveCount = 0;
	int nullMoveCount = 0;
	Pos* historyMoves;

	Piece playerToMove = Black;
	Piece playerToMoveOppo = White;
	Piece playerWon = Empty;

	U64 zobrist[2][MaxBoardSizeSqr];
	U64 zobristKey;

	CandArea area;
	CandArea* historyAreas;

	bool check5InLine(Pos origin, Delta d, Piece p);
	inline void setPiece(Pos pos, Piece piece);
	inline void delPiece(Pos pos);
	void initZobrish();
	void initBoard();

public:
	Board(UInt8 boardSize_);
	~Board();

	void clear();
	
	void move(Pos pos);
	void undo();
	
	void muiltMove(Pos pos);
	void muiltUndo();
	inline void switchSide();

	void makeNullMove();
	void undoNullMove();

	inline Piece get(Pos pos) const;
	inline bool isInBoard(Pos pos) const;
	inline bool isNearBoard(Pos pos, int distFromBorder) const;
	inline bool isEmpty(Pos pos) const;
	inline bool isNullMoveAvailable() const { return nullMoveCount == 0; }
	bool checkWin();

	inline int getMoveCount() const { return moveCount; };
	inline int getMoveLeftCount() const { return boardSizeSqr - moveCount; };
	inline Piece getPlayerToMove() const { return playerToMove; }
	inline Piece getPlayerToMoveOppo() const { return playerToMoveOppo; }
	inline Piece getPlayerWon() const { return playerWon; }

	inline Pos getHistoryMove(int moveIndex) const;
	inline Pos getMoveBackWard(int backIndex) const;
	inline Pos getLastMove() const;

	inline U64 getZobristKey() const { return zobristKey; }
	inline UInt8 size() const { return boardSize; }
	inline UInt8 centerPos() const { return center; }
	inline int startPos() const { return boardStartPos; }
	inline int endPos() const { return boardEndPos; }
	inline int maxCells() const { return boardSizeSqr; }
	inline CandArea const & candArea() const { return area; }

	void expendCandArea(Pos pos, int expendWidth = 0);
};

inline Piece Board::get(Pos pos) const {
	assert(pos < MaxBoardSizeSqr);
	return board[pos];
}

inline bool Board::isInBoard(Pos pos) const {
	assert(pos < MaxBoardSizeSqr);
	return board[pos] != Wrong;
}

inline bool Board::isEmpty(Pos pos) const {
	return get(pos) == Empty;
}

inline bool Board::isNearBoard(Pos pos, int distFromBorder) const {
	return CoordX(pos) < distFromBorder || CoordY(pos) < distFromBorder || CoordX(pos) >= boardSize - distFromBorder || CoordY(pos) >= boardSize - distFromBorder;
}

inline Pos Board::getHistoryMove(int moveIndex) const {
	assert(moveIndex <= moveCount);
	return historyMoves[moveIndex - 1];
}
// backIndex�����0,��С�ڵ���moveCount(���򷵻طǷ��ŷ�)
inline Pos Board::getMoveBackWard(int backIndex) const {
	assert(backIndex > 0);
	backIndex = moveCount - backIndex;
	return backIndex < 0 ? NullPos : historyMoves[backIndex];
}

inline Pos Board::getLastMove() const {
	assert(moveCount > 0);
	return moveCount <= 0 ? NullPos : historyMoves[moveCount - 1];
}

inline void Board::switchSide() {
	playerToMove = Opponent(playerToMove);
	playerToMoveOppo = Opponent(playerToMoveOppo);
}

// Adapter for the contest's non-standard rule: black is forbidden to make an
// overline or two distinct fours, while double-three is legal.  A four is
// deduplicated by its exact set of four stones, so one open four is not
// mistakenly counted twice merely because it has two winning endpoints.
static bool contestForbiddenBlack(const Board &board, Pos move) {
	auto stone = [&](Pos p, Pos extra = NullPos) {
		return p == move || p == extra || board.get(p) == Black;
	};
	auto run = [&](Pos center, int dir, Pos extra = NullPos) {
		int n = 1;
		for (int sign : {-1, 1})
			for (int k = 1; ; ++k) {
				Pos p = Pos(int(center) + sign * k * D[dir]);
				if (!board.isInBoard(p) || !stone(p, extra)) break;
				++n;
			}
		return n;
	};
	for (int d = 0; d < 4; ++d)
		if (run(move, d) > 5) return true;
	for (int d = 0; d < 4; ++d)
		if (run(move, d) == 5) return false; // exact five has priority over 4-4

	U64 seen[40] = {};
	int seenCount = 0;
	for (int d = 0; d < 4; ++d) {
		for (int start = -4; start <= 0; ++start) {
			int stones = 0, empties = 0;
			Pos gap = NullPos;
			U64 signature = U64(d + 1) << 40;
			bool blocked = false;
			for (int k = 0; k < 5; ++k) {
				Pos p = Pos(int(move) + (start + k) * D[d]);
				if (!board.isInBoard(p) || board.get(p) == White) {
					blocked = true; break;
				}
				if (stone(p)) {
					signature |= U64(p) << (10 * stones++);
				} else {
					++empties; gap = p;
				}
			}
			if (blocked || stones != 4 || empties != 1 || run(gap, d, gap) != 5)
				continue;
			bool duplicate = false;
			for (int i = 0; i < seenCount; ++i) duplicate |= seen[i] == signature;
			if (!duplicate) seen[seenCount++] = signature;
		}
	}
	return seenCount >= 2;
}

static inline bool contestLegalMove(const Board &board, Pos p, Piece side) {
	return side != Black || !contestForbiddenBlack(board, p);
}

/* ===== Evaluator.h ===== */
#define FOR_EVERY_POSITION(x, y) \
    for (int x = 0; x < board->size(); x++) \
        for (int y = 0; y < board->size(); y++) \

#define FOR_EVERY_POSITION_POS(pos) \
    for (Pos pos = board->startPos(); pos <= board->endPos(); pos++) \
        if (board->isInBoard(pos)) \

#define FOR_EVERY_EMPTY_POS(pos) \
    for (Pos pos = board->startPos(); pos <= board->endPos(); pos++) \
        if (board->isEmpty(pos)) \

#define FOR_EVERY_CAND_POS(pos) \
    for (UInt8 _x = board->candArea().x0, y0 = board->candArea().y0, yl = board->candArea().y1 - y0, _y = yl; _x <= board->candArea().x1; _x++, _y = yl) \
        for (Pos pos = POS(_x, y0); _y < Board::MaxBoardSize; _y--, pos++) \
            if (board->isEmpty(pos) && cell(pos).isCand()) \

#define FOR_EVERY_PIECE_POS(pos) \
	for (int i = 0; i < board->getMoveCount() ? pos = board->getHistoryMove(i), true : false; i++) \

#define SELF (board->getPlayerToMove())
#define OPPO (board->getPlayerToMoveOppo())

enum Pattern : UInt8 {
	DEAD, 
	B1,   F1,
	B2J0, B2J2,                // S ��ʾ�����͵���С�ռ�(S < 5==DEAD, S���Ϊ9)
	F2J0, F2J1, F2J2,          // J ��ʾ�����͵���Ծ���
	B3J0, B3J1, B3J2,
	F3J0, F3J1,
	B4,   F4,
	F5
};

enum Pattern4 : UInt8 {
	A_FIVE = 11, B_FLEX4 = 10, C_BLOCK4_FLEX3 = 9, D_BLOCK4_PLUS = 8,
	E_BLOCK4 = 7, F_FLEX3_2X = 6, G_FLEX3_PLUS = 5, H_FLEX3 = 4,
	I_BLOCK3_PLUS = 3, J_FLEX2_2X = 2, FORBID = 1, NONE = 0
};

typedef short PatternCode;

class Evaluator {
private:
	void init();

	Pattern getPattern(UInt8 key1, UInt8 key2);
	Pattern shortLinePattern(array<Piece, 9> & line);
	Pattern checkFlex3(array<Piece, 9> & line, Pattern p1, Pattern p2);
	Pattern checkFlex4(array<Piece, 9> & line, Pattern p1, Pattern p2);
	bool checkFive(array<Piece, 9> & line, int i);
	Pattern getType(int length, int fullLength, int count, bool block, int jump);
	Pattern4 getPattern4(Pattern p1, Pattern p2, Pattern p3, Pattern p4);

	// for debug
	bool checkP4Count();

	static Pattern PATTERN[256][256];  // 65536 = 256 * 256 = 4 ^ 8
	static PatternCode PCODE[16][16][16][16];  // 65536 = 16 ^ 4
	
	// 3876 �Ǵ�16�ֵ���������ѡ�����ظ���4�����͵������
	static Pattern4 PATTERN4[3876];

protected:
	static short Score[3876];
	static short Value[3876];

	struct Cell {
		UInt8 key[4][2]; // 4������� Black Key �� White Key
		Pattern pattern[2][4];
		short score[2];
		short eval[2];
		Pattern4 pattern4[2];
		UInt8 cand;
		bool isLose;

		inline void clearPattern4() {
			pattern4[White] = pattern4[Black] = NONE;
		}
		inline void clearEval() {
			eval[Black] = eval[White] = 0;
		}
		inline void updatePattern(int i) {
			pattern[Black][i] = PATTERN[key[i][Black]][key[i][White]];
			pattern[White][i] = PATTERN[key[i][White]][key[i][Black]];
		}
		inline PatternCode getPatternCode(Piece piece) {
			return PCODE[pattern[piece][0]][pattern[piece][1]][pattern[piece][2]][pattern[piece][3]];
		}
		inline void updatePattern4(PatternCode codeBlack, PatternCode codeWhite) {
			pattern4[Black] = PATTERN4[codeBlack];
			pattern4[White] = PATTERN4[codeWhite];
		}
		inline void updatePattern4(Piece piece) {
			pattern4[piece] = PATTERN4[getPatternCode(piece)];
		}
		inline void updateScore(PatternCode codeBlack, PatternCode codeWhite) {
			score[Black] = Score[codeBlack];
			score[White] = Score[codeWhite];
		}
		inline void updateEval(PatternCode codeBlack, PatternCode codeWhite) {
			eval[Black] = Value[codeBlack]; 
			eval[White] = Value[codeWhite];
		}
		inline int getScore(Piece player) { return (int)score[Black] + score[White] + score[player]; }
		inline int getScore() { return (int)score[Black] + score[White]; }
		inline int getScore_VC(Piece player) { return (int)score[player]; }
		inline bool isCand() { return cand > 0; }
	};

	Board * board;
	Cell cells[Board::MaxBoardSizeSqr];

	int eval[2], evalLower[2];
	int p4Count[2][12];
	int ply;

	inline Cell & cell(Pos p) { assert(p < Board::MaxBoardSizeSqr); return cells[p]; }
	inline Cell & cell(int x, int y) { return cell(POS(x, y)); }

	Pos findPosByPattern4(Piece piece, Pattern4 p4);
	
	Pos getCostPosAgainstB4(Pos posB4, Piece piece);
	void getCostPosAgainstF3(Pos posB, Piece piece, vector<Move> & list);
	void getAllCostPosAgainstF3(Pos posB, Piece piece, set<Pos> & set);

	void expendCand(Pos pos, int fillDist, int lineDist);
	void clearLose();

public:
	Evaluator(Board * board);
	~Evaluator() {}

	enum MoveType { Normal, VC, MuiltVC };
/// #define GENERATION_MIN
/// #define GENERATION_MIDDLE
#define GENERATION_LARGE

	template <MoveType MT = Normal> void makeMove(Pos pos);
	template <MoveType MT = Normal> void undoMove();
	
	template <bool Make> inline void switchSide() {
		board->switchSide();
		if (Make) ply++;
		else ply--;
	}

	void newGame();
	Pos getHighestScoreCandPos();
	Pos databaseMove();

	// for debug
	void trace(ostream & ss, const string & appendBefore = "");
};

/* ===== Search.h ===== */
class HashTable;

enum WinState { State_Unknown, State_Win, State_Lose, State_Draw };

enum NodeType { NonPV = 0, PV = 1 };

enum GenLevel { InNone, InLine, InArea, InFullBoard };

struct MoveList {
	static const int MAX_MOVES = 128;
	enum Phase : UInt8 { HashMove, GenAllMoves, AllMoves };

	typedef vector<Move>::iterator MoveIterator;

	vector<Move> moves;
	Pos hashMove;
	Phase phase;
	size_t n;

	MoveList() {
		moves.reserve(MAX_MOVES);
		init();
	}
	inline void init(const Pos & hashMove_ = NullPos) {
		moves.clear();
		phase = HashMove;
		hashMove = hashMove_;
		n = 0;
	}
	inline void init_GenAllMoves() {
		moves.clear();
		phase = GenAllMoves;
		n = 0;
	}
	inline void addMove(Pos p, int value) { moves.emplace_back(p, value); }
	inline size_t moveCount() { return moves.size(); }
	inline MoveIterator begin() { return moves.begin(); }
	inline MoveIterator end() { return moves.end(); }
};

struct Line {
	list<Pos> moves;

	void clear() { moves.clear(); }
	void pushMove(Pos pos) { moves.push_back(pos); }
	friend ostream & operator<<(ostream & out, const Line & line) {
		for (Pos p : line.moves) { out << PosStr(p); }
		return out;
	}
	string YXPrint(UInt8 boardSize) {
		std::ostringstream s;
		for (Pos p : moves) { s << YXPos(p, boardSize); }
		return s.str();
	}
};

//Search Preset Config
#define Internal_Iterative_Deepening
#define Futility_Pruning
#define Razoring
///#define Late_Move_Reduction
///#define Singular_Extension

#define Win_Check_FLEX3_2X

#define Hash_Probe
#define Hash_Record

#define VCF_Leaf
///#define VCF_Branch_Limit

class AI : public Evaluator {
private:
	static const long TIME_RESERVED = 40;            // ms
	static const long TIME_RESERVED_PER_MOVE = 200;  // ms
	static const int MATCH_SPARE = 23;         // how much is time spared for the rest of game
	static const int MATCH_SPARE_MIN = 7;      // min time spared for the rest of game
	static const int MATCH_SPARE_MAX = 40;     // max time spared for the rest of game
	static const int TIMEOUT_PREVENT = 45;     // alphabeta become slow when depth increases
	static const int TIMEOUT_PREVENT_MIN = 70; // (TIMEOUT_PREVENT_MIN / 100.0)�ٷ���
	static const int BM_CHANGE_MIN = 3;        // if bestmove changes more than this, increase time
	static const int BM_CHANGE_MIN_DEPTH = 7;  // max bestmove changes taken into account
	static const int BM_STABLE_MIN = 3;        // if bestmove remains same more than this, decrease time
	static const int TIME_INCRESE_PERCENTAGE = 105;  // ʱ�����ӵİٷ���
	static const int TIME_DECREASE_PERCENTAGE = 90;  // ʱ����ٵİٷ���
	static const int TURNTIME_MIN_DIVISION = 3;      // min time for this turn

	static const int MAX_SEARCH_DEPTH = 64;    // ����ȫ��������������
	static const int MAX_PLY = 150;            // �����ܼ����������
	static const int MAX_WINNING_CHECK_BRANCH = 50;  // ��ľ��������ķ�֧

	static const int EXTENSION_NUM_BASE = 20;  // ����Ļ�׼��,Խ������Խ��
	float depthReductionBase = 1.f / logf((float)EXTENSION_NUM_BASE);

	static const int IID_MIN_DEPTH = 8;
	int IIDMinDepth = IID_MIN_DEPTH;

	static const int FUTILITY_MAX_DEPTH = 4;
	int FutilityDepth = FUTILITY_MAX_DEPTH;
	int FutilityMargin[4] = { 100, 160, 200, 250 };

	static const int RAZORING_MAX_DEPTH = 4;
	int RazoringDepth = RAZORING_MAX_DEPTH;
	int RazoringMargin[4] = { 150, 200, 250, 300 };

	float SEBetaMargin = 3.0f;

	// VCF ����
	static const int MAX_VCF_BRANCH = 10;
	static const int MAX_VCF_PLY = 36;

	// ���������������پ���
	static const int CONTINUES_NEIGHBOR = 2;       // ���ŷ�(ͬ��)������
	static const int CONTINUES_DISTANCE = 4;       // ���ŷ�(ͬ��)������
	static const int CONTINUES_DISTANCE_LARGE = 6; // ���ŷ�(����)������

	bool useOpeningBook = true;   // �Ƿ�ʹ�ÿ��ֿ�
	bool reloadConfig = false;    // �Ƿ���ÿ��˼��ǰ��������config
	int maxSearchDepth = MAX_SEARCH_DEPTH;

	Piece aiPiece;
	Piece attackerPiece;
	HashTable * hashTable = nullptr;
	MoveList moveLists[MAX_PLY];

	bool isPvExact[MAX_PLY] = { true, false };
	int rawStaticEval[MAX_PLY];
	int minEvalPly = 0;
	int singularExtensionPly = -1;   // �����������������Ĳ���
	Pos excludedMove = NullPos;
	Pattern4 lastSelfP4, lastOppoP4;

	long startTime;
	bool terminateAI;
	int maxPlyReached;
	int VCFMaxPly, VCTMaxPly;
	int BestMoveChangeCount;

	// ����ͳ�Ʊ���
	int node, nodeExpended;

	/////////////////////////////////////////////////////////////
	// ʱ����� Time Management

	inline long timeUsed() { return getTime() - startTime; }
	inline long timeLeft() {
#ifdef _DEBUG
		return INF;
#else
		return info.time_left;
#endif
	}
	inline long timeForTurn() {
		double timePercentage = (double)board->getMoveLeftCount() / board->maxCells();
		return MIN(info.timeout_turn, 
			timeLeft() / (MAX((long)round(MATCH_SPARE * timePercentage), MATCH_SPARE_MIN))) - TIME_RESERVED;
	}
	inline long timeForTurnMax() {
		return MIN(info.timeout_turn, timeLeft() / MATCH_SPARE_MIN) - TIME_RESERVED;
	}

	/////////////////////////////////////////////////////////////

	// ��������
	Pos fullSearch();
	Move alphabeta_root(int depth, int alpha, int beta);
	template <NodeType NT> int alphabeta(float depth, int alpha, int beta, bool cutNode);
	template <bool Root = true> int quickVCFSearch();

	/////////////////////////////////////////////////////////////

	// �ŷ�ѭ��,���ݷ���ȷ���Ƿ�Ӧ��������
	bool moveNext(MoveList & moveList, Pos & pos);
	
	// �ŷ�����
	WinState genMove_Root(MoveList & moveList);
	void genMoves(MoveList & moveList);
	void genMoves_defence(MoveList & moveList);
	void genMoves_VCF(MoveList & moveList);
	void genContinueMoves_VCF(MoveList & moveList, const short * range, int n);

	/////////////////////////////////////////////////////////////

	// �������֧��
	inline int getMaxBranch(int ply) {
		return MAX(64 - 2 * ply, 25);
	}
	// ����������
	inline void updateMaxPlyReached(int ply) {
		if (ply > maxPlyReached) maxPlyReached = ply;
	}
	// ���ݵ�ǰ��������ȵݼ���
	float getDepthReduction();
	// ���û�������ȡPV·��
	void fetchPVLineInTT(Line & line, Pos firstMove, int maxDepth = MAX_PLY);
	// �����ж�˫�����Ƿ��ʤ
	int quickDefenceCheck();

public:
	static const int INF = INT_MAX - 1;
	static const int WIN_MAX = 30000;
	static const int WIN_MIN = 29000;
	struct Info {
		long timeout_turn = 5 * 1000;   // millseconds
		long timeout_match = 100000 * 1000;
		long time_left = timeout_match;
		long max_memory = LONG_MAX;
		bool exact5 = false;
		bool renju = false;

		void setMaxMemory(long maxMemory) { max_memory = maxMemory ? maxMemory : LONG_MAX; }
	} info;

	AI(Board * board);
	~AI();

	void stopThinking() { terminateAI = true; }
	void clearHash();
	void setMaxDepth(int depth);
	Pos turnMove();

	inline int evaluate();
	inline int rawEvaluate();
	int quickWinCheck();

	void newGame();

	// Read Config File
	void tryReadConfig(string path);
	bool shouldReloadConfig() { return reloadConfig; }
};

/* ===== HashTable.h ===== */
#include <climits>

enum HashFlag : UInt8 {
	Hash_Unknown = 0,
	Hash_Alpha = 1,
	Hash_Beta = 2,
	Hash_PV = 3
};

/*
	TTEntry Struct (10 bytes)
	key32        32 bit
	value        16 bit
	depth         8 bit
	generation    6 bit
	flag          2 bit
	best         16 bit
*/

#pragma pack(push, 2)
class TTEntry {
private:
	friend class HashTable;
	UInt _key32;
	short _value;
	Int8 _depth;
	UInt8 _genFlag;
	Pos _best = NullPos;

	inline void saveGeneration(UInt8 generation) { _genFlag = flag() | generation; }
	inline void clear() {
		_genFlag = HashFlag::Hash_Unknown;
		_depth = 0;
		_key32 = 0;
		_best = NullPos;
	}

public:
	inline int value(int ply) const { return _value >= AI::WIN_MIN ? _value - ply : (_value <= -AI::WIN_MIN ? _value + ply : _value); }
	inline int depth() const { return _depth; }
	inline Pos bestPos() const { return _best; }
	inline Move bestMove(int ply) const { return Move(bestPos(), value(ply)); }
	inline HashFlag flag() const { return static_cast<HashFlag>(_genFlag & 3); }
	inline UInt8 generation() const { return static_cast<UInt8>(_genFlag & 0xFC); }

	inline bool isMate() { return _value >= AI::WIN_MIN || _value <= -AI::WIN_MIN; }
	inline bool isValid(int depth, int alpha, int beta, int ply) {
		bool mate = false;
		int value = _value;
		if (value >= AI::WIN_MIN) {
			value -= ply;
			mate = true;
		} else if (value <= -AI::WIN_MIN) {
			value += ply;
			mate = true;
		}
		if (mate || _depth >= depth) {
			HashFlag f = flag();
			return f == HashFlag::Hash_PV
				|| f == HashFlag::Hash_Alpha && value <= alpha
				|| f == HashFlag::Hash_Beta  && value >= beta;
		}
		return false;
	}

	inline void save(U64 key, const Move & move, int depth, HashFlag flag, int ply, UInt8 gen) {
		int value = move.value;
		assert(value >= -AI::WIN_MAX && value <= AI::WIN_MAX);
		assert(depth >= SCHAR_MIN && depth <= SCHAR_MAX);

		UInt newKey = static_cast<UInt>(key >> 32);
		// ��������Ҫ��entry
		if (_key32 == key && depth < _depth) return;   // ���С�Ĳ��ܸ�����ȴ��

		if (value >= AI::WIN_MIN) value += ply;
		else if (value <= -AI::WIN_MIN) value -= ply;

		_key32 = newKey;
		_value = static_cast<short>(value);
		_depth = static_cast<Int8>(depth);
		_genFlag = flag | gen;
		_best = move.pos;
	}
};
#pragma pack(pop)

class HashTable {
private:
	static const int CACHE_LINE_SIZE = 64;
	static const int CLUSTER_SIZE = 3;
	static const int DEFAULT_HASH_SIZE = 23;
	UInt hashSize;
	UInt hashSizeMask;

	struct Cluster {
		TTEntry entry[CLUSTER_SIZE];
		UInt8 padding[2];    //����Cache line (32 bytes)

		inline TTEntry * first_entry() { return entry; }
		inline void clear() {
			for (int i = 0; i < CLUSTER_SIZE; i++) entry[i].clear();
		}
	};
	Cluster* hashTable;
	UInt8 generation; // Size must be not bigger than TTEntry::_genFlag

	static_assert(CACHE_LINE_SIZE % sizeof(Cluster) == 0, "Cluster size incorrect");

public:
	HashTable(int size = DEFAULT_HASH_SIZE);
	~HashTable();

	void clearHash();
	void newSearch() { generation += 4; } // Lower 2 bits are used by Bound

	inline UInt8 getGeneration() { return generation; }

	bool probe(U64 key, TTEntry* & tte);
};



/* ===== MoveDatabase.cpp ===== */
// Opening Database
static char MoveDatabase[] = {
	/*15, 1, 4, 0, 0, 0, 1, 1, 2, 3, 2, 2, 4, 2, 5, 3, 3, 4, 3, 3, 5, 5, 4, 3, 3, 6, 4, 5, 6, 5, 5, 4, 3, 1,

	11, 1, 1, 1, 0, 0, 2, 2, 3, 2, 3, 3, 1, 3, 2, 4, 1, 5, 3, 3, 3, 2, 5, 1, 4, 3,
	11, 1, 1, 0, 0, 0, 1, 1, 3, 2, 2, 2, 2, 3, 3, 3, 2, 4, 2, 5, 1, 5, 1, 4, 0, 3,

	9, 1, 0, 0, 0, 3, 0, 1, 1, 2, 0, 2, 2, 3, 3, 3, 3, 4, 2, 4, 1, 1,
	9, 1, 1, 1, 0, 0, 2, 2, 3, 2, 3, 3, 2, 3, 1, 4, 1, 3, 1, 5, 2, 4,
	9, 1, 1, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 1, 2, 3, 1, 2, 0, 3, 1, 4,
	9, 1, 0, 0, 2, 0, 1, 0, 1, 1, 0, 1, 2, 2, 3, 2, 3, 3, 2, 3, 0, 2,
	9, 1, 1, 1, 0, 0, 2, 2, 0, 2, 2, 1, 3, 1, 1, 2, 1, 3, 0, 3, 3, 0,
	9, 1, 0, 0, 2, 0, 1, 1, 1, 2, 2, 1, 1, 4, 3, 2, 4, 3, 2, 3, 0, 1,

	8, 1, 0, 2, 1, 1, 1, 2, 2, 0, 2, 2, 3, 0, 3, 1, 3, 2, 4, 0,
	8, 1, 1, 1, 3, 0, 0, 2, 3, 1, 1, 3, 2, 2, 2, 3, 1, 4, 3, 3,

	7, 1, 0, 0, 1, 1, 0, 1, 2, 2, 3, 2, 3, 3, 2, 3, 0, 2,
	7, 1, 3, 2, 2, 1, 2, 2, 1, 1, 1, 0, 0, 0, 0, 1, 1, 2,
	7, 1, 0, 0, 0, 1, 1, 0, 0, 3, 2, 1, 3, 2, 1, 2, 1, -1,
	7, 1, 1, 0, 0, 1, 0, 2, 2, 2, 2, 1, 3, 2, 5, 2, 3, 0,
	7, 1, 0, 0, 0, 1, 2, 0, 0, 3, 2, 1, 1, 3, 1, 2, 2, 3,
	7, 1, 1, 0, 0, 0, 0, 1, 1, 1, 2, 2, 1, 2, 1, 3, 2, 3,
	7, 1, 0, 1, 1, 1, 2, 0, 0, 2, 1, 2, 3, 4, 2, 3, 2, 1,

	6, 1, 1, 0, 0, 0, 0, 2, 1, 1, 3, 3, 2, 2, 1, -1,
	6, 1, 1, 0, 0, 0, 1, 1, 2, 1, 2, 2, 1, 2, 0, 3,
	6, 1, 0, 0, 1, 0, 2, 1, 3, 1, 3, 2, 2, 2, 1, -1,
	6, 1, 3, 0, 0, 0, 1, 1, 2, 1, 2, 2, 1, 2, -1, 0,
	6, 1, 0, 0, 1, 1, 3, 1, 2, 2, 4, 3, 3, 3, 2, 4,
	6, 1, 0, 2, 0, 0, 1, 1, 1, 2, 2, 2, 2, 1, 3, 0,
	6, 1, 2, 1, 0, 0, 2, 2, 0, 1, 2, 3, 1, 2, 2, 0,
	6, 1, 2, 1, 0, 0, 1, 3, 0, 1, 2, 3, 1, 2, 2, 0,*/

	5, 2, 2, 2, 0, 0, 1, 0, 1, 1, 0, 1, 2, 1, 1, 2,
	5, 1, 0, 0, 2, 1, 0, 1, 2, 3, 1, 2, 1, 3,
	5, 1, 1, 0, 0, 0, 1, 1, 1, 2, 2, 1, 1, -1,
	5, 1, 1, 1, 1, 0, 0, 2, 1, 3, 1, 2, 2, 2,
	5, 1, 1, 0, 0, 0, 0, 1, 1, 1, 2, 3, 2, 2,
	5, 2, 0, 0, 1, 1, 1, 2, 0, 1, 2, 1, 2, 0, 3, 0,
	5, 1, 0, 2, 1, 0, 2, 0, 2, 1, 1, 1, -1, 3,
	5, 1, 1, 0, 1, 1, 0, 1, 2, 1, 3, 2, 2, -1,
	5, 1, 1, 1, 0, 0, 2, 1, 0, 2, 3, 1, 0, 1,
	5, 2, 1, 1, 0, 0, 2, 1, 1, 3, 1, 2, 0, 1, 3, 1,
	5, 2, 1, 0, 0, 0, 0, 1, 2, 1, 1, 1, 1, 2, -1, 2,
	5, 1, 1, 0, 1, 2, 0, 1, 3, 2, 2, 1, 2, -1,
	5, 2, 0, 1, 1, 2, 2, 1, 3, 0, 0, 3, 1, 1, 0, 2,
	5, 1, 2, 0, 0, 2, 1, 1, 1, 2, 2, 2, 0, 0,
	5, 1, 0, 0, 1, 1, 1, 0, 1, 2, 0, 2, 0, 1,

	4, 2, 1, 0, 0, 1, 1, 3, 1, 2, -1, 0, 2, 3,
	4, 1, 1, 0, 0, 1, 1, 1, 1, 2, -1, 0,
	4, 2, 1, 0, 1, 1, 0, 2, 2, 2, 0, 0, 0, 1,
	4, 1, 2, 0, 0, 0, 0, 2, 1, 1, 2, 2,
	4, 2, 0, 0, 1, 1, 3, 1, 2, 2, 2, 0, 2, -1,
	4, 3, 2, 0, 0, 0, 2, 2, 1, 1, 2, 1, 3, 2, 3, 0,
	4, 1, 2, 0, 0, 0, 2, 1, 1, 1, 2, 2,
	4, 1, 1, 0, 0, 1, 2, 0, 1, 2, 1, -1,
	4, 2, 1, 0, 0, 0, 0, 2, 1, 1, 2, 2, -1, -1,
	4, 1, 1, 0, 0, 1, 2, 2, 1, 2, -1, 0,
	4, 2, 0, 0, 1, 1, 2, 1, 2, 2, 0, -1, 1, -1,
	4, 1, 0, 0, 1, 0, 0, 2, 0, 1, -1, 2,

	3, 2, 0, 1, 0, 0, 1, 0, 2, -1, -1, 2,
	3, 1, 0, 0, 2, 0, 1, 1, 2, 2,
	3, 4, 1, 0, 0, 1, 1, 2, 0, 0, 1, 1, 2, 0, 2, 2,
	3, 1, 1, 2, 0, 0, 1, 1, 1, 3,
	3, 1, 1, 0, 0, 0, 1, 1, 1, 2,
	3, 3, 0, 0, 0, 2, 0, 1, 0, -1, 1, 0, -1, 0,
	3, 1, 0, 0, 2, 1, 1, 1, 2, 2,
	3, 1, 0, 0, 0, 1, 1, 2, 1, 1,
	3, 2, 0, 0, 2, 2, 1, 1, 2, 1, 1, 2,
	3, 3, 0, 1, 3, 0, 2, 0, 2, -1, 1, 2, 1, 1,
	3, 1, 0, 0, 2, 1, 2, 2, 1, 1,
	3, 4, 0, 0, 1, 1, 2, 2, 1, -1, 0, 1, 1, 2, 0, 2,
	3, 1, 0, 0, 0, 2, 1, 2, 1, 0,
	3, 2, 1, 0, 0, 3, 1, 2, 2, 3, 1, 1,
	3, 2, 0, 0, 0, 3, 0, 2, 1, 3, -1, 3,
	3, 3, 0, 0, 0, 3, 1, 2, 2, 2, 2, 3, 0, 2,
	3, 2, 1, 0, 0, 2, 1, 2, 1, 1, 1, 3,
	3, 3, 0, 0, 2, 1, 1, 2, 1, 0, 1, 3, 0, 2, 
	3, 2, 0, 0, 2, 2, 1, 2, 2, 3, 0, 3,
	3, 2, 0, 0, 3, 3, 2, 2, 3, 4, 4, 3,
	3, 3, 0, 0, 3, 2, 2, 2, 3, 3, 2, 3, 1, 1,
	3, 2, 0, 0, 3, 1, 2, 2, 3, 3, 1, 1,
	3, 5, 0, 0, 1, 0, 2, 0, 1, 1, 1, -1, -1, -1, -1, 1, 0, 1,
	3, 2, 0, 0, 3, 2, 2, 1, 2, 2, 1, 0,

	2, 9, 0, 0, 1, 1, 0, 2, 2, 0, 1, 2, 1, 0, 1, -1, 0, 1, -1, 1, 0, -2, 2, -1,
	2, 10, 0, 0, 1, 0, -1, -1, 0, -1, 1, -1, 2, 1, 0, 1, 1, 1, 2, 1, 2, 0, 0, 2, 0, -2,
	2, 4, 0, 0, 2, 2, 1, -1, -1, 1, 0, -1, -1, 0,

	1, 8, 0, 0, -1, 0, 1, 0, 0, 1, 0, -1, 1, 1, -1, 1, 1, -1, -1, -1,
	0, 0
};

/* ===== Config.cpp ===== */
short Evaluator::Score[3876] = {
	0,1,2,3,3,5,5,5,9,9,9,18,16,25,100,500,
	2,3,4,4,6,6,6,10,10,10,19,17,26,101,501,4,
	5,5,7,7,7,11,11,11,20,18,27,102,502,6,6,8,
	8,8,12,12,12,21,19,28,103,503,6,8,8,8,12,12,
	12,21,19,28,103,503,10,10,10,14,14,14,23,21,30,105,
	505,10,10,14,14,14,23,21,30,105,505,10,14,14,14,23,
	21,30,105,505,18,18,18,27,25,34,109,509,18,18,27,25,
	34,109,509,18,27,25,34,109,509,36,34,43,118,518,32,41,
	116,516,50,125,525,200,600,1000,3,4,5,5,7,7,7,11,
	11,11,20,18,27,102,502,5,6,6,8,8,8,12,12,12,
	21,19,28,103,503,7,7,9,9,9,13,13,13,22,20,29,
	104,504,7,9,9,9,13,13,13,22,20,29,104,504,11,11,
	11,15,15,15,24,22,31,106,506,11,11,15,15,15,24,22,
	31,106,506,11,15,15,15,24,22,31,106,506,19,19,19,28,
	26,35,110,510,19,19,28,26,35,110,510,19,28,26,35,110,
	510,37,35,44,119,519,33,42,117,517,51,126,526,201,601,1001,
	6,7,7,9,9,9,13,13,13,22,20,29,104,504,8,8,
	10,10,10,14,14,14,23,21,30,105,505,8,10,10,10,14,
	14,14,23,21,30,105,505,12,12,12,16,16,16,25,23,32,
	107,507,12,12,16,16,16,25,23,32,107,507,12,16,16,16,
	25,23,32,107,507,20,20,20,29,27,36,111,511,20,20,29,
	27,36,111,511,20,29,27,36,111,511,38,36,45,120,520,34,
	43,118,518,52,127,527,202,602,1002,9,9,11,11,11,15,15,
	15,24,22,31,106,506,9,11,11,11,15,15,15,24,22,31,
	106,506,13,13,13,17,17,17,26,24,33,108,508,13,13,17,
	17,17,26,24,33,108,508,13,17,17,17,26,24,33,108,508,
	21,21,21,30,28,37,112,512,21,21,30,28,37,112,512,21,
	30,28,37,112,512,39,37,46,121,521,35,44,119,519,53,128,
	528,203,603,1003,9,11,11,11,15,15,15,24,22,31,106,506,
	13,13,13,17,17,17,26,24,33,108,508,13,13,17,17,17,
	26,24,33,108,508,13,17,17,17,26,24,33,108,508,21,21,
	21,30,28,37,112,512,21,21,30,28,37,112,512,21,30,28,
	37,112,512,39,37,46,121,521,35,44,119,519,53,128,528,203,
	603,1003,15,15,15,19,19,19,28,26,35,110,510,15,15,19,
	19,19,28,26,35,110,510,15,19,19,19,28,26,35,110,510,
	23,23,23,32,30,39,114,514,23,23,32,30,39,114,514,23,
	32,30,39,114,514,41,39,48,123,523,37,46,121,521,55,130,
	530,205,605,1005,15,15,19,19,19,28,26,35,110,510,15,19,
	19,19,28,26,35,110,510,23,23,23,32,30,39,114,514,23,
	23,32,30,39,114,514,23,32,30,39,114,514,41,39,48,123,
	523,37,46,121,521,55,130,530,205,605,1005,15,19,19,19,28,
	26,35,110,510,23,23,23,32,30,39,114,514,23,23,32,30,
	39,114,514,23,32,30,39,114,514,41,39,48,123,523,37,46,
	121,521,55,130,530,205,605,1005,27,27,27,36,34,43,118,518,
	27,27,36,34,43,118,518,27,36,34,43,118,518,45,43,52,
	127,527,41,50,125,525,59,134,534,209,609,1009,27,27,36,34,
	43,118,518,27,36,34,43,118,518,45,43,52,127,527,41,50,
	125,525,59,134,534,209,609,1009,27,36,34,43,118,518,45,43,
	52,127,527,41,50,125,525,59,134,534,209,609,1009,54,52,61,
	136,536,50,59,134,534,68,143,543,218,618,1018,48,57,132,532,
	66,141,541,216,616,1016,75,150,550,225,625,1025,300,700,1100,1500,
	4,5,6,6,8,8,8,12,12,12,21,19,28,103,503,6,
	7,7,9,9,9,13,13,13,22,20,29,104,504,8,8,10,
	10,10,14,14,14,23,21,30,105,505,8,10,10,10,14,14,
	14,23,21,30,105,505,12,12,12,16,16,16,25,23,32,107,
	507,12,12,16,16,16,25,23,32,107,507,12,16,16,16,25,
	23,32,107,507,20,20,20,29,27,36,111,511,20,20,29,27,
	36,111,511,20,29,27,36,111,511,38,36,45,120,520,34,43,
	118,518,52,127,527,202,602,1002,7,8,8,10,10,10,14,14,
	14,23,21,30,105,505,9,9,11,11,11,15,15,15,24,22,
	31,106,506,9,11,11,11,15,15,15,24,22,31,106,506,13,
	13,13,17,17,17,26,24,33,108,508,13,13,17,17,17,26,
	24,33,108,508,13,17,17,17,26,24,33,108,508,21,21,21,
	30,28,37,112,512,21,21,30,28,37,112,512,21,30,28,37,
	112,512,39,37,46,121,521,35,44,119,519,53,128,528,203,603,
	1003,10,10,12,12,12,16,16,16,25,23,32,107,507,10,12,
	12,12,16,16,16,25,23,32,107,507,14,14,14,18,18,18,
	27,25,34,109,509,14,14,18,18,18,27,25,34,109,509,14,
	18,18,18,27,25,34,109,509,22,22,22,31,29,38,113,513,
	22,22,31,29,38,113,513,22,31,29,38,113,513,40,38,47,
	122,522,36,45,120,520,54,129,529,204,604,1004,10,12,12,12,
	16,16,16,25,23,32,107,507,14,14,14,18,18,18,27,25,
	34,109,509,14,14,18,18,18,27,25,34,109,509,14,18,18,
	18,27,25,34,109,509,22,22,22,31,29,38,113,513,22,22,
	31,29,38,113,513,22,31,29,38,113,513,40,38,47,122,522,
	36,45,120,520,54,129,529,204,604,1004,16,16,16,20,20,20,
	29,27,36,111,511,16,16,20,20,20,29,27,36,111,511,16,
	20,20,20,29,27,36,111,511,24,24,24,33,31,40,115,515,
	24,24,33,31,40,115,515,24,33,31,40,115,515,42,40,49,
	124,524,38,47,122,522,56,131,531,206,606,1006,16,16,20,20,
	20,29,27,36,111,511,16,20,20,20,29,27,36,111,511,24,
	24,24,33,31,40,115,515,24,24,33,31,40,115,515,24,33,
	31,40,115,515,42,40,49,124,524,38,47,122,522,56,131,531,
	206,606,1006,16,20,20,20,29,27,36,111,511,24,24,24,33,
	31,40,115,515,24,24,33,31,40,115,515,24,33,31,40,115,
	515,42,40,49,124,524,38,47,122,522,56,131,531,206,606,1006,
	28,28,28,37,35,44,119,519,28,28,37,35,44,119,519,28,
	37,35,44,119,519,46,44,53,128,528,42,51,126,526,60,135,
	535,210,610,1010,28,28,37,35,44,119,519,28,37,35,44,119,
	519,46,44,53,128,528,42,51,126,526,60,135,535,210,610,1010,
	28,37,35,44,119,519,46,44,53,128,528,42,51,126,526,60,
	135,535,210,610,1010,55,53,62,137,537,51,60,135,535,69,144,
	544,219,619,1019,49,58,133,533,67,142,542,217,617,1017,76,151,
	551,226,626,1026,301,701,1101,1501,8,9,9,11,11,11,15,15,
	15,24,22,31,106,506,10,10,12,12,12,16,16,16,25,23,
	32,107,507,10,12,12,12,16,16,16,25,23,32,107,507,14,
	14,14,18,18,18,27,25,34,109,509,14,14,18,18,18,27,
	25,34,109,509,14,18,18,18,27,25,34,109,509,22,22,22,
	31,29,38,113,513,22,22,31,29,38,113,513,22,31,29,38,
	113,513,40,38,47,122,522,36,45,120,520,54,129,529,204,604,
	1004,11,11,13,13,13,17,17,17,26,24,33,108,508,11,13,
	13,13,17,17,17,26,24,33,108,508,15,15,15,19,19,19,
	28,26,35,110,510,15,15,19,19,19,28,26,35,110,510,15,
	19,19,19,28,26,35,110,510,23,23,23,32,30,39,114,514,
	23,23,32,30,39,114,514,23,32,30,39,114,514,41,39,48,
	123,523,37,46,121,521,55,130,530,205,605,1005,11,13,13,13,
	17,17,17,26,24,33,108,508,15,15,15,19,19,19,28,26,
	35,110,510,15,15,19,19,19,28,26,35,110,510,15,19,19,
	19,28,26,35,110,510,23,23,23,32,30,39,114,514,23,23,
	32,30,39,114,514,23,32,30,39,114,514,41,39,48,123,523,
	37,46,121,521,55,130,530,205,605,1005,17,17,17,21,21,21,
	30,28,37,112,512,17,17,21,21,21,30,28,37,112,512,17,
	21,21,21,30,28,37,112,512,25,25,25,34,32,41,116,516,
	25,25,34,32,41,116,516,25,34,32,41,116,516,43,41,50,
	125,525,39,48,123,523,57,132,532,207,607,1007,17,17,21,21,
	21,30,28,37,112,512,17,21,21,21,30,28,37,112,512,25,
	25,25,34,32,41,116,516,25,25,34,32,41,116,516,25,34,
	32,41,116,516,43,41,50,125,525,39,48,123,523,57,132,532,
	207,607,1007,17,21,21,21,30,28,37,112,512,25,25,25,34,
	32,41,116,516,25,25,34,32,41,116,516,25,34,32,41,116,
	516,43,41,50,125,525,39,48,123,523,57,132,532,207,607,1007,
	29,29,29,38,36,45,120,520,29,29,38,36,45,120,520,29,
	38,36,45,120,520,47,45,54,129,529,43,52,127,527,61,136,
	536,211,611,1011,29,29,38,36,45,120,520,29,38,36,45,120,
	520,47,45,54,129,529,43,52,127,527,61,136,536,211,611,1011,
	29,38,36,45,120,520,47,45,54,129,529,43,52,127,527,61,
	136,536,211,611,1011,56,54,63,138,538,52,61,136,536,70,145,
	545,220,620,1020,50,59,134,534,68,143,543,218,618,1018,77,152,
	552,227,627,1027,302,702,1102,1502,12,12,14,14,14,18,18,18,
	27,25,34,109,509,12,14,14,14,18,18,18,27,25,34,109,
	509,16,16,16,20,20,20,29,27,36,111,511,16,16,20,20,
	20,29,27,36,111,511,16,20,20,20,29,27,36,111,511,24,
	24,24,33,31,40,115,515,24,24,33,31,40,115,515,24,33,
	31,40,115,515,42,40,49,124,524,38,47,122,522,56,131,531,
	206,606,1006,12,14,14,14,18,18,18,27,25,34,109,509,16,
	16,16,20,20,20,29,27,36,111,511,16,16,20,20,20,29,
	27,36,111,511,16,20,20,20,29,27,36,111,511,24,24,24,
	33,31,40,115,515,24,24,33,31,40,115,515,24,33,31,40,
	115,515,42,40,49,124,524,38,47,122,522,56,131,531,206,606,
	1006,18,18,18,22,22,22,31,29,38,113,513,18,18,22,22,
	22,31,29,38,113,513,18,22,22,22,31,29,38,113,513,26,
	26,26,35,33,42,117,517,26,26,35,33,42,117,517,26,35,
	33,42,117,517,44,42,51,126,526,40,49,124,524,58,133,533,
	208,608,1008,18,18,22,22,22,31,29,38,113,513,18,22,22,
	22,31,29,38,113,513,26,26,26,35,33,42,117,517,26,26,
	35,33,42,117,517,26,35,33,42,117,517,44,42,51,126,526,
	40,49,124,524,58,133,533,208,608,1008,18,22,22,22,31,29,
	38,113,513,26,26,26,35,33,42,117,517,26,26,35,33,42,
	117,517,26,35,33,42,117,517,44,42,51,126,526,40,49,124,
	524,58,133,533,208,608,1008,30,30,30,39,37,46,121,521,30,
	30,39,37,46,121,521,30,39,37,46,121,521,48,46,55,130,
	530,44,53,128,528,62,137,537,212,612,1012,30,30,39,37,46,
	121,521,30,39,37,46,121,521,48,46,55,130,530,44,53,128,
	528,62,137,537,212,612,1012,30,39,37,46,121,521,48,46,55,
	130,530,44,53,128,528,62,137,537,212,612,1012,57,55,64,139,
	539,53,62,137,537,71,146,546,221,621,1021,51,60,135,535,69,
	144,544,219,619,1019,78,153,553,228,628,1028,303,703,1103,1503,12,
	14,14,14,18,18,18,27,25,34,109,509,16,16,16,20,20,
	20,29,27,36,111,511,16,16,20,20,20,29,27,36,111,511,
	16,20,20,20,29,27,36,111,511,24,24,24,33,31,40,115,
	515,24,24,33,31,40,115,515,24,33,31,40,115,515,42,40,
	49,124,524,38,47,122,522,56,131,531,206,606,1006,18,18,18,
	22,22,22,31,29,38,113,513,18,18,22,22,22,31,29,38,
	113,513,18,22,22,22,31,29,38,113,513,26,26,26,35,33,
	42,117,517,26,26,35,33,42,117,517,26,35,33,42,117,517,
	44,42,51,126,526,40,49,124,524,58,133,533,208,608,1008,18,
	18,22,22,22,31,29,38,113,513,18,22,22,22,31,29,38,
	113,513,26,26,26,35,33,42,117,517,26,26,35,33,42,117,
	517,26,35,33,42,117,517,44,42,51,126,526,40,49,124,524,
	58,133,533,208,608,1008,18,22,22,22,31,29,38,113,513,26,
	26,26,35,33,42,117,517,26,26,35,33,42,117,517,26,35,
	33,42,117,517,44,42,51,126,526,40,49,124,524,58,133,533,
	208,608,1008,30,30,30,39,37,46,121,521,30,30,39,37,46,
	121,521,30,39,37,46,121,521,48,46,55,130,530,44,53,128,
	528,62,137,537,212,612,1012,30,30,39,37,46,121,521,30,39,
	37,46,121,521,48,46,55,130,530,44,53,128,528,62,137,537,
	212,612,1012,30,39,37,46,121,521,48,46,55,130,530,44,53,
	128,528,62,137,537,212,612,1012,57,55,64,139,539,53,62,137,
	537,71,146,546,221,621,1021,51,60,135,535,69,144,544,219,619,
	1019,78,153,553,228,628,1028,303,703,1103,1503,20,20,20,24,24,
	24,33,31,40,115,515,20,20,24,24,24,33,31,40,115,515,
	20,24,24,24,33,31,40,115,515,28,28,28,37,35,44,119,
	519,28,28,37,35,44,119,519,28,37,35,44,119,519,46,44,
	53,128,528,42,51,126,526,60,135,535,210,610,1010,20,20,24,
	24,24,33,31,40,115,515,20,24,24,24,33,31,40,115,515,
	28,28,28,37,35,44,119,519,28,28,37,35,44,119,519,28,
	37,35,44,119,519,46,44,53,128,528,42,51,126,526,60,135,
	535,210,610,1010,20,24,24,24,33,31,40,115,515,28,28,28,
	37,35,44,119,519,28,28,37,35,44,119,519,28,37,35,44,
	119,519,46,44,53,128,528,42,51,126,526,60,135,535,210,610,
	1010,32,32,32,41,39,48,123,523,32,32,41,39,48,123,523,
	32,41,39,48,123,523,50,48,57,132,532,46,55,130,530,64,
	139,539,214,614,1014,32,32,41,39,48,123,523,32,41,39,48,
	123,523,50,48,57,132,532,46,55,130,530,64,139,539,214,614,
	1014,32,41,39,48,123,523,50,48,57,132,532,46,55,130,530,
	64,139,539,214,614,1014,59,57,66,141,541,55,64,139,539,73,
	148,548,223,623,1023,53,62,137,537,71,146,546,221,621,1021,80,
	155,555,230,630,1030,305,705,1105,1505,20,20,24,24,24,33,31,
	40,115,515,20,24,24,24,33,31,40,115,515,28,28,28,37,
	35,44,119,519,28,28,37,35,44,119,519,28,37,35,44,119,
	519,46,44,53,128,528,42,51,126,526,60,135,535,210,610,1010,
	20,24,24,24,33,31,40,115,515,28,28,28,37,35,44,119,
	519,28,28,37,35,44,119,519,28,37,35,44,119,519,46,44,
	53,128,528,42,51,126,526,60,135,535,210,610,1010,32,32,32,
	41,39,48,123,523,32,32,41,39,48,123,523,32,41,39,48,
	123,523,50,48,57,132,532,46,55,130,530,64,139,539,214,614,
	1014,32,32,41,39,48,123,523,32,41,39,48,123,523,50,48,
	57,132,532,46,55,130,530,64,139,539,214,614,1014,32,41,39,
	48,123,523,50,48,57,132,532,46,55,130,530,64,139,539,214,
	614,1014,59,57,66,141,541,55,64,139,539,73,148,548,223,623,
	1023,53,62,137,537,71,146,546,221,621,1021,80,155,555,230,630,
	1030,305,705,1105,1505,20,24,24,24,33,31,40,115,515,28,28,
	28,37,35,44,119,519,28,28,37,35,44,119,519,28,37,35,
	44,119,519,46,44,53,128,528,42,51,126,526,60,135,535,210,
	610,1010,32,32,32,41,39,48,123,523,32,32,41,39,48,123,
	523,32,41,39,48,123,523,50,48,57,132,532,46,55,130,530,
	64,139,539,214,614,1014,32,32,41,39,48,123,523,32,41,39,
	48,123,523,50,48,57,132,532,46,55,130,530,64,139,539,214,
	614,1014,32,41,39,48,123,523,50,48,57,132,532,46,55,130,
	530,64,139,539,214,614,1014,59,57,66,141,541,55,64,139,539,
	73,148,548,223,623,1023,53,62,137,537,71,146,546,221,621,1021,
	80,155,555,230,630,1030,305,705,1105,1505,36,36,36,45,43,52,
	127,527,36,36,45,43,52,127,527,36,45,43,52,127,527,54,
	52,61,136,536,50,59,134,534,68,143,543,218,618,1018,36,36,
	45,43,52,127,527,36,45,43,52,127,527,54,52,61,136,536,
	50,59,134,534,68,143,543,218,618,1018,36,45,43,52,127,527,
	54,52,61,136,536,50,59,134,534,68,143,543,218,618,1018,63,
	61,70,145,545,59,68,143,543,77,152,552,227,627,1027,57,66,
	141,541,75,150,550,225,625,1025,84,159,559,234,634,1034,309,709,
	1109,1509,36,36,45,43,52,127,527,36,45,43,52,127,527,54,
	52,61,136,536,50,59,134,534,68,143,543,218,618,1018,36,45,
	43,52,127,527,54,52,61,136,536,50,59,134,534,68,143,543,
	218,618,1018,63,61,70,145,545,59,68,143,543,77,152,552,227,
	627,1027,57,66,141,541,75,150,550,225,625,1025,84,159,559,234,
	634,1034,309,709,1109,1509,36,45,43,52,127,527,54,52,61,136,
	536,50,59,134,534,68,143,543,218,618,1018,63,61,70,145,545,
	59,68,143,543,77,152,552,227,627,1027,57,66,141,541,75,150,
	550,225,625,1025,84,159,559,234,634,1034,309,709,1109,1509,72,70,
	79,154,554,68,77,152,552,86,161,561,236,636,1036,66,75,150,
	550,84,159,559,234,634,1034,93,168,568,243,643,1043,318,718,1118,
	1518,64,73,148,548,82,157,557,232,632,1032,91,166,566,241,641,
	1041,316,716,1116,1516,100,175,575,250,650,1050,325,725,1125,1525,400,
	800,1200,1600,2000
};

short Evaluator::Value[3876] = {
	0,0,0,0,0,0,0,0,3,3,3,9,9,15,72,72,
	0,0,0,0,1,1,1,4,4,4,12,12,19,73,72,0,
	0,0,1,1,1,4,4,4,12,12,19,73,72,1,1,2,
	2,2,5,5,5,13,13,20,75,74,1,2,2,2,5,5,
	5,13,13,20,75,74,3,3,3,6,6,6,14,14,21,77,
	75,3,3,6,6,6,14,14,21,77,75,3,6,6,6,14,
	14,21,77,75,9,9,9,17,17,25,81,79,9,9,17,17,
	25,81,79,9,17,17,25,81,79,26,26,34,91,89,26,34,
	91,89,41,99,98,159,157,156,0,0,0,0,2,2,2,5,
	5,5,15,15,23,74,72,0,0,0,2,2,2,5,5,5,
	15,15,23,74,72,1,1,3,3,3,6,6,6,16,16,24,
	76,74,1,3,3,3,6,6,6,16,16,24,76,74,5,5,
	5,8,8,8,18,18,26,79,76,5,5,8,8,8,18,18,
	26,79,76,5,8,8,8,18,18,26,79,76,11,11,11,21,
	21,30,83,80,11,11,21,21,30,83,80,11,21,21,30,83,
	80,32,32,41,95,92,32,41,95,92,49,104,102,161,158,156,
	0,0,0,2,2,2,5,5,5,15,15,23,74,72,1,1,
	3,3,3,6,6,6,16,16,24,76,74,1,3,3,3,6,
	6,6,16,16,24,76,74,5,5,5,8,8,8,18,18,26,
	79,76,5,5,8,8,8,18,18,26,79,76,5,8,8,8,
	18,18,26,79,76,11,11,11,21,21,30,83,80,11,11,21,
	21,30,83,80,11,21,21,30,83,80,32,32,41,95,92,32,
	41,95,92,49,104,102,161,158,156,3,3,5,5,5,8,8,
	8,18,18,26,79,77,3,5,5,5,8,8,8,18,18,26,
	79,77,7,7,7,10,10,10,20,20,28,82,79,7,7,10,
	10,10,20,20,28,82,79,7,10,10,10,20,20,28,82,79,
	13,13,13,23,23,32,86,83,13,13,23,23,32,86,83,13,
	23,23,32,86,83,34,34,43,98,95,34,43,98,95,51,107,
	105,165,162,160,3,5,5,5,8,8,8,18,18,26,79,77,
	7,7,7,10,10,10,20,20,28,82,79,7,7,10,10,10,
	20,20,28,82,79,7,10,10,10,20,20,28,82,79,13,13,
	13,23,23,32,86,83,13,13,23,23,32,86,83,13,23,23,
	32,86,83,34,34,43,98,95,34,43,98,95,51,107,105,165,
	162,160,9,9,9,12,12,12,22,22,30,85,81,9,9,12,
	12,12,22,22,30,85,81,9,12,12,12,22,22,30,85,81,
	15,15,15,25,25,34,89,85,15,15,25,25,34,89,85,15,
	25,25,34,89,85,36,36,45,101,97,36,45,101,97,53,110,
	107,169,165,162,9,9,12,12,12,22,22,30,85,81,9,12,
	12,12,22,22,30,85,81,15,15,15,25,25,34,89,85,15,
	15,25,25,34,89,85,15,25,25,34,89,85,36,36,45,101,
	97,36,45,101,97,53,110,107,169,165,162,9,12,12,12,22,
	22,30,85,81,15,15,15,25,25,34,89,85,15,15,25,25,
	34,89,85,15,25,25,34,89,85,36,36,45,101,97,36,45,
	101,97,53,110,107,169,165,162,18,18,18,28,28,38,93,89,
	18,18,28,28,38,93,89,18,28,28,38,93,89,39,39,49,
	105,101,39,49,105,101,58,115,112,174,170,167,18,18,28,28,
	38,93,89,18,28,28,38,93,89,39,39,49,105,101,39,49,
	105,101,58,115,112,174,170,167,18,28,28,38,93,89,39,39,
	49,105,101,39,49,105,101,58,115,112,174,170,167,51,51,61,
	118,114,51,61,118,114,70,128,125,188,184,181,51,61,118,114,
	70,128,125,188,184,181,78,137,135,198,195,193,261,257,254,252,
	0,0,0,0,3,3,3,6,6,6,18,18,27,75,72,0,
	0,0,3,3,3,6,6,6,18,18,27,75,72,1,1,4,
	4,4,7,7,7,19,19,28,77,74,1,4,4,4,7,7,
	7,19,19,28,77,74,7,7,7,10,10,10,22,22,31,81,
	77,7,7,10,10,10,22,22,31,81,77,7,10,10,10,22,
	22,31,81,77,13,13,13,25,25,35,85,81,13,13,25,25,
	35,85,81,13,25,25,35,85,81,38,38,48,99,95,38,48,
	99,95,57,109,106,163,159,156,0,0,0,3,3,3,6,6,
	6,18,18,27,75,72,1,1,4,4,4,7,7,7,19,19,
	28,77,74,1,4,4,4,7,7,7,19,19,28,77,74,7,
	7,7,10,10,10,22,22,31,81,77,7,7,10,10,10,22,
	22,31,81,77,7,10,10,10,22,22,31,81,77,13,13,13,
	25,25,35,85,81,13,13,25,25,35,85,81,13,25,25,35,
	85,81,38,38,48,99,95,38,48,99,95,57,109,106,163,159,
	156,3,3,6,6,6,9,9,9,21,21,30,80,77,3,6,
	6,6,9,9,9,21,21,30,80,77,9,9,9,12,12,12,
	24,24,33,84,80,9,9,12,12,12,24,24,33,84,80,9,
	12,12,12,24,24,33,84,80,15,15,15,27,27,37,88,84,
	15,15,27,27,37,88,84,15,27,27,37,88,84,40,40,50,
	102,98,40,50,102,98,59,112,109,167,163,160,3,6,6,6,
	9,9,9,21,21,30,80,77,9,9,9,12,12,12,24,24,
	33,84,80,9,9,12,12,12,24,24,33,84,80,9,12,12,
	12,24,24,33,84,80,15,15,15,27,27,37,88,84,15,15,
	27,27,37,88,84,15,27,27,37,88,84,40,40,50,102,98,
	40,50,102,98,59,112,109,167,163,160,12,12,12,15,15,15,
	27,27,36,88,83,12,12,15,15,15,27,27,36,88,83,12,
	15,15,15,27,27,36,88,83,18,18,18,30,30,40,92,87,
	18,18,30,30,40,92,87,18,30,30,40,92,87,43,43,53,
	106,101,43,53,106,101,62,116,112,172,167,163,12,12,15,15,
	15,27,27,36,88,83,12,15,15,15,27,27,36,88,83,18,
	18,18,30,30,40,92,87,18,18,30,30,40,92,87,18,30,
	30,40,92,87,43,43,53,106,101,43,53,106,101,62,116,112,
	172,167,163,12,15,15,15,27,27,36,88,83,18,18,18,30,
	30,40,92,87,18,18,30,30,40,92,87,18,30,30,40,92,
	87,43,43,53,106,101,43,53,106,101,62,116,112,172,167,163,
	21,21,21,33,33,44,96,91,21,21,33,33,44,96,91,21,
	33,33,44,96,91,46,46,57,110,105,46,57,110,105,67,121,
	117,177,172,168,21,21,33,33,44,96,91,21,33,33,44,96,
	91,46,46,57,110,105,46,57,110,105,67,121,117,177,172,168,
	21,33,33,44,96,91,46,46,57,110,105,46,57,110,105,67,
	121,117,177,172,168,60,60,71,125,120,60,71,125,120,81,136,
	132,193,188,184,60,71,125,120,81,136,132,193,188,184,90,146,
	143,204,200,197,264,259,255,252,0,0,0,3,3,3,6,6,
	6,18,18,27,75,72,1,1,4,4,4,7,7,7,19,19,
	28,77,74,1,4,4,4,7,7,7,19,19,28,77,74,7,
	7,7,10,10,10,22,22,31,81,77,7,7,10,10,10,22,
	22,31,81,77,7,10,10,10,22,22,31,81,77,13,13,13,
	25,25,35,85,81,13,13,25,25,35,85,81,13,25,25,35,
	85,81,38,38,48,99,95,38,48,99,95,57,109,106,163,159,
	156,3,3,6,6,6,9,9,9,21,21,30,80,77,3,6,
	6,6,9,9,9,21,21,30,80,77,9,9,9,12,12,12,
	24,24,33,84,80,9,9,12,12,12,24,24,33,84,80,9,
	12,12,12,24,24,33,84,80,15,15,15,27,27,37,88,84,
	15,15,27,27,37,88,84,15,27,27,37,88,84,40,40,50,
	102,98,40,50,102,98,59,112,109,167,163,160,3,6,6,6,
	9,9,9,21,21,30,80,77,9,9,9,12,12,12,24,24,
	33,84,80,9,9,12,12,12,24,24,33,84,80,9,12,12,
	12,24,24,33,84,80,15,15,15,27,27,37,88,84,15,15,
	27,27,37,88,84,15,27,27,37,88,84,40,40,50,102,98,
	40,50,102,98,59,112,109,167,163,160,12,12,12,15,15,15,
	27,27,36,88,83,12,12,15,15,15,27,27,36,88,83,12,
	15,15,15,27,27,36,88,83,18,18,18,30,30,40,92,87,
	18,18,30,30,40,92,87,18,30,30,40,92,87,43,43,53,
	106,101,43,53,106,101,62,116,112,172,167,163,12,12,15,15,
	15,27,27,36,88,83,12,15,15,15,27,27,36,88,83,18,
	18,18,30,30,40,92,87,18,18,30,30,40,92,87,18,30,
	30,40,92,87,43,43,53,106,101,43,53,106,101,62,116,112,
	172,167,163,12,15,15,15,27,27,36,88,83,18,18,18,30,
	30,40,92,87,18,18,30,30,40,92,87,18,30,30,40,92,
	87,43,43,53,106,101,43,53,106,101,62,116,112,172,167,163,
	21,21,21,33,33,44,96,91,21,21,33,33,44,96,91,21,
	33,33,44,96,91,46,46,57,110,105,46,57,110,105,67,121,
	117,177,172,168,21,21,33,33,44,96,91,21,33,33,44,96,
	91,46,46,57,110,105,46,57,110,105,67,121,117,177,172,168,
	21,33,33,44,96,91,46,46,57,110,105,46,57,110,105,67,
	121,117,177,172,168,60,60,71,125,120,60,71,125,120,81,136,
	132,193,188,184,60,71,125,120,81,136,132,193,188,184,90,146,
	143,204,200,197,264,259,255,252,6,6,9,9,9,12,12,12,
	24,24,33,84,81,6,9,9,9,12,12,12,24,24,33,84,
	81,12,12,12,15,15,15,27,27,36,88,84,12,12,15,15,
	15,27,27,36,88,84,12,15,15,15,27,27,36,88,84,18,
	18,18,30,30,40,92,88,18,18,30,30,40,92,88,18,30,
	30,40,92,88,43,43,53,106,102,43,53,106,102,62,116,113,
	172,168,165,6,9,9,9,12,12,12,24,24,33,84,81,12,
	12,12,15,15,15,27,27,36,88,84,12,12,15,15,15,27,
	27,36,88,84,12,15,15,15,27,27,36,88,84,18,18,18,
	30,30,40,92,88,18,18,30,30,40,92,88,18,30,30,40,
	92,88,43,43,53,106,102,43,53,106,102,62,116,113,172,168,
	165,15,15,15,18,18,18,30,30,39,92,87,15,15,18,18,
	18,30,30,39,92,87,15,18,18,18,30,30,39,92,87,21,
	21,21,33,33,43,96,91,21,21,33,33,43,96,91,21,33,
	33,43,96,91,46,46,56,110,105,46,56,110,105,65,120,116,
	177,172,168,15,15,18,18,18,30,30,39,92,87,15,18,18,
	18,30,30,39,92,87,21,21,21,33,33,43,96,91,21,21,
	33,33,43,96,91,21,33,33,43,96,91,46,46,56,110,105,
	46,56,110,105,65,120,116,177,172,168,15,18,18,18,30,30,
	39,92,87,21,21,21,33,33,43,96,91,21,21,33,33,43,
	96,91,21,33,33,43,96,91,46,46,56,110,105,46,56,110,
	105,65,120,116,177,172,168,24,24,24,36,36,47,100,95,24,
	24,36,36,47,100,95,24,36,36,47,100,95,49,49,60,114,
	109,49,60,114,109,70,125,121,182,177,173,24,24,36,36,47,
	100,95,24,36,36,47,100,95,49,49,60,114,109,49,60,114,
	109,70,125,121,182,177,173,24,36,36,47,100,95,49,49,60,
	114,109,49,60,114,109,70,125,121,182,177,173,63,63,74,129,
	124,63,74,129,124,84,140,136,198,193,189,63,74,129,124,84,
	140,136,198,193,189,93,150,147,209,205,202,270,265,261,258,6,
	9,9,9,12,12,12,24,24,33,84,81,12,12,12,15,15,
	15,27,27,36,88,84,12,12,15,15,15,27,27,36,88,84,
	12,15,15,15,27,27,36,88,84,18,18,18,30,30,40,92,
	88,18,18,30,30,40,92,88,18,30,30,40,92,88,43,43,
	53,106,102,43,53,106,102,62,116,113,172,168,165,15,15,15,
	18,18,18,30,30,39,92,87,15,15,18,18,18,30,30,39,
	92,87,15,18,18,18,30,30,39,92,87,21,21,21,33,33,
	43,96,91,21,21,33,33,43,96,91,21,33,33,43,96,91,
	46,46,56,110,105,46,56,110,105,65,120,116,177,172,168,15,
	15,18,18,18,30,30,39,92,87,15,18,18,18,30,30,39,
	92,87,21,21,21,33,33,43,96,91,21,21,33,33,43,96,
	91,21,33,33,43,96,91,46,46,56,110,105,46,56,110,105,
	65,120,116,177,172,168,15,18,18,18,30,30,39,92,87,21,
	21,21,33,33,43,96,91,21,21,33,33,43,96,91,21,33,
	33,43,96,91,46,46,56,110,105,46,56,110,105,65,120,116,
	177,172,168,24,24,24,36,36,47,100,95,24,24,36,36,47,
	100,95,24,36,36,47,100,95,49,49,60,114,109,49,60,114,
	109,70,125,121,182,177,173,24,24,36,36,47,100,95,24,36,
	36,47,100,95,49,49,60,114,109,49,60,114,109,70,125,121,
	182,177,173,24,36,36,47,100,95,49,49,60,114,109,49,60,
	114,109,70,125,121,182,177,173,63,63,74,129,124,63,74,129,
	124,84,140,136,198,193,189,63,74,129,124,84,140,136,198,193,
	189,93,150,147,209,205,202,270,265,261,258,18,18,18,21,21,
	21,33,33,42,96,90,18,18,21,21,21,33,33,42,96,90,
	18,21,21,21,33,33,42,96,90,24,24,24,36,36,46,100,
	94,24,24,36,36,46,100,94,24,36,36,46,100,94,49,49,
	59,114,108,49,59,114,108,68,124,119,182,176,171,18,18,21,
	21,21,33,33,42,96,90,18,21,21,21,33,33,42,96,90,
	24,24,24,36,36,46,100,94,24,24,36,36,46,100,94,24,
	36,36,46,100,94,49,49,59,114,108,49,59,114,108,68,124,
	119,182,176,171,18,21,21,21,33,33,42,96,90,24,24,24,
	36,36,46,100,94,24,24,36,36,46,100,94,24,36,36,46,
	100,94,49,49,59,114,108,49,59,114,108,68,124,119,182,176,
	171,27,27,27,39,39,50,104,98,27,27,39,39,50,104,98,
	27,39,39,50,104,98,52,52,63,118,112,52,63,118,112,73,
	129,124,187,181,176,27,27,39,39,50,104,98,27,39,39,50,
	104,98,52,52,63,118,112,52,63,118,112,73,129,124,187,181,
	176,27,39,39,50,104,98,52,52,63,118,112,52,63,118,112,
	73,129,124,187,181,176,66,66,77,133,127,66,77,133,127,87,
	144,139,203,197,192,66,77,133,127,87,144,139,203,197,192,96,
	154,150,214,209,205,276,270,265,261,18,18,21,21,21,33,33,
	42,96,90,18,21,21,21,33,33,42,96,90,24,24,24,36,
	36,46,100,94,24,24,36,36,46,100,94,24,36,36,46,100,
	94,49,49,59,114,108,49,59,114,108,68,124,119,182,176,171,
	18,21,21,21,33,33,42,96,90,24,24,24,36,36,46,100,
	94,24,24,36,36,46,100,94,24,36,36,46,100,94,49,49,
	59,114,108,49,59,114,108,68,124,119,182,176,171,27,27,27,
	39,39,50,104,98,27,27,39,39,50,104,98,27,39,39,50,
	104,98,52,52,63,118,112,52,63,118,112,73,129,124,187,181,
	176,27,27,39,39,50,104,98,27,39,39,50,104,98,52,52,
	63,118,112,52,63,118,112,73,129,124,187,181,176,27,39,39,
	50,104,98,52,52,63,118,112,52,63,118,112,73,129,124,187,
	181,176,66,66,77,133,127,66,77,133,127,87,144,139,203,197,
	192,66,77,133,127,87,144,139,203,197,192,96,154,150,214,209,
	205,276,270,265,261,18,21,21,21,33,33,42,96,90,24,24,
	24,36,36,46,100,94,24,24,36,36,46,100,94,24,36,36,
	46,100,94,49,49,59,114,108,49,59,114,108,68,124,119,182,
	176,171,27,27,27,39,39,50,104,98,27,27,39,39,50,104,
	98,27,39,39,50,104,98,52,52,63,118,112,52,63,118,112,
	73,129,124,187,181,176,27,27,39,39,50,104,98,27,39,39,
	50,104,98,52,52,63,118,112,52,63,118,112,73,129,124,187,
	181,176,27,39,39,50,104,98,52,52,63,118,112,52,63,118,
	112,73,129,124,187,181,176,66,66,77,133,127,66,77,133,127,
	87,144,139,203,197,192,66,77,133,127,87,144,139,203,197,192,
	96,154,150,214,209,205,276,270,265,261,30,30,30,42,42,54,
	108,102,30,30,42,42,54,108,102,30,42,42,54,108,102,55,
	55,67,122,116,55,67,122,116,78,134,129,192,186,181,30,30,
	42,42,54,108,102,30,42,42,54,108,102,55,55,67,122,116,
	55,67,122,116,78,134,129,192,186,181,30,42,42,54,108,102,
	55,55,67,122,116,55,67,122,116,78,134,129,192,186,181,69,
	69,81,137,131,69,81,137,131,92,149,144,208,202,197,69,81,
	137,131,92,149,144,208,202,197,102,160,156,220,215,211,282,276,
	271,267,30,30,42,42,54,108,102,30,42,42,54,108,102,55,
	55,67,122,116,55,67,122,116,78,134,129,192,186,181,30,42,
	42,54,108,102,55,55,67,122,116,55,67,122,116,78,134,129,
	192,186,181,69,69,81,137,131,69,81,137,131,92,149,144,208,
	202,197,69,81,137,131,92,149,144,208,202,197,102,160,156,220,
	215,211,282,276,271,267,30,42,42,54,108,102,55,55,67,122,
	116,55,67,122,116,78,134,129,192,186,181,69,69,81,137,131,
	69,81,137,131,92,149,144,208,202,197,69,81,137,131,92,149,
	144,208,202,197,102,160,156,220,215,211,282,276,271,267,84,84,
	96,153,147,84,96,153,147,107,165,160,225,219,214,84,96,153,
	147,107,165,160,225,219,214,117,176,172,237,232,228,300,294,289,
	285,84,96,153,147,107,165,160,225,219,214,117,176,172,237,232,
	228,300,294,289,285,126,186,183,248,244,241,312,307,303,300,378,
	372,367,363,360
};

/* ===== Board.cpp ===== */
Board::Board(UInt8 boardSize_) {
	boardSize = boardSize_;
	boardSizeSqr = boardSize * boardSize;
	center = boardSize / 2;
	boardStartPos = POS(0, 0);
	boardEndPos = POS(boardSize - 1, boardSize - 1);
	initBoard();
	initZobrish();
	historyMoves = new Pos[boardSizeSqr];
	historyAreas = new CandArea[boardSizeSqr];
}

Board::~Board() {
	delete[] historyMoves;
	delete[] historyAreas;
}

void Board::initZobrish() {
	for (int i = 0; i < MaxBoardSizeSqr; i++)
		zobrist[0][i] = random();
	for (int i = 0; i < MaxBoardSizeSqr; i++)
		zobrist[1][i] = random();
	zobristKey = random();
}

void Board::initBoard() {
	for (int i = 0; i < MaxBoardSizeSqr; i++) {
		board[i] = (CoordX(i) >= 0 && CoordX(i) < boardSize && CoordY(i) >= 0 && CoordY(i) < boardSize) ? Empty : Wrong;
	}
}

void Board::clear() {
	initBoard();
	moveCount = 0;
	nullMoveCount = 0;
	playerToMove = Black;
	playerToMoveOppo = White;
	playerWon = Empty;
	zobristKey = 0;
	area = CandArea();
}

inline void Board::setPiece(Pos pos, Piece piece) {
	assert(isInBoard(pos));
	assert(board[pos] == Empty);
	board[pos] = piece;
	zobristKey ^= zobrist[piece][pos];
}

inline void Board::delPiece(Pos pos) {
	assert(isInBoard(pos));
	assert(board[pos] <= White);
	zobristKey ^= zobrist[board[pos]][pos];
	board[pos] = Empty;
}

void Board::move(Pos pos) {
	setPiece(pos, playerToMove);
	historyMoves[moveCount] = pos;
	historyAreas[moveCount] = area;
	area.expend(pos, boardSize);
	playerToMove = Opponent(playerToMove);
	playerToMoveOppo = Opponent(playerToMoveOppo);
	moveCount++;
}

void Board::undo() {
	assert(moveCount > 0);
	moveCount--;
	delPiece(historyMoves[moveCount]);
	area = historyAreas[moveCount];
	playerToMove = Opponent(playerToMove);
	playerToMoveOppo = Opponent(playerToMoveOppo);
}

void Board::muiltMove(Pos pos) {
	setPiece(pos, playerToMove);
	historyMoves[moveCount] = pos;
	historyAreas[moveCount] = area;
	area.expend(pos, boardSize);
	moveCount++;
}

void Board::muiltUndo() {
	assert(moveCount > 0);
	moveCount--;
	delPiece(historyMoves[moveCount]);
	area = historyAreas[moveCount];
}

void Board::makeNullMove() {
	if (nullMoveCount == 0) {
		playerToMove = Opponent(playerToMove);
		playerToMoveOppo = Opponent(playerToMoveOppo);
	}
	nullMoveCount++;
}

void Board::undoNullMove() {
	assert(nullMoveCount > 0);
	nullMoveCount--;
	if (nullMoveCount == 0) {
		playerToMove = Opponent(playerToMove);
		playerToMoveOppo = Opponent(playerToMoveOppo);
	}
}

bool Board::check5InLine(Pos origin, Delta d, Piece p) {
	int count1 = 0, count2 = 0;
	Pos pos = origin;
	pos += d;
	for (int i = 0; i < 4; i++) {
		if (get(pos) == p) {
			count1++;
			pos += d;
		} else break;
	}
	origin -= d;
	for (int i = 0; i < 4 - count1; i++) {
		if (get(origin) == p) {
			count2++;
			origin -= d;
		} else break;
	}
	if (count1 + count2 >= 4)
		return true;
	else
		return false;
}

bool Board::checkWin() {
	if (moveCount < 5) return false;
	Pos lastPos = historyMoves[moveCount - 1];
	Piece lastPiece = get(lastPos);
	assert(lastPiece != Empty);
	for (int i = 0; i < 4; i++) {
		if (check5InLine(lastPos, D[i], lastPiece)) {
			playerWon = lastPiece;
			return true;
		}
	}
	return false;
}

void CandArea::expend(Pos p, UInt8 boardSize) {
	x0 = MIN(MIN(x0, MAX(CoordX(p) - 3, 0)), boardSize - 5);
	y0 = MIN(MIN(y0, MAX(CoordY(p) - 3, 0)), boardSize - 5);
	x1 = MAX(MAX(x1, MIN(CoordX(p) + 3, boardSize - 1)), 4);
	y1 = MAX(MAX(y1, MIN(CoordY(p) + 3, boardSize - 1)), 4);
}

void Board::expendCandArea(Pos pos, int expendWidth) {
	area.x0 = MIN(MIN(area.x0, MAX(CoordX(pos) - expendWidth, 0)), boardSize - 5);
	area.y0 = MIN(MIN(area.y0, MAX(CoordY(pos) - expendWidth, 0)), boardSize - 5);
	area.x1 = MAX(MAX(area.x1, MIN(CoordX(pos) + expendWidth, boardSize - 1)), 4);
	area.y1 = MAX(MAX(area.y1, MIN(CoordY(pos) + expendWidth, boardSize - 1)), 4);
}

/* ===== Evaluator.cpp ===== */
#include <sstream>
#include <iomanip>
Pattern Evaluator::PATTERN[256][256];
PatternCode Evaluator::PCODE[16][16][16][16];

Pattern4 Evaluator::PATTERN4[3876];

Evaluator::Evaluator(Board * board) : board(board) {
	init();
	newGame();
}

template <Evaluator::MoveType MT>
void Evaluator::makeMove(Pos pos) {
	Piece self = SELF;
	if (MT == MoveType::MuiltVC) {
		board->muiltMove(pos);
	} else {
		board->move(pos);
		ply++;
	}

	Pos p;
	Cell * c;
	PatternCode pCodeBlack, pCodeWhite;
	for (int i = 0; i < 4; i++) {
		p = pos - D[i] * 4;
		for (UInt8 k = 1; k < (1 << 4); k <<= 1) {
			if (board->isEmpty(p)) {
				c = &cell(p);
				c->key[i][self] |= k;

				c->updatePattern(i);
				
				pCodeBlack = c->getPatternCode(Black);
				pCodeWhite = c->getPatternCode(White);

				c->updateScore(pCodeBlack, pCodeWhite);

				if (MT == MoveType::Normal) {
					eval[Black] -= c->eval[Black]; eval[White] -= c->eval[White];
					c->updateEval(pCodeBlack, pCodeWhite);
					eval[Black] += c->eval[Black]; eval[White] += c->eval[White];
				}

				p4Count[Black][c->pattern4[Black]]--; p4Count[White][c->pattern4[White]]--;
				c->updatePattern4(pCodeBlack, pCodeWhite);
				if (contestForbiddenBlack(*board, p)) c->pattern4[Black] = FORBID;
				p4Count[Black][c->pattern4[Black]]++; p4Count[White][c->pattern4[White]]++;
			}
			p += D[i];
		}
		for (UInt8 k = 1 << 4; k != 0; k <<= 1) {
			p += D[i];
			if (board->isEmpty(p)) {
				c = &cell(p);
				c->key[i][self] |= k;

				c->updatePattern(i);

				pCodeBlack = c->getPatternCode(Black);
				pCodeWhite = c->getPatternCode(White);

				c->updateScore(pCodeBlack, pCodeWhite);

				if (MT == MoveType::Normal) {
					eval[Black] -= c->eval[Black]; eval[White] -= c->eval[White];
					c->updateEval(pCodeBlack, pCodeWhite);
					eval[Black] += c->eval[Black]; eval[White] += c->eval[White];
				}

				p4Count[Black][c->pattern4[Black]]--; p4Count[White][c->pattern4[White]]--;
				c->updatePattern4(pCodeBlack, pCodeWhite);
				if (contestForbiddenBlack(*board, p)) c->pattern4[Black] = FORBID;
				p4Count[Black][c->pattern4[Black]]++; p4Count[White][c->pattern4[White]]++;
			}
		}
	}
	// The original freestyle key stores four cells per side. Refresh the
	// fifth-away endpoints explicitly so a newly-created six is not cached as
	// an exact five.
	for (int i = 0; i < 4; ++i) for (int sign : {-1, 1}) {
		int raw = int(pos) + sign * 5 * D[i];
		if (raw < 0 || raw >= Board::MaxBoardSizeSqr) continue;
		p = Pos(raw);
		if (!board->isInBoard(p) || !board->isEmpty(p)) continue;
		c = &cell(p);
		p4Count[Black][c->pattern4[Black]]--; p4Count[White][c->pattern4[White]]--;
		pCodeBlack = c->getPatternCode(Black); pCodeWhite = c->getPatternCode(White);
		c->updatePattern4(pCodeBlack, pCodeWhite);
		if (contestForbiddenBlack(*board, p)) c->pattern4[Black] = FORBID;
		p4Count[Black][c->pattern4[Black]]++; p4Count[White][c->pattern4[White]]++;
	}

	c = &cell(pos);
	if (MT == MoveType::Normal) {
		eval[Black] -= c->eval[Black]; 
		eval[White] -= c->eval[White];
	}
	p4Count[Black][c->pattern4[Black]]--;
	p4Count[White][c->pattern4[White]]--;

	assert(checkP4Count());

#ifdef GENERATION_MIN
	for (int k = 0; k < 16; k++)
		cell(pos + RANGE_MIN[k]).cand++;
#endif
#ifdef GENERATION_MIDDLE
	for (int k = 0; k < 24; k++)
		cell(pos + RANGE_MIDDLE[k]).cand++;
#endif
#ifdef GENERATION_LARGE
    for (int k = 0; k < 32; k++)
	    cell(pos + RANGE_LARGE[k]).cand++;
#endif
}

template void Evaluator::makeMove<Evaluator::Normal>(Pos pos);
template void Evaluator::makeMove<Evaluator::VC>(Pos pos);
template void Evaluator::makeMove<Evaluator::MuiltVC>(Pos pos);

template <Evaluator::MoveType MT>
void Evaluator::undoMove() {
	assert(board->getMoveCount() > 0);
	PatternCode pCodeBlack, pCodeWhite;
	Pos p, pos = board->getLastMove();
	if (MT == MoveType::MuiltVC) {
		board->muiltUndo();
	} else {
		board->undo();
		ply--;
	}
	Piece self = SELF;

	Cell * c = &cell(pos);
	if (MT == MoveType::Normal) {
		eval[Black] += c->eval[Black]; 
		eval[White] += c->eval[White];
	}
	p4Count[Black][c->pattern4[Black]]++;
	p4Count[White][c->pattern4[White]]++;
	
	for (int i = 0; i < 4; i++) {
		p = pos - 4 * D[i];
		for (UInt8 k = 1; k < (1 << 4); k <<= 1) {
			if (board->isEmpty(p)) {
				c = &cell(p);
				c->key[i][self] ^= k;

				c->updatePattern(i);

				pCodeBlack = c->getPatternCode(Black);
				pCodeWhite = c->getPatternCode(White);

				c->updateScore(pCodeBlack, pCodeWhite);

				if (MT == MoveType::Normal) {
					eval[Black] -= c->eval[Black]; eval[White] -= c->eval[White];
					c->updateEval(pCodeBlack, pCodeWhite);
					eval[Black] += c->eval[Black]; eval[White] += c->eval[White];
				}

				p4Count[Black][c->pattern4[Black]]--; p4Count[White][c->pattern4[White]]--;
				c->updatePattern4(pCodeBlack, pCodeWhite);
				if (contestForbiddenBlack(*board, p)) c->pattern4[Black] = FORBID;
				p4Count[Black][c->pattern4[Black]]++; p4Count[White][c->pattern4[White]]++;
			}
			p += D[i];
		}
		for (UInt8 k = 1 << 4; k != 0; k <<= 1) {
			p += D[i];
			if (board->isEmpty(p)) {
				c = &cell(p);
				c->key[i][self] ^= k;

				c->updatePattern(i);

				pCodeBlack = c->getPatternCode(Black);
				pCodeWhite = c->getPatternCode(White);

				c->updateScore(pCodeBlack, pCodeWhite);

				if (MT == MoveType::Normal) {
					eval[Black] -= c->eval[Black]; eval[White] -= c->eval[White];
					c->updateEval(pCodeBlack, pCodeWhite);
					eval[Black] += c->eval[Black]; eval[White] += c->eval[White];
				}

				p4Count[Black][c->pattern4[Black]]--; p4Count[White][c->pattern4[White]]--;
				c->updatePattern4(pCodeBlack, pCodeWhite);
				if (contestForbiddenBlack(*board, p)) c->pattern4[Black] = FORBID;
				p4Count[Black][c->pattern4[Black]]++; p4Count[White][c->pattern4[White]]++;
			}
		}
	}
	for (int i = 0; i < 4; ++i) for (int sign : {-1, 1}) {
		int raw = int(pos) + sign * 5 * D[i];
		if (raw < 0 || raw >= Board::MaxBoardSizeSqr) continue;
		p = Pos(raw);
		if (!board->isInBoard(p) || !board->isEmpty(p)) continue;
		c = &cell(p);
		p4Count[Black][c->pattern4[Black]]--; p4Count[White][c->pattern4[White]]--;
		pCodeBlack = c->getPatternCode(Black); pCodeWhite = c->getPatternCode(White);
		c->updatePattern4(pCodeBlack, pCodeWhite);
		if (contestForbiddenBlack(*board, p)) c->pattern4[Black] = FORBID;
		p4Count[Black][c->pattern4[Black]]++; p4Count[White][c->pattern4[White]]++;
	}
	assert(checkP4Count());

#ifdef GENERATION_MIN
	for (int k = 0; k < 16; k++)
		cell(pos + RANGE_MIN[k]).cand--;
#endif
#ifdef GENERATION_MIDDLE
	for (int k = 0; k < 24; k++)
		cell(pos + RANGE_MIDDLE[k]).cand--;
#endif
#ifdef GENERATION_LARGE
	for (int k = 0; k < 32; k++)
		cell(pos + RANGE_LARGE[k]).cand--;
#endif
}

template void Evaluator::undoMove<Evaluator::Normal>();
template void Evaluator::undoMove<Evaluator::VC>();
template void Evaluator::undoMove<Evaluator::MuiltVC>();

bool Evaluator::checkP4Count() {
	int p4[2][12] = { 0 };
	FOR_EVERY_EMPTY_POS(p) {
		p4[Black][cell(p).pattern4[Black]]++;
		p4[White][cell(p).pattern4[White]]++;
	}
	for (int k = 0; k < 2; k++) {
		for (int i = 1; i < 12; i++) {
			if (p4[k][i] != p4Count[k][i])
				return false;
		}
	}
	return true;
}

void Evaluator::newGame() {
	board->clear();
	ply = 0;
	eval[0] = eval[1] = 0;
	evalLower[0] = evalLower[1] = 0;
	memset(cells, 0, sizeof(cells));
	memset(p4Count, 0, sizeof(p4Count));

	FOR_EVERY_POSITION_POS(p) {
		Cell & c = cell(p);
		for (int i = 0; i < 4; i++) {
			UInt key = 0;
			Pos pi = p - 4 * D[i];
			for (UInt8 k = 1 << 7; k >= (1 << 4); k >>= 1) {
				if (board->get(pi) == Wrong) key |= k;
				pi += D[i];
			}
			for (UInt8 k = 1 << 3; k != 0; k >>= 1) {
				pi += D[i];
				if (board->get(pi) == Wrong) key |= k;
			}
			c.key[i][White] = c.key[i][Black] = key;
			c.updatePattern(i);
			assert(c.pattern[Black][i] <= F1);
			assert(c.pattern[White][i] <= F1);
		}
		PatternCode pCodeBlack = c.getPatternCode(Black);
		PatternCode pCodeWhite = c.getPatternCode(White);
		c.updateEval(pCodeBlack, pCodeWhite);
		eval[Black] += c.eval[Black]; 
		eval[White] += c.eval[White];
		c.updatePattern4(pCodeBlack, pCodeWhite);
		c.updateScore(pCodeBlack, pCodeWhite);
		p4Count[Black][c.pattern4[Black]]++; p4Count[White][c.pattern4[White]]++;
	}
	assert(checkP4Count());
}

void Evaluator::init() {
	const int N = 16;
	int v[N * N * N * N] = { -1 };

	for (int x = 0, i = 0; x < N; x++)
		for (int y = 0; y < N; y++)
			for (int z = 0; z < N; z++)
				for (int w = 0; w < N; w++) {
					int a = x, b = y, c = z, d = w;
					if (b > a) swap(a, b);
					if (c > a) swap(a, c);
					if (d > a) swap(a, d);
					if (c > b) swap(c, b);
					if (d > b) swap(d, b);
					if (d > c) swap(d, c);

					v[i++] = d * (N * N * N) + c * (N * N) + b * N + a;
				}

	for (int i = 0, sum = N * N * N * N; i < N * N * N * N; i++)
		if (v[i] > -1)
			for (int j = i + 1; j < N * N * N * N; j++)
				if (v[i] == v[j]) v[j] = -1, sum--;

	for (int i = 0, count = 0; i < N * N * N * N; i++)
		if (v[i] > -1) v[i] = count++;

	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
			for (int m = 0; m < N; m++)
				for (int n = 0; n < N; n++) {
					int a = i, b = j, c = m, d = n;
					if (b > a) swap(a, b);
					if (c > a) swap(a, c);
					if (d > a) swap(a, d);
					if (c > b) swap(c, b);
					if (d > b) swap(d, b);
					if (d > c) swap(d, c);

					int pcode = d * (N * N * N) + c * (N * N) + b * N + a;
					pcode = v[pcode];
					PCODE[i][j][m][n] = pcode;
					PATTERN4[pcode] = getPattern4(Pattern(a), Pattern(b), Pattern(c), Pattern(d));
				}

	for (int key1 = 0; key1 < 256; key1++) {
		for (int key2 = 0; key2 < 256; key2++) {
			PATTERN[key1][key2] = getPattern(key1, key2);
		}
	}
}

Pattern Evaluator::getPattern(UInt8 key1, UInt8 key2) {
	array<Piece, 9> line;
	line[4] = Black;
	for (int i = 0; i < 8; i++) {
		UInt p = 1 << i;
		UInt k1 = key1 & p;
		UInt k2 = key2 & p;
		Piece piece = k1 ? (k2 ? Wrong : Black) : (k2 ? White : Empty);
		line[i < 4 ? i : i + 1] = piece;
	}

	// ˫���жϣ�ȡ��������
	Pattern p1 = shortLinePattern(line);
	// �����ͷ����ٴ��ж�
	reverse(line.begin(), line.end());
	Pattern p2 = shortLinePattern(line);

	Pattern p;

	// ͬ��˫�ģ�˫��
	if (p1 == B4 && p2 == B4)
		p = checkFlex4(line, p1, p2);
	else if ((p1 == B3J0 || p1 == B3J1 || p1 == B3J2) && (p2 == B3J0 || p2 == B3J1 || p2 == B3J2))
		p = checkFlex3(line, p1, p2);
	else
		p = MAX(p1, p2);

	return p;
}
// �жϵ�������
Pattern Evaluator::shortLinePattern(array<Piece, 9> & line) {
	int empty = 0, block = 0;
	int len = 1, len2 = 1, count = 1;

	Piece self = line[4];
	for (int i = 5; i <= 8; i++) {
		if (line[i] == self) {
			assert(empty + count <= 4);
			count++;
			len++;
			len2 = empty + count;
		} else if (line[i] == Empty) {
			len++;
			empty++;
		} else {
			if (line[i - 1] == self) block++;
			break;
		}
	}
	// �����м�ո�
	empty = len2 - count;
	for (int i = 3; i >= 0; i--) {
		if (line[i] == self) {
			if (empty + count > 4) break;
			count++;
			len++;
			len2 = empty + count;
		} else if (line[i] == Empty) {
			if (empty + count > 4) break;
			len++;
			empty++;
		} else {
			if (line[i + 1] == self) block++;
			break;
		}
	}
	return getType(len, len2, count, block > 0, len2 - count);
}
// ͬ��˫������
Pattern Evaluator::checkFlex3(array<Piece, 9> & line, Pattern p1, Pattern p2) {
	Piece self = line[4];
	Pattern type;
	for (int i = 0; i < 9; i++) {
		if (line[i] == Empty) {
			line[i] = self;
			type = checkFlex4(line, p1, p2);
			line[i] = Empty;
			if (type >= F4)
				return F3J1;
		}
	}
	return MAX(p1, p2);
}
// ͬ��˫������
Pattern Evaluator::checkFlex4(array<Piece, 9> & line, Pattern p1, Pattern p2) {
	if (checkFive(line, 4)) return F5;
	int five = 0;
	for (int i = 0; i < 9; i++) {
		if (line[i] == Empty)
			five += checkFive(line, i);
	}
	return five >= 2 ? F4 : MAX(p1, p2);
}
// ���ĳ�����Ƿ�������
bool Evaluator::checkFive(array<Piece, 9> & line, int i) {
	int count = 0;
	Piece self = line[4];
	for (int j = i - 1; j >= 0 && line[j] == self; j--)
		count++;
	for (int j = i + 1; j <= 8 && line[j] == self; j++)
		count++;
	return count == 4;
}
// ��ö�Ӧ�ĵ�������
Pattern Evaluator::getType(int length, int fullLength, int count, bool block, int jump) {
	if (length < 5) return DEAD;
	if (count >= 5) return count == 5 ? F5 : DEAD;
	if (length > 5 && fullLength < 5 && (!block)) {
		switch (count) {
		case 1: return F1;
		case 2: 
			switch (jump) {
			case 0: return F2J0;
			case 1: return F2J1;
			case 2: return F2J2;
			default: assert(false);
			}
		case 3: 
			switch (jump) {
			case 0: return F3J0;
			case 1: return F3J1;
			default: assert(false);
			}
		case 4: return F4;
		}
	} else {
		switch (count) {
		case 1: return B1;
		case 2:
			switch (jump) {
			case 0:
			case 1: return B2J0;
			case 2:
			case 3: return B2J2;
			default: assert(false);
			}
		case 3:
			switch (jump) {
			case 0: return B3J0;
			case 1: return B3J1;
			case 2: return B3J2;
			default: assert(false);
			}
		case 4: return B4;
		}
	}
	return DEAD;
}

Pattern4 Evaluator::getPattern4(Pattern p1, Pattern p2, Pattern p3, Pattern p4) {
	int n[16] = { 0 };
	n[p1]++; n[p2]++; n[p3]++; n[p4]++;

	if (n[F5] >= 1) return A_FIVE;                                               // OOOO_
	if (n[B4] >= 2) return B_FLEX4;                                              // XOOO_ * _OOOX
	if (n[F4] >= 1) return B_FLEX4;                                              // OOO_
	if (n[B4] >= 1) {
		if (n[F3J0] >= 1 || n[F3J1] >= 1) return C_BLOCK4_FLEX3;                 // XOOO_ * _OO
		if (n[B3J0] >= 1 || n[B3J1] >= 1 || n[B3J2] >= 1) return D_BLOCK4_PLUS;  // XOOO_ * _OOX
		if (n[F2J0] >= 1 || n[F2J1] >= 1 || n[F2J2] >= 1) return D_BLOCK4_PLUS;  // XOOO_ * _O
		else return E_BLOCK4;                                                    // XOOO_
	}
	if (n[F3J0] >= 1 || n[F3J1] >= 1) {
		if (n[F3J0] + n[F3J1] >= 2) return F_FLEX3_2X;                           // OO_ * _OO
		if (n[B3J0] >= 1 || n[B3J1] >= 1 || n[B3J2] >= 1) return G_FLEX3_PLUS;   // OO_ * _OOX
		if (n[F2J0] >= 1 || n[F2J1] >= 1 || n[F2J2] >= 1) return G_FLEX3_PLUS;   // OO_ * _O
		else return H_FLEX3;                                                     // OO_
	}
	if (n[B3J0] >= 1 || n[B3J1] >= 1 || n[B3J2] >= 1) {
		if (n[B3J0] + n[B3J1] + n[B3J2] >= 2) return I_BLOCK3_PLUS;              // XOO_ * XOO_
		if (n[F2J0] >= 1 || n[F2J1] >= 1 || n[F2J2] >= 1) return I_BLOCK3_PLUS;  // XOO_ * O_
	}
	if (n[F2J0] + n[F2J1] + n[F2J2] >= 2) {
		return J_FLEX2_2X;                                                       // O_ * O_
	}

	return NONE;
}

Pos Evaluator::findPosByPattern4(Piece piece, Pattern4 p4) {
	FOR_EVERY_CAND_POS(p) {
		if (cell(p).pattern4[piece] == p4) return p;
	}
	assert(false);
	return NullPos;
}

Pos Evaluator::getCostPosAgainstB4(Pos posB4, Piece piece) {
	const int FindDistMax = 4;
	assert(p4Count[piece][A_FIVE] > 0);

	int dir;
	Cell & c = cell(posB4);
	for (dir = 0; dir < 4; dir++) {
		if (c.pattern[piece][dir] >= B4)
			break;
		assert(dir < 3);
	}

	Piece p;
	Pos pos = posB4;
	int i, j;
	for (i = 0; i < FindDistMax; i++) {
		pos -= D[dir];
		p = board->get(pos);
		if (p == piece) continue;
		else if (p == Empty) {
			if (cell(pos).pattern[piece][dir] == F5)
				return pos;
		}
		break;
	}
	pos = posB4;
	for (j = FindDistMax - i; j >= 1; j--) {
		pos += D[dir];
		p = board->get(pos);
		if (p == piece) continue;
		else if (p == Empty) {
			if (cell(pos).pattern[piece][dir] == F5) {
				return pos;
			}
		}
		break;
	}
	MESSAGEL("ERROR!");
	trace(cout, "MESSAGE ");
	assert(false);
	return findPosByPattern4(piece, A_FIVE);
}

// ���ĳ����ĳλ�û���ʱ���Է������µ�ĳһ���
// posB: �Է���һ��Ҫ�µĻ��ĵ�
// dirIndex: �Է���һ���µĻ�������
void Evaluator::getCostPosAgainstF3(Pos posB, Piece piece, vector<Move> & list) {
	const int FindDistMax_F4 = 5;
	const int FindDistMax_B4 = 4;

	bool flex3 = false;
	int dir;
	Cell & c = cell(posB);
	list.emplace_back(posB, c.getScore_VC(piece));
	for (dir = 0; dir < 4; dir++) {
		if (c.pattern[piece][dir] == F4) {
			flex3 = true;
			break;
		}
	}

	if (flex3) {
		Piece p;
		Pos pos = posB, posL = NullPos, posR = NullPos;
		int i, j;
		for (i = 1; i <= FindDistMax_F4; i++) {
			pos -= D[dir];
			p = board->get(pos);
			if (p == piece) continue;
			else if (p == Empty) {
				Pattern pattern = cell(pos).pattern[piece][dir];
				if (pattern >= F4) {
					list.emplace_back(pos, cell(pos).getScore_VC(piece));
					continue;
				} else if (pattern >= B4)
					posL = pos;
			}
			break;
		}
		pos = posB;
		for (j = FindDistMax_F4 - i; j >= 1; j--) {
			pos += D[dir];
			p = board->get(pos);
			if (p == piece) continue;
			else if (p == Empty) {
				Pattern pattern = cell(pos).pattern[piece][dir];
				if (pattern >= F4) {
					list.emplace_back(pos, cell(pos).getScore_VC(piece));
					continue;
				} else if (pattern >= B4)
					posR = pos;
			}
			break;
		}
		if (posR && i <= FindDistMax_F4) list.emplace_back(posR, cell(posR).getScore_VC(piece));
		if (posL && j >= 1) list.emplace_back(posL, cell(posL).getScore_VC(piece));
	} else {
		for (dir = 0; dir < 4; dir++) {
			if (c.pattern[piece][dir] < B4) continue;

			Pos pos = posB;
			Piece p;
			for (int i = 1; i <= FindDistMax_B4; i++) {
				pos -= D[dir];
				p = board->get(pos);
				if (p == piece) continue;
				else if (p == Empty) {
					if (cell(pos).pattern[piece][dir] >= B4) {
						list.emplace_back(pos, cell(pos).getScore_VC(piece));
						goto NoCheck_another_direction;
					}
				}
				break;
			}
			pos = posB;
			for (int i = 1; i <= FindDistMax_B4; i++) {
				pos += D[dir];
				p = board->get(pos);
				if (p == piece) continue;
				else if (p == Empty) {
					if (cell(pos).pattern[piece][dir] >= B4)
						list.emplace_back(pos, cell(pos).getScore_VC(piece));
				}
				break;
			}
		NoCheck_another_direction:
			continue;
		}
	}
	assert(list.size() >= 2);
}
// ���ĳ����ĳλ�û���ʱ���Է������µ����е�
void Evaluator::getAllCostPosAgainstF3(Pos posB, Piece piece, set<Pos> & set) {
	const int FindDistMax_F4 = 5;
	const int FindDistMax_B4 = 4;

	bool flex3 = false;
	int dir;
	Cell & c = cell(posB);
	set.insert(posB);
	for (dir = 0; dir < 4; dir++) {
		if (c.pattern[piece][dir] == F4) {
			flex3 = true;

			Piece p;
			Pos pos = posB, posL = NullPos, posR = NullPos;
			int i, j;
			for (i = 1; i <= FindDistMax_F4; i++) {
				pos -= D[dir];
				p = board->get(pos);
				if (p == piece) continue;
				else if (p == Empty) {
					Pattern pattern = cell(pos).pattern[piece][dir];
					if (pattern >= F4) {
						set.insert(pos);
						continue;
					} else if (pattern >= B4)
						posL = pos;
				}
				break;
			}
			pos = posB;
			for (j = FindDistMax_F4 - i; j >= 1; j--) {
				pos += D[dir];
				p = board->get(pos);
				if (p == piece) continue;
				else if (p == Empty) {
					Pattern pattern = cell(pos).pattern[piece][dir];
					if (pattern >= F4) {
						set.insert(pos);
						continue;
					} else if (pattern >= B4)
						posR = pos;
				}
				break;
			}
			if (posR && i <= FindDistMax_F4) set.insert(posR);
			if (posL && j >= 1) set.insert(posL);
		}
	}

	if (!flex3) {
		for (dir = 0; dir < 4; dir++) {
			if (c.pattern[piece][dir] < B4) continue;

			Pos pos = posB;
			Piece p;
			for (int i = 1; i <= FindDistMax_B4; i++) {
				pos -= D[dir];
				p = board->get(pos);
				if (p == piece) continue;
				else if (p == Empty) {
					if (cell(pos).pattern[piece][dir] >= B4) {
						set.insert(pos);
						goto NoCheck_another_direction;
					}
				}
				break;
			}
			pos = posB;
			for (int i = 1; i <= FindDistMax_B4; i++) {
				pos += D[dir];
				p = board->get(pos);
				if (p == piece) continue;
				else if (p == Empty) {
					if (cell(pos).pattern[piece][dir] >= B4)
						set.insert(pos);
				}
				break;
			}
		NoCheck_another_direction:
			continue;
		}
	}
	assert(set.size() >= 2);
}

// ��չѡ��(���������ڱ߽�λ��ʱ)
void Evaluator::expendCand(Pos pos, int fillDist, int lineDist) {
	board->expendCandArea(pos, MAX(lineDist, fillDist));
	int x = CoordX(pos), y = CoordY(pos);
	Pos p;
	for (int xi = -fillDist; xi <= fillDist; xi++) {
		for (int yi = -fillDist; yi <= fillDist; yi++) {
			if (xi == 0 && yi == 0) continue;
			p = POS(x + xi, y + yi);
			if (board->isEmpty(p) && cell(p).cand == 0) {
				cell(p).cand++;
			}
		}
	}
	p = POS(x, y);
	for (int i = MAX(3, fillDist + 1); i <= lineDist; i++) {
		for (int k = 0; k < 8; k++)
			cell(p + RANGE_NEIGHBOR[k] * i).cand++;
	}
}

void Evaluator::clearLose() {
	FOR_EVERY_POSITION_POS(p) {
		cell(p).isLose = false;
	}
}

Pos Evaluator::getHighestScoreCandPos() {
	int highestScore = INT32_MIN;
	Pos hp = NullPos;
	FOR_EVERY_CAND_POS(p) {
		int score = cell(p).getScore();
		if (score > highestScore) {
			hp = p;
			highestScore = score;
		}
	}
	return hp;
}
// ���ֿ����
// A Function From Pela
Pos Evaluator::databaseMove() {
	const int MinDistFromBoard = 5;
	char *s, *sn;
	int i, x, y, x1, y1, flip, len1, len2, left, top, right, bottom;

	// board rectangle
	if (board->isNearBoard(POS(board->candArea().x0, board->candArea().y0), 2) ||
		board->isNearBoard(POS(board->candArea().x1, board->candArea().y1), 2))
		return NullPos;
	left = board->candArea().x0 + 2;
	top = board->candArea().y0 + 2;
	right = board->candArea().x1 - 2;
	bottom = board->candArea().y1 - 2;
	// find current board in the database
	for (s = MoveDatabase; ; s = sn) {
		len1 = *s++;
		len2 = *s++;
		sn = s + 2 * (len1 + len2);
		if (len1 != board->getMoveCount()) {
			// data must be sorted by moveCount descending
			if (len1 < board->getMoveCount()) 
				return NullPos; 
			continue;
		}
		// try all symmetries
		for (flip = 0; flip < 8; flip++) {
			for (i = 0;; i++) {
				x1 = s[2 * i];
				y1 = s[2 * i + 1];
				if (i == len1) {
					std::uniform_int_distribution<> dis(0, len2 - 1);
					s += 2 * (len1 + dis(random));
					x1 = *s++;
					y1 = *s;
				}
				switch (flip) {
				case 0: x = left + x1; y = top + y1; break;
				case 1: x = right - x1; y = top + y1; break;
				case 2: x = left + x1; y = bottom - y1; break;
				case 3: x = right - x1; y = bottom - y1; break;
				case 4: x = left + y1; y = top + x1; break;
				case 5: x = right - y1; y = top + x1; break;
				case 6: x = left + y1; y = bottom - x1; break;
				default: x = right - y1; y = bottom - x1; break;
				}
				if (board->isNearBoard(POS(x, y), MinDistFromBoard)) break;
				if (i == len1) return POS(x, y);
				// compare current board and database
				if (board->get(POS(x, y)) != (i & 1)) break;
			}
		}
	}
	return NullPos;
}

void Evaluator::trace(ostream & ss, const string & appendBefore) {
#define SET_COLOR(CCODE) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), CCODE)
	int bg = 0;
	ss << appendBefore;
	if (board->getMoveCount() > 0) ss << "LastPos:" << PosStr(board->getLastMove()) << endl << appendBefore;
	FOR_EVERY_POSITION(x, y) {
		if (x != 0 || y != 0) ss << " ";
		if (y == 0 && x != 0) ss << endl << appendBefore;
		if (board->isEmpty(POS(x, y))) {
			if (cell(x, y).isLose) {
				ss << "L";
			} else {
				if (cell(x, y).isCand()) ss << '*';
				else ss << '.';
			}
		} else {
			if (board->getLastMove() == POS(x, y))
				bg = BACKGROUND_INTENSITY;
			switch (board->get(POS(x, y))) {
			case Black: SET_COLOR(bg | FOREGROUND_RED); ss << 'X'; break;
			case White: SET_COLOR(bg | FOREGROUND_GREEN); ss << 'O'; break;
			case Empty: SET_COLOR(bg | 7); ss << '.'; break;
			default: break;
			}
			bg = 0;
			SET_COLOR(bg | 7);
		}
		if (y == board->size() - 1) ss << " " << x + 1;
	}
	ss << endl << appendBefore;
	for (int y = 0; y < board->size(); y++) {
		ss << char(y + 65) << " ";
	}
	ss << endl << appendBefore << "---Score-------------" << endl << appendBefore;
	FOR_EVERY_POSITION(x, y) {
		if (y == 0 && x != 0) ss << endl << appendBefore;
		if (board->isEmpty(POS(x, y))) {
			if (cell(x, y).isCand()) ss << std::setw(5) << cell(x, y).getScore();
			else ss << "    .";
		} else {
			ss << "    ";
			switch (board->get(POS(x, y))) {
			case Black: SET_COLOR(FOREGROUND_RED); ss << 'X'; break;
			case White: SET_COLOR(FOREGROUND_GREEN); ss << 'O'; break;
			case Empty: SET_COLOR(7); ss << '.'; break;
			default: break;
			}
			SET_COLOR(7);
		}
	}
	ss << endl << appendBefore << "---Pattern4--Black------" << endl << appendBefore;
	FOR_EVERY_POSITION(x, y) {
		if (x != 0 || y != 0) ss << " ";
		if (y == 0 && x != 0) ss << endl << appendBefore;
		switch (board->get(POS(x, y))) {
		case Black: SET_COLOR(FOREGROUND_RED); break;
		case White: SET_COLOR(FOREGROUND_GREEN); break;
		case Empty: SET_COLOR(7); break;
		default: break;
		}
		if (board->isEmpty(POS(x, y))) {
			switch (cell(x, y).pattern4[Black]) {
			case A_FIVE: ss << 'A'; break;
			case B_FLEX4: ss << 'B'; break;
			case C_BLOCK4_FLEX3: ss << 'C'; break;
			case D_BLOCK4_PLUS: ss << 'D'; break;
			case E_BLOCK4: ss << 'E'; break;
			case F_FLEX3_2X: ss << 'F'; break;
			case G_FLEX3_PLUS: ss << 'G'; break;
			case H_FLEX3: ss << 'H'; break;
			case I_BLOCK3_PLUS: ss << 'I'; break;
			default: ss << '.'; break;
			}
		} else
			ss << '.';
		SET_COLOR(7);
	}
	ss << endl << appendBefore << "---Pattern4--White------" << endl << appendBefore;
	FOR_EVERY_POSITION(x, y) {
		if (x != 0 || y != 0) ss << " ";
		if (y == 0 && x != 0) ss << endl << appendBefore;
		switch (board->get(POS(x, y))) {
		case Black: SET_COLOR(FOREGROUND_RED); break;
		case White: SET_COLOR(FOREGROUND_GREEN); break;
		case Empty: SET_COLOR(7); break;
		default: break;
		}
		if (board->isEmpty(POS(x, y))) {
			switch (cell(x, y).pattern4[White]) {
			case A_FIVE: ss << 'A'; break;
			case B_FLEX4: ss << 'B'; break;
			case C_BLOCK4_FLEX3: ss << 'C'; break;
			case D_BLOCK4_PLUS: ss << 'D'; break;
			case E_BLOCK4: ss << 'E'; break;
			case F_FLEX3_2X: ss << 'F'; break;
			case G_FLEX3_PLUS: ss << 'G'; break;
			case H_FLEX3: ss << 'H'; break;
			case I_BLOCK3_PLUS: ss << 'I'; break;
			default: ss << '.'; break;
			}
		} else
			ss << '.';
		SET_COLOR(7);
	}
	ss << endl << appendBefore;
	ss << "===============================" << endl;
}



/* ===== HashTable.cpp ===== */
HashTable::HashTable(int size) {
	hashSize = 1U << size;
	hashSizeMask = hashSize - 1;
	hashTable = new Cluster[hashSize];
}

HashTable::~HashTable() {
	delete[] hashTable;
}

void HashTable::clearHash() {
	for (UInt i = 0; i < hashSize; i++) hashTable[i].clear();
	generation = 0;
}

bool HashTable::probe(U64 key, TTEntry* & tte) {
	TTEntry * entry = hashTable[key & hashSizeMask].first_entry();
	UInt key32 = key >> 32;
	tte = nullptr;

	for (int i = 0; i < CLUSTER_SIZE; i++, entry++) {
		if (entry->_key32 == key32) {
			if ((entry->_genFlag & 0xFC) != generation)
				entry->saveGeneration(generation); // ����Generation

			tte = entry;
			return true;
		} else if (entry->flag() == HashFlag::Hash_Unknown) {
			tte = entry;
			// From StockFish:
			// Due to our packed storage format for generation and its cyclic
			// nature we add 259 (256 is the modulus plus 3 to keep the lowest
			// two bound bits from affecting the result) to calculate the entry
			// age correctly even after generation8 overflows into the next cycle.
		} else if (!tte || tte->flag() != HashFlag::Hash_Unknown
			&&   tte->_depth - ((259 + generation -   tte->_genFlag) & 0xFC) * 2
	         > entry->_depth - ((259 + generation - entry->_genFlag) & 0xFC) * 2) {
			tte = entry;
		}
	}
	assert(tte);
	return false;
}

/* ===== Search.cpp ===== */
#include <fstream>

AI::AI(Board * board) : Evaluator(board) {
	hashTable = new HashTable();
}

AI::~AI() {
	delete hashTable;
}

void AI::newGame() {
	Evaluator::newGame();
	clearHash();
}

void AI::clearHash() {
	hashTable->clearHash();
}

void AI::setMaxDepth(int depth) {
	maxSearchDepth = MAX(MIN(depth, 255), 2);
}

Pos AI::turnMove() {
	startTime = getTime();
	terminateAI = false;

	if (board->getMoveCount() == 0)
		return POS(board->centerPos(), board->centerPos());
	else if (board->getMoveCount() == 1) {
		Pos p = board->getLastMove();
		if (!board->isNearBoard(p, 5)) {
			Pos b;
			std::uniform_int_distribution<> dis(-1, 1);
			do {
				b = POS((CoordX(p)) + dis(random), CoordY(p) + dis(random));
			} while (!board->isEmpty(b));
			return b;
		} else if (board->isNearBoard(p, 1)) {
			expendCand(p, 3, 5);
		} else if (board->isNearBoard(p, 2)) {
			expendCand(p, 3, 4);
		}
	} else if (board->getMoveLeftCount() == 0)
		return NullPos;

	Pos best;
#ifndef VERSION_YIXIN_BOARD
	if (useOpeningBook) {
		best = databaseMove();
		if (board->isEmpty(best))
			return best;
	}
#endif

	node = nodeExpended = 0;
	ply = 0;
	maxPlyReached = 0;
	clearLose();

#ifdef VERSION_YIXIN_BOARD
	clearHash();
	MESSAGEL("Expect Time: " << timeForTurn() << "ms");
#else
	if (reloadConfig)
		clearHash();
#endif
	aiPiece = SELF;
	hashTable->newSearch();

	best = fullSearch();

	long time = timeUsed();
#ifdef VERSION_YIXIN_BOARD
	MESSAGEL("Node: " << node << " Speed: " << node / (time + 1) << "K");
	MESSAGEL("Time: " << time << " ms");
#else
	MESSAGEL("�ڵ���: " << node << " NPS: " << node / (time + 1) << "K");
	MESSAGEL("�ڵ�չ����: " << nodeExpended << " NPS: " << nodeExpended / (time + 1) << "K");
	MESSAGEL("ƽ����֧: " << (nodeExpended == 0 ? 0 : double(node) / nodeExpended));
	MESSAGEL("�ܺ�ʱ: " << time << " ms");
#endif
	return best;
}

Pos AI::fullSearch() {
	long lastDepthTime, turnTime;
	Move bestMove, move;
	int lastValue = -INF;
	int lastNode, lastNodeExpended;
	int searchDepth;
	int PVStableCount = 0;
	bool shouldBreak;

	turnTime = timeForTurn();

	moveLists[0].init_GenAllMoves();
	for (searchDepth = 2; searchDepth <= maxSearchDepth; searchDepth++) {
		lastDepthTime = getTime();
		lastNode = node;
		lastNodeExpended = nodeExpended;
		BestMoveChangeCount = 0;

		move = alphabeta_root(searchDepth, -INF, INF);
		if (!board->isEmpty(move.pos)) break;
		else if (move.value == -INF)  // ���ʱ��֪����һ����������ֵ������һ���Ĵ���
			move.value = bestMove.value; 
		bestMove = move;

		if (nodeExpended != lastNodeExpended) {
			if (BestMoveChangeCount == 0) {
				if (++PVStableCount >= BM_STABLE_MIN)
					turnTime = MAX(turnTime * TIME_DECREASE_PERCENTAGE / 100, timeForTurn() / TURNTIME_MIN_DIVISION);
			} else {
				PVStableCount = 0;
				if (searchDepth >= BM_CHANGE_MIN_DEPTH)
					turnTime = MIN(turnTime * TIME_INCRESE_PERCENTAGE / 100, timeForTurnMax());
			}
		}

		// ��ǰ�˳�|��ʱ�˳�|Ӯ�����˳�|ƽ���˳�
		shouldBreak = (timeLeft() / MATCH_SPARE_MAX < info.timeout_turn || timeUsed() > turnTime * TIMEOUT_PREVENT_MIN / 100) && (turnTime * 10 <= (getTime() - lastDepthTime) * TIMEOUT_PREVENT)
			|| terminateAI
			|| bestMove.value >= WIN_MIN || bestMove.value <= -WIN_MIN;

		// �������ڵ�(����ȡ�û���)���������Ϣ
		if (nodeExpended != lastNodeExpended || shouldBreak)
		#ifdef VERSION_YIXIN_BOARD
			MESSAGEL("Depth:" << searchDepth << "-" << maxPlyReached << " Val:" << bestMove.value << " Time:" << getTime() - lastDepthTime << "ms Node:" << (node - lastNode) / 1000000 << "M " << YXPos(bestMove.pos, board->size()));
	#else
			MESSAGEL("���: " << searchDepth << "-" << maxPlyReached << " ��ֵ: " << bestMove.value << " ���: " << PosStr(bestMove.pos) << " ���ʱ: " << getTime() - lastDepthTime << " ms");
	#endif
		if (shouldBreak) break;
		lastValue = bestMove.value;
	}

	if (!board->isEmpty(bestMove.pos))
		bestMove = Move(getHighestScoreCandPos(), 0);

#ifdef VERSION_YIXIN_BOARD
	MESSAGEL("Evaluation: " << bestMove.value << " | Best Point: " << YXPos(bestMove.pos, board->size()));
#else
	MESSAGEL("��ֵ: " << bestMove.value << " | ���: " << PosStr(bestMove.pos));
#endif

	Line line;
	fetchPVLineInTT(line, bestMove.pos, searchDepth);
#ifdef VERSION_YIXIN_BOARD
	MESSAGEL("BestLine: " << line.YXPrint(board->size()));
#else
	MESSAGEL("���·��: " << line);
#endif

	bool hasLose = false;
	FOR_EVERY_POSITION_POS(p) {
		if (cell(p).isLose) {
			if (!hasLose) {
				MESSAGES_BEGIN;
			#ifdef VERSION_YIXIN_BOARD
				MESSAGES("Lose Points: ");
			#else
				MESSAGES("�ذܵ�: ");
			#endif
				hasLose = true;
			}
		#ifdef VERSION_YIXIN_BOARD
			MESSAGES(" " << YXPos(p, board->size()));
		#else
			MESSAGES(" " << PosStr(p));
		#endif

		}
	}
	if (hasLose) MESSAGES_END;

	return bestMove.pos;
}

Move AI::alphabeta_root(int depth, int alpha, int beta) {
	node++;

	Move best;
	TTEntry * tte;	// �����û���
#ifdef Hash_Probe
	if (hashTable->probe(board->getZobristKey(), tte)) {
		if (tte->isValid(depth, alpha, beta, ply))
			return tte->bestMove(ply);
		else
			best.pos = tte->bestPos();
	}
#endif

	//���ɸ��ڵ��ŷ�
	MoveList & moveList = moveLists[0];
	moveList.hashMove = best.pos;
	WinState state = genMove_Root(moveList);
	MoveList::MoveIterator move = moveList.begin();

	if (state == State_Win || state == State_Lose) {
		return *move;
	} else if (moveList.moveCount() == 1) {
		terminateAI = true;
		TTEntry * tte;
		best.value = hashTable->probe(board->getZobristKey(), tte) ? tte->value(ply) : evaluate();
		best.pos = move->pos;
		return best;
	}

	nodeExpended++;
	float newDepth = (float)depth - getDepthReduction();
	HashFlag hashFlag = Hash_Alpha;
	int value;
	int availableCount = 0;

	rawStaticEval[ply] = rawEvaluate();
	minEvalPly = depth;

	do {
		if (cell(move->pos).isLose) {
			DEBUGL("PVS����" << PosStr(move->pos) << ": Lose");
			ANALYSIS("LOST", move->pos);
			continue;
		}

		lastSelfP4 = cell(move->pos).pattern4[SELF];
		lastOppoP4 = cell(move->pos).pattern4[OPPO];

		ANALYSIS("POS", move->pos);

		makeMove(move->pos);
		if (hashFlag == Hash_PV) {
			value = -alphabeta<NonPV>(newDepth, -(alpha + 1), -alpha, true);
			if (value > alpha && value < beta)
				value = -alphabeta<PV>(newDepth, -beta, -alpha, false);
		} else {
			value = -alphabeta<PV>(newDepth, -beta, -alpha, false);
		}
		undoMove();

		ANALYSIS("DONE", move->pos);

		if (terminateAI) {
			// ��ֹʱ,��δ������ķ�֧����
			if (availableCount == 0) { // �����δ�ѵ�����ѡ��
				if (best.value >= -WIN_MAX) { // ǰ���ѡ�㶼�Ǳذ�,ѡ��ǰѡ��
					best.value = -INF;
					best.pos = move->pos;
				} else {  // �����һ��ѡ�㶼û������,�˲�����
					best.pos = NullPos;
				}
			}
			break;
		}

		DEBUGL("PVS����" << PosStr(move->pos) << "  Ԥ��:" << move->value << "  ���:" << value << " Best.value = " << best.value);

		if (value <= -WIN_MIN) {
			cell(move->pos).isLose = true;
			move->value = -value;
			ANALYSIS("LOST", move->pos);
		} else {
			availableCount++;
			move->value -= 100;
		}

		if (value > best.value) {
			best.value = value;
			best.pos = move->pos;
			ANALYSIS("BEST", best.pos);
			BestMoveChangeCount++;
			move->value = value + 1000;
			// ���ڵ��value�����beta��
			if (value > alpha) {
				hashFlag = Hash_PV;
				alpha = value;
			}
		}

		if (availableCount > 0 && timeUsed() > timeForTurn() - TIME_RESERVED_PER_MOVE)
			terminateAI = true;

	} while (++move < moveList.end());

	if (availableCount <= 1) terminateAI = true;

#ifdef Hash_Record
	if (!terminateAI) {
		assert(board->isEmpty(best.pos));
		tte->save(board->getZobristKey(), best, depth, hashFlag, ply, hashTable->getGeneration());
	}
#endif
	ANALYSIS("REFRESH", NullPos);

	return best;
}

template <NodeType NT>
int AI::alphabeta(float depth, int alpha, int beta, bool cutNode) {
	const bool PvNode = NT == PV;
	assert(PvNode || (alpha == beta - 1));
	assert(!(PvNode && cutNode));

	node++;

	// Step 01. Mate Distance Purning
	alpha = MAX(-WIN_MAX + ply, alpha);
	beta = MIN(WIN_MAX - ply - 1, beta);
	if (alpha >= beta) return alpha;

	// Step 02. ��ǰʤ���ж�
	int staticEval = quickWinCheck();
	if (staticEval != 0) {
		updateMaxPlyReached(ply);
		return staticEval;
	}

	// Step 03. ƽ���ж�
	if (board->getMoveLeftCount() <= 1) return 0;

	// Step 04. ���澲̬��ֵ
	staticEval = evaluate();
	const Piece self = SELF, oppo = OPPO;
	int oppo5 = p4Count[oppo][A_FIVE];          // �Է��ĳ�����
	int oppo4 = oppo5 + p4Count[oppo][B_FLEX4]; // �Է��ĳ��ĺͻ�����

	// Step 05. Ҷ�ӽڵ��ֵ
	
	if (depth <= 0
		&& ply >= minEvalPly) {   // �Ƿ�ﵽ��Ͳ���
		updateMaxPlyReached(ply);

	#ifdef VCF_Leaf
		if (staticEval >= beta) {  // Ϊ�Է���ɱ
			if (oppo5 > 0) {  // �Է�������VCF
				VCFMaxPly = ply + MAX_VCF_PLY;
				attackerPiece = oppo;
				int mateValue = quickVCFSearch();
				if (mateValue <= -WIN_MIN) return mateValue;
			}
		} else {
			if (oppo5 == 0) {  // �ҿ��Գ���VCF
				VCFMaxPly = ply + MAX_VCF_PLY;
				attackerPiece = self;
				int mateValue = quickVCFSearch();
				if (mateValue >= WIN_MIN) return mateValue;
			} else if (staticEval >= alpha) {  // �Է�������VCF
				VCFMaxPly = ply + MAX_VCF_PLY;
				attackerPiece = oppo;
				int mateValue = quickVCFSearch();
				if (mateValue <= -WIN_MIN) return mateValue;
			}
		}
	#endif
		return staticEval;
	}
	
	TTEntry * tte;
	Pos ttMove = NullPos;  // �û�����¼���ŷ�
	bool ttHit;            // �Ƿ������û���
	int ttValue;           // �û����б������һ�������Ĺ�ֵ
	bool pvExact;          // �Ƿ�Ϊȷ����PV�ڵ�
	// Step 06. �����û���(Transposition Table Probe)
#ifdef Hash_Probe
	if (ttHit = hashTable->probe(board->getZobristKey(), tte)) {
		if (ply > singularExtensionPly) {   // ��������ʱ�������û����ض�
			int ttDepth = PvNode ? (int)roundf(depth) + 1 : (int)roundf(depth);
			ttValue = tte->value(ply);

			if (tte->isValid(ttDepth, alpha, beta, ply)) {
				return ttValue;
			} else {
				ttMove = tte->bestPos();
				// �û����б����ֵ�Ⱦ�̬��ֵ����ȷ
				if (tte->flag() == (ttValue > staticEval ? Hash_Beta : Hash_Alpha))
					staticEval = ttValue;
			}
		}
	}
#endif
	pvExact = PvNode ? isPvExact[ply - 1] && (isPvExact[ply] = ttHit && tte->flag() == Hash_PV)
		: (isPvExact[ply] = false);

	nodeExpended++;

	// Step 07. ��ʱ�ж�(Time Control)
	static int cnt = 0;
	if (--cnt < 0) {
		cnt = 3000;
		if (timeUsed() > timeForTurnMax()) 
			terminateAI = true;
	}

	rawStaticEval[ply] = rawEvaluate();    // ����ÿһ���ԭʼ��ֵ

	// Singular Extension skip all early purning (�������е����ڼ�֦)
	if (excludedMove) goto MoveLoops;

	// Step 08. Razoring (skipped when oppo4 > 0)
#ifdef Razoring
	if (!PvNode && depth < RazoringDepth) {
		if (staticEval + RazoringMargin[MAX((int)floorf(depth), 0)] < alpha) {
			return staticEval;
		}
	}
#endif

	// Step 09. Futility Purning : child node
#ifdef Futility_Pruning
	if (depth < FutilityDepth) {
		if (staticEval - FutilityMargin[MAX((int)floorf(depth), 0)] >= beta)
			return staticEval;
	}
#endif

	// Step 10. �ڲ���������(IID)
#ifdef Internal_Iterative_Deepening
	if (PvNode && !board->isEmpty(ttMove) && depth >= IIDMinDepth && oppo4 == 0) {
		TTEntry * tteTemp;
		alphabeta<NT>(depth * (2.f / 3.f), -beta, -alpha, cutNode);
		if (hashTable->probe(board->getZobristKey(), tteTemp)) {
			ttMove = tteTemp->bestPos();
		}
		assert(board->isEmpty(ttMove));
	}
#endif

MoveLoops:

	// Step 11. ѭ��ȫ���ŷ�
	assert(ply >= 0 && ply < MAX_PLY - 1);
	MoveList & moveList = moveLists[excludedMove ? ply + 1 : ply];
	moveList.init(ttMove);

	HashFlag hashFlag = Hash_Alpha;
	Move best;
	Pos move;
	int value;
	int branch = 0, maxBranch = getMaxBranch(ply);
	float newDepth = depth - (depth < 0 ? MIN(1.f, getDepthReduction())
		                                : getDepthReduction()); // ������ȵݼ�
	assert(best.value >= SHRT_MIN);

	bool singularExtensionNode = depth >= 8 && oppo5 == 0 && !excludedMove && ttHit  // �������ݹ����singular extension search
		&& tte->flag() == Hash_Beta && tte->depth() >= (int)roundf(depth) - 3;

	Pos last1 = board->getMoveBackWard(1);
	Pos last2 = board->getMoveBackWard(2);

	while (moveNext(moveList, move)) {
		if (move == excludedMove && ply == singularExtensionPly) continue;
		branch++;

		lastSelfP4 = cell(move).pattern4[self];
		lastOppoP4 = cell(move).pattern4[oppo];

		// Step 12. ����ʽ��֦
		int distance1 = distance(move, last1);
		int distance2 = distance(move, last2);
		bool isNear1 = distance1 <= CONTINUES_NEIGHBOR || isInLine(move, last1) && distance1 <= CONTINUES_DISTANCE;
		bool isNear2 = distance2 <= CONTINUES_NEIGHBOR || isInLine(move, last2) && distance2 <= CONTINUES_DISTANCE;;
		bool noImportantP4 = lastSelfP4 == NONE && lastOppoP4 == NONE;
		// ��֧����֦(���ܻ������ɱ)
		if (noImportantP4) {
			if (best.value > -WIN_MIN) {
				if (branch > maxBranch) continue;
			} else {
				if (branch > MAX(maxBranch, MAX_WINNING_CHECK_BRANCH)) {
					bool isNear3 = distance(move, board->getMoveBackWard(3)) <= CONTINUES_DISTANCE;
					if (!(isNear1 && isNear3))
						continue;
				}
			}

			// ���ؼ�֦
			if (!PvNode && best.value > -WIN_MIN) {
				// near-horizon
				if (ply >= minEvalPly - 2 && newDepth <= 1) {
					const int MinPreFrontierBranch = isNear1 ? (isNear2 ? 24 : 18) : 10;
					if (branch >= MinPreFrontierBranch) continue;
				}
			}
		}

		float moveDepth = newDepth;

		// Step 13. Singular extension search(��������)
		// ������һ���ŷ�����beta�������ŷ�����������(alpha-s, beta-s)ʱ��fail low,
		// ˵������ŷ��ǵ�һ��,��Ҫ����
	#ifdef Singular_Extension
		if (singularExtensionNode && move == ttMove) {
			int rBeta = MAX(ttValue - (int)ceilf(SEBetaMargin * depth), -WIN_MAX);
			float SEDepth = depth * 0.5f;
			int minEvalPlyTemp = minEvalPly;

			excludedMove = move;
			minEvalPly = 0;
			singularExtensionPly = ply;

			value = alphabeta<NonPV>(SEDepth, rBeta - 1, rBeta, cutNode);

			singularExtensionPly = -1;
			minEvalPly = minEvalPlyTemp;
			excludedMove = NullPos;

			if (value < rBeta)
				moveDepth += 1.0f;
		}
	#endif

		// Step 14. �³��ŷ�(Make move)
		makeMove(move);

		bool doFullDepthSearch = !PvNode || branch > 1;

		// Step 15. LMR(Late Move Reduction)
	#ifdef Late_Move_Reduction
		const int LMR_MinBranch = PvNode ? 30 : 20;
		if (depth >= 3 && oppo4 == 0 && branch >= LMR_MinBranch) {
			float reduction = 0.f;
			reduction = (branch - LMR_MinBranch) * 0.5f;
			
			if (pvExact) 
				reduction -= 1;

			if (selfP4 >= H_FLEX3)
				reduction += 1;

			if (oppoP4 >= H_FLEX3)
				reduction -= 1;
			
			if (cutNode)
				reduction += 2;

			reduction = MIN(reduction, moveDepth - newDepth * 0.4f);
			
			if (reduction > 0) {
				value = -alphabeta<false>(moveDepth - reduction, -(alpha + 1), -alpha, !cutNode);

				if (value <= alpha)
					doFullDepthSearch = false;
			}
		}
	#endif

		// Step 16. ��ȫ���� (Full depth search when no cut exist and LMR failed)
		if (doFullDepthSearch) {
			value = -alphabeta<NonPV>(moveDepth, -(alpha + 1), -alpha, !cutNode);
		}

		// Step 17. PV node full search.
		if (PvNode && (branch <= 1 || (value > alpha && value < beta))) {
			value = -alphabeta<PV>(moveDepth, -beta, -alpha, false);
		}

		// Step 18. �����ŷ�(Undo move)
		undoMove();

		// Step 19. ��ǰ��ֹ����(��ֹʱ����δ������ķ�֧����)
		if (terminateAI) break;

		// Step 20. ��������ŷ�
		if (value > best.value) {
			best.value = value;
			best.pos = move;
			if (value >= beta) {
				hashFlag = Hash_Beta;
				break;
			} else if (value > alpha) {
				hashFlag = Hash_PV;
				alpha = value;
			}
		}
	}

	assert(terminateAI || best.value >= -WIN_MAX && best.value <= WIN_MAX);

	// Step 21. �û�������(Transposition Table Record)
	// ��ǰ��ֹ��Singular Extensionʱ����
#ifdef Hash_Record
	if (!terminateAI && !excludedMove)
		tte->save(board->getZobristKey(), best, (int)roundf(depth), hashFlag, ply, hashTable->getGeneration());
#endif
	return best.value;
}

template int AI::alphabeta<PV>(float depth, int alpha, int beta, bool cutNode);
template int AI::alphabeta<NonPV>(float depth, int alpha, int beta, bool cutNode);

// ���� 0 ���û���ҵ� VCF
template <bool Root>
int AI::quickVCFSearch() {
	assert(attackerPiece == Black || attackerPiece == White);
	node++;

	const Piece self = SELF, oppo = OPPO;
	int value;
	TTEntry * tte;

	if (Root) {
	#ifdef Hash_Probe
		if (hashTable->probe(board->getZobristKey(), tte)) {
			if (tte->isMate())
				return tte->value(ply);
		}
	#endif
	}

	// ��ǰʤ���ж�
	value = quickWinCheck();
	if (value != 0) {
		updateMaxPlyReached(ply);
		return value;
	}

	// VCF����������
	if (ply > VCFMaxPly) {
		updateMaxPlyReached(ply);
		return 0;
	}

	if (p4Count[oppo][A_FIVE] > 0) {  // VCF�ڵ��ж���һ���Ƿ��ǳ���
		Pos move = getCostPosAgainstB4(board->getLastMove(), oppo);
		if (self == attackerPiece) {  // ������ǹ�����,��������
			if (cell(move).pattern4[self] < E_BLOCK4) {  // ����¶Է��ĳ��ĵ��岻���ҵĳ���
				updateMaxPlyReached(ply);
				return 0;
			}
		}

		makeMove<VC>(move);
		value = -quickVCFSearch<Root>();
		undoMove<VC>();

		return value;
	}

	assert(p4Count[oppo][A_FIVE] == 0);
	assert(self == attackerPiece);

	// ����������Ƿ���VCF�ŷ�
	if (p4Count[self][C_BLOCK4_FLEX3] == 0 && p4Count[self][D_BLOCK4_PLUS] == 0 && p4Count[self][E_BLOCK4] == 0) {
		updateMaxPlyReached(ply);
		return 0;
	}

	// ��ʱ�ж�
	static int cnt = 0;
	if (--cnt < 0) {
		cnt = 7000;
		if (timeUsed() > timeForTurnMax()) 
			return terminateAI = true, 0;
	}

	nodeExpended++;

	const GenLevel Level = Root ? InFullBoard : InLine;
	assert(ply >= 0 && ply < MAX_PLY);
	MoveList & moveList = moveLists[ply];
	moveList.init_GenAllMoves();
	if (Root)        // �������ɲ㼶����VCF�ŷ�
		genMoves_VCF(moveList);
	else
		genContinueMoves_VCF(moveList, RANGE_LINE_4, 32);

	if (moveList.moveCount() == 0)  // �ж��Ƿ�������Ľ����ŷ�
		return 0;

	sort(moveList.begin(), moveList.end(), std::greater<Move>());  // VCF�ŷ�����

	Move best;
	Pos attMove, defMove;
	assert(moveList.n == 0);

	do {
		attMove = moveList.moves[moveList.n].pos;
		
		makeMove<VC>(attMove);

		defMove = getCostPosAgainstB4(attMove, self);
		assert(cell(defMove).pattern4[self] == A_FIVE);
		assert(p4Count[self][A_FIVE] > 1 || defMove == findPosByPattern4(self, A_FIVE));

		makeMove<VC>(defMove);
		value = quickVCFSearch<false>();  // ����move�󲻷���
		undoMove<VC>();

		undoMove<VC>();

		if (value > best.value) {
			best.value = value;
			best.pos = attMove;
			if (value >= WIN_MIN) break;
		}
		if (terminateAI) break;

	#ifdef VCF_Branch_Limit
		if (moveList.n >= MAX_VCF_BRANCH - 1) break;
	#endif
	} while (++moveList.n < moveList.moveCount());

	if (best.value <= -WIN_MIN)  // ��Ϊ������������ֻ��VCF�ŷ�����ʹ����Ҳ������������
		best.value = 0;

#ifdef Hash_Record
	if (Root) {
		if (!terminateAI && best.value >= WIN_MIN) {
			tte->save(board->getZobristKey(), best, 0, HashFlag::Hash_PV, ply, hashTable->getGeneration());
		}
	}
#endif

	return best.value;
}

template int AI::quickVCFSearch<true>();
template int AI::quickVCFSearch<false>();

WinState AI::genMove_Root(MoveList & moveList) {
	switch (moveList.phase) {
	case MoveList::AllMoves:
		if (moveList.moveCount() > 0) {
			// ���ڵ������Ҫ����ԭʼ˳��
			stable_sort(moveList.begin(), moveList.end(), std::greater<Move>());
			return State_Unknown;
		} else
			moveList.phase = MoveList::GenAllMoves;
	case MoveList::GenAllMoves:
	{
		Piece self = SELF, oppo = OPPO;
		if (p4Count[self][A_FIVE] > 0) {
			moveList.addMove(findPosByPattern4(self, A_FIVE), WIN_MAX);
			return State_Win;
		} else if (p4Count[oppo][A_FIVE] >= 2) {
			moveList.addMove(findPosByPattern4(oppo, A_FIVE), -WIN_MAX + 1);
			return State_Lose;
		} else if (p4Count[oppo][A_FIVE] == 1) {
			moveList.addMove(findPosByPattern4(oppo, A_FIVE), 0);
			return State_Unknown;
		} else {
			if (p4Count[self][B_FLEX4] > 0) {
				moveList.addMove(findPosByPattern4(self, B_FLEX4), WIN_MAX - 2);
				return State_Win;
			} else if (p4Count[oppo][B_FLEX4] > 0) {
				genMoves_defence(moveList);
			} else {
				genMoves(moveList);
			}
		}
		// ��HashMove�ᵽ��ǰ
		if (board->isEmpty(moveList.hashMove)) {
			for (MoveList::MoveIterator move = moveList.begin(); move != moveList.end(); move++)
				if (move->pos == moveList.hashMove) {
					move->value += 10000;
					break;
				}
		}
		assert(moveList.moveCount() > 0);
		sort(moveList.begin(), moveList.end(), std::greater<Move>());
		moveList.phase = MoveList::AllMoves;
		return State_Unknown;
	}
	default: 
		assert(false);
		return State_Unknown;
	}
}

bool AI::moveNext(MoveList & moveList, Pos & pos) {
	switch (moveList.phase) {
	case MoveList::HashMove:
		moveList.phase = MoveList::GenAllMoves;
		if (board->isEmpty(moveList.hashMove)) {
			pos = moveList.hashMove;
			return true;
		}
	case MoveList::GenAllMoves:
	{
		moveList.phase = MoveList::AllMoves;
		if (p4Count[OPPO][A_FIVE] > 0) {
			pos = findPosByPattern4(OPPO, A_FIVE);
		} else {
			if (p4Count[OPPO][B_FLEX4] > 0) {
				genMoves_defence(moveList);
			} else {
				genMoves(moveList);
			}
			assert(moveList.moveCount() > 0);
			sort(moveList.begin(), moveList.end(), std::greater<Move>());
			pos = moveList.moves.front().pos;
		}
		return true;
	}
	case MoveList::AllMoves:
		if (++moveList.n >= moveList.moveCount())
			return false;
		pos = moveList.moves[moveList.n].pos;
		return true;
	default: assert(false); return false;
	}
}

// ����ȫ���ŷ�
void AI::genMoves(MoveList & moveList) {
	Piece self = SELF;
	int score;

	FOR_EVERY_CAND_POS(p) {
		if (!contestLegalMove(*board, p, self)) continue;
		score = cell(p).getScore(self);
		moveList.addMove(p, score);
	}
}
// ����ȫ�������������ŷ����������ĳ��ģ��Է����ĺͳ��ģ�
void AI::genMoves_defence(MoveList & moveList) {
	Piece self = SELF, oppo = OPPO;
	static set<Pos> defence;
	defence.clear();
	assert(p4Count[OPPO][B_FLEX4] > 0);

	FOR_EVERY_CAND_POS(p) {
		if (!contestLegalMove(*board, p, self)) continue;
		if (cell(p).pattern4[oppo] == B_FLEX4) {
			getAllCostPosAgainstF3(p, oppo, defence);
		} else if (cell(p).pattern4[self] >= E_BLOCK4) {
			moveList.addMove(p, cell(p).getScore(self));
		}
	}

	set<Pos>::iterator it2 = defence.end();
	for (set<Pos>::iterator it1 = defence.begin(); it1 != it2; it1++) {
		if (!contestLegalMove(*board, *it1, self)) continue;
		moveList.addMove(*it1, cell(*it1).getScore(oppo) + 10000);
	}
	assert(moveList.moveCount() > 0);
}
// ֻ��������/�����ڵĳ����ŷ�
void AI::genMoves_VCF(MoveList & moveList) {
	Piece self = SELF;

	FOR_EVERY_CAND_POS(p) {
		if (contestLegalMove(*board, p, self) && cell(p).pattern4[self] >= E_BLOCK4) {
			moveList.addMove(p, cell(p).getScore_VC(self));
		}
	}
}
// �����������Ľ����ŷ�
void AI::genContinueMoves_VCF(MoveList & moveList, const short * range, int n) {
	Piece self = SELF;
	Pos last = board->getMoveBackWard(2), p;

	if (last == NullPos) return;

	for (int i = 0; i < n; i++) {
		p = last + range[i];

		if (board->isEmpty(p) && contestLegalMove(*board, p, self)) {
			if (cell(p).pattern4[self] >= E_BLOCK4) {
				moveList.addMove(p, cell(p).getScore_VC(self));
			}
		}
	}
}

inline int AI::evaluate() {
	assert(p4Count[SELF][A_FIVE] == 0);

	// ȫ������
	int value = eval[SELF] - eval[OPPO];

	value = (value - rawStaticEval[ply - 1]) / 2;   // ��һ������ķ��������һ���Ǹ���

	return value;
}

inline int AI::rawEvaluate() {
	return eval[SELF] - eval[OPPO];
}

int AI::quickWinCheck() {
	Piece self = SELF, oppo = OPPO;
#ifdef Win_Check_FLEX3_2X
	bool has_FLEX3_2X = lastSelfP4 == F_FLEX3_2X;
#else
	bool has_FLEX3_2X = false;
#endif
	has_FLEX3_2X = false;

	if (p4Count[self][A_FIVE] >= 1) return WIN_MAX - ply;
	if (p4Count[oppo][A_FIVE] >= 2) return -WIN_MAX + ply + 1;
	if (p4Count[oppo][A_FIVE] == 1) return 0;
	if (p4Count[self][B_FLEX4] >= 1) return WIN_MAX - ply - 2;

	int self_C_count = p4Count[self][C_BLOCK4_FLEX3];
	if (self_C_count >= 1) {
		if (p4Count[oppo][B_FLEX4] == 0 && p4Count[oppo][C_BLOCK4_FLEX3] == 0 && p4Count[oppo][D_BLOCK4_PLUS] == 0 && p4Count[oppo][E_BLOCK4] == 0)
			return WIN_MAX - ply - 4;
		FOR_EVERY_CAND_POS(p) {    // ��43���ͷ����Ŀ�����֤(��̬�ж�)
			if (cell(p).pattern4[self] == C_BLOCK4_FLEX3) {
				makeMove<VC>(p);
				Pos defMove = getCostPosAgainstB4(p, self);
				if (cell(defMove).pattern4[oppo] < E_BLOCK4) {
					undoMove<VC>();
					return WIN_MAX - ply - 4;
				}
				undoMove<VC>();
				if (--self_C_count <= 0) goto Check_Flex3;
			}
		}
	}
Check_Flex3:
	if (p4Count[self][F_FLEX3_2X] >= 1) {
		if (p4Count[oppo][B_FLEX4] == 0 && p4Count[oppo][C_BLOCK4_FLEX3] == 0 && p4Count[oppo][D_BLOCK4_PLUS] == 0 && p4Count[oppo][E_BLOCK4] == 0)
			return WIN_MAX - ply - 4;
	}

#ifdef Win_Check_FLEX3_2X
	if (has_FLEX3_2X) { // �Է���������������
		assert(p4Count[oppo][B_FLEX4] > 0);

		// �ȼ������������ǲ����Ѿ��жϹ���
		TTEntry * tte;
		if (!hashTable->probe(board->getZobristKey(), tte)) { // �����û�жϹ�
			int value = quickDefenceCheck();
			if (value <= -WIN_MIN) return value;
		}
	}
#endif
	return 0;
}

int AI::quickDefenceCheck() {
	Piece self = SELF, oppo = OPPO;
	int oppoB_count = p4Count[oppo][B_FLEX4];
	assert(oppoB_count > 0);
	size_t selfB4_count = 0;
	static vector<Pos> self_BLOCK4;

	while (p4Count[self][D_BLOCK4_PLUS] + p4Count[self][E_BLOCK4] > 0) { // һֱ����ֱ��û�п��Գ��ĵ�ѡ��
		self_BLOCK4.clear();
		FOR_EVERY_CAND_POS(p) {
			Pattern4 p4 = cell(p).pattern4[self];
			if (p4 >= E_BLOCK4 && p4 != B_FLEX4) {
				Cell & c = cell(p);
				int dir;
				for (dir = 0; dir < 4; dir++) {
					Pattern pattern = c.pattern[self][dir];
					if (pattern >= B4) {
						Pos pos;
						int i;
						if (pattern == F5) pos = p;
						else {
							if (cell(pos = p - D[dir]).pattern4[self] == A_FIVE);
							else if (cell(pos = p + D[dir]).pattern4[self] == A_FIVE);
							else break;
						}
						for (i = 1; i <= 7; i++) {
							pos -= D[dir];
							Piece piece = board->get(pos);
							if (piece != self) break;
						}
						if (i > 7) continue;
						for (i = 1; i <= 7; i++) {
							pos += D[dir];
							Piece piece = board->get(pos);
							if (piece != self) break;
						}
						if (i > 7) continue;
						break;
					}
				}
				if (dir < 4) self_BLOCK4.push_back(p);
			}
		}
		if (self_BLOCK4.size() == 0) break;
		for (size_t i = 0; i < self_BLOCK4.size(); i++)
			makeMove<MuiltVC>(self_BLOCK4[i]);
		selfB4_count += self_BLOCK4.size();
		if (p4Count[self][B_FLEX4] > 0 || p4Count[self][C_BLOCK4_FLEX3] > 0) {
			oppoB_count = 0;
			break;  // ����VCF(α)ʱ�Լ����˻���
		}
	}
	if (oppoB_count > 0 && p4Count[oppo][B_FLEX4] == oppoB_count)
		oppoB_count = INF;

	for (size_t i = 0; i < selfB4_count; i++)
		undoMove<MuiltVC>();

	// ��Ϊÿ�����Ļ����2����,���ع��Ʋ�������2
	if (oppoB_count == INF)
		return -WIN_MAX + ply + 3 + (int)selfB4_count / 2;  
	return 0;
}

float AI::getDepthReduction() {
	Piece self = SELF, oppo = OPPO;

	int branchCount;

	if (p4Count[oppo][A_FIVE] == 1) {
		branchCount = 1;
	} else {
		int oppo_B_Count = p4Count[oppo][B_FLEX4];
		if (oppo_B_Count > 0) {
			branchCount = oppo_B_Count == 1 ? 3 : oppo_B_Count * 2;
		} else {
			branchCount = 0;
			FOR_EVERY_CAND_POS(p) {
				branchCount++;
			}
		}
	}
	assert(branchCount > 0);

	return logf((float)branchCount /*+ 1e-3f*/) * depthReductionBase;
}

void AI::fetchPVLineInTT(Line & line, Pos firstMove, int maxDepth) {
	if (maxDepth <= 0) return;
	line.pushMove(firstMove);
	makeMove(firstMove);
	TTEntry * tte;
	if (hashTable->probe(board->getZobristKey(), tte)) {
		Pos next = tte->bestPos();
		if (board->isEmpty(next))
			fetchPVLineInTT(line, next, maxDepth - 1);
	}
	undoMove();
}

void AI::tryReadConfig(string path) {
	std::ifstream file(path, std::ifstream::in);
	if (!file) return;
	const int LineSize = 1000;
	char line[LineSize];

	int override;
	file.getline(line, LineSize);
	sscanf_s(line, "Override:%d", &override);
	if (override != 1) {
		file.close();
		return;
	}
	override = 0;

	while (!file.eof()) {
		file.getline(line, LineSize);

		if (strncmp(line, "Eval:", 10) == 0) {
			for (int i = 0; i < 3876; i++)
				file >> Value[i];
			override++;
		} else if (strncmp(line, "Score:", 6) == 0) {
			for (int i = 0; i < 3876; i++)
				file >> Score[i];
			override++;
		} else if (strncmp(line, "ExtensionCoefficient:", 21) == 0) {
			int ExtensionNumBase;
			sscanf_s(line, "ExtensionCoefficient:%d", &ExtensionNumBase);
			depthReductionBase = ExtensionNumBase <= 1 ? 1e6f : 1.f / logf((float)ExtensionNumBase);
			override++;
		} else if (strncmp(line, "UseOpeningBook:", 15) == 0) {
			int opening;
			sscanf_s(line, "UseOpeningBook:%d", &opening);
			useOpeningBook = opening == 1;
			override++;
		} else if (strncmp(line, "FutilityPurningMargin:", 22) == 0) {
			FutilityDepth = sscanf_s(line, "FutilityPurningMargin:%d%d%d%d", FutilityMargin, FutilityMargin + 1, FutilityMargin + 2, FutilityMargin + 3);
			if (FutilityDepth > FUTILITY_MAX_DEPTH) FutilityDepth = FUTILITY_MAX_DEPTH;
			override++;
		} else if (strncmp(line, "RazoringMargin:", 15) == 0) {
			sscanf_s(line, "RazoringMargin:%d%d%d%d", RazoringMargin, RazoringMargin + 1, RazoringMargin + 2, RazoringMargin + 3);
			if (RazoringDepth > RAZORING_MAX_DEPTH) RazoringDepth = RAZORING_MAX_DEPTH;
			override++;
		} else if (strncmp(line, "IIDMinDepth:", 12) == 0) {
			sscanf_s(line, "IIDMinDepth:%d", &IIDMinDepth);
			override++;
		} else if (strncmp(line, "SEBetaMargin:", 13) == 0) {
			sscanf_s(line, "SEBetaMargin:%f", &SEBetaMargin);
			override++;
		} else if (strncmp(line, "ReloadConfigOnEachMove:", 23) == 0) {
			int reload;
			sscanf_s(line, "ReloadConfigOnEachMove:%d", &reload);
			reloadConfig = reload == 1;
			override++;
		}
	}
	file.close();

#ifdef VERSION_YIXIN_BOARD
	MESSAGEL("Custom config has been read, " << override << " properties changed.");
#else
	MESSAGEL("Custom config has been read, " << override << " properties changed.");
#endif
}


/* ===== rapfi_main.inc ===== */

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int me, x;
    if (!(std::cin >> me)) return 0;
    std::vector<Pos> stones[2];
    for (int r = 0; r < 15; ++r)
        for (int c = 0; c < 15; ++c) {
            std::cin >> x;
            if (x == 0 || x == 1) stones[x].push_back(POS(r, c));
        }

    Board board(15);
    AI ai(&board);
    size_t rounds = std::max(stones[0].size(), stones[1].size());
    for (size_t i = 0; i < rounds; ++i) {
        if (i < stones[0].size()) ai.makeMove(stones[0][i]);
        if (i < stones[1].size()) ai.makeMove(stones[1][i]);
    }
    if (int(board.getPlayerToMove()) != me) return 2;

    ai.info.timeout_turn = 780;
    ai.info.time_left = 100000000;
    ai.info.setMaxMemory(256 * 1024 * 1024L);
    std::ostringstream diagnosticSink;
    std::streambuf *originalOut = std::cout.rdbuf(diagnosticSink.rdbuf());
    Pos p = ai.turnMove();
    std::cout.rdbuf(originalOut);
    std::cout << int(CoordX(p)) << ' ' << int(CoordY(p)) << '\n';
    return 0;
}

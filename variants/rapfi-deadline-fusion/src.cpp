
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
// GCC 11.2.0 on the judge compiles this file with a fixed `-O2`, but -O3 plus
// loop unrolling is worth about 8% on the incremental pattern update, which is
// two thirds of the search.  Measured node-for-node identical, and warning-free
// under the judge's exact command on both aarch64 and x86-64 GCC 11.2.0.
#pragma GCC optimize("O3","unroll-loops")
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <climits>
#include <chrono>
#include <csignal>
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#define CONTEST_POSIX_WATCHDOG 1
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wnarrowing"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif
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

// Plain `char` is unsigned by default on the judge's aarch64 target.  Rapfi
// uses Int8 for negative board deltas, so make the original x86 assumption
// explicit or directions such as -33 become 223 and index outside the board.
typedef signed char Int8;
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

// Seeded by the clock upstream, which in a per-move process means the same
// position answers differently from one second to the next: the opening
// database picks a random stored reply and the second move is a random
// neighbour.  A fixed seed makes a game reproducible, which is what lets a
// line be studied offline and then replayed move for move.
static std::mt19937_64 rapfiRandom(0x9E3779B97F4A7C15ull);

inline void toupper(string & str) {
	for (size_t i = 0; i < str.size(); i++) {
		char &c = str[i];
		if (c >= 'a' && c <= 'z') {
			c += 'A' - 'a';
		}
	}
}

// ���ص�ǰʱ��(��λ:ms)
using WallClock = std::chrono::steady_clock;
static const WallClock::time_point processStart = WallClock::now();

// This clock starts running at static-initialization time, which is already
// after fork/exec and after the dynamic linker has done its work, while the
// judge's one second is counted from process creation.  On Linux that gap can
// be measured instead of guessed: /proc/self/stat field 22 is the process start
// time in clock ticks since boot, and /proc/uptime is the current time on the
// same scale.  Reading them costs well under a millisecond and turns the
// remaining timing risk into a number.  If either file is unavailable the offset
// stays zero and the deadlines simply behave as before.
static long measureStartupOffsetMs() {
#ifdef CONTEST_POSIX_WATCHDOG
	std::FILE * uptimeFile = std::fopen("/proc/uptime", "r");
	if (!uptimeFile) return 0;
	double uptime = 0.0;
	int uptimeFields = std::fscanf(uptimeFile, "%lf", &uptime);
	std::fclose(uptimeFile);
	if (uptimeFields != 1) return 0;

	std::FILE * statFile = std::fopen("/proc/self/stat", "r");
	if (!statFile) return 0;
	char line[1024];
	char * text = std::fgets(line, sizeof(line), statFile);
	std::fclose(statFile);
	if (!text) return 0;

	// Field 2 is the executable name in parentheses and may itself contain
	// spaces, so fields are counted from the last closing parenthesis.
	char * cursor = std::strrchr(line, ')');
	if (!cursor) return 0;
	++cursor;
	long long startTicks = -1;
	for (int field = 3; field <= 22; ++field) {
		while (*cursor == ' ') ++cursor;
		if (!*cursor) return 0;
		char * end = cursor;
		while (*end && *end != ' ') ++end;
		if (field == 22) {
			char saved = *end;
			*end = '\0';
			startTicks = std::atoll(cursor);
			*end = saved;
		}
		cursor = end;
	}
	if (startTicks < 0) return 0;

	long ticksPerSecond = sysconf(_SC_CLK_TCK);
	if (ticksPerSecond <= 0) return 0;
	double age = uptime - double(startTicks) / double(ticksPerSecond);
	if (age < 0.0 || age > 1.0) return 0;   // implausible, do not trust it
	return long(age * 1000.0);
#else
	return 0;
#endif
}

static const long startupOffsetMs = measureStartupOffsetMs();
// The judge restarts this executable for every move and gives each process its
// own 1 s wall clock, so time that is not spent on this move is lost, not
// banked for a later one.  PROCESS_DEADLINE_MS is the soft target the search
// aims for; WATCHDOG_DEADLINE_MS is a hard backstop enforced by SIGALRM, which
// prints the best move found so far and exits.  With the backstop in place the
// soft target can sit much closer to the limit without risking a TLE loss.
// The judge reports each run's wall time, so the gap is measured rather than
// guessed: our own submission aiming at 820 ms came back as 802.7 ms, and the
// strongest opponent on the board runs 912-918 ms without ever being cut off.
// A 900 ms target therefore lands around 883 ms, still inside what that opponent
// demonstrates is accepted, with the SIGALRM backstop 60 ms further out.  Both
// values can be raised through GOMOKU_DEADLINE_MS for offline analysis (opening
// book generation, deep position study); the contest never sets that variable
// and therefore always uses the defaults below.
static long PROCESS_DEADLINE_MS = 900;
static long WATCHDOG_DEADLINE_MS = 960;
static const long WATCHDOG_MARGIN_MS = 80;
// Milliseconds since the judge started this process, not since main() began.
inline long getTime() {
	return startupOffsetMs
		+ (long)std::chrono::duration_cast<std::chrono::milliseconds>(WallClock::now() - processStart).count();
}

// ---- Hard wall-clock backstop ---------------------------------------------
// The search updates contestPublish() with the best legal move it has proven so
// far.  If anything overruns the soft deadline the SIGALRM handler writes that
// move and exits, so the process can never lose on time.  Only write(2) and
// _exit(2) are used inside the handler; both are async-signal-safe.
static volatile sig_atomic_t contestBestRow = -1;
static volatile sig_atomic_t contestBestCol = -1;

static inline void contestPublish(int row, int col) {
	if (row >= 0 && row < 15 && col >= 0 && col < 15) {
		contestBestRow = row;
		contestBestCol = col;
	}
}

#ifdef CONTEST_POSIX_WATCHDOG
extern "C" void contestWatchdogHandler(int) {
	int row = contestBestRow, col = contestBestCol;
	if (row < 0 || col < 0) _exit(1);
	char buf[8];
	int n = 0;
	if (row >= 10) buf[n++] = char('0' + row / 10);
	buf[n++] = char('0' + row % 10);
	buf[n++] = ' ';
	if (col >= 10) buf[n++] = char('0' + col / 10);
	buf[n++] = char('0' + col % 10);
	buf[n++] = '\n';
	ssize_t written = write(1, buf, n);
	(void)written;
	_exit(0);
}

static void contestArmWatchdog() {
	struct sigaction sa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = contestWatchdogHandler;
	sigaction(SIGALRM, &sa, nullptr);

	long remain = WATCHDOG_DEADLINE_MS - getTime();
	if (remain < 1) remain = 1;
	struct itimerval timer;
	std::memset(&timer, 0, sizeof(timer));
	timer.it_value.tv_sec = remain / 1000;
	timer.it_value.tv_usec = (remain % 1000) * 1000;
	setitimer(ITIMER_REAL, &timer, nullptr);
}

static void contestDisarmWatchdog() {
	struct itimerval timer;
	std::memset(&timer, 0, sizeof(timer));
	setitimer(ITIMER_REAL, &timer, nullptr);
}
#else
static void contestArmWatchdog() {}
static void contestDisarmWatchdog() {}
#endif

/* ===== Opening book =========================================================
 * Generated by book_build.py from searches given seconds per move instead of
 * the contest's one, and inserted between the two markers below.  A position is
 * stored in one canonical orientation out of the eight board symmetries, so an
 * entry found during analysis also covers all of its rotations and mirrors.
 * Keys are FNV-1a 64 over the 225-character board plane; the move is packed as
 * row * 15 + col in the same canonical orientation.
 */
struct ContestBookEntry {
	unsigned long long key;
	unsigned char move;
};

static const ContestBookEntry CONTEST_BOOK[] = {
/* BOOK-BEGIN */
	{136724680927428445ull, 142},
	{155075007725977348ull, 125},
	{244459415681846240ull, 145},
	{270288769295963212ull, 141},
	{291441968897777859ull, 140},
	{472522793932075537ull, 127},
	{578828477200917452ull, 94},
	{585355502992913896ull, 129},
	{607469691932018989ull, 109},
	{794954091736609172ull, 110},
	{814229270163483290ull, 143},
	{831506863838267259ull, 159},
	{856267815196294961ull, 66},
	{938809129822981162ull, 142},
	{1098916733502333799ull, 139},
	{1162965746810963002ull, 81},
	{1171569356945566706ull, 84},
	{1273410702916178188ull, 157},
	{1312936826703684914ull, 83},
	{1350558649169574846ull, 146},
	{1427611622849644988ull, 83},
	{1450960246397437906ull, 69},
	{1531761003600154926ull, 154},
	{1738539680693823502ull, 97},
	{1764865594543103185ull, 110},
	{1790615249282420862ull, 53},
	{2014942935636039867ull, 111},
	{2204426660194354804ull, 159},
	{2296770630526993064ull, 155},
	{2499867430927436912ull, 69},
	{2576108008822723355ull, 53},
	{2729232781046406742ull, 82},
	{3006453112047185290ull, 155},
	{3119223309062398081ull, 142},
	{3141687293862521892ull, 125},
	{3270274571439125984ull, 84},
	{3299566117633650225ull, 112},
	{3309864790411829353ull, 109},
	{3384372374210444029ull, 82},
	{3578347126636653518ull, 127},
	{3587814620640917712ull, 172},
	{3623172059533187559ull, 95},
	{3767321938806917690ull, 69},
	{3798024370608596635ull, 130},
	{3997787440687074979ull, 130},
	{4027030714714727482ull, 141},
	{4096143896979018760ull, 99},
	{4264490368042224342ull, 98},
	{4298072642671422273ull, 113},
	{4404837856064721053ull, 140},
	{4495254606419232509ull, 144},
	{4547393408656045356ull, 158},
	{4637938735819134774ull, 98},
	{4736309422065500626ull, 81},
	{5192803440222850372ull, 142},
	{5288444797591842940ull, 157},
	{5871562639656917642ull, 109},
	{5913330134648294096ull, 115},
	{5924581508340007823ull, 83},
	{5938526365443820796ull, 156},
	{5987708504432644427ull, 140},
	{6484968531220541784ull, 82},
	{6531283151983102048ull, 144},
	{6648647068637481100ull, 115},
	{6700610041740672645ull, 144},
	{6818318053093977288ull, 99},
	{6831357710860921684ull, 141},
	{6851818516163393180ull, 111},
	{6856128424025385120ull, 158},
	{6923292006151795646ull, 84},
	{7403041861433522893ull, 93},
	{7507679342375161100ull, 155},
	{7774234538829876610ull, 139},
	{7938481996401810169ull, 79},
	{8151722485012787272ull, 99},
	{8310322032992541944ull, 130},
	{8348887967035475632ull, 174},
	{8387051519771245816ull, 113},
	{8406718149365111690ull, 94},
	{8544152777556706256ull, 114},
	{8628722313892958578ull, 96},
	{8706368968943965070ull, 121},
	{8835850785849034619ull, 145},
	{8893587380797456807ull, 154},
	{8912985201160313632ull, 124},
	{8930817629954057509ull, 83},
	{8998782232872514234ull, 172},
	{9008082481175298360ull, 145},
	{9019373303830839768ull, 125},
	{9318125225923670176ull, 130},
	{9346411331797609429ull, 82},
	{9508379580731954546ull, 115},
	{9645641625170286573ull, 96},
	{9803630106753202908ull, 143},
	{9885904725519324954ull, 171},
	{9955821253719382899ull, 155},
	{10018755187192241772ull, 84},
	{10442597814046487848ull, 81},
	{10638976654756225124ull, 144},
	{10907389580706149740ull, 110},
	{10979560152960394264ull, 101},
	{11084244360952641127ull, 96},
	{11099146183339727704ull, 80},
	{11119414655678148803ull, 141},
	{11312290930966897302ull, 159},
	{11427757145854826852ull, 158},
	{11430954485656643670ull, 99},
	{11432037041403312222ull, 130},
	{11835240388745314594ull, 96},
	{11910218148801061274ull, 83},
	{11945840583873523675ull, 145},
	{12219700443787896595ull, 158},
	{12243446553510203219ull, 115},
	{12264716189294849619ull, 110},
	{12368250543610409402ull, 114},
	{12436993072919958368ull, 189},
	{12466544160905376152ull, 155},
	{12593529832626500987ull, 155},
	{12663575443540092246ull, 157},
	{13006700459009227428ull, 145},
	{13182238951063273181ull, 109},
	{13205102898710970232ull, 110},
	{13206725170263094596ull, 65},
	{13290364515772506098ull, 110},
	{13322459881913264914ull, 124},
	{13529656597765675498ull, 143},
	{13571617300608126004ull, 144},
	{13572301763705136290ull, 154},
	{13689783417385623110ull, 98},
	{14107883947512476622ull, 155},
	{14280837240968947668ull, 155},
	{14354858358642271104ull, 66},
	{14511227165574234409ull, 129},
	{14556987318473674006ull, 94},
	{14592046235987181520ull, 144},
	{14688593780829692501ull, 82},
	{14882269207463631837ull, 82},
	{15074663354974703320ull, 155},
	{15252191417141825832ull, 101},
	{15392980598294909600ull, 100},
	{15888254911385447986ull, 143},
	{15926120742100504423ull, 96},
	{16426542252868945110ull, 97},
	{16471038341541501916ull, 125},
	{16581639009516568137ull, 113},
	{16863775629951365502ull, 126},
	{16894694673467278143ull, 108},
	{17587868615005650127ull, 129},
	{17669003234996236814ull, 140},
	{17674497544075264176ull, 141},
	{18194190516208731678ull, 156},
	{18209320786928440922ull, 158},
	{18253832013342230792ull, 70},
	{18269926925656313914ull, 140},
/* BOOK-END */
};
static const int CONTEST_BOOK_SIZE = int(sizeof(CONTEST_BOOK) / sizeof(CONTEST_BOOK[0]));

// The eight symmetries of the square board.  Must stay identical to TRANSFORMS
// in book_build.py, including their order, or generated keys will not match.
static inline void contestTransform(int index, int row, int col, int & outRow, int & outCol) {
	switch (index) {
	case 0: outRow = row;          outCol = col;          break;
	case 1: outRow = col;          outCol = 14 - row;     break;
	case 2: outRow = 14 - row;     outCol = 14 - col;     break;
	case 3: outRow = 14 - col;     outCol = row;          break;
	case 4: outRow = row;          outCol = 14 - col;     break;
	case 5: outRow = 14 - row;     outCol = col;          break;
	case 6: outRow = col;          outCol = row;          break;
	default: outRow = 14 - col;    outCol = 14 - row;     break;
	}
}
static const int CONTEST_INVERSE[8] = {0, 3, 2, 1, 4, 5, 6, 7};

static unsigned long long contestBookHash(const char * plane, int length) {
	unsigned long long hash = 14695981039346656037ull;
	for (int i = 0; i < length; ++i) {
		hash ^= (unsigned char)plane[i];
		hash *= 1099511628211ull;
	}
	return hash;
}

// Root moves the search must skip.  Only ever filled from GOMOKU_EXCLUDE, which
// exists so the book generator can ask for the second and third best reply in a
// position instead of only the principal variation.
static std::vector<int> contestRootExclude;   // row * 15 + col
static int contestMaxDepth = 0;               // 0 = no cap
// Budget for the root threat search (VCT).  Both are overridable so that the
// two halves of an A/B run can be built from one source.
static long VCT_ROOT_BUDGET_MS = 170;
static long VCT_ROOT_NODE_BUDGET = 400000;
static bool contestStats = false;

// Reads the two analysis-only environment variables.  Absent variables leave the
// contest defaults untouched.
static void contestReadEnv() {
	if (const char * budget = std::getenv("GOMOKU_DEADLINE_MS")) {
		long ms = std::atol(budget);
		if (ms > 0) {
			PROCESS_DEADLINE_MS = ms;
			WATCHDOG_DEADLINE_MS = ms + WATCHDOG_MARGIN_MS;
		}
	}
	// A depth cap makes a measurement independent of machine load: the same search
	// is run either way, so only the wall time differs between two builds.
	if (const char * depth = std::getenv("GOMOKU_MAX_DEPTH")) {
		int d = std::atoi(depth);
		if (d > 0) contestMaxDepth = d;
	}
	if (const char * vctMs = std::getenv("GOMOKU_VCT_MS")) {
		long ms = std::atol(vctMs);
		if (ms >= 0) VCT_ROOT_BUDGET_MS = ms;
	}
	if (const char * vctNodes = std::getenv("GOMOKU_VCT_NODES")) {
		long n = std::atol(vctNodes);
		if (n >= 0) VCT_ROOT_NODE_BUDGET = n;
	}
	if (std::getenv("GOMOKU_STATS")) contestStats = true;
	if (const char * excluded = std::getenv("GOMOKU_EXCLUDE")) {
		int row = -1, col = -1;
		const char * cursor = excluded;
		while (*cursor) {
			if (std::sscanf(cursor, "%d,%d", &row, &col) == 2
				&& row >= 0 && row < 15 && col >= 0 && col < 15)
				contestRootExclude.push_back(row * 15 + col);
			while (*cursor && *cursor != ';') ++cursor;
			if (*cursor == ';') ++cursor;
		}
	}
}

#ifdef _DEBUG
#define DEBUGL(message) cout << message << endl
#define MESSAGEL(message) ((void) 0)
#define MESSAGES_BEGIN 
#define MESSAGES(message) ((void) 0)
#define MESSAGES_END
#define ANALYSIS(type, pos) ((void) 0)
#else
#define DEBUGL(message)
#define MESSAGEL(message) ((void) 0)
#define MESSAGES_BEGIN
#define MESSAGES(message) ((void) 0)
#define MESSAGES_END

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
	// A move that completes an exact five is NOT exempt from the four-four ban.
	// The website checks the ban before it checks for a win (judge order in
	// arena_server.play_game and gomoku_match, and SITE_API.md), so a black
	// point that makes five in one direction while forming two fours in others
	// is an immediate loss, not a victory.  Treating it as a win made the search
	// actively seek out that exact losing move.

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

// True when placing `side` at `move` completes a run of exactly five somewhere.
// An overline wins for nobody here, so a point that only ever reaches six or more
// is not a winning point.  For black such a point is the long-connection ban and
// contestForbiddenBlack already rejects it; white may legally play it, it simply
// does not win, and the pattern tables classify any run of five or more as a
// five.  Without this the search reports a win it will never be awarded and
// stops looking for the move that would actually win.
static bool contestMakesExactFive(const Board &board, Pos move, Piece side) {
	for (int d = 0; d < 4; ++d) {
		int n = 1;
		for (int sign : {-1, 1})
			for (int k = 1; ; ++k) {
				Pos p = Pos(int(move) + sign * k * D[d]);
				if (!board.isInBoard(p) || board.get(p) != side) break;
				++n;
			}
		if (n == 5) return true;
	}
	return false;
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
            if (board->isEmpty(pos) && isCand(pos)) \

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

	// PATTERN2[k1][k2] packs both colours' patterns for one direction into one
	// byte: the low nibble is the pattern of the side whose key is k1, the high
	// nibble the pattern of the side whose key is k2.  The old table needed two
	// lookups at unrelated addresses (PATTERN[k1][k2] and PATTERN[k2][k1]) to
	// learn the same thing; this is the same 64 KB and half the accesses.
	static UInt8 PATTERN2[256][256];  // 65536 = 256 * 256 = 4 ^ 8
	// Flat, because PCODE[a][b][c][d] was already at flat index
	// a<<12 | b<<8 | c<<4 | d, which is exactly how Cell::pat packs the four
	// direction patterns.  The packed word is the index; nothing has to be
	// reassembled from bytes on every cell update.
	static PatternCode PCODE[65536];  // 65536 = 16 ^ 4
	
	// 3876 �Ǵ�16�ֵ���������ѡ�����ظ���4�����͵������
	static Pattern4 PATTERN4[3876];

protected:
	static short Score[3876];
	static short Value[3876];

	// Score[code], Value[code] and PATTERN4[code] are always wanted together and
	// were three arrays, so every touched cell paid three address computations
	// and three scattered loads per colour.  Packed into one 64-bit word per
	// code they are a single load: score | value << 16 | pattern4 << 32.
	static U64 PINFO[3876];
	static inline short pinfoScore(U64 v) { return (short)(UShort)v; }
	static inline short pinfoValue(U64 v) { return (short)(UShort)(v >> 16); }
	static inline Pattern4 pinfoP4(U64 v) { return Pattern4((UInt8)(v >> 32)); }
	static void buildPatternInfo();

	struct Cell {
		UInt8 key[4][2]; // 4������� Black Key �� White Key
		// The four direction patterns of one colour, one per nibble, most
		// significant first: p0<<12 | p1<<8 | p2<<4 | p3.  Storing them packed
		// instead of as Pattern pattern[2][4] means the pattern code is a single
		// 16-bit load used directly as the PCODE index, rather than four byte
		// loads plus three shifts and three adds per colour per touched cell.
		UShort pat[2];
		short score[2];
		short eval[2];
		Pattern4 pattern4[2];
		// Non-zero when the table's own Pattern4 for this cell is one the contest
		// rules can still override: B_FLEX4 or better for black (four-four and
		// overline bans) or A_FIVE for white (a five that may be an overline).
		// A cell whose stone pattern did not change can only change pattern4 if
		// this is set, which is what lets the fifth-away refresh skip 99% of its
		// work.  It cannot be derived from pattern4 itself, because an overridden
		// white A_FIVE is stored as NONE and is then indistinguishable from a
		// cell that is genuinely NONE.
		UInt8 overridable;
		UInt8 padding_;

		static inline int patShift(int dir) { return 12 - 4 * dir; }
		inline Pattern pattern(Piece piece, int dir) const {
			return Pattern((pat[piece] >> patShift(dir)) & 0xF);
		}

		inline void clearPattern4() {
			pattern4[White] = pattern4[Black] = NONE;
			overridable = 0;
		}
		inline void clearEval() {
			eval[Black] = eval[White] = 0;
		}
		inline void updatePattern(UInt8 keyBlack, UInt8 keyWhite, int sh, UShort keep) {
			const UInt8 both = PATTERN2[keyBlack][keyWhite];
			pat[Black] = UShort((pat[Black] & keep) | (UShort(both & 0xF) << sh));
			pat[White] = UShort((pat[White] & keep) | (UShort(both >> 4) << sh));
		}
		inline void updatePattern(int i) {
			updatePattern(key[i][Black], key[i][White], patShift(i),
				UShort(~(0xF << patShift(i))));
		}
		inline PatternCode getPatternCode(Piece piece) const {
			return PCODE[pat[piece]];
		}
		static inline UInt8 isOverridable(Pattern4 rawBlack, Pattern4 rawWhite) {
			return UInt8(rawBlack >= B_FLEX4 || rawWhite == A_FIVE);
		}
		inline void updatePattern4(PatternCode codeBlack, PatternCode codeWhite) {
			pattern4[Black] = pinfoP4(PINFO[codeBlack]);
			pattern4[White] = pinfoP4(PINFO[codeWhite]);
			overridable = isOverridable(pattern4[Black], pattern4[White]);
		}
		inline void updatePattern4(Piece piece) {
			pattern4[piece] = pinfoP4(PINFO[getPatternCode(piece)]);
		}
		inline void updateScore(PatternCode codeBlack, PatternCode codeWhite) {
			score[Black] = pinfoScore(PINFO[codeBlack]);
			score[White] = pinfoScore(PINFO[codeWhite]);
		}
		inline void updateEval(PatternCode codeBlack, PatternCode codeWhite) {
			eval[Black] = pinfoValue(PINFO[codeBlack]);
			eval[White] = pinfoValue(PINFO[codeWhite]);
		}
		inline int getScore(Piece player) { return (int)score[Black] + score[White] + score[player]; }
		inline int getScore() { return (int)score[Black] + score[White]; }
		inline int getScore_VC(Piece player) { return (int)score[player]; }
	};

	// Undo by snapshot.  undoMove used to run the whole incremental update a
	// second time in reverse, which costs as much as making the move; instead
	// makeMove copies each Cell it is about to change into a per-level buffer and
	// undoMove copies them back.  The restored bytes are the bytes that were
	// there, so the state after an undo is identical by construction rather than
	// by the update happening to be exactly invertible.
	//
	// The slots are addressed positionally, with no bookkeeping: the walk visits
	// the same cells on the way back as on the way out.  It never touches the
	// played point itself -- only its neighbours at 1..4 and 5 steps -- so the
	// stone that makeMove placed (and undoMove removed) cannot change any walked
	// cell's emptiness, and `board->isEmpty(p)` answers the same both times.
	static const int UNDO_DIR_SLOTS = 8;                   // 4 each side
	static const int UNDO_SLOTS = 4 * UNDO_DIR_SLOTS + 8;  // + the fifth-away ends
	// A move consumes an empty point, so make/undo can never nest deeper than the
	// 225 points of the board.  Static rather than a member so the one Evaluator
	// object still fits comfortably on the stack.
	static const int UNDO_LEVELS = 256;
	static Cell undoCells[UNDO_LEVELS][UNDO_SLOTS];
	// The two running aggregates that are not stored in the cells.  Copying all
	// 104 bytes of them costs thirteen 8-byte moves, against the 4 p4Count
	// read-modify-writes plus one eval read-modify-write per touched cell that
	// undoing them one cell at a time needs -- about ninety dependent memory
	// operations for the twenty-odd cells a move touches.
	struct Aggregates {
		int eval[2];
		int p4Count[2][12];
	};
	static Aggregates undoAggregates[UNDO_LEVELS];
	int undoLevel;

	Board * board;
	Cell cells[Board::MaxBoardSizeSqr];
	UInt8 cands[Board::MaxBoardSizeSqr];
	bool loses[Board::MaxBoardSizeSqr];

	int eval[2], evalLower[2];
	int p4Count[2][12];
	int ply;

	inline Cell & cell(Pos p) { assert(p < Board::MaxBoardSizeSqr); return cells[p]; }
	inline Cell & cell(int x, int y) { return cell(POS(x, y)); }
	inline UInt8 & cand(Pos p) { return cands[p]; }
	inline bool isCand(Pos p) const { return cands[p] > 0; }
	inline bool & isLose(Pos p) { return loses[p]; }

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

private:
	// The incremental update of one touched cell in one direction, and the walk
	// over the eight cells of one direction.  Dir is a template parameter so the
	// nibble position inside Cell::pat and the step D[Dir] are compile-time
	// constants: as a runtime loop variable they cost a variable shift and about
	// seven extra instructions on every one of the ~21 cells a move touches.
	template <MoveType MT>
	void updateCell(Pos p, int dir, UInt8 kBlack, UInt8 kWhite, int sh, UShort keep,
		Cell * slot);
	template <MoveType MT> void updateLine(Pos pos, int dir, Piece self, Cell * slot);
	template <MoveType MT> void undoLine(Pos pos, int dir, const Cell * slot);
	void refreshPattern4(Pos p);
public:
	
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
	static const long TIME_RESERVED_PER_MOVE = 60;   // ms
	static const int MATCH_SPARE = 23;         // how much is time spared for the rest of game
	static const int MATCH_SPARE_MIN = 7;      // min time spared for the rest of game
	static const int MATCH_SPARE_MAX = 40;     // max time spared for the rest of game
	static const int TIMEOUT_PREVENT = 45;     // alphabeta become slow when depth increases
	// A new iteration is started only if the time already spent plus this
	// multiple of the previous iteration's cost still fits inside the budget.
	// 150 means "assume the next depth costs about 1.5x the last one", which is
	// deliberately optimistic: overshooting is cheap because the mid-iteration
	// cutoff keeps whatever the partial search proved, while stopping early
	// wastes budget outright, since the judge does not carry time to the next
	// move.
	static const int NEXT_ITERATION_COST_PERCENTAGE = 150;
	static const int TIMEOUT_PREVENT_MIN = 70; // (TIMEOUT_PREVENT_MIN / 100.0)�ٷ���
	static const int BM_CHANGE_MIN = 3;        // if bestmove changes more than this, increase time
	static const int BM_CHANGE_MIN_DEPTH = 7;  // max bestmove changes taken into account
	static const int BM_STABLE_MIN = 3;        // if bestmove remains same more than this, decrease time
	static const int TIME_INCRESE_PERCENTAGE = 105;  // ʱ�����ӵİٷ���
	static const int TIME_DECREASE_PERCENTAGE = 100;  // ʱ����ٵİٷ���
	static const int TURNTIME_MIN_DIVISION = 1;      // min time for this turn

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

	// VCT (victory by continuous threats) limits.  A threat search branches far
	// wider than a VCF, so every one of these is a hard cap: an attack that
	// finds nothing has to cost a small fixed slice of the move's second.
	static const int MAX_VCT_MOVES = 64;      // attacking candidates collected
	static const int MAX_VCT_BRANCH = 12;     // attacking candidates searched
	static const int MAX_VCT_DEFENCE = 24;    // defending replies to one threat
	static const int MAX_VCT_FIVE = 4;        // five points remembered per point
	static const int MAX_VCT_OPEN4 = 8;       // open four points per threat
	static const int VCT_ROOT_MAX_DEPTH = 9;  // attacking moves in one line

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
	long vctNodeBudget;    // nodes the current threat search may still spend
	long vctDeadline;      // wall clock the current threat search must stop at
	bool vctAborted;       // a budget ran out, so "no win" proves nothing
	Pos vctBestMove;       // winning attack found by the root threat search
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
		return MIN(info.time_left, MAX(0L, PROCESS_DEADLINE_MS - getTime()));
#endif
	}
	// Both budgets are absolute wall-clock targets measured from process start
	// (turnMove sets startTime to 0, so timeUsed() is time since process start).
	// timeLeft() must not be used here: it shrinks as the search runs, which
	// used to make every iteration compare elapsed time against a moving target
	// and stop the search at roughly a third of the available budget.
	inline long timeForTurn() {
		return MAX(0L, MIN(info.timeout_turn, PROCESS_DEADLINE_MS) - TIME_RESERVED);
	}
	inline long timeForTurnMax() {
		return MAX(0L, MIN(info.timeout_turn, PROCESS_DEADLINE_MS));
	}

	/////////////////////////////////////////////////////////////

	// ��������
	Pos fullSearch();
	Move alphabeta_root(int depth, int alpha, int beta);
	template <NodeType NT> int alphabeta(float depth, int alpha, int beta, bool cutNode);
	template <bool Root = true> int quickVCFSearch();
	int quickVCTSearch(int depthLeft, bool root);
	int genMoves_VCT(Move * out, int maxOut);
	int vctFivePoints(Piece piece, Pos * out, int maxOut);
	Pos contestVCTRoot(long budgetMs, long nodeBudget);

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
	Pos contestWinningFlex4(Piece self);

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
	UInt _key32 = 0;
	short _value = 0;
	Int8 _depth = 0;
	UInt8 _genFlag = HashFlag::Hash_Unknown;
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
		if (_key32 == newKey && depth < _depth) return;   // ���С�Ĳ��ܸ�����ȴ��

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
	// The judge starts a fresh process for every move.  A 256 MiB table spends
	// a large part of the one-second budget constructing entries that can never
	// be reused on the next turn; 32 MiB is ample for a sub-second search.
	static const int DEFAULT_HASH_SIZE = 20;
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
	UInt8 generation = 0; // Size must be not bigger than TTEntry::_genFlag

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
		zobrist[0][i] = rapfiRandom();
	for (int i = 0; i < MaxBoardSizeSqr; i++)
		zobrist[1][i] = rapfiRandom();
	zobristKey = rapfiRandom();
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
UInt8 Evaluator::PATTERN2[256][256];
PatternCode Evaluator::PCODE[65536];

Pattern4 Evaluator::PATTERN4[3876];
U64 Evaluator::PINFO[3876];
Evaluator::Cell Evaluator::undoCells[Evaluator::UNDO_LEVELS][Evaluator::UNDO_SLOTS];
Evaluator::Aggregates Evaluator::undoAggregates[Evaluator::UNDO_LEVELS];

void Evaluator::buildPatternInfo() {
	for (int i = 0; i < 3876; i++)
		PINFO[i] = U64((UShort)Score[i])
			| (U64((UShort)Value[i]) << 16)
			| (U64((UInt8)PATTERN4[i]) << 32);
}

Evaluator::Evaluator(Board * board) : board(board) {
	init();
	newGame();
}


// GCC will not inline this on its own -- it is over its size limit -- and an
// out-of-line call for each of the ~21 cells a move touches costs about 4%.
template <Evaluator::MoveType MT>
__attribute__((always_inline)) inline void Evaluator::updateCell(Pos p, int dir, UInt8 kBlack, UInt8 kWhite,
		int sh, UShort keep, Cell * slot) {
	Cell * c = &cell(p);
	*slot = *c;
	// Exactly one of kBlack/kWhite is the moved side's bit and the other is zero,
	// so this writes the same two bytes the indexed form did.  Doing it through
	// registers matters: the old `key[dir][self] |= k` was immediately followed by
	// reloading both key bytes to index PATTERN2, and that store-to-load pair sits
	// at the head of the dependency chain of every touched cell.
	UInt8 keyBlack = c->key[dir][Black];
	UInt8 keyWhite = c->key[dir][White];
	keyBlack |= kBlack; keyWhite |= kWhite;
	c->key[dir][Black] = keyBlack;
	c->key[dir][White] = keyWhite;

	c->updatePattern(keyBlack, keyWhite, sh, keep);

	const U64 infoBlack = PINFO[c->getPatternCode(Black)];
	const U64 infoWhite = PINFO[c->getPatternCode(White)];

	c->score[Black] = pinfoScore(infoBlack);
	c->score[White] = pinfoScore(infoWhite);

	if (MT == MoveType::Normal) {
		eval[Black] -= c->eval[Black]; eval[White] -= c->eval[White];
		c->eval[Black] = pinfoValue(infoBlack);
		c->eval[White] = pinfoValue(infoWhite);
		eval[Black] += c->eval[Black]; eval[White] += c->eval[White];
	}

	p4Count[Black][c->pattern4[Black]]--; p4Count[White][c->pattern4[White]]--;
	c->pattern4[Black] = pinfoP4(infoBlack);
	c->pattern4[White] = pinfoP4(infoWhite);
	c->overridable = Cell::isOverridable(c->pattern4[Black], c->pattern4[White]);
	if (c->pattern4[White] == A_FIVE && !contestMakesExactFive(*board, p, White))
		c->pattern4[White] = NONE;
	// Both bans need either two fours in different directions or a run of five
	// and over, and every one of those lands on B_FLEX4 or A_FIVE, so anything
	// weaker cannot be forbidden.  This scan is the most expensive work in the
	// incremental update and it used to run on every empty cell touched by every
	// move; gating it on the pattern raises search throughput about 4x.
	if (c->pattern4[Black] >= B_FLEX4 && contestForbiddenBlack(*board, p))
		c->pattern4[Black] = FORBID;
	p4Count[Black][c->pattern4[Black]]++; p4Count[White][c->pattern4[White]]++;
}

template <Evaluator::MoveType MT>
__attribute__((always_inline)) inline void Evaluator::updateLine(Pos pos, int dir, Piece self, Cell * slot) {
	const int sh = Cell::patShift(dir);
	const UShort keep = UShort(~(0xF << sh));
	const int d = D[dir];
	const UInt8 blackMask = self == Black ? UInt8(0xFF) : UInt8(0);
	Pos p = pos - d * 4;
	int i = 0;
	for (UInt8 k = 1; k < (1 << 4); k <<= 1, i++) {
		if (board->isEmpty(p))
			updateCell<MT>(p, dir, UInt8(k & blackMask), UInt8(k & ~blackMask), sh, keep, slot + i);
		p += d;
	}
	for (UInt8 k = 1 << 4; k != 0; k <<= 1, i++) {
		p += d;
		if (board->isEmpty(p))
			updateCell<MT>(p, dir, UInt8(k & blackMask), UInt8(k & ~blackMask), sh, keep, slot + i);
	}
}

template <Evaluator::MoveType MT>
__attribute__((always_inline)) inline void Evaluator::undoLine(Pos pos, int dir, const Cell * slot) {
	const int d = D[dir];
	Pos p = pos - d * 4;
	int i = 0;
	for (int j = 0; j < 4; j++, i++) {
		if (board->isEmpty(p)) cell(p) = slot[i];
		p += d;
	}
	for (int j = 0; j < 4; j++, i++) {
		p += d;
		if (board->isEmpty(p)) cell(p) = slot[i];
	}
}

// The original freestyle key stores four cells per side. Refresh the
// fifth-away endpoints explicitly so a newly-created six is not cached as
// an exact five.
void Evaluator::refreshPattern4(Pos p) {
	Cell * c = &cell(p);
	p4Count[Black][c->pattern4[Black]]--; p4Count[White][c->pattern4[White]]--;
	PatternCode pCodeBlack = c->getPatternCode(Black);
	PatternCode pCodeWhite = c->getPatternCode(White);
	c->updatePattern4(pCodeBlack, pCodeWhite);
	if (c->pattern4[White] == A_FIVE && !contestMakesExactFive(*board, p, White))
		c->pattern4[White] = NONE;
	if (c->pattern4[Black] >= B_FLEX4 && contestForbiddenBlack(*board, p))
		c->pattern4[Black] = FORBID;
	p4Count[Black][c->pattern4[Black]]++; p4Count[White][c->pattern4[White]]++;
}

template <Evaluator::MoveType MT>
void Evaluator::makeMove(Pos pos) {
	Piece self = SELF;
	Aggregates & agg = undoAggregates[undoLevel];
	agg.eval[0] = eval[0]; agg.eval[1] = eval[1];
	memcpy(agg.p4Count, p4Count, sizeof(p4Count));
	if (MT == MoveType::MuiltVC) {
		board->muiltMove(pos);
	} else {
		board->move(pos);
		ply++;
	}

	Cell * snap = undoCells[undoLevel++];
	for (int dir = 0; dir < 4; dir++)
		updateLine<MT>(pos, dir, self, snap + dir * UNDO_DIR_SLOTS);
	// The original freestyle key stores four cells per side. Refresh the
	// fifth-away endpoints explicitly so a newly-created six is not cached as
	// an exact five.  Nothing here saw a stone change, so the pattern code and
	// therefore the table's Pattern4 are the same as before; only a contest
	// override can differ, and only for the cells `overridable` marks.  Measured
	// over the search, 98.7% of the cells this loop visits are not marked.
	for (int i = 0; i < 4; ++i) for (int j = 0; j < 2; ++j) {
		int raw = int(pos) + (j ? 5 : -5) * D[i];
		if (raw < 0 || raw >= Board::MaxBoardSizeSqr) continue;
		Pos p = Pos(raw);
		if (!board->isInBoard(p) || !board->isEmpty(p)) continue;
		if (!cell(p).overridable) continue;
		snap[4 * UNDO_DIR_SLOTS + i * 2 + j] = cell(p);
		refreshPattern4(p);
	}

	Cell * c = &cell(pos);
	if (MT == MoveType::Normal) {
		eval[Black] -= c->eval[Black]; 
		eval[White] -= c->eval[White];
	}
	p4Count[Black][c->pattern4[Black]]--;
	p4Count[White][c->pattern4[White]]--;

	assert(checkP4Count());

#ifdef GENERATION_MIN
	for (int k = 0; k < 16; k++)
		cand(pos + RANGE_MIN[k])++;
#endif
#ifdef GENERATION_MIDDLE
	for (int k = 0; k < 24; k++)
		cand(pos + RANGE_MIDDLE[k])++;
#endif
#ifdef GENERATION_LARGE
    for (int k = 0; k < 32; k++)
	    cand(pos + RANGE_LARGE[k])++;
#endif
}

template void Evaluator::makeMove<Evaluator::Normal>(Pos pos);
template void Evaluator::makeMove<Evaluator::VC>(Pos pos);
template void Evaluator::makeMove<Evaluator::MuiltVC>(Pos pos);

template <Evaluator::MoveType MT>
void Evaluator::undoMove() {
	assert(board->getMoveCount() > 0);
	Pos pos = board->getLastMove();
	if (MT == MoveType::MuiltVC) {
		board->muiltUndo();
	} else {
		board->undo();
		ply--;
	}
	const Cell * snap = undoCells[--undoLevel];
	const Aggregates & agg = undoAggregates[undoLevel];
	eval[0] = agg.eval[0]; eval[1] = agg.eval[1];
	memcpy(p4Count, agg.p4Count, sizeof(p4Count));
	// `overridable` is derived from the pattern code, which the fifth-away
	// refresh does not change, and the direction walk cannot reach these cells
	// (1..4 steps versus 5), so this test sees exactly what makeMove saw.
	for (int i = 0; i < 4; ++i) for (int j = 0; j < 2; ++j) {
		int raw = int(pos) + (j ? 5 : -5) * D[i];
		if (raw < 0 || raw >= Board::MaxBoardSizeSqr) continue;
		Pos p = Pos(raw);
		if (!board->isInBoard(p) || !board->isEmpty(p)) continue;
		if (!cell(p).overridable) continue;
		cell(p) = snap[4 * UNDO_DIR_SLOTS + i * 2 + j];
	}
	for (int dir = 0; dir < 4; dir++)
		undoLine<MT>(pos, dir, snap + dir * UNDO_DIR_SLOTS);
	assert(checkP4Count());

#ifdef GENERATION_MIN
	for (int k = 0; k < 16; k++)
		cand(pos + RANGE_MIN[k])--;
#endif
#ifdef GENERATION_MIDDLE
	for (int k = 0; k < 24; k++)
		cand(pos + RANGE_MIDDLE[k])--;
#endif
#ifdef GENERATION_LARGE
	for (int k = 0; k < 32; k++)
		cand(pos + RANGE_LARGE[k])--;
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
	undoLevel = 0;
	eval[0] = eval[1] = 0;
	evalLower[0] = evalLower[1] = 0;
	memset(cells, 0, sizeof(cells));
	memset(cands, 0, sizeof(cands));
	memset(loses, 0, sizeof(loses));
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
			assert(c.pattern(Black, i) <= F1);
			assert(c.pattern(White, i) <= F1);
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

	// v[i] holds the sorted (canonical) form of index i, so the canonical codes
	// are exactly the fixed points v[i] == i: sorting the digits ascending into
	// the most significant positions yields the smallest index of its class.
	// The original code found them with an O(n^2) scan over 65536 entries, about
	// 254 million comparisons and ~140ms of every move's budget.  The judge
	// restarts the process for every move, so this identical table was being
	// rebuilt from scratch each time.  Numbering the fixed points in a single
	// pass produces a bit-identical table in about 6ms.
	for (int i = 0, count = 0; i < N * N * N * N; i++)
		v[i] = (v[i] == i) ? count++ : -1;

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
					PCODE[(i << 12) | (j << 8) | (m << 4) | n] = pcode;
					PATTERN4[pcode] = getPattern4(Pattern(a), Pattern(b), Pattern(c), Pattern(d));
				}

	// Built in place in two passes so no separate 64 KB PATTERN table has to
	// exist: first every entry holds its own colour's pattern in the low nibble,
	// then each mirrored pair exchanges nibbles.
	for (int key1 = 0; key1 < 256; key1++)
		for (int key2 = 0; key2 < 256; key2++)
			PATTERN2[key1][key2] = UInt8(getPattern(UInt8(key1), UInt8(key2)));
	for (int key1 = 0; key1 < 256; key1++)
		for (int key2 = key1; key2 < 256; key2++) {
			UInt8 a = UInt8(PATTERN2[key1][key2] & 0xF);
			UInt8 b = UInt8(PATTERN2[key2][key1] & 0xF);
			PATTERN2[key1][key2] = UInt8(a | (b << 4));
			PATTERN2[key2][key1] = UInt8(b | (a << 4));
		}

	buildPatternInfo();
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
		if (c.pattern(piece, dir) >= B4)
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
			if (cell(pos).pattern(piece, dir) == F5)
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
			if (cell(pos).pattern(piece, dir) == F5) {
				return pos;
			}
		}
		break;
	}
	MESSAGEL("ERROR!");
	// Diagnostics must never share the protocol stdout stream.
	trace(std::cerr, "MESSAGE ");
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
		if (c.pattern(piece, dir) == F4) {
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
				Pattern pattern = cell(pos).pattern(piece, dir);
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
				Pattern pattern = cell(pos).pattern(piece, dir);
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
			if (c.pattern(piece, dir) < B4) continue;

			Pos pos = posB;
			Piece p;
			for (int i = 1; i <= FindDistMax_B4; i++) {
				pos -= D[dir];
				p = board->get(pos);
				if (p == piece) continue;
				else if (p == Empty) {
					if (cell(pos).pattern(piece, dir) >= B4) {
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
					if (cell(pos).pattern(piece, dir) >= B4)
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
		if (c.pattern(piece, dir) == F4) {
			flex3 = true;

			Piece p;
			Pos pos = posB, posL = NullPos, posR = NullPos;
			int i, j;
			for (i = 1; i <= FindDistMax_F4; i++) {
				pos -= D[dir];
				p = board->get(pos);
				if (p == piece) continue;
				else if (p == Empty) {
					Pattern pattern = cell(pos).pattern(piece, dir);
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
					Pattern pattern = cell(pos).pattern(piece, dir);
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
			if (c.pattern(piece, dir) < B4) continue;

			Pos pos = posB;
			Piece p;
			for (int i = 1; i <= FindDistMax_B4; i++) {
				pos -= D[dir];
				p = board->get(pos);
				if (p == piece) continue;
				else if (p == Empty) {
					if (cell(pos).pattern(piece, dir) >= B4) {
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
					if (cell(pos).pattern(piece, dir) >= B4)
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
			if (board->isEmpty(p) && cand(p) == 0) {
				cand(p)++;
			}
		}
	}
	p = POS(x, y);
	for (int i = MAX(3, fillDist + 1); i <= lineDist; i++) {
		for (int k = 0; k < 8; k++) {
			// The array is padded by four, so a step of five off a board edge
			// leaves it: from the top row the index goes negative and the
			// process dies on the spot.  An opponent opening anywhere on row 0
			// was a segfault and an instant loss, and any other edge opening
			// silently raised the candidate count of an unrelated cell.
			int target = int(p) + int(RANGE_NEIGHBOR[k]) * i;
			if (target < 0 || target >= Board::MaxBoardSizeSqr) continue;
			if (!board->isInBoard(Pos(target))) continue;
			cand(Pos(target))++;
		}
	}
}

void Evaluator::clearLose() {
	FOR_EVERY_POSITION_POS(p) {
		isLose(p) = false;
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
					s += 2 * (len1 + dis(rapfiRandom));
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
			if (isLose(POS(x, y))) {
				ss << "L";
			} else {
				if (isCand(POS(x, y))) ss << '*';
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
			if (isCand(POS(x, y))) ss << std::setw(5) << cell(x, y).getScore();
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

// Returns the book move for this position, or NullPos when the position is not
// in the book.  Costs eight 225-byte board rewrites and a binary search, which
// is far below a millisecond.
static Pos contestBookMove(const Board & board) {
	if (CONTEST_BOOK_SIZE == 0 || CONTEST_BOOK[0].key == 0ull) return NullPos;
	if (board.size() != 15) return NullPos;

	char plane[225];
	for (int r = 0; r < 15; ++r)
		for (int c = 0; c < 15; ++c) {
			Piece piece = board.get(POS(r, c));
			plane[r * 15 + c] = piece == Black ? 'b' : (piece == White ? 'w' : '.');
		}

	char canonicalPlane[225], candidate[225];
	int canonicalIndex = -1;
	for (int t = 0; t < 8; ++t) {
		for (int r = 0; r < 15; ++r)
			for (int c = 0; c < 15; ++c) {
				int nr, nc;
				contestTransform(t, r, c, nr, nc);
				candidate[nr * 15 + nc] = plane[r * 15 + c];
			}
		if (canonicalIndex < 0 || std::memcmp(candidate, canonicalPlane, 225) < 0) {
			std::memcpy(canonicalPlane, candidate, 225);
			canonicalIndex = t;
		}
	}

	unsigned long long key = contestBookHash(canonicalPlane, 225);
	int low = 0, high = CONTEST_BOOK_SIZE - 1, found = -1;
	while (low <= high) {
		int mid = (low + high) / 2;
		if (CONTEST_BOOK[mid].key == key) { found = mid; break; }
		if (CONTEST_BOOK[mid].key < key) low = mid + 1;
		else high = mid - 1;
	}
	if (found < 0) return NullPos;

	int row, col;
	contestTransform(CONTEST_INVERSE[canonicalIndex],
		CONTEST_BOOK[found].move / 15, CONTEST_BOOK[found].move % 15, row, col);
	Pos move = POS(row, col);
	return board.isEmpty(move) ? move : NullPos;
}

// The incremental update already stamps FORBID onto every forbidden black point
// and keeps it current, so inside move generation the ban is a field read rather
// than a rescan of the four directions.  turnMove and the root keep calling
// contestLegalMove directly: that happens once per move and doubles as a check
// on the cache before the answer leaves the process.
#define CONTEST_LEGAL_CACHED(p, side) ((side) != Black || cell(p).pattern4[Black] != FORBID)

Pos AI::turnMove() {
	startTime = 0;
	terminateAI = false;

	// Our own generated book comes first: its entries are the moves a search
	// with seconds of thinking time played, which is depth this process cannot
	// reach inside the contest's one second.  It has to be consulted before the
	// early returns below, or the second move never reaches it: with one stone
	// on the board Rapfi answers with a random neighbour and leaves.
	Pos bookMove = contestBookMove(*board);
	// GOMOKU_EXCLUDE has to reach the book as well.  Without this the analysis
	// tools ask for the second choice in a position, get the book's first choice
	// back, and silently study the line they were trying to leave.
	if (bookMove != NullPos && contestLegalMove(*board, bookMove, SELF)
		&& std::find(contestRootExclude.begin(), contestRootExclude.end(),
			int(CoordX(bookMove)) * 15 + int(CoordY(bookMove))) == contestRootExclude.end())
		return bookMove;

	if (board->getMoveCount() == 0)
		return POS(board->centerPos(), board->centerPos());
	else if (board->getMoveCount() == 1) {
		Pos p = board->getLastMove();
		if (!board->isNearBoard(p, 5)) {
			// Rapfi answered a lone central stone with a random neighbour, on the
			// freestyle reasoning that they are all much the same.  Under this
			// contest's bans they are not, and the shortcut also returned before
			// the book and before GOMOKU_EXCLUDE, so the second move could neither
			// be looked up nor enumerated.  Search it like any other position.
			expendCand(p, 3, 5);
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
		// Rapfi's own move database is a second book and needs the same
		// exclusion, or the analysis tools cannot get past it either.
		if (board->isEmpty(best)
			&& std::find(contestRootExclude.begin(), contestRootExclude.end(),
				int(CoordX(best)) * 15 + int(CoordY(best))) == contestRootExclude.end())
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

	// Cheap legal move first, so the watchdog always has something sane to print
	// even if the search is cut off before it finishes its first iteration.
	Pos legal = NullPos;
	{
		int score = INT_MIN;
		FOR_EVERY_EMPTY_POS(p) {
			if (!contestLegalMove(*board, p, SELF)) continue;
			int s = cell(p).getScore();
			if (legal == NullPos || s > score) { legal = p; score = s; }
		}
		if (legal != NullPos) contestPublish(CoordX(legal), CoordY(legal));
	}

	// Look for a victory by continuous threats before the main search starts.
	// A forced win five or six moves out is beyond the depth one second buys,
	// but it is shallow for a threat search, and finding one ends the move.
	{
		long spend = MIN(VCT_ROOT_BUDGET_MS, MAX(0L, timeForTurn() - timeUsed()) / 4);
		Pos vct = contestVCTRoot(spend, VCT_ROOT_NODE_BUDGET);
		if (vct != NullPos && board->isEmpty(vct) && contestLegalMove(*board, vct, SELF)) {
			contestPublish(CoordX(vct), CoordY(vct));
			// Keep the stats line in the shape the analysis tools expect, and name
			// the threat search so a claimed mate can be traced back to it.
			if (contestStats)
				std::fprintf(stderr, "depth=0 value=%d node=%d expanded=%d ms=%ld vct=%d,%d\n",
					WIN_MAX, node, nodeExpended, timeUsed(),
					int(CoordX(vct)), int(CoordY(vct)));
			return vct;
		}
	}

	best = fullSearch();
	if (!board->isEmpty(best) || !contestLegalMove(*board, best, SELF))
		best = legal;

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
	int depthLimit = contestMaxDepth > 0 ? MIN(maxSearchDepth, contestMaxDepth) : maxSearchDepth;
	for (searchDepth = 2; searchDepth <= depthLimit; searchDepth++) {
		lastDepthTime = getTime();
		lastNode = node;
		lastNodeExpended = nodeExpended;
		BestMoveChangeCount = 0;

		move = alphabeta_root(searchDepth, -INF, INF);
		if (!board->isEmpty(move.pos)) break;
		else if (move.value == -INF)  // ���ʱ��֪����һ����������ֵ������һ���Ĵ���
			move.value = bestMove.value; 
		bestMove = move;
		if (board->isEmpty(bestMove.pos) && contestLegalMove(*board, bestMove.pos, SELF))
			contestPublish(CoordX(bestMove.pos), CoordY(bestMove.pos));

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
		// Rapfi's original condition compared timeLeft() against the whole-match
		// reserve, which in a per-move process is always true, so it collapsed
		// into "stop as soon as one iteration costs more than about 170ms" and
		// returned with most of the budget unspent.  Predicting the next
		// iteration's cost uses the budget instead of abandoning it.
		long lastIterationTime = getTime() - lastDepthTime;
		shouldBreak = timeUsed() + lastIterationTime * NEXT_ITERATION_COST_PERCENTAGE / 100 > turnTime
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

	if (contestStats)
		std::fprintf(stderr, "depth=%d value=%d node=%d expanded=%d ms=%ld\n",
			searchDepth - 1, bestMove.value, node, nodeExpended, timeUsed());

	bool hasLose = false;
	FOR_EVERY_POSITION_POS(p) {
		if (isLose(p)) {
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
		// evaluate() subtracts rawStaticEval[ply - 1], which at the root would be
		// rawStaticEval[-1]: an out-of-bounds read that made the reported value
		// jump around between runs of the same position.  A forced root move has
		// no previous ply to compare against, and this value is only reported --
		// terminateAI above ends the iteration, so it is never searched or stored.
		best.value = hashTable->probe(board->getZobristKey(), tte) ? tte->value(ply) : 0;
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
		if (!contestRootExclude.empty()
			&& std::find(contestRootExclude.begin(), contestRootExclude.end(),
				int(CoordX(move->pos)) * 15 + int(CoordY(move->pos))) != contestRootExclude.end())
			continue;
		if (isLose(move->pos)) {
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
			isLose(move->pos) = true;
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
			if (ply == 0 && board->isEmpty(best.pos) && contestLegalMove(*board, best.pos, SELF))
				contestPublish(CoordX(best.pos), CoordY(best.pos));
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
		cnt = 1000;
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
		cnt = 2000;
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
		// Same ban, inside the four sequence: if black cannot legally cover the
		// five point the sequence has already won and must not be searched on.
		if (oppo == Black && cell(defMove).pattern4[Black] == FORBID) {
			undoMove<VC>();
			best.value = WIN_MAX - ply - 1;
			best.pos = attMove;
			break;
		}
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

///////////////////////////////////////////////////////////////////////////////
// VCT: victory by continuous threats.
//
// A VCF wins with fours alone.  A VCT lets the attacker play open threes as
// well, so the defender still has to answer every single move and the line
// stays forced.  That is the one classical device this engine was missing, and
// at the six or seven plies one second buys it reaches wins alphabeta cannot.
//
// Nothing below asks the pattern tables what a threat is.  Those tables come
// from freestyle, where five *or more* wins, so a gap whose fill would make six
// still reads as a four in them; here an overline wins for nobody and such a
// "four" threatens nothing.  Every four and every open four is instead verified
// by playing the point and counting the five squares that survive the rules.
// p4Count[side][A_FIVE] is already rule-corrected -- a forbidden black five is
// marked FORBID and a white five that would be an overline is marked NONE -- so
// counting A_FIVE cells after a move is exact.
//
// Why the defending reply sets are complete, which is the whole question of
// whether a reported mate is real:
//
//   * After a verified four with one five point, the only answers are that
//     point and a five of the defender's own.  The defender cannot have one: a
//     five for them was read before the attacker moved, and a stone of the
//     attacker's colour never creates a five for the defender.
//   * After a verified open three the defender must stop every open four or
//     play a four of their own.  A defending stone never changes how many of
//     the attacker's stones stand in a row, so once it is down the five points
//     of an open four q are exactly the old set minus the square it took: q
//     survives unless the defender plays q itself or leaves one point standing.
//     That makes "does this stop q" an exact test rather than a table lookup,
//     and since a white stone can only relax black's bans, an attacking point
//     that was legal stays legal.
//   * Any other defending move loses to the very continuation this search has
//     already verified, so leaving it out cannot invent a mate.
//
// A budget that runs out sets vctAborted, and an aborted search only ever
// returns 0: a win is reported only when every defence in the line was searched.
///////////////////////////////////////////////////////////////////////////////

// Squares where `piece` completes an exact five, at most maxOut of them.
int AI::vctFivePoints(Piece piece, Pos * out, int maxOut) {
	if (p4Count[piece][A_FIVE] == 0) return 0;
	int n = 0;
	FOR_EVERY_CAND_POS(p) {
		if (cell(p).pattern4[piece] == A_FIVE) {
			out[n] = p;
			if (++n >= maxOut) break;
		}
	}
	return n;
}

// Fours and open threes for the side to move, best first.  Anything weaker does
// not force an answer and has no place in a threat sequence.  Whether these are
// really fours and threes under the contest rules is settled later, by playing
// them; this only has to be a superset, and a table that reads five out of six
// can only ever be generous.
int AI::genMoves_VCT(Move * out, int maxOut) {
	const Piece self = SELF;
	int n = 0;
	FOR_EVERY_CAND_POS(p) {
		if (n >= maxOut) break;
		if (!CONTEST_LEGAL_CACHED(p, self)) continue;
		Pattern4 p4 = cell(p).pattern4[self];
		if (p4 < H_FLEX3) continue;
		out[n++] = Move(p, int(p4) * 4096 + MIN(cell(p).getScore_VC(self), 4095));
	}
	std::sort(out, out + n, std::greater<Move>());
	return n;
}

// The attacker is the side to move.  Returns a mate score for the attacker, or
// 0 when no forced win was proved.
int AI::quickVCTSearch(int depthLeft, bool root) {
	const Piece att = SELF, def = OPPO;

	node++;
	if (--vctNodeBudget < 0) { vctAborted = true; return 0; }
	if ((vctNodeBudget & 511) == 0 && (terminateAI || timeUsed() > vctDeadline)) {
		vctAborted = true;
		return 0;
	}

	if (p4Count[att][A_FIVE] >= 1) return WIN_MAX - ply;
	if (p4Count[def][A_FIVE] >= 2) return 0;   // the attacker is the one who is lost

	Move cands[MAX_VCT_MOVES];
	int candCount;

	if (p4Count[def][A_FIVE] == 1) {
		// One square stops the defender's five, so it is the attacker's only move,
		// and the attack survives only if that square is itself a threat.  A square
		// black is banned from is not a move at all: the line is lost, not won.
		Pos block = findPosByPattern4(def, A_FIVE);
		if (!CONTEST_LEGAL_CACHED(block, att)) return 0;
		cands[0] = Move(block, 0);
		candCount = 1;
	} else {
		// A verified open four wins outright: two five points cannot both be covered.
		if (p4Count[att][B_FLEX4] >= 1) {
			Pos flex4Win = contestWinningFlex4(att);
			if (flex4Win != NullPos) {
				if (root) vctBestMove = flex4Win;
				return WIN_MAX - ply - 2;
			}
		}
		if (depthLeft <= 0) return 0;
		candCount = genMoves_VCT(cands, MAX_VCT_MOVES);
		if (candCount > MAX_VCT_BRANCH) candCount = MAX_VCT_BRANCH;
	}
	if (candCount == 0) return 0;
	nodeExpended++;

	Pos five[MAX_VCT_FIVE];
	Pos flex4[MAX_VCT_OPEN4];
	Pos openFour[MAX_VCT_OPEN4];
	Pos ofFive[MAX_VCT_OPEN4][MAX_VCT_FIVE];
	int ofFiveN[MAX_VCT_OPEN4];
	Pos defence[MAX_VCT_DEFENCE];

	int best = 0;
	for (int i = 0; i < candCount; i++) {
		const Pos move = cands[i].pos;
		int ofCount = 0;

		// What does the move actually threaten?  MuiltVC puts down a stone of the
		// attacker's colour without handing over the turn, so the follow-up point
		// can be tried in the same colour before the move is really made.
		makeMove<MuiltVC>(move);
		int fiveN = vctFivePoints(att, five, MAX_VCT_FIVE);
		if (fiveN == 0 && p4Count[att][B_FLEX4] > 0) {
			int flex4N = 0;
			FOR_EVERY_CAND_POS(q) {
				if (cell(q).pattern4[att] != B_FLEX4) continue;
				flex4[flex4N] = q;
				if (++flex4N >= MAX_VCT_OPEN4) break;
			}
			for (int k = 0; k < flex4N; k++) {
				makeMove<MuiltVC>(flex4[k]);
				int m = vctFivePoints(att, ofFive[ofCount], MAX_VCT_FIVE);
				undoMove<MuiltVC>();
				if (m >= 2) {
					openFour[ofCount] = flex4[k];
					ofFiveN[ofCount] = m;
					ofCount++;
				}
			}
		}
		undoMove<MuiltVC>();

		if (fiveN == 0 && ofCount == 0) continue;   // neither a four nor an open three

		int value = 0;
		makeMove<VC>(move);

		if (fiveN >= 2) {
			value = WIN_MAX - ply - 1;      // two five points, one move cannot cover both
		} else {
			int defCount = 0;
			bool overflow = false;

			if (fiveN == 1) {
				if (CONTEST_LEGAL_CACHED(five[0], def)) defence[defCount++] = five[0];
			} else {
				// Everything that stops the first open four is either that point or,
				// when it leaves exactly two five points, one of those two.  A move
				// that stops every open four has to be somewhere in that short list.
				Pos probe[1 + MAX_VCT_FIVE];
				int probeN = 0;
				probe[probeN++] = openFour[0];
				if (ofFiveN[0] == 2) {
					probe[probeN++] = ofFive[0][0];
					probe[probeN++] = ofFive[0][1];
				}
				for (int k = 0; k < probeN; k++) {
					Pos d = probe[k];
					if (!board->isEmpty(d) || !CONTEST_LEGAL_CACHED(d, def)) continue;
					bool stopsAll = true;
					for (int j = 0; j < ofCount && stopsAll; j++) {
						if (d == openFour[j]) continue;
						int left = 0;
						for (int m = 0; m < ofFiveN[j]; m++)
							if (ofFive[j][m] != d) left++;
						if (left >= 2) stopsAll = false;
					}
					if (stopsAll) defence[defCount++] = d;
				}
				// A four of the defender's own forces the attacker to answer and can
				// buy exactly the tempo the defence needs, so it has to be searched
				// too.  Only a four that really leaves an exact five square counts.
				FOR_EVERY_CAND_POS(d) {
					if (cell(d).pattern4[def] < E_BLOCK4) continue;
					if (!CONTEST_LEGAL_CACHED(d, def)) continue;
					bool seen = false;
					for (int k = 0; k < defCount; k++) seen = seen || defence[k] == d;
					if (seen) continue;
					makeMove<MuiltVC>(d);
					bool real = p4Count[def][A_FIVE] > 0;
					undoMove<MuiltVC>();
					if (!real) continue;
					if (defCount >= MAX_VCT_DEFENCE) { overflow = true; break; }
					defence[defCount++] = d;
				}
			}

			if (overflow) {
				value = 0;                      // too many answers to prove anything
			} else if (defCount == 0) {
				value = WIN_MAX - ply - 1;      // no legal answer exists
			} else {
				value = WIN_MAX;
				for (int k = 0; k < defCount; k++) {
					makeMove<VC>(defence[k]);
					int reply = quickVCTSearch(depthLeft - 1, false);
					undoMove<VC>();
					if (reply < value) value = reply;
					if (value == 0) break;
				}
			}
		}

		undoMove<VC>();

		if (value > best) {
			best = value;
			if (root) vctBestMove = move;
		}
		if (best >= WIN_MIN || vctAborted) break;
	}
	return best;
}

// Root threat search.  Iterative deepening keeps a shallow forced win cheap and
// gives the budget a natural place to stop; the node and time caps are shared
// across all iterations, so a search that finds nothing costs a bounded slice
// of the move.  Returns the winning attack, or NullPos.
Pos AI::contestVCTRoot(long budgetMs, long nodeBudget) {
	const Piece self = SELF;
	// Without a four or an open three on the board there is no threat to start
	// from, and that is the common case, so the early exit is worth its line.
	if (p4Count[self][A_FIVE] == 0 && p4Count[self][B_FLEX4] == 0
		&& p4Count[self][C_BLOCK4_FLEX3] == 0 && p4Count[self][D_BLOCK4_PLUS] == 0
		&& p4Count[self][E_BLOCK4] == 0 && p4Count[self][F_FLEX3_2X] == 0
		&& p4Count[self][G_FLEX3_PLUS] == 0 && p4Count[self][H_FLEX3] == 0)
		return NullPos;

	vctDeadline = MIN(timeUsed() + budgetMs, timeForTurnMax());
	vctNodeBudget = nodeBudget;
	vctAborted = false;

	for (int depth = 2; depth <= VCT_ROOT_MAX_DEPTH; depth++) {
		vctBestMove = NullPos;
		int value = quickVCTSearch(depth, true);
		if (value >= WIN_MIN && vctBestMove != NullPos && board->isEmpty(vctBestMove))
			return vctBestMove;
		if (vctAborted || terminateAI || timeUsed() > vctDeadline) break;
	}
	return NullPos;
}

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
		bool lostToBan = false;
		if (p4Count[self][A_FIVE] > 0) {
			moveList.addMove(findPosByPattern4(self, A_FIVE), WIN_MAX);
			return State_Win;
		} else if (p4Count[oppo][A_FIVE] >= 2) {
			moveList.addMove(findPosByPattern4(oppo, A_FIVE), -WIN_MAX + 1);
			return State_Lose;
		} else if (p4Count[oppo][A_FIVE] == 1) {
			Pos block = findPosByPattern4(oppo, A_FIVE);
			if (CONTEST_LEGAL_CACHED(block, self)) {
				moveList.addMove(block, 0);
				return State_Unknown;
			}
			// The one square that stops the five is banned for us.  The game is
			// lost, but the answer still has to be a move we are allowed to play.
			lostToBan = true;
			genMoves(moveList);
			if (moveList.moveCount() == 0) moveList.addMove(block, 0);
		} else {
			Pos flex4 = p4Count[self][B_FLEX4] > 0 ? contestWinningFlex4(self) : NullPos;
			if (flex4 != NullPos) {
				moveList.addMove(flex4, WIN_MAX - 2);
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
		return lostToBan ? State_Lose : State_Unknown;
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
		if (!CONTEST_LEGAL_CACHED(p, self)) continue;
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
		if (!CONTEST_LEGAL_CACHED(p, self)) continue;
		if (cell(p).pattern4[oppo] == B_FLEX4) {
			getAllCostPosAgainstF3(p, oppo, defence);
		} else if (cell(p).pattern4[self] >= E_BLOCK4) {
			moveList.addMove(p, cell(p).getScore(self));
		}
	}

	set<Pos>::iterator it2 = defence.end();
	for (set<Pos>::iterator it1 = defence.begin(); it1 != it2; it1++) {
		if (!CONTEST_LEGAL_CACHED(*it1, self)) continue;
		moveList.addMove(*it1, cell(*it1).getScore(oppo) + 10000);
	}
	assert(moveList.moveCount() > 0);
}
// ֻ��������/�����ڵĳ����ŷ�
void AI::genMoves_VCF(MoveList & moveList) {
	Piece self = SELF;

	FOR_EVERY_CAND_POS(p) {
		if (CONTEST_LEGAL_CACHED(p, self) && cell(p).pattern4[self] >= E_BLOCK4) {
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

		if (board->isEmpty(p) && CONTEST_LEGAL_CACHED(p, self)) {
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

// Rapfi's tables come from freestyle, where five or more wins, so a gap whose
// fill would make six still counts as a four.  Here only an exact five wins and
// an overline is nothing, so two of those "fours" are not the double four that
// B_FLEX4 claims.  Believing it made the engine announce a forced win three
// moves running in a game it then lost.  Play the move and count the five points
// that survive the rules: an open four leaves two, and two is what cannot be
// covered.  Returns the winning point, or NullPos when the claim does not hold.
Pos AI::contestWinningFlex4(Piece self) {
	FOR_EVERY_CAND_POS(p) {
		if (cell(p).pattern4[self] != B_FLEX4) continue;
		makeMove<VC>(p);
		bool wins = p4Count[self][A_FIVE] >= 2;
		undoMove<VC>();
		if (wins) return p;
	}
	return NullPos;
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
	if (p4Count[oppo][A_FIVE] == 1) {
		// A four leaves exactly one square that stops the five, and for black that
		// square can itself be a four-four or an overline.  A block black is not
		// allowed to play is a won four for white, so the search may steer into it
		// rather than assume every four can be answered.  The scan only runs when
		// black is to move and some ban exists, which keeps it off the hot path.
		if (self == Black && p4Count[Black][FORBID] > 0
			&& cell(findPosByPattern4(oppo, A_FIVE)).pattern4[Black] == FORBID)
			return -WIN_MAX + ply + 1;
		return 0;
	}
	if (p4Count[self][B_FLEX4] >= 1 && contestWinningFlex4(self) != NullPos)
		return WIN_MAX - ply - 2;

	int self_C_count = p4Count[self][C_BLOCK4_FLEX3];
	if (self_C_count >= 1) {
		// The four-three: a move that is a four and an open three at once.  The
		// four forces a block, the block cannot also stop the three, and the three
		// becomes an open four.  All three of those steps are freestyle reasoning.
		// Here a four whose gap makes six is not a four, an "open three" whose
		// extensions make six is not a three, and the square that blocks the four
		// can lie on the three's own line and kill it.  So play the point, play
		// the block, and require a real open four to be standing afterwards --
		// the same verify-by-playing that B_FLEX4 and the four itself already use.
		FOR_EVERY_CAND_POS(p) {
			if (cell(p).pattern4[self] != C_BLOCK4_FLEX3) continue;
			makeMove<VC>(p);
			int fives = p4Count[self][A_FIVE];
			int value = 0;
			if (fives >= 2) {
				value = WIN_MAX - ply - 1;      // two five points, nothing covers both
			} else if (fives == 1) {
				Pos block = findPosByPattern4(self, A_FIVE);
				if (!CONTEST_LEGAL_CACHED(block, oppo)) {
					value = WIN_MAX - ply - 1;  // black is banned from the only block
				} else {
					makeMove<VC>(block);
					// Our move again.  The block is forced -- they had no five of
					// their own before we moved and our stone cannot make them one
					// -- so a verified open four now, with no four of theirs to
					// answer first, is a win.
					if (p4Count[oppo][A_FIVE] == 0 && p4Count[self][B_FLEX4] > 0
						&& contestWinningFlex4(self) != NullPos)
						value = WIN_MAX - ply - 2;
					undoMove<VC>();
				}
			}
			undoMove<VC>();
			if (value != 0) return value;
			if (--self_C_count <= 0) break;
		}
	}

	// Rapfi's "double open three with no enemy four wins" shortcut is a freestyle
	// rule of thumb.  Under exactly-five scoring an "open three" whose extensions
	// would make an overline is not a real threat, so the shortcut announces mates
	// that do not exist (measured: 21% of all mate claims in games we lost).  The
	// main search finds the genuine ones anyway, so drop the shortcut entirely.

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
					Pattern pattern = c.pattern(self, dir);
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
			buildPatternInfo();
			override++;
		} else if (strncmp(line, "Score:", 6) == 0) {
			for (int i = 0; i < 3876; i++)
				file >> Score[i];
			buildPatternInfo();
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
    contestReadEnv();
    int me, x;
    if (!(std::cin >> me)) return 0;
    std::vector<Pos> stones[2];
    int grid[15][15];
    for (int r = 0; r < 15; ++r)
        for (int c = 0; c < 15; ++c) {
            std::cin >> x;
            grid[r][c] = x;
            if (x == 0 || x == 1) stones[x].push_back(POS(r, c));
        }

    // Arm the backstop before Rapfi's per-process initialization, using the
    // closest empty point to the centre as the answer of last resort.  Legality
    // is refined as soon as the evaluator is up; this only has to beat a TLE.
    for (int radius = 0; radius < 15 && contestBestRow < 0; ++radius)
        for (int r = 7 - radius; r <= 7 + radius && contestBestRow < 0; ++r)
            for (int c = 7 - radius; c <= 7 + radius && contestBestRow < 0; ++c)
                if (r >= 0 && r < 15 && c >= 0 && c < 15 && grid[r][c] == -1)
                    contestPublish(r, c);
    contestArmWatchdog();

    Board board(15);
    AI ai(&board);
    size_t rounds = std::max(stones[0].size(), stones[1].size());
    for (size_t i = 0; i < rounds; ++i) {
        if (i < stones[0].size()) ai.makeMove(stones[0][i]);
        if (i < stones[1].size()) ai.makeMove(stones[1][i]);
    }
    if (int(board.getPlayerToMove()) != me) return 2;

    // Search timing starts after Rapfi's per-process initialization and board
    // reconstruction.  Leave enough of the judge's 1 s budget for that work.
    ai.info.timeout_turn = PROCESS_DEADLINE_MS;
    ai.info.time_left = 100000000;
    ai.info.setMaxMemory(256 * 1024 * 1024L);
    Pos p = ai.turnMove();
    contestDisarmWatchdog();
    std::cout << int(CoordX(p)) << ' ' << int(CoordY(p)) << '\n';
    return 0;
}

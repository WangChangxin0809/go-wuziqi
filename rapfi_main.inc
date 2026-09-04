
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

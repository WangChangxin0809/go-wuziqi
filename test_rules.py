import unittest

from arena_server import black_forbidden, winner


def empty_board():
    return [[-1] * 15 for _ in range(15)]


class WebsiteRuleTests(unittest.TestCase):
    def test_exact_five_wins(self):
        board = empty_board()
        for c in range(3, 8):
            board[7][c] = 0
        self.assertEqual(winner(board), 0)

    def test_overline_does_not_win(self):
        board = empty_board()
        for c in range(3, 9):
            board[7][c] = 1
        self.assertEqual(winner(board), -1)

    def test_cross_is_four_four(self):
        board = empty_board()
        for r, c in ((7, 5), (7, 6), (7, 8), (5, 7), (6, 7), (8, 7)):
            board[r][c] = 0
        board[7][7] = 0
        self.assertEqual(black_forbidden(board, 7, 7), "FourFourBan")

    def test_six_is_long_ban(self):
        board = empty_board()
        for c in range(3, 9):
            board[7][c] = 0
        self.assertEqual(black_forbidden(board, 7, 6), "LongBan")


if __name__ == "__main__":
    unittest.main()

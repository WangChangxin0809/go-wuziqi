import os
import subprocess
import unittest
from pathlib import Path

from arena_server import black_forbidden, winner
from gomoku_match import compile_source

ROOT = Path(__file__).resolve().parent


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

    def test_exact_five_does_not_beat_four_four(self):
        board = empty_board()
        for c in range(3, 7):
            board[7][c] = 0
        for r in range(4, 7):
            board[r][7] = 0
        for k in range(4, 7):
            board[k][k] = 0
        board[7][7] = 0
        # five horizontally, plus a four vertically and a four diagonally
        self.assertEqual(winner(board), 0)
        self.assertEqual(black_forbidden(board, 7, 7), "FourFourBan")

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


class EngineRuleTests(unittest.TestCase):
    """The engine must never answer with a point the website calls forbidden.

    A move that completes an exact five while also forming two fours is a loss
    for black, not a win: the site checks the four-four ban before it checks for
    five.  The engine used to treat the five as taking priority and would walk
    straight into that point when it was the only way to make five.
    """

    @classmethod
    def setUpClass(cls):
        cls.binary = compile_source(ROOT / "src.cpp")

    def ask(self, board, side=0, budget_ms=200):
        env = dict(os.environ, GOMOKU_DEADLINE_MS=str(budget_ms))
        text = f"{side}\n" + "\n".join(" ".join(str(v) for v in row) for row in board) + "\n"
        out = subprocess.run([str(self.binary)], input=text, env=env,
                             capture_output=True, text=True, timeout=30).stdout.split()
        return int(out[0]), int(out[1])

    def read_fixture(self, name):
        values = (ROOT / "fixtures" / name).read_text().split()
        side = int(values[0])
        rest = [int(v) for v in values[1:226]]
        return side, [rest[r * 15:(r + 1) * 15] for r in range(15)]

    def test_never_plays_a_forbidden_five(self):
        side, board = self.read_fixture("black-five-is-four-four.txt")
        r, c = self.ask(board, side)
        self.assertEqual(board[r][c], -1, "engine played an occupied point")
        board[r][c] = 0
        self.assertIsNone(black_forbidden(board, r, c),
                          f"engine played the forbidden point ({r}, {c})")

    def test_does_not_treat_an_overline_as_a_win(self):
        side, board = self.read_fixture("white-overline-not-a-win.txt")
        r, c = self.ask(board, side)
        board[r][c] = side
        # the only run this point could complete is six long, which wins for nobody
        self.assertNotEqual((r, c), (7, 7))
        self.assertEqual(winner(board), -1)

    def test_still_takes_a_legal_win(self):
        side, board = self.read_fixture("white-win-11-6.txt")
        self.assertEqual(self.ask(board, side), (11, 6))


if __name__ == "__main__":
    unittest.main()

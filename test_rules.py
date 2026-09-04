import os
import re
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

    def ask(self, board, side=0, budget_ms=200, want_value=False):
        env = dict(os.environ, GOMOKU_DEADLINE_MS=str(budget_ms), GOMOKU_STATS="1")
        text = f"{side}\n" + "\n".join(" ".join(str(v) for v in row) for row in board) + "\n"
        proc = subprocess.run([str(self.binary)], input=text, env=env,
                              capture_output=True, text=True, timeout=30)
        out = proc.stdout.split()
        move = int(out[0]), int(out[1])
        if not want_value:
            return move
        found = re.findall(r"value=(-?\d+) ", proc.stderr)
        return move, (int(found[-1]) if found else None)

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

    def test_answers_a_first_move_on_the_border(self):
        """The board array is padded by four and the candidate expansion stepped
        five, so an opening on row 0 sent the index negative and the process
        died: an opponent playing the corner beat us in 22 ms.  Edge openings
        elsewhere did not crash but bumped the candidate count of an unrelated
        cell.
        """
        for r, c in ((0, 0), (0, 7), (0, 14), (14, 0), (14, 14), (7, 0), (0, 1)):
            board = [[-1] * 15 for _ in range(15)]
            board[r][c] = 0
            reply = self.ask(board, side=1, budget_ms=120)
            self.assertEqual(board[reply[0]][reply[1]], -1,
                             f"after black ({r}, {c}) the engine answered {reply}")

    def test_does_not_claim_a_win_from_a_fake_four(self):
        """A gap whose fill would make six is not a four under these rules.

        Rapfi's freestyle tables counted two of them as a double four and the
        root reported a forced win, so the engine stopped searching after 26 ms
        and answered with a move that only ever made one five point.  It said
        that three moves running in a game it went on to lose.
        """
        side, board = self.read_fixture("black-fake-double-four.txt")
        (r, c), value = self.ask(board, side, budget_ms=900, want_value=True)
        self.assertIsNotNone(value, "engine answered from the book, not the search")
        self.assertLess(value, 29000, "engine claimed a forced win it does not have")
        board[r][c] = side
        five_points = [(rr, cc) for rr in range(15) for cc in range(15)
                       if board[rr][cc] == -1 and self.makes_five(board, rr, cc, side)]
        self.assertLess(len(five_points), 2,
                        f"the move really was unanswerable: {five_points}")

    def makes_five(self, board, r, c, side):
        board[r][c] = side
        won = winner(board) == side
        board[r][c] = -1
        return won

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

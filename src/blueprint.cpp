#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cassert>

#include "blueprint.hpp"

std::mt19937 rand_gen((std::random_device())());

const float Blueprint::ROWS_PER_ROOM = 16;
const float Blueprint::COLS_PER_ROOM = 26;

Blueprint::Blueprint(size_t cols, size_t rows):
	dim(cols, rows),
	max_obj(2, 2),
	moversNum(0),
	map(rows + ROWS_PER_ROOM, cols + COLS_PER_ROOM),
	coin(0, 1)
{
	rooms.x = (uint16_t) ceilf(cols / COLS_PER_ROOM);
	rooms.y = (uint16_t) ceilf(rows / ROWS_PER_ROOM);

	if (DEBUG) {
		std::cout << "Size..."
			<< "\n  ...in rooms: " << rooms.x << 'x' << rooms.y
			<< "\n  ...in blocks: " << cols << 'x' << rows
			<< std::endl;
	}

	horiz.resize(rooms.y + 1, rooms.x);
	vert.resize(rooms.y, rooms.x + 1);

	// Boundaries only at the edges of the world.
	RoomIndex ri;
	for (ri.across = 0; ri.across < rooms.x + 1; ri.across++) {
		for (ri.down = 0; ri.down < rooms.y + 1; ri.down++) {
			if (ri.across < rooms.x) {
				horiz[ri.down][ri.across] =
					(ri.down == 0) || (ri.down == rooms.y);
			}

			if (ri.down < rooms.y) {
				vert[ri.down][ri.across] =
					(ri.across == 0)
					|| (ri.across == rooms.x);
			}
		}
	}

	fill_random();

	// Compute middles
	middleRows.reserve(rooms.y + 1);
	for (int riR = 0; riR < rooms.y; riR++) {
		std::uniform_int_distribution<> choice(1, ROWS_PER_ROOM - 2 * (max_obj.y + 1));
		middleRows.push_back(max_obj.y + choice(rand_gen));
	}

	middleCols.reserve(rooms.x + 1);
	for (int riC = 0; riC < rooms.x; riC++) {
		std::uniform_int_distribution<> choice(1, COLS_PER_ROOM - 2 * (max_obj.x + 1));
		middleCols.push_back(max_obj.x + choice(rand_gen));
	}

	// Fill in walls between rooms and ladders,
	// connecting adjacent rooms.
	implement_rooms();

	add_movers();

	extra_walls();
}

void Blueprint::dump(const char *filename)
{
	const int SCALE = 16;
	const char* const colormap[] = {
		"255 255 255", // Wempty
		"0 0 0", // Wwall
		"0 0 255", // Wladder
		"255 0 0", // WliftTrack
		"255 255 0", // WmoverTrack
	};
	std::ofstream out(filename);
	out << "P3\n" << map.numCols()*SCALE << ' ' << map.numRows()*SCALE << '\n' << "255\n";

	for (const auto& row: this->map)
		for(uint8_t i = 0; i < SCALE; ++i) {
			for (const auto& col: row) {
				for(uint8_t j = 0; j < SCALE; ++j)
					out << colormap[col] << ' ';
			}
			out << '\n';
		}
}

bool Blueprint::is_room_open(const RoomIndex & ri) const
{
	return !(((ri.across > 0) && horiz[ri.down][ri.across - 1])
			|| ((ri.across < rooms.x) && horiz[ri.down][ri.across])
			|| ((ri.down > 0) && vert[ri.down - 1][ri.across])
			|| ((ri.down < rooms.y) && vert[ri.down][ri.across])
		);
}

void Blueprint::fill_random()
{
	if ((rooms.x < 2) || (rooms.y < 2))
		return;

	const float STRAND_PERCENT = 0.5f;
	const int STRAND_LENGTH = 8;

	std::uniform_int_distribution <> rand_strand(0, STRAND_LENGTH - 1);
	std::uniform_int_distribution <> rand_across(1, rooms.x);
	std::uniform_int_distribution <> rand_down(1, rooms.y);

	int strandsNum = (int)(rooms.x * rooms.y * STRAND_PERCENT);

	for (int n = 0; n < strandsNum; n++) {
		// Starting position.
		RoomIndex ri;
		ri.across = rand_across(rand_gen);
		ri.down = rand_down(rand_gen);

		bool ok = is_room_open(ri);

		// open implies that ri must not be on outer boundaries.
		while (rand_strand(rand_gen) && ok) {
			// Move horiz.
			if (coin(rand_gen)) {
				// Increase.
				if (coin(rand_gen)) {
					horiz[ri.down][ri.across] = true;
					ri.across++;
				}
				// Decrease.
				else {
					horiz[ri.down][ri.across - 1] = true;
					ri.across--;
				}
			}
			// Move vert.
			else {
				// Increase.
				if (coin(rand_gen)) {
					vert[ri.down][ri.across] = true;
					ri.down++;
				}
				// Decrease.
				else {
					vert[ri.down - 1][ri.across] = true;
					ri.down--;
				}
			}
			ok = is_room_open(ri);
		}
	}
}

void Blueprint::has_top(const RoomIndex &, const IVec2 & roomLoc)
{
	for (int c = roomLoc.x; c < roomLoc.x + COLS_PER_ROOM; c++) {
		map[roomLoc.y][c] = Wwall;
	}
}

void Blueprint::missing_top(const RoomIndex & roomIndex,
		const IVec2 & roomLoc)
{
	for (int c = roomLoc.x + middleCols[roomIndex.across];
			c < roomLoc.x + middleCols[roomIndex.across] + max_obj.x;
			c++) {
		for (int r = roomLoc.y; r < roomLoc.y + ROWS_PER_ROOM - 1;
				r++) {
			map[r][c] = Wladder;
		}
	}
}

void Blueprint::has_bottom(const RoomIndex &, const IVec2 & roomLoc)
{
	for (int c = roomLoc.x; c < roomLoc.x + COLS_PER_ROOM; c++) {
		map[roomLoc.y + ROWS_PER_ROOM - 1][c] = Wwall;
	}
}

void Blueprint::missing_bottom(const RoomIndex & roomIndex,
		const IVec2 & roomLoc)
{
	for (int c = roomLoc.x + middleCols[roomIndex.across];
	     c < roomLoc.x + middleCols[roomIndex.across] + max_obj.x;
	     c++) {
		for (int r =
				roomLoc.y + middleRows[roomIndex.down] - max_obj.y;
				r < roomLoc.y + ROWS_PER_ROOM; r++) {
			map[r][c] = Wladder;
		}
	}
}

void Blueprint::has_left(const RoomIndex &, const IVec2 & roomLoc)
{
	for (int r = roomLoc.y; r < roomLoc.y + ROWS_PER_ROOM; r++) {
		map[r][roomLoc.x] = Wwall;
	}
}

void Blueprint::missing_left(const RoomIndex & roomIndex,
			     const IVec2 & roomLoc)
{
	for (int c = roomLoc.x;
			c < roomLoc.x + middleCols[roomIndex.across]; c++) {
		map[roomLoc.y + middleRows[roomIndex.down]][c] = Wwall;
	}
}

void Blueprint::has_right(const RoomIndex &, const IVec2 & roomLoc)
{
	for (int r = roomLoc.y; r < roomLoc.y + ROWS_PER_ROOM; r++) {
		map[r][roomLoc.x + COLS_PER_ROOM - 1] = Wwall;
	}
}

void Blueprint::missing_right(const RoomIndex & roomIndex,
			      const IVec2 & roomLoc)
{
	for (int c = roomLoc.x + middleCols[roomIndex.across] + max_obj.x;
			c < roomLoc.x + COLS_PER_ROOM; c++) {
		map[roomLoc.y + middleRows[roomIndex.down]][c] = Wwall;
	}
}

void Blueprint::implement_rooms()
{
	RoomIndex ri;
	IVec2 rl;

	// Walls between rooms.
	for (ri.across = 0, rl.x = 0;
			ri.across < rooms.x;
			ri.across++, rl.x += COLS_PER_ROOM) {
		for (ri.down = 0, rl.y = 0;
				ri.down < rooms.y;
				ri.down++, rl.y += ROWS_PER_ROOM) {
			if (!horiz[ri.down][ri.across]) {
				missing_top(ri, rl);
			} else {
				has_top(ri, rl);
			}
			if (!horiz[ri.down + 1][ri.across]) {
				missing_bottom(ri, rl);
			} else {
				has_bottom(ri, rl);
			}
			if (!vert[ri.down][ri.across]) {
				missing_left(ri, rl);
			} else {
				has_left(ri, rl);
			}
			if (!vert[ri.down][ri.across + 1]) {
				missing_right(ri, rl);
			} else {
				has_right(ri, rl);
			}
		}
	}
}

bool Blueprint::inside(const IVec2 &l)
{
	return l.y >= 0 && l.x >= 0
			&& l.y < dim.y && l.x < dim.x;
}

bool Blueprint::inside(int r, int c)
{
	return inside(IVec2(c, r));
}

bool Blueprint::is_tile_open(const IVec2 &loc, bool laddersClosed,
		bool postersClosed, bool doorsClosed, bool outsideClosed)
{
	if (!inside(loc)) {
		return !outsideClosed;
	}

	/*
	// a poster isn't closed at loc
	bool posterOpen = (
		!postersClosed
		|| !(unionSquares[loc.y][loc.x]
		&& unionSquares[loc.ro][loc.x]->type == UN_POSTER));

	// a door isn't "closed" at loc
	bool doorOpen = (
		!doorsClosed
		|| !(unionSquares[loc.y][loc.x]
		&& unionSquares[loc.y][loc.x]->type == UN_DOOR));
	*/

	// a moverSquare isn't "closed" at loc  (uses laddersClosed for now)
	bool moverSquareOpen = (
		!laddersClosed
		|| !(map[loc.y][loc.x] == WliftTrack
			|| map[loc.y][loc.x] == WmoverTrack)
		);

	bool ret =
		// map contains an open square at loc
		(map[loc.y][loc.x] == Wempty
		|| (map[loc.y][loc.x] == Wladder && !laddersClosed))
		/*&& posterOpen
		&& doorOpen*/
		&& moverSquareOpen;

  // quick sanity check.
  /*if (unionSquares[loc.r][loc.c]) {
    assert(map[loc.r][loc.c] == Wempty || map[loc.r][loc.c] == Wwall);
  }*/

  return ret;
}

void Blueprint::add_movers()
{
	// A factor of "density" of movers on the level
	const float MOVERS_PERCENT = 0.4;

	// Try to place each mover at most this number
	// of times before giving up.
	const int MOVER_TRIES = 6;

	// Movers shouldn't be too big.
	assert(max_obj.x >= 2);

	// Number of movers to create.
	std::uniform_int_distribution<> rand_movers(0,
		(int)(rooms.x * rooms.y * MOVERS_PERCENT));
	const int moversActual = rand_movers(rand_gen);

	for (int n = 0; n < moversActual; n++) {
		bool which = coin(rand_gen);
		bool ok = false;
	    int tries = 0;
	    while (!ok && tries < MOVER_TRIES) {
			// Decide between horizontal and vertical mover.
			ok = which ? add_vert_mover() : add_horiz_mover();
			tries++;
		}
		if(ok)
			++moversNum;
	}
}

bool Blueprint::add_horiz_mover()
{
	// Min number of movers across a horizontal track must be.
	const int MOVERS_HORIZ_MIN_TRACK = 6;
	const int MOVERS_HORIZ_TRACK_LENGTH = 25;

	// loc will walk right or left, filling in track.
	IVec2 loc;
	loc.y = std::uniform_int_distribution<>(0, dim.y - 1)(rand_gen);
	loc.x = std::uniform_int_distribution<>(0, dim.x - 1)(rand_gen);

	// Check space surrounding initial choice for minimum track length.
	int delta = coin(rand_gen) ? 1 : -1;  // right or left.
	IVec2 check;
	for (check.x = loc.x - (delta == -1 ? MOVERS_HORIZ_MIN_TRACK : 1) * max_obj.x;
			check.x < loc.x + (delta == 1 ? MOVERS_HORIZ_MIN_TRACK + 1 : 2) * max_obj.x; // mover track plus blank space
			check.x++) {
		for (check.y = loc.y - max_obj.y;
				check.y <= loc.y + max_obj.y;
				check.y++) {
			if (!is_tile_open(check, true, true, true)) {
				return false;
			}
		}
	}

	// Start at the right edge of the mover and go left
	if (delta == -1) {
		loc.x += max_obj.x - 1;
	}
	// else we are at the left edge going right.


	// minimum and maximum values of left side of mover, inclusive
	int left = loc.x;
	int right = loc.x;

	// Go right/left until hit a wall or decide to stop.
	// Must do at least MOVERS_HORIZ_MIN_TRACK multiples of the mover width.
	int n = 0;
	bool ok = true;
	while(ok && (n <= MOVERS_HORIZ_MIN_TRACK * max_obj.x || 
			std::uniform_int_distribution<>(0, MOVERS_HORIZ_TRACK_LENGTH-1)(rand_gen))) {
		// Check if we can put a mover at loc.
		for (check.x = loc.x;
				check.x != loc.x + delta * max_obj.x;
				check.x += delta) {
			for (check.y = loc.y - max_obj.y;
					check.y <= loc.y + max_obj.y;
					check.y++) {
				if (!is_tile_open(check, true, true, true)) {
					ok = false;
					// Our above check of the surrounding space was not correct.
					assert(n >= MOVERS_HORIZ_MIN_TRACK * max_obj.x);
				}
				if (!ok) {
					break;
				}
			}
			if (!ok) {
				break;
			}
		}
                
		// Add mover track at loc.
		if (ok) {
			// Add a mover square.
			map[loc.y][loc.x] = WmoverTrack;

			left = std::min<int>(left, loc.x);
			right = std::max<int>(right, loc.x);
			n++;

			loc.x += delta;
		}
	} // while ok

	// Check again that we got enough track.
	assert(right - left + 1 >= MOVERS_HORIZ_MIN_TRACK * max_obj.x);

	return true;
}

// What an ugly function.  I should redo this.
bool Blueprint::add_vert_mover()
{
	// Choose a random room, check if its middle is a ladder, if so, change
	// the ladder to a bunch of mover squares and create a new mover.
	// Else continue.
	IVec2 init;
	int m; // holder for the down/across value of the room chosen.
	m = std::uniform_int_distribution<>(0, rooms.x - 1)(rand_gen);
	init.x =  m * COLS_PER_ROOM + middleCols[m];
	m = std::uniform_int_distribution<>(0, rooms.y - 1)(rand_gen);
	init.y =  m * ROWS_PER_ROOM + middleRows[m];

	// Found a ladder, create a new mover and set all the ladder wsquares to be
	// MoverSquares pointing to the new mover.
	if (!(map[init.y][init.x] == Wladder && 
			map[init.y][init.x+1] == Wladder)) {
		return false;
	} 

	// All wsquares between top and bottom, exclusive, are part of the
	// mover.
	int top = init.y;
	int bottom = init.y;

	// delta is -1, then 1.  Go up, then down..
	for (int delta = -1; delta <= 1; delta += 2) {
		IVec2 loc = init;

		// To avoid hitting init twice, skip it when going down.
		if (delta == 1) {
			loc.y++;
		}

		// Scan up/down depending on value of delta.
		bool bothInside = inside(loc) && inside(loc.y, loc.x + 1);
		bool bothLadders = bothInside &&
				(map[loc.y][loc.x] == Wladder && map[loc.y][loc.x+1] == Wladder);
		bool aboveInside = inside(loc.y - 1, loc.x) && inside(loc.y - 1, loc.x + 1);
		bool aboveMoverSq = aboveInside
				&& ((map[loc.y-1][loc.x] == WmoverTrack
					&& map[loc.y-1][loc.x+1] == WmoverTrack)
				|| (map[loc.y-1][loc.x] == WliftTrack
					&& map[loc.y-1][loc.x+1] == WliftTrack));
		bool bothWalls = bothInside && 
			(map[loc.y][loc.x] == Wwall && map[loc.y][loc.x + 1] == Wwall);

		// This shit is all so that movers can stick down one square into floor.
		// CAREFUL!! Crappy code duplicated below.
		while (bothLadders ||  // normal case 
				(delta == 1 && aboveMoverSq && bothWalls) ) { // sticking down one
			// Scan to right.
			for (; loc.x < init.x + max_obj.x; loc.x++) {
				// Add a mover square.
				assert(inside(loc) && 
					(map[loc.y][loc.x] == Wladder || map[loc.y][loc.x] == Wwall));
				map[loc.y][loc.x] = WliftTrack;

				// Still leave overlap of wall and moversquare.
				if (map[loc.y][loc.x] != Wwall) {
					map[loc.y][loc.x] = Wempty;
				}

				top = std::min<int>(top, loc.y);
				bottom = std::max<int>(bottom, loc.y + 1);
			}
			loc.x = init.x;

			// When up at the top.
			// Remove annoying ladder sticking up.
			if (delta ==  -1 &&
					map[loc.y - max_obj.y - 1][loc.x] != Wladder) {
				int locTop = loc.y - max_obj.y;
				for (loc.y--; loc.y >= locTop; loc.y--) {
					for (loc.x = init.x; loc.x < init.x + max_obj.x; loc.x++) {
						map[loc.y][loc.x] = Wempty;
					}
				}

				break;
			} // annoying ladder sticking up.

			loc.x = init.x;
			loc.y += delta;

			bothInside = inside(loc) && inside(loc.y, loc.x+1);
			bothLadders = bothInside && 
					(map[loc.y][loc.x] == Wladder && map[loc.y][loc.x+1] == Wladder);
			aboveInside = inside(loc.y-1, loc.x) && inside(loc.y-1, loc.x+1);
			aboveMoverSq = aboveInside
					&& ((map[loc.y-1][loc.x] == WmoverTrack
						&& map[loc.y-1][loc.x+1] == WmoverTrack)
					|| (map[loc.y-1][loc.x] == WliftTrack
						&& map[loc.y-1][loc.x+1] == WliftTrack));
			bothWalls = bothInside &&
					(map[loc.y][loc.x] == Wwall && map[loc.y][loc.x+1] == Wwall);
			// This shit is all so that movers can stick down one square into 
			// the floor.
		} // while
	} // for delta
	// Note: loc is now at the left side of the former ladder,
	 // one square beneath it.

	assert(top < bottom);  // Or no squares were added.

	return true;
}

void Blueprint::ladder(const IVec2 &init) {
	// delta is only -1 and 1.
	for (int delta = -1; delta <= 1; delta += 2) {
		IVec2 loc = init;

		// To avoid hitting init twice.
		if (delta == 1) {
			loc.y++;
		}

		// Running directly into wall or ladder.
		while (is_tile_open(loc, true)) {
			map[loc.y][loc.x] = Wladder;

			// Stop when there is a wall to the left or right.
			IVec2 left, right;
			right.y = left.y = loc.y;
			left.x = loc.x - 1;
			right.x = loc.x + 1;
			if ((!is_tile_open(left) || !is_tile_open(right))) {
				// Extra extension at top.
				if (delta == -1) {
					for (int extra = 1; extra <= max_obj.y; extra++) {
						IVec2 extraLoc;
						extraLoc.x = loc.x;
						extraLoc.y = loc.y - extra;
						if (is_tile_open(extraLoc, true)) {
							map[extraLoc.y][extraLoc.x] = Wladder;
						}
						else {
							break;
						}
					}
				}
				break;
			}

			loc.y += delta;
		} // while
	} // for delta
}

void Blueprint::extra_walls()
{
	const float WALLS_PERCENT = 0.01;
	const int MEAN_WALL_LENGTH = 30;

	const int WALLS_HORIZ_CHANCE = 4; // 2 gives equal chance
	const int LADDER_CHANCE = 10;
	const int UP_DOWN_CHANCE = 4;

	for(int walls = 0; walls < dim.y * dim.x * WALLS_PERCENT; ++walls) {
		bool ok = true;
		IVec2 loc;
		loc.y = std::uniform_int_distribution<>(0, dim.y - 1)(rand_gen);
		loc.x = std::uniform_int_distribution<>(0, dim.x - 1)(rand_gen);
		int delta = coin(rand_gen) ? 1 : -1;
		int horiz = std::uniform_int_distribution<>(0, WALLS_HORIZ_CHANCE - 1)(rand_gen);

		IVec2 check;
		for (check.x = loc.x - max_obj.x;
				check.x <= loc.x + max_obj.x;
				check.x++)
		{
			for (check.y = loc.y - max_obj.y;
					check.y <= loc.y + max_obj.y;
					check.y++)
			{
				if (!is_tile_open(check, true)) {
					ok = false;
				}
			}
		}

		// Horizontal extra wall.
		if (horiz) {
			while (ok && std::uniform_int_distribution<>(0, MEAN_WALL_LENGTH - 1)(rand_gen)) {
				for (check.x = loc.x;
						check.x != loc.x + delta * (max_obj.x + 1);
						check.x += delta)
				{
					for (check.y = loc.y - max_obj.y;
							check.y <= loc.y + max_obj.y;
							check.y++)
					{
						if (!is_tile_open(check, true)) {
							ok = false;
						}
					}
				}

				if (ok) {
					if (!std::uniform_int_distribution<>(0, LADDER_CHANCE-1)(rand_gen)) {
						ladder(loc);
					}
					else {
						map[loc.y][loc.x] = Wwall;
					}

					loc.x += delta;

					if (std::uniform_int_distribution<>(0, UP_DOWN_CHANCE-1)(rand_gen) == 0) {
						loc.y += (coin(rand_gen) ? 1 : -1);
					}
				}
			}
		}

		// Vertical wall, horiz == 0
		else {
			while (ok && std::uniform_int_distribution<>(0, MEAN_WALL_LENGTH - 1)(rand_gen)) {
				for (check.y = loc.y;
						check.y != loc.y + delta * (max_obj.y + 1);
						check.y += delta)
				{
					for (check.x = loc.x - max_obj.x;
							check.x <= loc.x + max_obj.x;
							check.x++)
					{
						if (!is_tile_open(check, true)) {
							ok = false;
						}
					}
				}
				if (ok) {
					map[loc.y][loc.x] = Wwall;
					loc.y += delta;
				}
			}
		}
	} // for walls.
}

/*
int main(int argc, char **argv)
{
	uint16_t cols = 130, rows = 32;
	const char *filename = "map.pnm";

	if (argc > 1)
		filename = argv[1];

	if (argc > 2)
		cols = atoi(argv[2]);

	if (argc > 3)
		rows = atoi(argv[3]);

	auto b = Blueprint(cols, rows);
	b.dump(filename);
}
*/

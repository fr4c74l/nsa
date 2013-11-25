#include <random>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cassert>
#include <cstdlib>

#define DEBUG 1

std::mt19937 rand_gen((std::random_device())());

struct RoomIndex {
	uint16_t across;
	uint16_t down;
};

struct Dimension {
	Dimension()
	{}
	
	Dimension(uint16_t c, uint16_t r):cols(c), rows(r)
	{}
	uint16_t cols, rows;
};

class Blueprint {
public:
	Blueprint(size_t cols, size_t rows);
	void dump(const char *filename);

private:
	enum Tiles {
		Wempty,
		Wwall,
		Wladder,
		WliftTrack,
		WmoverTrack,
		// Just a visual block that tells if there is  a nearby ladder or lift,
		// functions like a Wwall:
		WupDown, 
		Woutside,
	};

	bool is_room_open(const RoomIndex & ri) const;
	void fill_random();
	void implement_rooms();

	void has_right(const RoomIndex & ri, const Dimension & rl);
	void missing_right(const RoomIndex & ri, const Dimension & rl);
	void has_bottom(const RoomIndex & ri, const Dimension & rl);
	void missing_bottom(const RoomIndex & ri, const Dimension & rl);
	void has_top(const RoomIndex & ri, const Dimension & rl);
	void missing_top(const RoomIndex & ri, const Dimension & rl);
	void has_left(const RoomIndex & ri, const Dimension & rl);
	void missing_left(const RoomIndex & ri, const Dimension & rl);

	bool inside(const Dimension &l);
	bool is_tile_open(const Dimension &loc, bool laddersClosed=false,
			bool postersClosed=false, bool doorsClosed=false, bool outsideClosed=true);
	void add_movers();
	bool add_vert_mover();
	bool add_horiz_mover();

	static const float ROWS_PER_ROOM;
	static const float COLS_PER_ROOM;

	Dimension rooms;	// in rooms...
	Dimension dim;		// in tiles ???
	Dimension size;		// in pixels ??

	Dimension max_obj;	// biggest object in scene

	size_t moversNum;

	std::vector < std::vector < bool >> horiz, vert;
	std::vector < std::vector < uint8_t >> map;
	std::vector < int >middleCols, middleRows;

	std::uniform_int_distribution<> coin;
};

const float Blueprint::ROWS_PER_ROOM = 16;
const float Blueprint::COLS_PER_ROOM = 26;

Blueprint::Blueprint(size_t cols, size_t rows):
	dim(cols, rows),
	moversNum(0),
	map(rows + ROWS_PER_ROOM, std::vector < uint8_t > (cols + COLS_PER_ROOM)),
	coin(0, 1)
{
	const uint16_t WSQUARE_WIDTH = 16;
	const uint16_t WSQUARE_HEIGHT = 16;

	rooms.cols = (uint16_t) ceilf(cols / COLS_PER_ROOM);
	rooms.rows = (uint16_t) ceilf(rows / ROWS_PER_ROOM);

	size.cols = dim.cols * WSQUARE_WIDTH;
	size.rows = dim.rows * WSQUARE_HEIGHT;

	if (DEBUG) {
		std::cout << "Size..."
			<< "\n  ...in rooms: " << rooms.cols << 'x' << rooms.rows
			<< "\n  ...in blocks: " << cols << 'x' << rows
			<< "\n  ...in pixels(?): " << size.cols << 'x' << size.rows
			<< std::endl;
	}

	horiz.resize(rooms.rows + 1, std::vector < bool > (rooms.cols));
	vert.resize(rooms.rows, std::vector < bool > (rooms.cols + 1));

	middleCols.resize(rooms.cols + 1);
	middleRows.resize(rooms.rows + 1);

	// Boundaries only at the edges of the world.
	RoomIndex ri;
	for (ri.across = 0; ri.across < rooms.cols + 1; ri.across++) {
		for (ri.down = 0; ri.down < rooms.rows + 1; ri.down++) {
			if (ri.across < rooms.cols) {
				horiz[ri.down][ri.across] =
					(ri.down == 0) || (ri.down == rooms.rows);
			}

			if (ri.down < rooms.rows) {
				vert[ri.down][ri.across] =
					(ri.across == 0)
					|| (ri.across == rooms.cols);
			}
		}
	}

	fill_random();

	// Fill in walls between rooms and ladders,
	// connecting adjacent rooms.
	implement_rooms();
}

void Blueprint::dump(const char *filename)
{
	std::ofstream out(filename);
	out << "P2\n" << map[0].size() << ' ' << map.size() << '\n' << "20\n";

	for (const auto & row:this->map) {
		for (const auto & col:row) {
			out << (10 * (int)col) << ' ';
		}
		out << '\n';
	}
}

bool Blueprint::is_room_open(const RoomIndex & ri) const
{
	return !(((ri.across > 0) && horiz[ri.down][ri.across - 1])
			|| ((ri.across < rooms.cols) && horiz[ri.down][ri.across])
			|| ((ri.down > 0) && vert[ri.down - 1][ri.across])
			|| ((ri.down < rooms.rows) && vert[ri.down][ri.across])
		);
}

void Blueprint::fill_random()
{
	if ((rooms.cols < 2) || (rooms.rows < 2))
		return;

	const float STRAND_PERCENT = 0.5f;
	const int STRAND_LENGTH = 8;

	std::uniform_int_distribution <> rand_strand(0, STRAND_LENGTH - 1);
	std::uniform_int_distribution <> rand_across(1, rooms.cols);
	std::uniform_int_distribution <> rand_down(1, rooms.rows);

	int strandsNum = (int)(rooms.cols * rooms.rows * STRAND_PERCENT);

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

void Blueprint::has_top(const RoomIndex &, const Dimension & roomLoc)
{
	for (int c = roomLoc.cols; c < roomLoc.cols + COLS_PER_ROOM; c++) {
		map[roomLoc.rows][c] = Wwall;
	}
}

void Blueprint::missing_top(const RoomIndex & roomIndex,
		const Dimension & roomLoc)
{
	for (int c = roomLoc.cols + middleCols[roomIndex.across];
			c < roomLoc.cols + middleCols[roomIndex.across] + max_obj.cols;
			c++) {
		for (int r = roomLoc.rows; r < roomLoc.rows + ROWS_PER_ROOM - 1;
				r++) {
			map[r][c] = Wladder;
		}
	}
}

void Blueprint::has_bottom(const RoomIndex &, const Dimension & roomLoc)
{
	for (int c = roomLoc.cols; c < roomLoc.cols + COLS_PER_ROOM; c++) {
		map[roomLoc.rows + ROWS_PER_ROOM - 1][c] = Wwall;
	}
}

void Blueprint::missing_bottom(const RoomIndex & roomIndex,
		const Dimension & roomLoc)
{
	for (int c = roomLoc.cols + middleCols[roomIndex.across];
	     c < roomLoc.cols + middleCols[roomIndex.across] + max_obj.cols;
	     c++) {
		for (int r =
		     roomLoc.rows + middleRows[roomIndex.down] - max_obj.rows;
		     r < roomLoc.rows + ROWS_PER_ROOM; r++) {
			map[r][c] = Wladder;
		}
	}
}

void Blueprint::has_left(const RoomIndex &, const Dimension & roomLoc)
{
	for (int r = roomLoc.rows; r < roomLoc.rows + ROWS_PER_ROOM; r++) {
		map[r][roomLoc.cols] = Wwall;
	}
}

void Blueprint::missing_left(const RoomIndex & roomIndex,
			     const Dimension & roomLoc)
{
	for (int c = roomLoc.cols;
	     c < roomLoc.cols + middleCols[roomIndex.across]; c++) {
		map[roomLoc.rows + middleRows[roomIndex.down]][c] = Wwall;
	}
}

void Blueprint::has_right(const RoomIndex &, const Dimension & roomLoc)
{
	for (int r = roomLoc.rows; r < roomLoc.rows + ROWS_PER_ROOM; r++) {
		map[r][roomLoc.cols + COLS_PER_ROOM - 1] = Wwall;
	}
}

void Blueprint::missing_right(const RoomIndex & roomIndex,
			      const Dimension & roomLoc)
{
	for (int c = roomLoc.cols + middleCols[roomIndex.across] + max_obj.cols;
	     c < roomLoc.cols + COLS_PER_ROOM; c++) {
		map[roomLoc.rows + middleRows[roomIndex.down]][c] = Wwall;
	}
}

void Blueprint::implement_rooms()
{
	RoomIndex ri;
	Dimension rl;

	// Walls between rooms.
	for (ri.across = 0, rl.cols = 0;
	     ri.across < rooms.cols; ri.across++, rl.cols += COLS_PER_ROOM) {
		for (ri.down = 0, rl.rows = 0;
		     ri.down < rooms.rows;
		     ri.down++, rl.rows += ROWS_PER_ROOM) {
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

bool Blueprint::inside(const Dimension &l)
{
	return l.rows >= 0 && l.cols >= 0
		&& l.rows < dim.rows && l.cols < dim.cols;
}

bool Blueprint::is_tile_open(const Dimension &loc, bool laddersClosed,
		bool postersClosed, bool doorsClosed, bool outsideClosed)
{
	if (!inside(loc)) {
		return !outsideClosed;
	}

	/*
	// a poster isn't closed at loc
	bool posterOpen = (
		!postersClosed
		|| !(unionSquares[loc.rows][loc.cols]
		&& unionSquares[loc.ro][loc.cols]->type == UN_POSTER));

	// a door isn't "closed" at loc
	bool doorOpen = (
		!doorsClosed
		|| !(unionSquares[loc.rows][loc.cols]
		&& unionSquares[loc.rows][loc.cols]->type == UN_DOOR));
	*/

	// a moverSquare isn't "closed" at loc  (uses laddersClosed for now)
	bool moverSquareOpen = (
		!laddersClosed
		|| !(map[loc.rows][loc.cols] == WliftTrack
			|| map[loc.rows][loc.cols] == WmoverTrack)
		);

	bool ret =
		// map contains an open square at loc
		(map[loc.rows][loc.cols] == Wempty
		|| (map[loc.rows][loc.cols] == Wladder && !laddersClosed))
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
	assert(max_obj.cols >= 2);

	// Number of movers to create.
	std::uniform_int_distribution<> rand_movers(0,
		(int)(rooms.cols * rooms.rows * MOVERS_PERCENT));
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
	Dimension loc;
	loc.rows = std::uniform_int_distribution<>(0, dim.rows - 1)(rand_gen);
	loc.cols = std::uniform_int_distribution<>(0, dim.cols - 1)(rand_gen);

	// Check space surrounding initial choice for minimum track length.
	int delta = coin(rand_gen) ? 1 : -1;  // right or left.
	Dimension check;
	for (check.cols = loc.cols - (delta == -1 ? MOVERS_HORIZ_MIN_TRACK : 1) * max_obj.cols;
			check.cols < loc.cols + (delta == 1 ? MOVERS_HORIZ_MIN_TRACK + 1 : 2) * max_obj.cols; // mover track plus blank space
			check.cols++) {
		for (check.rows = loc.rows - max_obj.rows;
				check.rows <= loc.rows + max_obj.rows;
				check.rows++) {
			if (!is_tile_open(check, true, true, true)) {
				return false;
			}
		}
	}

	// Start at the right edge of the mover and go left
	if (delta == -1) {
		loc.cols += max_obj.cols - 1;
	}
	// else we are at the left edge going right.


	// minimum and maximum values of left side of mover, inclusive
	int left = loc.cols;
	int right = loc.cols;

	// Go right/left until hit a wall or decide to stop.
	// Must do at least MOVERS_HORIZ_MIN_TRACK multiples of the mover width.
	int n = 0;
	bool ok = true;
	while(ok && (n <= MOVERS_HORIZ_MIN_TRACK * max_obj.cols || 
			std::uniform_int_distribution<>(0, MOVERS_HORIZ_TRACK_LENGTH-1)(rand_gen))) {
		// Check if we can put a mover at loc.
		for (check.cols = loc.cols;
				check.cols != loc.cols + delta * max_obj.cols;
				check.cols += delta) {
			for (check.rows = loc.rows - max_obj.rows;
					check.rows <= loc.rows + max_obj.rows;
					check.rows++) {
				if (!is_tile_open(check, true, true, true)) {
					ok = false;
					// Our above check of the surrounding space was not correct.
					assert(n >= MOVERS_HORIZ_MIN_TRACK * max_obj.cols);
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
			map[loc.rows][loc.cols] = WmoverTrack;

			left = std::min(left, loc.cols);
			right = std::max(right, loc.cols);
			n++;

			loc.cols += delta;
		}
	} // while ok

	// Check again that we got enough track.
	assert(right - left + 1 >= MOVERS_HORIZ_MIN_TRACK * max_obj.cols);

	return true;
}


// TODO: to be contined...
// What an ugly function.  I should redo this.
Boolean World::add_vert_mover() {
  assert(moversNum <= MOVERS_MAX && blueprints);
  // Movers shouldn't be too big.
  assert(objectDimMax.colMax * WSQUARE_WIDTH >= moverSize.width);


  // Choose a random room, check if its middle is a ladder, if so, change
  // the ladder to a bunch of mover squares and create a new mover.
  // Else continue.
  Loc init;
  int m; // holder for the down/across value of the room chosen.
  m = Utils::choose(rooms.acrossMax);
  init.c =  m * W_ROOM_COL_MAX + blueprints->get_middle_col(m);
  m = Utils::choose(rooms.downMax);
  init.r =  m * W_ROOM_ROW_MAX + blueprints->get_middle_row(m);


  // Found a ladder, create a new mover and set all the ladder wsquares to be
  // MoverSquares pointing to the new mover.
  if (!(map[init.r][init.c] == Wladder && 
        map[init.r][init.c+1] == Wladder)) {
    return False;
  } 

  assert(!unionSquares[init.r][init.c]);

  // All wsquares between mTop and mBottom, exclusive, are part of the
  // mover.
  int mTop = init.r;
  int mBottom = init.r;

  movers[moversNum] = new Mover;
  assert(movers[moversNum]);

  // delta is -1, then 1.  Go up, then down..
  for (int delta = -1; delta <= 1; delta += 2) {
    Loc loc = init;

    // To avoid hitting init twice, skip it when going down.
    if (delta == 1) {
      loc.r++;
    }

    // Scan up/down depending on value of delta.
    Boolean bothInside = inside(loc) && inside(loc.r,loc.c+1);
    Boolean bothLadders = bothInside && 
      (map[loc.r][loc.c] == Wladder && map[loc.r][loc.c+1] == Wladder);
    Boolean aboveInside = inside(loc.r-1,loc.c) && inside(loc.r-1,loc.c+1);
    Boolean aboveMoverSq = aboveInside && 
      map[loc.r-1][loc.c] == Wempty && 
      map[loc.r-1][loc.c+1] == Wempty &&
      unionSquares[loc.r-1][loc.c] && 
      unionSquares[loc.r-1][loc.c+1] &&
      unionSquares[loc.r-1][loc.c]->type == UN_MOVER && 
      unionSquares[loc.r-1][loc.c+1]->type == UN_MOVER;
    Boolean bothWalls = bothInside && 
      (map[loc.r][loc.c] == Wwall && map[loc.r][loc.c+1] == Wwall);
    // This shit is all so that movers can stick down one square into floor.
    // CAREFUL!! Crappy code duplicated below.

    while (bothLadders ||  // normal case 
           (delta == 1 && aboveMoverSq && bothWalls) ) { // sticking down one
      
      // Scan to right.
      for (; loc.c < init.c + objectDimMax.colMax; loc.c++) {
        // Add a mover square.
        assert(inside(loc) && 
               (map[loc.r][loc.c] == Wladder || map[loc.r][loc.c] == Wwall) &&
               !unionSquares[loc.r][loc.c]);
        unionSquares[loc.r][loc.c] = new UnionSquare;
        assert(unionSquares[loc.r][loc.c]);
        unionSquares[loc.r][loc.c]->type = UN_MOVER;
        unionSquares[loc.r][loc.c]->mSquare.mover = movers[moversNum];
        unionSquares[loc.r][loc.c]->mSquare.orientation = OR_VERT;

        // Still leave overlap of wall and moversquare.
        if (map[loc.r][loc.c] != Wwall) {
          map[loc.r][loc.c] = Wempty;
        }

        mTop = Utils::minimum(mTop,loc.r);
        mBottom = Utils::maximum(mBottom,loc.r + 1);
    	}
	    loc.c = init.c;

      // When up at the top.
      // Remove annoying ladder sticking up.
      if (delta ==  -1 &&
          map[loc.r - objectDimMax.rowMax - 1][loc.c] != Wladder &&
          // Just to be sure we don't kill of some unionSquares.
          !unionSquares[loc.r - objectDimMax.rowMax - 1][loc.c]) {
        int locTop = loc.r - objectDimMax.rowMax;
        for (loc.r--; loc.r >= locTop; loc.r--) {
          for (loc.c = init.c; loc.c < init.c + objectDimMax.colMax; 
               loc.c++) {
            map[loc.r][loc.c] = Wempty;
          }
        }
        
        break;
      } // annoying ladder sticking up.
      
      loc.c = init.c;
      loc.r += delta;

      
      bothInside = inside(loc) && inside(loc.r,loc.c+1);
      bothLadders = bothInside && 
        (map[loc.r][loc.c] == Wladder && map[loc.r][loc.c+1] == Wladder);
      aboveInside = inside(loc.r-1,loc.c) && inside(loc.r-1,loc.c+1);
      aboveMoverSq = aboveInside && 
        map[loc.r-1][loc.c] == Wempty && 
        map[loc.r-1][loc.c+1] == Wempty &&
        unionSquares[loc.r-1][loc.c] && 
        unionSquares[loc.r-1][loc.c+1] &&
        unionSquares[loc.r-1][loc.c]->type == UN_MOVER && 
        unionSquares[loc.r-1][loc.c+1]->type == UN_MOVER;
      bothWalls = bothInside && 
        (map[loc.r][loc.c] == Wwall && map[loc.r][loc.c+1] == Wwall);
      // This shit is all so that movers can stick down one square into 
      // the floor.

    } // while
  } // for delta
  // Note: loc is now at the left side of the former ladder,
  // one square beneath it.

  assert(mTop < mBottom);  // Or no squares were added.

  // Choose Y position somewhere in range of mover squares.
  int rangeY = (mBottom - mTop) * WSQUARE_HEIGHT - moverSize.height + 1;
  assert(rangeY > 0); // or movers are more than one square high.
  int startY = Utils::choose(rangeY) + mTop * WSQUARE_HEIGHT;

  Pos pos(init.c * WSQUARE_WIDTH,startY);
  Area area(AR_RECT,pos,moverSize);

  Size vel;
  vel.set(0,Utils::coin_flip() ? W_MOVER_SPEED : -W_MOVER_SPEED);
  // initialize the mover.
  MoverId moverId;
  moverId.index = moversNum;
  moverId.unique = uniqueGen++;

  movers[moversNum]->init(this,locator,area,vel,moverId);

  // Dummy mover object, so mover gets redrawn.
  PhysMoverP p = new PhysMover(this,locator,movers[moversNum]);
  assert(p);
  locator->add(p);

  movers[moversNum]->set_phys_mover_id(p->get_id());

  moversNum++;
  return True;
}


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

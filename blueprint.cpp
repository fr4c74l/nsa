#include <random>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <fstream>

#define DEBUG 1

std::mt19937 rand_gen((std::random_device())());

struct RoomIndex {
    uint16_t across;
    uint16_t down;
};

struct Dimension {
    Dimension()
    {}

    Dimension(uint16_t c, uint16_t r):
        cols(c), rows(r)
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
        Woutside,
        Wsquanch,
        WupDown,
    };

    bool is_open(const RoomIndex &ri) const;
    void fill_random();
    void implement_rooms();

    void has_right(const RoomIndex &ri,const Dimension &rl);
    void missing_right(const RoomIndex &ri,const Dimension &rl);
    void has_bottom(const RoomIndex &ri,const Dimension &rl);
    void missing_bottom(const RoomIndex &ri,const Dimension &rl);
    void has_top(const RoomIndex &ri,const Dimension &rl);
    void missing_top(const RoomIndex &ri,const Dimension &rl);
    void has_left(const RoomIndex &ri,const Dimension &rl);
    void missing_left(const RoomIndex &ri,const Dimension &rl);

    static const float ROWS_PER_ROOM;
    static const float COLS_PER_ROOM;

    Dimension rooms; // in rooms...
    Dimension dim; // in tiles ???
    Dimension size; // in pixels ??

    Dimension max_obj; // biggest object in scene
    
    std::vector<std::vector<bool>> horiz, vert;
    std::vector<std::vector<uint8_t>> map;
    std::vector<int> middleCols, middleRows;
};

const float Blueprint::ROWS_PER_ROOM = 16;
const float Blueprint::COLS_PER_ROOM = 26;

Blueprint::Blueprint(size_t cols, size_t rows):
    dim(cols, rows),
    map(rows + ROWS_PER_ROOM, std::vector<uint8_t>(cols + COLS_PER_ROOM))
{
    const uint16_t WSQUARE_WIDTH = 16;
    const uint16_t WSQUARE_HEIGHT = 16;

    rooms.cols = (uint16_t)ceilf(cols / COLS_PER_ROOM);
    rooms.rows = (uint16_t)ceilf(rows / ROWS_PER_ROOM);

    size.cols = dim.cols * WSQUARE_WIDTH;
    size.rows = dim.rows * WSQUARE_HEIGHT;

    if(DEBUG) {
        std::cout << "Size..."
            << "\n  ...in rooms: " << rooms.cols << 'x' << rooms.rows
            << "\n  ...in blocks: " << cols << 'x' << rows
            << "\n  ...in pixels(?): " << size.cols << 'x' << size.rows
            << std::endl;
    }

    horiz.resize(rooms.rows+1, std::vector<bool>(rooms.cols));
    vert.resize(rooms.rows, std::vector<bool>(rooms.cols+1));

    middleCols.resize(rooms.cols + 1);
    middleRows.resize(rooms.rows + 1);

    // Boundaries only at the edges of the world.
    RoomIndex ri;
    for (ri.across = 0; ri.across < rooms.cols + 1; ri.across++)
    {
        for (ri.down = 0; ri.down < rooms.rows + 1; ri.down++) {
            if (ri.across < rooms.cols) {
                horiz[ri.down][ri.across] =
                    (ri.down == 0) || (ri.down == rooms.rows);
    	    }

            if (ri.down < rooms.rows) {
                vert[ri.down][ri.across] =
                    (ri.across == 0) || (ri.across == rooms.cols);
    	    }
        }
    }

    fill_random();
    
    // Fill in walls between rooms and ladders,
    // connecting adjacent rooms.
    implement_rooms();
}

void Blueprint::dump(const char* filename)
{
    std::ofstream out(filename);
    out << "P2\n"
        << map[0].size() << ' ' << map.size() << '\n'
        << "20\n";
    for(const auto& row: this->map) {
        for(const auto& col: row) {
            out << (10 * (int)col) << ' ';
        }
        out << '\n';
    }
}

bool Blueprint::is_open(const RoomIndex &ri) const
{
    return !(
        ((ri.across > 0) && horiz[ri.down][ri.across - 1])
        || ((ri.across < rooms.cols) && horiz[ri.down][ri.across])
        || ((ri.down > 0) && vert[ri.down - 1][ri.across])
        || ((ri.down < rooms.rows) && vert[ri.down][ri.across])
    );
}

void Blueprint::fill_random() {
    if ((rooms.cols < 2) || (rooms.rows < 2))
        return;

    const float STRAND_PERCENT = 0.5f;
    const int STRAND_LENGTH = 8;

    std::uniform_int_distribution<>
        rand_strand(0, STRAND_LENGTH - 1);
    std::uniform_int_distribution<> rand_across(1, rooms.cols);
    std::uniform_int_distribution<> rand_down(1, rooms.rows);
    std::uniform_int_distribution<> coin(0, 1);

    int strandsNum =
        (int)(rooms.cols * rooms.rows * STRAND_PERCENT);

    for (int n = 0; n < strandsNum; n++) {
        // Starting position.
        RoomIndex ri;
        ri.across = rand_across(rand_gen);
        ri.down = rand_down(rand_gen);

        bool ok = is_open(ri);

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
            ok = is_open(ri);
        }
    }
}

void Blueprint::has_top(const RoomIndex &, const Dimension &roomLoc) {
  for (int c = roomLoc.cols; c < roomLoc.cols + COLS_PER_ROOM; c++) {
    map[roomLoc.rows][c] = Wwall;
  }
}

void Blueprint::missing_top(const RoomIndex &roomIndex,
        const Dimension &roomLoc) {
  for (int c = roomLoc.cols + middleCols[roomIndex.across];
       c < roomLoc.cols + middleCols[roomIndex.across] + max_obj.cols;
       c++) {
    for (int r = roomLoc.rows; r < roomLoc.rows + ROWS_PER_ROOM - 1; r++) {
      map[r][c] = Wladder;
    }
  }
}

void Blueprint::has_bottom(const RoomIndex &,
		const Dimension &roomLoc) {
  for (int c = roomLoc.cols; c < roomLoc.cols + COLS_PER_ROOM; c++) {
    map[roomLoc.rows + ROWS_PER_ROOM - 1][c] = Wwall;
  }
}

void Blueprint::missing_bottom(const RoomIndex &roomIndex,
				                        const Dimension &roomLoc) {
  for (int c = roomLoc.cols + middleCols[roomIndex.across];
       c < roomLoc.cols + middleCols[roomIndex.across] + max_obj.cols;
       c++) {
    for (int r = roomLoc.rows + middleRows[roomIndex.down] - max_obj.rows;
         r < roomLoc.rows + ROWS_PER_ROOM;
         r++) {
      map[r][c] = Wladder;
    }
  }
}

void Blueprint::has_left(const RoomIndex &,const Dimension &roomLoc) {
  for (int r = roomLoc.rows; r < roomLoc.rows + ROWS_PER_ROOM; r++) {
    map[r][roomLoc.cols] = Wwall;
  }
}

void Blueprint::missing_left(const RoomIndex &roomIndex,
                              const Dimension &roomLoc) {
  for (int c = roomLoc.cols; c < roomLoc.cols + middleCols[roomIndex.across]; c++) {
    map[roomLoc.rows + middleRows[roomIndex.down]][c] = Wwall;
  }
}

void Blueprint::has_right(const RoomIndex &,const Dimension &roomLoc) {
  for (int r = roomLoc.rows; r < roomLoc.rows + ROWS_PER_ROOM; r++) {
    map[r][roomLoc.cols + COLS_PER_ROOM - 1] = Wwall;
  }
}

void Blueprint::missing_right(const RoomIndex &roomIndex,
                               const Dimension &roomLoc) {
  for (int c = roomLoc.cols + middleCols[roomIndex.across] + max_obj.cols;
       c < roomLoc.cols + COLS_PER_ROOM;
       c++) {
    map[roomLoc.rows + middleRows[roomIndex.down]][c] = Wwall;
  }
}

void Blueprint::implement_rooms()
{
    RoomIndex ri;
    Dimension rl;

    // Walls between rooms.
    for (ri.across = 0, rl.cols = 0;
        ri.across < rooms.cols;
        ri.across++, rl.cols += COLS_PER_ROOM) {
        for (ri.down = 0, rl.rows = 0;
	        ri.down < rooms.rows;
	        ri.down++, rl.rows += ROWS_PER_ROOM)
        {
            if (! horiz[ri.down][ri.across]) {
                missing_top(ri,rl);
            }
            else {
                has_top(ri,rl);
            }
            if (! horiz[ri.down + 1][ri.across]) {
                missing_bottom(ri,rl);
            }
            else {
                has_bottom(ri,rl);
            }
            if (! vert[ri.down][ri.across]) {
                missing_left(ri,rl);
            }
            else {
                has_left(ri,rl);
            }
            if (! vert[ri.down][ri.across + 1]) {
                missing_right(ri,rl);
            }
            else {
                has_right(ri,rl);
            }
        }
    }
}

int main(int argc, char **argv) {
    uint16_t cols = 130, rows = 32;
    const char* filename = "map.pnm";

    if(argc > 1)
        filename = argv[1];

    if(argc > 2)
        cols = atoi(argv[2]);

    if(argc > 3)
        rows = atoi(argv[3]);

    auto b = Blueprint(cols, rows);
    b.dump(filename);
}

#pragma once

#include <cstdint>
#include <cstdlib>
#include <random>

#include "heapmatrix.hpp"

struct Dimension {
	Dimension()
	{}	
	Dimension(uint16_t c, uint16_t r):
		cols(c), rows(r)
	{}
	int16_t cols, rows;
};

class Blueprint {
public:
	Blueprint(size_t cols, size_t rows);
	void dump(const char *filename);

	enum Tiles {
		Wempty,
		Wwall,
		Wladder,
		WliftTrack,
		WmoverTrack,
	};

	HeapMatrix<uint8_t>& getMap()
	{
		return map;
	}

private:

	struct RoomIndex {
		uint16_t across;
		uint16_t down;
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
	bool inside(int r, int c);
	bool is_tile_open(const Dimension &loc, bool laddersClosed=false,
			bool postersClosed=false, bool doorsClosed=false, bool outsideClosed=true);
	void add_movers();
	bool add_vert_mover();
	bool add_horiz_mover();

	void ladder(const Dimension& init);
	void extra_walls();

	static const float ROWS_PER_ROOM;
	static const float COLS_PER_ROOM;

	Dimension rooms;	// in rooms
	Dimension dim;		// in tiles

	Dimension max_obj;	// biggest object in scene

	size_t moversNum;

	HeapMatrix<bool> horiz, vert;
	HeapMatrix<uint8_t> map;
	std::vector < int >middleCols, middleRows;

	std::uniform_int_distribution<> coin;
};


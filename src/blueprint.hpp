#pragma once

#include "precompiled.hpp"

#include <cstdint>
#include <cstdlib>
#include <random>
#include "heapmatrix.hpp"
#include "vec2.hpp"

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

	static b2Vec2 toCoord(const IVec2& pos) {
		return b2Vec2(pos.x, -pos.y);
	}

private:

	struct RoomIndex {
		uint16_t across;
		uint16_t down;
	};

	bool is_room_open(const RoomIndex & ri) const;
	void fill_random();
	void implement_rooms();

	void has_right(const RoomIndex & ri, const IVec2 & rl);
	void missing_right(const RoomIndex & ri, const IVec2 & rl);
	void has_bottom(const RoomIndex & ri, const IVec2 & rl);
	void missing_bottom(const RoomIndex & ri, const IVec2 & rl);
	void has_top(const RoomIndex & ri, const IVec2 & rl);
	void missing_top(const RoomIndex & ri, const IVec2 & rl);
	void has_left(const RoomIndex & ri, const IVec2 & rl);
	void missing_left(const RoomIndex & ri, const IVec2 & rl);

	bool inside(const IVec2 &l);
	bool inside(int r, int c);
	bool is_tile_open(const IVec2 &loc, bool laddersClosed=false,
			bool postersClosed=false, bool doorsClosed=false, bool outsideClosed=true);
	void add_movers();
	bool add_vert_mover();
	bool add_horiz_mover();

	void ladder(const IVec2& init);
	void extra_walls();

	static const float ROWS_PER_ROOM;
	static const float COLS_PER_ROOM;

	IVec2 rooms;	// in rooms
	IVec2 dim;		// in tiles

	IVec2 max_obj;	// biggest object in scene

	size_t moversNum;

	HeapMatrix<bool> horiz, vert;
	HeapMatrix<uint8_t> map;
	std::vector < int >middleCols, middleRows;

	std::uniform_int_distribution<> coin;
};


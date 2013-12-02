#include "precompiled.hpp"

#include <unordered_set>
#include <iostream>
#include "blueprint.hpp"

class Circuit
{
public:
	Circuit(const HeapMatrix<uint8_t>& map, HeapMatrix<bool>& visited, const IVec2& max, IVec2 cur);
	const std::vector<b2Vec2>& getPath() const
	{
		return mVertices;
	}
private:
	enum Side {
		DOWN,
		LEFT,
		UP,
		RIGHT
	};

	static const b2Vec2 OFFSET[4];
	static const IVec2 OPPOSITE[4];

	bool is_wall(const IVec2& pos) const;
	bool edge(Side s, IVec2 pos) const;
	IVec2 next_from(Side s) const;

	void trace(Side s);

	const IVec2& mMax;
	const HeapMatrix<uint8_t>& mMap;
	HeapMatrix<bool>& mVisited;
	std::vector<b2Vec2> mVertices;

	IVec2 mCur;
};

const b2Vec2 Circuit::OFFSET[4] = {
	b2Vec2(-0.5, -0.5),
	b2Vec2(-0.5, 0.5),
	b2Vec2(0.5, 0.5),
	b2Vec2(0.5, -0.5)
};

const IVec2 Circuit::OPPOSITE[4] = {
	IVec2(-1, 1),
	IVec2(-1, -1),
	IVec2(1, -1),
	IVec2(1, 1)
};

Circuit::Circuit(const HeapMatrix<uint8_t>& map, HeapMatrix<bool>& visited, const IVec2& max, IVec2 cur):
	mMax(max),
	mMap(map),
	mVisited(visited),
	mCur(cur)
{
	assert(is_wall(cur));

	mVisited[cur.y][cur.x] = true;
	for(int s = 0; s < 4; ++s) {
		Side side = static_cast<Side>(s);
		if(edge(side, mCur)) {
			trace(side);
			break;
		}
	}
}

bool Circuit::is_wall(const IVec2& pos) const
{
	return static_cast<Blueprint::Tiles>(mMap[pos.y][pos.x]) == Blueprint::Wwall;
}

bool Circuit::edge(Side s, IVec2 pos) const
{
	// We work under assumption this position is a wall...
	assert(is_wall(pos));

	// Displace pos to the relevant side:
	switch(s)
	{
	case DOWN:
		if(++pos.y >= mMax.y)
			return false;
		break;
	case LEFT:
		if(--pos.x < 0)
			return false;
		break;
	case UP:
		if(--pos.y < 0)
			return false;
		break;
	case RIGHT:
		if(++pos.x >= mMax.x)
			return false;
		break;
	}

	// If new position is not a wall, and previous was,
	// we found an edge!
	return !is_wall(pos);
}

IVec2 Circuit::next_from(Side s) const
{
	IVec2 ret = mCur;
	switch(s)
	{
	case DOWN:
		--ret.x;
		break;
	case LEFT:
		--ret.y;
		break;
	case UP:
		++ret.x;
		break;
	case RIGHT:
		++ret.y;
		break;
	}
	return ret;
}

void Circuit::trace(Side s)
{
	for(;;) {
		IVec2 next = next_from(s);
		while(is_wall(next) && edge(s, next)) {
			mCur = next;
			mVisited[next.y][next.x] = true;
			next = next_from(s);
		}

		b2Vec2 vertex = Blueprint::toCoord(mCur) + OFFSET[s];
		if(!mVertices.empty()) {
			b2Vec2 delta = mVertices[0] - vertex;
			if(delta.x > -0.5 && delta.x < 0.5 && delta.y > -0.5 && delta.y < 0.5)
				break;
		}
		mVertices.push_back(vertex);

		Side maybe_next_side = static_cast<Side>((s+1) % 4);
		if(edge(maybe_next_side, mCur)) {
			// The edge follows the same tile around, clockwise...
			s = maybe_next_side;
		} else {
			// The edge follows counter-clockwise, on another
			// tile diagonally touching the current one...
			mCur += OPPOSITE[s];
			s = static_cast<Side>((s+3) % 4);

			mVisited[mCur.y][mCur.x] = true;
			assert(edge(s, mCur));
		}
	}
}

void create_line_material()
{
	// NOTE: The second parameter to the create method is the resource group the material will be added to.
	// If the group you name does not exist (in your resources.cfg file) the library will assert() and your program will crash
	Ogre::MaterialPtr myManualObjectMaterial = Ogre::MaterialManager::getSingleton().create("line","General"); 
	myManualObjectMaterial->setReceiveShadows(false); 
	myManualObjectMaterial->getTechnique(0)->setLightingEnabled(true); 
	myManualObjectMaterial->getTechnique(0)->getPass(0)->setDiffuse(0,1,1,0); 
	myManualObjectMaterial->getTechnique(0)->getPass(0)->setAmbient(0,1,1); 
	myManualObjectMaterial->getTechnique(0)->getPass(0)->setSelfIllumination(0,0,1);
}

void draw_lines(Ogre::SceneManager *sm, Ogre::SceneNode* root, std::vector<b2Vec2> verts)
{
	Ogre::ManualObject* myManualObject = sm->createManualObject(); 
	Ogre::SceneNode* myManualObjectNode = root->createChildSceneNode(); 
	 
	myManualObject->begin("line", Ogre::RenderOperation::OT_LINE_STRIP);
	for(const auto& v: verts) {
		myManualObject->position(v.x, v.y, 2);
	}
	myManualObject->position(verts[0].x, verts[0].y, 2);
	myManualObject->end();
	 
	myManualObjectNode->attachObject(myManualObject);
}

void build_level(Ogre::SceneManager* sm, b2Body* world_body,
		uint16_t cols=130, uint16_t rows=32)
{
	Ogre::SceneNode* root = sm->getRootSceneNode();

	// First, we define a plane that will be the background of the level
	auto &meshmngr = Ogre::MeshManager::getSingleton();
	auto bg_wall_mesh = meshmngr.createPlane("bgWall", "General",
			Ogre::Plane(Ogre::Vector3::UNIT_Z, 0),
			cols, rows
			// TODO: the rest of the parameters must be adjusted in order to use texture
	);
	auto bg_wall = sm->createEntity(bg_wall_mesh);
	bg_wall->setMaterialName("grey");
	root->createChildSceneNode(Ogre::Vector3(0, 0, -1.5))->attachObject(bg_wall);

	// Drawing map.
	// Generate map with XEvil algorithm and grab the matrix for usage.
	HeapMatrix<uint8_t> map = std::move(Blueprint(cols, rows).getMap());

	// Create a scene node to displace to whole map to correct position 
	auto walls = root->createChildSceneNode();
	walls->setPosition(Ogre::Vector3((cols - 1) * -0.5, (rows - 1) * 0.5, 0));

	// Pre-load the tile mesh
	auto wall_tile = meshmngr.load("wall_tile.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
	wall_tile->getSubMesh(0)->setMaterialName("darkgrey");

	// Assemble the blocks
	for(int i = 0; i < rows; ++i) {
		for(int j = 0; j < cols; ++j) {
			auto t = static_cast<Blueprint::Tiles>(map[i][j]);
			if(t != Blueprint::Wempty) {
				auto node = walls->createChildSceneNode(Ogre::Vector3(j, -i, 0));
				// TODO: use different meshes for each tile...
				auto block = sm->createEntity(wall_tile);
				node->attachObject(block);

				const char* mat_name = nullptr;
				switch(t) {
					case Blueprint::Wladder:
						node->setScale(Ogre::Vector3(1, 1, 1.0/3.0));
						node->translate(Ogre::Vector3(0, 0, -1));
						mat_name = "blue";
						break;
					case Blueprint::WliftTrack:
						mat_name = "red";
						node->setScale(Ogre::Vector3(1, 1, 1.0/3.0));
						break;
					case Blueprint::WmoverTrack:
						mat_name = "yellow";
						node->setScale(Ogre::Vector3(1, 1, 1.0/3.0));
						node->translate(Ogre::Vector3(0, 0, 1));
						break;
					case Blueprint::Wwall:
						// Already set...
					case Blueprint::Wempty:
						// Can't happen...
						;
				}
				if(mat_name)
					block->setMaterialName(mat_name);
			}	
		}
	}

	// Set correct position for world physics body
	auto& walls_pos = walls->getPosition();
	world_body->SetTransform(b2Vec2(walls_pos.x, walls_pos.y), 0);

	if(DEBUG)
		create_line_material();
	
	// Build map collidable shape
	HeapMatrix<bool> visited(rows, cols, false);
	const IVec2 max(cols, rows);
	int closed_edges_count = 0;
	for(int i = 0; i < rows; ++i) {
		for(int j = 0; j < cols; ++j) {
			auto t = static_cast<Blueprint::Tiles>(map[i][j]);
			if(t == Blueprint::Wwall && !visited[i][j]) {
				Circuit c(map, visited, max, IVec2(j, i));
				auto& path = c.getPath();
				if(!path.empty()) {
					b2ChainShape circuit_shape;
					circuit_shape.CreateLoop(&path[0], path.size());
					world_body->CreateFixture(&circuit_shape, 0);
					++closed_edges_count;
					if(DEBUG)
						draw_lines(sm, walls, path);
				}
			}
		}
	}

	if(DEBUG)
		std::cout << "Closed edges count: " << closed_edges_count << std::endl;
}

class Updater:
	public Ogre::FrameListener
{
public:
	Updater(Ogre::SceneNode* cube, Ogre::Camera* cam):
		x(-60), mCam(cam), mCube(cube)
	{}

	bool frameStarted(const Ogre::FrameEvent&)
	{
		/*auto angle = Ogre::Radian(Ogre::Math::UnitRandom() * 0.1);
		auto rand_dir = Ogre::Vector3::UNIT_X.randomDeviant(angle, Ogre::Vector3::UNIT_Y);
		auto rot = Ogre::Vector3::UNIT_X.getRotationTo(rand_dir);
		mCube->rotate(rot);*/

		x += 0.05;
		if(x > 60.0f)
			return false;		
		float y = sinf(x/5) * 15;
		//float z = sqrtf(100*100 - x*x);

		mCube->setPosition(Ogre::Vector3(x, 0, 0));
		mCam->setPosition(Ogre::Vector3(x, y, 20));
		//mCam->lookAt(Ogre::Vector3::ZERO);

		return true;
	}
private:
	float x;
	Ogre::Camera* mCam;
	Ogre::SceneNode* mCube;
};

int main()
{
	// Setup physics simulation Box2D
	b2World physics(b2Vec2(0, -9.8));

	b2Body* world_body;
	{
		b2BodyDef def;
		world_body = physics.CreateBody(&def);
	}

	// Setup graphics engine Ogre
	Ogre::Root renderer("", "", "renderer.log");

	// Load plugins
	{
		// A list of required plugins
		Ogre::StringVector required_plugins;
		required_plugins.push_back("GL RenderSystem");
		required_plugins.push_back("Octree Scene Manager");

		// List of plugins to load
		Ogre::StringVector plugins_to_load;
		plugins_to_load.push_back("/usr/lib/x86_64-linux-gnu/OGRE-1.8.0/RenderSystem_GL");
		plugins_to_load.push_back("/usr/lib/x86_64-linux-gnu/OGRE-1.8.0/Plugin_OctreeSceneManager");

		for(auto& plugin_name: plugins_to_load) {
			renderer.loadPlugin(plugin_name);
		}
		std::unordered_set<Ogre::String> installed_plugins;
		std::cout << "Installed plugins:\n";
		for(auto& plugin: renderer.getInstalledPlugins()) {
			std::cout << "  - " << plugin->getName() << "\n";
			installed_plugins.insert(plugin->getName());
		}

		for(auto& plugin_name: required_plugins) {
			if(installed_plugins.find(plugin_name) == installed_plugins.end()) {
				std::cerr << "Needed plugin " << plugin_name << " was not loaded!\n";
				return 1;
			}
		}
	}

	// Configure OpenGL RenderSystem
	{
		Ogre::RenderSystem* rs = renderer.getRenderSystemByName("OpenGL Rendering Subsystem");
		assert(rs->getName() == "OpenGL Rendering Subsystem");
		// configure our RenderSystem
		rs->setConfigOption("Full Screen", "No");
		rs->setConfigOption("VSync", "Yes");
		rs->setConfigOption("Video Mode", "800 x 600 @ 32-bit");
	 
		renderer.setRenderSystem(rs);
	}

	// Build scene
	{
		renderer.addResourceLocation("./assets", "FileSystem", "General");
		Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

		auto window = renderer.initialise(true, "No Such Arrocha");
		auto sceneManager = renderer.createSceneManager("OctreeSceneManager");
		auto camera = sceneManager->createCamera("PlayerCam");
		
		// Position it at 10 in Z direction
		//camera->setPosition(Ogre::Vector3(0,0,20));
		// Look back along -Z
		camera->lookAt(Ogre::Vector3(0,0,-1));
		camera->setNearClipDistance(1);
		
		auto vp = window->addViewport(camera);
		vp->setBackgroundColour(Ogre::ColourValue(0.4, 0.3, 1.0));
		camera->setAspectRatio(
			Ogre::Real(vp->getActualWidth()) / Ogre::Real(vp->getActualHeight()));

		auto pill = sceneManager->createEntity("pill", "pill.mesh");
		pill->setMaterialName("green");
		auto pill_node = sceneManager->getRootSceneNode()->createChildSceneNode();
		pill_node->attachObject(pill);

		// Lighting
		sceneManager->setAmbientLight(Ogre::ColourValue(0.5, 0.5, 0.5));

		auto sun = sceneManager->createLight("sun");
		sun->setType(Ogre::Light::LT_DIRECTIONAL);
		sun->setDiffuseColour(Ogre::ColourValue::White);
		sun->setSpecularColour(Ogre::ColourValue::White);
		sun->setDirection(Ogre::Vector3(-1, -5, -2));

		build_level(sceneManager, world_body);
		renderer.addFrameListener(new Updater(pill_node, camera));
	}

	renderer.startRendering();

	return 0;
}

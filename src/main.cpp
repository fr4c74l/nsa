#include "precompiled.hpp"

#include <unordered_set>
#include <iostream>
#include "blueprint.hpp"

void build_level(Ogre::SceneManager* sm, uint16_t cols=130, uint16_t rows=32)
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
	walls->setPosition(Ogre::Vector3(cols * -0.5, rows * 0.5, 0));

	// Pre-load the tile mesh
	auto wall_tile = meshmngr.load("wall_tile.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

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
					case Blueprint::Wwall:
						mat_name = "dark_grey";
						break;
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
						break;
					case Blueprint::Wempty:
						// Can't happen
						break;
				}
				block->setMaterialName(mat_name);
			}	
		}
	}
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

		x += 0.15;
		if(x > 60.0f)
			return false;		
		float y = sinf(x/5) * 10;

		mCube->setPosition(Ogre::Vector3(x, 0, 0));
		mCam->setPosition(Ogre::Vector3(x, y, 20));

		return true;
	}
private:
	float x;
	Ogre::Camera* mCam;
	Ogre::SceneNode* mCube;
};

int main()
{
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
		rs->setConfigOption("VSync", "No");
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

		build_level(sceneManager);
		renderer.addFrameListener(new Updater(pill_node, camera));
	}

	renderer.startRendering();

	return 0;
}

#include "precompiled.hpp"

#include <unordered_set>
#include <iostream>

class Updater:
	public Ogre::FrameListener
{
public:
	Updater(Ogre::SceneNode *cube):
		mCube(cube)
	{}

	bool frameStarted(const Ogre::FrameEvent&)
	{
		auto angle = Ogre::Radian(Ogre::Math::UnitRandom() * 0.1);
		auto rand_dir = Ogre::Vector3::UNIT_X.randomDeviant(angle, Ogre::Vector3::UNIT_Y);
		auto rot = Ogre::Vector3::UNIT_X.getRotationTo(rand_dir);
		mCube->rotate(rot);
		return true;
	}
private:
	Ogre::SceneNode *mCube;
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

		// Position it at 500 in Z direction
		camera->setPosition(Ogre::Vector3(0,0,10));
		// Look back along -Z
		camera->lookAt(Ogre::Vector3(0,0,-1));
		camera->setNearClipDistance(1);
		
		auto vp = window->addViewport(camera);
		vp->setBackgroundColour(Ogre::ColourValue(0.4, 0.3, 1.0));
		camera->setAspectRatio(
			Ogre::Real(vp->getActualWidth()) / Ogre::Real(vp->getActualHeight()));

		auto cube = sceneManager->createEntity("cube", "Prefab_Cube");
		cube->setMaterialName("green");
		auto cube_node = sceneManager->getRootSceneNode()->createChildSceneNode();
		cube_node->attachObject(cube);
		cube_node->setScale(Ogre::Vector3(0.02, 0.02, 0.02));

		// Lighting
		sceneManager->setAmbientLight(Ogre::ColourValue(0.5, 0.5, 0.5));

		auto sun = sceneManager->createLight("sun");
		sun->setType(Ogre::Light::LT_DIRECTIONAL);
		sun->setDiffuseColour(Ogre::ColourValue::White);
		sun->setSpecularColour(Ogre::ColourValue::White);
		sun->setDirection(Ogre::Vector3(-1, -5, -2));

		renderer.addFrameListener(new Updater(cube_node));
	}

	renderer.startRendering();

	return 0;
}

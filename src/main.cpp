#include <unordered_set>
#include <iostream>
#include <Ogre.h>
#include <OgrePlugin.h>

int main()
{
	Ogre::Root renderer("", "", "renderer.log");

	{
		// A list of required plugins
		Ogre::StringVector required_plugins;
		required_plugins.push_back("GL RenderSystem");
		required_plugins.push_back("Octree & Terrain Scene Manager");

		// List of plugins to load
		Ogre::StringVector plugins_to_load;
		plugins_to_load.push_back("RenderSystem_GL");
		plugins_to_load.push_back("Plugin_OctreeSceneManager");

		for(auto& plugin_name: plugins_to_load) {
			renderer.loadPlugin(plugin_name);
		}
		std::unordered_set<Ogre::String> installed_plugins;
		for(auto& plugin: renderer.getInstalledPlugins()) {
			installed_plugins.insert(plugin->getName());
		}

		for(auto& plugin_name: required_plugins) {
			if(installed_plugins.find(plugin_name) == installed_plugins.end()) {
				std::cerr << "Needed plugin " << plugin_name << " was not loaded!\n";
				return 1;
			}
		}
	}

	if(!renderer.restoreConfig() && !renderer.showConfigDialog())
	{
		return 1;
	}

	auto window = renderer.initialise(true, "No Such Arrocha");
	auto sceneManager = renderer.createSceneManager("Octree & Terrain Scene Manager");

	auto camera = sceneManager->createCamera("PlayerCam");

	// Position it at 500 in Z direction
	camera->setPosition(Ogre::Vector3(0,0,10));
	// Look back along -Z
	camera->lookAt(Ogre::Vector3(0,0,-1));
	camera->setNearClipDistance(1);
	
	auto vp = window->addViewport(camera);
	vp->setBackgroundColour(Ogre::ColourValue(0.8, 0.8, 1.0));
	camera->setAspectRatio(
		Ogre::Real(vp->getActualWidth()) / Ogre::Real(vp->getActualHeight()));

	auto cube_node = sceneManager->getRootSceneNode()->createChildSceneNode();
	cube_node->attachObject(
		sceneManager->createEntity("cube", "Prefab_Cube")
	);

	sceneManager->setAmbientLight(Ogre::ColourValue(0.5, 0.5, 0.5));

	renderer.startRendering();

	return 0;
}

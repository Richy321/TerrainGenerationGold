#include "TerrainGenerator.h"

namespace Terrain
{
	/// Scene containing a box with octet.
	class TerrainGeneration : public octet::app
	{
		// scene for drawing box
		octet::ref<octet::visual_scene> app_scene;
		CustomTerrain* terrain;
		octet::camera_instance *camera;				  /// main camera instance 

	public:
		/// this is called when we construct the class before everything is initialised.
		TerrainGeneration(int argc, char **argv) : app(argc, argv)
		{
		}

		/// this is called once OpenGL is initialized
		void app_init()
		{
			app_scene = new octet::visual_scene();
			app_scene->create_default_camera_and_lights();

			octet::material *green = new octet::material(octet::vec4(0, 1, 0, 1));
			octet::scene_node *node = new octet::scene_node();


			octet::vec3 size(100.0f, 0.0f, 100.0f);
			octet::ivec3 dimensions(20, 0, 20);
			CustomTerrain::Algorithm genAlgorithm = CustomTerrain::DiamondSquare;


			//change camera pos
			camera = app_scene->get_camera_instance(0);
			camera->set_far_plane(1000.0f);
			camera->get_node()->loadIdentity();
			camera->get_node()->rotate(-35, octet::vec3(1, 0, 0));
			camera->get_node()->translate(octet::vec3(size.x(), -size.z(), 400.0f));
			

			terrain = TerrainGenerator::Generate(genAlgorithm, size, dimensions);

			app_scene->add_child(node);
			app_scene->add_mesh_instance(new octet::mesh_instance(node, terrain, green));
		}

		/// this is called to draw the world
		void draw_world(int x, int y, int w, int h)
		{
			int vx = 0, vy = 0;
			get_viewport_size(vx, vy);
			app_scene->begin_render(vx, vy);



			// update matrices. assume 30 fps.
			app_scene->update(1.0f / 30);

			HandleKeyboardControl();


			// draw the scene
			app_scene->render((float)vx / vy);
		}

		void HandleKeyboardControl()
		{
			if (is_key_down('W')){
				app_scene->get_camera_instance(0)->get_node()->access_nodeToParent().translate(0, 2.5, 0);
			}
			else if (is_key_down('S')){
				app_scene->get_camera_instance(0)->get_node()->access_nodeToParent().translate(0, -2.5, 0);
			}
			else if (is_key_down('A')){
				app_scene->get_camera_instance(0)->get_node()->access_nodeToParent().translate(-2.5, 0, 0);
			}
			else if (is_key_down('D')){
				app_scene->get_camera_instance(0)->get_node()->access_nodeToParent().translate(2.5, 0, 0);
			}
			else if (is_key_down('Q')){
				app_scene->get_camera_instance(0)->get_node()->access_nodeToParent().translate(0, 0, -2.5);
			}
			else if (is_key_down('E')){
				app_scene->get_camera_instance(0)->get_node()->access_nodeToParent().translate(0, 0, 2.5);
			}
		}

	};
}

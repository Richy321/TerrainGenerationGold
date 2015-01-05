////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
namespace Terrain 
{
  /// Scene containing a box with octet.
  class TerrainGeneration : public octet::app 
  {
    // scene for drawing box
	  octet::ref<octet::visual_scene> app_scene;
  public:
    /// this is called when we construct the class before everything is initialised.
    TerrainGeneration(int argc, char **argv) : app(argc, argv) {
    }

    /// this is called once OpenGL is initialized
    void app_init() {
		app_scene = new octet::visual_scene();
      app_scene->create_default_camera_and_lights();

	  octet::material *red = new octet::material(octet::vec4(1, 0, 0, 1));
	  octet::mesh_box *box = new octet::mesh_box(octet::vec3(4));
	  octet::scene_node *node = new octet::scene_node();
      app_scene->add_child(node);
	  app_scene->add_mesh_instance(new octet::mesh_instance(node, box, red));
    }

    /// this is called to draw the world
	void draw_world(int x, int y, int w, int h)
	{
		int vx = 0, vy = 0;
		get_viewport_size(vx, vy);
		app_scene->begin_render(vx, vy);

		// update matrices. assume 30 fps.
		app_scene->update(1.0f / 30);

		// draw the scene
		app_scene->render((float)vx / vy);

		// tumble the box  (there is only one mesh instance)
		octet::scene_node *node = app_scene->get_mesh_instance(0)->get_node();
		node->rotate(1, octet::vec3(1, 0, 0));
		node->rotate(1, octet::vec3(0, 1, 0));
	}


  };
}

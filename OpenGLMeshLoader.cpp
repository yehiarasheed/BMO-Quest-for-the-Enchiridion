#include "TextureBuilder.h"
#include "Model_3DS.h"
#include "Model_OBJ.h"
#include "GLTexture.h"
#include <glut.h>

int WIDTH = 1280;
int HEIGHT = 720;

GLuint tex;
char title[] = "3D Model Loader Sample";

// 3D Projection Options
GLdouble fovy = 45.0;
GLdouble aspectRatio = (GLdouble)WIDTH / (GLdouble)HEIGHT;
GLdouble zNear = 0.1;
GLdouble zFar = 100;

class Vector
{
public:
	GLdouble x, y, z;
	Vector() {}
	Vector(GLdouble _x, GLdouble _y, GLdouble _z) : x(_x), y(_y), z(_z) {}
	//================================================================================================//
	// Operator Overloading; In C++ you can override the behavior of operators for you class objects. //
	// Here we are overloading the += operator to add a given value to all vector coordinates.        //
	//================================================================================================//
	void operator +=(float value)
	{
		x += value;
		y += value;
		z += value;
	}
};

Vector Eye(20, 5, 20);
Vector At(0, 0, 0);
Vector Up(0, 1, 0);

int cameraZoom = 0;

// Camera Mode
enum CameraMode { FIRST_PERSON, THIRD_PERSON };
CameraMode currentCamera = THIRD_PERSON;

// Model Variables
//Model_3DS model_house;
//Model_3DS model_tree;
Model_3DS model_donut;
Model_OBJ model_candy_kingdom;
Model_OBJ model_bmo;
Model_OBJ model_finn; // non-collidable
Model_OBJ model_cupcake;
Model_OBJ model_coin;
Model_OBJ model_lich;
Model_OBJ model_jelly; // non-collidable

// Cupcake array for collectibles
const int NUM_CUPCAKES = 5;
Model_OBJ model_cupcakes[NUM_CUPCAKES];
bool cupcakeVisible[NUM_CUPCAKES];

// Animation variables
float cupcakeRotation = 0.0f;

// Collision detection radius (smaller = harder to collect)
float collisionRadius = 1.2f;

// Textures
GLTexture tex_ground;
GLTexture tex_bmo;
GLTexture tex_cupcake;

//=======================================================================
// Lighting Configuration Function
//=======================================================================
void InitLightSource()
{
	// Enable Lighting for this OpenGL Program
	glEnable(GL_LIGHTING);

	// Enable Light Source number 0
	// OpengL has 8 light sources
	glEnable(GL_LIGHT0);

	// Define Light source 0 ambient light
	GLfloat ambient[] = { 0.1f, 0.1f, 0.1, 1.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

	// Define Light source 0 diffuse light
	GLfloat diffuse[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

	// Define Light source 0 Specular light
	GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

	// Finally, define light source 0 position in World Space
	GLfloat light_position[] = { 0.0f, 10.0f, 0.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
}

//=======================================================================
// Material Configuration Function
//======================================================================
void InitMaterial()
{
	// Enable Material Tracking
	glEnable(GL_COLOR_MATERIAL);

	// Sich will be assigneet Material Properties whd by glColor
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	// Set Material's Specular Color
	// Will be applied to all objects
	GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);

	// Set Material's Shine value (0->128)
	GLfloat shininess[] = { 96.0f };
	glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
}

//=======================================================================
// OpengGL Configuration Function
//=======================================================================
void myInit(void)
{
	glClearColor(0.0, 0.0, 0.0, 0.0);

	glMatrixMode(GL_PROJECTION);

	glLoadIdentity();

	gluPerspective(fovy, aspectRatio, zNear, zFar);
	//*******************************************************************************************//
	// fovy:			Angle between the bottom and top of the projectors, in degrees.			 //
	// aspectRatio:		Ratio of width to height of the clipping plane.							 //
	// zNear and zFar:	Specify the front and back clipping planes distances from camera.		 //
	//*******************************************************************************************//

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();

	gluLookAt(Eye.x, Eye.y, Eye.z, At.x, At.y, At.z, Up.x, Up.y, Up.z);
	//*******************************************************************************************//
	// EYE (ex, ey, ez): defines the location of the camera.									 //
	// AT (ax, ay, az):	 denotes the direction where the camera is aiming at.					 //
	// UP (ux, uy, uz):  denotes the upward orientation of the camera.							 //
	//*******************************************************************************************//

	InitLightSource();

	InitMaterial();

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_NORMALIZE);
}

//=======================================================================
// Render Ground Function
//=======================================================================
void RenderGround()
{
	glDisable(GL_LIGHTING);	// Disable lighting 

	glColor3f(0.6, 0.6, 0.6);	// Dim the ground texture a bit

	glEnable(GL_TEXTURE_2D);	// Enable 2D texturing

	glBindTexture(GL_TEXTURE_2D, tex_ground.texture[0]);	// Bind the ground texture

	glPushMatrix();
	glBegin(GL_QUADS);
	glNormal3f(0, 1, 0);	// Set quad normal direction.
	glTexCoord2f(0, 0);		// Set tex coordinates ( Using (0,0) -> (5,5) with texture wrapping set to GL_REPEAT to simulate the ground repeated grass texture).
	glVertex3f(-20, 0, -20);
	glTexCoord2f(5, 0);
	glVertex3f(20, 0, -20);
	glTexCoord2f(5, 5);
	glVertex3f(20, 0, 20);
	glTexCoord2f(0, 5);
	glVertex3f(-20, 0, 20);
	glEnd();
	glPopMatrix();

	glEnable(GL_LIGHTING);	// Enable lighting again for other entites coming throung the pipeline.

	glColor3f(1, 1, 1);	// Set material back to white instead of grey used for the ground texture.
}

//=======================================================================
// Collision Detection Functions
//=======================================================================
bool CheckJellyCollision(float newX, float newZ)
{	
	float jellyRadius = 1.35f; // Collision radius matches jelly's actual visible size
	
	// Calculate distance between proposed BMO position and jelly
	float dx = newX - model_jelly.pos_x;
	float dz = newZ - model_jelly.pos_z;
	float distance = sqrt(dx * dx + dz * dz);
	
	// Return true if collision detected
	return (distance < jellyRadius);
}

void CheckCupcakeCollisions()
{
	for (int i = 0; i < NUM_CUPCAKES; i++)
	{
		if (!cupcakeVisible[i]) continue;
		
		// Calculate distance between BMO and cupcake
		float dx = model_bmo.pos_x - model_cupcakes[i].pos_x;
		float dz = model_bmo.pos_z - model_cupcakes[i].pos_z;
		float distance = sqrt(dx * dx + dz * dz);
		
		// Check if collision occurred
		if (distance < collisionRadius)
		{
			cupcakeVisible[i] = false;
			printf("Cupcake %d collected!\n", i + 1);
		}
	}
}

//=======================================================================
// Display Function
//=======================================================================
void myDisplay(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Update camera based on mode
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	if (currentCamera == FIRST_PERSON)
	{
		// First person - camera is at BMO's position, looking forward
		gluLookAt(
			model_bmo.pos_x, model_bmo.pos_y +2.0f, model_bmo.pos_z, // Eye at BMO's position + height offset
			model_bmo.pos_x + sin(model_bmo.rot_y *3.14159 /180.0), model_bmo.pos_y +2.0f, model_bmo.pos_z - cos(model_bmo.rot_y *3.14159 /180.0), // Look in direction BMO is facing
			0,1,0 // Up vector
		);
	}
	else // THIRD_PERSON
	{
		// Third person - camera behind and above BMO (follow BMO)
		float camDistance = 15.0f;
		float camHeight = 8.0f;
		float angle = model_bmo.rot_y * 3.14159265f / 180.0f;

		// Calculate the direction BMO is facing
		float forwardX = sinf(angle);
		float forwardZ = -cosf(angle);

		// Position camera behind BMO (opposite of forward direction)
		float camX = model_bmo.pos_x - forwardX * camDistance;
		float camY = model_bmo.pos_y + camHeight;
		float camZ = model_bmo.pos_z - forwardZ * camDistance;

		// Look at point slightly above BMO
		float targetX = model_bmo.pos_x;
		float targetY = model_bmo.pos_y + 2.0f;
		float targetZ = model_bmo.pos_z;

		gluLookAt(
			camX, camY, camZ,    // Camera position (behind BMO)
			targetX, targetY, targetZ,  // Look at BMO
			0.0f, 1.0f, 0.0f          // Up vector
		);
	}

	GLfloat lightIntensity[] = {0.7,0.7,0.7,1.0f };
	GLfloat lightPosition[] = {0.0f,100.0f,0.0f,0.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightIntensity);

	model_candy_kingdom.Draw();
	
	// Don't draw BMO in first person mode (since camera is at BMO's eyes)
	if (currentCamera != FIRST_PERSON)
	{
		model_bmo.Draw();
	}
	
	model_finn.Draw();
	
	// Draw rotating cupcakes (only if visible)
	for (int i = 0; i < NUM_CUPCAKES; i++)
	{
		if (cupcakeVisible[i])
		{
			glPushMatrix();
			glTranslatef(model_cupcakes[i].pos_x, model_cupcakes[i].pos_y, model_cupcakes[i].pos_z);
			glRotatef(cupcakeRotation, 0.0f, 1.0f, 0.0f); // Rotate around Y axis
			glTranslatef(-model_cupcakes[i].pos_x, -model_cupcakes[i].pos_y, -model_cupcakes[i].pos_z);
			model_cupcakes[i].Draw();
			glPopMatrix();
		}
	}
	
	model_coin.Draw();
	//model_lich.Draw();
	
	// Draw Donut
	glPushMatrix();
	model_donut.Draw();
	glPopMatrix();

	model_jelly.Draw();

	glutSwapBuffers();
}

//=======================================================================
// Keyboard Function
//=======================================================================
void myKeyboard(unsigned char button, int x, int y)
{
	float moveSpeed = 2.0f;
	float rotSpeed = 5.0f;
	float angle = model_bmo.rot_y * 3.14159 / 180.0;
	
	switch (button)
	{
	case 'w':
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		break;
	case 'r':
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	
	// Camera switch
	case 'c':
	case 'C':
		currentCamera = (currentCamera == FIRST_PERSON) ? THIRD_PERSON : FIRST_PERSON;
		printf("Camera switched to %s\n", currentCamera == FIRST_PERSON ? "First Person" : "Third Person");
		break;
	
	// BMO movement (IJKL) - rotate BMO to face movement direction
	case 'i':
	case 'I':
		// Move forward in current direction
		{
			float newX = model_bmo.pos_x + sin(angle) * moveSpeed;
			float newZ = model_bmo.pos_z - cos(angle) * moveSpeed;
			if (!CheckJellyCollision(newX, newZ)) {
				model_bmo.pos_x = newX;
				model_bmo.pos_z = newZ;
				CheckCupcakeCollisions();
			}
		}
		break;
	case 'k':
	case 'K':
		// Move backward - rotate 180° to face backward, move, rotate back
		{
			model_bmo.rot_y += 180.0f;
			angle = model_bmo.rot_y * 3.14159 / 180.0;
			float newX = model_bmo.pos_x + sin(angle) * moveSpeed;
			float newZ = model_bmo.pos_z - cos(angle) * moveSpeed;
			model_bmo.rot_y -= 180.0f;
			if (!CheckJellyCollision(newX, newZ)) {
				model_bmo.pos_x = newX;
				model_bmo.pos_z = newZ;
				CheckCupcakeCollisions();
			}
		}
		break;
	case 'j':
	case 'J':
		// Move left - rotate 90° left, move forward, rotate back
		{
			model_bmo.rot_y -= 90.0f;
			angle = model_bmo.rot_y * 3.14159 / 180.0;
			float newX = model_bmo.pos_x + sin(angle) * moveSpeed;
			float newZ = model_bmo.pos_z - cos(angle) * moveSpeed;
			model_bmo.rot_y += 90.0f;
			if (!CheckJellyCollision(newX, newZ)) {
				model_bmo.pos_x = newX;
				model_bmo.pos_z = newZ;
				CheckCupcakeCollisions();
			}
		}
		break;
	case 'l':
	case 'L':
		// Move right - rotate 90° right, move forward, rotate back
		{
			model_bmo.rot_y += 90.0f;
			angle = model_bmo.rot_y * 3.14159 / 180.0;
			float newX = model_bmo.pos_x + sin(angle) * moveSpeed;
			float newZ = model_bmo.pos_z - cos(angle) * moveSpeed;
			model_bmo.rot_y -= 90.0f;
			if (!CheckJellyCollision(newX, newZ)) {
				model_bmo.pos_x = newX;
				model_bmo.pos_z = newZ;
				CheckCupcakeCollisions();
			}
		}
		break;
	
	// Rotate BMO permanently
	case 'u':
	case 'U':
		model_bmo.rot_y -= rotSpeed;
		break;
	case 'o':
	case 'O':
		model_bmo.rot_y += rotSpeed;
		break;
		
	case 27:
		exit(0);
		break;
	default:
		break;
	}

	glutPostRedisplay();
}

//=======================================================================
// Special Keys Function (Arrow keys for movement)
//=======================================================================
void mySpecialKeys(int key, int x, int y)
{
	float moveSpeed = 2.0f;
	float rotSpeed = 5.0f;
	float angle = model_bmo.rot_y * 3.14159 / 180.0;
	
	switch (key)
	{
	case GLUT_KEY_UP:
		// Move forward in current direction
		{
			float newX = model_bmo.pos_x + sin(angle) * moveSpeed;
			float newZ = model_bmo.pos_z - cos(angle) * moveSpeed;
			if (!CheckJellyCollision(newX, newZ)) {
				model_bmo.pos_x = newX;
				model_bmo.pos_z = newZ;
				CheckCupcakeCollisions();
			}
		}
		break;
	case GLUT_KEY_DOWN:
		// Move backward - rotate 180°, move, rotate back
		{
			model_bmo.rot_y += 180.0f;
			angle = model_bmo.rot_y * 3.14159 / 180.0;
			float newX = model_bmo.pos_x + sin(angle) * moveSpeed;
			float newZ = model_bmo.pos_z - cos(angle) * moveSpeed;
			model_bmo.rot_y -= 180.0f;
			if (!CheckJellyCollision(newX, newZ)) {
				model_bmo.pos_x = newX;
				model_bmo.pos_z = newZ;
				CheckCupcakeCollisions();
			}
		}
		break;
	case GLUT_KEY_LEFT:
		// Rotate left permanently
		model_bmo.rot_y -= rotSpeed;
		break;
	case GLUT_KEY_RIGHT:
		// Rotate right permanently
		model_bmo.rot_y += rotSpeed;
		break;
	}
	
	glutPostRedisplay();
}

//=======================================================================
// Motion Function
//=======================================================================
void myMotion(int x, int y)
{
	y = HEIGHT - y;

	if (cameraZoom - y > 0)
	{
		Eye.x += -0.1;
		Eye.z += -0.1;
	}
	else
	{
		Eye.x += 0.1;
		Eye.z += 0.1;
	}

	cameraZoom = y;

	glLoadIdentity();	//Clear Model_View Matrix

	gluLookAt(Eye.x, Eye.y, Eye.z, At.x, At.y, At.z, Up.x, Up.y, Up.z);	//Setup Camera with modified paramters

	GLfloat light_position[] = { 0.0f, 10.0f, 0.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glutPostRedisplay();	//Re-draw scene 
}

//=======================================================================
// Mouse Function
//=======================================================================
void myMouse(int button, int state, int x, int y)
{
	y = HEIGHT - y;

	if (state == GLUT_DOWN)
	{
		cameraZoom = y;
	}
}

//=======================================================================
// Reshape Function
//=======================================================================
void myReshape(int w, int h)
{
	if (h == 0) {
		h = 1;
	}

	WIDTH = w;
	HEIGHT = h;

	// set the drawable region of the window
	glViewport(0, 0, w, h);

	// set up the projection matrix 
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fovy, (GLdouble)WIDTH / (GLdouble)HEIGHT, zNear, zFar);

	// go back to modelview matrix so we can move the objects about
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(Eye.x, Eye.y, Eye.z, At.x, At.y, At.z, Up.x, Up.y, Up.z);
}

//=======================================================================
// Assets Loading Function
//=======================================================================
void LoadAssets()
{
	// --- CANDY KINGDOM ---
	printf("Loading OBJ Model: Candy Kingdom...\n");
	model_candy_kingdom.Load("Models/candy/candyKingdom.obj", "Models/candy/");
	model_candy_kingdom.scale_xyz = 300.0f;
	printf("Candy Kingdom Loaded.\n");

	// --- BMO (With Texture Fix) ---
	printf("Loading OBJ Model: BMO...\n");
	model_bmo.Load("Models/bmo/BIMO.obj", "Models/bmo/");

	// 1. Load the BMP manually
	printf("Loading BMO Texture...\n");
	tex_bmo.Load("Textures/bimo.bmp");

	// 2. Force assign this texture to the model
	for (auto& entry : model_bmo.materials) {
		entry.second.tex = tex_bmo;       // Assign the BMP
		entry.second.hasTexture = true;   // Turn on texturing
		// Force white color so texture isn't tinted
		entry.second.diffColor[0] = 1.0f;
		entry.second.diffColor[1] = 1.0f;
		entry.second.diffColor[2] = 1.0f;
	}
	// 3. Update the GPU
	model_bmo.GenerateDisplayList();

	// Positioning
	model_bmo.scale_xyz = 10.0f;
	model_bmo.pos_x = 65.0f;
	model_bmo.rot_y = -90.0f;
	printf("BMO Ready.\n");


	// --- CUPCAKE (With Texture Fix) ---
	printf("Loading OBJ Model: Cupcakes...\n");
	
	// Load and configure cupcake texture once
	printf("Loading Cupcake Texture...\n");
	tex_cupcake.Load("Textures/cupcake.bmp");
	
	// Create multiple cupcakes at different positions
	float cupcakeSpacing = 15.0f; // Distance between cupcakes
	float startZ = 60.0f;         // Start further out due to larger candy kingdom
	
	for (int i = 0; i < NUM_CUPCAKES; i++)
	{
		model_cupcakes[i].Load("Models/cupcake/cupcake.obj", "Models/cupcake/");
		
		// Force assign texture to each cupcake
		for (auto& entry : model_cupcakes[i].materials) {
			entry.second.tex = tex_cupcake;
			entry.second.hasTexture = true;
			entry.second.diffColor[0] = 1.0f;
			entry.second.diffColor[1] = 1.0f;
			entry.second.diffColor[2] = 1.0f;
		}
		
		// Update the GPU
		model_cupcakes[i].GenerateDisplayList();
		
		// Position cupcakes in a line
		model_cupcakes[i].scale_xyz = 50.0f;
		model_cupcakes[i].pos_x = 65.0f;
		model_cupcakes[i].pos_y = 0.5f;  // Slightly lifted from ground
		model_cupcakes[i].pos_z = startZ + (i * cupcakeSpacing);
        // Initialize as visible
        cupcakeVisible[i] = true;  // <-- ADD THIS LINE

		printf("Cupcake %d positioned at Z = %.1f\n", i + 1, model_cupcakes[i].pos_z);
	}
	
	printf("All Cupcakes Ready.\n");


	// --- COIN ---
	printf("Loading OBJ Model: Coin...\n");
	model_coin.Load("Models/coin/coin.obj", "Models/coin/");
	
	// Position coin next to Finn so it's visible
	model_coin.scale_xyz = 150.0f;        // Smaller scale for visibility
	model_coin.pos_x = 65.0f;           // Same X as Finn and BMO
	model_coin.pos_y = 1.0f;            // Raised above ground
	model_coin.pos_z = 12.0f;           // A bit further from Finn
	
	model_coin.GenerateDisplayList();
	printf("Coin Loaded.\n");


	// --- FINN (Uncomment if needed) ---
	printf("Loading OBJ Model: Finn...\n");
	model_finn.Load("Models/finn/Finn.obj", "Models/finn/");
	
	// Positioning - place Finn next to BMO (smaller size)
	model_finn.scale_xyz = 0.1f;   // Half the size of BMO (BMO is 10.0f)
	model_finn.pos_x = 65.0f;      // Same X as BMO
	model_finn.pos_y = 0.0f;
	model_finn.pos_z = 8.0f; // Closer to BMO (standing right next to him)
	model_finn.rot_y = -90.0f;   // Face same direction as BMO
				
	model_finn.GenerateDisplayList();
	printf("Finn Ready.\n");

	// --- DONUT ---
	printf("Loading 3DS Model: Donut...\n");
	model_donut.Load("models/donut/donut.3ds");
	model_donut.scale = 5.0f;  // Adjust scale as needed
	model_donut.pos.x = 50.0f;  // Position it visible in scene
	model_donut.pos.y = 5.0f;
	model_donut.pos.z = 0.0f;
	printf("Donut Loaded.\n");

	// --- JELLY (Non-collidable obstacle) ---
	printf("Loading OBJ Model: Jelly...\n");
	model_jelly.Load("models/jelly/jelly.obj", "models/jelly/");
	model_jelly.scale_xyz = 30.0f;
	model_jelly.pos_x = 80.0f;  // To the right of cupcake row (cupcakes at X=65)
	model_jelly.pos_y = 0.0f;   // On the ground
	model_jelly.pos_z = 50.0f;  // Adjusted for larger candy kingdom
	model_jelly.GenerateDisplayList();
	printf("Jelly Ready (Non-collidable obstacle).\n");

	// --- OTHER TEXTURES ---
	//tex_ground.Load("Textures/ground.bmp");
	//loadBMP(&tex, "Textures/blu-sky-3.bmp", true);
}

//=======================================================================
// Animation/Idle Function
//=======================================================================
void myIdle(void)
{
	// Update cupcake rotation
	cupcakeRotation += 1.0f;
	if (cupcakeRotation >= 360.0f)
		cupcakeRotation = 0.0f;
	
	glutPostRedisplay();
}

//=======================================================================
// Main Function
//=======================================================================
void main(int argc, char** argv)
{
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	glutInitWindowSize(WIDTH, HEIGHT);

	glutInitWindowPosition(100, 150);

	glutCreateWindow(title);
	printf("Window Created.\n");

	glutDisplayFunc(myDisplay);

	glutKeyboardFunc(myKeyboard);
	
	glutSpecialFunc(mySpecialKeys);  // Add special keys handler
	
	glutIdleFunc(myIdle);  // Add idle function for animation

	glutMotionFunc(myMotion);

	glutMouseFunc(myMouse);

	glutReshapeFunc(myReshape);

	myInit();
	printf("Init Done.\n");

	LoadAssets();
	printf("Entering Main Loop.\n");
	printf("\n=== CONTROLS ===\n");
	printf("Arrow Keys / IJKL: Move BMO\n");
	printf("U/O: Rotate BMO left/right\n");
	printf("C: Switch between First Person (BMO's view) and Third Person (behind Finn)\n");
	printf("W: Wireframe mode\n");
	printf("R: Fill mode\n");
	printf("ESC: Exit\n");
	printf("================\n\n");
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);

	glShadeModel(GL_SMOOTH);

	glutMainLoop();
}
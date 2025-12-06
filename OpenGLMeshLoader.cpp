#define GLUT_DISABLE_ATEXIT_HACK
#include "TextureBuilder.h"
#include "Model_3DS.h"
#include "Model_OBJ.h"
#include "GLTexture.h"
#include <glut.h>
#include <math.h>
#include <stdio.h>

int WIDTH = 1280;
int HEIGHT = 720;

GLuint tex;
char title[] = "3D Model Loader Sample";

// 3D Projection Options
GLdouble fovy = 45.0;
GLdouble aspectRatio = (GLdouble)WIDTH / (GLdouble)HEIGHT;
GLdouble zNear = 0.1;
GLdouble zFar = 1000;

class Vector
{
public:
	GLdouble x, y, z;
	Vector() {}
	Vector(GLdouble _x, GLdouble _y, GLdouble _z) : x(_x), y(_y), z(_z) {}
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

// Debugging Camera Height
float cameraHeightOffset = 0.0f;

// Model Variables
Model_OBJ model_donut;
Model_OBJ model_candy_kingdom;
Model_OBJ model_bmo;
Model_OBJ model_finn;
Model_OBJ model_cupcake;

// --- JELLY VARIABLES ---
Model_OBJ model_jelly;
GLTexture tex_jelly;
Model_OBJ model_lich;

// Cupcake array for collectibles
const int NUM_CUPCAKES = 5;
Model_OBJ model_cupcakes[NUM_CUPCAKES];
bool cupcakeVisible[NUM_CUPCAKES];

// --- COIN VARIABLES ---
const int NUM_COINS = 5;
Model_OBJ model_coins[NUM_COINS];
bool coinVisible[NUM_COINS];

// Coin Positions - SCATTERED AROUND THE JELLY (Center: 80, 0, 50)
float coinPositions[NUM_COINS][3] = {
	{ 80.0f, 2.0f, 65.0f },   // Front of Jelly
	{ 95.0f, 2.0f, 50.0f },   // Right of Jelly
	{ 80.0f, 2.0f, 35.0f },   // Back of Jelly
	{ 65.0f, 2.0f, 50.0f },   // Left of Jelly
	{ 80.0f, 8.0f, 50.0f }    // High above the Jelly!
};

// Animation variables
float cupcakeRotation = 0.0f;
float coinRotation = 0.0f;
float coinBounceAngle = 0.0f; // For up/down movement

// Collision detection radius
float collisionRadius = 1.2f;

// Textures
GLTexture tex_ground;
GLTexture tex_bmo;
GLTexture tex_cupcake;
GLTexture tex_coin;

//=======================================================================
// Lighting Configuration Function
//=======================================================================
void InitLightSource()
{
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	GLfloat ambient[] = { 0.1f, 0.1f, 0.1, 1.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

	GLfloat diffuse[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

	GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

	GLfloat light_position[] = { 0.0f, 10.0f, 0.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
}

//=======================================================================
// Material Configuration Function
//======================================================================
void InitMaterial()
{
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);

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
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(Eye.x, Eye.y, Eye.z, At.x, At.y, At.z, Up.x, Up.y, Up.z);
	InitLightSource();
	InitMaterial();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
}

//=======================================================================
// Collision Detection Functions
//=======================================================================
bool CheckJellyCollision(float newX, float newZ)
{
	float jellyRadius = 1.35f;
	float dx = newX - model_jelly.pos_x;
	float dz = newZ - model_jelly.pos_z;
	float distance = sqrt(dx * dx + dz * dz);
	return (distance < jellyRadius);
}

void CheckCupcakeCollisions()
{
	for (int i = 0; i < NUM_CUPCAKES; i++)
	{
		if (!cupcakeVisible[i]) continue;

		float dx = model_bmo.pos_x - model_cupcakes[i].pos_x;
		float dz = model_bmo.pos_z - model_cupcakes[i].pos_z;
		float distance = sqrt(dx * dx + dz * dz);

		if (distance < collisionRadius)
		{
			cupcakeVisible[i] = false;
			printf("Cupcake %d collected!\n", i + 1);
		}
	}
}

// --- CHECK MULTIPLE COINS ---
void CheckCoinCollision()
{
	float coinCollisionRadius = 3.0f; // Increased slightly for easier collection

	for (int i = 0; i < NUM_COINS; i++)
	{
		if (!coinVisible[i]) continue;

		float dx = model_bmo.pos_x - coinPositions[i][0];
		// We ignore Y height difference for basic collection so you can catch the high one
		// or you can require a jump if jumping was implemented. 
		// For now, infinite vertical collision cylinder.
		float dz = model_bmo.pos_z - coinPositions[i][2];
		float distance = sqrt(dx * dx + dz * dz);

		if (distance < coinCollisionRadius)
		{
			coinVisible[i] = false;
			printf("Coin %d Collected! +10 Points\n", i + 1);
		}
	}
}

//=======================================================================
// Display Function
//=======================================================================
void myDisplay(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (currentCamera == FIRST_PERSON)
	{
		gluLookAt(
			model_bmo.pos_x, model_bmo.pos_y + 2.0f + cameraHeightOffset, model_bmo.pos_z,
			model_bmo.pos_x + sin(model_bmo.rot_y * 3.14159 / 180.0), model_bmo.pos_y + 2.0f + cameraHeightOffset, model_bmo.pos_z - cos(model_bmo.rot_y * 3.14159 / 180.0),
			0, 1, 0
		);
	}
	else
	{
		float camDistance = 15.0f;
		float camHeight = 8.0f + cameraHeightOffset;

		float angle = model_bmo.rot_y * 3.14159265f / 180.0f;
		float forwardX = sinf(angle);
		float forwardZ = -cosf(angle);
		float camX = model_bmo.pos_x - forwardX * camDistance;
		float camY = model_bmo.pos_y + camHeight;
		float camZ = model_bmo.pos_z - forwardZ * camDistance;
		float targetX = model_bmo.pos_x;
		float targetY = model_bmo.pos_y + 2.0f;
		float targetZ = model_bmo.pos_z;

		gluLookAt(camX, camY, camZ, targetX, targetY, targetZ, 0.0f, 1.0f, 0.0f);
	}

	GLfloat lightIntensity[] = { 0.7,0.7,0.7,1.0f };
	GLfloat lightPosition[] = { 0.0f,100.0f,0.0f,0.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightIntensity);

	model_candy_kingdom.Draw();

	if (currentCamera != FIRST_PERSON)
	{
		model_bmo.Draw();
	}

	model_finn.Draw();

	for (int i = 0; i < NUM_CUPCAKES; i++)
	{
		if (cupcakeVisible[i])
		{
			glPushMatrix();
			glTranslatef(model_cupcakes[i].pos_x, model_cupcakes[i].pos_y, model_cupcakes[i].pos_z);
			glRotatef(cupcakeRotation, 0.0f, 1.0f, 0.0f);
			glTranslatef(-model_cupcakes[i].pos_x, -model_cupcakes[i].pos_y, -model_cupcakes[i].pos_z);
			model_cupcakes[i].Draw();
			glPopMatrix();
		}
	}

	// ============================================
	// --- DRAW MULTIPLE COINS ---
	// ============================================

	// Force Texture State for Coins
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_coin.texture[0]);
	glColor3f(1.0f, 1.0f, 1.0f);

	for (int i = 0; i < NUM_COINS; i++)
	{
		if (coinVisible[i])
		{
			glPushMatrix();

			// BOUNCE CALCULATION
			float bounceHeight = 0.5f * sin(coinBounceAngle);

			// Positions from Array
			float ox = coinPositions[i][0];
			float oy = coinPositions[i][1];
			float oz = coinPositions[i][2];

			// 1. Move to coin location + bounce
			glTranslatef(ox, oy + bounceHeight, oz);

			// 2. Rotate in place
			glRotatef(coinRotation, 0.0f, 1.0f, 0.0f);

			// 3. Draw Model at (0,0,0) relative to this new matrix
			model_coins[i].pos_x = 0;
			model_coins[i].pos_y = 0;
			model_coins[i].pos_z = 0;

			model_coins[i].Draw();

			glPopMatrix();
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	// --- DRAW JELLY ---
	glPushMatrix();
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_jelly.texture[0]);
	glColor3f(1.0f, 1.0f, 1.0f);
	model_jelly.Draw();
	glBindTexture(GL_TEXTURE_2D, 0);
	glPopMatrix();

	glPushMatrix();
	model_donut.Draw();
	glPopMatrix();

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
	case 'c': case 'C':
		currentCamera = (currentCamera == FIRST_PERSON) ? THIRD_PERSON : FIRST_PERSON;
		break;
		// BMO movement
	case 'i': case 'I':
	{
		float newX = model_bmo.pos_x + sin(angle) * moveSpeed;
		float newZ = model_bmo.pos_z - cos(angle) * moveSpeed;
		if (!CheckJellyCollision(newX, newZ)) {
			model_bmo.pos_x = newX;
			model_bmo.pos_z = newZ;
			CheckCupcakeCollisions();
			CheckCoinCollision();
		}
	}
	break;
	case 'k': case 'K':
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
			CheckCoinCollision();
		}
	}
	break;
	case 'j': case 'J':
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
			CheckCoinCollision();
		}
	}
	break;
	case 'l': case 'L':
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
			CheckCoinCollision();
		}
	}
	break;
	case 'u': case 'U':
		model_bmo.rot_y -= rotSpeed;
		break;
	case 'o': case 'O':
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
// Special Keys Function (Arrow keys)
//=======================================================================
void mySpecialKeys(int key, int x, int y)
{
	float moveSpeed = 2.0f;
	float rotSpeed = 5.0f;
	float angle = model_bmo.rot_y * 3.14159 / 180.0;

	switch (key)
	{
	case GLUT_KEY_UP:
	{
		float newX = model_bmo.pos_x + sin(angle) * moveSpeed;
		float newZ = model_bmo.pos_z - cos(angle) * moveSpeed;
		if (!CheckJellyCollision(newX, newZ)) {
			model_bmo.pos_x = newX;
			model_bmo.pos_z = newZ;
			CheckCupcakeCollisions();
			CheckCoinCollision();
		}
	}
	break;
	case GLUT_KEY_DOWN:
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
			CheckCoinCollision();
		}
	}
	break;
	case GLUT_KEY_LEFT:
		model_bmo.rot_y -= rotSpeed;
		break;
	case GLUT_KEY_RIGHT:
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
	glLoadIdentity();
	gluLookAt(Eye.x, Eye.y, Eye.z, At.x, At.y, At.z, Up.x, Up.y, Up.z);
	GLfloat light_position[] = { 0.0f, 10.0f, 0.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glutPostRedisplay();
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
	if (h == 0) h = 1;
	WIDTH = w;
	HEIGHT = h;
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fovy, (GLdouble)WIDTH / (GLdouble)HEIGHT, zNear, zFar);
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

	// --- BMO ---
	printf("Loading OBJ Model: BMO...\n");
	model_bmo.Load("Models/bmo/BIMO.obj", "Models/bmo/");
	tex_bmo.Load("Textures/bimo.bmp");
	for (auto& entry : model_bmo.materials) {
		entry.second.tex = tex_bmo;
		entry.second.hasTexture = true;
		entry.second.diffColor[0] = 1.0f;
		entry.second.diffColor[1] = 1.0f;
		entry.second.diffColor[2] = 1.0f;
	}
	model_bmo.GenerateDisplayList();

	model_bmo.scale_xyz = 10.0f;
	model_bmo.pos_x = 65.0f;
	model_bmo.pos_z = 55.0f;
	model_bmo.rot_y = -90.0f;
	printf("BMO Ready.\n");

	// --- CUPCAKE ---
	printf("Loading OBJ Model: Cupcakes...\n");
	tex_cupcake.Load("Textures/cupcake.bmp");

	float cupcakeSpacing = 15.0f;
	float startZ = 60.0f;

	for (int i = 0; i < NUM_CUPCAKES; i++)
	{
		model_cupcakes[i].Load("Models/cupcake/cupcake.obj", "Models/cupcake/");
		for (auto& entry : model_cupcakes[i].materials) {
			entry.second.tex = tex_cupcake;
			entry.second.hasTexture = true;
			entry.second.diffColor[0] = 1.0f;
			entry.second.diffColor[1] = 1.0f;
			entry.second.diffColor[2] = 1.0f;
		}
		model_cupcakes[i].GenerateDisplayList();
		model_cupcakes[i].scale_xyz = 50.0f;
		model_cupcakes[i].pos_x = 65.0f;
		model_cupcakes[i].pos_y = 0.5f;
		model_cupcakes[i].pos_z = startZ + (i * cupcakeSpacing);
		cupcakeVisible[i] = true;
	}
	printf("All Cupcakes Ready.\n");

	// --- COINS ---
	printf("Loading OBJ Model: Coins...\n");
	tex_coin.Load("Textures/coin.bmp");

	// Load 5 instances
	for (int i = 0; i < NUM_COINS; i++) {
		model_coins[i].Load("Models/coin/coin.obj", "Models/coin/");

		// Apply Texture
		for (auto& entry : model_coins[i].materials) {
			entry.second.tex = tex_coin;
			entry.second.hasTexture = true;
			entry.second.diffColor[0] = 1.0f;
			entry.second.diffColor[1] = 1.0f;
			entry.second.diffColor[2] = 1.0f;
		}

		model_coins[i].scale_xyz = 2.0f;

		// Initial positions (doesn't affect display as display uses array)
		model_coins[i].pos_x = coinPositions[i][0];
		model_coins[i].pos_y = coinPositions[i][1];
		model_coins[i].pos_z = coinPositions[i][2];

		model_coins[i].GenerateDisplayList();

		coinVisible[i] = true;
	}
	printf("Coins Loaded.\n");

	// --- FINN ---
	printf("Loading OBJ Model: Finn...\n");
	model_finn.Load("Models/finn/Finn.obj", "Models/finn/");

	model_finn.scale_xyz = 0.1f;
	model_finn.pos_x = 70.0f;
	model_finn.pos_y = 0.0f;
	model_finn.pos_z = 53.0f;
	model_finn.rot_y = -90.0f;

	model_finn.GenerateDisplayList();
	printf("Finn Ready.\n");

	// --- DONUT ---
	printf("Loading OBJ Model: Donut...\n");
	model_donut.Load("models/donut/donut.obj", "models/donut/");
	model_donut.scale_xyz = 50.0f;
	model_donut.pos_x = model_finn.pos_x + 5.0f;
	model_donut.pos_y = model_finn.pos_y;
	model_donut.pos_z = model_finn.pos_z + 2.0f;
	model_donut.rot_y = 0.0f;
	model_donut.GenerateDisplayList();
	printf("Donut Loaded.\n");

	// --- JELLY ---
	printf("Loading OBJ Model: Jelly...\n");
	tex_jelly.Load("Textures/jelly.bmp");
	model_jelly.Load("Models/jelly/jelly.obj", "Models/jelly/");

	for (auto& entry : model_jelly.materials) {
		entry.second.hasTexture = false;
		entry.second.diffColor[0] = 1.0f;
		entry.second.diffColor[1] = 0.4f;
		entry.second.diffColor[2] = 0.1f;
	}

	model_jelly.GenerateDisplayList();

	model_jelly.scale_xyz = 30.0f;
	model_jelly.pos_x = 80.0f;
	model_jelly.pos_y = 0.0f;
	model_jelly.pos_z = 50.0f;

	printf("Jelly Loaded.\n");
}

//=======================================================================
// Animation/Idle Function
//=======================================================================
void myIdle(void)
{
	cupcakeRotation += 1.0f;
	if (cupcakeRotation >= 360.0f)
		cupcakeRotation = 0.0f;

	// SPIN THE COIN
	coinRotation += 2.0f;
	if (coinRotation >= 360.0f)
		coinRotation = 0.0f;

	// BOUNCE THE COIN
	coinBounceAngle += 0.1f;

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
	glutSpecialFunc(mySpecialKeys);
	glutIdleFunc(myIdle);
	glutMotionFunc(myMotion);
	glutMouseFunc(myMouse);
	glutReshapeFunc(myReshape);

	myInit();
	printf("Init Done.\n");

	LoadAssets();
	printf("Entering Main Loop.\n");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glShadeModel(GL_SMOOTH);

	glutMainLoop();
}
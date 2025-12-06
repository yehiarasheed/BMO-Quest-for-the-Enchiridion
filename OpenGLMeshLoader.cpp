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

// Jump state and movement
bool isJumping = false;
float jumpVelocity = 0.0f;
const float jumpStrength = 0.6f;
const float gravity = 0.03f;

// Mouse control
int lastMouseX = -1;
int lastMouseY = -1;
bool mouseRotationEnabled = false;
float mouseSensitivity = 0.2f;

// FPS-style mouse look
bool mouseLookEnabled = true;
int centerX = WIDTH / 2;
int centerY = HEIGHT / 2;
bool firstMouse = true;
// Camera pitch (vertical look)
float cameraPitch = 0.0f; // degrees, positive = look up
const float pitchLimit = 45.0f; // clamp up/down
// Score
int score = 0;
const int CUPCAKE_POINTS = 10;
const int COIN_POINTS = 5;

// Textures
GLTexture tex_ground;
GLTexture tex_bmo;
GLTexture tex_cupcake;
GLTexture tex_coin;

// Render HUD (score)
void RenderHUD()
{
	// Save matrices
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, WIDTH, 0, HEIGHT);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// Draw background box
	int boxW = 220;
	int boxH = 40;
	int margin = 10;
	int x = margin;
	int y = HEIGHT - boxH - margin;

	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	// semi-transparent background
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
	glBegin(GL_QUADS);
	glVertex2i(x, y);
	glVertex2i(x + boxW, y);
	glVertex2i(x + boxW, y + boxH);
	glVertex2i(x, y + boxH);
	glEnd();
	glDisable(GL_BLEND);

	// Draw score text
	char buf[64];
	sprintf(buf, "Score: %d", score);
	// white text
	glColor3f(1.0f, 1.0f, 1.0f);
	glRasterPos2i(x + 12, y + 12);
	for (char* c = buf; *c != '\0'; ++c)
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

	// restore states
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

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
			score += CUPCAKE_POINTS; // cupcakes worth more
			printf("Cupcake %d collected! Score: %d\n", i + 1, score);
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
			score += COIN_POINTS;
			printf("Coin %d Collected! +%d Points\n", i + 1, COIN_POINTS);
		}
	}
}

// Try to move BMO to new position; if collides with jelly, apply bounce
bool TryMove(float newX, float newZ)
{
	if (CheckJellyCollision(newX, newZ))
	{
		// Bounce back from jelly
		float dx = model_bmo.pos_x - model_jelly.pos_x;
		float dz = model_bmo.pos_z - model_jelly.pos_z;
		float len = sqrt(dx * dx + dz * dz);
		float nx = 0.0f, nz = -1.0f;
		if (len > 0.001f) { nx = dx / len; nz = dz / len; }

		float pushBack = 2.0f;
		model_bmo.pos_x = model_jelly.pos_x + nx * (pushBack + 1.0f);
		model_bmo.pos_z = model_jelly.pos_z + nz * (pushBack + 1.0f);

		// vertical bounce
		if (!isJumping) {
			isJumping = true;
			jumpVelocity = jumpStrength * 0.9f;
		}
		else if (jumpVelocity < 0.0f) {
			jumpVelocity = jumpStrength * 1.8f;
		}

		return false;
	}

	model_bmo.pos_x = newX;
	model_bmo.pos_z = newZ;
	return true;
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
		// Compute forward vector from yaw (rot_y) and pitch (cameraPitch)
		float yawRad = model_bmo.rot_y * 3.14159265f / 180.0f;
		float pitchRad = cameraPitch * 3.14159265f / 180.0f;
		float fx = sinf(yawRad) * cosf(pitchRad);
		float fy = sinf(pitchRad);
		float fz = -cosf(yawRad) * cosf(pitchRad);
		gluLookAt(
			model_bmo.pos_x, model_bmo.pos_y + 2.0f, model_bmo.pos_z,
			model_bmo.pos_x + fx, model_bmo.pos_y + 2.0f + fy, model_bmo.pos_z + fz,
			0, 1, 0
		);
	}
	else
	{
		float camDistance = 15.0f;
		float camHeight = 8.0f;
		// Include pitch in third-person camera offset
		float yawRad = model_bmo.rot_y * 3.14159265f / 180.0f;
		float pitchRad = cameraPitch * 3.14159265f / 180.0f;
		float forwardX = sinf(yawRad) * cosf(pitchRad);
		float forwardY = sinf(pitchRad);
		float forwardZ = -cosf(yawRad) * cosf(pitchRad);
		float camX = model_bmo.pos_x - forwardX * camDistance;
		float camY = model_bmo.pos_y + camHeight + forwardY * camDistance;
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
		glPushMatrix();

		// 1. Enable Texturing
		glEnable(GL_TEXTURE_2D);

		// 2. Reset color to white so the texture isn't dark or tinted
		glColor3f(1.0f, 1.0f, 1.0f);

		// 3. Force bind BMO's specific texture
		glBindTexture(GL_TEXTURE_2D, tex_bmo.texture[0]);

		// 4. Draw the model
		model_bmo.Draw();

		// 5. Unbind to stay safe
		glBindTexture(GL_TEXTURE_2D, 0);

		glPopMatrix();
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

	// Draw HUD on top
	RenderHUD();

	glutSwapBuffers();
}

//=======================================================================
// Keyboard Function (WASD + Space Jump + Sprint)
//=======================================================================
void myKeyboard(unsigned char button, int x, int y)
{
	float moveSpeed = 2.0f;
	float rotSpeed = 5.0f;
	float angle = model_bmo.rot_y * 3.14159 / 180.0;

	// Sprint when Shift is held
	int mods = glutGetModifiers();
	if (mods & GLUT_ACTIVE_SHIFT) moveSpeed *= 1.8f;

	switch (button)
	{
	case 'z': case 'Z':
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		break;
	case 'x': case 'X':
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	case 'c': case 'C':
		currentCamera = (currentCamera == FIRST_PERSON) ? THIRD_PERSON : FIRST_PERSON;
		break;

		// WASD controls
	case 'w': case 'W':
	{
		float newX = model_bmo.pos_x + sin(angle) * moveSpeed;
		float newZ = model_bmo.pos_z - cos(angle) * moveSpeed;
		if (TryMove(newX, newZ)) {
			CheckCupcakeCollisions();
			CheckCoinCollision();
		}
	}
	break;
	case 's': case 'S':
	{
		float backAngle = model_bmo.rot_y + 180.0f;
		float a = backAngle * 3.14159 / 180.0;
		float newX = model_bmo.pos_x + sin(a) * moveSpeed;
		float newZ = model_bmo.pos_z - cos(a) * moveSpeed;
		if (TryMove(newX, newZ)) {
			CheckCupcakeCollisions();
			CheckCoinCollision();
		}
	}
	break;
	case 'a': case 'A':
	{
		float strafeAngle = model_bmo.rot_y - 90.0f;
		float sAngle = strafeAngle * 3.14159 / 180.0;
		float newX = model_bmo.pos_x + sin(sAngle) * moveSpeed;
		float newZ = model_bmo.pos_z - cos(sAngle) * moveSpeed;
		if (TryMove(newX, newZ)) {
			CheckCupcakeCollisions();
			CheckCoinCollision();
		}
	}
	break;
	case 'd': case 'D':
	{
		float strafeAngle = model_bmo.rot_y + 90.0f;
		float sAngle = strafeAngle * 3.14159 / 180.0;
		float newX = model_bmo.pos_x + sin(sAngle) * moveSpeed;
		float newZ = model_bmo.pos_z - cos(sAngle) * moveSpeed;
		if (TryMove(newX, newZ)) {
			CheckCupcakeCollisions();
			CheckCoinCollision();
		}
	}
	break;
	case ' ': // Spacebar jump
		if (!isJumping) {
			isJumping = true;
			jumpVelocity = jumpStrength;
		}
		break;
	case 'u': case 'U':
		model_bmo.rot_y -= rotSpeed;
		break;
	case 'o': case 'O':
		model_bmo.rot_y += rotSpeed;
		break;
	case 'p': case 'P':
		printf("BMO: pos=(%.2f, %.2f, %.2f) rot=%.2f\n", model_bmo.pos_x, model_bmo.pos_y, model_bmo.pos_z, model_bmo.rot_y);
		printf("Score: %d\n", score);
		break;
	case 'q': case 'Q':
		exit(0);
		break;
	case 27: // ESC
		mouseLookEnabled = !mouseLookEnabled;
		if (mouseLookEnabled) {
			glutSetCursor(GLUT_CURSOR_NONE);
			glutWarpPointer(centerX, centerY);
			firstMouse = true;
		}
		else {
			glutSetCursor(GLUT_CURSOR_INHERIT);
		}
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
	// Treat arrows as movement (Up/Down = forward/backward, Left/Right = strafe)
	// Check for Shift modifier (sprint)
	int mods = glutGetModifiers();
	if (mods & GLUT_ACTIVE_SHIFT) moveSpeed *= 1.8f;

	switch (key)
	{
	case GLUT_KEY_UP:
	{
		float newX = model_bmo.pos_x + sin(angle) * moveSpeed;
		float newZ = model_bmo.pos_z - cos(angle) * moveSpeed;
		if (TryMove(newX, newZ)) {
			CheckCupcakeCollisions();
			CheckCoinCollision();
		}
	}
	break;
	case GLUT_KEY_DOWN:
	{
		float backAngle = model_bmo.rot_y + 180.0f;
		float a = backAngle * 3.14159 / 180.0;
		float newX = model_bmo.pos_x + sin(a) * moveSpeed;
		float newZ = model_bmo.pos_z - cos(a) * moveSpeed;
		if (TryMove(newX, newZ)) {
			CheckCupcakeCollisions();
			CheckCoinCollision();
		}
	}
	break;
	case GLUT_KEY_LEFT:
	{
		// Strafe left
		float strafeAngle = model_bmo.rot_y - 90.0f;
		float sAngle = strafeAngle * 3.14159 / 180.0;
		float newX = model_bmo.pos_x + sin(sAngle) * moveSpeed;
		float newZ = model_bmo.pos_z - cos(sAngle) * moveSpeed;
		if (TryMove(newX, newZ)) {
			CheckCupcakeCollisions();
			CheckCoinCollision();
		}
	}
	break;
	case GLUT_KEY_RIGHT:
	{
		// Strafe right
		float strafeAngle = model_bmo.rot_y + 90.0f;
		float sAngle = strafeAngle * 3.14159 / 180.0;
		float newX = model_bmo.pos_x + sin(sAngle) * moveSpeed;
		float newZ = model_bmo.pos_z - cos(sAngle) * moveSpeed;
		if (TryMove(newX, newZ)) {
			CheckCupcakeCollisions();
			CheckCoinCollision();
		}
	}
	break;
	}
	glutPostRedisplay();
}

//=======================================================================
// Motion Function (Mouse Movement - FPS Style)
//=======================================================================
void myMotion(int x, int y)
{
	if (!mouseLookEnabled) return;

	if (firstMouse) {
		lastMouseX = x;
		lastMouseY = y;
		firstMouse = false;
		return;
	}

	// Calculate mouse movement delta from center
	int deltaX = x - centerX;
	int deltaY = y - centerY;

	// Only update if mouse moved significantly from center
	if (abs(deltaX) > 1 || abs(deltaY) > 1) {
		// Update yaw (horizontal)
		model_bmo.rot_y -= deltaX * mouseSensitivity;
		// Update pitch (vertical) and clamp
		cameraPitch += -deltaY * mouseSensitivity; // moving mouse down -> look down
		if (cameraPitch > pitchLimit) cameraPitch = pitchLimit;
		if (cameraPitch < -pitchLimit) cameraPitch = -pitchLimit;

		// Normalize rotation to 0-360 range
		while (model_bmo.rot_y > 360.0f) model_bmo.rot_y -= 360.0f;
		while (model_bmo.rot_y < 0.0f) model_bmo.rot_y += 360.0f;

		// Re-center cursor for continuous rotation
		glutWarpPointer(centerX, centerY);
	}

	glutPostRedisplay();
}

//=======================================================================
// Passive Motion Function (Mouse Look when not clicking)
//=======================================================================
void myPassiveMotion(int x, int y)
{
	// Call the same motion handler for FPS-style look
	myMotion(x, y);
}

//=======================================================================
// Mouse Function (Click Handlers)
//=======================================================================
void myMouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		// Left click = Jump
		if (!isJumping) {
			isJumping = true;
			jumpVelocity = jumpStrength;
			printf("Jump! (mouse)\n");
		}
	}
	else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
	{
		// Right click = Toggle camera
		currentCamera = (currentCamera == FIRST_PERSON) ? THIRD_PERSON : FIRST_PERSON;
		printf("Camera switched to %s\n", currentCamera == FIRST_PERSON ? "First Person" : "Third Person");
	}
	else if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN)
	{
		// Middle button = Toggle mouse look on/off
		mouseLookEnabled = !mouseLookEnabled;
		if (mouseLookEnabled) {
			glutSetCursor(GLUT_CURSOR_NONE); // Hide cursor
			glutWarpPointer(centerX, centerY);
			firstMouse = true;
			printf("Mouse look enabled\n");
		}
		else {
			glutSetCursor(GLUT_CURSOR_INHERIT); // Show cursor
			printf("Mouse look disabled\n");
		}
	}

	glutPostRedisplay();
}

//=======================================================================
// Reshape Function
//=======================================================================
void myReshape(int w, int h)
{
	if (h == 0) h = 1;
	WIDTH = w;
	HEIGHT = h;

	// Update center point for mouse look
	centerX = WIDTH / 2;
	centerY = HEIGHT / 2;

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
	// Jump physics
	if (isJumping) {
		model_bmo.pos_y += jumpVelocity;
		jumpVelocity -= gravity;
		if (model_bmo.pos_y <= 0.0f) {
			model_bmo.pos_y = 0.0f;
			isJumping = false;
			jumpVelocity = 0.0f;
		}
	}

	// Cupcake rotation animation
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
	glutPassiveMotionFunc(myPassiveMotion); // FPS-style mouse look
	glutMouseFunc(myMouse);
	glutReshapeFunc(myReshape);

	// Hide cursor for FPS experience
	glutSetCursor(GLUT_CURSOR_NONE);
	glutWarpPointer(centerX, centerY);

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
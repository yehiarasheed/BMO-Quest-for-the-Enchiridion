#define GLUT_DISABLE_ATEXIT_HACK
#include "TextureBuilder.h"
#include "Model_3DS.h"
#include "Model_OBJ.h"
#include "GLTexture.h"
#include <glut.h>
#include <math.h>
#include <stdio.h>
#include <string>

int WIDTH = 1280;
int HEIGHT = 720;

GLuint tex;
char title[] = "BMO's Quest";

// 3D Projection Options
GLdouble fovy = 45.0;
GLdouble aspectRatio = (GLdouble)WIDTH / (GLdouble)HEIGHT;
GLdouble zNear = 0.1;
// Increased zFar so we can see the ground when falling from the sky
GLdouble zFar = 3000;

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

// --- LEVEL MANAGEMENT ---
enum GameLevel { LEVEL_CANDY, LEVEL_FIRE };
GameLevel currentLevel = LEVEL_CANDY;

// Debugging Camera Height
float cameraHeightOffset = 0.0f;

// Model Variables
Model_OBJ model_candy_kingdom;
Model_OBJ model_candy_cane;
Model_OBJ model_bmo;
Model_OBJ model_finn;
Model_OBJ model_cupcake;

// --- JELLY VARIABLES ---
Model_OBJ model_jelly;
GLTexture tex_jelly;

// --- DONUT VARIABLES ---
Model_OBJ model_donut;
GLTexture tex_donut;
// Animation for Donut
float donutShakeAngle = 0.0f;

// --- FIRE KINGDOM VARIABLES ---
Model_OBJ model_fire_temple;
GLTexture tex_fire_temple;

// --- GOLEM VARIABLES ---
Model_OBJ model_golem;
GLTexture tex_golem_em_map;
GLTexture tex_golem_lava_eye;
GLTexture tex_golem_norma;
GLTexture tex_golem_ao;
GLTexture tex_golem_podstavka;
GLTexture tex_golem_final;

// --- FLAME PRINCESS VARIABLES ---
Model_OBJ model_flame_princess;
GLTexture tex_flame_princess;

// --- FIRE ROCK VARIABLES ---
Model_OBJ model_fire_rock;
GLTexture tex_fire_rock_20;
GLTexture tex_fire_rock_0;

// --- ENCHIRIDION VARIABLES ---
Model_OBJ model_enchiridion;
GLTexture tex_enchiridion_01;
GLTexture tex_enchiridion_02;
GLTexture tex_enchiridion_paper;

// --- SKY VARIABLES ---
Model_OBJ model_sky;
GLTexture tex_sky;

// Cupcake array for collectibles
const int NUM_CUPCAKES = 5;
Model_OBJ model_cupcakes[NUM_CUPCAKES];
bool cupcakeVisible[NUM_CUPCAKES];

// --- COIN VARIABLES ---
const int NUM_COINS = 5;
Model_OBJ model_coins[NUM_COINS];
bool coinVisible[NUM_COINS];
Model_OBJ model_coin;

// Coin Positions
float coinPositions[NUM_COINS][3] = {
	{ 80.0f, 2.0f, 65.0f },
	{ 95.0f, 2.0f, 50.0f },
	{ 80.0f, 2.0f, 35.0f },
	{ 65.0f, 2.0f, 50.0f },
	{ 80.0f, 8.0f, 50.0f }
};

// Animation variables
float cupcakeRotation = 0.0f;
float coinRotation = 0.0f;
float coinBounceAngle = 0.0f;

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
float cameraPitch = 0.0f;
const float pitchLimit = 45.0f;
// Score
int score = 0;
const int CUPCAKE_POINTS = 10;
const int COIN_POINTS = 5;

// Textures
GLTexture tex_ground;
GLTexture tex_bmo;
GLTexture tex_cupcake;
GLTexture tex_coin;
GLTexture tex_finn;
GLTexture tex_candy_cane;

// Render HUD (score)
void RenderHUD()
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, WIDTH, 0, HEIGHT);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	int boxW = 220;
	int boxH = 40;
	int margin = 10;
	int x = margin;
	int y = HEIGHT - boxH - margin;

	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

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

	char buf[64];
	sprintf(buf, "Score: %d | Level: %s", score, (currentLevel == LEVEL_CANDY ? "Candy" : "Fire"));
	glColor3f(1.0f, 1.0f, 1.0f);
	glRasterPos2i(x + 12, y + 12);
	for (char* c = buf; *c != '\0'; ++c)
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

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

void InitMaterial()
{
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);

	GLfloat shininess[] = { 96.0f };
	glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
}

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

bool CheckJellyCollision(float newX, float newZ)
{
	if (currentLevel != LEVEL_CANDY) return false;

	float jellyRadius = 1.35f;
	float dx = newX - model_jelly.pos_x;
	float dz = newZ - model_jelly.pos_z;
	float distance = sqrt(dx * dx + dz * dz);
	return (distance < jellyRadius);
}

// --- DONUT OBSTACLE LOGIC ---
bool CheckDonutCollision(float newX, float newZ)
{
	if (currentLevel != LEVEL_CANDY) return false;

	float donutRadius = 2.0f; // Radius for collision
	float dx = newX - model_donut.pos_x;
	float dz = newZ - model_donut.pos_z;
	float distance = sqrt(dx * dx + dz * dz);
	return (distance < donutRadius);
}

void CheckCupcakeCollisions()
{
	if (currentLevel != LEVEL_CANDY) return;

	for (int i = 0; i < NUM_CUPCAKES; i++)
	{
		if (!cupcakeVisible[i]) continue;

		float dx = model_bmo.pos_x - model_cupcakes[i].pos_x;
		float dz = model_bmo.pos_z - model_cupcakes[i].pos_z;
		float distance = sqrt(dx * dx + dz * dz);

		if (distance < collisionRadius)
		{
			cupcakeVisible[i] = false;
			score += CUPCAKE_POINTS;
			printf("Cupcake %d collected! Score: %d\n", i + 1, score);
		}
	}
}

void CheckCoinCollision()
{
	if (currentLevel != LEVEL_CANDY) return;

	float coinCollisionRadius = 3.0f;

	for (int i = 0; i < NUM_COINS; i++)
	{
		if (!coinVisible[i]) continue;

		float dx = model_bmo.pos_x - coinPositions[i][0];
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

// --- FINN COLLISION / LEVEL TRANSITION ---
void CheckFinnCollision()
{
	if (currentLevel != LEVEL_CANDY) return;

	float finnRadius = 2.0f;
	float dx = model_bmo.pos_x - model_finn.pos_x;
	float dz = model_bmo.pos_z - model_finn.pos_z;
	float distance = sqrt(dx * dx + dz * dz);

	if (distance < finnRadius)
	{
		printf(">>> TRAVELING TO FIRE KINGDOM! <<<\n");
		currentLevel = LEVEL_FIRE;

		// Place BMO on the Fire Kingdom ground - FINAL TESTED VALUES
		model_bmo.pos_x = -111.0f;   // Final X position from testing
		model_bmo.pos_z = 2416.1f;   // Final Z position from testing
		model_bmo.pos_y = 0.0f;      // Ground level

		// Apply final rotation values - TESTED ORIENTATION
		model_bmo.rot_x = -240.0f;   // X-axis rotation (pitch)
		model_bmo.rot_y = 329.0f;    // Y-axis rotation (yaw/facing)
		model_bmo.rot_z = 240.0f;    // Z-axis rotation (roll)

		// Ensure physics state is stable on arrival
		isJumping = false;
		jumpVelocity = 0.0f;
	}
}

bool TryMove(float newX, float newZ)
{
	// 1. Check Jelly Obstacle
	if (CheckJellyCollision(newX, newZ))
	{
		float dx = model_bmo.pos_x - model_jelly.pos_x;
		float dz = model_bmo.pos_z - model_jelly.pos_z;
		float len = sqrt(dx * dx + dz * dz);
		float nx = 0.0f, nz = -1.0f;
		if (len > 0.001f) { nx = dx / len; nz = dz / len; }

		float pushBack = 2.0f;
		model_bmo.pos_x = model_jelly.pos_x + nx * (pushBack + 1.0f);
		model_bmo.pos_z = model_jelly.pos_z + nz * (pushBack + 1.0f);

		if (!isJumping) {
			isJumping = true;
			jumpVelocity = jumpStrength * 0.9f;
		}
		else if (jumpVelocity < 0.0f) {
			jumpVelocity = jumpStrength * 1.8f;
		}
		return false;
	}

	// 2. Check Donut Obstacle (Similar bounce logic)
	if (CheckDonutCollision(newX, newZ))
	{
		float dx = model_bmo.pos_x - model_donut.pos_x;
		float dz = model_bmo.pos_z - model_donut.pos_z;
		float len = sqrt(dx * dx + dz * dz);
		float nx = 0.0f, nz = -1.0f;
		if (len > 0.001f) { nx = dx / len; nz = dz / len; }

		float pushBack = 2.5f; // Push slightly harder than jelly
		model_bmo.pos_x = model_donut.pos_x + nx * (pushBack + 1.5f);
		model_bmo.pos_z = model_donut.pos_z + nz * (pushBack + 1.5f);

		// Bounce the player up
		if (!isJumping) {
			isJumping = true;
			jumpVelocity = jumpStrength * 0.9f;
		}
		else if (jumpVelocity < 0.0f) {
			jumpVelocity = jumpStrength * 1.8f;
		}
		printf("Bonk! You hit the Donut.\n");
		return false;
	}

	model_bmo.pos_x = newX;
	model_bmo.pos_z = newZ;
	return true;
}

void myDisplay(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (currentCamera == FIRST_PERSON)
	{
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

	// ============================================
	// RENDER ENVIRONMENT BASED ON LEVEL
	// ============================================

	if (currentLevel == LEVEL_CANDY)
	{
		// --- DRAW SKY (Background) ---
		glPushMatrix();
		glDisable(GL_LIGHTING);  // Sky doesn't need lighting
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 1.0f);
		
		// Bind sky texture if available
		if (tex_sky.texture[0] > 0) {
			glBindTexture(GL_TEXTURE_2D, tex_sky.texture[0]);
		}
		
		// Position sky centered on the camera/world
		glTranslatef(model_sky.pos_x, model_sky.pos_y, model_sky.pos_z);
		model_sky.Draw();
		
		glBindTexture(GL_TEXTURE_2D, 0);
		glEnable(GL_LIGHTING);  // Re-enable lighting for other objects
		glPopMatrix();
		
		// Draw Candy Kingdom
		model_candy_kingdom.Draw();

		// Candy Cane
		glPushMatrix();
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, tex_candy_cane.texture[0]);
		model_candy_cane.Draw();
		glBindTexture(GL_TEXTURE_2D, 0);
		glPopMatrix();

		// Finn (The Portal)
		glPushMatrix();
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, tex_finn.texture[0]);
		model_finn.Draw();
		glBindTexture(GL_TEXTURE_2D, 0);
		glPopMatrix();

		// Cupcakes
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

		// Donut
		glPushMatrix();
		glColor3f(1.0f, 1.0f, 1.0f);
		float donutBounce = 0.3f * sin(donutShakeAngle);
		float donutWiggle = 5.0f * cos(donutShakeAngle * 2.0f);
		float ox = model_donut.pos_x;
		float oy = model_donut.pos_y;
		float oz = model_donut.pos_z;
		glTranslatef(ox, oy + donutBounce, oz);
		glRotatef(model_donut.rot_y, 0, 1, 0);
		glRotatef(donutWiggle, 0, 0, 1);
		model_donut.pos_x = 0;
		model_donut.pos_y = 0;
		model_donut.pos_z = 0;
		model_donut.rot_y = 0;
		model_donut.Draw();
		model_donut.pos_x = ox;
		model_donut.pos_y = oy;
		model_donut.pos_z = oz;
		model_donut.rot_y = 45.0f;
		glPopMatrix();

		// Coins
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex_coin.texture[0]);
		glColor3f(1.0f, 1.0f, 1.0f);
		for (int i = 0; i < NUM_COINS; i++)
		{
			if (coinVisible[i])
			{
				glPushMatrix();
				float bounceHeight = 0.5f * sin(coinBounceAngle);
				float cx = coinPositions[i][0];
				float cy = coinPositions[i][1];
				float cz = coinPositions[i][2];
				glTranslatef(cx, cy + bounceHeight, cz);
				glRotatef(coinRotation, 0.0f, 1.0f, 0.0f);
				model_coins[i].pos_x = 0;
				model_coins[i].pos_y = 0;
				model_coins[i].pos_z = 0;
				model_coins[i].Draw();
				glPopMatrix();
			}
		}
		glBindTexture(GL_TEXTURE_2D, 0);

		// Jelly
		glPushMatrix();
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex_jelly.texture[0]);
		glColor3f(1.0f, 1.0f, 1.0f);
		model_jelly.Draw();
		glBindTexture(GL_TEXTURE_2D, 0);
		glPopMatrix();
	}
	else if (currentLevel == LEVEL_FIRE)
	{
		// --- DRAW SKY (Background) ---
		glPushMatrix();
		glDisable(GL_LIGHTING);  // Sky doesn't need lighting
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 1.0f);
		
		// Bind sky texture if available
		if (tex_sky.texture[0] > 0) {
			glBindTexture(GL_TEXTURE_2D, tex_sky.texture[0]);
		}
		
		// Position sky centered on the camera/world
		glTranslatef(model_sky.pos_x, model_sky.pos_y, model_sky.pos_z);
		model_sky.Draw();
		
		glBindTexture(GL_TEXTURE_2D, 0);
		glEnable(GL_LIGHTING);  // Re-enable lighting for other objects
		glPopMatrix();
		
		// --- DRAW FIRE KINGDOM TEMPLE ---
		glPushMatrix();
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 1.0f);

		// Bind the Fire Temple Texture
		glBindTexture(GL_TEXTURE_2D, tex_fire_temple.texture[0]);

		// Apply transformations to flip it right-side up
		glTranslatef(model_fire_temple.pos_x, model_fire_temple.pos_y, model_fire_temple.pos_z);
		
		// Apply rotation - X-axis first (flip upside down)
		glRotatef(model_fire_temple.rot_x, 1.0f, 0.0f, 0.0f);  // X-axis rotation (180 degrees)
		glRotatef(model_fire_temple.rot_y, 0.0f, 1.0f, 0.0f);  // Y-axis rotation
		glRotatef(model_fire_temple.rot_z, 0.0f, 0.0f, 1.0f);  // Z-axis rotation
		
		// Temporarily reset position to origin for proper rotation
		float temp_x = model_fire_temple.pos_x;
		float temp_y = model_fire_temple.pos_y;
		float temp_z = model_fire_temple.pos_z;
		model_fire_temple.pos_x = 0.0f;
		model_fire_temple.pos_y = 0.0f;
		model_fire_temple.pos_z = 0.0f;

		model_fire_temple.Draw();

		// Restore position
		model_fire_temple.pos_x = temp_x;
		model_fire_temple.pos_y = temp_y;
		model_fire_temple.pos_z = temp_z;

		glBindTexture(GL_TEXTURE_2D, 0);
		glPopMatrix();
		
		// --- DRAW GOLEM ---
		glPushMatrix();
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 1.0f);
		
		// Apply Golem transformations
		glTranslatef(model_golem.pos_x, model_golem.pos_y, model_golem.pos_z);
		glRotatef(model_golem.rot_y, 0.0f, 1.0f, 0.0f);  // Y-axis (yaw)
		glRotatef(model_golem.rot_x, 1.0f, 0.0f, 0.0f);  // X-axis (pitch)
		glRotatef(model_golem.rot_z, 0.0f, 0.0f, 1.0f);  // Z-axis (roll)
		
		// Temporarily reset position for proper rendering
		float temp_golem_x = model_golem.pos_x;
		float temp_golem_y = model_golem.pos_y;
		float temp_golem_z = model_golem.pos_z;
		model_golem.pos_x = 0.0f;
		model_golem.pos_y = 0.0f;
		model_golem.pos_z = 0.0f;
		
		// Draw Golem (it will use its own material textures from the model)
		model_golem.Draw();
		
		// Restore position
		model_golem.pos_x = temp_golem_x;
		model_golem.pos_y = temp_golem_y;
		model_golem.pos_z = temp_golem_z;
		
		glPopMatrix();
		
		// --- DRAW FLAME PRINCESS ---
		glPushMatrix();
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 1.0f);
		
		// Apply Flame Princess transformations
		glTranslatef(model_flame_princess.pos_x, model_flame_princess.pos_y, model_flame_princess.pos_z);
		glRotatef(model_flame_princess.rot_y, 0.0f, 1.0f, 0.0f);  // Y-axis (yaw)
		glRotatef(model_flame_princess.rot_x, 1.0f, 0.0f, 0.0f);  // X-axis (pitch)
		glRotatef(model_flame_princess.rot_z, 0.0f, 0.0f, 1.0f);  // Z-axis (roll)
		
		// Temporarily reset position for proper rendering
		float temp_fp_x = model_flame_princess.pos_x;
		float temp_fp_y = model_flame_princess.pos_y;
		float temp_fp_z = model_flame_princess.pos_z;
		model_flame_princess.pos_x = 0.0f;
		model_flame_princess.pos_y = 0.0f;
		model_flame_princess.pos_z = 0.0f;
		
		// Draw Flame Princess
		model_flame_princess.Draw();
		
		// Restore position
		model_flame_princess.pos_x = temp_fp_x;
		model_flame_princess.pos_y = temp_fp_y;
		model_flame_princess.pos_z = temp_fp_z;
		
		glPopMatrix();
		
		// --- DRAW FIRE ROCK ---
		glPushMatrix();
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 1.0f);
		
		// Apply Fire Rock transformations
		glTranslatef(model_fire_rock.pos_x, model_fire_rock.pos_y, model_fire_rock.pos_z);
		glRotatef(model_fire_rock.rot_y, 0.0f, 1.0f, 0.0f);  // Y-axis (yaw)
		glRotatef(model_fire_rock.rot_x, 1.0f, 0.0f, 0.0f);  // X-axis (pitch)
		glRotatef(model_fire_rock.rot_z, 0.0f, 0.0f, 1.0f);  // Z-axis (roll)
		
		// Temporarily reset position for proper rendering
		float temp_rock_x = model_fire_rock.pos_x;
		float temp_rock_y = model_fire_rock.pos_y;
		float temp_rock_z = model_fire_rock.pos_z;
		model_fire_rock.pos_x = 0.0f;
		model_fire_rock.pos_y = 0.0f;
		model_fire_rock.pos_z = 0.0f;
		
		// Draw Fire Rock
		model_fire_rock.Draw();
		
		// Restore position
		model_fire_rock.pos_x = temp_rock_x;
		model_fire_rock.pos_y = temp_rock_y;
		model_fire_rock.pos_z = temp_rock_z;
		
		glPopMatrix();
		
		// --- DRAW ENCHIRIDION ---
		glPushMatrix();
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 1.0f);
		
		// Apply Enchiridion transformations
		glTranslatef(model_enchiridion.pos_x, model_enchiridion.pos_y, model_enchiridion.pos_z);
		glRotatef(model_enchiridion.rot_y, 0.0f, 1.0f, 0.0f);  // Y-axis (yaw)
		glRotatef(model_enchiridion.rot_x, 1.0f, 0.0f, 0.0f);  // X-axis (pitch)
		glRotatef(model_enchiridion.rot_z, 0.0f, 0.0f, 1.0f);  // Z-axis (roll)
		
		// Temporarily reset position for proper rendering
		float temp_ench_x = model_enchiridion.pos_x;
		float temp_ench_y = model_enchiridion.pos_y;
		float temp_ench_z = model_enchiridion.pos_z;
		model_enchiridion.pos_x = 0.0f;
		model_enchiridion.pos_y = 0.0f;
		model_enchiridion.pos_z = 0.0f;
		
		// Draw Enchiridion
		model_enchiridion.Draw();
		
		// Restore position
		model_enchiridion.pos_x = temp_ench_x;
		model_enchiridion.pos_y = temp_ench_y;
		model_enchiridion.pos_z = temp_ench_z;
		
		glPopMatrix();
	}

	// ============================================
	// DRAW BMO (Available in both levels)
	// ============================================
	if (currentCamera != FIRST_PERSON)
	{
		glPushMatrix();
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, tex_bmo.texture[0]);
		
		// Apply BMO transformations (position and rotation)
		glTranslatef(model_bmo.pos_x, model_bmo.pos_y, model_bmo.pos_z);
		glRotatef(model_bmo.rot_y, 0.0f, 1.0f, 0.0f);  // Y-axis (yaw)
		glRotatef(model_bmo.rot_x, 1.0f, 0.0f, 0.0f);  // X-axis (pitch)
		glRotatef(model_bmo.rot_z, 0.0f, 0.0f, 1.0f);  // Z-axis (roll)
		
		// Temporarily reset position to origin for proper rendering
		float temp_bmo_x = model_bmo.pos_x;
		float temp_bmo_y = model_bmo.pos_y;
		float temp_bmo_z = model_bmo.pos_z;
		float temp_bmo_rot_y = model_bmo.rot_y;
		
		model_bmo.pos_x = 0.0f;
		model_bmo.pos_y = 0.0f;
		model_bmo.pos_z = 0.0f;
		model_bmo.rot_y = 0.0f;
		
		model_bmo.Draw();
		
		// Restore BMO values
		model_bmo.pos_x = temp_bmo_x;
		model_bmo.pos_y = temp_bmo_y;
		model_bmo.pos_z = temp_bmo_z;
		model_bmo.rot_y = temp_bmo_rot_y;
		
		glBindTexture(GL_TEXTURE_2D, 0);
		glPopMatrix();
	}

	RenderHUD();
	glutSwapBuffers();
}

void myKeyboard(unsigned char button, int x, int y)
{
	float moveSpeed = 2.0f;
	float rotSpeed = 5.0f;
	float angle = model_bmo.rot_y * 3.14159 / 180.0;

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
	case 'w': case 'W':
	{
		float newX = model_bmo.pos_x + sin(angle) * moveSpeed;
		float newZ = model_bmo.pos_z - cos(angle) * moveSpeed;
		if (TryMove(newX, newZ)) {
			CheckCupcakeCollisions();
			CheckCoinCollision();
			CheckFinnCollision(); // Check for Level Transfer
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
			CheckFinnCollision();
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
			CheckFinnCollision();
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
			CheckFinnCollision();
		}
	}
	break;
	case ' ':
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
	case 27:
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

void mySpecialKeys(int key, int x, int y)
{
	float moveSpeed = 2.0f;
	float rotSpeed = 5.0f;
	float angle = model_bmo.rot_y * 3.14159 / 180.0;
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
			CheckFinnCollision();
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
			CheckFinnCollision();
		}
	}
	break;
	case GLUT_KEY_LEFT:
	{
		float strafeAngle = model_bmo.rot_y - 90.0f;
		float sAngle = strafeAngle * 3.14159 / 180.0;
		float newX = model_bmo.pos_x + sin(sAngle) * moveSpeed;
		float newZ = model_bmo.pos_z - cos(sAngle) * moveSpeed;
		if (TryMove(newX, newZ)) {
			CheckCupcakeCollisions();
			CheckCoinCollision();
			CheckFinnCollision();
		}
	}
	break;
	case GLUT_KEY_RIGHT:
	{
		float strafeAngle = model_bmo.rot_y + 90.0f;
		float sAngle = strafeAngle * 3.14159 / 180.0;
		float newX = model_bmo.pos_x + sin(sAngle) * moveSpeed;
		float newZ = model_bmo.pos_z - cos(sAngle) * moveSpeed;
		if (TryMove(newX, newZ)) {
			CheckCupcakeCollisions();
			CheckCoinCollision();
			CheckFinnCollision();
		}
	}
	break;
	}
	glutPostRedisplay();
}

void myMotion(int x, int y)
{
	if (!mouseRotationEnabled) return;

	if (firstMouse) {
		lastMouseX = x;
		lastMouseY = y;
		firstMouse = false;
		return;
	}

	int deltaX = x - centerX;
	int deltaY = y - centerY;

	if (abs(deltaX) > 1 || abs(deltaY) > 1) {
		model_bmo.rot_y -= deltaX * mouseSensitivity;
		cameraPitch += -deltaY * mouseSensitivity;
		if (cameraPitch > pitchLimit) cameraPitch = pitchLimit;
		if (cameraPitch < -pitchLimit) cameraPitch = -pitchLimit;

		while (model_bmo.rot_y > 360.0f) model_bmo.rot_y -= 360.0f;
		while (model_bmo.rot_y < 0.0f) model_bmo.rot_y += 360.0f;

		glutWarpPointer(centerX, centerY);
	}

	glutPostRedisplay();
}

void myPassiveMotion(int x, int y)
{
	myMotion(x, y);
}

void myMouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		if (!isJumping) {
			isJumping = true;
			jumpVelocity = jumpStrength;
			printf("Jump! (mouse)\n");
		}
	}
	else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
	{
		currentCamera = (currentCamera == FIRST_PERSON) ? THIRD_PERSON : FIRST_PERSON;
		printf("Camera switched to %s\n", currentCamera == FIRST_PERSON ? "First Person" : "Third Person");
	}
	else if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN)
	{
		mouseLookEnabled = !mouseLookEnabled;
		if (mouseLookEnabled) {
			glutSetCursor(GLUT_CURSOR_NONE);
			glutWarpPointer(centerX, centerY);
			firstMouse = true;
			printf("Mouse look enabled\n");
		}
		else {
			glutSetCursor(GLUT_CURSOR_INHERIT);
			printf("Mouse look disabled\n");
		}
	}

	glutPostRedisplay();
}

void myReshape(int w, int h)
{
	if (h == 0) h = 1;
	WIDTH = w;
	HEIGHT = h;

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

void LoadAssets()
{
	// --- CANDY KINGDOM ---
	printf("Loading OBJ Model: Candy Kingdom...\n");
	model_candy_kingdom.Load("Models/candy/candyKingdom.obj", "Models/candy/");
	model_candy_kingdom.scale_xyz = 300.0f;
	printf("Candy Kingdom Loaded.\n");

	// --- FIRE KINGDOM TEMPLE ---
	printf("Loading OBJ Model: Fire Kingdom Temple...\n");
	// Make sure your OBJ filename matches exactly what you have on disk
	model_fire_temple.Load("Models/firekingdom/temple.obj", "Models/firekingdom/");

	// Load the specific texture from the textures folder
	tex_fire_temple.Load("Textures/great-temple-of-the-eternal-fire_textured_u1_v1.bmp");

	// Apply texture to all materials in the fire temple model
	for (auto& entry : model_fire_temple.materials) {
		entry.second.tex = tex_fire_temple;
		entry.second.hasTexture = true;
		entry.second.diffColor[0] = 1.0f;
		entry.second.diffColor[1] = 1.0f;
		entry.second.diffColor[2] = 1.0f;
	}

	// Scale it BIG
	model_fire_temple.scale_xyz = 200.0f;

	// Position it - FINAL TESTED VALUES
	model_fire_temple.pos_x = 0.0f;
	model_fire_temple.pos_y = 595.0f;  // Final value from testing
	model_fire_temple.pos_z = 0.0f;
	
	// Rotate to flip it right-side up - CORRECT VALUE
	model_fire_temple.rot_x = -290.0f;  // Correct rotation value found through testing
	model_fire_temple.rot_y = 0.0f;
	model_fire_temple.rot_z = 0.0f;

	model_fire_temple.GenerateDisplayList();
	printf("Fire Temple Loaded.\n");

	// --- GOLEM ---
	printf("Loading OBJ Model: Golem...\n");
	model_golem.Load("Models/golem/golem.obj", "Models/golem/");
	
	// Load all Golem textures with new BMP filenames
	printf("Loading Golem textures...\n");
	tex_golem_final.Load("Textures/texturesgolem/six.bmp");
	printf("  - Main texture loaded (six.bmp)\n");
	tex_golem_lava_eye.Load("Textures/texturesgolem/two.bmp");
	printf("  - Lava eye texture loaded (two.bmp)\n");
	tex_golem_em_map.Load("Textures/texturesgolem/one.bmp");
	printf("  - EM map loaded (one.bmp)\n");
	tex_golem_norma.Load("Textures/texturesgolem/three.bmp");
	printf("  - Normal map loaded (three.bmp)\n");
	tex_golem_ao.Load("Textures/texturesgolem/four.bmp");
	printf("  - AO map loaded (four.bmp)\n");
	tex_golem_podstavka.Load("Textures/texturesgolem/five.bmp");
	printf("  - Base texture loaded (five.bmp)\n");
	
	// Debug: Print all material names
	printf("Golem materials found:\n");
	for (auto& entry : model_golem.materials) {
		printf("  - Material: %s\n", entry.first.c_str());
	}
	
	// Apply textures to Golem materials based on material names
	for (auto& entry : model_golem.materials) {
		std::string materialName = entry.first;
		
		// Apply appropriate texture based on material name
		if (materialName.find("Eye") != std::string::npos || 
		    materialName.find("eye") != std::string::npos ||
		    materialName.find("Lava") != std::string::npos ||
		    materialName.find("lava") != std::string::npos) {
			entry.second.tex = tex_golem_lava_eye;
			entry.second.hasTexture = true;
			printf("  - Applied lava eye texture to: %s\n", materialName.c_str());
		}
		else if (materialName.find("Podstavka") != std::string::npos || 
		  materialName.find("podstavka") != std::string::npos ||
		 materialName.find("Base") != std::string::npos ||
		      materialName.find("base") != std::string::npos) {
			entry.second.tex = tex_golem_podstavka;
			entry.second.hasTexture = true;
			printf("  - Applied base texture to: %s\n", materialName.c_str());
		}
		else {
			// Use main texture for body
			entry.second.tex = tex_golem_final;
			entry.second.hasTexture = true;
			printf("  - Applied main texture to: %s\n", materialName.c_str());
		}
		
		// Ensure proper color for texture display
		entry.second.diffColor[0] = 1.0f;
		entry.second.diffColor[1] = 1.0f;
		entry.second.diffColor[2] = 1.0f;
	}
	
	// Set Golem size MUCH smaller than BMO (BMO is 10.0, make Golem 2.0)
	model_golem.scale_xyz = 0.5f;  // Made even smaller (was 2.0f, now 0.5f)
	
	// Position Golem next to BMO in Fire Kingdom
	model_golem.pos_x = -115.0f;  // Near BMO's spawn position (-111.0)
	model_golem.pos_y = 0.0f;     // Ground level
	model_golem.pos_z = 2418.0f;  // Near BMO's spawn position (2416.1)
	
	// Rotate Golem to face BMO/temple - simplified rotation
	model_golem.rot_x = 0.0f;     // No pitch rotation
	model_golem.rot_y = 180.0f;   // Face forward
	model_golem.rot_z = 0.0f;     // No roll rotation
	
	model_golem.GenerateDisplayList();
	printf("Golem Loaded with textures.\n");

	// --- FLAME PRINCESS ---
	printf("Loading OBJ Model: fire Princess...\n");
	model_flame_princess.Load("Models/firePrincess/firePrincess.obj", "Models/firePrincess/");
	
	//// Load Flame Princess texture (assuming it exists in the textures folder)
	//printf("Loading Flame Princess texture...\n");
	//tex_flame_princess.Load("Textures/flameprincess.bmp");
	
	// Apply texture to all materials
	for (auto& entry : model_flame_princess.materials) {
		entry.second.tex = tex_flame_princess;
		entry.second.hasTexture = true;
		entry.second.diffColor[0] = 1.0f;
		entry.second.diffColor[1] = 1.0f;
		entry.second.diffColor[2] = 1.0f;
	}
	
	// Set Flame Princess to same size as Golem
	model_flame_princess.scale_xyz = 0.5f;  // Same as Golem
	
	// Position Flame Princess next to Golem in Fire Kingdom
	model_flame_princess.pos_x = -115.0f;  // Next to Golem
	model_flame_princess.pos_y = 0.0f;     // Ground level
	model_flame_princess.pos_z = 2422.0f;  // Slightly offset from Golem (Golem is at 2418.0)
	
	// Rotate Flame Princess to face forward
	model_flame_princess.rot_x = 0.0f;
	model_flame_princess.rot_y = 180.0f;  // Face forward
	model_flame_princess.rot_z = 0.0f;
	
	model_flame_princess.GenerateDisplayList();
	printf("Flame Princess Loaded.\n");

	// --- FIRE ROCK ---
	printf("Loading OBJ Model: Fire Rock...\n");
	model_fire_rock.Load("Models/firerock/firerock.obj", "Models/firerock/");
	
	// Load Fire Rock textures
	printf("Loading Fire Rock textures...\n");
	tex_fire_rock_20.Load("Textures/texturesrock/rock20_tex00.bmp");
	printf("  - Rock 20 texture loaded\n");
	tex_fire_rock_0.Load("Textures/texturesrock/rock0_tex00.bmp");
	printf("  - Rock 0 texture loaded\n");
	
	// Apply textures to Fire Rock materials based on material names
	for (auto& entry : model_fire_rock.materials) {
		std::string materialName = entry.first;
		
		// Apply appropriate texture based on material name
		if (materialName.find("20") != std::string::npos || 
		materialName.find("Rock20") != std::string::npos ||
		    materialName.find("rock20") != std::string::npos) {
			entry.second.tex = tex_fire_rock_20;
			entry.second.hasTexture = true;
			printf("  - Applied rock20 texture to: %s\n", materialName.c_str());
		}
		else if (materialName.find("0") != std::string::npos || 
		         materialName.find("Rock0") != std::string::npos ||
		         materialName.find("rock0") != std::string::npos) {
			entry.second.tex = tex_fire_rock_0;
			entry.second.hasTexture = true;
			printf("  - Applied rock0 texture to: %s\n", materialName.c_str());
		}
		else {
			// Default to rock20 texture
			entry.second.tex = tex_fire_rock_20;
			entry.second.hasTexture = true;
			printf("  - Applied default rock20 texture to: %s\n", materialName.c_str());
		}
		
		// Ensure proper color for texture display
		entry.second.diffColor[0] = 1.0f;
		entry.second.diffColor[1] = 1.0f;
		entry.second.diffColor[2] = 1.0f;
	}
	
	// Set Fire Rock size (similar to Golem)
	model_fire_rock.scale_xyz = 1.0f;  // Increased from 0.5f to 2.0f for better visibility
	
	// Position Fire Rock next to Golem in Fire Kingdom
	model_fire_rock.pos_x = -118.0f;  // Slightly left of Golem
	model_fire_rock.pos_y = 0.0f;     // Ground level
	model_fire_rock.pos_z = 2414.0f;  // Slightly in front of Golem
	
	// Rotate Fire Rock
	model_fire_rock.rot_x = 0.0f;
	model_fire_rock.rot_y = 45.0f;    // Angled
	model_fire_rock.rot_z = 0.0f;
	
	model_fire_rock.GenerateDisplayList();
	printf("Fire Rock Loaded.\n");

	// --- ENCHIRIDION ---
	printf("Loading OBJ Model: Enchiridion...\n");
	model_enchiridion.Load("Models/enchiridion/enchiridion.obj", "Models/enchiridion/");
	
	// Load Enchiridion textures
	printf("Loading Enchiridion textures...\n");
	tex_enchiridion_01.Load("Textures/texturesenchiridion/enchiridion_tex_map_01.bmp");
	printf("  - Enchiridion texture 01 loaded\n");
	tex_enchiridion_02.Load("Textures/texturesenchiridion/enchiridion_tex_map_02.bmp");
	printf("  - Enchiridion texture 02 loaded\n");
	tex_enchiridion_paper.Load("Textures/texturesenchiridion/LT_AntiquePaper_03.bmp");
	printf("  - Antique paper texture loaded\n");
	
	// Apply textures to Enchiridion materials based on material names
	for (auto& entry : model_enchiridion.materials) {
		std::string materialName = entry.first;
		
		// Apply appropriate texture based on material name
		if (materialName.find("01") != std::string::npos || 
		    materialName.find("_01") != std::string::npos ||
		    materialName.find("Map01") != std::string::npos) {
			entry.second.tex = tex_enchiridion_01;
			entry.second.hasTexture = true;
			printf("  - Applied enchiridion_01 texture to: %s\n", materialName.c_str());
		}
		else if (materialName.find("02") != std::string::npos || 
		      materialName.find("_02") != std::string::npos ||
		         materialName.find("Map02") != std::string::npos) {
			entry.second.tex = tex_enchiridion_02;
			entry.second.hasTexture = true;
			printf("  - Applied enchiridion_02 texture to: %s\n", materialName.c_str());
		}
		else if (materialName.find("Paper") != std::string::npos || 
		   materialName.find("paper") != std::string::npos ||
		       materialName.find("Antique") != std::string::npos) {
			entry.second.tex = tex_enchiridion_paper;
			entry.second.hasTexture = true;
			printf("  - Applied paper texture to: %s\n", materialName.c_str());
		}
		else {
			// Default to texture 01
			entry.second.tex = tex_enchiridion_01;
			entry.second.hasTexture = true;
			printf("  - Applied default enchiridion_01 texture to: %s\n", materialName.c_str());
		}
		
		// Ensure proper color for texture display
		entry.second.diffColor[0] = 1.0f;
		entry.second.diffColor[1] = 1.0f;
		entry.second.diffColor[2] = 1.0f;
	}
	
	// Set Enchiridion size (smaller than Golem)
	model_enchiridion.scale_xyz = 1.5f;  // Increased from 0.3f to 1.5f for better visibility
	
	// Position Enchiridion next to Golem in Fire Kingdom (on a rock or pedestal-like position)
	model_enchiridion.pos_x = -112.0f;  // Right of Golem
	model_enchiridion.pos_y = 1.0f;     // Slightly elevated (on a pedestal/rock)
	model_enchiridion.pos_z = 2420.0f;  // Next to Golem
	
	model_enchiridion.GenerateDisplayList();
	printf("Enchiridion Loaded.\n");

	// --- SKY ---
	printf("Loading OBJ Model: Sky...\n");
	model_sky.Load("Models/sky/sky.obj", "Models/sky/");
	
	// Load sky texture - BMP format
	printf("Loading sky texture...\n");
	tex_sky.Load("Textures/sky.bmp");  // Load sky.bmp
	printf("  - Sky texture loaded: sky.bmp\n");
	
	// Sky is typically very large and encompasses the entire scene
	model_sky.scale_xyz = 500.0f;  // Very large scale to act as skybox
	
	// Center the sky at the origin
	model_sky.pos_x = 0.0f;
	model_sky.pos_y = 0.0f;
	model_sky.pos_z = 0.0f;
	
	// No rotation needed
	model_sky.rot_x = 0.0f;
	model_sky.rot_y = 0.0f;
	model_sky.rot_z = 0.0f;
	
	// Apply sky texture to all materials or use bright color if no texture
	for (auto& entry : model_sky.materials) {
		entry.second.tex = tex_sky;
		entry.second.hasTexture = true;
		// Use bright sky blue color as fallback
		entry.second.diffColor[0] = 0.53f;  // Sky blue R
		entry.second.diffColor[1] = 0.81f;  // Sky blue G
		entry.second.diffColor[2] = 0.92f;  // Sky blue B
	}
	
	model_sky.GenerateDisplayList();
	printf("Sky Loaded.\n");

	// --- CANDY CANE ---
	printf("Loading OBJ Model: Candy Cane...\n");
	model_candy_cane.Load("Models/candycane/Candy_Cane.obj", "Models/candycane/");
	tex_candy_cane.Load("Textures/candy-cane.bmp");
	for (auto& entry : model_candy_cane.materials) {
		entry.second.tex = tex_candy_cane;
		entry.second.hasTexture = true;
		entry.second.diffColor[0] = 1.0f;
		entry.second.diffColor[1] = 1.0f;
		entry.second.diffColor[2] = 1.0f;
	}
	model_candy_cane.GenerateDisplayList();

	model_candy_cane.scale_xyz = 1.5f;
	model_candy_cane.pos_x = 72.0f;
	model_candy_cane.pos_y = 0.0f;
	model_candy_cane.pos_z = 58.0f;
	model_candy_cane.rot_y = 0.0f;
	printf("Candy Cane Loaded.\n");

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

	// --- CUPCAKES ---
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

	// --- DONUT ---
	printf("Loading OBJ Model: Donut...\n");
	model_donut.Load("Models/donut/donut.obj", "Models/donut/");

	// Load the orange/dough texture
	tex_donut.Load("Textures/donut.bmp");

	for (auto& entry : model_donut.materials) {
		std::string name = entry.first;

		// If material name contains Icing, turn off texture and make it brown
		if (name.find("Icing") != std::string::npos || name.find("icing") != std::string::npos || name.find("Material.002") != std::string::npos)
		{
			entry.second.hasTexture = false;
			// Chocolate Color
			entry.second.diffColor[0] = 0.36f;
			entry.second.diffColor[1] = 0.20f;
			entry.second.diffColor[2] = 0.09f;
		}
		// Sprinkles (disable texture, make colorful)
		else if (name.find("Sprinkle") != std::string::npos || name.find("sprinkle") != std::string::npos)
		{
			entry.second.hasTexture = false;
			entry.second.diffColor[0] = 1.0f;
			entry.second.diffColor[1] = 0.0f;
			entry.second.diffColor[2] = 0.0f;
		}
		// Dough (keep texture)
		else
		{
			entry.second.tex = tex_donut;
			entry.second.hasTexture = true;
			// Reset color to white so texture shows
			entry.second.diffColor[0] = 1.0f;
			entry.second.diffColor[1] = 1.0f;
			entry.second.diffColor[2] = 1.0f;
		}
	}

	model_donut.scale_xyz = 20.0f;
	model_donut.pos_x = 95.0f;
	model_donut.pos_y = 1.0f;
	model_donut.pos_z = 55.0f;
	model_donut.rot_y = 45.0f;

	model_donut.GenerateDisplayList();
	printf("Donut Loaded.\n");

	// --- COINS ---
	printf("Loading OBJ Model: Coins...\n");
	tex_coin.Load("Textures/coin.bmp");

	for (int i = 0; i < NUM_COINS; i++) {
		model_coins[i].Load("Models/coin/coin.obj", "Models/coin/");
		for (auto& entry : model_coins[i].materials) {
			entry.second.tex = tex_coin;
			entry.second.hasTexture = true;
			entry.second.diffColor[0] = 1.0f;
			entry.second.diffColor[1] = 1.0f;
			entry.second.diffColor[2] = 1.0f;
		}
		model_coins[i].scale_xyz = 2.0f;
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
	tex_finn.Load("Textures/finn.bmp");

	for (auto& entry : model_finn.materials) {
		entry.second.tex = tex_finn;
		entry.second.hasTexture = true;
		entry.second.diffColor[0] = 1.0f;
		entry.second.diffColor[1] = 1.0f;
		entry.second.diffColor[2] = 1.0f;
	}

	model_finn.scale_xyz = 0.1f;
	model_finn.pos_x = 70.0f;
	model_finn.pos_y = 0.0f;
	model_finn.pos_z = 53.0f;
	model_finn.rot_y = -90.0f;

	model_finn.GenerateDisplayList();
	printf("Finn Ready.\n");

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

void myIdle(void)
{
	if (isJumping) {
		model_bmo.pos_y += jumpVelocity;
		jumpVelocity -= gravity;

		// Floor collision check: Land on Y = 0
		if (model_bmo.pos_y <= 0.0f) {
			model_bmo.pos_y = 0.0f;
			isJumping = false;
			jumpVelocity = 0.0f;
		}
	}

	// Animate Objects
	cupcakeRotation += 1.0f;
	if (cupcakeRotation >= 360.0f)
		cupcakeRotation = 0.0f;

	coinRotation += 2.0f;
	if (coinRotation >= 360.0f)
		coinRotation = 0.0f;

	coinBounceAngle += 0.1f;

	// Animate Donut (Shake/Bounce)
	donutShakeAngle += 0.1f;

	glutPostRedisplay();
}

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
	glutPassiveMotionFunc(myPassiveMotion);
	glutMouseFunc(myMouse);
	glutReshapeFunc(myReshape);

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
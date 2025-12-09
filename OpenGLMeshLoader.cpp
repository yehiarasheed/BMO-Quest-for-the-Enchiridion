#define GLUT_DISABLE_ATEXIT_HACK
#include "TextureBuilder.h"
#include "Model_3DS.h"
#include "Model_OBJ.h"
#include "GLTexture.h"
#include <glut.h>
#include <math.h>
#include <stdio.h>
#include <string>

// --- FMOD INCLUDES ---
#include <fmod.hpp>

int WIDTH = 1280;
int HEIGHT = 720;

GLuint tex;
char title[] = "BMO's Quest";

// --- FMOD GLOBAL VARIABLES ---
FMOD::System* fmodSystem;
FMOD::Sound* sndJump;
FMOD::Sound* sndCollect;
FMOD::Sound* sndBonk;
FMOD::Sound* sndLevelWarp;
FMOD::Sound* sndSparkle;
FMOD::Sound* sndJellyBounce;
FMOD::Sound* sndRescue;
FMOD::Sound* sndCane;
FMOD::Sound* bgmCandy;
FMOD::Sound* bgmFire;
FMOD::Channel* channelBGM = 0;

// Game state flags
bool enchiridionFound = false;
bool gameFinished = false;

// 3D Projection Options
GLdouble fovy = 45.0;
GLdouble aspectRatio = (GLdouble)WIDTH / (GLdouble)HEIGHT;
GLdouble zNear = 0.1;
GLdouble zFar = 5000;

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

// --- ANIMATION VARIABLES (PROPOSAL IMPLEMENTATION) ---
// Lighting
float sunAngle = 0.0f;        // For Day/Night cycle
float lavaIntensity = 1.0f;   // For flickering fire light
bool lavaDimming = true;

// Object Interaction Animations State
// 0 = Idle, >0 = Animating
float jellySquash = 1.0f;
float donutShake = 0.0f;
bool donutIsShaking = false;
int donutShakeTimer = 0;

// Collectible Animation Timers (For "On Interaction" logic)
// When hit, timer goes up. When timer hits max, object becomes invisible.
const int NUM_CUPCAKES = 5;
float cupcakeAnimTimers[NUM_CUPCAKES] = { 0 };

const int NUM_COINS = 5;
float coinAnimTimers[NUM_COINS] = { 0 };

const int NUM_DEMON_SWORDS = 15;
float swordAnimTimers[NUM_DEMON_SWORDS] = { 0 };

// Obstacle Reaction Timers
const int NUM_LAVA_HAMMERS = 8;
float hammerAnimTimers[NUM_LAVA_HAMMERS] = { 0 };

const int NUM_GOLEMS = 12;
float golemAnimTimers[NUM_GOLEMS] = { 0 };

const int NUM_FIRE_ROCKS = 10;
float rockAnimTimers[NUM_FIRE_ROCKS] = { 0 };

// Target Animations
float finnJumpTimer = 0.0f;       // Moves up and down along Y
float enchiridionPulseTimer = 0.0f; // Scale up and down

// Re-declared to prevent compilation errors, but will remain 0.0 for static look
float cupcakeRotation = 0.0f;
float coinRotation = 0.0f;
float globalBounceAngle = 0.0f;
float demonSwordRotations[NUM_DEMON_SWORDS] = { 0 };
float demonSwordBounceAngles[NUM_DEMON_SWORDS] = { 0 };

// Model Variables
Model_OBJ model_candy_kingdom;
Model_OBJ model_candy_cane;
Model_OBJ model_bmo;
Model_OBJ model_finn;
Model_OBJ model_finn_rescue;
bool isFinnRescued = false;
Model_OBJ model_cupcake;

// --- LICH VARIABLES ---
Model_OBJ model_lich;
GLTexture tex_lich;

// --- JELLY VARIABLES ---
Model_OBJ model_jelly;
GLTexture tex_jelly;

// --- DONUT VARIABLES ---
Model_OBJ model_donut;
GLTexture tex_donut;

// --- FIRE KINGDOM VARIABLES ---
Model_OBJ model_fire_temple;
GLTexture tex_fire_temple;
Model_OBJ model_lava_rock_ground;

// --- GOLEM VARIABLES ---
GLTexture tex_golem_em_map;
GLTexture tex_golem_lava_eye;
GLTexture tex_golem_norma;
GLTexture tex_golem_ao;
GLTexture tex_golem_podstavka;
GLTexture tex_golem_final;
Model_OBJ model_golem;

// --- FLAME PRINCESS VARIABLES ---
Model_OBJ model_flame_princess;
GLTexture tex_flame_princess;

// --- FIRE ROCK VARIABLES ---
GLTexture tex_fire_rock_20;
GLTexture tex_fire_rock_0;
Model_OBJ model_fire_rock;

// --- ENCHIRIDION VARIABLES ---
Model_OBJ model_enchiridion;
GLTexture tex_enchiridion_01;
GLTexture tex_enchiridion_02;
GLTexture tex_enchiridion_paper;

// --- LAVA HAMMER VARIABLES ---
GLTexture tex_lava_hammer_base;
GLTexture tex_lava_hammer_emissive;
GLTexture tex_lava_hammer_roughness;
GLTexture tex_lava_hammer_metallic;
GLTexture tex_lava_hammer_normal;
Model_OBJ model_lava_hammer;

// --- DEMON SWORD VARIABLES ---
GLTexture tex_demon_sword_albedo;
GLTexture tex_demon_sword_ao;
GLTexture tex_demon_sword_gloss;
GLTexture tex_demon_sword_normal;
GLTexture tex_demon_sword_specular;

// --- SKY VARIABLES ---
Model_OBJ model_sky;
GLTexture tex_sky;
GLTexture tex_sky_fire;
GLUquadricObj* skyQuad;

// Cupcake array for collectibles
Model_OBJ model_cupcakes[NUM_CUPCAKES];
bool cupcakeVisible[NUM_CUPCAKES];

// --- COIN VARIABLES ---
Model_OBJ model_coins[NUM_COINS];
bool coinVisible[NUM_COINS];
Model_OBJ model_coin;

// --- DEMON SWORD COLLECTIBLE ---
bool demonSwordVisible = true;
const int DEMON_SWORD_POINTS = 20;

// --- FIRE KINGDOM OBSTACLES (Multiple) ---
Model_OBJ model_fire_rocks[NUM_FIRE_ROCKS];
Model_OBJ model_lava_hammers[NUM_LAVA_HAMMERS];
Model_OBJ model_golems[NUM_GOLEMS];
Model_OBJ model_demon_swords[NUM_DEMON_SWORDS];

bool demonSwordsVisible[NUM_DEMON_SWORDS];

// --- LICH MOVEMENT VARIABLES ---
float lichStartX = 0.0f;
float lichRange = 40.0f;
float lichSpeed = 0.25f;
int lichDirection = 1;

// Random positions 
float fireRockPositions[NUM_FIRE_ROCKS][3] = {
	{ -118.0f, 0.0f, 2408.0f }, { -105.0f, 0.0f, 2415.0f }, { -112.0f, 0.0f, 2422.0f }, { -116.0f, 0.0f, 2430.0f },
	{ -108.0f, 0.0f, 2438.0f }, { -114.0f, 0.0f, 2445.0f }, { -106.0f, 0.0f, 2453.0f }, { -119.0f, 0.0f, 2460.0f },
	{ -110.0f, 0.0f, 2467.0f }, { -103.0f, 0.0f, 2474.0f }
};

float lavaHammerPositions[NUM_LAVA_HAMMERS][3] = {
	{ -115.0f, 0.0f, 2410.0f }, { -107.0f, 0.0f, 2420.0f }, { -113.0f, 0.0f, 2428.0f }, { -109.0f, 0.0f, 2436.0f },
	{ -117.0f, 0.0f, 2448.0f }, { -105.0f, 0.0f, 2456.0f }, { -111.0f, 0.0f, 2464.0f }, { -104.0f, 0.0f, 2472.0f }
};

float golemPositions[NUM_GOLEMS][3] = {
	{ -119.0f, 0.0f, 2406.0f }, { -108.0f, 0.0f, 2413.0f }, { -114.0f, 0.0f, 2419.0f }, { -106.0f, 0.0f, 2426.0f },
	{ -117.0f, 0.0f, 2434.0f }, { -111.0f, 0.0f, 2441.0f }, { -104.0f, 0.0f, 2449.0f }, { -115.0f, 0.0f, 2456.0f },
	{ -109.0f, 0.0f, 2463.0f }, { -118.0f, 0.0f, 2469.0f }, { -107.0f, 0.0f, 2475.0f }, { -103.0f, 0.0f, 2407.0f }
};

float demonSwordPositions[NUM_DEMON_SWORDS][3] = {
	{ -116.0f, 2.0f, 2407.0f }, { -110.0f, 2.0f, 2412.0f }, { -106.0f, 2.0f, 2418.0f }, { -113.0f, 2.0f, 2424.0f },
	{ -119.0f, 2.0f, 2431.0f }, { -108.0f, 2.0f, 2437.0f }, { -115.0f, 2.0f, 2443.0f }, { -104.0f, 2.0f, 2450.0f },
	{ -111.0f, 2.0f, 2456.0f }, { -117.0f, 2.0f, 2462.0f }, { -107.0f, 2.0f, 2468.0f }, { -114.0f, 2.0f, 2473.0f },
	{ -105.0f, 2.0f, 2411.0f }, { -118.0f, 2.0f, 2447.0f }, { -109.0f, 2.0f, 2465.0f }
};

// Coin Positions
float coinPositions[NUM_COINS][3] = {
	{ 80.0f, 2.0f, 65.0f },
	{ 95.0f, 2.0f, 50.0f },
	{ 80.0f, 2.0f, 35.0f },
	{ 65.0f, 2.0f, 50.0f },
	{ 80.0f, 8.0f, 50.0f }
};

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
bool mouseRotationEnabled = true;
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

// --- FMOD INITIALIZATION ---
void InitAudio()
{
	FMOD_RESULT result;
	result = FMOD::System_Create(&fmodSystem);
	fmodSystem->init(32, FMOD_INIT_NORMAL, 0);

	// Load Sound Effects
	fmodSystem->createSound("Audio/jump.wav", FMOD_DEFAULT, 0, &sndJump);
	fmodSystem->createSound("Audio/coin.wav", FMOD_DEFAULT, 0, &sndCollect);
	fmodSystem->createSound("Audio/bonk.wav", FMOD_DEFAULT, 0, &sndBonk);
	fmodSystem->createSound("Audio/warp.wav", FMOD_DEFAULT, 0, &sndLevelWarp);
	fmodSystem->createSound("Audio/sparkle.wav", FMOD_DEFAULT, 0, &sndSparkle);
	fmodSystem->createSound("Audio/jellybounce.wav", FMOD_DEFAULT, 0, &sndJellyBounce);
	fmodSystem->createSound("Audio/rescue.wav", FMOD_DEFAULT, 0, &sndRescue);
	fmodSystem->createSound("Audio/candycane.wav", FMOD_DEFAULT, 0, &sndCane);

	// Load Background Music
	fmodSystem->createSound("Audio/bgm_candy.mp3", FMOD_LOOP_NORMAL, 0, &bgmCandy);
	fmodSystem->createSound("Audio/bgm_fire.mp3", FMOD_LOOP_NORMAL, 0, &bgmFire);

	// Start Candy Kingdom Music immediately
	fmodSystem->playSound(bgmCandy, 0, false, &channelBGM);
	channelBGM->setVolume(0.4f);
}

// --- LIGHTING UPDATE FUNCTION (For Animation) ---
void UpdateLighting()
{
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	if (currentLevel == LEVEL_CANDY) {
		// --- CANDY KINGDOM: SUN ANIMATION ---
		// Moves across the sky based on sunAngle
		GLfloat light_position[] = { 100.0f * cos(sunAngle), 150.0f, 100.0f * sin(sunAngle), 1.0f };
		glLightfv(GL_LIGHT0, GL_POSITION, light_position);

		// Color changes: White (noon) -> Orange/Pink (sunset)
		float intensity = fabs(cos(sunAngle * 0.5));
		GLfloat diffuse[] = { 1.0f, 0.9f * intensity, 0.7f * intensity, 1.0f };
		GLfloat ambient[] = { 0.3f, 0.3f * intensity, 0.3f * intensity, 1.0f };

		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	}
	else {
		// --- FIRE KINGDOM: LAVA FLICKER ---
		// Static position high above
		GLfloat light_position[] = { 0.0f, 200.0f, 2440.0f, 1.0f };
		glLightfv(GL_LIGHT0, GL_POSITION, light_position);

		// Intensity flickers between Red/Yellow
		GLfloat diffuse[] = { 1.0f, 0.5f * lavaIntensity, 0.0f, 1.0f };
		GLfloat ambient[] = { 0.4f, 0.1f * lavaIntensity, 0.0f, 1.0f };

		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	}

	GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
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

	InitMaterial();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);

	// --- INITIALIZE AUDIO ---
	InitAudio();

	// --- SETUP SKY SPHERE GENERATOR ---
	skyQuad = gluNewQuadric();
	gluQuadricTexture(skyQuad, GL_TRUE);
	gluQuadricOrientation(skyQuad, GLU_INSIDE);
}

// --- COLLISION LOGIC ---
bool CheckJellyCollision(float newX, float newZ)
{
	if (currentLevel != LEVEL_CANDY) return false;
	float jellyRadius = 1.35f;
	float dx = newX - model_jelly.pos_x;
	float dz = newZ - model_jelly.pos_z;
	float distance = sqrt(dx * dx + dz * dz);
	return (distance < jellyRadius);
}

bool CheckDonutCollision(float newX, float newZ)
{
	if (currentLevel != LEVEL_CANDY) return false;
	float donutRadius = 2.0f;
	float dx = newX - model_donut.pos_x;
	float dz = newZ - model_donut.pos_z;
	float distance = sqrt(dx * dx + dz * dz);
	return (distance < donutRadius);
}

bool CheckCandyCaneCollision(float newX, float newZ)
{
	if (currentLevel != LEVEL_CANDY) return false;
	float caneRadius = 1.85f;
	float dx = newX - model_candy_cane.pos_x;
	float dz = newZ - model_candy_cane.pos_z;
	float distance = sqrt(dx * dx + dz * dz);
	return (distance < caneRadius);
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
			// PROPOSAL: Rotate + Pop/sparkle sound ON INTERACTION
			// Start the animation timer
			if (cupcakeAnimTimers[i] == 0) {
				cupcakeAnimTimers[i] = 1.0f;
				score += CUPCAKE_POINTS;
				fmodSystem->playSound(sndSparkle, 0, false, 0);
				printf("Cupcake %d collected! Score: %d\n", i + 1, score);
			}
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
			// PROPOSAL: Spin Y-axis, small bounce + Coin jingle ON INTERACTION
			if (coinAnimTimers[i] == 0) {
				coinAnimTimers[i] = 1.0f;
				score += COIN_POINTS;
				fmodSystem->playSound(sndCollect, 0, false, 0);
				printf("Coin %d Collected! +%d Points\n", i + 1, COIN_POINTS);
			}
		}
	}
}

void CheckDemonSwordCollision()
{
	if (currentLevel != LEVEL_FIRE) return;
	float swordCollisionRadius = 3.0f;
	for (int i = 0; i < NUM_DEMON_SWORDS; i++)
	{
		if (!demonSwordsVisible[i]) continue;
		float dx = model_bmo.pos_x - model_demon_swords[i].pos_x;
		float dz = model_bmo.pos_z - model_demon_swords[i].pos_z;
		float distance = sqrt(dx * dx + dz * dz);

		if (distance < swordCollisionRadius)
		{
			// PROPOSAL: Float up, rotate + Dark magical chime ON INTERACTION
			if (swordAnimTimers[i] == 0) {
				swordAnimTimers[i] = 1.0f;
				score += DEMON_SWORD_POINTS;
				fmodSystem->playSound(sndCollect, 0, false, 0);
				printf("Demon Sword %d Collected! +%d Points\n", i + 1, DEMON_SWORD_POINTS);
			}
		}
	}
}

void CheckEnchiridionCollision()
{
	if (currentLevel != LEVEL_FIRE) return;
	if (enchiridionFound) return;

	float enchRadius = 5.0f;
	float dx = model_bmo.pos_x - model_enchiridion.pos_x;
	float dz = model_bmo.pos_z - model_enchiridion.pos_z;
	float distance = sqrt(dx * dx + dz * dz);

	if (distance < enchRadius)
	{
		// PROPOSAL: Scale up and down + Magical unlocking chime
		enchiridionFound = true;
		gameFinished = true;
		printf(">>> ENCHIRIDION FOUND! FINAL SCORE: %d <<<\n", score);

		FMOD::Channel* sfxChannel = 0;
		fmodSystem->playSound(sndRescue, 0, false, &sfxChannel);
		if (sfxChannel) sfxChannel->setVolume(1.0f);
		if (channelBGM) channelBGM->stop();
	}
}

void CheckFinnCollision()
{
	if (currentLevel == LEVEL_CANDY)
	{
		float finnRadius = 2.0f;
		float dx = model_bmo.pos_x - model_finn.pos_x;
		float dz = model_bmo.pos_z - model_finn.pos_z;
		float distance = sqrt(dx * dx + dz * dz);

		if (distance < finnRadius)
		{
			printf(">>> TRAVELING TO FIRE KINGDOM! <<<\n");
			currentLevel = LEVEL_FIRE;
			fmodSystem->playSound(sndLevelWarp, 0, false, 0);
			channelBGM->stop();
			fmodSystem->playSound(bgmFire, 0, false, &channelBGM);
			channelBGM->setVolume(0.4f);

			model_bmo.pos_x = -111.0f;
			model_bmo.pos_z = 2416.1f;
			model_bmo.pos_y = 0.0f;
			model_bmo.rot_x = -240.0f;
			model_bmo.rot_y = 329.0f;
			model_bmo.rot_z = 240.0f;
			isJumping = false;
			jumpVelocity = 0.0f;
		}
	}
	else if (currentLevel == LEVEL_FIRE)
	{
		if (isFinnRescued) return;
		float rescueRadius = 3.0f;
		float dx = model_bmo.pos_x - model_finn_rescue.pos_x;
		float dz = model_bmo.pos_z - model_finn_rescue.pos_z;
		float distance = sqrt(dx * dx + dz * dz);

		if (distance < rescueRadius)
		{
			isFinnRescued = true;
			printf(">>> FINN RESCUED! <<<\n");
			// PROPOSAL: moves up and down along the Y-axis + Cheer / rescued
			FMOD::Channel* sfxChannel = 0;
			fmodSystem->playSound(sndRescue, 0, false, &sfxChannel);
			sfxChannel->setVolume(1.0f);
		}
	}
}

bool CheckGolemCollision(float newX, float newZ)
{
	if (currentLevel != LEVEL_FIRE) return false;
	float golemRadius = 3.0f;
	for (int i = 0; i < NUM_GOLEMS; i++)
	{
		float dx = newX - model_golems[i].pos_x;
		float dz = newZ - model_golems[i].pos_z;
		float distance = sqrt(dx * dx + dz * dz);
		if (distance < golemRadius)
		{
			// PROPOSAL: Translate back and forth + Roar / elemental
			golemAnimTimers[i] = 20.0f; // Start animation timer
			return true;
		}
	}
	return false;
}

bool CheckFireRockCollision(float newX, float newZ)
{
	if (currentLevel != LEVEL_FIRE) return false;
	float rockRadius = 2.5f;
	for (int i = 0; i < NUM_FIRE_ROCKS; i++)
	{
		float dx = newX - model_fire_rocks[i].pos_x;
		float dz = newZ - model_fire_rocks[i].pos_z;
		float distance = sqrt(dx * dx + dz * dz);
		if (distance < rockRadius)
		{
			// PROPOSAL: Small shake / bounce + Thud / impact
			rockAnimTimers[i] = 20.0f;
			return true;
		}
	}
	return false;
}

bool CheckLavaHammerCollision(float newX, float newZ)
{
	if (currentLevel != LEVEL_FIRE) return false;
	float hammerRadius = 2.5f;
	for (int i = 0; i < NUM_LAVA_HAMMERS; i++)
	{
		float dx = newX - model_lava_hammers[i].pos_x;
		float dz = newZ - model_lava_hammers[i].pos_z;
		float distance = sqrt(dx * dx + dz * dz);
		if (distance < hammerRadius)
		{
			// PROPOSAL: Pivot(rotate) + Metallic clang
			hammerAnimTimers[i] = 20.0f;
			return true;
		}
	}
	return false;
}

// --- INTERACTION / ANIMATION TRIGGER LOGIC ---
bool TryMove(float newX, float newZ)
{
	if (CheckCandyCaneCollision(newX, newZ)) {
		fmodSystem->playSound(sndCane, 0, false, 0);
		score -= 10;
		if (score < 0) score = 0;
		return false;
	}

	// 1. Jelly - SQUASH ANIMATION (PROPOSAL: Scale+ squash& stretch(jiggle) + Bubbling / squishy)
	if (CheckJellyCollision(newX, newZ))
	{
		jellySquash = 0.5f; // Trigger Squash

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

		FMOD::Channel* sfxChannel = 0;
		fmodSystem->playSound(sndJellyBounce, 0, false, &sfxChannel);
		if (sfxChannel) sfxChannel->setVolume(1.0f);

		score -= 5;
		if (score < 0) score = 0;
		return false;
	}

	// 2. Donut - SHAKE ANIMATION (PROPOSAL: Small shake / bounce + Thud / impact)
	if (CheckDonutCollision(newX, newZ))
	{
		donutIsShaking = true; // Trigger Shake
		donutShakeTimer = 20;

		float dx = model_bmo.pos_x - model_donut.pos_x;
		float dz = model_bmo.pos_z - model_donut.pos_z;
		float len = sqrt(dx * dx + dz * dz);
		float nx = 0.0f, nz = -1.0f;
		if (len > 0.001f) { nx = dx / len; nz = dz / len; }

		float pushBack = 2.5f;
		model_bmo.pos_x = model_donut.pos_x + nx * (pushBack + 1.5f);
		model_bmo.pos_z = model_donut.pos_z + nz * (pushBack + 1.5f);

		if (!isJumping) {
			isJumping = true;
			jumpVelocity = jumpStrength * 0.9f;
		}
		else if (jumpVelocity < 0.0f) {
			jumpVelocity = jumpStrength * 1.8f;
		}

		fmodSystem->playSound(sndBonk, 0, false, 0);
		score -= 8;
		if (score < 0) score = 0;
		return false;
	}

	// 3. Golem
	if (CheckGolemCollision(newX, newZ))
	{
		float minDist = 99999.0f;
		int closestIdx = 0;
		for (int i = 0; i < NUM_GOLEMS; i++)
		{
			float dx = model_bmo.pos_x - model_golems[i].pos_x;
			float dz = model_bmo.pos_z - model_golems[i].pos_z;
			float dist = sqrt(dx * dx + dz * dz);
			if (dist < minDist) { minDist = dist; closestIdx = i; }
		}
		float dx = model_bmo.pos_x - model_golems[closestIdx].pos_x;
		float dz = model_bmo.pos_z - model_golems[closestIdx].pos_z;
		float len = sqrt(dx * dx + dz * dz);
		float nx = 0.0f, nz = -1.0f;
		if (len > 0.001f) { nx = dx / len; nz = dz / len; }

		float pushBack = 3.0f;
		model_bmo.pos_x = model_golems[closestIdx].pos_x + nx * (pushBack + 1.5f);
		model_bmo.pos_z = model_golems[closestIdx].pos_z + nz * (pushBack + 1.5f);

		if (!isJumping) {
			isJumping = true;
			jumpVelocity = jumpStrength * 0.9f;
		}
		else if (jumpVelocity < 0.0f) {
			jumpVelocity = jumpStrength * 1.8f;
		}
		fmodSystem->playSound(sndBonk, 0, false, 0);
		return false;
	}

	// 4. Fire Rock
	if (CheckFireRockCollision(newX, newZ))
	{
		float minDist = 99999.0f;
		int closestIdx = 0;
		for (int i = 0; i < NUM_GOLEMS; i++)
		{
			float dx = model_bmo.pos_x - model_golems[i].pos_x;
			float dz = model_bmo.pos_z - model_golems[i].pos_z;
			float dist = sqrt(dx * dx + dz * dz);
			if (dist < minDist) { minDist = dist; closestIdx = i; }
		}
		float dx = model_bmo.pos_x - model_fire_rocks[closestIdx].pos_x;
		float dz = model_bmo.pos_z - model_fire_rocks[closestIdx].pos_z;
		float len = sqrt(dx * dx + dz * dz);
		float nx = 0.0f, nz = -1.0f;
		if (len > 0.001f) { nx = dx / len; nz = dz / len; }

		float pushBack = 2.5f;
		model_bmo.pos_x = model_fire_rocks[closestIdx].pos_x + nx * (pushBack + 1.5f);
		model_bmo.pos_z = model_fire_rocks[closestIdx].pos_z + nz * (pushBack + 1.5f);

		if (!isJumping) {
			isJumping = true;
			jumpVelocity = jumpStrength * 0.9f;
		}
		else if (jumpVelocity < 0.0f) {
			jumpVelocity = jumpStrength * 1.8f;
		}
		fmodSystem->playSound(sndBonk, 0, false, 0);
		return false;
	}

	// 5. Lava Hammer
	if (CheckLavaHammerCollision(newX, newZ))
	{
		float minDist = 99999.0f;
		int closestIdx = 0;
		for (int i = 0; i < NUM_GOLEMS; i++)
		{
			float dx = model_bmo.pos_x - model_golems[i].pos_x;
			float dz = model_bmo.pos_z - model_golems[i].pos_z;
			float dist = sqrt(dx * dx + dz * dz);
			if (dist < minDist) { minDist = dist; closestIdx = i; }
		}
		float dx = model_bmo.pos_x - model_golems[closestIdx].pos_x;
		float dz = model_bmo.pos_z - model_golems[closestIdx].pos_z;
		float len = sqrt(dx * dx + dz * dz);
		float nx = 0.0f, nz = -1.0f;
		if (len > 0.001f) { nx = dx / len; nz = dz / len; }
		if (!isJumping) {
			isJumping = true;
			jumpVelocity = jumpStrength * 0.9f;
		}
		else if (jumpVelocity < 0.0f) {
			jumpVelocity = jumpStrength * 1.8f;
		}
		fmodSystem->playSound(sndBonk, 0, false, 0);
		return false;
	}

	model_bmo.pos_x = newX;
	model_bmo.pos_z = newZ;
	return true;
}

// --- DRAW SKY SPHERE ---
void DrawSkySphere()
{
	glPushMatrix();
	glDisable(GL_LIGHTING);
	glDepthMask(GL_FALSE);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 1.0f);

	if (currentLevel == LEVEL_CANDY) {
		if (tex_sky.texture[0] > 0) glBindTexture(GL_TEXTURE_2D, tex_sky.texture[0]);
	}
	else {
		if (tex_sky_fire.texture[0] > 0) glBindTexture(GL_TEXTURE_2D, tex_sky_fire.texture[0]);
	}

	glTranslatef(model_bmo.pos_x, model_bmo.pos_y, model_bmo.pos_z);
	glRotatef(90, 1, 0, 0);
	gluSphere(skyQuad, 2000.0, 32, 32);

	glBindTexture(GL_TEXTURE_2D, 0);
	glDepthMask(GL_TRUE);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

// --- RENDER HUD ---
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

	if (isFinnRescued) {
		char rescueMsg[] = "FINN RESCUED!";
		glColor3f(0.0f, 1.0f, 0.0f);
		glRasterPos2i(WIDTH / 2 - 50, HEIGHT - 100);
		for (char* c = rescueMsg; *c != '\0'; ++c)
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
	}

	if (enchiridionFound)
	{
		char finalMsg[128];
		sprintf(finalMsg, "ENCHIRIDION FOUND! FINAL SCORE: %d", score);
		glColor3f(1.0f, 0.8f, 0.0f);
		glRasterPos2i(WIDTH / 2 - 120, HEIGHT - 70);
		for (char* c = finalMsg; *c != '\0'; ++c)
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void myDisplay(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// UPDATE LIGHTING FOR ANIMATION
	UpdateLighting();

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

	// ============================================
	// RENDER ENVIRONMENT
	// ============================================

	DrawSkySphere();

	if (currentLevel == LEVEL_CANDY)
	{
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

		// --- DRAW THE LICH ---
		glPushMatrix();
		glColor3f(1.0f, 1.0f, 1.0f);
		glTranslatef(model_lich.pos_x, model_lich.pos_y, model_lich.pos_z);
		glRotatef(model_lich.rot_y, 0.0f, 1.0f, 0.0f);
		glRotatef(model_lich.rot_x, 1.0f, 0.0f, 0.0f);
		glRotatef(model_lich.rot_z, 0.0f, 0.0f, 1.0f);

		// PROPOSAL: Translate back and forth + Roar / elemental (Lich Movement handled in Idle)
		// No extra per-frame animation needed here beyond position update

		float temp_lich_x = model_lich.pos_x;
		float temp_lich_y = model_lich.pos_y;
		float temp_lich_z = model_lich.pos_z;
		model_lich.pos_x = 0.0f;
		model_lich.pos_y = 0.0f;
		model_lich.pos_z = 0.0f;
		model_lich.Draw();
		model_lich.pos_x = temp_lich_x;
		model_lich.pos_y = temp_lich_y;
		model_lich.pos_z = temp_lich_z;
		glPopMatrix();

		// Cupcakes (PROPOSAL: Rotate on Interaction)
		for (int i = 0; i < NUM_CUPCAKES; i++)
		{
			if (cupcakeVisible[i] || cupcakeAnimTimers[i] > 0) // Draw if visible or animating
			{
				if (cupcakeAnimTimers[i] > 0) { // If being collected
					glPushMatrix();
					glTranslatef(model_cupcakes[i].pos_x, model_cupcakes[i].pos_y, model_cupcakes[i].pos_z);
					glRotatef(cupcakeAnimTimers[i] * 20.0f, 0.0f, 1.0f, 0.0f); // Fast spin
					// Scale down as it disappears
					float scale = 1.0f - (cupcakeAnimTimers[i] / 50.0f);
					glScalef(scale, scale, scale);
					glTranslatef(-model_cupcakes[i].pos_x, -model_cupcakes[i].pos_y, -model_cupcakes[i].pos_z);
					model_cupcakes[i].Draw();
					glPopMatrix();
				}
				else {
					// Normal static draw
					glPushMatrix();
					glTranslatef(model_cupcakes[i].pos_x, model_cupcakes[i].pos_y, model_cupcakes[i].pos_z);
					glTranslatef(-model_cupcakes[i].pos_x, -model_cupcakes[i].pos_y, -model_cupcakes[i].pos_z);
					model_cupcakes[i].Draw();
					glPopMatrix();
				}
			}
		}

		// Donut (Shake Animation on Collision)
		glPushMatrix();
		glColor3f(1.0f, 1.0f, 1.0f);
		float ox = model_donut.pos_x;
		float oy = model_donut.pos_y;
		float oz = model_donut.pos_z;

		// Apply Shake if impacted
		float currentShake = donutIsShaking ? sin(donutShake * 20.0f) * 10.0f : 0.0f;

		glTranslatef(ox, oy, oz);
		glRotatef(model_donut.rot_y, 0, 1, 0);
		glRotatef(currentShake, 0, 0, 1);

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

		// Coins (PROPOSAL: Spin Y-axis, small bounce ON INTERACTION)
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex_coin.texture[0]);
		glColor3f(1.0f, 1.0f, 1.0f);
		for (int i = 0; i < NUM_COINS; i++)
		{
			if (coinVisible[i] || coinAnimTimers[i] > 0)
			{
				if (coinAnimTimers[i] > 0) { // If being collected
					glPushMatrix();
					float bounceHeight = coinAnimTimers[i] * 0.1f; // Fly up
					float cx = coinPositions[i][0];
					float cy = coinPositions[i][1];
					float cz = coinPositions[i][2];
					glTranslatef(cx, cy + bounceHeight, cz);
					glRotatef(coinAnimTimers[i] * 30.0f, 0.0f, 1.0f, 0.0f); // Spin fast
					model_coins[i].pos_x = 0;
					model_coins[i].pos_y = 0;
					model_coins[i].pos_z = 0;
					model_coins[i].Draw();
					glPopMatrix();
				}
				else {
					// Normal static draw (maybe slow idle spin?)
					glPushMatrix();
					float cx = coinPositions[i][0];
					float cy = coinPositions[i][1];
					float cz = coinPositions[i][2];
					glTranslatef(cx, cy, cz);
					model_coins[i].pos_x = 0;
					model_coins[i].pos_y = 0;
					model_coins[i].pos_z = 0;
					model_coins[i].Draw();
					glPopMatrix();
				}
			}
		}
		glBindTexture(GL_TEXTURE_2D, 0);

		// Jelly (Squash Animation on Collision)
		glPushMatrix();
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex_jelly.texture[0]);
		glColor3f(1.0f, 1.0f, 1.0f);

		// Apply Scaling based on interaction
		float jx = model_jelly.pos_x;
		float jy = model_jelly.pos_y;
		float jz = model_jelly.pos_z;
		glTranslatef(jx, jy, jz);
		// Scale Y down, Scale X/Z up slightly to preserve volume
		glScalef(1.0f + (1.0f - jellySquash), jellySquash, 1.0f + (1.0f - jellySquash));
		glTranslatef(-jx, -jy, -jz);

		model_jelly.Draw();
		glBindTexture(GL_TEXTURE_2D, 0);
		glPopMatrix();
	}
	else if (currentLevel == LEVEL_FIRE)
	{
		// --- DRAW FIRE KINGDOM TEMPLE ---
		glPushMatrix();
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, tex_fire_temple.texture[0]);
		glTranslatef(model_fire_temple.pos_x, model_fire_temple.pos_y, model_fire_temple.pos_z);
		glRotatef(model_fire_temple.rot_x, 1.0f, 0.0f, 0.0f);
		glRotatef(model_fire_temple.rot_y, 0.0f, 1.0f, 0.0f);
		glRotatef(model_fire_temple.rot_z, 0.0f, 0.0f, 1.0f);

		float temp_x = model_fire_temple.pos_x;
		float temp_y = model_fire_temple.pos_y;
		float temp_z = model_fire_temple.pos_z;
		model_fire_temple.pos_x = 0.0f;
		model_fire_temple.pos_y = 0.0f;
		model_fire_temple.pos_z = 0.0f;
		model_fire_temple.Draw();
		model_fire_temple.pos_x = temp_x;
		model_fire_temple.pos_y = temp_y;
		model_fire_temple.pos_z = temp_z;
		glBindTexture(GL_TEXTURE_2D, 0);
		glPopMatrix();

		// --- DRAW GOLEMS (PROPOSAL: Translate back and forth) ---
		for (int i = 0; i < NUM_GOLEMS; i++)
		{
			glPushMatrix();
			glEnable(GL_TEXTURE_2D);
			glColor3f(1.0f, 1.0f, 1.0f);

			// Animation: Translate back if hit
			float pushZ = (golemAnimTimers[i] > 0) ? sin(golemAnimTimers[i] * 0.5f) * 2.0f : 0.0f;

			glTranslatef(model_golems[i].pos_x, model_golems[i].pos_y, model_golems[i].pos_z + pushZ);
			glRotatef(model_golems[i].rot_y, 0.0f, 1.0f, 0.0f);
			glRotatef(model_golems[i].rot_x, 1.0f, 0.0f, 0.0f);
			glRotatef(model_golems[i].rot_z, 0.0f, 0.0f, 1.0f);

			float temp_x = model_golems[i].pos_x;
			float temp_y = model_golems[i].pos_y;
			float temp_z = model_golems[i].pos_z;
			model_golems[i].pos_x = 0.0f;
			model_golems[i].pos_y = 0.0f;
			model_golems[i].pos_z = 0.0f;

			model_golems[i].Draw();

			model_golems[i].pos_x = temp_x;
			model_golems[i].pos_y = temp_y;
			model_golems[i].pos_z = temp_z;
			glPopMatrix();
		}

		// --- DRAW FIRE ROCKS (PROPOSAL: Small shake / bounce) ---
		for (int i = 0; i < NUM_FIRE_ROCKS; i++)
		{
			glPushMatrix();
			glEnable(GL_TEXTURE_2D);
			glColor3f(1.0f, 1.0f, 1.0f);

			// Animation: Small bounce if hit
			float bounceY = (rockAnimTimers[i] > 0) ? fabs(sin(rockAnimTimers[i])) * 1.0f : 0.0f;

			glTranslatef(model_fire_rocks[i].pos_x, model_fire_rocks[i].pos_y + bounceY, model_fire_rocks[i].pos_z);
			glRotatef(model_fire_rocks[i].rot_y, 0.0f, 1.0f, 0.0f);
			glRotatef(model_fire_rocks[i].rot_x, 1.0f, 0.0f, 0.0f);
			glRotatef(model_fire_rocks[i].rot_z, 0.0f, 0.0f, 1.0f);

			float temp_x = model_fire_rocks[i].pos_x;
			float temp_y = model_fire_rocks[i].pos_y;
			float temp_z = model_fire_rocks[i].pos_z;
			model_fire_rocks[i].pos_x = 0.0f;
			model_fire_rocks[i].pos_y = 0.0f;
			model_fire_rocks[i].pos_z = 0.0f;

			model_fire_rocks[i].Draw();

			model_fire_rocks[i].pos_x = temp_x;
			model_fire_rocks[i].pos_y = temp_y;
			model_fire_rocks[i].pos_z = temp_z;
			glPopMatrix();
		}

		// --- DRAW LAVA HAMMERS (PROPOSAL: Pivot/rotate) ---
		for (int i = 0; i < NUM_LAVA_HAMMERS; i++)
		{
			glPushMatrix();
			glEnable(GL_TEXTURE_2D);
			glColor3f(1.0f, 1.0f, 1.0f);

			// Animation: Pivot if hit
			float pivotAngle = (hammerAnimTimers[i] > 0) ? sin(hammerAnimTimers[i]) * 45.0f : 0.0f;

			glTranslatef(model_lava_hammers[i].pos_x, model_lava_hammers[i].pos_y, model_lava_hammers[i].pos_z);
			glRotatef(model_lava_hammers[i].rot_y, 0.0f, 1.0f, 0.0f);
			glRotatef(model_lava_hammers[i].rot_x + pivotAngle, 1.0f, 0.0f, 0.0f); // Pivot on X
			glRotatef(model_lava_hammers[i].rot_z, 0.0f, 0.0f, 1.0f);

			float temp_x = model_lava_hammers[i].pos_x;
			float temp_y = model_lava_hammers[i].pos_y;
			float temp_z = model_lava_hammers[i].pos_z;
			model_lava_hammers[i].pos_x = 0.0f;
			model_lava_hammers[i].pos_y = 0.0f;
			model_lava_hammers[i].pos_z = 0.0f;

			model_lava_hammers[i].Draw();

			model_lava_hammers[i].pos_x = temp_x;
			model_lava_hammers[i].pos_y = temp_y;
			model_lava_hammers[i].pos_z = temp_z;
			glPopMatrix();
		}

		// --- DRAW DEMON SWORDS (PROPOSAL: Float up, rotate ON INTERACTION) ---
		for (int i = 0; i < NUM_DEMON_SWORDS; i++)
		{
			if (demonSwordsVisible[i] || swordAnimTimers[i] > 0)
			{
				if (swordAnimTimers[i] > 0) {
					glPushMatrix();
					glEnable(GL_TEXTURE_2D);
					glColor3f(1.0f, 1.0f, 1.0f);

					// Float up and Rotate Animation
					float swordBounce = swordAnimTimers[i] * 0.1f; // Float Up
					float sx = model_demon_swords[i].pos_x;
					float sy = model_demon_swords[i].pos_y;
					float sz = model_demon_swords[i].pos_z;

					glTranslatef(sx, sy + swordBounce, sz);
					glRotatef(swordAnimTimers[i] * 20.0f, 0.0f, 1.0f, 0.0f); // Spin
					glRotatef(90.0f, 1.0f, 0.0f, 0.0f);  // Keep vertical

					model_demon_swords[i].pos_x = 0.0f;
					model_demon_swords[i].pos_y = 0.0f;
					model_demon_swords[i].pos_z = 0.0f;
					model_demon_swords[i].Draw();
					glPopMatrix();
				}
				else {
					// Normal idle
					glPushMatrix();
					glEnable(GL_TEXTURE_2D);
					glColor3f(1.0f, 1.0f, 1.0f);
					float sx = model_demon_swords[i].pos_x;
					float sy = model_demon_swords[i].pos_y;
					float sz = model_demon_swords[i].pos_z;
					glTranslatef(sx, sy, sz);
					glRotatef(90.0f, 1.0f, 0.0f, 0.0f);  // Keep vertical
					model_demon_swords[i].pos_x = 0.0f;
					model_demon_swords[i].pos_y = 0.0f;
					model_demon_swords[i].pos_z = 0.0f;
					model_demon_swords[i].Draw();
					glPopMatrix();
				}
			}
		}
		// --- DRAW ENCHIRIDION ---
		glPushMatrix();
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 1.0f);
		glTranslatef(model_enchiridion.pos_x, model_enchiridion.pos_y, model_enchiridion.pos_z);
		glRotatef(model_enchiridion.rot_y, 0.0f, 1.0f, 0.0f);
		glRotatef(model_enchiridion.rot_x, 1.0f, 0.0f, 0.0f);
		glRotatef(model_enchiridion.rot_z, 0.0f, 0.0f, 1.0f);

		// PROPOSAL: Scale up and down (Enchiridion Pedestal)
		if (enchiridionFound) {
			float scale = 1.0f + (sin(enchiridionPulseTimer) * 0.2f);
			glScalef(scale, scale, scale);
		}

		float temp_ench_x = model_enchiridion.pos_x;
		float temp_ench_y = model_enchiridion.pos_y;
		float temp_ench_z = model_enchiridion.pos_z;
		model_enchiridion.pos_x = 0.0f;
		model_enchiridion.pos_y = 0.0f;
		model_enchiridion.pos_z = 0.0f;

		model_enchiridion.Draw();

		model_enchiridion.pos_x = temp_ench_x;
		model_enchiridion.pos_y = temp_ench_y;
		model_enchiridion.pos_z = temp_ench_z;
		glPopMatrix();

		// --- DRAW FLAME PRINCESS ---
		glPushMatrix();
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 1.0f);
		glTranslatef(model_flame_princess.pos_x, model_flame_princess.pos_y, model_flame_princess.pos_z);
		glRotatef(model_flame_princess.rot_y, 0.0f, 1.0f, 0.0f);
		glRotatef(model_flame_princess.rot_x, 1.0f, 0.0f, 0.0f);
		glRotatef(model_flame_princess.rot_z, 0.0f, 0.0f, 1.0f);

		float temp_fp_x = model_flame_princess.pos_x;
		float temp_fp_y = model_flame_princess.pos_y;
		float temp_fp_z = model_flame_princess.pos_z;
		model_flame_princess.pos_x = 0.0f;
		model_flame_princess.pos_y = 0.0f;
		model_flame_princess.pos_z = 0.0f;

		model_flame_princess.Draw();

		model_flame_princess.pos_x = temp_fp_x;
		model_flame_princess.pos_y = temp_fp_y;
		model_flame_princess.pos_z = temp_fp_z;
		glPopMatrix();

		// --- DRAW RESCUE FINN (FIRE KINGDOM) ---
		glPushMatrix();
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, tex_finn.texture[0]);

		// PROPOSAL: moves up and down along the Y-axis
		float finnJump = (isFinnRescued) ? fabs(sin(finnJumpTimer) * 2.0f) : 0.0f;

		glTranslatef(model_finn_rescue.pos_x, model_finn_rescue.pos_y + finnJump, model_finn_rescue.pos_z);
		glRotatef(model_finn_rescue.rot_y, 0.0f, 1.0f, 0.0f);

		// Reset for draw
		float temp_finn_r_x = model_finn_rescue.pos_x;
		float temp_finn_r_y = model_finn_rescue.pos_y;
		float temp_finn_r_z = model_finn_rescue.pos_z;
		model_finn_rescue.pos_x = 0.0f;
		model_finn_rescue.pos_y = 0.0f;
		model_finn_rescue.pos_z = 0.0f;

		model_finn_rescue.Draw();

		// Restore
		model_finn_rescue.pos_x = temp_finn_r_x;
		model_finn_rescue.pos_y = temp_finn_r_y;
		model_finn_rescue.pos_z = temp_finn_r_z;

		glBindTexture(GL_TEXTURE_2D, 0);
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
	if (gameFinished) return; // ignore input after finishing

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
			CheckEnchiridionCollision();
			CheckDemonSwordCollision();
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
			CheckEnchiridionCollision();
			CheckDemonSwordCollision();
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
			CheckEnchiridionCollision();
			CheckDemonSwordCollision();
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
			CheckEnchiridionCollision();
			CheckDemonSwordCollision();
		}
	}
	break;
	case ' ':
		if (!isJumping) {
			isJumping = true;
			jumpVelocity = jumpStrength;

			// --- PLAY JUMP SOUND ---
			fmodSystem->playSound(sndJump, 0, false, 0);
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
	if (gameFinished) return; // ignore input after finishing

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
			CheckEnchiridionCollision();
			CheckDemonSwordCollision();
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
			CheckEnchiridionCollision();
			CheckDemonSwordCollision();
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
			CheckEnchiridionCollision();
			CheckDemonSwordCollision();
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
			CheckEnchiridionCollision();
			CheckDemonSwordCollision();
		}
	}
	break;
	}
	glutPostRedisplay();
}

void myMotion(int x, int y)
{
	if (!mouseLookEnabled) return;

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

			// --- PLAY JUMP SOUND ---
			fmodSystem->playSound(sndJump, 0, false, 0);

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

	// --- THE LICH ---
	printf("Loading OBJ Model: The Lich...\n");
	model_lich.Load("Models/lich/Lich.obj", "Models/lich/");

	model_lich.scale_xyz = 0.5f;

	// Position and Rotation
	model_lich.pos_x = 88.0f;
	model_lich.pos_y = 10.0f;
	model_lich.pos_z = 50.0f;
	model_lich.rot_x = 180.0f;
	model_lich.rot_y = 180.0f;
	model_lich.rot_z = 0.0f;
	// Initialize the logic variable to match the position
	lichStartX = model_lich.pos_x;
	// --- MANUAL COLOR ASSIGNMENT ---
	for (auto& entry : model_lich.materials) {
		std::string name = entry.first; // Get the material name

		// Print name to console so you can check if they match!
		printf("Lich Material Found: %s\n", name.c_str());

		// 1. DISABLE TEXTURES (Since you don't have an image)
		entry.second.hasTexture = false;

		// 2. ASSIGN COLORS BASED ON KEYWORDS

		// CHECK FOR RED PARTS (The Sash / Cloth hanging down / Scythe Handle)
		if (name.find("Red") != std::string::npos ||
			name.find("Sash") != std::string::npos ||
			name.find("Cloth") != std::string::npos ||
			name.find("Handle") != std::string::npos)
		{
			// Dark Red
			entry.second.diffColor[0] = 0.6f; // R
			entry.second.diffColor[1] = 0.1f; // G
			entry.second.diffColor[2] = 0.1f; // B
		}
		// CHECK FOR BONE PARTS (Skulls, Hands)
		else if (name.find("Bone") != std::string::npos ||
			name.find("Skull") != std::string::npos ||
			name.find("Hand") != std::string::npos ||
			name.find("White") != std::string::npos)
		{
			// Bone White (Light Beige)
			entry.second.diffColor[0] = 0.9f;
			entry.second.diffColor[1] = 0.9f;
			entry.second.diffColor[2] = 0.8f;
		}
		// CHECK FOR METAL PARTS (Shoulder Armor, Chest)
		else if (name.find("Metal") != std::string::npos ||
			name.find("Armor") != std::string::npos ||
			name.find("Silver") != std::string::npos ||
			name.find("Grey") != std::string::npos)
		{
			// Light Grey
			entry.second.diffColor[0] = 0.7f;
			entry.second.diffColor[1] = 0.7f;
			entry.second.diffColor[2] = 0.75f;
		}
		// CHECK FOR SCYTHE BLADE
		else if (name.find("Blade") != std::string::npos)
		{
			// Dark Metal
			entry.second.diffColor[0] = 0.3f;
			entry.second.diffColor[1] = 0.3f;
			entry.second.diffColor[2] = 0.35f;
		}
		// DEFAULT: EVERYTHING ELSE (The Robe)
		else {
			// Dark Grey (Not pure black, so lighting still works)
			entry.second.diffColor[0] = 0.15f;
			entry.second.diffColor[1] = 0.15f;
			entry.second.diffColor[2] = 0.2f;
		}
	}

	model_lich.GenerateDisplayList();
	printf("The Lich Loaded with Manual Colors.\n");

	// --- SKY 1: Candy Kingdom ---
	printf("Loading Sky Texture 1...\n");
	tex_sky.Load("Textures/clouds_anime_6k.bmp");

	// --- SKY 2: Fire Kingdom (NEW) ---
	printf("Loading Sky Texture 2...\n");
	// Ensure this file exists and is resized (e.g., 2048x1024)
	tex_sky_fire.Load("Textures/dark sky.bmp");

	printf("Skies Loaded.\n");

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

	model_lava_rock_ground.scale_xyz = 600.0f;  // Larger ground to accommodate more objects
	model_lava_rock_ground.pos_x = -111.0f;      // Center with BMO spawn
	model_lava_rock_ground.pos_y = -50.0f;
	model_lava_rock_ground.pos_z = 2440.0f;      // Center between spawn and enchiridion
	model_lava_rock_ground.rot_x = 0.0f;
	model_lava_rock_ground.rot_y = 0.0f;
	model_lava_rock_ground.rot_z = 0.0f;
	model_lava_rock_ground.GenerateDisplayList();
	printf("Lava Rock Ground Loaded.\n");

	// --- LOAD SHARED TEXTURES FIRST ---
	printf("Loading Shared Fire Kingdom Textures...\n");

	// Golem Textures
	tex_golem_final.Load("Textures/texturesgolem/six.bmp");
	tex_golem_lava_eye.Load("Textures/texturesgolem/two.bmp");
	tex_golem_em_map.Load("Textures/texturesgolem/one.bmp");
	tex_golem_norma.Load("Textures/texturesgolem/three.bmp");
	tex_golem_ao.Load("Textures/texturesgolem/four.bmp");
	tex_golem_podstavka.Load("Textures/texturesgolem/five.bmp");

	// Fire Rock Textures
	tex_fire_rock_20.Load("Textures/texturesrock/rock20_tex00.bmp");
	tex_fire_rock_0.Load("Textures/texturesrock/rock0_tex00.bmp");

	// Lava Hammer Textures
	tex_lava_hammer_base.Load("Textures/textureslavahammer/phong1SG_Base_color.bmp");
	tex_lava_hammer_emissive.Load("Textures/textureslavahammer/phong1SG_Emissive.bmp");
	tex_lava_hammer_roughness.Load("Textures/textureslavahammer/phong1SG_Roughness.bmp");
	tex_lava_hammer_metallic.Load("Textures/textureslavahammer/phong1SG_Metallic.bmp");
	tex_lava_hammer_normal.Load("Textures/textureslavahammer/phong1SG_Normal_OpenGL.bmp");

	// Demon Sword Textures
	tex_demon_sword_albedo.Load("Textures/texturesdemonsword/albedo.bmp");
	tex_demon_sword_ao.Load("Textures/texturesdemonsword/ao.bmp");
	tex_demon_sword_gloss.Load("Textures/texturesdemonsword/gloss.bmp");
	tex_demon_sword_normal.Load("Textures/texturesdemonsword/normal.bmp");
	tex_demon_sword_specular.Load("Textures/texturesdemonsword/specular.bmp");

	printf("Shared Textures Loaded.\n");
	// --- GOLEMS (Multiple) ---
	printf("Loading OBJ Model: Golems...\n");
	for (int i = 0; i < NUM_GOLEMS; i++)
	{
		model_golems[i].Load("Models/golem/golem.obj", "Models/golem/");

		for (auto& entry : model_golems[i].materials) {
			std::string materialName = entry.first;
			if (materialName.find("Eye") != std::string::npos ||
				materialName.find("eye") != std::string::npos ||
				materialName.find("Lava") != std::string::npos ||
				materialName.find("lava") != std::string::npos) {
				entry.second.tex = tex_golem_lava_eye;
				entry.second.hasTexture = true;
			}
			else if (materialName.find("Podstavka") != std::string::npos ||
				materialName.find("podstavka") != std::string::npos ||
				materialName.find("Base") != std::string::npos ||
				materialName.find("base") != std::string::npos) {
				entry.second.tex = tex_golem_podstavka;
				entry.second.hasTexture = true;
			}
			else {
				entry.second.tex = tex_golem_final;
				entry.second.hasTexture = true;
			}
			entry.second.diffColor[0] = 1.0f;
			entry.second.diffColor[1] = 1.0f;
			entry.second.diffColor[2] = 1.0f;
		}

		model_golems[i].scale_xyz = 0.5f;
		model_golems[i].pos_x = golemPositions[i][0];
		model_golems[i].pos_y = golemPositions[i][1];
		model_golems[i].pos_z = golemPositions[i][2];
		model_golems[i].rot_x = 0.0f;
		model_golems[i].rot_y = 0.0f;
		model_golems[i].rot_z = 0.0f;
		model_golems[i].GenerateDisplayList();
	}
	printf("Golems Loaded.\n");
	model_golem.scale_xyz = 0.5f;
	model_golem.pos_x = -115.0f;
	model_golem.pos_y = 0.0f;
	model_golem.pos_z = 2418.0f;
	model_golem.rot_x = 0.0f;
	model_golem.rot_y = 180.0f;
	model_golem.rot_z = 0.0f;
	model_golem.GenerateDisplayList();
	printf("Golem Loaded.\n");

	// --- FLAME PRINCESS ---
	printf("Loading OBJ Model: fire Princess...\n");
	// FIXED: File path updated as requested
	model_flame_princess.Load("Models/firePrincess/flameprincess.obj", "Models/firePrincess/");
	for (auto& entry : model_flame_princess.materials) {
		entry.second.tex = tex_flame_princess;
		entry.second.hasTexture = true;
		entry.second.diffColor[0] = 1.0f;
		entry.second.diffColor[1] = 1.0f;
		entry.second.diffColor[2] = 1.0f;
	}
	model_flame_princess.scale_xyz = 0.5f;
	model_flame_princess.pos_x = -115.0f;
	model_flame_princess.pos_y = 0.0f;
	model_flame_princess.pos_z = 2422.0f;
	model_flame_princess.rot_x = 0.0f;
	model_flame_princess.rot_y = 180.0f;
	model_flame_princess.rot_z = 0.0f;
	model_flame_princess.GenerateDisplayList();
	printf("Flame Princess Loaded.\n");

	// --- FIRE ROCKS (Multiple) ---
	printf("Loading OBJ Model: Fire Rocks...\n");
	for (int i = 0; i < NUM_FIRE_ROCKS; i++)
	{
		model_fire_rocks[i].Load("Models/firerock/firerock.obj", "Models/firerock/");

		for (auto& entry : model_fire_rocks[i].materials) {
			std::string materialName = entry.first;
			if (materialName.find("20") != std::string::npos ||
				materialName.find("Rock20") != std::string::npos ||
				materialName.find("rock20") != std::string::npos) {
				entry.second.tex = tex_fire_rock_20;
				entry.second.hasTexture = true;
			}
			else if (materialName.find("0") != std::string::npos ||
				materialName.find("Rock0") != std::string::npos ||
				materialName.find("rock0") != std::string::npos) {
				entry.second.tex = tex_fire_rock_0;
				entry.second.hasTexture = true;
			}
			else {
				entry.second.tex = tex_fire_rock_20;
				entry.second.hasTexture = true;
			}
			entry.second.diffColor[0] = 1.0f;
			entry.second.diffColor[1] = 1.0f;
			entry.second.diffColor[2] = 1.0f;
		}

		model_fire_rocks[i].scale_xyz = 1.0f;
		model_fire_rocks[i].pos_x = fireRockPositions[i][0];
		model_fire_rocks[i].pos_y = fireRockPositions[i][1];
		model_fire_rocks[i].pos_z = fireRockPositions[i][2];
		model_fire_rocks[i].rot_x = 0.0f;
		model_fire_rocks[i].rot_y = 45.0f * i;  // Different rotation for variety
		model_fire_rocks[i].rot_z = 0.0f;
		model_fire_rocks[i].GenerateDisplayList();
	}
	printf("Fire Rocks Loaded.\n");
	model_fire_rock.scale_xyz = 1.0f;
	model_fire_rock.pos_x = -118.0f;
	model_fire_rock.pos_y = 0.0f;
	model_fire_rock.pos_z = 2414.0f;
	model_fire_rock.rot_x = 0.0f;
	model_fire_rock.rot_y = 45.0f;
	model_fire_rock.rot_z = 0.0f;
	model_fire_rock.GenerateDisplayList();
	printf("Fire Rock Loaded.\n");

	// --- ENCHIRIDION ---
	printf("Loading OBJ Model: Enchiridion...\n");
	model_enchiridion.Load("Models/enchiridion/enchiridion.obj", "Models/enchiridion/");
	tex_enchiridion_01.Load("Textures/texturesenchiridion/enchiridion_tex_map_01.bmp");
	tex_enchiridion_02.Load("Textures/texturesenchiridion/enchiridion_tex_map_02.bmp");
	tex_enchiridion_paper.Load("Textures/texturesenchiridion/LT_AntiquePaper_03.bmp");

	for (auto& entry : model_enchiridion.materials) {
		std::string materialName = entry.first;
		if (materialName.find("01") != std::string::npos ||
			materialName.find("_01") != std::string::npos ||
			materialName.find("Map01") != std::string::npos) {
			entry.second.tex = tex_enchiridion_01;
			entry.second.hasTexture = true;
		}
		else if (materialName.find("02") != std::string::npos ||
			materialName.find("_02") != std::string::npos ||
			materialName.find("Map02") != std::string::npos) {
			entry.second.tex = tex_enchiridion_02;
			entry.second.hasTexture = true;
		}
		else if (materialName.find("Paper") != std::string::npos ||
			materialName.find("paper") != std::string::npos ||
			materialName.find("Antique") != std::string::npos) {
			entry.second.tex = tex_enchiridion_paper;
			entry.second.hasTexture = true;
		}
		else {
			entry.second.tex = tex_enchiridion_01;
			entry.second.hasTexture = true;
		}
		entry.second.diffColor[0] = 1.0f;
		entry.second.diffColor[1] = 1.0f;
		entry.second.diffColor[2] = 1.0f;
	}

	model_enchiridion.scale_xyz = 4.0f;
	model_enchiridion.pos_x = -111.0f;
	model_enchiridion.pos_y = 3.0f;
	model_enchiridion.pos_z = 2480.0f;  // Far end, opposite from spawn (2400)
	model_enchiridion.GenerateDisplayList();
	printf("Enchiridion Loaded.\n");

	// --- LAVA HAMMERS (Multiple) ---
	printf("Loading OBJ Model: Lava Hammers...\n");
	for (int i = 0; i < NUM_LAVA_HAMMERS; i++)
	{
		model_lava_hammers[i].Load("Models/lavahammer/lavahammer.obj", "Models/lavahammer/");

		for (auto& entry : model_lava_hammers[i].materials) {
			std::string materialName = entry.first;
			if (materialName.find("Emissive") != std::string::npos ||
				materialName.find("emissive") != std::string::npos ||
				materialName.find("Glow") != std::string::npos) {
				entry.second.tex = tex_lava_hammer_emissive;
				entry.second.hasTexture = true;
			}
			else if (materialName.find("Metal") != std::string::npos ||
				materialName.find("metal") != std::string::npos) {
				entry.second.tex = tex_lava_hammer_metallic;
				entry.second.hasTexture = true;
			}
			else if (materialName.find("Rough") != std::string::npos ||
				materialName.find("rough") != std::string::npos) {
				entry.second.tex = tex_lava_hammer_roughness;
				entry.second.hasTexture = true;
			}
			else if (materialName.find("Normal") != std::string::npos ||
				materialName.find("normal") != std::string::npos) {
				entry.second.tex = tex_lava_hammer_normal;
				entry.second.hasTexture = true;
			}
			else {
				entry.second.tex = tex_lava_hammer_base;
				entry.second.hasTexture = true;
			}
			entry.second.diffColor[0] = 1.0f;
			entry.second.diffColor[1] = 1.0f;
			entry.second.diffColor[2] = 1.0f;
		}

		model_lava_hammers[i].scale_xyz = 1.0f;
		model_lava_hammers[i].pos_x = lavaHammerPositions[i][0];
		model_lava_hammers[i].pos_y = lavaHammerPositions[i][1];
		model_lava_hammers[i].pos_z = lavaHammerPositions[i][2];
		model_lava_hammers[i].rot_x = 0.0f;
		model_lava_hammers[i].rot_y = -30.0f + (i * 60.0f);  // Different rotations
		model_lava_hammers[i].rot_z = 0.0f;
		model_lava_hammers[i].GenerateDisplayList();
	}
	printf("Lava Hammers Loaded.\n");

	// --- DEMON SWORDS (Multiple Collectibles) ---
	printf("Loading OBJ Model: Demon Swords...\n");
	for (int i = 0; i < NUM_DEMON_SWORDS; i++)
	{
		model_demon_swords[i].Load("Models/demonsword/demonsword.obj", "Models/demonsword/");

		for (auto& entry : model_demon_swords[i].materials) {
			std::string materialName = entry.first;
			if (materialName.find("Albedo") != std::string::npos ||
				materialName.find("albedo") != std::string::npos ||
				materialName.find("Base") != std::string::npos ||
				materialName.find("base") != std::string::npos) {
				entry.second.tex = tex_demon_sword_albedo;
				entry.second.hasTexture = true;
			}
			else if (materialName.find("AO") != std::string::npos ||
				materialName.find("ao") != std::string::npos ||
				materialName.find("Occlusion") != std::string::npos) {
				entry.second.tex = tex_demon_sword_ao;
				entry.second.hasTexture = true;
			}
			else if (materialName.find("Gloss") != std::string::npos ||
				materialName.find("gloss") != std::string::npos) {
				entry.second.tex = tex_demon_sword_gloss;
				entry.second.hasTexture = true;
			}
			else if (materialName.find("Normal") != std::string::npos ||
				materialName.find("normal") != std::string::npos) {
				entry.second.tex = tex_demon_sword_normal;
				entry.second.hasTexture = true;
			}
			else if (materialName.find("Specular") != std::string::npos ||
				materialName.find("specular") != std::string::npos) {
				entry.second.tex = tex_demon_sword_specular;
				entry.second.hasTexture = true;
			}
			else {
				entry.second.tex = tex_demon_sword_albedo;
				entry.second.hasTexture = true;
			}
			entry.second.diffColor[0] = 1.0f;
			entry.second.diffColor[1] = 1.0f;
			entry.second.diffColor[2] = 1.0f;
		}

		model_demon_swords[i].scale_xyz = 0.05f;
		model_demon_swords[i].pos_x = demonSwordPositions[i][0];
		model_demon_swords[i].pos_y = demonSwordPositions[i][1];
		model_demon_swords[i].pos_z = demonSwordPositions[i][2];
		model_demon_swords[i].rot_x = 90.0f;  // Vertical
		model_demon_swords[i].rot_y = 0.0f;
		model_demon_swords[i].rot_z = 0.0f;
		model_demon_swords[i].GenerateDisplayList();

		demonSwordsVisible[i] = true;
		demonSwordRotations[i] = 0.0f;
		demonSwordBounceAngles[i] = i * 1.0f;  // Stagger animations
	}
	printf("Demon Swords Loaded.\n");
	model_lava_hammer.scale_xyz = 1.0f;
	model_lava_hammer.pos_x = -108.0f;
	model_lava_hammer.pos_y = 0.0f;
	model_lava_hammer.pos_z = 2416.0f;
	model_lava_hammer.rot_x = 0.0f;
	model_lava_hammer.rot_y = -30.0f;
	model_lava_hammer.rot_z = 0.0f;
	model_lava_hammer.GenerateDisplayList();
	printf("Lava Hammer Loaded.\n");

	// --- SKY ---
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
	tex_donut.Load("Textures/donut.bmp");

	for (auto& entry : model_donut.materials) {
		std::string name = entry.first;
		if (name.find("Icing") != std::string::npos || name.find("icing") != std::string::npos || name.find("Material.002") != std::string::npos)
		{
			entry.second.hasTexture = false;
			entry.second.diffColor[0] = 0.36f;
			entry.second.diffColor[1] = 0.20f;
			entry.second.diffColor[2] = 0.09f;
		}
		else if (name.find("Sprinkle") != std::string::npos || name.find("sprinkle") != std::string::npos)
		{
			entry.second.hasTexture = false;
			entry.second.diffColor[0] = 1.0f;
			entry.second.diffColor[1] = 0.0f;
			entry.second.diffColor[2] = 0.0f;
		}
		else
		{
			entry.second.tex = tex_donut;
			entry.second.hasTexture = true;
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

	// --- FINN (Candy) ---
	printf("Loading OBJ Model: Finn (Candy)...\n");
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

	// --- FINN (Fire Rescue) ---
	printf("Loading OBJ Model: Finn (Fire Rescue)...\n");
	model_finn_rescue.Load("Models/finn/Finn.obj", "Models/finn/"); // Reuse OBJ
	// Reuse texture settings
	for (auto& entry : model_finn_rescue.materials) {
		entry.second.tex = tex_finn;
		entry.second.hasTexture = true;
		entry.second.diffColor[0] = 1.0f;
		entry.second.diffColor[1] = 1.0f;
		entry.second.diffColor[2] = 1.0f;
	}
	// Position him in Fire Kingdom
	model_finn_rescue.scale_xyz = 0.1f;
	model_finn_rescue.pos_x = -111.0f;
	model_finn_rescue.pos_y = 0.0f;
	model_finn_rescue.pos_z = 2440.0f;  // Middle between 2400 (BMO) and 2480 (Enchiridion)
	model_finn_rescue.rot_y = 45.0f;

	model_finn_rescue.GenerateDisplayList();

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

	// --- ANIMATION LOGIC (LIGHTING & INTERACTION) ---

	// Day/Night Cycle (Sun Rotation)
	sunAngle += 0.005f;

	// Lava Flicker
	if (lavaDimming) {
		lavaIntensity -= 0.02f;
		if (lavaIntensity <= 0.5f) lavaDimming = false;
	}
	else {
		lavaIntensity += 0.02f;
		if (lavaIntensity >= 1.0f) lavaDimming = true;
	}

	// Jelly Squash Recovery (Return to normal shape)
	if (jellySquash < 1.0f) {
		jellySquash += 0.02f;
		if (jellySquash > 1.0f) jellySquash = 1.0f;
	}

	// Donut Shake Logic
	if (donutIsShaking) {
		donutShake += 0.1f;
		donutShakeTimer--;
		if (donutShakeTimer <= 0) {
			donutIsShaking = false;
			donutShake = 0.0f;
		}
	}

	// --- INTERACTION ANIMATION TIMERS ---
	// Update Cupcake Timers
	for (int i = 0; i < NUM_CUPCAKES; i++) {
		if (cupcakeAnimTimers[i] > 0) {
			cupcakeAnimTimers[i] += 1.0f;
			if (cupcakeAnimTimers[i] > 50.0f) {
				cupcakeVisible[i] = false; // Hide after animation
				cupcakeAnimTimers[i] = -1.0f; // Mark done
			}
		}
	}

	// Update Coin Timers
	for (int i = 0; i < NUM_COINS; i++) {
		if (coinAnimTimers[i] > 0) {
			coinAnimTimers[i] += 1.0f;
			if (coinAnimTimers[i] > 50.0f) {
				coinVisible[i] = false;
				coinAnimTimers[i] = -1.0f;
			}
		}
	}

	// Update Demon Sword Timers
	for (int i = 0; i < NUM_DEMON_SWORDS; i++) {
		if (swordAnimTimers[i] > 0) {
			swordAnimTimers[i] += 1.0f;
			if (swordAnimTimers[i] > 50.0f) {
				demonSwordsVisible[i] = false;
				swordAnimTimers[i] = -1.0f;
			}
		}
	}

	// Update Obstacle Reaction Timers (reset after animation)
	for (int i = 0; i < NUM_GOLEMS; i++) {
		if (golemAnimTimers[i] > 0) golemAnimTimers[i] += 0.5f; // animate
		// Reset or loop logic if needed, currently used for translate offset
	}
	for (int i = 0; i < NUM_FIRE_ROCKS; i++) {
		if (rockAnimTimers[i] > 0) rockAnimTimers[i] += 0.5f;
	}
	for (int i = 0; i < NUM_LAVA_HAMMERS; i++) {
		if (hammerAnimTimers[i] > 0) hammerAnimTimers[i] += 0.2f;
	}

	// Target Animations
	if (isFinnRescued) finnJumpTimer += 0.2f;
	if (enchiridionFound) enchiridionPulseTimer += 0.1f;

	// --- LICH MOVEMENT LOGIC ---
	if (currentLevel == LEVEL_CANDY)
	{
		// 1. Move the Lich along the X axis
		model_lich.pos_x += lichSpeed * lichDirection;

		// 2. Check boundaries (Too far right OR Too far left)
		// If he goes past (Start + Range) or (Start - Range), flip direction
		if (model_lich.pos_x > (lichStartX + lichRange))
		{
			lichDirection = -1; // Go Left
			// Rotate to face movement (180 is base X rotation fix, adding Y rotation)
			model_lich.rot_y = 270.0f;
		}
		else if (model_lich.pos_x < (lichStartX - lichRange))
		{
			lichDirection = 1;  // Go Right
			model_lich.rot_y = 90.0f;
		}

		// 3. Collision Detection (If BMO touches the Lich)
		float dx = model_bmo.pos_x - model_lich.pos_x;
		float dz = model_bmo.pos_z - model_lich.pos_z;
		float distance = sqrt(dx * dx + dz * dz);

		if (distance < 5.0f) // 5.0 is the hit radius
		{
			printf("BMO was caught by the Lich! Respawning...\n");

			// Push BMO back to start
			model_bmo.pos_x = 65.0f;
			model_bmo.pos_z = 55.0f;

			// Optional: Reduce score penalty
			if (score >= 5) score -= 5;
		}
	}

	// --- UPDATE FMOD ---
	fmodSystem->update();

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
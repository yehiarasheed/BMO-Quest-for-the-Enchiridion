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
GLdouble zFar = 1000; // Increased zFar slightly to accommodate larger map

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

// Model Variables
Model_OBJ model_donut;
Model_OBJ model_candy_kingdom;
Model_OBJ model_bmo;
Model_OBJ model_finn; 
Model_OBJ model_cupcake;
Model_OBJ model_coin;

// --- JELLY VARIABLES ---
Model_OBJ model_jelly;
GLTexture tex_jelly;
Model_OBJ model_lich; // Kept in case needed later, though unused currently

// Cupcake array for collectibles
const int NUM_CUPCAKES = 5;
Model_OBJ model_cupcakes[NUM_CUPCAKES];
bool cupcakeVisible[NUM_CUPCAKES];

// Animation variables
float cupcakeRotation = 0.0f;

// Collision detection radius
float collisionRadius = 1.2f;

// Jump state and movement
bool isJumping = false;
float jumpVelocity = 0.0f;
const float jumpStrength = 0.6f;
const float gravity = 0.03f;


// Textures
GLTexture tex_ground;
GLTexture tex_bmo;
GLTexture tex_cupcake;

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

// Try to move BMO to new position; if collides with jelly, apply bounce
bool TryMove(float newX, float newZ)
{
    if (CheckJellyCollision(newX, newZ))
    {
        // Bounce back from jelly
        float dx = model_bmo.pos_x - model_jelly.pos_x;
        float dz = model_bmo.pos_z - model_jelly.pos_z;
        float len = sqrt(dx*dx + dz*dz);
        float nx = 0.0f, nz = -1.0f;
        if (len > 0.001f) { nx = dx / len; nz = dz / len; }

        float pushBack = 2.0f;
        model_bmo.pos_x = model_jelly.pos_x + nx * (pushBack + 1.0f);
        model_bmo.pos_z = model_jelly.pos_z + nz * (pushBack + 1.0f);

        // vertical bounce
        if (!isJumping) {
            isJumping = true;
            jumpVelocity = jumpStrength * 0.9f;
        } else if (jumpVelocity < 0.0f) {
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
		gluLookAt(
			model_bmo.pos_x, model_bmo.pos_y + 2.0f, model_bmo.pos_z,
			model_bmo.pos_x + sin(model_bmo.rot_y * 3.14159 / 180.0), model_bmo.pos_y + 2.0f, model_bmo.pos_z - cos(model_bmo.rot_y * 3.14159 / 180.0),
			0, 1, 0
		);
	}
	else
	{
		float camDistance = 15.0f;
		float camHeight = 8.0f;
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

	model_coin.Draw();

	// ============================================
	// --- DRAW JELLY ---
	// ============================================

	glPushMatrix();

	// Apply texturing
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_jelly.texture[0]);
	glColor3f(1.0f, 1.0f, 1.0f);

	// Draw the geometry (Scale and Position handled in LoadAssets)
	model_jelly.Draw();

	glBindTexture(GL_TEXTURE_2D, 0);
	glPopMatrix();

	// ============================================

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
    // Check for shift (sprint)
    int mods = glutGetModifiers();
    if (mods & GLUT_ACTIVE_SHIFT) moveSpeed *= 1.8f;

	switch (button)
	{
	case 'z':
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		break;
	case 'x':
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	case 'c': case 'C':
		currentCamera = (currentCamera == FIRST_PERSON) ? THIRD_PERSON : FIRST_PERSON;
		break;
	
	// BMO movement (IJKL) - rotate BMO to face movement direction
    case 'w':
    case 'W':
		// Move forward in current direction
		{
			float newX = model_bmo.pos_x + sin(angle) * moveSpeed;
			float newZ = model_bmo.pos_z - cos(angle) * moveSpeed;
			if (TryMove(newX, newZ)) {
				CheckCupcakeCollisions();
			}
		}
		break;
	case 's':
    case 'S':
		// Move backward - rotate 180° to face backward, move, rotate back
		{
			model_bmo.rot_y += 180.0f;
			angle = model_bmo.rot_y * 3.14159 / 180.0;
			float newX = model_bmo.pos_x + sin(angle) * moveSpeed;
			float newZ = model_bmo.pos_z - cos(angle) * moveSpeed;
			model_bmo.rot_y -= 180.0f;
			if (TryMove(newX, newZ)) {
				CheckCupcakeCollisions();
			}
		}
		break;
    case 'a':
    case 'A':
        // Strafe left
        {
            float strafeAngle = model_bmo.rot_y - 90.0f;
            float sAngle = strafeAngle * 3.14159 / 180.0;
            float newX = model_bmo.pos_x + sin(sAngle) * moveSpeed;
            float newZ = model_bmo.pos_z - cos(sAngle) * moveSpeed;
            if (TryMove(newX, newZ)) {
                CheckCupcakeCollisions();
            }
        }
        break;
    case 'd':
    case 'D':
        // Strafe right
        {
            float strafeAngle = model_bmo.rot_y + 90.0f;
            float sAngle = strafeAngle * 3.14159 / 180.0;
            float newX = model_bmo.pos_x + sin(sAngle) * moveSpeed;
            float newZ = model_bmo.pos_z - cos(sAngle) * moveSpeed;
            if (TryMove(newX, newZ)) {
                CheckCupcakeCollisions();
            }
        }
        break;
    case ' ': // Space = jump
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
	case 'p':
	case 'P':
		printf("BMO: pos=(%.2f, %.2f, %.2f) scale=%.2f rot=%.2f\n", model_bmo.pos_x, model_bmo.pos_y, model_bmo.pos_z, model_bmo.scale_xyz, model_bmo.rot_y);
		printf("Finn: pos=(%.2f, %.2f, %.2f) scale=%.2f rot=%.2f\n", model_finn.pos_x, model_finn.pos_y, model_finn.pos_z, model_finn.scale_xyz, model_finn.rot_y);
		printf("Jelly: pos=(%.2f, %.2f, %.2f) scale=%.2f\n", model_jelly.pos_x, model_jelly.pos_y, model_jelly.pos_z, model_jelly.scale_xyz);
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
    int mods = glutGetModifiers();
    if (mods & GLUT_ACTIVE_SHIFT) moveSpeed *= 1.8f;

	switch (key)
	{
    case GLUT_KEY_UP:
        // Move forward
        {
            float newX = model_bmo.pos_x + sin(angle) * moveSpeed;
            float newZ = model_bmo.pos_z - cos(angle) * moveSpeed;
            if (TryMove(newX, newZ)) CheckCupcakeCollisions();
        }
        break;
    case GLUT_KEY_DOWN:
        // Move backward
        {
            float backAngle = model_bmo.rot_y + 180.0f;
            float a = backAngle * 3.14159 / 180.0;
            float newX = model_bmo.pos_x + sin(a) * moveSpeed;
            float newZ = model_bmo.pos_z - cos(a) * moveSpeed;
            if (TryMove(newX, newZ)) CheckCupcakeCollisions();
        }
        break;
    case GLUT_KEY_LEFT:
        // Strafe left
        {
            float strafeAngle = model_bmo.rot_y - 90.0f;
            float sAngle = strafeAngle * 3.14159 / 180.0;
            float newX = model_bmo.pos_x + sin(sAngle) * moveSpeed;
            float newZ = model_bmo.pos_z - cos(sAngle) * moveSpeed;
            if (TryMove(newX, newZ)) CheckCupcakeCollisions();
        }
        break;
    case GLUT_KEY_RIGHT:
        // Strafe right
        {
            float strafeAngle = model_bmo.rot_y + 90.0f;
            float sAngle = strafeAngle * 3.14159 / 180.0;
            float newX = model_bmo.pos_x + sin(sAngle) * moveSpeed;
            float newZ = model_bmo.pos_z - cos(sAngle) * moveSpeed;
            if (TryMove(newX, newZ)) CheckCupcakeCollisions();
        }
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

	// Positioning - place BMO close to jelly and cupcakes
	model_bmo.scale_xyz = 10.0f;
	model_bmo.pos_x = 65.0f;
	model_bmo.pos_z = 55.0f;
	model_bmo.rot_y = -90.0f;
	printf("BMO Ready.\n");

	// --- CUPCAKE ---
	printf("Loading OBJ Model: Cupcakes...\n");
	tex_cupcake.Load("Textures/cupcake.bmp");
	
	// Create multiple cupcakes at different positions
	// Using values from Main branch for larger map
	float cupcakeSpacing = 15.0f; // Distance between cupcakes
	float startZ = 60.0f;         // Start further out due to larger candy kingdom
	
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

	// --- COIN ---
	printf("Loading OBJ Model: Coin...\n");
	model_coin.Load("Models/coin/coin.obj", "Models/coin/");
	model_coin.scale_xyz = 150.0f;
	model_coin.pos_x = 65.0f;
	model_coin.pos_y = 1.0f;
	model_coin.pos_z = 12.0f;
	model_coin.GenerateDisplayList();
	printf("Coin Loaded.\n");

	// --- FINN ---
	printf("Loading OBJ Model: Finn...\n");
	model_finn.Load("Models/finn/Finn.obj", "Models/finn/");
	
	// Positioning - Using Main branch values
	model_finn.scale_xyz = 0.1f;
	model_finn.pos_x = 70.0f;
	model_finn.pos_y = 0.0f;
	model_finn.pos_z = 53.0f; // Just in front of BMO, near jelly and cupcakes
	model_finn.rot_y = -90.0f;   // Face same direction as BMO
				
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

	// ============================================
	// --- LOAD JELLY (COMBINED LOGIC) ---
	// ============================================
	printf("Loading OBJ Model: Jelly...\n");
	tex_jelly.Load("Textures/jelly.bmp");

	printf("DEBUG: Texture ID = %d\n", tex_jelly.texture[0]);

	model_jelly.Load("Models/jelly/jelly.obj", "Models/jelly/");

	// 2. We manually force the material to be ORANGE and disable any textures
	//    that might be defined in the .mtl file (like the star texture).
	for (auto& entry : model_jelly.materials) {
		entry.second.hasTexture = false; // Disable the star texture

		// Set RGB color to Orange
		entry.second.diffColor[0] = 1.0f; // Red = 1.0
		entry.second.diffColor[1] = 0.4f; // Green = 0.4 (Mixes to Orange)
		entry.second.diffColor[2] = 0.1f; // Blue = 0.1
	}

	model_jelly.GenerateDisplayList();

	// 3. Positioning (Using the larger map coordinates from Main)
	model_jelly.scale_xyz = 30.0f;
	model_jelly.pos_x = 80.0f;
	model_jelly.pos_y = 0.0f;
	model_jelly.pos_z = 50.0f;

	printf("Jelly Loaded (Restored to Orange).\n");

	// --- OTHER TEXTURES ---
	//tex_ground.Load("Textures/ground.bmp");
	//loadBMP(&tex, "Textures/blu-sky-3.bmp", true);
}

//=======================================================================
// Animation/Idle Function
//=======================================================================
void myIdle(void)
{
	if (isJumping) {
		model_bmo.pos_y += jumpVelocity;
		jumpVelocity -= gravity;
		if (model_bmo.pos_y <= 0.0f) {
			model_bmo.pos_y = 0.0f;
			isJumping = false;
			jumpVelocity = 0.0f;
		}
	}
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
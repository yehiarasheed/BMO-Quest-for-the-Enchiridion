#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <map>
#include <glut.h>
#include <stdio.h> // Required for fast C-style I/O
#include "GLTexture.h"

// Basic structures
struct ObjVector { float x, y, z; };
struct ObjTexCoord { float u, v; };
struct ObjNormal { float x, y, z; };

struct ObjMaterial {
    std::string name;
    GLTexture tex;
    bool hasTexture;
    float diffColor[3];
};

struct ObjGroup {
    std::string materialName;
    std::vector<int> vIndices;
    std::vector<int> uvIndices;
    std::vector<int> nIndices;
};

class Model_OBJ {
public:
    std::map<std::string, ObjMaterial> materials;
    std::vector<ObjGroup> groups;
    std::vector<ObjVector> vertices;
    std::vector<ObjTexCoord> uvs;
    std::vector<ObjNormal> normals;

    // Transformation variables
    float pos_x, pos_y, pos_z;
    float rot_x, rot_y, rot_z;
    float scale_xyz;

    // Display List ID for performance
    GLuint listID;

    Model_OBJ() {
        pos_x = pos_y = pos_z = 0;
        rot_x = rot_y = rot_z = 0;
        scale_xyz = 1.0;
        listID = 0;
    }

    void Load(const char* filename, const char* texturePathPrefix) {
        printf("Starting fast load for: %s\n", filename);

        FILE* file = fopen(filename, "r");
        if (file == NULL) {
            printf("Impossible to open the file !\n");
            return;
        }

        // Pre-allocate memory to speed up loading (guess-timate sizes)
        vertices.reserve(100000);
        uvs.reserve(100000);
        normals.reserve(100000);

        ObjGroup currentGroup;
        currentGroup.materialName = "default";

        char lineHeader[1024];
        int res;
        int lineCount = 0;

        while (1) {
            // Read the first word of the line
            res = fscanf(file, "%s", lineHeader);
            if (res == EOF) break;

            lineCount++;
            if (lineCount % 50000 == 0) printf("Processed %d lines...\n", lineCount);

            // Parse Vertex
            if (strcmp(lineHeader, "v") == 0) {
                ObjVector v;
                fscanf(file, "%f %f %f\n", &v.x, &v.y, &v.z);
                vertices.push_back(v);
            }
            // Parse UV
            else if (strcmp(lineHeader, "vt") == 0) {
                ObjTexCoord uv;
                fscanf(file, "%f %f\n", &uv.u, &uv.v);
                uvs.push_back(uv);
            }
            // Parse Normal
            else if (strcmp(lineHeader, "vn") == 0) {
                ObjNormal n;
                fscanf(file, "%f %f %f\n", &n.x, &n.y, &n.z);
                normals.push_back(n);
            }
            // Parse Material Library
            else if (strcmp(lineHeader, "mtllib") == 0) {
                char mtlFile[128];
                fscanf(file, "%s\n", mtlFile);
                std::string fullPath = std::string(texturePathPrefix) + mtlFile;
                LoadMaterials(fullPath.c_str(), texturePathPrefix);
            }
            // Parse Use Material
            else if (strcmp(lineHeader, "usemtl") == 0) {
                // Save old group if it has data
                if (!currentGroup.vIndices.empty()) {
                    groups.push_back(currentGroup);
                }
                char matName[128];
                fscanf(file, "%s\n", matName);
                currentGroup = ObjGroup(); // clear
                currentGroup.materialName = matName;
            }
            // Parse Face (Handles Triangles, Quads, and N-Gons)
            else if (strcmp(lineHeader, "f") == 0) {
                // 1. Read the rest of the line into a buffer
                char lineBuffer[1024];
                fgets(lineBuffer, 1024, file);

                // 2. We need temporary lists to store the indices for THIS face only
                std::vector<int> faceV, faceUV, faceN;

                // 3. Tokenize the line by space to get each "v/vt/vn" group
                char* token = strtok(lineBuffer, " \t\r\n");

                while (token != NULL) {
                    int v = 0, vt = 0, vn = 0;

                    // Count slashes to guess format
                    int slashes = 0;
                    for (int i = 0; token[i]; i++) if (token[i] == '/') slashes++;

                    // Parse based on format
                    if (slashes == 0) {
                        // Format: v
                        sscanf(token, "%d", &v);
                    }
                    else if (slashes == 1) {
                        // Format: v/vt
                        sscanf(token, "%d/%d", &v, &vt);
                    }
                    else if (strstr(token, "//")) {
                        // Format: v//vn
                        sscanf(token, "%d//%d", &v, &vn);
                    }
                    else {
                        // Format: v/vt/vn
                        sscanf(token, "%d/%d/%d", &v, &vt, &vn);
                    }

                    // Adjust OBJ 1-based indices to C++ 0-based
                    // Handle negative indices (relative) just in case
                    if (v > 0) v--; else if (v < 0) v = vertices.size() + v;
                    if (vt > 0) vt--; else if (vt < 0) vt = uvs.size() + vt;
                    if (vn > 0) vn--; else if (vn < 0) vn = normals.size() + vn;

                    faceV.push_back(v);
                    faceUV.push_back(vt);
                    faceN.push_back(vn);

                    token = strtok(NULL, " \t\r\n");
                }

                // 4. TRIANGULATE (Tessellate) the face
                // A triangle needs 3 vertices. If we have 4 (Quad), we make 2 triangles.
                // If we have 5, we make 3 triangles, etc.
                // Algorithm: Triangle Fan. Pivot around vertex 0.

                if (faceV.size() >= 3) {
                    for (size_t i = 1; i < faceV.size() - 1; i++) {
                        // Triangle Vertex A (Always the first vertex of the face)
                        currentGroup.vIndices.push_back(faceV[0]);
                        if (!faceUV.empty()) currentGroup.uvIndices.push_back(faceUV[0]);
                        if (!faceN.empty()) currentGroup.nIndices.push_back(faceN[0]);

                        // Triangle Vertex B (The current vertex i)
                        currentGroup.vIndices.push_back(faceV[i]);
                        if (!faceUV.empty()) currentGroup.uvIndices.push_back(faceUV[i]);
                        if (!faceN.empty()) currentGroup.nIndices.push_back(faceN[i]);

                        // Triangle Vertex C (The next vertex i+1)
                        currentGroup.vIndices.push_back(faceV[i + 1]);
                        if (!faceUV.empty()) currentGroup.uvIndices.push_back(faceUV[i + 1]);
                        if (!faceN.empty()) currentGroup.nIndices.push_back(faceN[i + 1]);
                    }
                }
            }
        }

        // Push final group
        if (!currentGroup.vIndices.empty()) groups.push_back(currentGroup);

        fclose(file);
        printf("OBJ Geometry loaded. Generating Display List...\n");
        GenerateDisplayList();
        printf("Model Ready.\n");
    }

    void LoadMaterials(const char* filename, const char* pathPrefix) {
        FILE* file = fopen(filename, "r");
        if (!file) { printf("Could not open MTL: %s\n", filename); return; }

        char lineHeader[128];
        std::string currentMtl;

        while (fscanf(file, "%s", lineHeader) != EOF) {
            if (strcmp(lineHeader, "newmtl") == 0) {
                char mtlName[128];
                fscanf(file, "%s\n", mtlName);
                currentMtl = mtlName;
                materials[currentMtl].name = currentMtl;
                materials[currentMtl].hasTexture = false;
                materials[currentMtl].diffColor[0] = 1.0;
                materials[currentMtl].diffColor[1] = 1.0;
                materials[currentMtl].diffColor[2] = 1.0;
            }
            else if (strcmp(lineHeader, "Kd") == 0) {
                float r, g, b;
                fscanf(file, "%f %f %f\n", &r, &g, &b);
                materials[currentMtl].diffColor[0] = r;
                materials[currentMtl].diffColor[1] = g;
                materials[currentMtl].diffColor[2] = b;
            }
            else if (strcmp(lineHeader, "map_Kd") == 0) {
                char texName[1024];
                fscanf(file, "%s\n", texName);
                std::string fullPath = std::string(pathPrefix) + texName;
                materials[currentMtl].tex.Load((char*)fullPath.c_str());
                materials[currentMtl].hasTexture = true;
            }
        }
        fclose(file);
    }

    // This compiles the geometry into GPU memory once
    void GenerateDisplayList() {
        listID = glGenLists(1);
        glNewList(listID, GL_COMPILE);

        for (size_t i = 0; i < groups.size(); i++) {
            ObjGroup& g = groups[i];

            bool useAutoUV = false;
            if (materials.count(g.materialName)) {
                ObjMaterial& mat = materials[g.materialName];
                if (mat.hasTexture) {
                    glEnable(GL_TEXTURE_2D);
                    glColor3f(1, 1, 1);
                    glBindTexture(GL_TEXTURE_2D, mat.tex.texture[0]);
                    // If material claims a texture but geometry has no UVs, we'll generate simple planar UVs
                    if (g.uvIndices.empty()) useAutoUV = true;
                }
                else {
                    glDisable(GL_TEXTURE_2D);
                    glColor3fv(mat.diffColor);
                }
            }
            else {
                // --- FIX APPLIED HERE ---
                // I commented out the Disable command. 
                // This lets us force the texture externally in Main.cpp!

                // glDisable(GL_TEXTURE_2D); // <--- THIS WAS THE PROBLEM LINE

                glColor3f(1, 1, 1);
            }

            glBegin(GL_TRIANGLES);
            for (size_t k = 0; k < g.vIndices.size(); k++) {
                // Apply Normals
                if (k < g.nIndices.size() && g.nIndices[k] < normals.size()) {
                    ObjNormal& n = normals[g.nIndices[k]];
                    glNormal3f(n.x, n.y, n.z);
                }
                // Apply UVs (use provided UVs if available, otherwise fallback to planar mapping)
                if (!useAutoUV) {
                    if (k < g.uvIndices.size() && g.uvIndices[k] < uvs.size()) {
                        ObjTexCoord& u = uvs[g.uvIndices[k]];
                        glTexCoord2f(u.u, u.v);
                    }
                } else {
                    // planar mapping using X,Z world coords (may need scaling)
                    if (g.vIndices[k] < vertices.size()) {
                        ObjVector& v = vertices[g.vIndices[k]];
                        float tu = v.x * 0.1f;
                        float tv = v.z * 0.1f;
                        glTexCoord2f(tu, tv);
                    }
                }
                // Apply Vertex
                if (g.vIndices[k] < vertices.size()) {
                    ObjVector& v = vertices[g.vIndices[k]];
                    glVertex3f(v.x, v.y, v.z);
                }
            }
            glEnd();
        }
        glEndList();
    }

    void Draw() {
        if (listID == 0) return; // Not loaded yet

        glPushMatrix();
        glTranslatef(pos_x, pos_y, pos_z);
        glRotatef(rot_x, 1, 0, 0);
        glRotatef(rot_y, 0, 1, 0);
        glRotatef(rot_z, 0, 0, 1);
        glScalef(scale_xyz, scale_xyz, scale_xyz);

        // This is now super fast
        glCallList(listID);

        glPopMatrix();
        glColor3f(1, 1, 1);
        glEnable(GL_TEXTURE_2D);
    }
};
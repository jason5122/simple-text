#import "BoingRenderer.h"
#import "util/file_util.h"
#import "util/shader_util.h"
#import <GLKit/GLKit.h>

// Shaders
enum { PROGRAM_LIGHTING, PROGRAM_PASSTHRU, NUM_PROGRAMS };

enum {
    UNIFORM_MVP,
    UNIFORM_MODELVIEW,
    UNIFORM_MODELVIEWIT,
    UNIFORM_LIGHTDIR,
    UNIFORM_AMBIENT,
    UNIFORM_DIFFUSE,
    UNIFORM_SPECULAR,
    UNIFORM_SHININESS,
    UNIFORM_CONSTANT_COLOR,
    NUM_UNIFORMS
};

enum { ATTRIB_VERTEX, ATTRIB_COLOR, ATTRIB_NORMAL, NUM_ATTRIBS };

typedef struct {
    char *vert, *frag;
    GLint uniform[NUM_UNIFORMS];
    GLuint id;
} programInfo_t;

programInfo_t program[NUM_PROGRAMS] = {
    {"lighting.vsh", "color.fsh"},  // PROGRAM_LIGHTING
    {"color.vsh", "color.fsh"},     // PROGRAM_PASSTHRU
};

typedef struct {
    float x;
    float y;
    float z;
    float nx;
    float ny;
    float nz;
    float r;
    float g;
    float b;
    float a;
} Vertex;

static float lightDir[3] = {0.8, 4.0, 1.0};
static float ambient[4] = {0.35, 0.35, 0.35, 0.35};
static float diffuse[4] = {1.0 - 0.35, 1.0 - 0.35, 1.0 - 0.35, 1.0};
static float specular[4] = {0.8, 0.8, 0.8, 1.0};
static float shininess = 10.0;

@interface BoingRenderer () {
    float angle;
    float angleDelta;
    float r;
    float xPos, yPos;
    float xVelocity, yVelocity;
    float scaleFactor;
    float yDecay;

    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint shaderProgram;

    GLKVector3 lightDirNormalized;
    GLKMatrix4 projectionMatrix;
    GLuint vboId, vaoId;
}
@end

@implementation BoingRenderer

- (void)generateBoingData {
    int x;
    int index = 0;

    float v1x, v1y, v1z;
    float v2x, v2y, v2z;
    float d;

    int theta, phi;

    float theta0, theta1;
    float phi0, phi1;

    Vertex quad[4];

    Vertex* boingData = malloc(8 * 16 * 6 * sizeof(Vertex));

    float delta = M_PI / 8.0f;

    // 8 vertical segments
    for (theta = 0; theta < 8; theta++) {
        theta0 = theta * delta;
        theta1 = (theta + 1) * delta;

        // 16 horizontal segments
        for (phi = 0; phi < 16; phi++) {
            phi0 = phi * delta;
            phi1 = (phi + 1) * delta;

            // Generate 4 points per quad
            quad[0].x = r * sin(theta0) * cos(phi0);
            quad[0].y = r * cos(theta0);
            quad[0].z = r * sin(theta0) * sin(phi0);

            quad[1].x = r * sin(theta0) * cos(phi1);
            quad[1].y = r * cos(theta0);
            quad[1].z = r * sin(theta0) * sin(phi1);

            quad[2].x = r * sin(theta1) * cos(phi1);
            quad[2].y = r * cos(theta1);
            quad[2].z = r * sin(theta1) * sin(phi1);

            quad[3].x = r * sin(theta1) * cos(phi0);
            quad[3].y = r * cos(theta1);
            quad[3].z = r * sin(theta1) * sin(phi0);

            // Generate normal
            if (theta >= 4) {
                v1x = quad[1].x - quad[0].x;
                v1y = quad[1].y - quad[0].y;
                v1z = quad[1].z - quad[0].z;

                v2x = quad[3].x - quad[0].x;
                v2y = quad[3].y - quad[0].y;
                v2z = quad[3].z - quad[0].z;
            } else {
                v1x = quad[0].x - quad[3].x;
                v1y = quad[0].y - quad[3].y;
                v1z = quad[0].z - quad[3].z;

                v2x = quad[2].x - quad[3].x;
                v2y = quad[2].y - quad[3].y;
                v2z = quad[2].z - quad[3].z;
            }

            quad[0].nx = (v1y * v2z) - (v2y * v1z);
            quad[0].ny = (v1z * v2x) - (v2z * v1x);
            quad[0].nz = (v1x * v2y) - (v2x * v1y);

            d = 1.0f /
                sqrt(quad[0].nx * quad[0].nx + quad[0].ny * quad[0].ny + quad[0].nz * quad[0].nz);

            quad[0].nx *= d;
            quad[0].ny *= d;
            quad[0].nz *= d;

            // Generate color
            if ((theta ^ phi) & 1) {
                quad[0].r = 1.0f;
                quad[0].g = 1.0f;
                quad[0].b = 1.0f;
                quad[0].a = 1.0f;
            } else {
                quad[0].r = 1.0f;
                quad[0].g = 0.0f;
                quad[0].b = 0.0f;
                quad[0].a = 1.0f;
            }

            // Replicate vertex info
            for (x = 1; x < 4; x++) {
                quad[x].nx = quad[0].nx;
                quad[x].ny = quad[0].ny;
                quad[x].nz = quad[0].nz;
                quad[x].r = quad[0].r;
                quad[x].g = quad[0].g;
                quad[x].b = quad[0].b;
                quad[x].a = quad[0].a;
            }

            // OpenGL draws triangles under the hood. Core Profile officially
            // drops support of the GL_QUADS mode in the glDrawArrays/Elements
            // calls. Store vertices as in two consisting triangles
            boingData[index++] = quad[0];
            boingData[index++] = quad[1];
            boingData[index++] = quad[2];

            boingData[index++] = quad[2];
            boingData[index++] = quad[3];
            boingData[index++] = quad[0];
        }
    }

    // Create a VAO (vertex array object).
    glGenVertexArrays(1, &vaoId);
    glBindVertexArray(vaoId);

    // Create a VBO (vertex buffer object) to hold our data.
    glGenBuffers(1, &vboId);
    glBindBuffer(GL_ARRAY_BUFFER, vboId);
    glBufferData(GL_ARRAY_BUFFER, 8 * 16 * 6 * sizeof(Vertex), boingData, GL_STATIC_DRAW);

    // positions
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (GLubyte*)(uintptr_t)offsetof(Vertex, x));

    // colors
    glEnableVertexAttribArray(ATTRIB_COLOR);
    glVertexAttribPointer(ATTRIB_COLOR, 4, GL_FLOAT, GL_TRUE, sizeof(Vertex),
                          (GLubyte*)(uintptr_t)offsetof(Vertex, r));

    // normals
    glEnableVertexAttribArray(ATTRIB_NORMAL);
    glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (GLubyte*)(uintptr_t)offsetof(Vertex, nx));

    // At this point the VAO is set up with three vertex attributes referencing
    // the same buffer object.

    free(boingData);
}

- (void)setupShaders {
    for (int i = 0; i < NUM_PROGRAMS; i++) {
        char* vsrc = readFile(pathForResource(program[i].vert));
        char* fsrc = readFile(pathForResource(program[i].frag));
        GLsizei attribCt = 0;
        GLchar* attribUsed[NUM_ATTRIBS];
        GLint attrib[NUM_ATTRIBS];
        GLchar* attribName[NUM_ATTRIBS] = {
            "inVertex",
            "inColor",
            "inNormal",
        };
        const GLchar* uniformName[NUM_UNIFORMS] = {
            "MVP",     "ModelView", "ModelViewIT", "lightDir",      "ambient",
            "diffuse", "specular",  "shininess",   "constantColor",
        };

        // auto-assign known attribs
        for (int j = 0; j < NUM_ATTRIBS; j++) {
            if (strstr(vsrc, attribName[j])) {
                attrib[attribCt] = j;
                attribUsed[attribCt++] = attribName[j];
            }
        }

        glueCreateProgram(vsrc, fsrc, attribCt, (const GLchar**)&attribUsed[0], attrib,
                          NUM_UNIFORMS, &uniformName[0], program[i].uniform, &program[i].id);
        free(vsrc);
        free(fsrc);

        // set constant uniforms
        glUseProgram(program[i].id);

        if (i == PROGRAM_LIGHTING) {
            // Set up lighting stuff used by the shaders
            glUniform3fv(program[i].uniform[UNIFORM_LIGHTDIR], 1, lightDirNormalized.v);
            glUniform4fv(program[i].uniform[UNIFORM_AMBIENT], 1, ambient);
            glUniform4fv(program[i].uniform[UNIFORM_DIFFUSE], 1, diffuse);
            glUniform4fv(program[i].uniform[UNIFORM_SPECULAR], 1, specular);
            glUniform1f(program[i].uniform[UNIFORM_SHININESS], shininess);
        } else if (i == PROGRAM_PASSTHRU) {
            glUniform4f(program[i].uniform[UNIFORM_CONSTANT_COLOR], 0.0f, 0.0f, 0.0f, 0.4f);
        }
    }
}

- (id)init {
    if (self = [super init]) {
        angleDelta = -0.05f;
        scaleFactor = 2.0f;
        r = scaleFactor * 48.0f;

        xVelocity = 8.0f;
        yVelocity = -10.0f;
        xPos = r * 2.0f;
        yPos = r * 6.0f;

        yDecay = -1.0f;

        // normalize light dir
        lightDirNormalized = GLKVector3Normalize(GLKVector3MakeWithArray(lightDir));

        projectionMatrix = GLKMatrix4Identity;

        [self generateBoingData];

        [self setupShaders];
    }
    return self;
}

- (void)makeOrthographicForWidth:(CGFloat)width height:(CGFloat)height {
    projectionMatrix = GLKMatrix4MakeOrtho(0, width, 0, height, 0.0f, 2000.0);
}

- (void)updateForWidth:(CGFloat)width height:(CGFloat)height {
    yVelocity -= 0.4f;

    xPos += xVelocity * scaleFactor;
    yPos += yVelocity * scaleFactor;

    if (xPos < (r + 10.0f)) {
        xPos = r + 10.f;
        xVelocity = -xVelocity;
        angleDelta = -angleDelta;
    } else if (xPos > (width * scaleFactor - r)) {
        xPos = width * scaleFactor - r;
        xVelocity = -xVelocity;
        angleDelta = -angleDelta;
    }
    if (yPos < r) {
        yPos = r;
        yVelocity = -yVelocity + yDecay;
    } else if (yPos > (height * scaleFactor - r)) {
        yPos = height * scaleFactor - r;
        yVelocity = -yVelocity + yDecay;
    }

    angle += angleDelta;
    // if (angle < 0.0f) angle += 360.0f;
    // else if (angle > 360.0f) angle -= 360.0f;
}

- (void)render {
    GLKMatrix4 modelViewMatrix, MVPMatrix, modelViewMatrixIT;
    GLKMatrix3 normalMatrix;

    glBindVertexArray(vaoId);

    // Draw "shadow"
    glUseProgram(program[PROGRAM_PASSTHRU].id);

    glEnable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE_MINUS_SRC_ALPHA);

    // Make the "shadow" move around a bit. This is not a real shadow
    // projection.
    GLKVector3 pos = GLKVector3Normalize(GLKVector3Make(xPos, yPos, -100.0f));
    modelViewMatrix =
        GLKMatrix4MakeTranslation(xPos + (pos.v[0] - lightDirNormalized.v[0]) * 20.0,
                                  yPos + (pos.v[1] - lightDirNormalized.v[1]) * 10.0, -800.0f);
    modelViewMatrix = GLKMatrix4Rotate(modelViewMatrix, -16.0f, 0.0f, 0.0f, 1.0f);
    modelViewMatrix = GLKMatrix4Rotate(modelViewMatrix, angle, 0.0f, 1.0f, 0.0f);
    modelViewMatrix = GLKMatrix4Scale(modelViewMatrix, 1.05f, 1.05f, 1.05f);

    MVPMatrix = GLKMatrix4Multiply(projectionMatrix, modelViewMatrix);
    glUniformMatrix4fv(program[PROGRAM_PASSTHRU].uniform[UNIFORM_MVP], 1, GL_FALSE, MVPMatrix.m);

    glDrawArrays(GL_TRIANGLES, 0, 8 * 16 * 6);

    // Draw real Boing
    glUseProgram(program[PROGRAM_LIGHTING].id);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);

    // ModelView
    modelViewMatrix = GLKMatrix4MakeTranslation(xPos, yPos, -100.0f);
    modelViewMatrix = GLKMatrix4Rotate(modelViewMatrix, -16.0f, 0.0f, 0.0f, 1.0f);
    modelViewMatrix = GLKMatrix4Rotate(modelViewMatrix, angle, 0.0f, 1.0f, 0.0f);
    glUniformMatrix4fv(program[PROGRAM_LIGHTING].uniform[UNIFORM_MODELVIEW], 1, GL_FALSE,
                       modelViewMatrix.m);

    // MVP
    MVPMatrix = GLKMatrix4Multiply(projectionMatrix, modelViewMatrix);
    glUniformMatrix4fv(program[PROGRAM_LIGHTING].uniform[UNIFORM_MVP], 1, GL_FALSE, MVPMatrix.m);

    // ModelViewIT (normal matrix)
    bool success;
    modelViewMatrixIT = GLKMatrix4InvertAndTranspose(modelViewMatrix, &success);
    if (success) {
        normalMatrix = GLKMatrix4GetMatrix3(modelViewMatrixIT);
        glUniformMatrix3fv(program[PROGRAM_LIGHTING].uniform[UNIFORM_MODELVIEWIT], 1, GL_FALSE,
                           normalMatrix.m);
    }

    glDrawArrays(GL_TRIANGLES, 0, 8 * 16 * 6);

    glUseProgram(0);
}

- (void)dealloc {
    if (vboId) {
        glDeleteBuffers(1, &vboId);
        vboId = 0;
    }
    if (vaoId) {
        glDeleteVertexArrays(1, &vaoId);
        vaoId = 0;
    }
    if (vertexShader) {
        glDeleteShader(vertexShader);
        vertexShader = 0;
    }
    if (fragmentShader) {
        glDeleteShader(fragmentShader);
        fragmentShader = 0;
    }
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
}

@end

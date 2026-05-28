#include "raylib.h"
#include <math.h>
#include <stdlib.h>

// if i make these too high the game starts lagging bad
#define MAX_MISSILES 60
#define MAX_INTERCEPTORS 15
#define MAX_EXPLOSIONS 30
#define MAX_LIVES 3 
#define MAX_STARS 80

// defining my own types to keep data grouped together
// using structs makes it easier to pass data to functions later
typedef struct { Vector2 pos; Vector2 speed; bool active; } Missile;
typedef struct { Vector2 pos; Vector2 speed; Vector2 target; bool active; } Interceptor;
typedef struct { Vector2 pos; float radius; int life; bool active; } Explosion;
typedef struct { Vector2 pos; float speed; } Star;

// global arrays to hold all the game objects
// initializing to zero so everything starts inactive
// this is the object pool i talked abot in the video
Missile missiles[MAX_MISSILES] = {0};
Interceptor interceptors[MAX_INTERCEPTORS] = {0};
Explosion explosions[MAX_EXPLOSIONS] = {0};
Star stars[MAX_STARS] = {0};

// global variables for tracking the game state
int screenW = 450;
int screenH = 600; 
int score = 0;
int health = MAX_LIVES;
float gameTimer = 0.0f;
float shotCooldown = 0.0f;
bool gameOver = false;

// starting position for the aim dot
// put it in the middle so player isnt lost at start
Vector2 crosshair = { 225, 300 };

// this makes the space background look better with moving dots
void InitStars() {
    for (int i = 0; i < MAX_STARS; i++) {
        // give every star a random spot on the screen
        stars[i].pos = (Vector2){ GetRandomValue(0, screenW), GetRandomValue(0, screenH) };
        // some stars move fastr than others to make it look like 3d space
        stars[i].speed = (float)GetRandomValue(1, 4) / 10.0f; 
    }
}

// handles spawning enamy missiles at the top
void SpawnMissile() {
    for (int i = 0; i < MAX_MISSILES; i++) {
        // only use a slot if its not already being used
        if (!missiles[i].active) {
            missiles[i].active = true;
            // start at random x but slightly above screen so they dont just pop in
            missiles[i].pos = (Vector2){ GetRandomValue(10, screenW - 10), -10 };
            Vector2 target = { GetRandomValue(10, screenW - 10), screenH };
            
            // math for moving towards the bottom
            // dx and dy is just the distance between start and end points
            float dx = target.x - missiles[i].pos.x;
            float dy = target.y - missiles[i].pos.y;
            float len = sqrt(dx*dx + dy*dy); // pythagoras to get the distance for speed math
            
            // normalizing the vector so speed is always same no matter the angle
            // if i dont divide by len they go way too fast
            // this is the math "phenomenon" i explaind in the script
            missiles[i].speed = (Vector2){ (dx/len) * 1.2f, (dy/len) * 1.2f };
            break; 
        }
    }
}

int main(void) {
    // setup the window and fps
    InitWindow(screenW, screenH, "missile command");
    InitStars();
    SetTargetFPS(60);

    // main loop runs every frame
    while (!WindowShouldClose()) {
        if (!gameOver) {
            // keep track of how long player survived for the timer ui
            gameTimer += GetFrameTime();
            
            // stop player from spamming spacebar too fast by reducing cooldown each frame
            if (shotCooldown > 0) shotCooldown -= GetFrameTime();
            
            // frequency logic - starts easy and gets fastr
            // i think i fixed the math here so it doesnt go negative
            int spawnFreq = 120 - ((int)gameTimer * 2); 
            if (spawnFreq < 45) spawnFreq = 45; 

            if ((int)(gameTimer * 60) % spawnFreq == 0) SpawnMissile();

            // player movement logic using arrow keys
            // added screen limits so the aim dot doesnt fly off the window
            if (IsKeyDown(KEY_RIGHT) && crosshair.x < screenW) crosshair.x += 5;
            if (IsKeyDown(KEY_LEFT) && crosshair.x > 0) crosshair.x -= 5;
            if (IsKeyDown(KEY_UP) && crosshair.y > 0) crosshair.y -= 5;
            if (IsKeyDown(KEY_DOWN) && crosshair.y < screenH - 50) crosshair.y += 5;

            // shoot logic
            if (IsKeyPressed(KEY_SPACE) && shotCooldown <= 0) {
                for (int i = 0; i < MAX_INTERCEPTORS; i++) {
                    if (!interceptors[i].active) {
                        interceptors[i].active = true;
                        // shoot from the middle base box at the bottom
                        interceptors[i].pos = (Vector2){ screenW/2, screenH - 45 };
                        interceptors[i].target = crosshair;
                        
                        // calculate directon for the interceptor to fly towards the aim
                        float dx = crosshair.x - interceptors[i].pos.x;
                        float dy = crosshair.y - interceptors[i].pos.y;
                        float len = sqrt(dx*dx + dy*dy);
                        
                        // these move way faster than enemy missiles so you can react
                        interceptors[i].speed = (Vector2){ (dx/len) * 8.5f, (dy/len) * 8.5f };
                        shotCooldown = 0.35f; 
                        break;
                    }
                }
            }

            // update stars for the scrolling effect
            for (int i = 0; i < MAX_STARS; i++) {
                stars[i].pos.y += stars[i].speed;
                // if star goes off bottom put it back to top for infinite loop
                if (stars[i].pos.y > screenH) stars[i].pos.y = 0; 
            }

            // update missiles pos and check if they hit the bottom defense line
            for (int i = 0; i < MAX_MISSILES; i++) {
                if (!missiles[i].active) continue;
                missiles[i].pos.x += missiles[i].speed.x;
                missiles[i].pos.y += missiles[i].speed.y;
                
                // if it goes past the jagged line you lose life
                if (missiles[i].pos.y > screenH - 40) {
                    missiles[i].active = false; // remove the missile
                    health--; // loose life
                    if (health <= 0) gameOver = true; 
                }
            }

            // move the shots up the screen
            for (int i = 0; i < MAX_INTERCEPTORS; i++) {
                if (!interceptors[i].active) continue;
                interceptors[i].pos.x += interceptors[i].speed.x;
                interceptors[i].pos.y += interceptors[i].speed.y;
                
                // when shot hits the crosshair point it explodes
                if (CheckCollisionPointCircle(interceptors[i].pos, interceptors[i].target, 8.0f)) {
                    for(int e=0; e<MAX_EXPLOSIONS; e++) {
                        if(!explosions[e].active) {
                            // spawn the explosion bubble at the point of impact
                            explosions[e] = (Explosion){interceptors[i].pos, 2.0f, 0, true};
                            break;
                        }
                    }
                    interceptors[i].active = false; 
                }
            }

            // expand the bubble and check if it touches a missile
            for (int i = 0; i < MAX_EXPLOSIONS; i++) {
                if (!explosions[i].active) continue;
                explosions[i].radius += 1.8f; // make bubble bigger every frame
                // life counter so the explosion eventually dissapears
                if (++explosions[i].life > 32) explosions[i].active = false; 
                
                // check collision with every active missile to see if we got a kill
                for (int j = 0; j < MAX_MISSILES; j++) {
                    if (missiles[j].active && CheckCollisionPointCircle(missiles[j].pos, explosions[i].pos, explosions[i].radius)) {
                        missiles[j].active = false; // annhilate missile
                        score += 150; 
                    }
                }
            }
        }

        // drawing section starts here
        BeginDrawing();
        ClearBackground((Color){ 5, 5, 15, 255 }); 

        // draw background stars first 
        for (int i = 0; i < MAX_STARS; i++) DrawCircleV(stars[i].pos, 1, GRAY);

        // deciding what color to draw the defense line
        Color lineCol = GREEN;
        if (health == 2) lineCol = YELLOW;
        else if (health == 1) lineCol = RED;

        // drawing the jagged dash line at bottom 
        // i used modulo here for the jagged shape look
        for (int x = 0; x < screenW; x += 10) {
            float yOffset = (x % 20 == 0) ? -3 : 3; 
            DrawLineEx((Vector2){(float)x, screenH - 40 + yOffset}, 
                       (Vector2){(float)x + 6, screenH - 40 + yOffset}, 2.0f, lineCol);
        }

        // draw enemy projectiles red 
        for (int i = 0; i < MAX_MISSILES; i++) if (missiles[i].active) DrawCircleV(missiles[i].pos, 4, MAROON);
        
        // draw the player shots 
        for (int i = 0; i < MAX_INTERCEPTORS; i++) if (interceptors[i].active) DrawCircleV(interceptors[i].pos, 2, GOLD);
        
        // draw the explosions bubbles
        for (int i = 0; i < MAX_EXPLOSIONS; i++) if (explosions[i].active) DrawCircleLines(explosions[i].pos.x, explosions[i].pos.y, explosions[i].radius, lineCol);

        // draw the shooter box
        DrawRectangle(screenW/2 - 20, screenH - 40, 40, 40, DARKGRAY);
        DrawCircleLines(crosshair.x, crosshair.y, 12, WHITE);

        // showing the stats on screen 
        DrawText(TextFormat("TIME: %.1f", gameTimer), 10, 10, 20, RAYWHITE);
        DrawText(TextFormat("SCORE: %d", score), 10, 35, 20, GRAY);
        DrawText(TextFormat("LIVES: %d", health), 340, 10, 20, lineCol);

        if (gameOver) {
            DrawText("DEFENSE BREACHED", 100, screenH/2, 25, RED);
            DrawText("Press R to Restart", 120, screenH/2 + 40, 20, LIGHTGRAY);
            
            // restart logic to reset variables
            // this fixes a bug i had where the score stayed high
            if (IsKeyPressed(KEY_R)) {
                score = 0;
                health = MAX_LIVES;
                gameTimer = 0.0f;
                gameOver = false;
                for (int i = 0; i < MAX_MISSILES; i++) missiles[i].active = false;
                for (int i = 0; i < MAX_INTERCEPTORS; i++) interceptors[i].active = false;
                for (int i = 0; i < MAX_EXPLOSIONS; i++) explosions[i].active = false;
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
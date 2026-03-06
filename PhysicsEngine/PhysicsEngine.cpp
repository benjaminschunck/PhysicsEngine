#include "PhysicsEngine.h"
#include "raylib.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <string>

//structs
struct Body2D {
	Vector2 position;
	Vector2 velocity;
	float radius;
	Color color;
	bool isStatic;
};

struct World {
	float restitution = 0.7f;
	Vector2 gravity = { 0.0f, 5000.0f };
	float groundDampingPerSecond = 6.0f;
};

struct Slider {
	Rectangle track;
	float minVal;
	float maxVal;
	float currentVal;

	float getHandleX() const {
		float t = (currentVal - minVal) / (maxVal - minVal);
		return track.x + t * track.width;
	}

	Rectangle getHandle() const {
		return { getHandleX() - 8.0f, track.y - 8.0f, 16.0f, 16.0f + track.height };
	}

};

// Function declarations
void updateObjects(Body2D& body, float dt, const World& world, float worldWidth, float worldHeight);
void render(const Body2D& body);
float getRandomVelo();

int main()
{

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(800, 600, "PhysicsEngine");
	MaximizeWindow();

	//Build Objects
	std::vector<Body2D> bodies;

	World world;

	const float WORLD_WIDTH = 5000.0f;
	const float WORLD_HEIGHT = 3000.0f;

	Rectangle worldBounds = { 0.0f, 0.0f, WORLD_WIDTH, WORLD_HEIGHT };

	Texture2D button = LoadTexture("C:\\dev\\PhysicsEngine\\resources\\button.png");

	float height = GetScreenHeight();
	float width = GetScreenWidth();

	Rectangle destRec = {
	width / 2.0f - 50.0f,   // x (centered)
	0.0f,					// top of the screen
	75.0f,                 // desired width
	75.0f                  // desired height
	};

	Rectangle sourceRec = { 0, 0, (float)button.width, (float)button.height };


	Rectangle btnBounds = destRec;

	Vector2 origin = { 0.0f, 0.0f };

	int btnState = 0;
	bool btnAction = false;
	bool paused = true;

	float ballSize = 30.0f;

	Vector2 mousePoint = { 0.0f, 0.0f };

	Camera2D camera = {};
	camera.target = { width / 2.0f, height / 2.0f };
	camera.offset = { width / 2.0f, height / 2.0f };
	camera.rotation = 0.0f;
	camera.zoom = 1.0f;

	const float MENU_WIDTH = 220.0f;
	bool menuOpen = false;
	
	Slider ballSizeSlider = {
		.track = { 20.0f, 80.0f, MENU_WIDTH - 40.0f, 4.0f },
		.minVal = 5.0f,
		.maxVal = 100.0f,
		.currentVal = ballSize
	};
	
	bool draggingSlider = false;

	Rectangle menuBtnBounds = { 0.0f, 0.0f, 60.0f, 30.0f };

	while (!WindowShouldClose()) {

		mousePoint = GetMousePosition();
		btnAction = false;

		//Deltatime
		float dt = GetFrameTime();
		if (dt > 0.05f) dt = 0.05f;

		if (IsKeyPressed(KEY_SPACE)) {
			paused = !paused;
		}

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			if (CheckCollisionPointRec(mousePoint, menuBtnBounds)) {
				menuOpen = !menuOpen;
			}
		}

		if (menuOpen) {
			Rectangle handle = ballSizeSlider.getHandle();

			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePoint, handle)) {
				draggingSlider = true;
			}

			if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
				draggingSlider = false;
			}

			if(draggingSlider) {
				float t = (mousePoint.x - ballSizeSlider.track.x) / ballSizeSlider.track.width;
				t = fmaxf(0.0f, fminf(1.0f, t)); // clamp 0..1
				ballSizeSlider.currentVal = ballSizeSlider.minVal +
					t * (ballSizeSlider.maxVal - ballSizeSlider.minVal);
				ballSize = ballSizeSlider.currentVal;
			}

		}

		float scroll = GetMouseWheelMove();
			if (scroll != 0.0f) {
				Vector2 mouseWorld = GetScreenToWorld2D(mousePoint, camera);
				camera.offset = mousePoint;
				camera.target = mouseWorld;
				camera.zoom += scroll * 0.1f;

				if (camera.zoom < 0.1f) camera.zoom = 0.1f;
			}

		if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
			Vector2 delta = GetMouseDelta();
			// Divide by zoom so dragging feels consistent at any zoom level
			camera.target.x -= delta.x / camera.zoom;
			camera.target.y -= delta.y / camera.zoom;
		}

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			
				Vector2 m = GetScreenToWorld2D(mousePoint, camera);

				if (CheckCollisionPointRec(m, worldBounds)) {

				bodies.push_back(Body2D{
					.position = m,
					.velocity = { getRandomVelo(), getRandomVelo()},
					.radius = ballSize,
					.color = BLUE,
					.isStatic = false,
				});
			}

		}

		if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
			
			Vector2 m = GetScreenToWorld2D(mousePoint, camera);
			if (CheckCollisionPointRec(m, worldBounds)) {
					bodies.push_back(Body2D{
						.position = m,
						.velocity = { 0, 0},
						.radius = ballSize,
						.color = RED,
						.isStatic = true,
					});
			}
		}

		if (CheckCollisionPointRec(mousePoint, btnBounds))
		{
			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) btnState = 2;
			else btnState = 1;

			if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) btnAction = true;
		}
		else btnState = 0;


		if (btnAction) {
			paused = !paused;
		}


		if (!paused) {
			for (auto& b : bodies) {
				updateObjects(b, dt, world, WORLD_WIDTH, WORLD_HEIGHT);
			}



			//update depending on collision detection
			for (size_t i = 0; i < bodies.size(); i++) {
				for (size_t j = i + 1; j < bodies.size(); j++) {
					Body2D& a = bodies[i];
					Body2D& b = bodies[j];

					float dx = b.position.x - a.position.x;
					float dy = b.position.y - a.position.y;

					float distSq = dx * dx + dy * dy;
					float r = a.radius + b.radius;

					if (distSq <= r * r) {
						float dist = std::sqrt(distSq);

						float nx, ny;
						if (dist < 0.0001f) {
							nx = 1.0f;
							ny = 0.0f;
						}
						else {
							nx = dx / dist;
							ny = dy / dist;
						}

						float overlap = r - dist;

						// Only push the non-static body the full overlap
						if (a.isStatic && !b.isStatic) {
							b.position.x += nx * overlap;
							b.position.y += ny * overlap;
						}
						else if (!a.isStatic && b.isStatic) {
							a.position.x -= nx * overlap;
							a.position.y -= ny * overlap;
						}
						else if (!a.isStatic && !b.isStatic) {
							a.position.x -= nx * overlap * 0.5f;
							a.position.y -= ny * overlap * 0.5f;
							b.position.x += nx * overlap * 0.5f;
							b.position.y += ny * overlap * 0.5f;
						}

						float rvx = b.velocity.x - a.velocity.x;
						float rvy = b.velocity.y - a.velocity.y;

						float velAlongNormal = rvx * nx + rvy * ny;

						if (velAlongNormal > 0) continue;

						float j = -(1.0f + world.restitution) * velAlongNormal;

						// Static bodies absorb the full impulse, don't receive any
						if (a.isStatic && !b.isStatic) {
							b.velocity.x += j * nx;
							b.velocity.y += j * ny;
						}
						else if (!a.isStatic && b.isStatic) {
							a.velocity.x -= j * nx;
							a.velocity.y -= j * ny;
						}
						else if (!a.isStatic && !b.isStatic) {
							j /= 2.0f;
							a.velocity.x -= j * nx;
							a.velocity.y -= j * ny;
							b.velocity.x += j * nx;
							b.velocity.y += j * ny;
						}
					}

				}
			}
		}

		// Drawings

		BeginDrawing();

		// Set background to LIGHT GRAY
		ClearBackground(DARKGRAY);

		BeginMode2D(camera);
		DrawRectangleLines(0, 0, WORLD_WIDTH, WORLD_HEIGHT, BLACK);
		//Draw objects
		for (const auto& b : bodies) render(b);
		EndMode2D();

		BeginBlendMode(BLEND_ALPHA);
		DrawTexturePro(button, sourceRec, destRec, origin, 0.0f, WHITE);
		DrawRectangleRec(menuBtnBounds, GRAY);
		DrawText("Menu", 10, 5, 20, BLACK);

		if (menuOpen) {

			DrawRectangle(0, 30, MENU_WIDTH, 300, DARKGRAY);
			DrawText("Ball Size", 10, 40, 20, WHITE);

			// Track
			DrawRectangleRec(ballSizeSlider.track, GRAY);

			// Fill
			Rectangle fill = ballSizeSlider.track;
			fill.width = ballSizeSlider.getHandleX() - fill.x;
			DrawRectangleRec(fill, BLUE);

			// Handle
			DrawRectangleRec(ballSizeSlider.getHandle(), WHITE);

			// Value label
			std::string label = std::to_string((int)ballSize);
			DrawText(label.c_str(), (int)MENU_WIDTH - 30, 45, 16, WHITE);

		}

		EndBlendMode();

		// Draw FPS in the top left corner
		DrawFPS(10, 975);

		EndDrawing();
	}

	UnloadTexture(button);

	CloseWindow();

	std::cout << "Window is closing..." << std::endl;

	return 0;
}

void updateObjects(Body2D& body, float dt, const World& world, float worldWidth, float worldHeight) {
	if (body.isStatic) return;

	body.velocity.x += world.gravity.x * dt;
	body.velocity.y += world.gravity.y * dt;

	body.position.x += body.velocity.x * dt;
	body.position.y += body.velocity.y * dt;

	const float minX = body.radius;
	const float maxX = worldWidth - body.radius;
	const float minY = body.radius;
	const float maxY = worldHeight - body.radius;

	if (body.position.x < minX) {
		body.position.x = minX;
		body.velocity.x = -body.velocity.x * world.restitution;
	}
	else if (body.position.x > maxX) {
		body.position.x = maxX;
		body.velocity.x = -body.velocity.x * world.restitution;
	}

	if (body.position.y < minY) {
		body.position.y = minY;
		body.velocity.y = -body.velocity.y * world.restitution;
	}
	else if (body.position.y > maxY) {
		body.position.y = maxY;
		body.velocity.y = -body.velocity.y * world.restitution;

		float damping = std::exp(-world.groundDampingPerSecond * dt);
		body.velocity.x *= damping;

		if (fabs(body.velocity.y) < 20.0f) body.velocity.y = 0.0f;
	}
}

void render(const Body2D& body) {
	DrawCircleV(body.position, body.radius, body.color);
}

float getRandomVelo() {
	return GetRandomValue(-200, 200);
}
#include <iostream>
#include <raylib.h>
#include <deque>
#include <raymath.h>
#include <vector>
using namespace std;

// Trạng thái của game
enum GameState
{
    MENU,
    PLAYING,
    GAME_OVER
};

// Các biến toàn cục
Color green = {173, 204, 96, 255};
Color darkGreen = {43, 51, 24, 255};
int cellSize = 30;
int cellCount = 25;
int offset = 75;
static bool allowMove = false;
double lastUpdateTime = 0;

// Hàm kiểm tra phần tử trong deque
bool ElementInDeque(Vector2 element, deque<Vector2> deque)
{
    for (unsigned int i = 0; i < deque.size(); i++)
    {
        if (Vector2Equals(deque[i], element))
        {
            return true;
        }
    }
    return false;
}

// Hàm kiểm tra sự kiện dựa trên khoảng thời gian
bool EventTriggered(double interval)
{
    double currentTime = GetTime();
    if (currentTime - lastUpdateTime >= interval)
    {
        lastUpdateTime = currentTime;
        return true;
    }
    return false;
}

// Hàm vẽ menu
void DrawMenu(GameState state, int score)
{
    ClearBackground(green);
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    const int fontSize = 40; // Kích thước font
    const int spacing = 20;  // Khoảng cách giữa các dòng

    // Tọa độ y của dòng đầu tiên
    int baseY = (screenHeight / 2) - (fontSize * 2);

    // Tiêu đề
    const char *title = "Retro Snake";
    int titleX = (screenWidth / 2) - (MeasureText(title, 50) / 2);
    DrawText(title, titleX, baseY - 80, 50, darkGreen);

    if (state == MENU)
    {
        // Menu lựa chọn
        const char *options[] = {"1. New Game", "2. Continue", "3. Quit"};
        for (int i = 0; i < 3; i++)
        {
            int textWidth = MeasureText(options[i], fontSize);
            int x = (screenWidth / 2) - (textWidth / 2);
            int y = baseY + (fontSize + spacing) * i;
            DrawText(options[i], x, y, fontSize, darkGreen);
        }
    }
    else if (state == GAME_OVER)
    {
        // Thông báo game over
        const char *gameOverText = "Game Over!";
        int gameOverX = (screenWidth / 2) - (MeasureText(gameOverText, 50) / 2);
        DrawText(gameOverText, gameOverX, baseY - 40, 50, darkGreen);

        const char *scoreText = TextFormat("Score: %i", score);
        int scoreX = (screenWidth / 2) - (MeasureText(scoreText, fontSize) / 2);
        DrawText(scoreText, scoreX, baseY, fontSize, darkGreen);

        // Menu lựa chọn sau khi game over
        const char *options[] = {"1. New Game", "2. Quit"};
        for (int i = 0; i < 2; i++)
        {
            int textWidth = MeasureText(options[i], fontSize);
            int x = (screenWidth / 2) - (textWidth / 2);
            int y = baseY + (fontSize + spacing) * (i + 1);
            DrawText(options[i], x, y, fontSize, darkGreen);
        }
    }
}


// Lớp Snake
class Snake
{
public:
    deque<Vector2> body = {Vector2{6, 9}, Vector2{5, 9}, Vector2{4, 9}};
    Vector2 direction = {1, 0};
    bool addSegment = false;

    void Draw()
    {
        for (unsigned int i = 0; i < body.size(); i++)
        {
            float x = body[i].x;
            float y = body[i].y;
            Rectangle segment = Rectangle{offset + x * cellSize, offset + y * cellSize, (float)cellSize, (float)cellSize};
            DrawRectangleRounded(segment, 0.5, 6, darkGreen);
        }
    }

    void Update()
    {
        body.push_front(Vector2Add(body[0], direction));
        if (addSegment == true)
        {
            addSegment = false;
        }
        else
        {
            body.pop_back();
        }
    }

    void Reset()
    {
        body = {Vector2{6, 9}, Vector2{5, 9}, Vector2{4, 9}};
        direction = {1, 0};
    }
};

// Lớp Food
class Food
{
public:
    Vector2 position;
    Texture2D texture;

    Food(deque<Vector2> snakeBody)
    {
        Image image = LoadImage("Graphics/food.png");
        texture = LoadTextureFromImage(image);
        UnloadImage(image);
        position = GenerateRandomPos(snakeBody);
    }

    ~Food()
    {
        UnloadTexture(texture);
    }

    void Draw()
    {
        DrawTexture(texture, offset + position.x * cellSize, offset + position.y * cellSize, WHITE);
    }

    Vector2 GenerateRandomCell()
    {
        float x = GetRandomValue(0, cellCount - 1);
        float y = GetRandomValue(0, cellCount - 1);
        return Vector2{x, y};
    }

    Vector2 GenerateRandomPos(deque<Vector2> snakeBody)
{
    vector<Vector2> emptyCells;

    // Duyệt qua toàn bộ lưới và tìm các ô trống
    for (int x = 0; x < cellCount; x++)
    {
        for (int y = 0; y < cellCount; y++)
        {
            Vector2 cell = {static_cast<float>(x), static_cast<float>(y)};
            if (!ElementInDeque(cell, snakeBody)) // Kiểm tra nếu ô này không thuộc cơ thể rắn
            {
                emptyCells.push_back(cell);
            }
        }
    }

    // Chọn ngẫu nhiên một ô từ danh sách các ô trống
    if (!emptyCells.empty())
    {
        return emptyCells[GetRandomValue(0, emptyCells.size() - 1)];
    }
    else
    {
        // Trường hợp khẩn cấp: không còn ô trống (hiếm khi xảy ra)
        return {0, 0}; // Hoặc một giá trị mặc định nào đó
    }
}

};

// Lớp Game
class Game
{
public:
    Snake snake = Snake();
    Food food = Food(snake.body);
    bool running = true;
    int score = 0;
    Sound eatSound;
    Sound wallSound;

    Game()
    {
        InitAudioDevice();
        eatSound = LoadSound("Sounds/eat.mp3");
        wallSound = LoadSound("Sounds/wall.mp3");
    }

    ~Game()
    {
        UnloadSound(eatSound);
        UnloadSound(wallSound);
        CloseAudioDevice();
    }

    void Draw()
    {
        food.Draw();
        snake.Draw();
    }

    void Update()
    {
        if (running)
        {
            snake.Update();
            CheckCollisionWithFood();
            CheckCollisionWithEdges();
            CheckCollisionWithTail();
        }
    }

    void CheckCollisionWithFood()
    {
        if (Vector2Equals(snake.body[0], food.position))
        {
            food.position = food.GenerateRandomPos(snake.body);
            snake.addSegment = true;
            score++;
            PlaySound(eatSound);
        }
    }

    void CheckCollisionWithEdges()
    {
        if (snake.body[0].x == cellCount || snake.body[0].x == -1 ||
            snake.body[0].y == cellCount || snake.body[0].y == -1)
        {
            GameOver();
        }
    }

    void GameOver()
    {
        running = false;
        PlaySound(wallSound);
    }

    void CheckCollisionWithTail()
    {
        deque<Vector2> headlessBody = snake.body;
        headlessBody.pop_front();
        if (ElementInDeque(snake.body[0], headlessBody))
        {
            GameOver();
        }
    }
};

// Hàm main
int main()
{
    cout << "Starting the game..." << endl;
    InitWindow(2 * offset + cellSize * cellCount, 2 * offset + cellSize * cellCount, "Retro Snake");
    SetTargetFPS(60);

    Game game = Game();
    GameState state = MENU;

    while (!WindowShouldClose())
    {
        BeginDrawing();

        if (state == MENU || state == GAME_OVER)
        {
            if (IsKeyPressed(KEY_ONE)) // New Game
            {
                game.snake.Reset();
                game.food.position = game.food.GenerateRandomPos(game.snake.body);
                game.running = true;
                game.score = 0;
                state = PLAYING;
            }
            else if (state == MENU && IsKeyPressed(KEY_TWO)) // Continue
            {
                state = PLAYING;
            }
            else if (IsKeyPressed(KEY_THREE) || (state == GAME_OVER && IsKeyPressed(KEY_TWO))) // Quit
            {
                break;
            }

            DrawMenu(state, game.score);
        }
        else if (state == PLAYING)
        {
            if (EventTriggered(0.2))
            {
                allowMove = true;
                game.Update();
            }

            if (IsKeyPressed(KEY_UP) && game.snake.direction.y != 1 && allowMove)
            {
                game.snake.direction = {0, -1};
                allowMove = false;
            }
            else if (IsKeyPressed(KEY_DOWN) && game.snake.direction.y != -1 && allowMove)
            {
                game.snake.direction = {0, 1};
                allowMove = false;
            }
            else if (IsKeyPressed(KEY_LEFT) && game.snake.direction.x != 1 && allowMove)
            {
                game.snake.direction = {-1, 0};
                allowMove = false;
            }
            else if (IsKeyPressed(KEY_RIGHT) && game.snake.direction.x != -1 && allowMove)
            {
                game.snake.direction = {1, 0};
                allowMove = false;
            }

            if (!game.running)
            {
                state = GAME_OVER;
            }

            ClearBackground(green);
            DrawRectangleLinesEx(Rectangle{(float)offset - 5, (float)offset - 5, (float)cellSize * cellCount + 10, (float)cellSize * cellCount + 10}, 5, darkGreen);
            DrawText("Retro Snake", offset - 5, 20, 40, darkGreen);
            DrawText(TextFormat("%i", game.score), offset - 5, offset + cellSize * cellCount + 10, 40, darkGreen);
            game.Draw();
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

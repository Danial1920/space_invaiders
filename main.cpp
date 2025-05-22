#include <ncurses.h>
#include <vector>
#include <memory>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

using namespace std;

// Базовый класс для всех игровых объектов
class Entity {
protected:
    int x, y;
    char symbol;
public:
    Entity(int x, int y, char symbol) : x(x), y(y), symbol(symbol) {}
    virtual ~Entity() {}
    
    int getX() const { return x; }
    int getY() const { return y; }
    char getSymbol() const { return symbol; }
    
    virtual void update() {}
    virtual void move(int dx, int dy) {
        x += dx;
        y += dy;
    }
    
    virtual bool isAlive() const { return true; }
    virtual void takeDamage() {}
};

// Класс пули
class Bullet : public Entity {
private:
    int direction; // 1 - вниз, -1 - вверх
    bool active;
public:
    Bullet(int x, int y, int direction) 
        : Entity(x, y, '|'), direction(direction), active(true) {}
    
    void update() {
        move(0, direction);
        if (y < 0 || y >= LINES) active = false;
    }
    
    bool isActive() const { return active; }
    
    int getDirection() const { return direction; }
};

// Класс игрока
class Player : public Entity {
private:
    int lives;
    int cooldown;
    int score;
public:
    Player() : Entity(COLS/2, LINES-3, 'A'), lives(3), cooldown(0), score(0) {}
    
    void moveLeft() { if (x > 0) move(-1, 0); }
    void moveRight() { if (x < COLS-1) move(1, 0); }
    
    Bullet* shoot() {
        if (cooldown <= 0) {
            cooldown = 10;
            return new Bullet(x, y-1, -1);  // Добавлено направление вверх (-1)
        }
        return NULL;
    }
    
    void update() {
        if (cooldown > 0) cooldown--;
    }
    
    int getLives() const { return lives; }
    int getScore() const { return score; }
    
    void addScore(int points) { score += points; }
    
    void takeDamage() {
        lives--;
    }
    
    bool isAlive() const {
        return lives > 0;
    }
};

// Класс врага
class Enemy : public Entity {
private:
    int direction;
    int lives;
    int moveCounter;
    int cooldown;
public:
    Enemy(int x, int y) : Entity(x, y, 'V'), direction(1), lives(1), moveCounter(0), cooldown(0) {}
    
    void update() {
        moveCounter++;
        if (moveCounter % 15 == 0) {
            if (x + direction < 1 || x + direction >= COLS-1) {
                move(0, 1);
                direction *= -1;
            } else {
                move(direction, 0);
            }
        }
        
        // Стрельба
        if (cooldown <= 0 && rand() % 100 == 0) {
            cooldown = 30;
        }
        
        if (cooldown > 0) cooldown--;
    }
    
    Bullet* shoot() {
        if (cooldown == 0) {
            cooldown = 30;
            return new Bullet(x, y+1, 1);  // Добавлено направление вниз (1)
        }
        return NULL;
    }
    
    void takeDamage() {
        lives--;
    }
    
    bool isAlive() const {
        return lives > 0;
    }
};

// Игровой класс
class Game {
private:
    Player* player;
    vector<Enemy*> enemies;
    vector<Bullet*> bullets;
    int wave;
    bool gameOver;
    
public:
    Game() : wave(1), gameOver(false) {
        initscr();
        noecho();
        curs_set(FALSE);
        keypad(stdscr, TRUE);
        nodelay(stdscr, TRUE);
        
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_RED, COLOR_BLACK);
        
        createEnemies();
        player = new Player();
    }
    
    ~Game() {
        endwin();
        delete player;
        for (vector<Enemy*>::iterator it = enemies.begin(); it != enemies.end(); ++it)
            delete *it;
        for (vector<Bullet*>::iterator it = bullets.begin(); it != bullets.end(); ++it)
            delete *it;
    }
    
    void createEnemies() {
        enemies.clear();
        int rows = 3 + wave;
        int cols = 8;
        for (int y = 2; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                enemies.push_back(new Enemy((COLS/cols)*x, y));
            }
        }
    }
    
    void checkCollisions() {
        // Проверка столкновений пуль
        for (vector<Bullet*>::iterator bulletIt = bullets.begin(); bulletIt != bullets.end();) {
            Bullet* bullet = *bulletIt;
            
            // Столкновение с врагами
            if (bullet->getDirection() == -1) {
                for (vector<Enemy*>::iterator enemyIt = enemies.begin(); enemyIt != enemies.end();) {
                    Enemy* enemy = *enemyIt;
                    if (abs(enemy->getX() - bullet->getX()) <= 1 && 
                        abs(enemy->getY() - bullet->getY()) <= 1) {
                        enemy->takeDamage();
                        delete bullet;
                        bulletIt = bullets.erase(bulletIt);
                        player->addScore(10);
                        if (!enemy->isAlive()) {
                            delete enemy;
                            enemyIt = enemies.erase(enemyIt);
                        } else {
                            ++enemyIt;
                        }
                        break;
                    } else {
                        ++enemyIt;
                    }
                }
                if (bulletIt != bullets.end()) ++bulletIt;
            }
            // Столкновение с игроком
            else {
                if (abs(player->getX() - bullet->getX()) <= 1 && 
                    abs(player->getY() - bullet->getY()) <= 1) {
                    player->takeDamage();
                    delete bullet;
                    bulletIt = bullets.erase(bulletIt);
                } else {
                    ++bulletIt;
                }
            }
        }
        
        // Проверка столкновения врагов с игроком
        for (vector<Enemy*>::iterator it = enemies.begin(); it != enemies.end(); ++it) {
            Enemy* enemy = *it;
            if (abs(enemy->getX() - player->getX()) <= 1 && 
                abs(enemy->getY() - player->getY()) <= 1) {
                player->takeDamage();
                enemy->takeDamage();
                break;
            }
        }
    }
    
    void update() {
        player->update();
        
        for (vector<Enemy*>::iterator it = enemies.begin(); it != enemies.end();) {
            Enemy* enemy = *it;
            enemy->update();
            
            // Стрельба
            Bullet* bullet = enemy->shoot();
            if (bullet) bullets.push_back(bullet);
            
            // Проверка выхода за границы
            if (enemy->getY() >= LINES-1) {
                player->takeDamage();
                delete enemy;
                it = enemies.erase(it);
            } else {
                ++it;
            }
        }
        
        for (vector<Bullet*>::iterator it = bullets.begin(); it != bullets.end();) {
            Bullet* bullet = *it;
            bullet->update();
            if (!bullet->isActive()) {
                delete bullet;
                it = bullets.erase(it);
            } else {
                ++it;
            }
        }
        
        checkCollisions();
        
        // Проверка завершения волны
        if (enemies.empty()) {
            wave++;
            createEnemies();
        }
        
        // Проверка конца игры
        if (!player->isAlive()) {
            gameOver = true;
        }
    }
    
    void render() {
        clear();
        
        // Рисуем игрока
        attron(COLOR_PAIR(1));
        mvprintw(player->getY(), player->getX(), "%c", player->getSymbol());
        attroff(COLOR_PAIR(1));
        
        // Рисуем врагов
        attron(COLOR_PAIR(2));
        for (vector<Enemy*>::iterator it = enemies.begin(); it != enemies.end(); ++it) {
            Enemy* enemy = *it;
            mvprintw(enemy->getY(), enemy->getX(), "%c", enemy->getSymbol());
        }
        attroff(COLOR_PAIR(2));
        
        // Рисуем пули
        for (vector<Bullet*>::iterator it = bullets.begin(); it != bullets.end(); ++it) {
            Bullet* bullet = *it;
            mvprintw(bullet->getY(), bullet->getX(), "%c", bullet->getSymbol());
        }
        
        // Информация
        mvprintw(0, 0, "Lives: %d  Score: %d  Wave: %d", 
                player->getLives(), player->getScore(), wave);
        
        refresh();
    }
    
    void gameOverScreen() {
        clear();
        const char* msg = "GAME OVER! Press any key to exit.";
        mvprintw(LINES/2, (COLS-strlen(msg))/2, "%s", msg);
        refresh();
        getch();
    }
    
    void victoryScreen() {
        clear();
        const char* msg = "VICTORY! Press any key to exit.";
        mvprintw(LINES/2, (COLS-strlen(msg))/2, "%s", msg);
        refresh();
        getch();
    }
    
    void run() {
        srand(time(NULL));
        while (!gameOver) {
            // Обработка ввода
            int ch = getch();
            switch(ch) {
                case KEY_LEFT:
                    player->moveLeft();
                    break;
                case KEY_RIGHT:
                    player->moveRight();
                    break;
                case ' ':
                    {
                        Bullet* bullet = player->shoot();
                        if (bullet) bullets.push_back(bullet);
                    }
                    break;
                case 'q':
                    gameOver = true;
                    break;
            }
            
            update();
            render();
            
            // Ограничение FPS
            usleep(50000);
        }
        
        if (player->isAlive()) {
            victoryScreen();
        } else {
            gameOverScreen();
        }
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}
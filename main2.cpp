#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <utility>
#include <algorithm>

using namespace std;

// Функция для очистки терминала с использованием ANSI-кодов
void clearTerminal() {
    cout << "\033[2J\033[H"; // Очистка экрана и перемещение курсора в начало
}

struct Cell { // данные о клетках
    bool hasBomb;
    bool isOpen;
    bool isFlagged;
    int neighborBombs;

    Cell() : hasBomb(false), isOpen(false), isFlagged(false), neighborBombs(0) {}
};

struct GameState { // все про игру
    int N;
    int K;
    vector<vector<Cell> > field;  // Добавлен пробел между > >
    pair<int, int> cursor;
    bool gameOver;
    bool gameWon;
};

void generateField(GameState& state) { // расстановка бомб, их подсчет
    state.field = vector<vector<Cell> >(state.N, vector<Cell>(state.N));  // Добавлен пробел между > >

    int bombsPlaced = 0;
    while (bombsPlaced < state.K) {
        int x = rand() % state.N;
        int y = rand() % state.N;
        if (!state.field[x][y].hasBomb) {
            state.field[x][y].hasBomb = true;
            bombsPlaced++;
        }
    }

    for (int i = 0; i < state.N; ++i) {
        for (int j = 0; j < state.N; ++j) {
            if (!state.field[i][j].hasBomb) {
                int count = 0;
                for (int dx = -1; dx <= 1; ++dx) {
                    for (int dy = -1; dy <= 1; ++dy) {
                        int nx = i + dx;
                        int ny = j + dy;
                        if (nx >= 0 && nx < state.N && ny >= 0 && ny < state.N && state.field[nx][ny].hasBomb) {
                            count++;
                        }
                    }
                }
                state.field[i][j].neighborBombs = count;
            }
        }
    }
}

void openCell(GameState& state, int x, int y) { // рекурсивное открытие клеток, проверка на победу
    if (x < 0 || x >= state.N || y < 0 || y >= state.N || state.field[x][y].isOpen || state.field[x][y].isFlagged) {
        return;
    }

    state.field[x][y].isOpen = true;

    if (state.field[x][y].hasBomb) {
        state.gameOver = true;
        return;
    }

    if (state.field[x][y].neighborBombs == 0) {
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                openCell(state, x + dx, y + dy);
            }
        }
    }

    bool allOpen = true;
    for (int i = 0; i < state.N; ++i) {
        for (int j = 0; j < state.N; ++j) {
            if (!state.field[i][j].hasBomb && !state.field[i][j].isOpen) {
                allOpen = false;
                break;
            }
        }
        if (!allOpen) break;
    }
    state.gameWon = allOpen;
}

void botMove(GameState& state) { // бот
    for (int i = 0; i < state.N; ++i) {
        for (int j = 0; j < state.N; ++j) {
            if (!state.field[i][j].isOpen && !state.field[i][j].isFlagged && state.field[i][j].hasBomb) {
                state.field[i][j].isFlagged = true;
            }
        }
    }

    vector<pair<int, int> > closedCells;  // Добавлен пробел между > >
    for (int i = 0; i < state.N; ++i) {
        for (int j = 0; j < state.N; ++j) {
            if (!state.field[i][j].isOpen && !state.field[i][j].isFlagged) {
                closedCells.emplace_back(i, j);
            }
        }
    }

    if (!closedCells.empty()) {
        int index = rand() % closedCells.size();
        openCell(state, closedCells[index].first, closedCells[index].second);
    }
}

void drawField(const GameState& state) { // визуалка
    for (int i = 0; i < state.N; ++i) {
        for (int j = 0; j < state.N; ++j) {
            if (i == state.cursor.first && j == state.cursor.second) {
                cout << "[";
            } else {
                cout << " ";
            }

            if (state.field[i][j].isFlagged) {
                cout << "F";
            } else if (!state.field[i][j].isOpen) {
                cout << "#";
            } else if (state.field[i][j].hasBomb) {
                cout << "*";
            } else {
                cout << state.field[i][j].neighborBombs;
            }

            if (i == state.cursor.first && j == state.cursor.second) {
                cout << "]";
            } else {
                cout << " ";
            }
        }
        cout << endl;
    }
}

void handleInput(GameState& state, char input) { // обработка команд
    if (state.gameOver || state.gameWon) return;

    int x = state.cursor.first;
    int y = state.cursor.second;

    switch (input) {
        case 'w': x = max(0, x - 1); break;
        case 's': x = min(state.N - 1, x + 1); break;
        case 'a': y = max(0, y - 1); break;
        case 'd': y = min(state.N - 1, y + 1); break;
        case 'f': 
            if (!state.field[x][y].isOpen) {
                state.field[x][y].isFlagged = !state.field[x][y].isFlagged;
            }
            break;
        case 'l': openCell(state, x, y); break;
        case 'b': botMove(state); break;
    }

    state.cursor = std::make_pair(x, y); 
}

int main() {
    srand(time(0));

    int N, K;
    cout << "Введите размер поля (N x N, где N от 5 до 20): ";
    cin >> N;
    cout << "Введите количество бомб (K < N * N): ";
    cin >> K;

    if (N < 5 || N > 20 || K >= N * N) {
        cout << "Некорректный ввод!" << endl;
        return 1;
    }

    GameState state;
    state.N = N;
    state.K = K;
    state.cursor = std::make_pair(0, 0); 
    state.gameOver = false;
    state.gameWon = false;
    generateField(state);

    char input;
    while (true) {
        clearTerminal(); // Очищаем терминал перед каждым рисованием поля
        drawField(state);
        cout << "Введите команду (WASD - движение, F - флаг, L - открыть, B - ход бота): ";
        cin >> input;

        handleInput(state, input);

        if (state.gameOver) {
            cout << "Игра окончена, ты проиграл((" << endl;
            break;
        }

        if (state.gameWon) {
            cout << "Поздравляю, ты выиграл!" << endl;
            break;
        }
    }

    return 0;
}
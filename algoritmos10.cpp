#include <stdio.h>
#include <Windows.h>
#include <conio.h>
#include <stdlib.h>
#include <list>
#include <mmsystem.h>
#include <time.h>
#include <vector>
#include <string>
using namespace std;

#pragma comment(lib, "winmm.lib")

#define ARRIBA 72
#define IZQUIERDA 75
#define DERECHA 77
#define ABAJO 80

const int PLAY_X_MIN = 3;
const int PLAY_X_MAX = 76;
const int PLAY_Y_MIN = 4;
const int PLAY_Y_MAX = 32;

void gotoxy(int x, int y) {
    HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD dwPos = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(hCon, dwPos);
}
void ocultarCursor() {
    HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cci;
    cci.dwSize = 1;
    cci.bVisible = FALSE;
    SetConsoleCursorInfo(hCon, &cci);
}
void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void pintarLimite() {
    for (int i = 2; i < 78; i++) {
        gotoxy(i, 3);  printf("%c", 205);
        gotoxy(i, 33); printf("%c", 205);
    }
    for (int i = 4; i < 33; i++) {
        gotoxy(2, i);  printf("%c", 186);
        gotoxy(77, i); printf("%c", 186);
    }
    gotoxy(2, 3);  printf("%c", 201);
    gotoxy(2, 33); printf("%c", 200);
    gotoxy(77, 3);  printf("%c", 187);
    gotoxy(77, 33); printf("%c", 188);
}

void sndDisparo() { sndPlaySound(TEXT("snd_disparo.wav"), SND_ASYNC | SND_NODEFAULT); }
void sndLose() { sndPlaySound(TEXT("snd_lose.wav"), SND_ASYNC | SND_NODEFAULT); }
void sndExplosion() { sndPlaySound(TEXT("explosion.wav"), SND_ASYNC | SND_NODEFAULT); }

// -------------------------------------------------
// Clase NAVE (no leer teclado dentro de la clase)
// -------------------------------------------------
class NAVE {
    int x, y;
    int corazones;
    int vidas;
    int width, height;
    vector<string> sprite;
public:
    NAVE(int _x = 37, int _y = 30, int _cor = 3, int _vidas = 3) {
        x = _x; y = _y; corazones = _cor; vidas = _vidas;

        sprite = vector<string>{
            "  ^  ",
            " /O\\ ",
            "/|\\"
        };
        width = (int)sprite[0].length();
        height = (int)sprite.size();
    }
    void pintar() {
        setColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        for (int r = 0; r < height; r++) {
            gotoxy(x, y + r);
            printf("%s", sprite[r].c_str());
        }
        setColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
    void borrar() {
        for (int r = 0; r < height; r++) {
            gotoxy(x, y + r);
            for (int c = 0; c < width; c++) putchar(' ');
        }
    }
    void moverPor(int dx, int dy) {
        borrar();
        x += dx; y += dy;

        int maxX = PLAY_X_MAX - (width - 1);
        int maxY = PLAY_Y_MAX - (height - 1);
        if (x < PLAY_X_MIN) x = PLAY_X_MIN;
        if (x > maxX) x = maxX;
        if (y < PLAY_Y_MIN) y = PLAY_Y_MIN;
        if (y > maxY) y = maxY;
        pintar();
    }
    void pintarHUD() {
        setColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        gotoxy(50, 2); printf("Vidas: %d  ", vidas);
        setColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        gotoxy(64, 2); printf("Salud: ");

        for (int i = 0; i < 3; i++) {
            gotoxy(71 + i, 2);
            if (i < corazones) printf("%c", 3); else printf(" ");
        }
        setColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
    void perderCorazon() {
        if (corazones > 0) corazones--;
    }

    void procesarVida() {
        if (corazones == 0) {

            borrar();
            setColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
            gotoxy(x, y);     printf("  ** ");
            gotoxy(x, y + 1); printf(" ****");
            gotoxy(x, y + 2); printf("  ** ");
            setColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            sndExplosion();
            Sleep(220);
            borrar();
            vidas--;
            corazones = 3;
            pintarHUD();
            if (vidas > 0) pintar();
            else {

                borrar();
                setColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
                gotoxy(x, y);     printf("   **  ");
                gotoxy(x, y + 1); printf("  **** ");
                gotoxy(x, y + 2); printf("   **  ");
                setColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            }
        }
    }


    int X() const { return x; }
    int Y() const { return y; }
    int Width() const { return width; }
    int Height() const { return height; }
    int Vidas() const { return vidas; }
    int Corazones() const { return corazones; }
    int puntoDisparoX() const { return x + width / 2; }
    int puntoDisparoY() const { return y - 1; }
};

// -------------------------------------------------
// Bala
// -------------------------------------------------
class Bala {
    int x, y;
public:
    Bala(int _x, int _y) : x(_x), y(_y) {}
    void dibujar() { gotoxy(x, y); printf("*"); }
    void borrar() { if (y >= PLAY_Y_MIN && y <= PLAY_Y_MAX) { gotoxy(x, y); printf(" "); } }
    void mover() {
        borrar();
        y--;
        if (y >= PLAY_Y_MIN) dibujar();
    }
    bool fuera() const { return (y < PLAY_Y_MIN); }
    int X() const { return x; }
    int Y() const { return y; }
};

// -------------------------------------------------
// Asteroide con sprite multi-line y "delay" (velocidad variable)
// -------------------------------------------------
class Asteroide {
    int x, y;
    int delay;
    int ticks;
    vector<string> sprite;
    int width, height;
    int puntos;
public:
    Asteroide() { reiniciar(); }

    void elegirFormaAleatoria() {
        int t = rand() % 3;
        if (t == 0) {
            sprite = { "@" };
            puntos = 5;
        }
        else if (t == 1) {
            sprite = { "(@)", "/#\\" };
            puntos = 12;
        }
        else {
            sprite = { " O ", "/###\\", "\\###/" };
            puntos = 25;
        }
        width = (int)sprite[0].length();
        height = (int)sprite.size();
    }

    void reiniciar() {
        elegirFormaAleatoria();
        delay = (rand() % 3) + 1;
        ticks = 0;
        x = (rand() % (PLAY_X_MAX - width - PLAY_X_MIN + 1)) + PLAY_X_MIN;
        y = (rand() % 6) + PLAY_Y_MIN;
    }

    void dibujar() {
        setColor(FOREGROUND_RED | FOREGROUND_GREEN);
        for (int r = 0; r < height; r++) {
            for (int c = 0; c < width; c++) {
                char ch = sprite[r][c];
                if (ch != ' ') {
                    gotoxy(x + c, y + r);
                    printf("%c", ch);
                }
            }
        }
        setColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
    void borrar() {
        for (int r = 0; r < height; r++) {
            for (int c = 0; c < width; c++) {
                gotoxy(x + c, y + r);
                printf(" ");
            }
        }
    }

    void actualizar() {
        ticks++;
        if (ticks >= delay) {
            borrar();
            y++;
            ticks = 0;
            if (y > PLAY_Y_MAX) {
                reiniciar();
            }
            dibujar();
        }
    }

    bool colisionConNave(const NAVE& nave) {
        int ax1 = x, ax2 = x + width - 1;
        int ay1 = y, ay2 = y + height - 1;
        int bx1 = nave.X(), bx2 = nave.X() + nave.Width() - 1;
        int by1 = nave.Y(), by2 = nave.Y() + nave.Height() - 1;
        bool overlap = !(ax2 < bx1 || ax1 > bx2 || ay2 < by1 || ay1 > by2);
        return overlap;
    }

    bool impactadaPorBala(int bx, int by) {
        if (bx >= x && bx <= x + width - 1 && by >= y && by <= y + height - 1) {
            int cx = bx - x;
            int ry = by - y;
            char ch = sprite[ry][cx];
            return (ch != ' ');
        }
        return false;
    }
    void resetearArriba() { borrar(); reiniciar(); dibujar(); }
    int X() const { return x; }
    int Y() const { return y; }
    int Puntos() const { return puntos; }
};

// -------------------------------------------------
// MAIN
// -------------------------------------------------
int main() {
    srand((unsigned)time(NULL));
    ocultarCursor();
    pintarLimite();

    NAVE nave(37, 28, 3, 3);
    nave.pintar();
    nave.pintarHUD();


    list<Asteroide*> asts;
    list<Bala*> balas;
    int puntos = 0;


    for (int i = 0; i < 6; ++i) {
        Asteroide* a = new Asteroide();
        a->dibujar();
        asts.push_back(a);
    }

    bool running = true;
    while (running) {

        setColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        gotoxy(4, 2); printf("Puntos: %5d  ", puntos);
        setColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);


        if (_kbhit()) {
            int tecla = _getch();
            if (tecla == 0 || tecla == 224) {
                int ext = _getch();
                if (ext == IZQUIERDA) nave.moverPor(-1, 0);
                else if (ext == DERECHA) nave.moverPor(1, 0);
                else if (ext == ARRIBA) nave.moverPor(0, -1);
                else if (ext == ABAJO) nave.moverPor(0, 1);
            }
            else {

                if (tecla == 'a' || tecla == 'A' || tecla == 32) { // 'a' o Space => disparar
                    Bala* b = new Bala(nave.puntoDisparoX(), nave.puntoDisparoY());
                    b->dibujar();
                    balas.push_back(b);

                    sndDisparo();
                }
                else if (tecla == 'q' || tecla == 'Q') {
                    running = false;
                }
            }
        }


        for (auto it = balas.begin(); it != balas.end();) {
            Bala* b = *it;
            b->mover();
            if (b->fuera()) {
                b->borrar();
                delete b;
                it = balas.erase(it);
            }
            else ++it;
        }


        for (auto a : asts) {
            a->actualizar();

            if (a->colisionConNave(nave)) {
                a->borrar();
                nave.perderCorazon();
                nave.pintarHUD();
                a->resetearArriba();
            }
        }


        for (auto itA = asts.begin(); itA != asts.end(); ++itA) {
            Asteroide* ast = *itA;
            bool eliminado = false;
            for (auto itB = balas.begin(); itB != balas.end();) {
                Bala* b = *itB;
                if (ast->impactadaPorBala(b->X(), b->Y())) {

                    b->borrar();
                    delete b;
                    itB = balas.erase(itB);

                    ast->borrar();
                    puntos += ast->Puntos();
                    sndExplosion();
                    ast->resetearArriba();

                    eliminado = true;
                    break;
                }
                else ++itB;
            }
            if (eliminado) {

            }
        }


        nave.procesarVida();
        nave.pintarHUD();


        if (nave.Vidas() <= 0) {
            sndLose();
            gotoxy(30, 18);
            setColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
            printf("### GAME OVER ###  Puntos: %d", puntos);
            setColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            running = false;
        }

        Sleep(60);
    }


    for (auto p : asts) delete p;
    for (auto p : balas) delete p;

    gotoxy(0, 35);
    return 0;
}
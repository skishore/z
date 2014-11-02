
#define STATUSX 96
#define STATUSY 4

// constants for the sleep/eat/stamina bars 
#define BARX 142
#define BARY 3
#define BARWIDTH 5
#define BAROFFSET 7
#define MAXMAX max(max(MAXSTATS[SLP], MAXSTATS[EAT]), MAXSTATS[STA])

// color constants for the bars and the cursor
#if !defined(__APPLE__)
#define WHITE 0x00FFFFFF
const Uint32 BARCOLOR[3] = {0x00FF8080, 0x0080FF40, 0x00FFFFFF};
const Uint32 ENDCOLOR[3] = {0x00C06060, 0x00408020, 0x00808080};
#else
#define WHITE 0xFFFFFF00
const Uint32 BARCOLOR[3] = {0x8080FF00, 0x40FF8000, 0xFFFFFF00};
const Uint32 ENDCOLOR[3] = {0x6060C000, 0x20804000, 0x80808000};
#endif
#define BLACK 0x00000000

Sprite* background;
Sprite* status;

void drawBars(const int stat=-1) {
    if (stat == -2)
        return;

    int y = BARY;

    for (int i = 0; i < STA; i++, y += BAROFFSET) {
        if ((stat != -1) && (i != stat))
            continue;

        SDL_FillRectangle(buffer, BARX, y, BARX + MAXSTATS[i] + 1, y, BARWIDTH, BLACK);
        SDL_FillRectangle(buffer, BARX, y, BARX + round(player->stats[i]), y, BARWIDTH, BARCOLOR[i]);
        SDL_DrawVLine(buffer, BARX + MAXSTATS[i], y, y + BARWIDTH - 1, ENDCOLOR[i]);
    }

    SDL_FillRectangle(buffer, BARX, y, BARX + MAXMAX + 1, y, BARWIDTH, BLACK);
    SDL_FillRectangle(buffer, BARX, y, BARX + round(player->stats[STA]), y, BARWIDTH, BARCOLOR[STA]);
    SDL_DrawVLine(buffer, BARX + MAXSTATS[STA], y, y + BARWIDTH - 1, ENDCOLOR[STA]);
    for (int i = 0; i < 2; i++)
        SDL_DrawVLine(buffer, BARX + round(player->stats[i]), y, y + BARWIDTH - 1, ENDCOLOR[i]);
}

void drawStatus() {
    status->frameCol = (player->running ? 1 : 0);
    status->draw(buffer);
}

void drawMouse() {
  /*
    screen->clip_rect.y = GUIHEIGHT;
    screen->clip_rect.h -= GUIHEIGHT;

    if (player->equipped == GUN) {
        float dist = fpoint(player->x + SPRITESIZE/2 - mouse.x, player->y + SPRITESIZE/2 - mouse.y).length();
        SDL_DrawCircle(screen, mouse.x, mouse.y + GUIHEIGHT, (int)(dist*player->aim + 1), WHITE);
    } else {
        SDL_DrawCircle(screen, mouse.x, mouse.y + GUIHEIGHT, 1, WHITE);
    }

    screen->clip_rect.y = 0;
    screen->clip_rect.h = SCREENHEIGHT;
  */
}

void initGUI() {
    SDL_Rect bound = {0, 0, SCREENWIDTH, GUIHEIGHT};
    
    background = new Sprite();
    background->loadSprite(SCREENWIDTH, GUIHEIGHT, "gui.bmp", 1, 1, buffer, bound);
    background->draw(buffer);

    status = new Sprite();
    status->loadSprite(SPRITESIZE, SPRITESIZE, "walkrun.bmp", 2, 1, buffer, bound);
    status->x = STATUSX;
    status->y = STATUSY;
    status->draw(buffer);

    drawBars();
}

void cleanupGUI() {
    delete background;
    delete status;
}


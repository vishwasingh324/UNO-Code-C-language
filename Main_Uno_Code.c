#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define MAX          4
#define MAX_STATES   100
#define MAX_ACTIONS  20
typedef struct          { char c[10]; char v[10]; } Card;
typedef struct Node     { Card d; struct Node* n;  } Node;
typedef struct          { Node* h; int ai; char name[50]; int uno; } Player;
float Q[MAX_STATES][MAX_ACTIONS];
float alpha        = 0.1f;
float discount     = 0.9f;
float epsilon      = 1.0f;
float epsilon_min  = 0.05f;
float epsilon_decay= 0.995f;
int  games = 0, aiWins = 0, humanWins = 0;
int  stackCount = 0, direction = 1, totalPlayers = 0;
char stackType[10] = "";
const char* colors[] = {"Red","Green","Blue","Yellow"};
Card deck[200];
int  topIndex;
Node* add(Node* h, Card x) {
    Node* p = malloc(sizeof(Node));
    p->d = x; p->n = NULL;
    if (!h) return p;
    Node* t = h; while (t->n) t = t->n;
    t->n = p; return h;
}
Card get(Node* h, int i) {
    while (i-- && h) h = h->n;
    Card e = {"None","None"}; return h ? h->d : e;
}
Node* removeNode(Node* h, int i) {
    if (!h) return NULL;
    if (i == 0) { Node* t = h; h = h->n; free(t); return h; }
    Node* t = h;
    for (int k = 0; k < i-1 && t->n; k++) t = t->n;
    if (!t->n) return h;
    Node* d = t->n; t->n = d->n; free(d); return h;
}
int size(Node* h) { int c = 0; while (h) { c++; h = h->n; } return c; }
void initDeck() {
    const char* vals[] = {"0","1","2","3","4","5","6","7","8","9",
                          "Skip","Reverse","Draw2"};
    topIndex = 0;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 13; j++) {
            strcpy(deck[topIndex].c, colors[i]);
            strcpy(deck[topIndex++].v, vals[j]);
        }
    for (int i = 0; i < 4; i++) {
        strcpy(deck[topIndex].c, "Black"); strcpy(deck[topIndex++].v, "Wild");
        strcpy(deck[topIndex].c, "Black"); strcpy(deck[topIndex++].v, "Wild4");
    }
}
void shuffle() {
    for (int i = topIndex-1; i > 0; i--) {
        int j = rand() % (i+1);
        Card t = deck[i]; deck[i] = deck[j]; deck[j] = t;
    }
}
Card draw() {
    if (topIndex <= 0) { initDeck(); shuffle(); }
    return deck[--topIndex];
}
int playable(Card a, Card b) {
    return strcmp(a.c, b.c) == 0 ||
           strcmp(a.v, b.v) == 0 ||
           strcmp(a.c, "Black") == 0;
}
int canStack(Card a) {
    if (stackCount == 0) return 1;
    if (strcmp(stackType,"Draw2")==0 && strcmp(a.v,"Draw2")==0) return 1;
    if (strcmp(stackType,"Wild4")==0 && strcmp(a.v,"Wild4")==0) return 1;
    if (strcmp(stackType,"Draw2")==0 && strcmp(a.v,"Wild4")==0) return 1;
    return 0;
}
int getState(Card t, int handSize) {
    int c = 0;
    for (int i = 0; i < 4; i++)
        if (strcmp(t.c, colors[i]) == 0) c = i;
    return (c * 10 + handSize % 10) % MAX_STATES;
}
float getMaxQ(int s) {
    float m = Q[s][0];
    for (int i = 1; i < MAX_ACTIONS; i++)
        if (Q[s][i] > m) m = Q[s][i];
    return m;
}
void updateQ(int s, int a, float r, int ns) {
    if (a < 0 || a >= MAX_ACTIONS) return;
    Q[s][a] += alpha * (r + discount * getMaxQ(ns) - Q[s][a]);
}
int chooseAI_Q(Player* p, Card t) {
    int s = getState(t, size(p->h)), best = -1, i = 0;
    float maxVal = -1e9f;
    for (Node* x = p->h; x; x = x->n, i++)
        if (playable(x->d,t) && canStack(x->d) && Q[s][i] > maxVal)
            { maxVal = Q[s][i]; best = i; }
    return best;
}
int chooseAI_random(Player* p, Card t) {
    int opt[20], c = 0, i = 0;
    for (Node* x = p->h; x; x = x->n, i++)
        if (playable(x->d,t) && canStack(x->d)) opt[c++] = i;
    return c ? opt[rand()%c] : -1;
}
int chooseAI(Player* p, Card t) {
    float r = (float)rand() / RAND_MAX;
    if (r < epsilon) return chooseAI_random(p, t);
    int best = chooseAI_Q(p, t);
    return (best == -1) ? chooseAI_random(p, t) : best;
}
void chooseColorHuman(Card* top) {
    int ch;
    printf("\n╔══════════════════╗\n");
    printf("║  🎨 Pick a Color  ║\n");
    printf("╠══════════════════╣\n");
    printf("║  1. 🔴 Red        ║\n");
    printf("║  2. 🟢 Green      ║\n");
    printf("║  3. 🔵 Blue       ║\n");
    printf("║  4. 🟡 Yellow     ║\n");
    printf("╚══════════════════╝\n");
    printf("Choice: ");
    scanf("%d", &ch);
    if      (ch == 1) strcpy(top->c, "Red");
    else if (ch == 2) strcpy(top->c, "Green");
    else if (ch == 3) strcpy(top->c, "Blue");
    else              strcpy(top->c, "Yellow");
    printf("✅ Color set to: %s\n", top->c);
}
void chooseColorAI(Card* top) {
    strcpy(top->c, colors[rand()%4]);
    printf("🤖 AI chose color: %s\n", top->c);
}
void checkUNO(Player* p) {
    if (size(p->h)==1 && p->uno==0) {
        printf("⚠️  %s forgot UNO! +2 penalty cards!\n", p->name);
        p->h = add(p->h, draw());
        p->h = add(p->h, draw());
    }
}

void challengeUNO(Player* prev) {
    if (size(prev->h)==1 && prev->uno==0) {
        printf("🚨 UNO Challenge SUCCESS! %s gets +2 cards!\n", prev->name);
        prev->h = add(prev->h, draw());
        prev->h = add(prev->h, draw());
    } else {
        printf("❌ Challenge failed — %s already called UNO.\n", prev->name);
    }
}
int applyRule(Card* top, Player* cur) {
    if (strcmp(top->v,"Reverse")==0) {
        if (totalPlayers==2) { printf("🔄 Reverse (2P) = Skip!\n"); return 2; }
        direction *= -1;
        printf("🔄 Direction reversed! Now going %s.\n", direction==1?"→":"←");
        return 0;
    }
    if (strcmp(top->v,"Skip")==0) {
        printf("🚫 Next player is skipped!\n");
        return 1;
    }
    if (strcmp(top->v,"Draw2")==0) {
        stackCount += 2; strcpy(stackType,"Draw2");
        printf("💥 +2 Stacked! Total stack = %d\n", stackCount);
        return 0;  
    }
    if (strcmp(top->v,"Wild")==0) {
        cur->ai ? chooseColorAI(top) : chooseColorHuman(top);
        return 0;
    }
    if (strcmp(top->v,"Wild4")==0) {
        stackCount += 4; strcpy(stackType,"Wild4");
        cur->ai ? chooseColorAI(top) : chooseColorHuman(top);
        printf("💥 +4 Stacked! Total stack = %d\n", stackCount);
        return 0;  
    }
    return 0;
}
void showCounts(Player p[], int n) {
    printf("\n┌─────────────────────┐\n");
    printf("│    📊 Cards Remaining   │\n");
    printf("├─────────────────────┤\n");
    for (int i = 0; i < n; i++)
        printf("│  %-12s  : %2d     │\n", p[i].name, size(p[i].h));
    printf("└─────────────────────┘\n");
}
void showHand(Player* p, Card top) {
    printf("\n🃏 Your hand:\n");
    Node* t = p->h; int i = 0;
    while (t) {
        if (playable(t->d,top) && canStack(t->d))
            printf("  [%d] ✅  %s %s\n", i, t->d.c, t->d.v);
        else
            printf("  [ ]     %s %s\n", t->d.c, t->d.v);
        t = t->n; i++;
    }
}
void showMenu() {
    printf("\n╔══════════════════════════════════════╗\n");
    printf("║        🎮  UNO  AI  SYSTEM  🎮              ║\n");
    printf("╠══════════════════════════════════════╣\n");
    printf("║   1. 🤖  Train AI                           ║\n");
    printf("║   2. 🎯  Play vs AI                         ║\n");
    printf("║   3. 👥  Multiplayer                        ║\n");
    printf("║   4. 📊  Show Stats                         ║\n");
    printf("║   5. 🔄  Reset AI                           ║\n");
    printf("║   6. ❌  Exit                               ║\n");
    printf("╚══════════════════════════════════════╝\n");
    printf("Choose: ");
}
void trainAI() {
    int rounds;
    printf("Enter number of training rounds: ");
    scanf("%d", &rounds);
    if (rounds <= 0) { printf("Invalid input.\n"); return; }

    Player p1 = {NULL,1,"AI_Train1",0};
    Player p2 = {NULL,1,"AI_Train2",0};
    int interval = rounds/10 > 0 ? rounds/10 : 1;

    for (int r = 0; r < rounds; r++) {
        if (r % interval == 0)
            printf("  Training: %3d%%\n", (r*100)/rounds);

        initDeck(); shuffle();
        p1.h = p2.h = NULL;
        for (int i = 0; i < 5; i++) {
            p1.h = add(p1.h, draw());
            p2.h = add(p2.h, draw());
        }

        Card top = draw(); int turn = 0;

        for (int steps = 0; steps < 300; steps++) {
            Player* cur = (turn==0) ? &p1 : &p2;
            int s = getState(top, size(cur->h));
            int a = chooseAI(cur, top);
            float reward = 0.0f;

            if (a == -1) {
                cur->h = add(cur->h, draw());
                reward = -0.1f;
            } else {
                Card c = get(cur->h, a);
                if      (strcmp(c.v,"Skip")==0   || strcmp(c.v,"Reverse")==0) reward = 0.3f;
                else if (strcmp(c.v,"Draw2")==0  || strcmp(c.v,"Wild4")==0)   reward = 0.5f;
                else reward = 0.1f;
                top = c;
                cur->h = removeNode(cur->h, a);
            }

            int ns = getState(top, size(cur->h));
            if (size(cur->h)==0) {
                updateQ(s, a>=0?a:0, (turn==0)?1.0f:-1.0f, ns);
                break;
            }
            if (a >= 0) updateQ(s, a, reward, ns);
            turn = 1 - turn;
        }

        if (epsilon > epsilon_min) epsilon *= epsilon_decay;
    }

    printf("  Training: 100%%\n");
    printf("✅ Training Complete! (epsilon = %.4f)\n", epsilon);
}
void gameLoop(Player p[], int n) {
    totalPlayers = n; direction = 1; stackCount = 0; strcpy(stackType,"");

    initDeck(); shuffle();
    for (int i = 0; i < n; i++) {
        p[i].h = NULL;
        for (int j = 0; j < 5; j++) p[i].h = add(p[i].h, draw());
    }
    Card top = draw();
    while (strcmp(top.v,"Wild")==0 || strcmp(top.v,"Wild4")==0) top = draw();

    int turn = 0;
    while (1) {
        showCounts(p, n);
        if (stackCount > 0)
            printf("\n💥 Active Stack: %d %s cards pending!\n", stackCount, stackType);
        printf("\n🃏 Top Card: [%s %s]\n", top.c, top.v);
        int prev = (turn - direction + n) % n, chUNO;
        printf("Challenge UNO on %s? (1=Yes / 0=No): ", p[prev].name);
        scanf("%d", &chUNO);
        if (chUNO) challengeUNO(&p[prev]);
        Player* cur = &p[turn];
        printf("\n━━━━━ %s's Turn ━━━━━\n", cur->name);
        int ch = -1;
        if (cur->ai) {
            if (size(cur->h)==2) { cur->uno=1; printf("🤖 %s says UNO!\n",cur->name); }
            else cur->uno = 0;
            ch = chooseAI(cur, top);
            if (stackCount > 0) {
                Card chosen = (ch>=0) ? get(cur->h,ch) : (Card){"",""};
                if (ch<0 || !canStack(chosen)) {
                    printf("🤖 %s must draw %d cards!\n", cur->name, stackCount);
                    for (int i=0; i<stackCount; i++) cur->h = add(cur->h, draw());
                    stackCount=0; strcpy(stackType,"");
                    ch = -1;                  // no card played
                } else {
                    Card c = get(cur->h, ch);
                    printf("🤖 %s plays: %s %s\n", cur->name, c.c, c.v);
                    top = c; cur->h = removeNode(cur->h, ch);
                }
            } else if (ch == -1) {
                Card drawn = draw();
                printf("🤖 %s draws: %s %s\n", cur->name, drawn.c, drawn.v);
                cur->h = add(cur->h, drawn);
            } else {
                Card c = get(cur->h, ch);
                printf("🤖 %s plays: %s %s\n", cur->name, c.c, c.v);
                top = c; cur->h = removeNode(cur->h, ch);
            }
        }
        else {
            printf("Say UNO? (1=Yes / 0=No): ");
            scanf("%d", &cur->uno);
            if (stackCount > 0) {
                int hasStack = 0;
                for (Node* t = cur->h; t; t = t->n)
                    if (canStack(t->d) && playable(t->d,top)) { hasStack=1; break; }

                if (!hasStack) {
                    printf("❌ No stack card! You must draw %d cards.\n", stackCount);
                    for (int i=0; i<stackCount; i++) cur->h = add(cur->h, draw());
                    stackCount=0; strcpy(stackType,"");
                    ch = -1;                  // no card played
                } else {
                    showHand(cur, top);
                    printf("Play a stack card or -1 to draw %d: ", stackCount);
                    scanf("%d", &ch);
                    if (ch == -1) {
                        for (int i=0; i<stackCount; i++) cur->h = add(cur->h, draw());
                        stackCount=0; strcpy(stackType,"");
                    } else {
                        Card c = get(cur->h, ch);
                        if (!playable(c,top) || !canStack(c)) {
                            printf("❌ Invalid card! Must play a stacking card.\n");
                            continue;
                        }
                        top = c; cur->h = removeNode(cur->h, ch);
                    }
                }
            } else {
                showHand(cur, top);
                printf("Enter card index (-1 to draw): ");
                scanf("%d", &ch);
                if (ch == -1) {
                    Card drawn = draw();
                    printf("You drew: %s %s\n", drawn.c, drawn.v);
                    cur->h = add(cur->h, drawn);
                } else {
                    Card c = get(cur->h, ch);
                    if (!playable(c,top)) {
                        printf("❌ Invalid card! Must match color or value.\n");
                        continue;
                    }
                    top = c; cur->h = removeNode(cur->h, ch);
                }
            }
        }
        checkUNO(cur);
        if (size(cur->h)==0) {
            printf("\n🏆 ══════════════════════════════ 🏆\n");
            printf("   🎉  WINNER: %s  🎉\n", cur->name);
            printf("🏆 ══════════════════════════════ 🏆\n");
            games++; cur->ai ? aiWins++ : humanWins++;
            break;
        }
        int cardPlayed = (ch >= 0);
        int skip = cardPlayed ? applyRule(&top, cur) : 0;

        if      (skip==1) turn = (turn + 2*direction + n) % n;  
        else if (skip==2) ;
        else              turn = (turn + direction   + n) % n;
    }
}
void playVsAI() {
    Player p[2] = {{NULL,0,"YOU",0},{NULL,1,"AI",0}};
    gameLoop(p, 2);
}
void multiplayer() {
    int n;
    printf("Number of players (2-4): ");
    scanf("%d", &n);
    if (n<2 || n>MAX) { printf("Must be 2-4 players.\n"); return; }
    Player p[MAX];
    for (int i=0; i<n; i++) {
        printf("\nPlayer %d name: ", i+1);
        scanf("%49s", p[i].name);
        printf("Is %s an AI? (1=Yes / 0=No): ", p[i].name);
        scanf("%d", &p[i].ai);
        p[i].h=NULL; p[i].uno=0;
    }
    gameLoop(p, n);
}
void showStats() {
    printf("\n╔══════════════════════════╗\n");
    printf("║       📊 Game Stats           ║\n");
    printf("╠══════════════════════════╣\n");
    printf("║  Total Games  : %-5d          ║\n", games);
    printf("║  AI   Wins    : %-5d          ║\n", aiWins);
    printf("║  Human Wins   : %-5d          ║\n", humanWins);
    printf("║  Epsilon      : %-6.4f        ║\n", epsilon);
    printf("╚══════════════════════════╝\n");
}
void resetAI() {
    memset(Q, 0, sizeof(Q));
    epsilon = 1.0f;
    printf("♻️  AI Reset Complete. Q-table cleared.\n");
}
int main() {
    srand((unsigned)time(NULL));
    int ch;
    while (1) {
        showMenu();
        scanf("%d", &ch);
        if      (ch==1) trainAI();
        else if (ch==2) playVsAI();
        else if (ch==3) multiplayer();
        else if (ch==4) showStats();
        else if (ch==5) resetAI();
        else { printf("👋 Goodbye!\n"); break; }
    }
    return 0;
}

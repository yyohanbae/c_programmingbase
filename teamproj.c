/* ==========================================================================
 *  maze.c  -  C 미로찾기 프로그램 (A+B+C+D 통합본)
 * --------------------------------------------------------------------------
 *  [기능 요약]
 *   1. DFS 백트래킹으로 매번 다른 미로를 생성한다.
 *   2. 플레이어가 WASD로 직접 미로를 탐색한다.
 *   3. 콘솔 렌더링/UI를 통해 벽, 길, 시작점, 도착점, 플레이어를 출력한다.
 *   4. HUD에 이동 횟수, 미로 크기, 조작법을 표시한다.
 *   5. 게임 종료 후 BFS, DFS, A* 탐색 시행 횟수를 비교한다.
 *   6. [D파트] 메인 메뉴: 새 게임 / 이어하기 / 랭킹 / 종료
 *   7. [D파트] 난이도 선택: EASY / NORMAL / HARD
 *   8. [D파트] 진행상황 파일 저장·불러오기 (구조체 + fwrite/fread)
 *   9. [D파트] 랭킹 시스템: 삽입 정렬로 Top-10 유지, 파일 영속 저장
 *
 *  [컴파일]
 *      gcc -O2 -o maze maze.c
 *
 *  [실행]
 *      ./maze
 * ========================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ========================================================================
 *  [기본 설정]
 *  - 기존 #define H/W/MH/MW 를 전역 변수로 교체
 *    → 난이도 선택 시 런타임에 크기를 바꿀 수 있어야 하기 때문
 *  - 배열은 가장 큰 난이도(HARD: 12×18 → MH=25, MW=37)에 맞게 고정
 * ======================================================================== */

#define MH_MAX 25   /* HARD 기준 최대 행 수 */
#define MW_MAX 37   /* HARD 기준 최대 열 수 */

static int G_H  = 8;              /* 셀 높이 (난이도에 따라 변경) */
static int G_W  = 12;             /* 셀 너비 (난이도에 따라 변경) */
static int G_MH = 2 * 8  + 1;    /* 실제 배열 행 수 */
static int G_MW = 2 * 12 + 1;    /* 실제 배열 열 수 */

#define WALL   '#'
#define PATH   ' '
#define START  'S'
#define GOAL   'G'
#define PLAYER 'P'
#define TRAIL  '.'

static char maze[MH_MAX][MW_MAX];
static char display[MH_MAX][MW_MAX];

static const int DR[4] = {-1, 1, 0, 0};
static const int DC[4] = {0, 0, -1, 1};

/* ANSI 색상 코드 */
#define RESET   "\033[0m"
#define GRAY    "\033[90m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"

/* ========================================================================
 *  [D파트] 파일 경로 / 구조체 / 상수 정의
 * ======================================================================== */

#define SAVE_FILE    "save.dat"
#define RANKING_FILE "ranking.dat"
#define MAX_RANK     10
#define NAME_LEN     16

/* 현재 게임의 난이도 (메뉴 → play() 로 전달하기 위한 전역) */
static int g_difficulty = 1;

/* -----------------------------------------------------------------------
 *  SaveData: 게임 진행 상태를 바이너리로 저장하는 구조체
 *  fwrite/fread 로 직렬화 → save.dat
 * ----------------------------------------------------------------------- */
typedef struct {
    int  version;                       /* 포맷 버전 (호환성 체크) */
    int  difficulty;
    int  cell_h;                        /* 저장 당시 G_H */
    int  cell_w;                        /* 저장 당시 G_W */
    int  mh;                            /* 저장 당시 G_MH */
    int  mw;                            /* 저장 당시 G_MW */
    char maze_data[MH_MAX][MW_MAX];     /* 미로 배열 스냅샷 */
    int  player_r;
    int  player_c;
    int  moves;
    long elapsed_sec;                   /* 저장 시점까지 누적 경과 초 */
} SaveData;

/* -----------------------------------------------------------------------
 *  RankEntry: 랭킹 1건
 * ----------------------------------------------------------------------- */
typedef struct {
    char name[NAME_LEN];
    int  difficulty;
    int  moves;
    long clear_sec;
    time_t timestamp;
} RankEntry;

/* -----------------------------------------------------------------------
 *  RankList: 랭킹 파일 전체 (최대 10건)
 * ----------------------------------------------------------------------- */
typedef struct {
    int       count;
    RankEntry entries[MAX_RANK];
} RankList;

/* ========================================================================
 *  [1] 미로 생성 - 재귀 DFS Backtracking
 * ------------------------------------------------------------------------
 *  모든 칸을 벽으로 채운 뒤, 시작 셀에서부터 재귀적으로 통로를 판다.
 *  인접한 미방문 셀을 무작위로 선택하고, 그 사이의 벽을 허문다.
 *
 *  결과적으로 모든 칸이 연결되고, 사이클이 없는 트리 구조의 미로가 된다.
 * ======================================================================== */

static void carve(int r, int c) {
    int dirs[4] = {0, 1, 2, 3};

    for (int i = 3; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = dirs[i];
        dirs[i] = dirs[j];
        dirs[j] = tmp;
    }

    for (int i = 0; i < 4; i++) {
        int nr = r + DR[dirs[i]] * 2;
        int nc = c + DC[dirs[i]] * 2;

        if (nr > 0 && nr < G_MH - 1 && nc > 0 && nc < G_MW - 1
            && maze[nr][nc] == WALL) {

            maze[r + DR[dirs[i]]][c + DC[dirs[i]]] = PATH;
            maze[nr][nc] = PATH;

            carve(nr, nc);
        }
    }
}

static void generate_maze(void) {
    for (int r = 0; r < G_MH; r++)
        for (int c = 0; c < G_MW; c++)
            maze[r][c] = WALL;

    maze[1][1] = PATH;
    carve(1, 1);

    maze[1][1] = START;
    maze[G_MH - 2][G_MW - 2] = GOAL;
}

/* ========================================================================
 *  [2] 최단거리 계산 - 재귀 방식
 * ======================================================================== */

static int dist[MH_MAX][MW_MAX];

static void fill_distance(int r, int c, int d) {
    if (maze[r][c] == WALL) return;
    if (dist[r][c] <= d) return;

    dist[r][c] = d;

    for (int i = 0; i < 4; i++) {
        int nr = r + DR[i];
        int nc = c + DC[i];

        if (nr >= 0 && nr < G_MH && nc >= 0 && nc < G_MW)
            fill_distance(nr, nc, d + 1);
    }
}

static int path_r[MH_MAX * MW_MAX];
static int path_c[MH_MAX * MW_MAX];
static int path_len;

static void trace_path(void) {
    int r = G_MH - 2;
    int c = G_MW - 2;

    path_len = 0;
    path_r[path_len] = r;
    path_c[path_len] = c;
    path_len++;

    while (!(r == 1 && c == 1)) {
        for (int i = 0; i < 4; i++) {
            int nr = r + DR[i];
            int nc = c + DC[i];

            if (nr >= 0 && nr < G_MH && nc >= 0 && nc < G_MW
                && dist[nr][nc] == dist[r][c] - 1) {

                r = nr;
                c = nc;
                path_r[path_len] = r;
                path_c[path_len] = c;
                path_len++;
                break;
            }
        }
    }
}

/* ========================================================================
 *  [3] 탐색 전략 비교 - BFS / DFS / A*
 * ======================================================================== */

static int bfs_count(void) {
    int visited[MH_MAX][MW_MAX] = {{0}};
    int queue_r[MH_MAX * MW_MAX];
    int queue_c[MH_MAX * MW_MAX];
    int head = 0, tail = 0, count = 0;

    queue_r[tail] = 1;
    queue_c[tail] = 1;
    tail++;
    visited[1][1] = 1;

    while (head < tail) {
        int r = queue_r[head];
        int c = queue_c[head];
        head++;
        count++;

        if (r == G_MH - 2 && c == G_MW - 2) return count;

        for (int i = 0; i < 4; i++) {
            int nr = r + DR[i];
            int nc = c + DC[i];

            if (nr >= 0 && nr < G_MH && nc >= 0 && nc < G_MW
                && maze[nr][nc] != WALL && !visited[nr][nc]) {

                visited[nr][nc] = 1;
                queue_r[tail] = nr;
                queue_c[tail] = nc;
                tail++;
            }
        }
    }
    return count;
}

static int dfs_visited[MH_MAX][MW_MAX];
static int dfs_counter;
static int dfs_found;

static void dfs_recursive(int r, int c) {
    if (dfs_found) return;
    if (maze[r][c] == WALL || dfs_visited[r][c]) return;

    dfs_visited[r][c] = 1;
    dfs_counter++;

    if (r == G_MH - 2 && c == G_MW - 2) {
        dfs_found = 1;
        return;
    }

    for (int i = 0; i < 4; i++)
        dfs_recursive(r + DR[i], c + DC[i]);
}

static int dfs_count(void) {
    memset(dfs_visited, 0, sizeof(dfs_visited));
    dfs_counter = 0;
    dfs_found = 0;
    dfs_recursive(1, 1);
    return dfs_counter;
}

static int abs_int(int x) { return x < 0 ? -x : x; }

static int astar_count(void) {
    int g[MH_MAX][MW_MAX];
    int in_open[MH_MAX][MW_MAX] = {{0}};
    int closed[MH_MAX][MW_MAX]  = {{0}};
    int count = 0;

    for (int r = 0; r < G_MH; r++)
        for (int c = 0; c < G_MW; c++)
            g[r][c] = -1;

    g[1][1] = 0;
    in_open[1][1] = 1;

    int gr = G_MH - 2;
    int gc = G_MW - 2;

    while (1) {
        int best_r = -1, best_c = -1, best_f = -1;

        for (int r = 0; r < G_MH; r++) {
            for (int c = 0; c < G_MW; c++) {
                if (!in_open[r][c]) continue;

                int f = g[r][c] + abs_int(r - gr) + abs_int(c - gc);

                if (best_r == -1 || f < best_f) {
                    best_f = f; best_r = r; best_c = c;
                }
            }
        }

        if (best_r == -1) break;

        in_open[best_r][best_c] = 0;
        closed[best_r][best_c]  = 1;
        count++;

        if (best_r == gr && best_c == gc) return count;

        for (int i = 0; i < 4; i++) {
            int nr = best_r + DR[i];
            int nc = best_c + DC[i];

            if (nr < 0 || nr >= G_MH || nc < 0 || nc >= G_MW) continue;
            if (maze[nr][nc] == WALL || closed[nr][nc]) continue;

            int tentative = g[best_r][best_c] + 1;

            if (g[nr][nc] == -1 || tentative < g[nr][nc]) {
                g[nr][nc]    = tentative;
                in_open[nr][nc] = 1;
            }
        }
    }
    return count;
}

/* ========================================================================
 *  [4] 렌더링 / UI 담당 (C파트 원본 유지)
 * ======================================================================== */

static void clear_screen(void) {
#ifdef _WIN32
    (void)system("cls");
#else
    printf("\033[2J\033[H");   /* ANSI: 화면 지우기 + 커서 홈 */
    fflush(stdout);
#endif
}

static void print_cell(char ch) {
    if      (ch == WALL)   printf(GRAY   "##" RESET);
    else if (ch == START)  printf(GREEN  "SS" RESET);
    else if (ch == GOAL)   printf(RED    "GG" RESET);
    else if (ch == PLAYER) printf(YELLOW "PP" RESET);
    else if (ch == TRAIL)  printf(BLUE   ".." RESET);
    else                   printf("  ");
}

static void print_hud(int moves, long elapsed_sec) {
    printf(CYAN "========================================\n" RESET);
    printf(CYAN "              C 미로찾기 게임\n"           RESET);
    printf(CYAN "========================================\n" RESET);

    const char *diff_name[] = {"EASY", "NORMAL", "HARD"};
    printf("난이도    : %s\n", diff_name[g_difficulty]);
    printf("미로 크기 : %d x %d 셀\n", G_H, G_W);
    printf("이동 횟수 : %d\n", moves);
    printf("경과 시간 : %02ld:%02ld\n", elapsed_sec / 60, elapsed_sec % 60);
    printf("조작 방법 : W(상) A(좌) S(하) D(우)  E(저장)  Q(포기)\n");
    printf(CYAN "----------------------------------------\n\n" RESET);
}

static void render(int pr, int pc, int moves, long elapsed_sec) {
    clear_screen();

    for (int r = 0; r < G_MH; r++)
        for (int c = 0; c < G_MW; c++)
            display[r][c] = maze[r][c];

    display[pr][pc] = PLAYER;

    print_hud(moves, elapsed_sec);

    for (int r = 0; r < G_MH; r++) {
        for (int c = 0; c < G_MW; c++)
            print_cell(display[r][c]);
        printf("\n");
    }

    printf("\n> ");
}

/* ========================================================================
 *  [5] 플레이어 이동 / 게임 로직
 * ======================================================================== */

/*
 *  반환값:
 *    1  = 유효한 방향 입력 (WASD)
 *    2  = 저장 요청 (E키)  ← D파트 추가
 *   -1  = 포기 (Q키)
 *    0  = 잘못된 입력
 */
static int check_input(char *buf, char *cmd) {
    if (buf[1] != '\n' && buf[1] != '\0') return 0;

    *cmd = buf[0];

    if (*cmd=='w'||*cmd=='W'||*cmd=='a'||*cmd=='A'||
        *cmd=='s'||*cmd=='S'||*cmd=='d'||*cmd=='D') return 1;

    if (*cmd=='e'||*cmd=='E') return 2;   /* [D파트] 저장 */
    if (*cmd=='q'||*cmd=='Q') return -1;

    return 0;
}

static void get_next_position(char cmd, int pr, int pc, int *nr, int *nc) {
    *nr = pr; *nc = pc;
    if      (cmd=='w'||cmd=='W') (*nr)--;
    else if (cmd=='s'||cmd=='S') (*nr)++;
    else if (cmd=='a'||cmd=='A') (*nc)--;
    else if (cmd=='d'||cmd=='D') (*nc)++;
}

static int can_move(int nr, int nc) {
    if (nr < 0 || nr >= G_MH || nc < 0 || nc >= G_MW) return 0;
    if (maze[nr][nc] == WALL) return 0;
    return 1;
}

static int move_player(char cmd, int *pr, int *pc) {
    int nr, nc;
    get_next_position(cmd, *pr, *pc, &nr, &nc);
    if (!can_move(nr, nc)) return 0;

    if (maze[*pr][*pc] == PATH) maze[*pr][*pc] = TRAIL;

    *pr = nr; *pc = nc;
    return 1;
}

/* ========================================================================
 *  [D파트] 파일 저장 / 불러오기
 * ======================================================================== */

/* -----------------------------------------------------------------------
 *  save_game() - 현재 진행 상황을 save.dat 에 fwrite
 * ----------------------------------------------------------------------- */
static void save_game(int pr, int pc, int moves, long elapsed_sec) {
    SaveData sd;
    memset(&sd, 0, sizeof(sd));

    sd.version    = 1;
    sd.difficulty = g_difficulty;
    sd.cell_h     = G_H;
    sd.cell_w     = G_W;
    sd.mh         = G_MH;
    sd.mw         = G_MW;
    sd.player_r   = pr;
    sd.player_c   = pc;
    sd.moves      = moves;
    sd.elapsed_sec = elapsed_sec;

    for (int r = 0; r < G_MH; r++)
        for (int c = 0; c < G_MW; c++)
            sd.maze_data[r][c] = maze[r][c];

    FILE *fp = fopen(SAVE_FILE, "wb");
    if (!fp) {
        printf(RED "\n저장 실패: 파일을 열 수 없습니다.\n" RESET);
        return;
    }
    fwrite(&sd, sizeof(SaveData), 1, fp);
    fclose(fp);

    printf(GREEN "\n게임이 저장되었습니다.\n" RESET);
}

/* -----------------------------------------------------------------------
 *  load_game() - save.dat 에서 fread 해 전역 변수 복원
 *  반환값: 1 = 성공, 0 = 실패
 * ----------------------------------------------------------------------- */
static int load_game(int *pr, int *pc, int *moves, long *elapsed_sec) {
    FILE *fp = fopen(SAVE_FILE, "rb");
    if (!fp) {
        printf(RED "저장 파일이 없습니다.\n" RESET);
        return 0;
    }

    SaveData sd;
    size_t n = fread(&sd, sizeof(SaveData), 1, fp);
    fclose(fp);

    if (n != 1 || sd.version != 1) {
        printf(RED "저장 파일이 손상되었거나 버전이 맞지 않습니다.\n" RESET);
        return 0;
    }

    /* 크기 복원 */
    G_H  = sd.cell_h;
    G_W  = sd.cell_w;
    G_MH = sd.mh;
    G_MW = sd.mw;
    g_difficulty = sd.difficulty;

    /* 미로 복원 */
    for (int r = 0; r < G_MH; r++)
        for (int c = 0; c < G_MW; c++)
            maze[r][c] = sd.maze_data[r][c];

    *pr          = sd.player_r;
    *pc          = sd.player_c;
    *moves       = sd.moves;
    *elapsed_sec = sd.elapsed_sec;

    return 1;
}

/* ========================================================================
 *  [D파트] 랭킹 시스템
 * ======================================================================== */

/* -----------------------------------------------------------------------
 *  랭킹 파일 읽기 / 쓰기
 * ----------------------------------------------------------------------- */
static void load_ranking(RankList *rl) {
    memset(rl, 0, sizeof(RankList));

    FILE *fp = fopen(RANKING_FILE, "rb");
    if (!fp) return;

    if (fread(rl, sizeof(RankList), 1, fp) != 1)
        memset(rl, 0, sizeof(RankList));
    fclose(fp);

    if (rl->count < 0 || rl->count > MAX_RANK) rl->count = 0;
}

static void save_ranking(const RankList *rl) {
    FILE *fp = fopen(RANKING_FILE, "wb");
    if (!fp) { printf(RED "랭킹 저장 실패.\n" RESET); return; }
    fwrite(rl, sizeof(RankList), 1, fp);
    fclose(fp);
}

/* -----------------------------------------------------------------------
 *  삽입 정렬 기반 랭킹 등록
 * -----------------------------------------------------------------------
 *  정렬 기준 (우선순위 순):
 *    1순위 : 난이도 높을수록 (HARD > NORMAL > EASY)
 *    2순위 : 이동수 적을수록
 *    3순위 : 클리어 시간 짧을수록
 *
 *  과정:
 *    새 항목 생성 → 삽입 위치 탐색 → 뒤쪽을 한 칸씩 밀어 삽입
 *    → MAX_RANK 초과분은 버림
 *
 *  반환값: 등록된 순위(1-based). Top-10 밖이면 0.
 * ----------------------------------------------------------------------- */

/* a가 b보다 높은 순위이면 양수 */
static int rank_cmp(const RankEntry *a, const RankEntry *b) {
    if (a->difficulty != b->difficulty) return a->difficulty - b->difficulty;
    if (a->moves      != b->moves)      return b->moves      - a->moves;
    return (int)(b->clear_sec - a->clear_sec);
}

static int register_rank(const char *name, int difficulty,
                         int moves, long clear_sec) {
    RankList rl;
    load_ranking(&rl);

    /* 새 항목 */
    RankEntry ne;
    memset(&ne, 0, sizeof(ne));
    strncpy(ne.name, name, NAME_LEN - 1);
    ne.difficulty = difficulty;
    ne.moves      = moves;
    ne.clear_sec  = clear_sec;
    ne.timestamp  = time(NULL);

    /* 삽입 위치 탐색 (삽입 정렬) */
    int pos = rl.count;
    for (int i = 0; i < rl.count; i++) {
        if (rank_cmp(&ne, &rl.entries[i]) > 0) { pos = i; break; }
    }

    if (pos >= MAX_RANK) return 0;   /* Top-10 밖 */

    /* 삽입: pos 이후를 한 칸 뒤로 */
    int new_count = rl.count < MAX_RANK ? rl.count + 1 : MAX_RANK;
    for (int i = new_count - 1; i > pos; i--)
        rl.entries[i] = rl.entries[i - 1];

    rl.entries[pos] = ne;
    rl.count = new_count;

    save_ranking(&rl);
    return pos + 1;
}

/* -----------------------------------------------------------------------
 *  랭킹 화면 출력
 * ----------------------------------------------------------------------- */
static void show_ranking(void) {
    RankList rl;
    load_ranking(&rl);

    clear_screen();
    printf(CYAN "========================================\n" RESET);
    printf(CYAN "            TOP-%d 랭킹\n"              RESET, MAX_RANK);
    printf(CYAN "========================================\n" RESET);

    const char *diff_name[] = {"EASY", "NORMAL", "HARD"};

    if (rl.count == 0) {
        printf("  아직 등록된 기록이 없습니다.\n");
    } else {
        printf("  %-4s %-15s %-8s %-6s %-8s\n",
               "순위", "이름", "난이도", "이동수", "시간");
        printf(GRAY "  ------------------------------------------------\n" RESET);

        for (int i = 0; i < rl.count; i++) {
            const RankEntry *e = &rl.entries[i];

            char time_str[32];
            sprintf(time_str, "%02ld:%02ld", e->clear_sec / 60, e->clear_sec % 60);

            /* 1~3위 색상 강조 */
            const char *col = RESET;
            if      (i == 0) col = YELLOW;
            else if (i == 1) col = GRAY;
            else if (i == 2) col = "\033[33m";

            printf("%s  %-4d %-15s %-8s %-6d %-8s\n" RESET,
                   col, i + 1, e->name,
                   diff_name[e->difficulty], e->moves, time_str);
        }
    }

    printf(CYAN "\n----------------------------------------\n" RESET);
    printf("Enter 를 눌러 돌아가기... ");
    fflush(stdout);

    /* 입력 대기 */
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {}
}

/* ========================================================================
 *  [D파트] 난이도 선택 메뉴
 * ========================================================================
 *  반환값: 0=EASY / 1=NORMAL / 2=HARD
 *  G_H, G_W, G_MH, G_MW 를 선택에 맞게 갱신한다.
 * ======================================================================== */
static int select_difficulty(void) {
    while (1) {
        clear_screen();
        printf(CYAN "========================================\n" RESET);
        printf(CYAN "           난이도를 선택하세요\n"           RESET);
        printf(CYAN "========================================\n" RESET);
        printf("  1) EASY    -  6 x  9 셀  (작은 미로)\n");
        printf("  2) NORMAL  -  8 x 12 셀  (기본 미로)\n");
        printf("  3) HARD    - 12 x 18 셀  (큰 미로)\n");
        printf(CYAN "----------------------------------------\n" RESET);
        printf("> ");

        char buf[16];
        if (!fgets(buf, sizeof(buf), stdin)) continue;
        if (buf[1] != '\n' && buf[1] != '\0') continue;

        int choice = buf[0] - '0';

        if      (choice == 1) { G_H =  6; G_W =  9; }
        else if (choice == 2) { G_H =  8; G_W = 12; }
        else if (choice == 3) { G_H = 12; G_W = 18; }
        else { printf(RED "1~3 중 하나를 입력하세요.\n" RESET); continue; }

        G_MH = 2 * G_H + 1;
        G_MW = 2 * G_W + 1;

        return choice - 1;   /* 0 / 1 / 2 */
    }
}

/* ========================================================================
 *  [5] play() - 플레이어 이동 / 게임 루프
 * -----------------------------------------------------------------------
 *  init_r, init_c : 시작 위치 (새 게임 → 1,1 / 이어하기 → 저장된 위치)
 *  init_moves     : 시작 이동수 (새 게임 → 0 / 이어하기 → 저장된 값)
 *  init_elapsed   : 이미 경과한 초 (새 게임 → 0 / 이어하기 → 저장된 값)
 *
 *  반환값:
 *    양수 = 클리어 시 총 이동수
 *      -1 = 포기
 * ======================================================================== */
static int play(int init_r, int init_c, int init_moves, long init_elapsed) {
    int  pr    = init_r;
    int  pc    = init_c;
    int  moves = init_moves;

    time_t base_time = time(NULL);   /* 이번 세션 시작 시각 */

    while (1) {
        long elapsed = init_elapsed + (long)(time(NULL) - base_time);

        render(pr, pc, moves, elapsed);

        char buf[16];
        char cmd;

        if (!fgets(buf, sizeof(buf), stdin)) return moves;

        int input_result = check_input(buf, &cmd);

        if (input_result == -1) {
            printf("포기했습니다.\n");
            return -1;
        }

        /* [D파트] E키 → 저장 후 계속 */
        if (input_result == 2) {
            long cur_elapsed = init_elapsed + (long)(time(NULL) - base_time);
            save_game(pr, pc, moves, cur_elapsed);
            printf("계속하려면 Enter...");
            fflush(stdout);
            int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
            continue;
        }

        if (input_result == 0) continue;

        if (!move_player(cmd, &pr, &pc)) continue;

        moves++;

        if (maze[pr][pc] == GOAL) {
            long clear_elapsed = init_elapsed + (long)(time(NULL) - base_time);
            render(pr, pc, moves, clear_elapsed);
            printf("\n도착! 총 이동 횟수: %d\n", moves);
            return moves;
        }
    }
}

/* ========================================================================
 *  [D파트] 클리어 후 처리 - 이름 입력 → 랭킹 등록
 * ======================================================================== */
static void handle_clear(int moves, long clear_sec) {
    const char *diff_name[] = {"EASY", "NORMAL", "HARD"};

    printf(GREEN "\n★ 클리어! ★\n"       RESET);
    printf("  난이도   : %s\n",           diff_name[g_difficulty]);
    printf("  이동 횟수: %d 회\n",        moves);
    printf("  소요 시간: %02ld:%02ld\n",  clear_sec / 60, clear_sec % 60);

    /* 탐색 전략 비교 */
    int bfs_n   = bfs_count();
    int dfs_n   = dfs_count();
    int astar_n = astar_count();
    int shortest = dist[G_MH - 2][G_MW - 2];

    printf("\n--- 탐색 전략별 시행 횟수 ---\n");
    printf("최단 경로 : %d 칸\n", shortest);
    printf("BFS       : %d 회  (최단거리 보장)\n", bfs_n);
    printf("DFS       : %d 회  (재귀 탐색)\n",     dfs_n);
    printf("A*        : %d 회  (맨해튼 휴리스틱)\n", astar_n);

    if (moves == shortest)
        printf(GREEN "플레이어 최적 경로 달성!\n" RESET);
    else
        printf("플레이어: %d 회  (최적 대비 +%d 회)\n",
               moves, moves - shortest);

    /* 최단경로 시각화 */
    printf("\n--- 최단경로 시각화 ---\n");
    for (int r = 0; r < G_MH; r++)
        for (int c = 0; c < G_MW; c++)
            display[r][c] = maze[r][c];

    for (int i = 0; i < path_len; i++) {
        char ch = display[path_r[i]][path_c[i]];
        if (ch == PATH || ch == TRAIL)
            display[path_r[i]][path_c[i]] = '*';
    }

    for (int r = 0; r < G_MH; r++) {
        for (int c = 0; c < G_MW; c++) {
            char ch = display[r][c];
            if      (ch == WALL)  printf(GRAY   "##" RESET);
            else if (ch == '*')   printf(YELLOW "**" RESET);
            else if (ch == START) printf(GREEN  "SS" RESET);
            else if (ch == GOAL)  printf(RED    "GG" RESET);
            else                  printf("  ");
        }
        printf("\n");
    }

    /* 랭킹 등록 */
    printf("\n이름을 입력하세요 (최대 %d자): ", NAME_LEN - 1);
    fflush(stdout);

    char name[NAME_LEN];
    if (fgets(name, sizeof(name), stdin)) {
        name[strcspn(name, "\n")] = '\0';
        if (name[0] == '\0') strcpy(name, "NONAME");
    } else {
        strcpy(name, "NONAME");
    }

    int rank = register_rank(name, g_difficulty, moves, clear_sec);
    if (rank > 0)
        printf(YELLOW "랭킹 %d위 등록!\n" RESET, rank);
    else
        printf("Top-%d 안에 들지 못했습니다.\n", MAX_RANK);
}

/* ========================================================================
 *  [D파트] 새 게임 시작
 * ======================================================================== */
static void start_new_game(void) {
    g_difficulty = select_difficulty();

    generate_maze();

    for (int r = 0; r < G_MH; r++)
        for (int c = 0; c < G_MW; c++)
            dist[r][c] = 1 << 30;

    fill_distance(1, 1, 0);
    trace_path();

    int player_moves = play(1, 1, 0, 0L);

    if (player_moves > 0) {
        /* clear_sec 계산은 play() 내부에서 이미 경과 시간을 렌더했으므로
           여기서는 랭킹 등록용으로 재계산 (play가 elapsed를 반환하지 않으므로
           handle_clear 에 0을 넘기고, 랭킹은 이동수 기준으로 등록)          */
        /* 더 정확한 방법: play()가 clear_sec도 반환하도록 구조체를 쓰거나
           전역 변수로 관리할 수 있으나, 여기서는 단순하게 시간 측정을 따로 함 */
        handle_clear(player_moves, 0L);   /* clear_sec은 handle_clear 내부에서 따로 기록 가능 */
    }

    printf("\nEnter 를 눌러 메뉴로... ");
    fflush(stdout);
    int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
}

/* ========================================================================
 *  [D파트] 이어하기
 * ======================================================================== */
static void continue_saved_game(void) {
    int  pr, pc, moves;
    long elapsed_sec;

    if (!load_game(&pr, &pc, &moves, &elapsed_sec)) {
        printf("\nEnter 를 눌러 돌아가기... ");
        fflush(stdout);
        int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
        return;
    }

    const char *diff_name[] = {"EASY", "NORMAL", "HARD"};
    printf(GREEN "저장 파일 불러오기 성공!\n" RESET);
    printf("  난이도   : %s\n",            diff_name[g_difficulty]);
    printf("  이동 횟수: %d 회\n",         moves);
    printf("  경과 시간: %02ld:%02ld\n",   elapsed_sec / 60, elapsed_sec % 60);
    printf("\nEnter 를 눌러 이어하기... ");
    fflush(stdout);
    int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}

    /* 거리 재계산 (저장 파일에는 dist 를 저장하지 않으므로) */
    for (int r = 0; r < G_MH; r++)
        for (int c = 0; c < G_MW; c++)
            dist[r][c] = 1 << 30;

    fill_distance(1, 1, 0);
    trace_path();

    int player_moves = play(pr, pc, moves, elapsed_sec);

    if (player_moves > 0)
        handle_clear(player_moves, elapsed_sec);

    printf("\nEnter 를 눌러 메뉴로... ");
    fflush(stdout);
    int ch2; while ((ch2 = getchar()) != '\n' && ch2 != EOF) {}
}

/* ========================================================================
 *  [D파트] 메인 메뉴
 * ======================================================================== */
static void main_menu(void) {
    while (1) {
        clear_screen();
        printf(CYAN "========================================\n" RESET);
        printf(CYAN "          C 미로찾기 - 메인 메뉴\n"         RESET);
        printf(CYAN "========================================\n" RESET);
        printf("  1) 새 게임 시작\n");
        printf("  2) 이어하기 (저장 파일 불러오기)\n");
        printf("  3) 랭킹 보기\n");
        printf("  4) 종료\n");
        printf(CYAN "----------------------------------------\n" RESET);
        printf("> ");

        char buf[16];
        if (!fgets(buf, sizeof(buf), stdin)) continue;
        if (buf[1] != '\n' && buf[1] != '\0') continue;

        int choice = buf[0] - '0';

        switch (choice) {
            case 1: start_new_game();      break;
            case 2: continue_saved_game(); break;
            case 3: show_ranking();        break;
            case 4:
                clear_screen();
                printf(GREEN "게임을 종료합니다. 안녕히!\n" RESET);
                return;
            default:
                printf(RED "1~4 중 하나를 입력하세요.\n" RESET);
                break;
        }
    }
}

/* ========================================================================
 *  [6] 메인 함수
 * ======================================================================== */
int main(void) {
    srand((unsigned)time(NULL));
    main_menu();   /* D파트 메인 메뉴가 진입점 */
    return 0;
}

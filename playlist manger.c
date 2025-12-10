// playlist_linkedlist_persistent.c
// Singly linked list playlist manager with reliable persistence.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_NAME "playlist.txt"
#define NAME_LEN 200

typedef struct Song {
    int id;
    char title[NAME_LEN];
    struct Song *next;
} Song;

Song *head = NULL;
int nextId = 1;

/* File helpers */
void appendSongToFile(const Song *s) {
    FILE *fp = fopen(FILE_NAME, "a");
    if (!fp) {
        perror("appendSongToFile: fopen");
        return;
    }
    if (fprintf(fp, "%d\t%s\n", s->id, s->title) < 0) {
        perror("appendSongToFile: fprintf");
    }
    fclose(fp);
}

void saveAllToFile() {
    FILE *fp = fopen(FILE_NAME, "w");
    if (!fp) {
        perror("saveAllToFile: fopen");
        return;
    }
    Song *cur = head;
    while (cur) {
        if (fprintf(fp, "%d\t%s\n", cur->id, cur->title) < 0) {
            perror("saveAllToFile: fprintf");
            break;
        }
        cur = cur->next;
    }
    fclose(fp);
}

void loadFromFile() {
    FILE *fp = fopen(FILE_NAME, "r");
    if (!fp) {
        /* No file yet or cannot open — that's okay for first run */
        return;
    }
    char line[NAME_LEN + 32];
    int maxId = 0;
    while (fgets(line, sizeof(line), fp)) {
        char *tab = strchr(line, '\t');
        if (!tab) continue;            // skip malformed lines
        *tab = '\0';
        int id = atoi(line);
        char *title = tab + 1;
        title[strcspn(title, "\r\n")] = '\0';

        Song *s = malloc(sizeof(Song));
        if (!s) { perror("loadFromFile: malloc"); fclose(fp); return; }
        s->id = id;
        strncpy(s->title, title, NAME_LEN - 1);
        s->title[NAME_LEN - 1] = '\0';
        s->next = NULL;

        if (!head) head = s;
        else {
            Song *c = head;
            while (c->next) c = c->next;
            c->next = s;
        }

        if (id > maxId) maxId = id;
    }
    nextId = maxId + 1;
    fclose(fp);
}

/* Linked list ops */
Song* createSong(const char *title) {
    Song *s = malloc(sizeof(Song));
    if (!s) { perror("createSong: malloc"); exit(1); }
    s->id = nextId++;
    strncpy(s->title, title, NAME_LEN - 1);
    s->title[NAME_LEN - 1] = '\0';
    s->next = NULL;
    return s;
}

void addSong(const char *title) {
    Song *s = createSong(title);
    if (!head) head = s;
    else {
        Song *c = head;
        while (c->next) c = c->next;
        c->next = s;
    }
    appendSongToFile(s);        /* persist immediately */
    printf("Added: #%d - %s\n", s->id, s->title);
}

void removeSong(int id) {
    Song *c = head, *p = NULL;
    while (c) {
        if (c->id == id) {
            if (!p) head = c->next;
            else p->next = c->next;
            printf("Removed: #%d - %s\n", c->id, c->title);
            free(c);
            saveAllToFile();      /* rewrite file after deletion */
            return;
        }
        p = c; c = c->next;
    }
    printf("Song #%d not found.\n", id);
}

void showPlaylist() {
    if (!head) { printf("Playlist empty.\n"); return; }
    printf("\n--- Playlist ---\n");
    Song *c = head;
    int idx = 1;
    while (c) {
        printf("%3d) #%d - %s\n", idx, c->id, c->title);
        c = c->next;
        idx++;
    }
}

/* Move a song one position up (towards head). Returns 1 on success */
int moveUp(int id) {
    if (!head || head->id == id) return 0;
    Song *prev = NULL, *cur = head;
    while (cur && cur->id != id) { prev = cur; cur = cur->next; }
    if (!cur || !prev) return 0;

    /* find prev of prev (pp) */
    Song *pp = NULL, *x = head;
    while (x && x != prev) { pp = x; x = x->next; }

    /* unlink cur (prev->next = cur->next) */
    prev->next = cur->next;

    /* insert cur before prev */
    if (!pp) {
        /* prev was head, so cur becomes new head */
        cur->next = head;
        head = cur;
    } else {
        cur->next = pp->next;
        pp->next = cur;
    }

    saveAllToFile();   /* persist reordered list */
    return 1;
}

/* Move a song one position down */
int moveDown(int id) {
    if (!head || head->next == NULL) return 0;
    Song *p = NULL, *c = head;
    while (c && c->id != id) { p = c; c = c->next; }
    if (!c || !c->next) return 0; /* not found or last node */

    Song *n = c->next;
    if (!p) {
        /* c is head -> swap head and n */
        c->next = n->next;
        n->next = c;
        head = n;
    } else {
        p->next = n;
        c->next = n->next;
        n->next = c;
    }

    saveAllToFile();   /* persist reordered list */
    return 1;
}

void clearPlaylist() {
    Song *c = head;
    while (c) { Song *t = c; c = c->next; free(t); }
    head = NULL;
    FILE *fp = fopen(FILE_NAME, "w");
    if (!fp) perror("clearPlaylist: fopen");
    else fclose(fp);
    printf("Playlist cleared.\n");
}

/* free memory on exit */
void freeAll() {
    Song *c = head;
    while (c) { Song *t = c; c = c->next; free(t); }
    head = NULL;
}

int main() {
    loadFromFile();
    printf("Linked-list-only Playlist Manager (persistent)\n");
    int choice, id;
    char title[NAME_LEN];

    while (1) {
        printf("\n1) Add song\n2) Remove song by id\n3) Show playlist\n4) Move up\n5) Move down\n6) Clear playlist\n0) Exit\nChoose: ");
        if (scanf("%d", &choice) != 1) {
            int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
            printf("Invalid input.\n");
            continue;
        }
        getchar(); /* consume newline */

        if (choice == 0) break;

        if (choice == 1) {
            printf("Enter song title: ");
            if (!fgets(title, sizeof(title), stdin)) continue;
            title[strcspn(title, "\r\n")] = '\0';
            if (strlen(title) == 0) { printf("Empty title.\n"); continue; }
            addSong(title);
        } else if (choice == 2) {
            printf("Enter song id: ");
            if (scanf("%d", &id) != 1) { getchar(); printf("Invalid.\n"); continue; }
            getchar();
            removeSong(id);
        } else if (choice == 3) {
            showPlaylist();
        } else if (choice == 4) {
            printf("Enter song id to move up: ");
            if (scanf("%d", &id) != 1) { getchar(); printf("Invalid.\n"); continue; }
            getchar();
            if (moveUp(id)) printf("Moved up.\n"); else printf("Cannot move up (maybe head or not found).\n");
        } else if (choice == 5) {
            printf("Enter song id to move down: ");
            if (scanf("%d", &id) != 1) { getchar(); printf("Invalid.\n"); continue; }
            getchar();
            if (moveDown(id)) printf("Moved down.\n"); else printf("Cannot move down (last or not found).\n");
        } else if (choice == 6) {
            printf("Confirm clear playlist? (y/N): ");
            char c = getchar(); while (getchar()!='\n');
            if (c=='y'||c=='Y') clearPlaylist(); else printf("Cancelled.\n");
        } else {
            printf("Invalid.\n");
        }
    }

    freeAll();
    printf("Exiting.\n");
    return 0;
}


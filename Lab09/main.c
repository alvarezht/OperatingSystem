#include "main.h"

/* Global simulated physical memory */
static Frame ram[NUM_FRAMES];

/* Initialize RAM with a random subset of frames marked as occupied.
 * Ensures at least required_free frames remain free.
 * Uses Fisher-Yates shuffle to avoid collisions.
 * Iteration cap documented in README. */
static void init_ram(int required_free, int *out_free, int *out_occupied)
{
    int indices[NUM_FRAMES];
    int occupied = 0;
    int iterations = 0;

    do {
        for (int i = 0; i < NUM_FRAMES; i++) {
            ram[i].state = FRAME_FREE;
            ram[i].owner = -1;
            indices[i] = i;
        }

        /* Fisher-Yates shuffle to pick a random subset without collisions */
        for (int i = NUM_FRAMES - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            int tmp = indices[i];
            indices[i] = indices[j];
            indices[j] = tmp;
        }

        int target = 10 + rand() % 51; /* range [10, 60] */
        for (int k = 0; k < target; k++) {
            int idx = indices[k];
            ram[idx].state = FRAME_OCCUPIED;
            ram[idx].owner = 0; /* system / pre-existing holes */
        }
        occupied = target;
        iterations++;
    } while ((NUM_FRAMES - occupied) < required_free && iterations < MAX_ITERATIONS);

    if (iterations >= MAX_ITERATIONS) {
        fprintf(stderr, "Warning: reached max iterations (%d) during RAM init.\n",
                MAX_ITERATIONS);
    }

    *out_free = NUM_FRAMES - occupied;
    *out_occupied = occupied;
}

static int count_free(void)
{
    int c = 0;
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (ram[i].state == FRAME_FREE) {
            c++;
        }
    }
    return c;
}

static void print_ram_map(const char *title, int free_count, int occ_count)
{
    printf("\n%s\n", title);
    printf("FREE=%d OCCUPIED=%d\n\n", free_count, occ_count);
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (ram[i].state == FRAME_FREE) {
            printf("%s%3d:F%s%s", COLOR_FREE, i, COLOR_RESET,
                   (i % 10 == 9) ? "\n" : " ");
        } else {
            printf("%s%3d:X%s%s", COLOR_SYS, i, COLOR_RESET,
                   (i % 10 == 9) ? "\n" : " ");
        }
    }
    if (NUM_FRAMES % 10 != 0) {
        printf("\n");
    }
    printf("\n");
}

static void print_ownership_map(void)
{
    printf("FRAME OWNERSHIP MAP (100 frames):\n");
    int free_c = 0, sys_c = 0, p1_c = 0, p2_c = 0;

    for (int i = 0; i < NUM_FRAMES; i++) {
        const char *color;
        char label;
        if (ram[i].state == FRAME_FREE) {
            color = COLOR_FREE;
            label = 'F';
            free_c++;
        } else if (ram[i].owner == 0) {
            color = COLOR_SYS;
            label = 'S';
            sys_c++;
        } else if (ram[i].owner == 1) {
            color = COLOR_P1;
            label = '1';
            p1_c++;
        } else if (ram[i].owner == 2) {
            color = COLOR_P2;
            label = '2';
            p2_c++;
        } else {
            color = COLOR_SYS;
            label = '?';
        }

        printf("%s%3d:%c%s%s", color, i, label, COLOR_RESET,
               (i % 10 == 9) ? "\n" : " ");
    }
    if (NUM_FRAMES % 10 != 0) {
        printf("\n");
    }

    printf("\nLegend: %sF%s=Free  %sS%s=System  %s1%s=Process1  %s2%s=Process2\n",
           COLOR_FREE, COLOR_RESET, COLOR_SYS, COLOR_RESET,
           COLOR_P1, COLOR_RESET, COLOR_P2, COLOR_RESET);
    printf("Counts: FREE=%d SYSTEM=%d PROC1=%d PROC2=%d\n\n",
           free_c, sys_c, p1_c, p2_c);
}

/* Simple linear scan frame allocator.
 * Returns allocated frame index or -1 on failure. */
static int allocate_frame(int owner)
{
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (ram[i].state == FRAME_FREE) {
            ram[i].state = FRAME_OCCUPIED;
            ram[i].owner = owner;
            return i;
        }
    }
    return -1;
}

/* Load a process: allocate one frame per virtual page and build page table.
 * On mid-load failure, rolls back all allocations and leaves table empty. */
static bool load_process(PageTable *pt, int num_pages, int pid)
{
    pt->num_pages = num_pages;
    pt->pid = pid;
    pt->entries = calloc((size_t)num_pages, sizeof(PTE));
    if (pt->entries == NULL) {
        fprintf(stderr, "Error: failed to allocate page table for PID %d\n", pid);
        pt->num_pages = 0;
        return false;
    }

    for (int i = 0; i < num_pages; i++) {
        int pfn = allocate_frame(pid);
        if (pfn < 0) {
            fprintf(stderr, "Error: frame allocation failed for PID %d at VPN %d\n",
                    pid, i);
            /* Rollback: free already allocated frames */
            for (int j = 0; j < i; j++) {
                if (pt->entries[j].valid) {
                    ram[pt->entries[j].pfn].state = FRAME_FREE;
                    ram[pt->entries[j].pfn].owner = -1;
                }
            }
            free(pt->entries);
            pt->entries = NULL;
            pt->num_pages = 0;
            return false;
        }
        pt->entries[i].valid = true;
        pt->entries[i].pfn = pfn;
    }
    return true;
}

/* Unload a process: mark its frames as free and release page table. */
static void unload_process(PageTable *pt)
{
    if (pt == NULL || pt->entries == NULL) {
        return;
    }
    for (int i = 0; i < pt->num_pages; i++) {
        if (pt->entries[i].valid) {
            ram[pt->entries[i].pfn].state = FRAME_FREE;
            ram[pt->entries[i].pfn].owner = -1;
        }
    }
    free(pt->entries);
    pt->entries = NULL;
    pt->num_pages = 0;
    pt->pid = 0;
}

/* Translate a virtual address to physical address using a linear page table.
 * Returns a TransResult code; outputs are written via pointers. */
static TransResult translate(const PageTable *pt, uint32_t va, uint32_t *pa,
                             uint32_t *vpn_out, uint32_t *offset_out, int *pfn_out)
{
    if (va > MAX_VA) {
        return VA_OUT_OF_RANGE;
    }

    uint32_t offset = va & OFFSET_MASK;
    uint32_t vpn = (va >> VPN_SHIFT) & VPN_MASK;

    *offset_out = offset;
    *vpn_out = vpn;

    if (vpn >= (uint32_t)pt->num_pages) {
        return VPN_OUT_OF_RANGE;
    }

    if (!pt->entries[vpn].valid) {
        *pfn_out = -1;
        return PAGE_NOT_MAPPED;
    }

    *pfn_out = pt->entries[vpn].pfn;
    *pa = (uint32_t)(pt->entries[vpn].pfn) * PAGE_SIZE + offset;
    return TRANS_OK;
}

static void print_translation_result(uint32_t va, TransResult res, uint32_t pa,
                                     uint32_t vpn, uint32_t offset, int pfn,
                                     int num_pages)
{
    if (res == VA_OUT_OF_RANGE) {
        printf("VA=%-10lu ERROR=VA_OUT_OF_RANGE\n", (unsigned long)va);
    } else if (res == VPN_OUT_OF_RANGE) {
        printf("VA=0x%04lX (%lu)     ERROR=VPN_OUT_OF_RANGE (vpn=%lu, V=%d)\n",
               (unsigned long)va, (unsigned long)va,
               (unsigned long)vpn, num_pages);
    } else if (res == PAGE_NOT_MAPPED) {
        printf("VA=0x%04lX (%lu)     ERROR=PAGE_NOT_MAPPED (vpn=%lu)\n",
               (unsigned long)va, (unsigned long)va,
               (unsigned long)vpn);
    } else {
        printf("VA=0x%04lX (%lu)     VPN=0x%02lX OFF=0x%02lX  PFN=%-3d  PA=0x%04lX (%lu)\n",
               (unsigned long)va, (unsigned long)va,
               (unsigned long)vpn, (unsigned long)offset,
               pfn,
               (unsigned long)pa, (unsigned long)pa);
    }
}

int main(int argc, char *argv[])
{
    int num_virtual_pages = 8;
    const char *addr_file = "addresses.txt";
    unsigned int seed = (unsigned int)time(NULL);
    bool v_set = false;

    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -s requires an argument\n");
                return EXIT_FAILURE;
            }
            char *endptr;
            long s = strtol(argv[++i], &endptr, 0);
            if (*endptr != '\0') {
                fprintf(stderr, "Error: invalid seed value\n");
                return EXIT_FAILURE;
            }
            seed = (unsigned int)s;
        } else if (strcmp(argv[i], "-v") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -v requires an argument\n");
                return EXIT_FAILURE;
            }
            char *endptr;
            long v = strtol(argv[++i], &endptr, 0);
            if (*endptr != '\0' || v < 1 || v > 256) {
                fprintf(stderr, "Error: V must be an integer between 1 and 256\n");
                return EXIT_FAILURE;
            }
            num_virtual_pages = (int)v;
            v_set = true;
        } else if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -f requires an argument\n");
                return EXIT_FAILURE;
            }
            addr_file = argv[++i];
        } else if (strncmp(argv[i], "seed=", 5) == 0) {
            char *endptr;
            long s = strtol(argv[i] + 5, &endptr, 0);
            if (*endptr != '\0') {
                fprintf(stderr, "Error: invalid seed value\n");
                return EXIT_FAILURE;
            }
            seed = (unsigned int)s;
        } else if (argv[i][0] != '-') {
            if (!v_set) {
                char *endptr;
                long v = strtol(argv[i], &endptr, 0);
                if (*endptr == '\0' && v >= 1 && v <= 256) {
                    num_virtual_pages = (int)v;
                    v_set = true;
                } else {
                    addr_file = argv[i];
                }
            } else {
                addr_file = argv[i];
            }
        } else {
            fprintf(stderr, "Warning: unknown option %s\n", argv[i]);
        }
    }

    if (num_virtual_pages > NUM_FRAMES) {
        fprintf(stderr, "Error: V (%d) exceeds total frame count (%d)\n",
                num_virtual_pages, NUM_FRAMES);
        return EXIT_FAILURE;
    }
    /* RAM init always leaves at least 10 frames occupied, so V cannot exceed 90 */
    if (num_virtual_pages > NUM_FRAMES - 10) {
        fprintf(stderr, "Error: V (%d) too large; max allowed is %d "
                "because RAM init reserves at least 10 occupied frames.\n",
                num_virtual_pages, NUM_FRAMES - 10);
        return EXIT_FAILURE;
    }

    srand(seed);

    int required_free = num_virtual_pages > 10 ? num_virtual_pages : 10;
    int free_count = 0;
    int occ_count = 0;
    init_ram(required_free, &free_count, &occ_count);

    char title[128];
    snprintf(title, sizeof(title),
             "PHYSICAL RAM (100 frames) after random init (seed=%u):", seed);
    print_ram_map(title, free_count, occ_count);

    /* Load process 1 */
    PageTable pt1 = {0};
    if (!load_process(&pt1, num_virtual_pages, 1)) {
        fprintf(stderr, "Fatal: failed to load process 1\n");
        return EXIT_FAILURE;
    }

    /* Determine size for process 2.
     * We choose V2 = min(V, remaining_free / 2) so both processes share
     * the pool without exhausting it, leaving at least half of the
     * post-P1 free frames available for future allocations. */
    int remaining_free = count_free();
    int v2 = num_virtual_pages;
    if (v2 > remaining_free / 2) {
        v2 = remaining_free / 2;
    }

    printf("Process 2 sizing: remaining_free=%d, V=%d -> V2=min(V, remaining_free/2)=%d\n\n",
           remaining_free, num_virtual_pages, v2);

    PageTable pt2 = {0};
    if (v2 > 0) {
        if (!load_process(&pt2, v2, 2)) {
            fprintf(stderr, "Warning: failed to load process 2 (V2=%d)\n", v2);
            v2 = 0;
        }
    }

    print_ownership_map();

    printf("Load process 1 (PID=1): V=%d -> VPN 0..%d mapped to PFNs [",
           pt1.num_pages, pt1.num_pages - 1);
    for (int i = 0; i < pt1.num_pages; i++) {
        printf("%d%s", pt1.entries[i].pfn,
               (i < pt1.num_pages - 1) ? " " : "");
    }
    printf("]\n");

    if (v2 > 0 && pt2.entries != NULL) {
        printf("Load process 2 (PID=2): V=%d -> VPN 0..%d mapped to PFNs [",
               pt2.num_pages, pt2.num_pages - 1);
        for (int i = 0; i < pt2.num_pages; i++) {
            printf("%d%s", pt2.entries[i].pfn,
                   (i < pt2.num_pages - 1) ? " " : "");
        }
        printf("]\n");
    } else {
        printf("Load process 2 (PID=2): not loaded (insufficient free frames)\n");
    }
    printf("\n");

    FILE *fp = fopen(addr_file, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: cannot open address file '%s'\n", addr_file);
        unload_process(&pt1);
        unload_process(&pt2);
        return EXIT_FAILURE;
    }

    printf("Translating addresses for Process 1 (PID=1) from file '%s':\n",
           addr_file);
    char line[256];
    int line_no = 0;
    while (fgets(line, sizeof(line), fp)) {
        line_no++;
        /* Remove trailing newline characters */
        line[strcspn(line, "\r\n")] = '\0';

        /* Skip empty or whitespace-only lines */
        size_t len = strlen(line);
        bool empty = true;
        for (size_t k = 0; k < len; k++) {
            if (line[k] != ' ' && line[k] != '\t') {
                empty = false;
                break;
            }
        }
        if (empty) {
            continue;
        }

        char *endptr = NULL;
        unsigned long val = strtoul(line, &endptr, 0);
        if (endptr != NULL && *endptr != '\0' && *endptr != ' ' && *endptr != '\t') {
            fprintf(stderr, "Warning: invalid number on line %d: '%s' (skipping)\n",
                    line_no, line);
            continue;
        }

        uint32_t va = (uint32_t)val;
        uint32_t pa = 0;
        uint32_t vpn = 0;
        uint32_t offset = 0;
        int pfn = 0;
        TransResult res = translate(&pt1, va, &pa, &vpn, &offset, &pfn);
        print_translation_result(va, res, pa, vpn, offset, pfn, num_virtual_pages);
    }

    fclose(fp);
    unload_process(&pt1);
    unload_process(&pt2);

    return EXIT_SUCCESS;
}

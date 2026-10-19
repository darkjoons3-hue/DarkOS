 #define PORT 0x3F8
#define MAX_CMD_LEN 256

unsigned char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' '
};

char cmd_buffer[MAX_CMD_LEN] = {0};
int cmd_length = 0;

// Глобальные буферы (чтобы не переполнять стек)
unsigned char sector_buf[512];
unsigned char dir_buf[512];
unsigned char file_buf[512];

void* memset(void* s, int c, unsigned int n) {
    unsigned char* p = s; while (n--) *p++ = (unsigned char)c; return s;
}

static inline void outb(unsigned short port, unsigned char val) { __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) ); }
static inline unsigned char inb(unsigned short port) { unsigned char ret; __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) ); return ret; }
static inline unsigned short inw(unsigned short port) { unsigned short ret; __asm__ volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) ); return ret; }

void print_char(char c) { outb(PORT, c); }

void print_string(const char* str) {
    while (*str) { print_char(*str); str++; }
}

void print_int(unsigned int num) {
    if (num == 0) { print_char('0'); return; }
    char buf[12]; int len = 0;
    while (num > 0) { buf[len++] = (num % 10) + '0'; num /= 10; }
    while (len > 0) { print_char(buf[--len]); }
}

// --- Драйвер ATA ---
void read_sector_ata(unsigned int lba, unsigned char* buffer) {
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    int timeout = 100000;
    while ((inb(0x1F7) & 0x80) && timeout--);
    if (timeout <= 0) return;

    outb(0x1F2, 1);
    outb(0x1F3, (unsigned char)lba);
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));
    outb(0x1F7, 0x20);

    timeout = 100000;
    while (1) {
        unsigned char status = inb(0x1F7);
        if (!(status & 0x80) && (status & 0x08)) break;
        if (timeout-- <= 0) return;
    }

    for (int i = 0; i < 256; i++) {
        unsigned short data = inw(0x1F0);
        buffer[i * 2] = (unsigned char)(data & 0xFF);
        buffer[i * 2 + 1] = (unsigned char)((data >> 8) & 0xFF);
    }
}

// --- Структуры FAT32 ---
struct fat32_bpb {
    unsigned char jmp[3]; unsigned char oem[8]; unsigned short bytes_per_sector;
    unsigned char sectors_per_cluster; unsigned short reserved_sectors;
    unsigned char num_fats; unsigned short root_entries; unsigned short total_sectors_16;
    unsigned char media_descriptor; unsigned short fat_size_16; unsigned short sectors_per_track;
    unsigned short num_heads; unsigned int hidden_sectors; unsigned int total_sectors_32;
    unsigned int fat_size_32; unsigned short ext_flags; unsigned short fs_version;
    unsigned int root_cluster; unsigned short fs_info; unsigned short backup_boot_sector;
    unsigned char reserved[12]; unsigned char drive_number; unsigned char reserved1;
    unsigned char boot_signature; unsigned int volume_id; unsigned char volume_label[11];
    unsigned char fs_type[8];
} __attribute__((packed));

struct dir_entry {
    unsigned char name[8]; unsigned char ext[3]; unsigned char attributes;
    unsigned char reserved; unsigned char creation_time_tenth; unsigned short creation_time;
    unsigned short creation_date; unsigned short last_access_date; unsigned short high_cluster;
    unsigned short write_time; unsigned short write_date; unsigned short low_cluster;
    unsigned int file_size;
} __attribute__((packed));

// --- IDT и PIC ---
struct idt_entry { unsigned short base_lo; unsigned short sel; unsigned char always0; unsigned char flags; unsigned short base_hi; } __attribute__((packed));
struct idt_ptr { unsigned short limit; unsigned int base; } __attribute__((packed));
struct idt_entry idt[256]; struct idt_ptr idtp;

void idt_set_gate(unsigned char num, unsigned int base, unsigned short sel, unsigned char flags) {
    idt[num].base_lo = (base & 0xFFFF); idt[num].base_hi = (base >> 16) & 0xFFFF;
    idt[num].sel = sel; idt[num].always0 = 0; idt[num].flags = flags;
}
void init_idt() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1; idtp.base = (unsigned int)&idt;
    memset(&idt, 0, sizeof(struct idt_entry) * 256);
    extern void keyboard_handler_asm();
    idt_set_gate(0x21, (unsigned int)keyboard_handler_asm, 0x08, 0x8E);
    __asm__ volatile ("lidt %0" : : "m" (idtp));
}
void init_pic() {
    outb(0x20, 0x11); outb(0xA0, 0x11); outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02); outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0xFD); outb(0xA1, 0xFF);
}

int is_cat(const char* s) { return (s[0]=='c' && s[1]=='a' && s[2]=='t' && s[3]=='\0'); }

void read_file_cat() {
    print_string("Reading disk...\r\n");
    read_sector_ata(0, sector_buf);
    struct fat32_bpb* bpb = (struct fat32_bpb*)sector_buf;

    print_string("FS: ");
    for(int i=0; i<8; i++) print_char(bpb->fs_type[i]);
    print_string("\r\n");

    unsigned int root_dir_lba = bpb->reserved_sectors + (bpb->num_fats * bpb->fat_size_32);
    if (bpb->root_cluster > 2) {
        root_dir_lba += (bpb->root_cluster - 2) * bpb->sectors_per_cluster;
    }

    read_sector_ata(root_dir_lba, dir_buf);
    struct dir_entry* entry = (struct dir_entry*)dir_buf;

    for (int i = 0; i < 16; i++) {
        if (entry[i].name[0] == 0x00) break;
        if (entry[i].attributes == 0x0F) continue;

        if (entry[i].name[0] == 'T' && entry[i].name[1] == 'E' && entry[i].name[2] == 'S' && entry[i].name[3] == 'T') {
            print_string("File found! Reading...\r\n");

            unsigned int first_cluster = entry[i].low_cluster | (entry[i].high_cluster << 16);
            unsigned int data_region_lba = bpb->reserved_sectors + (bpb->num_fats * bpb->fat_size_32);
            unsigned int file_lba = data_region_lba + (first_cluster - 2) * bpb->sectors_per_cluster;

            read_sector_ata(file_lba, file_buf);

            print_string("Content:\r\n---\r\n");
            unsigned int size = entry[i].file_size;
            if (size > 200) size = 200;
            for (unsigned int j = 0; j < size; j++) print_char(file_buf[j]);
            print_string("\r\n---\r\n");
            return;
        }
    }
    print_string("TEST.TXT not found.\r\n");
}

void keyboard_handler() {
    unsigned char scancode = inb(0x60);
    if (scancode < 0x80) {
        char c = kbd_us[scancode];
        if (c == '\n') {
            print_string("\r\n");
            cmd_buffer[cmd_length] = '\0';
            if (cmd_length > 0) {
                if (is_cat(cmd_buffer)) {
                    read_file_cat();
                } else {
                    print_string("Unknown command.\r\n");
                }
            }
            cmd_length = 0;
            print_string("> ");
        } else if (c == '\b') {
            if (cmd_length > 0) { cmd_length--; print_string("\b \b"); }
        } else if (c != 0) {
            if (cmd_length < MAX_CMD_LEN - 1) { cmd_buffer[cmd_length] = c; cmd_length++; print_char(c); }
        }
    }
    outb(0x20, 0x20);
}

void main() {
    print_string("DarkOS Terminal v1.1\r\n");
    print_string("Type 'cat'.\r\n> ");

    init_idt();
    init_pic();
    __asm__ volatile ("sti");

    while (1) { }
}

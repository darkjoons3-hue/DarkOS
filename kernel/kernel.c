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

void* memcpy(void* dest, const void* src, unsigned int n) {
    unsigned char* d = dest; const unsigned char* s = src;
    while (n--) *d++ = *s++; return dest;
}
void* memset(void* s, int c, unsigned int n) {
    unsigned char* p = s; while (n--) *p++ = (unsigned char)c; return s;
}

static inline void outb(unsigned short port, unsigned char val) { __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) ); }
static inline unsigned char inb(unsigned short port) { unsigned char ret; __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) ); return ret; }
static inline unsigned short inw(unsigned short port) { unsigned short ret; __asm__ volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) ); return ret; }

void print_char(char c) { outb(PORT, c); }
void print_string(const char* str) { for (int i = 0; str[i] != '\0'; i++) print_char(str[i]); }

// --- Драйвер ATA ---
void read_sector_ata(unsigned int lba, unsigned char* buffer) {
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);
    outb(0x1F3, (unsigned char)lba);
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));
    outb(0x1F7, 0x20);
    while ((inb(0x1F7) & 0x08) == 0) { }
    for (int i = 0; i < 256; i++) {
        unsigned short data = inw(0x1F0);
        buffer[i * 2] = (unsigned char)(data & 0xFF);
        buffer[i * 2 + 1] = (unsigned char)((data >> 8) & 0xFF);
    }
}

// --- Структура BPB для FAT32 ---
struct fat32_bpb {
    unsigned char jmp[3];
    unsigned char oem[8];
    unsigned short bytes_per_sector;
    unsigned char sectors_per_cluster;
    unsigned short reserved_sectors;
    unsigned char num_fats;
    // ... остальные поля пропущены для краткости
    unsigned int fat_size_32;       // Смещение 36
    unsigned int root_cluster;      // Смещение 44
    unsigned char fs_type[8];       // Смещение 82 (должно быть "FAT32   ")
} __attribute__((packed));

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

int is_help(const char* s) { return (s[0]=='h' && s[1]=='e' && s[2]=='l' && s[3]=='p' && s[4]=='\0'); }
int is_fat(const char* s) { return (s[0]=='f' && s[1]=='a' && s[2]=='t' && s[3]=='\0'); }

// --- Функция разбора FAT32 ---
void check_fat32() {
    unsigned char sector[512];
    read_sector_ata(0, sector); // Читаем загрузочный сектор (LBA 0)

    // Приводим байты к структуре BPB
    struct fat32_bpb* bpb = (struct fat32_bpb*)sector;

    print_string("Checking File System...\r\n");
    print_string("OEM Name: ");
    for (int i = 0; i < 8; i++) print_char(bpb->oem[i]);
    print_string("\r\n");

    print_string("Bytes per sector: ");
    print_char((bpb->bytes_per_sector >> 8) + '0'); // Очень грубый вывод числа
    print_char((bpb->bytes_per_sector & 0xFF) + '0'); 
    print_string("\r\n");

    print_string("File System Type: ");
    for (int i = 0; i < 8; i++) print_char(bpb->fs_type[i]);
    print_string("\r\n");
}

void keyboard_handler() {
    unsigned char scancode = inb(0x60);
    if (scancode < 0x80) {
        char c = kbd_us[scancode];
        if (c == '\n') {
            print_string("\r\n");
            cmd_buffer[cmd_length] = '\0';
            if (cmd_length > 0) {
                if (is_help(cmd_buffer)) {
                    print_string("Commands:\r\n  help\r\n  fat (check FAT32 volume)\r\n");
                } else if (is_fat(cmd_buffer)) {
                    check_fat32();
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
    print_string("DarkOS Terminal v0.8 (FAT32 Check)\r\n");
    print_string("Type 'help' or 'fat'.\r\n> ");

    init_idt();
    init_pic();
    __asm__ volatile ("sti");

    while (1) { }
}

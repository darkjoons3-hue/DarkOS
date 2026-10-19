#define PORT 0x3F8

// Таблица перевода скан-кодов в ASCII
unsigned char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' '
};

// --- Функции работы с памятью ---
void* memcpy(void* dest, const void* src, unsigned int n) {
    unsigned char* d = dest; const unsigned char* s = src;
    while (n--) *d++ = *s++; return dest;
}
void* memset(void* s, int c, unsigned int n) {
    unsigned char* p = s; while (n--) *p++ = (unsigned char)c; return s;
}

// --- Структуры IDT ---
struct idt_entry { unsigned short base_lo; unsigned short sel; unsigned char always0; unsigned char flags; unsigned short base_hi; } __attribute__((packed));
struct idt_ptr { unsigned short limit; unsigned int base; } __attribute__((packed));
struct idt_entry idt[256]; struct idt_ptr idtp;

// --- I/O ---
static inline void outb(unsigned short port, unsigned char val) { __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) ); }
static inline unsigned char inb(unsigned short port) { unsigned char ret; __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) ); return ret; }

// --- Настройка IDT и PIC ---
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

// --- Обработчик клавиатуры ---
void keyboard_handler() {
    unsigned char scancode = inb(0x60);
    if (scancode < 0x80) {
        char c = kbd_us[scancode];
        if (c != 0) {
            outb(PORT, c); // Печатаем символ напрямую
        }
    }
    outb(0x20, 0x20);
}

void main() {
    // Выводим текст по одной букве (без циклов!)
    outb(PORT, 'S'); outb(PORT, 'T'); outb(PORT, 'A'); outb(PORT, 'R'); outb(PORT, 'T'); outb(PORT, '\r'); outb(PORT, '\n');
    
    init_idt();
    outb(PORT, 'I'); outb(PORT, 'D'); outb(PORT, 'T'); outb(PORT, '\r'); outb(PORT, '\n');
    
    init_pic();
    outb(PORT, 'P'); outb(PORT, 'I'); outb(PORT, 'C'); outb(PORT, '\r'); outb(PORT, '\n');
    
    // Приглашение к вводу
    outb(PORT, '>'); outb(PORT, ' ');

    __asm__ volatile ("sti"); // Включаем прерывания

    while (1) { }
}

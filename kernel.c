void main() {
    // Указатель прямо на видеопамять
    char* vga = (char*) 0xB8000;
    
    // Наше сообщение
    const char* msg = "DarkOS: Kernel written in C is running!";
    
    int i = 0;
    // Просто пишем символы в память
    while (msg[i] != 0) {
        vga[i * 2] = msg[i];       // Символ
        vga[i * 2 + 1] = 0x0A;     // Зеленый цвет
        i++;
    }
    
    // Зависаем
    while (1) { }
}

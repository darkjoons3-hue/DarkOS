void main() {
    // Вставляем ассемблерный код прямо в C
    __asm__ volatile (
        "movw $0x3F8, %dx\n"  // Адрес порта COM1
        "movb $'C', %al\n"    // Символ C
        "outb %al, %dx\n"
        "movb $'!', %al\n"    // Символ !
        "outb %al, %dx\n"
        "movb $'\\r', %al\n"  // Возврат каретки
        "outb %al, %dx\n"
        "movb $'\\n', %al\n"  // Перевод строки
        "outb %al, %dx\n"
    );
    
    // Бесконечный цикл
    while (1) { }
}

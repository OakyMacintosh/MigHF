#include <LiquidCrystal.h>
#include <SPI.h>
#include <SD.h>
#include <USBHost.h>
#include <KeyboardController.h>

// LCD Configuration (adjust pins as needed)
// RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// USB Host and Keyboard
USBHost myusb;
KeyboardController keyboard1(myusb);

// MHF CPU Configuration
#define MEM_SIZE 4096        // Reduced for Arduino memory constraints
#define NUM_REGS 8
#define SD_CS_PIN 10         // SD card chip select pin

// MHF File Header Structure
struct __attribute__((packed)) mhf_header_t {
    char signature[4];       // "MHF\0"
    uint16_t version;
    uint16_t flags;
    uint32_t head_offset;
    uint32_t code_offset;
    uint32_t data_offset;
    uint32_t symbol_offset;
    uint32_t file_size;
};

// CPU State Structure
struct MighfCPU {
    uint8_t regs[NUM_REGS]; 
    uint16_t pc;
    uint8_t memory[MEM_SIZE];
    bool running;
    uint16_t lcd_x, lcd_y;   // LCD cursor position
};

// Global CPU instance
MighfCPU cpu;
bool debug_mode = false;
String input_buffer = "";
bool file_loaded = false;
String current_file = "";

// LCD Display buffer for scrolling
#define LCD_ROWS 4
#define LCD_COLS 20
char lcd_buffer[LCD_ROWS][LCD_COLS + 1];
int lcd_scroll_pos = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    // Initialize LCD
    lcd.begin(LCD_COLS, LCD_ROWS);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MHF CPU Emulator");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");
    
    // Initialize LCD buffer
    for (int i = 0; i < LCD_ROWS; i++) {
        memset(lcd_buffer[i], ' ', LCD_COLS);
        lcd_buffer[i][LCD_COLS] = '\0';
    }
    
    // Initialize USB Host
    myusb.begin();
    
    // Initialize SD card
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD card initialization failed!");
        lcd.setCursor(0, 2);
        lcd.print("SD Init Failed!");
        return;
    }
    
    Serial.println("MHF CPU Emulator v1.0");
    Serial.println("Commands: load <file>, run, step, reset, regs, mem <addr>, help");
    Serial.println("Press 'D' to enter debug mode");
    
    lcd.setCursor(0, 1);
    lcd.print("Ready - SD OK      ");
    lcd.setCursor(0, 2);
    lcd.print("Serial Debug: 115200");
    lcd.setCursor(0, 3);
    lcd.print("USB KB: Waiting    ");
    
    // Initialize CPU
    reset_cpu();
}

void loop() {
    // Process USB events
    myusb.Task();
    
    // Handle serial debug commands
    if (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (input_buffer.length() > 0) {
                process_debug_command(input_buffer);
                input_buffer = "";
            }
            if (debug_mode) Serial.print("mhf> ");
        } else if (c == 8 || c == 127) { // Backspace
            if (input_buffer.length() > 0) {
                input_buffer.remove(input_buffer.length() - 1);
                Serial.print("\b \b");
            }
        } else if (c >= 32) {
            input_buffer += c;
            Serial.print(c);
        }
    }
    
    // Run CPU if not in debug mode and file is loaded
    if (!debug_mode && file_loaded && cpu.running) {
        execute_instruction();
        delay(10); // Slow down execution for visibility
    }
    
    delay(1);
}

void reset_cpu() {
    memset(&cpu, 0, sizeof(cpu));
    cpu.running = false;
    cpu.lcd_x = 0;
    cpu.lcd_y = 0;
    
    // Clear LCD buffer
    for (int i = 0; i < LCD_ROWS; i++) {
        memset(lcd_buffer[i], ' ', LCD_COLS);
        lcd_buffer[i][LCD_COLS] = '\0';
    }
    lcd_scroll_pos = 0;
    refresh_lcd();
}

bool load_mhf_file(const String& filename) {
    File file = SD.open(filename.c_str());
    if (!file) {
        Serial.println("Error: Could not open file");
        return false;
    }
    
    // Read header
    mhf_header_t header;
    if (file.read((uint8_t*)&header, sizeof(header)) != sizeof(header)) {
        Serial.println("Error: Could not read header");
        file.close();
        return false;
    }
    
    // Verify signature
    if (memcmp(header.signature, "MHF", 3) != 0) {
        Serial.println("Error: Invalid MHF signature");
        file.close();
        return false;
    }
    
    // Check if file fits in memory
    if (header.file_size > MEM_SIZE) {
        Serial.println("Error: File too large for memory");
        file.close();
        return false;
    }
    
    // Load file content starting from head_offset
    file.seek(sizeof(header));
    int bytes_to_read = min((uint32_t)(header.file_size - sizeof(header)), (uint32_t)(MEM_SIZE - header.head_offset));
    int bytes_read = file.read(cpu.memory + header.head_offset, bytes_to_read);
    
    file.close();
    
    if (bytes_read <= 0) {
        Serial.println("Error: Could not read file content");
        return false;
    }
    
    // Set entry point
    cpu.pc = header.code_offset;
    cpu.running = true;
    
    Serial.print("Loaded ");
    Serial.print(bytes_read);
    Serial.print(" bytes, entry point: 0x");
    Serial.println(header.code_offset, HEX);
    
    current_file = filename;
    file_loaded = true;
    
    return true;
}

void execute_instruction() {
    if (!cpu.running || cpu.pc >= MEM_SIZE) {
        cpu.running = false;
        return;
    }
    
    uint8_t opcode = cpu.memory[cpu.pc++];
    
    switch (opcode) {
        case 0x00: // NOP
            break;
            
        case 0x01: // HALT
            Serial.println("\n[HALT] CPU halted.");
            lcd_print("[HALT]");
            cpu.running = false;
            break;
            
        case 0x10: { // LOAD reg, imm
            uint8_t reg = cpu.memory[cpu.pc++];
            uint8_t imm = cpu.memory[cpu.pc++];
            if (reg < NUM_REGS) {
                cpu.regs[reg] = imm;
            }
            break;
        }
        
        case 0x20: { // ADD reg, reg
            uint8_t reg1 = cpu.memory[cpu.pc++];
            uint8_t reg2 = cpu.memory[cpu.pc++];
            if (reg1 < NUM_REGS && reg2 < NUM_REGS) {
                cpu.regs[reg1] += cpu.regs[reg2];
            }
            break;
        }
        
        case 0x11: { // STORE reg, addr
            uint8_t reg = cpu.memory[cpu.pc++];
            uint8_t lo = cpu.memory[cpu.pc++];
            uint8_t hi = cpu.memory[cpu.pc++];
            uint16_t addr = (hi << 8) | lo;
            
            if (reg < NUM_REGS && addr < MEM_SIZE) {
                cpu.memory[addr] = cpu.regs[reg];
                
                // Memory-mapped LCD output
                if (addr == 0xFF00) {
                    char ch = (char)cpu.regs[reg];
                    lcd_print_char(ch);
                }
            }
            break;
        }
        
        case 0x30: { // SUB reg, reg
            uint8_t reg1 = cpu.memory[cpu.pc++];
            uint8_t reg2 = cpu.memory[cpu.pc++];
            if (reg1 < NUM_REGS && reg2 < NUM_REGS) {
                cpu.regs[reg1] -= cpu.regs[reg2];
            }
            break;
        }
        
        case 0x40: { // JMP addr
            uint8_t lo = cpu.memory[cpu.pc++];
            uint8_t hi = cpu.memory[cpu.pc++];
            uint16_t addr = (hi << 8) | lo;
            if (addr < MEM_SIZE) {
                cpu.pc = addr;
            }
            break;
        }
        
        default:
            Serial.print("[ERROR] Unknown opcode 0x");
            Serial.print(opcode, HEX);
            Serial.print(" at 0x");
            Serial.println(cpu.pc - 1, HEX);
            lcd_print("[ERROR]");
            cpu.running = false;
            break;
    }
}

void lcd_print_char(char ch) {
    if (ch == '\n' || ch == '\r') {
        cpu.lcd_x = 0;
        cpu.lcd_y++;
        if (cpu.lcd_y >= LCD_ROWS) {
            // Scroll up
            scroll_lcd_up();
            cpu.lcd_y = LCD_ROWS - 1;
        }
    } else if (ch >= 32 && ch <= 126) { // Printable characters
        lcd_buffer[cpu.lcd_y][cpu.lcd_x] = ch;
        cpu.lcd_x++;
        if (cpu.lcd_x >= LCD_COLS) {
            cpu.lcd_x = 0;
            cpu.lcd_y++;
            if (cpu.lcd_y >= LCD_ROWS) {
                scroll_lcd_up();
                cpu.lcd_y = LCD_ROWS - 1;
            }
        }
    }
    refresh_lcd();
}

void lcd_print(const String& text) {
    for (int i = 0; i < text.length(); i++) {
        lcd_print_char(text[i]);
    }
}

void scroll_lcd_up() {
    for (int row = 0; row < LCD_ROWS - 1; row++) {
        strcpy(lcd_buffer[row], lcd_buffer[row + 1]);
    }
    memset(lcd_buffer[LCD_ROWS - 1], ' ', LCD_COLS);
    lcd_buffer[LCD_ROWS - 1][LCD_COLS] = '\0';
}

void refresh_lcd() {
    lcd.clear();
    for (int row = 0; row < LCD_ROWS; row++) {
        lcd.setCursor(0, row);
        lcd.print(lcd_buffer[row]);
    }
}

void process_debug_command(const String& cmd) {
    String command = cmd;
    command.trim();
    command.toLowerCase();
    
    if (command == "help") {
        Serial.println("Commands:");
        Serial.println("  load <filename> - Load MHF file from SD card");
        Serial.println("  run            - Start/resume execution");
        Serial.println("  stop           - Stop execution");
        Serial.println("  step           - Execute one instruction");
        Serial.println("  reset          - Reset CPU state");
        Serial.println("  regs           - Show register values");
        Serial.println("  mem <addr>     - Show memory at address (hex)");
        Serial.println("  debug          - Toggle debug mode");
        Serial.println("  lcd            - Clear LCD");
        Serial.println("  files          - List SD card files");
        
    } else if (command.startsWith("load ")) {
        String filename = command.substring(5);
        filename.trim();
        reset_cpu();
        if (load_mhf_file(filename)) {
            Serial.println("File loaded successfully");
        }
        
    } else if (command == "run") {
        if (file_loaded) {
            cpu.running = true;
            debug_mode = false;
            Serial.println("CPU running...");
        } else {
            Serial.println("No file loaded");
        }
        
    } else if (command == "stop") {
        cpu.running = false;
        Serial.println("CPU stopped");
        
    } else if (command == "step") {
        if (file_loaded && !cpu.running) {
            cpu.running = true;
            execute_instruction();
            cpu.running = false;
            Serial.print("PC: 0x");
            Serial.println(cpu.pc, HEX);
        } else if (!file_loaded) {
            Serial.println("No file loaded");
        } else {
            Serial.println("CPU is running, use 'stop' first");
        }
        
    } else if (command == "reset") {
        reset_cpu();
        Serial.println("CPU reset");
        
    } else if (command == "regs") {
        Serial.println("Registers:");
        for (int i = 0; i < NUM_REGS; i++) {
            Serial.print("  r");
            Serial.print(i);
            Serial.print(" = ");
            Serial.print(cpu.regs[i]);
            Serial.print(" (0x");
            Serial.print(cpu.regs[i], HEX);
            Serial.println(")");
        }
        Serial.print("  PC = 0x");
        Serial.println(cpu.pc, HEX);
        
    } else if (command.startsWith("mem ")) {
        String addr_str = command.substring(4);
        addr_str.trim();
        uint16_t addr = strtol(addr_str.c_str(), NULL, 16);
        if (addr < MEM_SIZE) {
            Serial.print("Memory[0x");
            Serial.print(addr, HEX);
            Serial.print("] = ");
            Serial.print(cpu.memory[addr]);
            Serial.print(" (0x");
            Serial.print(cpu.memory[addr], HEX);
            Serial.println(")");
        } else {
            Serial.println("Address out of range");
        }
        
    } else if (command == "debug") {
        debug_mode = !debug_mode;
        if (debug_mode) {
            Serial.println("Debug mode ON");
            cpu.running = false;
        } else {
            Serial.println("Debug mode OFF");
        }
        
    } else if (command == "lcd") {
        reset_cpu();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("LCD Cleared");
        Serial.println("LCD cleared");
        
    } else if (command == "files") {
        Serial.println("SD Card Files:");
        File root = SD.open("/");
        while (true) {
            File entry = root.openNextFile();
            if (!entry) break;
            if (!entry.isDirectory()) {
                Serial.print("  ");
                Serial.print(entry.name());
                Serial.print(" (");
                Serial.print(entry.size());
                Serial.println(" bytes)");
            }
            entry.close();
        }
        root.close();
        
    } else if (command.length() > 0) {
        Serial.println("Unknown command. Type 'help' for available commands.");
    }
}

// USB Keyboard event handlers
void OnPress(int key) {
    Serial.print("Key pressed: ");
    Serial.println(key);
    
    // Handle special keys for debug mode
    if (key == 68 || key == 100) { // 'D' or 'd'
        debug_mode = true;
        cpu.running = false;
        Serial.println("\nEntering debug mode...");
        Serial.print("mhf> ");
    }
    
    // You can add more keyboard handling here
    // For example, map keys to CPU memory locations
}

void OnRelease(int key) {
    // Handle key releases if needed
}
